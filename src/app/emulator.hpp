#ifndef BUDDY8800_SRC_APP_EMULATOR_HPP_
#define BUDDY8800_SRC_APP_EMULATOR_HPP_

#include <csignal>
#include <functional>
#include <ostream>
#include <utility>
#include <vector>
#include "core/cpu/cpu.hpp"
#include "app/system_config.hpp"

/**
 * @brief Owns and executes a configured machine.
 *
 * Terminal selection, paths and command lines belong to the application/frontend.
 */
class emulator {
    system_config conf;
    bus& cardbus;
    cpu<bus&> processor;

public:
    /// @brief Construct the machine from a configuration file.
    explicit emulator(const char* config_filename);

    /// @name Loading and execution
    /// \{

    /// @brief Load bytes and optionally install a reset jump to their address.
    void load_program(const std::vector<u8>& bytes, u16 address, bool reset_vector);

    /// @brief Set the configured initial program counter.
    void start();

    /// @brief Execute one instruction and service a pending bus interrupt.
    template <bool report_trace = true>
    void step() {
        if (processor.is_halted())
            return;
        processor.step<report_trace>();
        if (cardbus.is_irq())
            processor.interrupt<report_trace>(cardbus.get_irq());
    }

    /// @brief Execute until halted or a stop is requested.
    void run(const volatile std::sig_atomic_t& stop);

    /// \}
    /// @name Observation
    /// \{

    /// @brief Report whether the processor is halted.
    bool halted() const { return processor.is_halted(); }

    /// @brief Copy the current processor registers and flags.
    cpu_state state() const { return processor.save_state(); }

    /// @brief Replace the instruction trace observer.
    void trace(std::function<void(u16, const std::array<u8, 3>&, usize)> observer) {
        processor.set_trace_observer(std::move(observer));
    }

    /// @brief Describe installed devices without servicing their I/O.
    auto devices() const { return cardbus.describe(); }

    /// @brief Bind diagnostic output and return the previous stream.
    /// @deprecated The frontend retains ownership of this diagnostic-only stream.
    std::ostream& diagnostic_stream(std::ostream& stream) { return processor.set_pseudo_bdos_stream(stream); }
};

#endif
