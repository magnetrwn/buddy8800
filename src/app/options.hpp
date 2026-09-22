#ifndef BUDDY8800_SRC_APP_OPTIONS_HPP_
#define BUDDY8800_SRC_APP_OPTIONS_HPP_

#include <filesystem>
#include <string>
#include <vector>
#include "util/typedef.hpp"

namespace buddy8800::app {

/// @brief A binary filename and its guest load address.
struct program_load {
    std::filesystem::path filename;
    u16 address;
};

/// @brief Parsed command-line settings.
struct options {
    std::filesystem::path config;
    bool help = false;
    std::vector<program_load> programs;
};

/// @brief Parse binary load requests from command-line arguments.
std::vector<program_load> parse_programs(int argc, char** argv);

/// @brief Parse application options without constructing a machine.
options parse_options(int argc, char** argv);

/// @brief Describe command-line options for the available frontend.
std::string usage(bool tui_available);

}

#endif
