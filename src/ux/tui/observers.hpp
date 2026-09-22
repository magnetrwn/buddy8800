#ifndef BUDDY8800_SRC_UX_TUI_OBSERVERS_HPP_
#define BUDDY8800_SRC_UX_TUI_OBSERVERS_HPP_

#include "app/emulator.hpp"
#include "ux/trace_view.hpp"

namespace buddy8800::ux::tui {

/**
 * @brief Bind TUI tracing and diagnostic output for a scoped lifetime.
 *
 * Remove captured references and restore diagnostic output before the borrowed
 * history and output are destroyed, including during exception unwinding.
 */
class observers {
    emulator& machine;
    std::ostream* previous_output;

public:
    observers(emulator& machine, trace_history& history, std::ostream& output) : machine(machine) {
        machine.trace([&history](u16 address, const std::array<u8, 3>& bytes, usize size) {
            history.append(instruction_line(address, bytes, size));
        });
        previous_output = &machine.diagnostic_stream(output);
    }

    observers(const observers&) = delete;

    observers& operator=(const observers&) = delete;

    ~observers() {
        machine.trace({});
        machine.diagnostic_stream(*previous_output);
    }
};

}

#endif
