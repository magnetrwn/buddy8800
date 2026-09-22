#ifndef BUDDY8800_SRC_UX_FRONTEND_HPP_
#define BUDDY8800_SRC_UX_FRONTEND_HPP_

#include <csignal>
class emulator;

namespace buddy8800::ux {

/// @brief Report whether this build includes the terminal UI.
bool tui_available();

/// @brief Run the frontend selected by this build.
int run_frontend(emulator& machine, const volatile std::sig_atomic_t& stop);

/// @brief Run the machine with plain terminal output until stopped.
int run_plain(emulator& machine, const volatile std::sig_atomic_t& stop);

/// @brief Run the terminal UI in ncurses builds.
int run_tui(emulator& machine, const volatile std::sig_atomic_t& stop);

}

#endif
