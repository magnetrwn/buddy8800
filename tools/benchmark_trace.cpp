// Optional diagnostic benchmark, not a timing-sensitive regression test.
// Build/run commands and interpretation are documented in performance.md.
#include "cpu.hpp"
#include "trace_view.hpp"
#include <algorithm>
#include <chrono>
#include <functional>

static std::uint64_t checksum = 0;
static trace_history history;
static void count_record(u16 pc, const std::array<u8, 3>& bytes, usize size) {
    checksum += pc + bytes[0] + size;
}
static void format_record(u16 pc, const std::array<u8, 3>& bytes, usize size) {
    history.append(instruction_line(pc, bytes, size));
}

template <bool trace>
void execute_loop(cpu<bus&>& processor, bus& cards, usize count) {
    for (usize i = 0; i < count; ++i) {
#ifdef BUDDY8800_BENCH_BASELINE
        processor.step();
        if (cards.is_irq()) processor.interrupt(cards.get_irq());
#else
        processor.step<trace>();
        if (cards.is_irq()) processor.interrupt<trace>(cards.get_irq());
#endif
    }
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "fast";
    const usize count = argc > 2 ? std::stoul(argv[2]) : 2000000;
    std::vector<double> durations;
    for (int round = 0; round < 5; ++round) {
        ram_card ram(0, 65536, 0);
        bus cards;
        cards.insert(&ram, 0);
        const std::array<u8, 15> program{0x3E, 0x5A, 0x21, 0x00, 0x20,
            0x77, 0x34, 0x7E, 0xA8, 0x23, 0x0D, 0xC3, 0, 0, 0};
        cpu<bus&> processor(cards);
        processor.load(program.begin(), program.end());
        if (mode == "count") processor.set_trace_observer(count_record);
        if (mode == "format") processor.set_trace_observer(format_record);
        const std::array<u8, 3> bytes{0x21, 0, 0x20};
        std::function<void(u16, const std::array<u8, 3>&, usize)> callback = format_record;
        const auto begin = std::chrono::steady_clock::now();
        if (mode == "direct-format")
            for (usize i = 0; i < count; ++i) format_record(static_cast<u16>(i), bytes, 3);
        else if (mode == "function-format")
            for (usize i = 0; i < count; ++i) callback(static_cast<u16>(i), bytes, 3);
        else if (mode == "fast") execute_loop<false>(processor, cards, count);
        else execute_loop<true>(processor, cards, count);
        durations.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count());
        checksum += processor.save_state().AF() + processor.save_state().PC() + cards.read(0x2000);
    }
    std::sort(durations.begin(), durations.end());
    std::cout << mode << ": " << count / durations[2] / 1e6 << " million operations/s, median "
              << durations[2] << " s; checksum " << checksum << "; history " << history.size() << '\n';
}
