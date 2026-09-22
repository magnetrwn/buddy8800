#ifndef BUDDY8800_SRC_UTIL_FORMAT_HPP_
#define BUDDY8800_SRC_UTIL_FORMAT_HPP_

#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>
#include "util/typedef.hpp"

namespace buddy8800::format {

/// @brief Format an integral value with a hexadecimal prefix and minimum width.
template <typename value_type>
std::string to_hex_s(value_type value, usize zfill = 4) {
    static_assert(std::is_integral_v<value_type>, "value_type must be an integral type");
    std::ostringstream stream;
    stream << "0x" << std::setfill('0') << std::setw(zfill) << std::hex << value;
    return stream.str();
}

/// @brief Format uppercase hexadecimal digits without a prefix.
inline std::string hex_value(unsigned value, int digits) {
    std::ostringstream text;
    text << std::uppercase << std::hex << std::setfill('0') << std::setw(digits) << value;
    return text.str();
}

}

#endif
