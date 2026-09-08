#include "ux.hpp"
#include "util.hpp"

namespace {
volatile std::sig_atomic_t stopped = 0;
void stop(int) { stopped = 1; }
}

int main(int argc, char** argv) {
    try {
        std::string config = util::get_absolute_dir() + "config.toml";
        if (argc > 1 && std::string(argv[1]) == "--help") {
            std::cout << "Usage: buddy8800 [--config FILE] [BINARY ADDRESS ...]\n"
                         "Connect a terminal to the serial PTY shown at startup.\n"
#ifndef DISABLE_TRACE
                         "TUI: Space run/pause, s step, x trace on/off, q quit; starts paused.\n"
#endif
                         "Redirected I/O uses the plain frontend. Ctrl-C stops the emulator.\n";
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--config") {
            if (argc < 3) throw std::invalid_argument("--config requires a filename");
            config = argv[2];
            argc -= 2;
            argv += 2;
        }
        std::signal(SIGINT, stop);
        std::signal(SIGTERM, stop);
        terminal_ux ux(config.c_str());
        return ux.main(argc, argv, stopped);
    } catch (const std::exception& error) {
        std::cerr << "buddy8800: " << error.what() << '\n';
        return 1;
    }
}
