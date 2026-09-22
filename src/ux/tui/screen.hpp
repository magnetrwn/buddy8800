#ifndef BUDDY8800_SRC_UX_TUI_SCREEN_HPP_
#define BUDDY8800_SRC_UX_TUI_SCREEN_HPP_

#include <curses.h>
#include <memory>
#include <vector>
#include "core/cpu/cpu_state.hpp"
#include "core/bus/device_description.hpp"
#include "ux/trace_view.hpp"

namespace buddy8800::ux::tui {

/// @brief Retain viewport positions and execution controls between frames.
struct view_state {
    usize card_offset = 0;
    usize trace_offset = 0;
    bool running = false;
    bool reporting = true;
    cpu_state previous;
};

/// @brief User actions understood by the TUI controller.
enum class command { NONE, QUIT, TOGGLE_RUN, TOGGLE_REPORTING, STEP, UP, DOWN, PAGE_UP, PAGE_DOWN };

/// @brief Decode a terminal key, blocking only when execution is paused.
command read_command(bool running);

/// @brief Own and render the machine, trace and register windows.
class screen {
    using window = std::unique_ptr<WINDOW, decltype(&delwin)>;
    window machine{nullptr, delwin};
    window instructions{nullptr, delwin};
    window registers{nullptr, delwin};
    int rows = 0;
    int columns = 0;
    static constexpr int MINIMUM_ROWS = 18;
    static constexpr int MINIMUM_COLUMNS = 76;

public:
    /// @brief Report whether all display windows are available.
    bool usable() const { return machine && instructions && registers; }

    /// @brief Return the visible trace line count.
    usize trace_page_size() const { return usable() ? getmaxy(instructions.get()) - 2 : 0; }

    /// @brief Render a snapshot, rebuilding windows after terminal resizing.
    void draw(const std::vector<device_description>& devices, const cpu_state& snapshot, bool halted,
              const trace_history& history, view_state& ui);
};

}

#endif
