#include "app/execution.hpp"
#include "app/emulator.hpp"

namespace buddy8800::app {

void execute_frame(emulator& machine, bool reporting, const volatile std::sig_atomic_t& stop) {
    if (!reporting) {
        const auto deadline = std::chrono::steady_clock::now() + FRAME_INTERVAL;
        do {
            for (usize count = 0; count < 256 && !stop && !machine.halted(); ++count)
                machine.step<false>();
        } while (!stop && !machine.halted() && std::chrono::steady_clock::now() < deadline);
    } else {
        const auto deadline = std::chrono::steady_clock::now() + TRACED_BUDGET;
        for (usize count = 0;
             count < 4096 && !stop && !machine.halted() && std::chrono::steady_clock::now() < deadline;
             ++count)
            machine.step();
    }
}

}
