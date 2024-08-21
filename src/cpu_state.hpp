#ifndef CPU_STATE_HPP_
#define CPU_STATE_HPP_

#include <array>

#include "typedef.hpp"

/// @brief Enumerates the 8 bit registers of the CPU (matches cpu_registers16).
enum class cpu_registers8 {
    A, F, B, C, D, E, H, L, HIGH_SP, LOW_SP, HIGH_PC, LOW_PC, _M
};

/// @brief Check if a register is actually a memory reference.
static constexpr bool is_memref(cpu_registers8 reg) {
    return reg == cpu_registers8::_M;
}

/// @brief Provides correct registers coming from opcode register selection.
static constexpr cpu_registers8 cpu_reg8_decode[8] {
    cpu_registers8::B, cpu_registers8::C, cpu_registers8::D, cpu_registers8::E,
    cpu_registers8::H, cpu_registers8::L, cpu_registers8::_M, cpu_registers8::A
};

/// @brief Enumerates the 16 bit registers of the CPU (matches cpu_registers8).
enum class cpu_registers16 {
    AF, PSW = AF, BC, DE, HL, SP, PC
};

/// @brief Enumerates the flags of the CPU at their bit position.
enum class cpu_flags {
    C = 0x01, CY = C, P = 0x04, AC = 0x10, Z = 0x40, S = 0x80
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
        return (registers[static_cast<usize>(reg) >> 1] >> (8 * !(static_cast<usize>(reg) & 1))) & 0xFF;
    }

    /// @brief Get the value of a 16 bit register (including any pair of 8 bit registers).
    constexpr u16 get_register16(cpu_registers16 pair) const {
        return registers[static_cast<usize>(pair)];
    }

    /// @brief Set the value of an 8 bit register (including halves of SP and PC).
    constexpr void set_register8(cpu_registers8 reg, u8 value) {
        registers[static_cast<usize>(reg) >> 1] &= (0xFF00 >> (8 * !(static_cast<usize>(reg) & 1)));
        registers[static_cast<usize>(reg) >> 1] |= (value << (8 * !(static_cast<usize>(reg) & 1)));
    }

    /// @brief Set the value of a 16 bit register (including any pair of 8 bit registers).
    constexpr void set_register16(cpu_registers16 pair, u16 value) {
        registers[static_cast<usize>(pair)] = value;
    }

    /// \}
    /// @name Increment methods.
    /// \{

    /// @brief Get the value of an 8 bit register (including halves of SP and PC) and then increment it.
    constexpr u8 get_then_inc_register8(cpu_registers8 reg) {
        u8 value = get_register8(reg);
        set_register8(reg, value + 1);
        return value;
    }

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
        return (get_register8(cpu_registers8::F) & static_cast<u8>(flag));
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

    /// @brief Set or unset all non-carry flags based on a provided value.
    constexpr void set_Z_S_P_flags(u8 is) {
        set_if_flag(cpu_flags::Z, !is);
        set_if_flag(cpu_flags::S, is & 0x80);
        set_if_flag(cpu_flags::P, !__builtin_parity(is));
    }

    /// \}
    /// @name Shortcut methods.
    /// \{

    constexpr u16 AF() const { return get_register16(cpu_registers16::AF); }
    constexpr u16 BC() const { return get_register16(cpu_registers16::BC); }
    constexpr u16 DE() const { return get_register16(cpu_registers16::DE); }
    constexpr u16 HL() const { return get_register16(cpu_registers16::HL); }
    constexpr u16 SP() const { return get_register16(cpu_registers16::SP); }
    constexpr u16 PC() const { return get_register16(cpu_registers16::PC); }

    constexpr u8 A() const { return get_register8(cpu_registers8::A); }
    constexpr u8 F() const { return get_register8(cpu_registers8::F); }
    constexpr u8 B() const { return get_register8(cpu_registers8::B); }
    constexpr u8 C() const { return get_register8(cpu_registers8::C); }
    constexpr u8 D() const { return get_register8(cpu_registers8::D); }
    constexpr u8 E() const { return get_register8(cpu_registers8::E); }
    constexpr u8 H() const { return get_register8(cpu_registers8::H); }
    constexpr u8 L() const { return get_register8(cpu_registers8::L); }

    constexpr void AF(u16 value) { set_register16(cpu_registers16::AF, value); }
    constexpr void BC(u16 value) { set_register16(cpu_registers16::BC, value); }
    constexpr void DE(u16 value) { set_register16(cpu_registers16::DE, value); }
    constexpr void HL(u16 value) { set_register16(cpu_registers16::HL, value); }
    constexpr void SP(u16 value) { set_register16(cpu_registers16::SP, value); }
    constexpr void PC(u16 value) { set_register16(cpu_registers16::PC, value); }

    constexpr void A(u8 value) { set_register8(cpu_registers8::A, value); }
    constexpr void F(u8 value) { set_register8(cpu_registers8::F, value); }
    constexpr void B(u8 value) { set_register8(cpu_registers8::B, value); }
    constexpr void C(u8 value) { set_register8(cpu_registers8::C, value); }
    constexpr void D(u8 value) { set_register8(cpu_registers8::D, value); }
    constexpr void E(u8 value) { set_register8(cpu_registers8::E, value); }
    constexpr void H(u8 value) { set_register8(cpu_registers8::H, value); }
    constexpr void L(u8 value) { set_register8(cpu_registers8::L, value); }

    /// \}

    cpu_state() : registers({}) { set_register16(cpu_registers16::AF, 0x02); }
};

#endif