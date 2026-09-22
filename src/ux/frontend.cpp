#include "ux/frontend.hpp"
#include "app/emulator.hpp"
#include <unistd.h>

namespace buddy8800::ux {

bool tui_available() {
#ifdef DISABLE_TRACE
    return false;
#else
    return true;
#endif
}

int run_frontend(emulator& machine, const volatile std::sig_atomic_t& stop) {
#ifndef DISABLE_TRACE
    if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))
        return run_tui(machine, stop);
#endif
    return run_plain(machine, stop);
}

}
