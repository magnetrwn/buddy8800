#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <cstdio>
#include <stdexcept>

#include "cpu_state.hpp"
#include "typedef.hpp"
#include "util.hpp"

class cpu {
private:
    cpu_state state;
    std::array<u8, 0x10000> memory;

    inline u16 pc() const { return state.get_register16(cpu_registers16::PC); }
    inline u8 fetch() { return memory[state.get_then_inc_register16(cpu_registers16::PC)]; }

    template <usize ops>
    void _trace(u8 opc) {
        if constexpr (ops == 1)
            printf("%04hX    %02hhX      \t %s\n", pc() - 1, opc, util::get_opcode_str(opc));
        else if constexpr (ops == 2)
            printf("%04hX    %02hhX %02hhX   \t %s\n", pc() - 1, opc, memory[pc()], util::get_opcode_str(opc));
        else if constexpr (ops == 3)
            printf("%04hX    %02hhX %02hhX %02hhX\t %s\n", pc() - 1, opc, memory[pc()], memory[pc() + 1], util::get_opcode_str(opc));
        else
            static_assert(false, "Invalid number of operands.");
    }

    void _trace_state() {
        printf("\x1B[44;01mA: %02hhX BC: %04hX DE: %04hX HL: %04hX \x1B[0m\n",
            state.get_register8(cpu_registers8::A),
            state.get_register16(cpu_registers16::BC),
            state.get_register16(cpu_registers16::DE),
            state.get_register16(cpu_registers16::HL));
        printf("\x1B[44;01m SP: %04hX PC: %04hX F: %c %c %c %c %c  \x1B[0m\n",
            state.get_register16(cpu_registers16::SP),
            state.get_register16(cpu_registers16::PC),
            state.get_flag(cpu_flags::S) ? 'S' : '/',
            state.get_flag(cpu_flags::Z) ? 'Z' : '/',
            state.get_flag(cpu_flags::AC) ? 'A' : '/',
            state.get_flag(cpu_flags::P) ? 'P' : '/',
            state.get_flag(cpu_flags::C) ? 'C' : '/');
    }

    void _trace_reg16_deref(cpu_registers16 reg) {
        printf("\x1B[42;01m(%s: %04hX): %02hhX                   \x1B[0m\n",
            (reg == cpu_registers16::AF) ? "AF" : 
                (reg == cpu_registers16::BC) ? "BC" : 
                (reg == cpu_registers16::DE) ? "DE" : 
                (reg == cpu_registers16::HL) ? "HL" : 
                (reg == cpu_registers16::SP) ? "SP" : 
                "PC",
            state.get_register16(reg),
            memory[state.get_register16(reg)]);
    }

    void _trace_error(u8 opc) {
        printf("%04hX    %02hhX      \t \x1B[31;01mUNKNOWN\x1B[0m\n", pc() - 1, opc);
    }

    void execute(u8 opcode);

public:
    /// @name Opcode implementations.
    /// \{

    inline void NOP() {}
    inline void LXI(cpu_registers16 pair) { u16 lo = fetch(); u16 hi = fetch(); state.set_register16(pair, (hi << 8) | lo); }
    inline void STAX(cpu_registers16 pair) { memory[state.get_register16(pair)] = state.get_register8(cpu_registers8::A); }
    inline void INX(cpu_registers16 pair) { state.inc_register16(pair); }
    inline void INR(cpu_registers8 reg) { state.set_Z_S_P_AC_flags(reg, state.get_then_inc_register8(reg)); }
    inline void INR_M() { 
        u8 value = memory[state.get_register16(cpu_registers16::HL)];
        ++memory[state.get_register16(cpu_registers16::HL)];
        state.set_Z_S_P_AC_flags(value, value + 1);
    }
    inline void DCR(cpu_registers8 reg) {
        u8 value = state.get_register8(reg);
        state.set_register8(reg, value - 1);
        state.set_Z_S_P_AC_flags(reg, value);
    }
    inline void DCR_M() {
        u8 value = memory[state.get_register16(cpu_registers16::HL)];
        --memory[state.get_register16(cpu_registers16::HL)];
        state.set_Z_S_P_AC_flags(value, value - 1);
    }
    inline void MVI(cpu_registers8 reg) { state.set_register8(reg, fetch()); }
    inline void MVI_M() { memory[state.get_register16(cpu_registers16::HL)] = fetch(); }
    inline void DAD(cpu_registers16 pair) {
        u32 hl = state.get_register16(cpu_registers16::HL);
        u32 pval = state.get_register16(pair);
        u32 result = hl + pval;
        state.set_if_flag(cpu_flags::C, result & 0xFFFF0000);
        state.set_register16(cpu_registers16::HL, result);
    }
    inline void LDAX(cpu_registers16 pair) { state.set_register8(cpu_registers8::A, memory[state.get_register16(pair)]); }
    inline void DCX(cpu_registers16 pair) { state.set_register16(pair, state.get_register16(pair) - 1); }

    /// \}

    void step() { execute(fetch()); }

    template <typename FwdIt>
    void load(FwdIt begin, FwdIt end, usize offset = 0) {
        usize dist = std::distance(begin, end);

        if (dist > memory.size() - offset)
            throw std::out_of_range("Not enough space in emulated memory.");

        std::copy(begin, end, memory.begin() + offset);
    }

    void load_state(const cpu_state& new_state) { state = new_state; }
    cpu_state save_state() const { return state; }

    cpu() : state(), memory({}) {}
};

#endif