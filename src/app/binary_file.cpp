#include "app/binary_file.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace buddy8800::app {

std::vector<u8> read_binary(const std::filesystem::path& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Could not open file: " + filename.string());
    std::vector<u8> bytes(std::istreambuf_iterator<char>(file), {});
    if (file.bad() || bytes.empty())
        throw std::runtime_error("Empty or unreadable file: " + filename.string());
    return bytes;
}

}
