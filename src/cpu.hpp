#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <cstdio>
#include <vector>
#include <stdexcept>

#include "cpu_state.hpp"
#include "typedef.hpp"
#include "util.hpp"

class cpu {
private:
    cpu_state state;
    std::array<u8, 0x10000> memory;
    bool halted;

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

    void _trace_mem16_deref() {
        printf("\x1B[43;01m(%02hhX%02hhX): %02hhX                   \x1B[0m\n",
            memory[pc()],
            memory[pc() + 1],
            memory[(memory[pc() + 1] << 8) | memory[pc()]]);
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

    inline void RLC() {
        u8 a = state.get_register8(cpu_registers8::A);
        state.set_register8(cpu_registers8::A, (a << 1) | (a >> 7));
        state.set_if_flag(cpu_flags::C, a & 0x80);
    }

    inline void RRC() {
        u8 a = state.get_register8(cpu_registers8::A);
        state.set_register8(cpu_registers8::A, (a >> 1) | (a << 7));
        state.set_if_flag(cpu_flags::C, a & 0x01);
    }

    inline void RAL() {
        u8 a = state.get_register8(cpu_registers8::A);
        state.set_register8(cpu_registers8::A, (a << 1) | state.get_flag(cpu_flags::C));
        state.set_if_flag(cpu_flags::C, a & 0x80);
    }

    inline void RAR() {
        u8 a = state.get_register8(cpu_registers8::A);
        state.set_register8(cpu_registers8::A, (a >> 1) | (state.get_flag(cpu_flags::C) << 7));
        state.set_if_flag(cpu_flags::C, a & 0x01);
    }

    inline void SHLD() {
        u16 lo = fetch();
        u16 hi = fetch();
        u16 addr = (hi << 8) | lo;
        memory[addr] = state.get_register8(cpu_registers8::L);
        memory[addr + 1] = state.get_register8(cpu_registers8::H);
    }

    inline void DAA() {
        u8 a = state.get_register8(cpu_registers8::A);
        bool is_lo = ((a & 0x0F) > 0x09 or state.get_flag(cpu_flags::AC));
        a += 0x06 * is_lo;
        state.set_if_flag(cpu_flags::AC, is_lo);
        bool is_hi = ((a & 0xF0) > 0x90 or state.get_flag(cpu_flags::C));
        a += 0x60 * is_hi;
        state.set_if_flag(cpu_flags::C, is_hi);
        state.set_register8(cpu_registers8::A, a);
    }

    inline void LHLD() {
        u16 lo = fetch();
        u16 hi = fetch();
        u16 addr = (hi << 8) | lo;
        state.set_register8(cpu_registers8::L, memory[addr]);
        state.set_register8(cpu_registers8::H, memory[addr + 1]);
    }

    inline void CMA() { state.set_register8(cpu_registers8::A, ~state.get_register8(cpu_registers8::A)); }

    inline void STA() { u16 lo = fetch(); u16 hi = fetch(); memory[(hi << 8) | lo] = state.get_register8(cpu_registers8::A); }

    inline void STC() { state.set_flag(cpu_flags::C); }

    inline void LDA() { u16 lo = fetch(); u16 hi = fetch(); state.set_register8(cpu_registers8::A, memory[(hi << 8) | lo]); }

    inline void CMC() { state.set_if_flag(cpu_flags::C, !state.get_flag(cpu_flags::C)); }

    inline void MOV(cpu_registers8 dst, cpu_registers8 src) { state.set_register8(dst, state.get_register8(src)); }

    inline void MOV_FROM_M(cpu_registers8 dst) { state.set_register8(dst, memory[state.get_register16(cpu_registers16::HL)]); }

    inline void MOV_TO_M(cpu_registers8 src) { memory[state.get_register16(cpu_registers16::HL)] = state.get_register8(src); }

    inline void HLT() { halted = true; }

    /// \}

    void step();

    void load(std::vector<u8>::iterator begin, std::vector<u8>::iterator end, usize offset = 0);
    void load_state(const cpu_state& new_state);
    cpu_state save_state() const;

    cpu() : state(), memory({}), halted(false) {}
};

#endif