#include "platform/executable_path.hpp"
#include <unistd.h>
#include <stdexcept>
#include <vector>

namespace buddy8800::platform {

std::string executable_directory() {
    std::vector<char> buffer(512);
    for (;;) {
        const auto size = readlink("/proc/self/exe", buffer.data(), buffer.size());
        if (size < 0)
            throw std::runtime_error("failed to get_absolute_dir()");
        if (static_cast<std::size_t>(size) == buffer.size()) {
            buffer.resize(buffer.size() * 2);
            continue;
        }
        const std::string path(buffer.data(), static_cast<std::size_t>(size));
        return path.substr(0, path.rfind('/') + 1);
    }
}

}
