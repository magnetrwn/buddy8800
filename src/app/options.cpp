#include "app/options.hpp"
#include "platform/executable_path.hpp"
#include <stdexcept>

namespace buddy8800::app {

std::vector<program_load> parse_programs(int argc, char** argv) {
    if (argc < 1 || !(argc & 1))
        throw std::invalid_argument(
            "Invalid number of arguments. Provide pairs of ROM/data files and integer load addresses.");
    std::vector<program_load> result;
    for (int i = 1; i < argc; i += 2) {
        const std::string address(argv[i + 1]);
        usize consumed = 0;
        const auto offset = std::stoul(address, &consumed, 0);
        if (consumed != address.size() || offset > 65535 || address.front() == '-')
            throw std::invalid_argument("Invalid load address: " + address);
        result.push_back({argv[i], static_cast<u16>(offset)});
    }
    return result;
}

options parse_options(int argc, char** argv) {
    options result;
    if (argc > 1 && std::string(argv[1]) == "--help") {
        result.help = true;
        return result;
    }
    if (argc > 1 && std::string(argv[1]) == "--config") {
        if (argc < 3)
            throw std::invalid_argument("--config requires a filename");
        result.config = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        result.config = std::filesystem::path(platform::executable_directory()) / "config.toml";
    }
    result.programs = parse_programs(argc, argv);
    return result;
}

std::string usage(bool tui_available) {
    std::string text = "Usage: buddy8800 [--config FILE] [BINARY ADDRESS ...]\n"
                       "Connect a terminal to the serial PTY shown at startup.\n";
    if (tui_available)
        text += "TUI: Space run/pause, s step, x trace on/off, q quit; starts paused.\n";
    return text + "Redirected I/O uses the plain frontend. Ctrl-C stops the emulator.\n";
}

}
