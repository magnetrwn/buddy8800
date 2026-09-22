#ifndef BUDDY8800_SRC_APP_CONFIGURATION_HPP_
#define BUDDY8800_SRC_APP_CONFIGURATION_HPP_

#include <filesystem>
#include <string>
#include <vector>
#include "util/typedef.hpp"

namespace buddy8800::app {

/// @brief Parsed placement and initialization settings for one card.
struct card_config {
    std::string type;
    u16 at;
    usize slot;
    usize range;
    std::filesystem::path load;
    bool allow_collision;
    u8 switches = 0;
};

/// @brief Machine settings without live devices or host resources.
struct machine_config {
    u16 start_pc = 0;
    // Deprecated diagnostic setting; retain existing TOML files.
    bool pseudo_bdos = false;
    std::vector<card_config> cards;
};

/// @brief Parse configuration and resolve filenames without opening PTYs.
machine_config read_config(const std::filesystem::path& filename);

}

#endif
