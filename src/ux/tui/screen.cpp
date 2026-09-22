#include "ux/tui/screen.hpp"
#include "ux/machine_view.hpp"
#include "util/format.hpp"
#include "app/execution.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace buddy8800::ux::tui {

namespace {

using format::hex_value;

void put(WINDOW* target, int row, const std::string& text) {
    if (row <= 0 || row >= getmaxy(target) - 1)
        return;
    mvwaddnstr(target, row, 2, text.c_str(), std::max(0, getmaxx(target) - 4));
}

void frame(WINDOW* target, const std::string& title) {
    werase(target);
    box(target, 0, 0);
    wattron(target, A_BOLD);
    mvwaddnstr(target, 0, 2, title.c_str(), getmaxx(target) - 4);
    wattroff(target, A_BOLD);
}

std::array<std::string, 7> register_values(const cpu_state& state) {
    return {"A:  " + hex_value(state.A(), 2) + "   F: " + hex_value(state.F(), 2),
            "BC: " + hex_value(state.BC(), 4),
            "DE: " + hex_value(state.DE(), 4),
            "HL: " + hex_value(state.HL(), 4),
            "SP: " + hex_value(state.SP(), 4),
            "PC: " + hex_value(state.PC(), 4),
            "S Z AC P C: " + std::to_string(state.flgS()) + " " + std::to_string(state.flgZ()) + " " +
                std::to_string(state.flgAC()) + " " + std::to_string(state.flgP()) + " " +
                std::to_string(state.flgC())};
}

}

void screen::draw(const std::vector<device_description>& devices, const cpu_state& snapshot, bool halted,
                  const trace_history& history, view_state& ui) {
    int height, width;
    getmaxyx(stdscr, height, width);
    if (height != rows || width != columns) {
        machine.reset();
        instructions.reset();
        registers.reset();
        rows = height;
        columns = width;
        erase();
        if (rows >= MINIMUM_ROWS && columns >= MINIMUM_COLUMNS) {
            const auto cards = machine_lines(devices, columns - 4);
            const int top = std::min(std::max(5, static_cast<int>(cards.size()) + 3), rows / 3);
            machine.reset(newwin(top, columns, 0, 0));
            instructions.reset(newwin(rows - top, columns / 2, top, 0));
            registers.reset(newwin(rows - top, columns - columns / 2, top, columns / 2));
            if (!machine || !instructions || !registers)
                throw std::runtime_error("Could not allocate TUI panels");
        }
    }
    const bool usable = machine && instructions && registers;
    if (!usable) {
        ui.running = false;
        erase();
        mvaddnstr(0, 0, "Terminal too small: resize to 76x18. Paused; q quits.", std::max(0, columns - 1));
        refresh();
    } else {
        wnoutrefresh(stdscr);
        const auto cards = machine_lines(devices, columns - 4);
        const usize visible_cards = getmaxy(machine.get()) - 3;
        ui.card_offset =
            std::min(ui.card_offset, cards.size() > visible_cards ? cards.size() - visible_cards : 0);
        frame(machine.get(), std::string(" Machine | ") +
                                 (halted       ? "HALTED"
                                  : ui.running ? "RUNNING"
                                               : "PAUSED") +
                                 " | Up/Down: cards ");
        for (usize i = 0; i < visible_cards && ui.card_offset + i < cards.size(); ++i)
            put(machine.get(), i + 1, cards[ui.card_offset + i]);
        put(machine.get(), getmaxy(machine.get()) - 2,
            "Space: run/pause | s: step | x: trace on/off | q: quit | PgUp/PgDn");

        frame(instructions.get(), " Instructions | executed ");
        frame(registers.get(), " CPU registers ");
        if (!ui.reporting) {
            put(instructions.get(), 1, "Disabled (x to enable)");
            put(registers.get(), 1, "Disabled (x to enable)");
        } else {
            const usize visible_trace = getmaxy(instructions.get()) - 2;
            ui.trace_offset = std::min(ui.trace_offset,
                                       history.size() > visible_trace ? history.size() - visible_trace : 0);
            const usize end = history.size() - ui.trace_offset;
            const usize begin = end > visible_trace ? end - visible_trace : 0;
            for (usize i = begin; i < end; ++i)
                put(instructions.get(), i - begin + 1, history[i]);
            if (!history.size())
                put(instructions.get(), 1, "No instructions recorded.");

            const auto& state = snapshot;
            const auto values = register_values(state);
            const auto previous_values = register_values(ui.previous);
            for (usize i = 0; i < values.size(); ++i) {
                const bool changed = values[i] != previous_values[i];
                if (changed)
                    wattron(registers.get(), A_BOLD);
                put(registers.get(), i + 1, values[i]);
                if (changed)
                    wattroff(registers.get(), A_BOLD);
            }
            ui.previous = state;
        }
        wnoutrefresh(machine.get());
        wnoutrefresh(instructions.get());
        wnoutrefresh(registers.get());
        doupdate();
    }
}

command read_command(bool running) {
    wtimeout(stdscr, running ? 0 : static_cast<int>(app::FRAME_INTERVAL.count()));
    switch (wgetch(stdscr)) {
        case 'q':
        case 'Q':
            return command::QUIT;
        case ' ':
            return command::TOGGLE_RUN;
        case 'x':
        case 'X':
            return command::TOGGLE_REPORTING;
        case 's':
        case 'S':
            return command::STEP;
        case KEY_UP:
            return command::UP;
        case KEY_DOWN:
            return command::DOWN;
        case KEY_PPAGE:
            return command::PAGE_UP;
        case KEY_NPAGE:
            return command::PAGE_DOWN;
        default:
            return command::NONE;
    }
}

}
