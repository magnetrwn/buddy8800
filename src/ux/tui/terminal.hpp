#ifndef BUDDY8800_SRC_UX_TUI_TERMINAL_HPP_
#define BUDDY8800_SRC_UX_TUI_TERMINAL_HPP_

#include <curses.h>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace buddy8800::ux::tui {

/// @brief Own an ncurses session and restore terminal state on destruction.
class terminal_session {
    SCREEN* screen = nullptr;

public:
    terminal_session() {
        const char* term = std::getenv("TERM");
        if (!term || !*term || std::string(term) == "dumb")
            throw std::runtime_error(
                "TUI requires a usable TERM; use a terminal or build with DISABLE_TRACE=ON");
        screen = newterm(nullptr, stdout, stdin);
        if (!screen)
            throw std::runtime_error("Could not initialize ncurses");
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        set_escdelay(25);
    }

    terminal_session(const terminal_session&) = delete;

    terminal_session& operator=(const terminal_session&) = delete;

    ~terminal_session() {
        endwin();
        delscreen(screen);
    }
};

}

#endif
