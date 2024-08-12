#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include "typedef.hpp"

enum cpu_registers8 {
    A, F, B, C, D, E, H, L, HIGH_SP, LOW_SP, HIGH_PC, LOW_PC
};

enum cpu_registers16 {
    AF, BC, DE, HL, SP, PC
};

struct cpu_state {
    // AF, BC, DE, HL, SP, PC
    std::array<u16, 6> registers;
    cpu_state() : registers({}) {}

    template <cpu_registers8 reg>
    constexpr u8 get_register8() const {
        if constexpr (reg & 1)
            return registers[reg >> 1] >> 8;
        else
            return registers[reg >> 1] & 0xFF;
    }

    template <cpu_registers16 pair>
    constexpr u16 get_register16() const {
        return registers[pair];
    }

    template <cpu_registers8 reg>
    constexpr void set_register8(u8 value) {
        if constexpr (reg & 1) {
            registers[reg >> 1] &= 0x00FF;
            registers[reg >> 1] |= (value << 8);
        } else {
            registers[reg >> 1] &= 0xFF00;
            registers[reg >> 1] |= value;
        }
    }

    template <cpu_registers16 pair>
    constexpr void set_register16(u16 value) {
        registers[pair] = value;
    }
};

#endif