#ifndef TYPEDEF_HPP_
#define TYPEDEF_HPP_

#include <cstdint>
#include <unistd.h>

/**
 * @brief Common type definitions for easier use of types.
 * @warning By including this file, you are injecting the definition namespace in the global one.
 */
namespace type_definitions {

typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
typedef std::size_t usize;

typedef std::int8_t i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef std::int64_t i64;

typedef float f32;
typedef double f64;

typedef int fd;
typedef ssize_t isize;

}

using namespace type_definitions;

static constexpr u8 BAD_U8 = 0xFF;

#endif
