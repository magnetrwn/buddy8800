#include "trace_view.hpp"

TEST_CASE("Instruction observer captures operands and wrapping addresses without changing execution", "[trace]") {
    std::array<u8, 65536> memory{};
    memory[0xFFFF] = 0x21; // LXI H,1234 across the address-space boundary
    memory[0] = 0x34;
    memory[1] = 0x12;
    memory[2] = 0x3E; // MVI A,5A
    memory[3] = 0x5A;
    memory[4] = 0x3C; // INR A
    memory[5] = 0x76; // HLT
    cpu_t traced(memory), plain(memory);
    traced.set_pc(0xFFFF);
    plain.set_pc(0xFFFF);
    std::vector<std::string> records;
    traced.set_trace_observer([&](u16 pc, const std::array<u8, 3>& bytes, usize size) {
        records.push_back(instruction_line(pc, bytes, size));
    });
    traced.step(4);
    plain.step(4);
    REQUIRE(traced.save_state().registers == plain.save_state().registers);
    REQUIRE(traced.is_halted());
    REQUIRE(traced.save_state().HL() == 0x1234);
    REQUIRE(traced.save_state().A() == 0x5B);
    REQUIRE(records == std::vector<std::string>{
        "FFFF  21 34 12  LXI H, D16", "0002  3E 5A     MVI A, D8",
        "0004  3C        INR A", "0005  76        HLT"});
    traced.set_trace_observer({});
}

TEST_CASE("Trace history has a fixed memory bound and keeps the latest instructions", "[trace]") {
    trace_history history;
    for (usize i = 0; i < trace_history::capacity + 10; ++i) history.append(std::to_string(i));
    REQUIRE(history.size() == trace_history::capacity);
    REQUIRE(history[0] == "10");
    REQUIRE(history[history.size() - 1] == std::to_string(trace_history::capacity + 9));
}

TEST_CASE("Untraced execution skips operand inspection and resumes the same CPU", "[trace]") {
    struct counted_ram : ram_card {
        usize reads = 0;
        counted_ram() : ram_card(0, 65536, 0) {}
        u8 read(u16 address) override { ++reads; return ram_card::read(address); }
    } ram;
    bus cards;
    cards.insert(&ram, 0);
    cpu<bus&> processor(cards);
    const std::array<u8, 6> code{0x21, 0x34, 0x12, 0x3E, 0x5A, 0x76};
    processor.load(code.begin(), code.end());
    usize records = 0;
    processor.set_trace_observer([&](u16, const std::array<u8, 3>&, usize) { ++records; });
    processor.step<false>();
    REQUIRE(records == 0);
    REQUIRE(ram.reads == 3); // Only actual opcode/operand fetches; no tracing reads.
    REQUIRE(processor.save_state().HL() == 0x1234);
    processor.step();
    REQUIRE(records == 1);
    REQUIRE(processor.save_state().A() == 0x5A);
    processor.step<false>();
    REQUIRE(records == 1);
    REQUIRE(processor.is_halted());
    REQUIRE(processor.save_state().PC() == 6);
}
