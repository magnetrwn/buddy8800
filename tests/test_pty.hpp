#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <poll.h>
#include "pty.hpp"
#include "card.hpp"

struct slave_connection {
    int descriptor;
    explicit slave_connection(const char* name)
        : descriptor(::open(name, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC)) {
        REQUIRE(descriptor >= 0);
    }
    ~slave_connection() { ::close(descriptor); }
    void send(const std::string& bytes) {
        REQUIRE(::write(descriptor, bytes.data(), bytes.size()) == static_cast<ssize_t>(bytes.size()));
    }
    std::string receive(usize size) {
        std::string result;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (result.size() < size && std::chrono::steady_clock::now() < deadline) {
            pollfd event{descriptor, POLLIN, 0};
            if (::poll(&event, 1, 10) > 0) {
                char buffer[256];
                const auto count = ::read(descriptor, buffer, std::min(sizeof(buffer), size - result.size()));
                if (count > 0) result.append(buffer, count);
            }
        }
        REQUIRE(result.size() == size);
        return result;
    }
};
template <typename Predicate>
void eventually(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    FAIL("Timed out waiting for serial input");
}

TEST_CASE("PTY preserves binary data and survives client reconnect", "[pty]") {
    pty terminal;
    terminal.open();
    REQUIRE_THROWS(terminal.open());
    REQUIRE_FALSE(terminal.poll());
    terminal.send("startup");
    std::string bytes;
    for (int i = 0; i < 256; ++i) bytes.push_back(static_cast<char>(i));
    {
        slave_connection client(terminal.name());
        REQUIRE(client.receive(7) == "startup");
        client.send(bytes);
        for (unsigned char expected : bytes) {
            u8 actual = 0;
            eventually([&] { return terminal.try_getch(actual); });
            REQUIRE(actual == expected);
        }
        REQUIRE_FALSE(terminal.poll());
        terminal.send(bytes.data(), bytes.size());
        REQUIRE(client.receive(bytes.size()) == bytes);
        terminal.set_baud_rate(19200);
        termios settings{};
        REQUIRE(tcgetattr(client.descriptor, &settings) == 0);
        REQUIRE(cfgetospeed(&settings) == B19200);
    }
    REQUIRE_FALSE(terminal.poll());
    terminal.send("reconnect");
    {
        slave_connection client(terminal.name());
        REQUIRE(client.receive(9) == "reconnect");
    }
    terminal.close();
    terminal.close();
    REQUIRE(std::string(terminal.name()).empty());
    REQUIRE_THROWS(terminal.poll());
    terminal.open();
    REQUIRE_FALSE(terminal.poll());
}

TEST_CASE("PTY recv stops at terminator without consuming the next command", "[pty]") {
    pty terminal;
    terminal.open();
    slave_connection client(terminal.name());
    client.send("one\rtwo\r");
    eventually([&] { return terminal.poll(); });
    char buffer[32];
    terminal.recv(buffer, sizeof(buffer));
    REQUIRE(std::string(buffer) == "one\r");
    terminal.set_echo_received_back(true);
    terminal.recv(buffer, sizeof(buffer));
    REQUIRE(std::string(buffer) == "two\r");
    REQUIRE(client.receive(4) == "two\r");
    REQUIRE_THROWS_AS(terminal.recv(buffer, 0), std::invalid_argument);
}

TEST_CASE("Serial registers consume input once and handle control reset", "[serial]") {
    serial_card serial(0x10);
    slave_connection client(serial.pty_name());
    REQUIRE(serial.in_range(0x1010));
    REQUIRE(serial.in_range(0x1111));
    REQUIRE_FALSE(serial.in_range(0x12));
    REQUIRE(serial.read(0x10) == 2);
    // ALTMON's master reset followed by 8N2 must not fail Linux termios.
    serial.write(0x10, 3);
    serial.write(0x10, 0x11);
    client.send("AB");
    eventually([&] { return serial.read(0x10) & 1; });
    REQUIRE((serial.read(0x10) & 1) == 1);
    REQUIRE(serial.read(0x1111) == 'A');
    eventually([&] { return serial.read(0x10) & 1; });
    REQUIRE(serial.read(0x11) == 'B');
    REQUIRE(serial.read(0x10) == 2);
    serial.write(0x1111, 0xFF);
    REQUIRE(client.receive(1) == std::string(1, static_cast<char>(0xFF)));
    client.send("C");
    eventually([&] { return serial.read(0x10) & 1; });
    serial.write(0x10, 3);
    REQUIRE(serial.read(0x10) == 2);
    REQUIRE_FALSE(serial.is_irq());
    REQUIRE_THROWS_AS(serial_card(0xFF), std::out_of_range);
}

TEST_CASE("Serial transmitter applies backpressure without blocking", "[serial]") {
    serial_card serial(0x10);
    slave_connection client(serial.pty_name());
    usize sent = 0;
    while ((serial.read(0x10) & 2) && sent < 1024 * 1024) {
        serial.write(0x11, 'X');
        ++sent;
    }
    REQUIRE(sent < 1024 * 1024);
    REQUIRE((serial.read(0x10) & 2) == 0);
    // Drain the OS queue and let a status read retry the pending UART byte.
    usize received = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (received < sent && std::chrono::steady_clock::now() < deadline) {
        char buffer[4096];
        const auto count = ::read(client.descriptor, buffer, sizeof(buffer));
        if (count > 0) {
            for (ssize_t i = 0; i < count; ++i) REQUIRE(buffer[i] == 'X');
            received += count;
        }
        serial.read(0x10);
    }
    REQUIRE(received == sent);
    REQUIRE(serial.read(0x10) == 2);
}
