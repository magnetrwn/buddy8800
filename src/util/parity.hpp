#ifndef BUDDY8800_SRC_UTIL_PARITY_HPP_
#define BUDDY8800_SRC_UTIL_PARITY_HPP_

#include <type_traits>
#include "util/typedef.hpp"

namespace buddy8800 {

template <typename value_type>
constexpr bool odd_parity(value_type value) {
    static_assert(std::is_integral_v<value_type>, "value_type must be an integral type");
    usize count = 0;
    for (usize i = 0; i < 8 * sizeof(value_type); ++i)
        count += (value >> i) & 1;
    return count & 1;
}

constexpr bool byte_odd_parity(u8 value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_parity(value);
#else
    return odd_parity(value);
#endif
}

}

#endif
