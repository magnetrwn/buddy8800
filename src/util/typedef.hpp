#ifndef BUDDY8800_SRC_UTIL_TYPEDEF_HPP_
#define BUDDY8800_SRC_UTIL_TYPEDEF_HPP_

#include <cstdint>
#include <cstddef>

/**
 * @brief Common type definitions for easier use of types.
 * Global aliases below preserve the existing public API; new facilities can use
 * buddy8800::types without importing a namespace or Unix headers.
 */
namespace buddy8800::types {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using usize = std::size_t;

using fd = int;

}

using buddy8800::types::fd;
using buddy8800::types::u16;
using buddy8800::types::u32;
using buddy8800::types::u8;
using buddy8800::types::usize;

static constexpr u8 BAD_U8 = 0xFF;

#endif
