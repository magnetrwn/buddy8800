#ifndef BUDDY8800_SRC_UX_MACHINE_VIEW_HPP_
#define BUDDY8800_SRC_UX_MACHINE_VIEW_HPP_

#include <string>
#include <vector>
#include "core/bus/device_description.hpp"

namespace buddy8800::ux {

/// @brief Format a machine description with optional terminal colors.
std::string format_machine(const std::vector<device_description>& devices, bool color);

/// @brief Format device descriptions into lines bounded by the display width.
std::vector<std::string> machine_lines(const std::vector<device_description>& devices, int width);

}

#endif
