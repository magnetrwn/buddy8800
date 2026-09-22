#include "ux/frontend.hpp"
#include "app/emulator.hpp"
#include "app/execution.hpp"
#include "ux/tui/terminal.hpp"
#include "ux/tui/screen.hpp"
#include "ux/tui/diagnostic_console.hpp"
#include "ux/tui/observers.hpp"

namespace buddy8800::ux::tui {

namespace {

void apply_command(command action, emulator& machine, view_state& ui, usize page, trace_history& history,
                   diagnostic_console& console) {
    switch (action) {
        case command::TOGGLE_RUN:
            if (!machine.halted()) {
                ui.running = !ui.running;
                ui.trace_offset = 0;
            }
            break;
        case command::TOGGLE_REPORTING:
            ui.reporting = !ui.reporting;
            console.enable(ui.reporting);
            history.clear();
            ui.trace_offset = 0;
            break;
        case command::STEP:
            ui.running = false;
            ui.trace_offset = 0;
            if (ui.reporting)
                machine.step();
            else
                machine.step<false>();
            break;
        case command::UP:
            if (ui.card_offset)
                --ui.card_offset;
            break;
        case command::DOWN:
            ++ui.card_offset;
            break;
        case command::PAGE_UP:
            if (ui.reporting) {
                ui.running = false;
                ui.trace_offset += page;
            }
            break;
        case command::PAGE_DOWN:
            if (ui.reporting)
                ui.trace_offset = ui.trace_offset > page ? ui.trace_offset - page : 0;
            break;
        default:
            break;
    }
}

}

}

int buddy8800::ux::run_tui(emulator& machine, const volatile std::sig_atomic_t& stop) {
    using namespace buddy8800::ux::tui;
    terminal_session terminal;
    buddy8800::ux::tui::screen display;
    trace_history history;
    diagnostic_console console(history);
    observers subscriptions(machine, history, console.stream());
    view_state ui;
    ui.previous = machine.state();

    while (!stop) {
        console.flush_pending();
        const auto snapshot = ui.reporting ? machine.state() : ui.previous;
        display.draw(machine.devices(), snapshot, machine.halted(), history, ui);
        const auto action = read_command(ui.running);
        if (action == command::QUIT)
            break;
        if (display.usable())
            apply_command(action, machine, ui, display.trace_page_size(), history, console);
        if (ui.running) {
            buddy8800::app::execute_frame(machine, ui.reporting, stop);
            if (ui.reporting)
                napms(buddy8800::app::TRACED_REST_MS);
            if (machine.halted())
                ui.running = false;
        }
    }
    return 0;
}
