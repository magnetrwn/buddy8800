#include "app/configuration.hpp"
#include "core/cards/front_panel.hpp"
#include <fstream>

TEST_CASE("Front panel switches are fixed and decode only I/O port bits", "[front_panel]") {
    front_panel panel(0xFF, 0x10);
    ram_card memory(0, 65536, 0x42);
    bus cards;
    cards.insert(&memory, 0);
    cards.insert(&panel, 1);
    REQUIRE(cards.read(0xFF) == 0x42);
    REQUIRE(cards.read(0xFF, true) == 0x10);
    REQUIRE(cards.read(0x12FF, true) == 0x10);
    REQUIRE_FALSE(panel.in_range(0xFE));
    cards.write(0xFF, 0xFF, true);
    cards.write_force(0xFF, 0, true);
    panel.raise_irq(true);
    cards.clear();
    REQUIRE(cards.read(0xFF, true) == 0x10);
    REQUIRE_FALSE(panel.is_irq());
    REQUIRE(panel.identify().name == "front panel");
    REQUIRE(panel.identify().adr_range == 1);
    front_panel duplicate;
    REQUIRE_THROWS(cards.insert(&duplicate, 2));
    front_panel relocated(0x20, 0xA5);
    cards.insert(&relocated, 2);
    REQUIRE(cards.read(0xAB20, true) == 0xA5);
    REQUIRE_THROWS_AS(front_panel(256), std::out_of_range);
}

struct front_panel_config_file {
    std::filesystem::path path;
    front_panel_config_file() {
        char name[] = "/tmp/buddy8800-front-panel-XXXXXX";
        const int fd = ::mkstemp(name);
        REQUIRE(fd >= 0);
        ::close(fd);
        path = name;
    }
    ~front_panel_config_file() { std::filesystem::remove(path); }
    buddy8800::app::machine_config parse(const std::string& fields) {
        {
            std::ofstream file(path);
            file << "[emulator]\n[[card]]\nslot = 0\ntype = \"front_panel\"\n" << fields;
        }
        return buddy8800::app::read_config(path);
    }
};

TEST_CASE("Front panel TOML defaults and validation", "[front_panel][config]") {
    front_panel_config_file file;
    const auto defaults = file.parse("");
    REQUIRE(defaults.cards.at(0).at == 0xFF);
    REQUIRE(defaults.cards.at(0).switches == 0);
    system_config default_machine(defaults);
    REQUIRE(default_machine.get_bus().read(0xFF, true) == 0);
    system_config machine(file.parse("at = 0x20\nswitches = 0xA5\n"));
    REQUIRE(machine.get_bus().read(0x20, true) == 0xA5);
    for (const auto* fields : {"at = -1", "at = 256", "switches = -1", "switches = 256",
                               "at = 'bad'", "switches = true", "range = 0", "load = ''"}) {
        INFO(fields);
        REQUIRE_THROWS(file.parse(fields));
    }
}

TEST_CASE("MITS 8K BASIC runs through 88-2SIO after ALTMON", "[front_panel][basic]") {
    system_config config(BUDDY8800_BASIC_CONFIG);
    auto& cards = config.get_bus();
    cpu<bus&> processor(cards);
    processor.set_pc(config.get_start_pc());
    auto* serial = dynamic_cast<serial_card*>(config.get_cards_vec().at(4).get());
    REQUIRE(serial != nullptr);
    slave_connection client(serial->pty_name());
    auto expect = [&](const std::string& expected) {
        std::string output;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (output.find(expected) == std::string::npos && std::chrono::steady_clock::now() < deadline) {
            processor.step<false>(10000);
            char buffer[1024];
            ssize_t count;
            while ((count = ::read(client.descriptor, buffer, sizeof(buffer))) > 0)
                for (ssize_t i = 0; i < count; ++i)
                    output.push_back(buffer[i] & 0x7F); // BASIC may set the parity bit.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        INFO("Expected: " << expected << " Output: " << output);
        REQUIRE(output.find(expected) != std::string::npos);
    };
    expect("*");
    client.send("G0000");
    expect("MEMORY SIZE?");
    client.send("\r");
    expect("TERMINAL WIDTH?");
    client.send("\r");
    expect("WANT SIN-COS-TAN-ATN");
    client.send("N\r");
    expect("OK");
    client.send("PRINT 2+2\r");
    expect(" 4");
    REQUIRE(cards.read(0xFF, true) == 0x10);
    REQUIRE_FALSE(processor.is_halted());
}
