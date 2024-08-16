#ifndef CPU_STATE_HPP_
#define CPU_STATE_HPP_

#include <array>

#include "typedef.hpp"

/// @brief Enumerates the 8 bit registers of the CPU (matches cpu_registers16).
enum class cpu_registers8 {
    A, F, B, C, D, E, H, L, HIGH_SP, LOW_SP, HIGH_PC, LOW_PC
};

/// @brief Enumerates the 16 bit registers of the CPU (matches cpu_registers8).
enum class cpu_registers16 {
    AF, BC, DE, HL, SP, PC
};

/// @brief Enumerates the flags of the CPU at their bit position.
enum class cpu_flags {
    C = 0x01, P = 0x04, AC = 0x10, Z = 0x40, S = 0x80
};

/**
 * @brief Represents the state of the CPU.
 * 
 * This struct represents the CPU state and provides useful methods to work with it.
 * On construction all register space is initialized to zero (except bit 1 of the F register, which
 * is supposed to always be one).
 */
struct cpu_state {
    std::array<u16, 6> registers;

    /// @name Register state get/setters.
    /// \{

    /// @brief Get the value of an 8 bit register (including halves of SP and PC).
    constexpr u8 get_register8(cpu_registers8 reg) const {
        if (static_cast<usize>(reg) & 1)
            return registers[static_cast<usize>(reg) >> 1] & 0xFF;
        else
            return registers[static_cast<usize>(reg) >> 1] >> 8;
    }

    /// @brief Get the value of a 16 bit register (including any pair of 8 bit registers).
    constexpr u16 get_register16(cpu_registers16 pair) const {
        return registers[static_cast<usize>(pair)];
    }

    /// @brief Set the value of an 8 bit register (including halves of SP and PC).
    constexpr void set_register8(cpu_registers8 reg, u8 value) {
        if (static_cast<usize>(reg) & 1) {
            registers[static_cast<usize>(reg) >> 1] &= 0xFF00;
            registers[static_cast<usize>(reg) >> 1] |= value;
        } else {
            registers[static_cast<usize>(reg) >> 1] &= 0x00FF;
            registers[static_cast<usize>(reg) >> 1] |= (value << 8);
        }
    }

    /// @brief Set the value of a 16 bit register (including any pair of 8 bit registers).
    constexpr void set_register16(cpu_registers16 pair, u16 value) {
        registers[static_cast<usize>(pair)] = value;
    }

    /// \}
    /// @name Incrementing getters and other shortcuts.
    /// \{

    /// @brief Get the value of a 16 bit register (including any pair of 8 bit registers) and then increment it.
    constexpr u16 get_then_inc_register16(cpu_registers16 pair) {
        return registers[static_cast<usize>(pair)]++;
    }

    /// @brief Increment an 8 bit register.
    constexpr void inc_register8(cpu_registers8 reg) {
        set_register8(reg, get_register8(reg) + 1);
    }

    /// @brief Increment a 16 bit register.
    constexpr void inc_register16(cpu_registers16 pair) {
        ++registers[static_cast<usize>(pair)];
    }

    /// \}
    /// @name Flag state get/set/unsetters.
    /// \{

    /// @brief Get the value of a flag.
    constexpr bool get_flag(cpu_flags flag) const {
        return (get_register8(cpu_registers8::F) & static_cast<u8>(flag)) != 0x00;
    }

    /// @brief Set a flag.
    constexpr void set_flag(cpu_flags flag) {
        return set_register8(cpu_registers8::F, get_register8(cpu_registers8::F) | static_cast<u8>(flag));
    }

    /// @brief Unset a flag.
    constexpr void unset_flag(cpu_flags flag) {
        return set_register8(cpu_registers8::F, get_register8(cpu_registers8::F) & ~static_cast<u8>(flag));
    }

    /// @brief Set or unset a flag based on a boolean condition.
    constexpr void set_if_flag(cpu_flags flag, bool cond) {
        if (cond)
            set_flag(flag);
        else
            unset_flag(flag);
    }

    /*/// @brief Set or unset all flags based on the older value of an 8 bit register, and current state.
    template <cpu_registers8 reg>
    constexpr void set_all_flags(u8 was) {
        u8 is = get_register8<reg>();
        set_if_flag<cpu_flags::Z>(!is);
        set_if_flag<cpu_flags::S>(is & 0x80);
        set_if_flag<cpu_flags::P>(__builtin_parity(is));
        set_if_flag<cpu_flags::AC>(!(was & 0xF0) and (is & 0x08));
    }*/

    /// \}

    cpu_state() : registers({}) { set_register16(cpu_registers16::AF, 0x02); }
};

#endif