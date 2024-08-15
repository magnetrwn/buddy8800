#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <cstdio>

#include "cpu_state.hpp"
#include "typedef.hpp"
#include "util.hpp"

class cpu {
private:
    cpu_state state;
    std::array<u8, 0x10000> memory; // TODO: just temporary

    constexpr u16 pc() const { return state.get_register16<cpu_registers16::PC>(); }
    constexpr u8 fetch() { return memory[state.get_then_inc_register16<cpu_registers16::PC>()]; }

    template <u8 opc, usize ops>
    constexpr void _trace() {
        if constexpr (ops == 1)
            printf("%04hX    %02hhX      \t %s\n", pc() - 1, opc, util::get_opcode_str<opc>());
        else if constexpr (ops == 2)
            printf("%04hX    %02hhX %02hhX   \t %s\n", pc() - 1, opc, memory[pc()], util::get_opcode_str<opc>());
        else if constexpr (ops == 3)
            printf("%04hX    %02hhX %02hhX %02hhX\t %s\n", pc() - 1, opc, memory[pc()], memory[pc() + 1], util::get_opcode_str<opc>());
        else
            static_assert(false, "Invalid number of operands.");
    }

    constexpr void _trace_state() {
        printf("\x1B[44;01mA: %02hhX BC: %04hX DE: %04hX HL: %04hX \x1B[0m\n",
            state.get_register8<cpu_registers8::A>(),
            state.get_register16<cpu_registers16::BC>(),
            state.get_register16<cpu_registers16::DE>(),
            state.get_register16<cpu_registers16::HL>());
        printf("\x1B[44;01mF: %c %c %c %c %c                     \x1B[0m\n",
            state.get_flag<cpu_flags::S>() ? 'S' : '/',
            state.get_flag<cpu_flags::Z>() ? 'Z' : '/',
            state.get_flag<cpu_flags::AC>() ? 'A' : '/',
            state.get_flag<cpu_flags::P>() ? 'P' : '/',
            state.get_flag<cpu_flags::C>() ? 'C' : '/');
    }

    template <cpu_registers16 reg>
    constexpr void _trace_reg16_deref() {
        printf("\x1B[42;01m(%s: %04hX): %02hhX                   \x1B[0m\n",
            (reg == cpu_registers16::AF) ? "AF" : 
                (reg == cpu_registers16::BC) ? "BC" : 
                (reg == cpu_registers16::DE) ? "DE" : 
                (reg == cpu_registers16::HL) ? "HL" : 
                (reg == cpu_registers16::SP) ? "SP" : 
                "PC",
            state.get_register16<reg>(),
            memory[state.get_register16<reg>()]);
    }

    constexpr void _trace_error(u8 opc) {
        printf("%04hX    %02hhX      \t \x1B[31;01mUNKNOWN\x1B[0m\n", pc() - 1, opc);
    }

    void execute(u8 opcode);

public:
    /// @name Opcode implementations.
    /// \{

    inline void NOP() {}

    template <cpu_registers16 pair>
    inline void LXI() { u16 lo = fetch(); u16 hi = fetch(); state.set_register16<pair>((hi << 8) | lo); }

    template <cpu_registers16 pair>
    inline void STAX() { memory[state.get_register16<pair>()] = state.get_register8<cpu_registers8::A>(); }

    template <cpu_registers16 pair>
    inline void INX() { state.inc_register16<pair>(); }

    /*template <cpu_registers8 reg>
    inline void INR() { 
        state.inc_register8<reg>();
        u8 r = state.get_register8<reg>();

    }*/

    inline void INR_M() { ++memory[state.get_register16<cpu_registers16::HL>()]; }

    /// \}

    void step() { execute(fetch()); }

    cpu() : state(), memory({}) {
        for (int i = 0; i < 0x10000; i++)
            memory[i] = (i / 3) % 256;
    }
};

#endif