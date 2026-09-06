#include "sysconf.hpp"

TEST_CASE("ALTMON executes commands on the card bus", "[monitor]") {
    system_config config(BUDDY8800_MONITOR_CONFIG);
    auto& cardbus = config.get_bus();
    cpu<bus&> processor(cardbus);
    processor.set_pc(config.get_start_pc());
    auto* serial = dynamic_cast<serial_card*>(config.get_cards_vec()[1].get());
    REQUIRE(serial != nullptr);
    slave_connection client(serial->pty_name());
    processor.step(10000);
    client.send("K200020035A");
    processor.step(100000);
    const auto state = processor.save_state();
    INFO("PC=" << util::to_hex_s(state.PC()) << " A=" << util::to_hex_s(state.A()));
    REQUIRE_FALSE(processor.is_halted());
    REQUIRE(cardbus.read(0x2000) == 0x5A);
    REQUIRE(cardbus.read(0x2003) == 0x5A);
}
