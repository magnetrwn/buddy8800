#include "app/emulator.hpp"
#include "app/options.hpp"
#include "app/program_loader.hpp"
#include "ux/frontend.hpp"
#include <csignal>
#include <iostream>

namespace {

volatile std::sig_atomic_t stopped = 0;

void stop(int) {
    stopped = 1;
}

}

int main(int argc, char** argv) {
    try {
        const auto options = buddy8800::app::parse_options(argc, argv);
        if (options.help) {
            std::cout << buddy8800::app::usage(buddy8800::ux::tui_available());
            return 0;
        }
        std::signal(SIGINT, stop);
        std::signal(SIGTERM, stop);
        emulator machine(options.config.c_str());
        buddy8800::app::load_programs(machine, options.programs);
        return buddy8800::ux::run_frontend(machine, stopped);
    } catch (const std::exception& error) {
        std::cerr << "buddy8800: " << error.what() << '\n';
        return 1;
    }
}
