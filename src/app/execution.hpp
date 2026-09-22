#ifndef BUDDY8800_SRC_APP_EXECUTION_HPP_
#define BUDDY8800_SRC_APP_EXECUTION_HPP_

#include <chrono>
#include <csignal>
class emulator;

namespace buddy8800::app {

// UI scheduling policy, not guest CPU timing.
inline constexpr auto FRAME_INTERVAL = std::chrono::milliseconds(33);
inline constexpr auto TRACED_BUDGET = std::chrono::milliseconds(8);
inline constexpr int TRACED_REST_MS = 25;

/// @brief Execute one UI frame with bounded tracing and periodic stop checks.
void execute_frame(emulator& machine, bool reporting, const volatile std::sig_atomic_t& stop);

}

#endif
