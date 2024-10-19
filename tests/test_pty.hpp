#include <catch2/catch_test_macros.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <random>

#include "pty.hpp"

constexpr static usize BUFFER_SIZE = 1024;
constexpr static usize ROUNDS = 200;

inline const char* random_string(usize length) {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<> distribution(1, 127);
    static std::string str;

    if (str.capacity() < length)
        str.reserve(length);

    str.clear();
    for (std::size_t i = 0; i < length; ++i)
        str += static_cast<char>(distribution(generator));

    return str.c_str();
}


TEST_CASE("Pseudo-terminal operation test", "[pty]") {
    pty pty_instance;
    char buffer[BUFFER_SIZE];

    REQUIRE_NOTHROW(pty_instance.open());

    fd slave_fd = open(pty_instance.name(), O_RDWR | O_NOCTTY);
    REQUIRE(slave_fd >= 0);

    alarm(3);

    SECTION("Send from slave, receive from master. This will test pty::recv().") {
        for (usize i = 0; i < ROUNDS; ++i) {
            const char* message = random_string(BUFFER_SIZE - 1);

            isize bytes_written = write(slave_fd, message, std::strlen(message));
            REQUIRE(bytes_written == static_cast<isize>(std::strlen(message)));

            pty_instance.recv(buffer, BUFFER_SIZE);
            REQUIRE(std::strcmp(buffer, message) == 0);
        }
    }

    SECTION("Send from slave, receive from master. This will test pty::getch().") {
        for (usize i = 0; i < ROUNDS; ++i) {
            const char* message = random_string(1);

            isize bytes_written = write(slave_fd, message, 1);
            REQUIRE(bytes_written == 1);
            REQUIRE(pty_instance.getch() == message[0]);
        }
    }

    SECTION("Send from master, receive from slave. This will test pty::send().") {
        for (usize i = 0; i < ROUNDS; ++i) {
            const char* message = random_string(BUFFER_SIZE - 1);

            REQUIRE_NOTHROW(pty_instance.send(message));

            isize bytes_read = read(slave_fd, buffer, BUFFER_SIZE - 1);
            buffer[bytes_read] = '\0';
            REQUIRE(bytes_read > 0);
            REQUIRE(std::strcmp(buffer, message) == 0);
        }
    }

    close(slave_fd);
}
