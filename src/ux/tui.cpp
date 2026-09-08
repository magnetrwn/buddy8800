#include "ux.hpp"
#include "trace_view.hpp"
#include <curses.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <streambuf>

namespace {
using window = std::unique_ptr<WINDOW, decltype(&delwin)>;

class terminal_session {
    SCREEN* screen = nullptr;
public:
    terminal_session() {
        const char* term = std::getenv("TERM");
        if (!term || !*term || std::string(term) == "dumb")
            throw std::runtime_error("TUI requires a usable TERM; use a terminal or build with DISABLE_TRACE=ON");
        screen = newterm(nullptr, stdout, stdin);
        if (!screen) throw std::runtime_error("Could not initialize ncurses");
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        set_escdelay(25);
    }
    ~terminal_session() { endwin(); delscreen(screen); }
};

// Pseudo-BDOS writes to std::cout. Keep guest console output inside the TUI too.
class console_capture : public std::streambuf {
    trace_history& history;
    std::string line;
    std::streambuf* previous;
    bool enabled = true;
    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof())) return traits_type::not_eof(ch);
        if (!enabled) return ch;
        const char byte = traits_type::to_char_type(ch);
        if (byte == '\n' || line.size() == 120) {
            history.append("BDOS: " + line);
            line.clear();
        }
        if (byte != '\r' && byte != '\n') line += (byte >= 32 && byte < 127) ? byte : '.';
        return ch;
    }
public:
    explicit console_capture(trace_history& history) : history(history), previous(std::cout.rdbuf(this)) {}
    void enable(bool value) { enabled = value; line.clear(); }
    void flush_pending() {
        if (!line.empty()) { history.append("BDOS: " + line); line.clear(); }
    }
    ~console_capture() { std::cout.rdbuf(previous); }
};

std::vector<std::string> machine_lines(const std::string& map, int width) {
    std::string plain;
    bool escape = false;
    for (char ch : map) {
        if (ch == '\x1b') { escape = true; continue; }
        if (escape) { if (ch == 'm') escape = false; continue; }
        plain += ch;
    }
    std::istringstream stream(plain);
    std::vector<std::string> result;
    for (std::string line; std::getline(stream, line);) {
        while (line.size() > static_cast<usize>(width)) {
            result.push_back(line.substr(0, width));
            line.erase(0, width);
        }
        result.push_back(line);
    }
    return result;
}

void put(WINDOW* target, int row, const std::string& text) {
    if (row <= 0 || row >= getmaxy(target) - 1) return;
    mvwaddnstr(target, row, 2, text.c_str(), std::max(0, getmaxx(target) - 4));
}
void frame(WINDOW* target, const std::string& title) {
    werase(target);
    box(target, 0, 0);
    wattron(target, A_BOLD);
    mvwaddnstr(target, 0, 2, title.c_str(), getmaxx(target) - 4);
    wattroff(target, A_BOLD);
}
std::string hex_value(unsigned value, int digits) {
    std::ostringstream text;
    text << std::uppercase << std::hex << std::setfill('0') << std::setw(digits) << value;
    return text.str();
}
}

int run_tui(emulator& emu, const volatile std::sig_atomic_t& stop) {
    terminal_session terminal;
    trace_history history;
    console_capture console(history);
    emu.trace([&](u16 address, const std::array<u8, 3>& bytes, usize size) {
        history.append(instruction_line(address, bytes, size));
    });
    // Remove the observer before its captured history goes out of scope, including on exceptions.
    struct observer_guard {
        emulator& emu;
        ~observer_guard() { emu.trace({}); }
    } observer{emu};

    window machine(nullptr, delwin), instructions(nullptr, delwin), registers(nullptr, delwin);
    int rows = 0, columns = 0;
    usize card_offset = 0, trace_offset = 0;
    bool running = false;
    bool reporting = true;
    auto previous = emu.state();
    const auto minimum_rows = 18;
    const auto minimum_columns = 76;

    while (!stop) {
        console.flush_pending();
        int height, width;
        getmaxyx(stdscr, height, width);
        if (height != rows || width != columns) {
            machine.reset(); instructions.reset(); registers.reset();
            rows = height; columns = width;
            erase();
            if (rows >= minimum_rows && columns >= minimum_columns) {
                const auto cards = machine_lines(emu.info(), columns - 4);
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
            running = false;
            erase();
            mvaddnstr(0, 0, "Terminal too small: resize to 76x18. Paused; q quits.", std::max(0, columns - 1));
            refresh();
        } else {
            wnoutrefresh(stdscr);
            const auto cards = machine_lines(emu.info(), columns - 4);
            const usize visible_cards = getmaxy(machine.get()) - 3;
            card_offset = std::min(card_offset, cards.size() > visible_cards ? cards.size() - visible_cards : 0);
            frame(machine.get(), std::string(" Machine | ") + (emu.halted() ? "HALTED" : running ? "RUNNING" : "PAUSED") + " | Up/Down: cards ");
            for (usize i = 0; i < visible_cards && card_offset + i < cards.size(); ++i)
                put(machine.get(), i + 1, cards[card_offset + i]);
            put(machine.get(), getmaxy(machine.get()) - 2, "Space: run/pause | s: step | x: trace on/off | q: quit | PgUp/PgDn");

            frame(instructions.get(), " Instructions | executed ");
            frame(registers.get(), " CPU registers ");
            if (!reporting) {
                put(instructions.get(), 1, "Disabled (x to enable)");
                put(registers.get(), 1, "Disabled (x to enable)");
            } else {
                const usize visible_trace = getmaxy(instructions.get()) - 2;
                trace_offset = std::min(trace_offset, history.size() > visible_trace ? history.size() - visible_trace : 0);
                const usize end = history.size() - trace_offset;
                const usize begin = end > visible_trace ? end - visible_trace : 0;
                for (usize i = begin; i < end; ++i) put(instructions.get(), i - begin + 1, history[i]);
                if (!history.size()) put(instructions.get(), 1, "No instructions recorded.");

                const auto state = emu.state();
                const std::array<std::string, 7> values{
                    "A:  " + hex_value(state.A(), 2) + "   F: " + hex_value(state.F(), 2),
                    "BC: " + hex_value(state.BC(), 4), "DE: " + hex_value(state.DE(), 4),
                    "HL: " + hex_value(state.HL(), 4), "SP: " + hex_value(state.SP(), 4),
                    "PC: " + hex_value(state.PC(), 4),
                    "S Z AC P C: " + std::to_string(state.flgS()) + " " + std::to_string(state.flgZ()) + " " +
                        std::to_string(state.flgAC()) + " " + std::to_string(state.flgP()) + " " + std::to_string(state.flgC())};
                for (usize i = 0; i < values.size(); ++i) {
                    const bool changed = i < 6 ? state.registers[i] != previous.registers[i] : state.F() != previous.F();
                    if (changed) wattron(registers.get(), A_BOLD);
                    put(registers.get(), i + 1, values[i]);
                    if (changed) wattroff(registers.get(), A_BOLD);
                }
                previous = state;
            }
            wnoutrefresh(machine.get()); wnoutrefresh(instructions.get()); wnoutrefresh(registers.get());
            doupdate();
        }

        // Bound execution work per frame as well as refresh frequency. Each
        // instruction is recorded, but the screen is drawn at about 30 Hz.
        wtimeout(stdscr, running ? 0 : 33);
        const int key = wgetch(stdscr);
        if (key == 'q' || key == 'Q') break;
        if (usable) {
            if (key == ' ' && !emu.halted()) { running = !running; trace_offset = 0; }
            if (key == 'x' || key == 'X') {
                reporting = !reporting;
                console.enable(reporting);
                history.clear();
                trace_offset = 0;
            }
            if (key == 's' || key == 'S') {
                running = false; trace_offset = 0;
                if (reporting) emu.step(); else emu.step<false>();
            }
            if (key == KEY_UP && card_offset) --card_offset;
            if (key == KEY_DOWN) ++card_offset;
            if (reporting && key == KEY_PPAGE) { running = false; trace_offset += getmaxy(instructions.get()) - 2; }
            if (reporting && key == KEY_NPAGE) {
                const usize page = getmaxy(instructions.get()) - 2;
                trace_offset = trace_offset > page ? trace_offset - page : 0;
            }
        }
        if (running) {
            if (!reporting) {
                // Spend the frame executing, not sleeping. Check time in batches
                // so the hot path has neither tracing nor a clock call per opcode.
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(33);
                do {
                    for (usize count = 0; count < 256 && !stop && !emu.halted(); ++count)
                        emu.step<false>();
                } while (!stop && !emu.halted() && std::chrono::steady_clock::now() < deadline);
            } else {
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(8);
                for (usize count = 0; count < 4096 && !stop && !emu.halted() &&
                     std::chrono::steady_clock::now() < deadline; ++count) emu.step();
                napms(25);
            }
            if (emu.halted()) running = false;
        }
    }
    return 0;
}
