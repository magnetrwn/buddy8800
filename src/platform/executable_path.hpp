#ifndef BUDDY8800_SRC_PLATFORM_EXECUTABLE_PATH_HPP_
#define BUDDY8800_SRC_PLATFORM_EXECUTABLE_PATH_HPP_

#include <string>

namespace buddy8800::platform {

/// @brief Locate the running executable's directory.
std::string executable_directory();

}

#endif
