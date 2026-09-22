#ifndef BUDDY8800_SRC_APP_BINARY_FILE_HPP_
#define BUDDY8800_SRC_APP_BINARY_FILE_HPP_

#include <filesystem>
#include <vector>
#include "util/typedef.hpp"

namespace buddy8800::app {

/// @brief Read a complete binary file or throw on an I/O error.
std::vector<u8> read_binary(const std::filesystem::path& filename);

}

#endif
