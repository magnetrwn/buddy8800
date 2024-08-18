#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <cstdio>
#include <vector>
#include <stdexcept>

#include "cpu_state.hpp"
#include "typedef.hpp"

#if defined ENABLE_TRACE or defined ENABLE_TRACE_ESSENTIAL
#include "util.hpp"
#endif

class cpu {
private:
    cpu_state state;
    std::array<u8, 0x10000> memory;
    bool halted;

    inline u16 pc() const { return state.get_register16(cpu_registers16::PC); }
    inline u8 fetch() { return memory[state.get_then_inc_register16(cpu_registers16::PC)]; }

    template <usize ops>
    void _trace([[maybe_unused]] u8 opc) {
        #if defined ENABLE_TRACE or defined ENABLE_TRACE_ESSENTIAL
        if constexpr (ops == 1)
            printf("%04hX    %02hhX      \t %s\n", pc() - 1, opc, util::get_opcode_str(opc));
        else if constexpr (ops == 2)
            printf("%04hX    %02hhX %02hhX   \t %s\n", pc() - 1, opc, memory[pc()], util::get_opcode_str(opc));
        else if constexpr (ops == 3)
            printf("%04hX    %02hhX %02hhX %02hhX\t %s\n", pc() - 1, opc, memory[pc()], memory[pc() + 1], util::get_opcode_str(opc));
        else
            static_assert(false, "Invalid number of operands.");
        #endif
    }

    void _trace_state();
    void _trace_reg16_deref(cpu_registers16 reg);
    //void _trace_mem16_deref();
    void _trace_stackptr16_deref();
    void _trace_error(u8 opc);

    void execute(u8 opcode);
    bool resolve_flag_cond(u8 cc);
    void handle_bdos();

public:
    /// @name Opcode implementations.
    /// @todo Turn the _M functions into actually using conditional checking for the register being == _M
    /// @todo Make all this set(get()) into a single change()
    /// @todo Make a convenient function for getting (SP) and (SP+1), or just 2x8 bytes
    /// @todo Double-check everything is little-endian
    /// \{

    inline void NOP() {}

    inline void LXI(cpu_registers16 pair) { u16 lo = fetch(); u16 hi = fetch(); state.set_register16(pair, (hi << 8) | lo); }

    inline void STAX(cpu_registers16 pair) { memory[state.get_register16(pair)] = state.get_register8(cpu_registers8::A); }

    inline void INX(cpu_registers16 pair) { state.inc_register16(pair); }

    inline void INR(cpu_registers8 reg) {
        if (is_memref(reg)) return _INR_M();
        u8 value = state.get_register8(reg);
        state.set_register8(reg, value + 1);
        state.set_Z_S_P_AC_flags(value + 1, value);
    }

    inline void _INR_M() { 
        u8 value = memory[state.get_register16(cpu_registers16::HL)];
        ++memory[state.get_register16(cpu_registers16::HL)];
        state.set_Z_S_P_AC_flags(value + 1, value);
    }

    inline void DCR(cpu_registers8 reg) {
        if (is_memref(reg)) return _DCR_M();
        u8 value = state.get_register8(reg);
        state.set_register8(reg, value - 1);
        state.set_Z_S_P_AC_flags(value - 1, value);
    }

    inline void _DCR_M() {
        u8 value = memory[state.get_register16(cpu_registers16::HL)];
        --memory[state.get_register16(cpu_registers16::HL)];
        state.set_Z_S_P_AC_flags(value - 1, value);
    }

    inline void MVI(cpu_registers8 reg) { if (is_memref(reg)) return _MVI_M(); state.set_register8(reg, fetch()); }

    inline void _MVI_M() { memory[state.get_register16(cpu_registers16::HL)] = fetch(); }

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

    inline void MOV(cpu_registers8 dst, cpu_registers8 src) { 
        if (is_memref(src)) return _MOV_FROM_M(dst); 
        if (is_memref(dst)) return _MOV_TO_M(src); 
        state.set_register8(dst, state.get_register8(src)); 
    }

    inline void _MOV_FROM_M(cpu_registers8 dst) { state.set_register8(dst, memory[state.get_register16(cpu_registers16::HL)]); }

    inline void _MOV_TO_M(cpu_registers8 src) { memory[state.get_register16(cpu_registers16::HL)] = state.get_register8(src); }

    inline void HLT() { halted = true; }

    inline void ALU_OPERATIONS_A(cpu_registers8 src, u8 alu) {
        u16 a = state.get_register8(cpu_registers8::A);
        u16 with = ((is_memref(src)) ? memory[state.get_register16(cpu_registers16::HL)] : state.get_register8(src));
        u16 result;

        switch (alu) {
            case 0b000: result = a + with; break;
            case 0b001: result = a + with + state.get_flag(cpu_flags::C); break;
            case 0b010: result = a - with; break;
            case 0b011: result = a - with - state.get_flag(cpu_flags::C); break;
            case 0b100: result = a & with; break;
            case 0b101: result = a ^ with; break;
            case 0b110: result = a | with; break;
            case 0b111: 
                result = a - with;
                state.set_if_flag(cpu_flags::C, result & 0xFF00);
                state.set_Z_S_P_AC_flags(result, a);
                return;
        }

        state.set_register8(cpu_registers8::A, result);
        state.set_if_flag(cpu_flags::C, result & 0xFF00);
        state.set_Z_S_P_AC_flags(result, a);
    }

    inline void RETURN_ON(u8 cc) { if (resolve_flag_cond(cc)) RETURN(); }

    inline void POP(cpu_registers16 pair) {
        if (pair == cpu_registers16::SP) pair = cpu_registers16::AF;
        _trace_stackptr16_deref();
        u16 lo = memory[state.get_register16(cpu_registers16::SP)];
        u16 hi = memory[state.get_register16(cpu_registers16::SP) + 1];
        state.set_register16(pair, (hi << 8) | lo);
        state.set_register16(cpu_registers16::SP, state.get_register16(cpu_registers16::SP) + 2);
    }

    inline void JUMP_ON(u8 cc) { if (resolve_flag_cond(cc)) return JMP(); fetch(); fetch(); }

    inline void JMP() { u16 lo = fetch(); u16 hi = fetch(); state.set_register16(cpu_registers16::PC, (hi << 8) | lo); }

    inline void CALL_ON(u8 cc) { if (resolve_flag_cond(cc)) return CALL(); fetch(); fetch(); }

    inline void PUSH(cpu_registers16 pair) {
        if (pair == cpu_registers16::SP) pair = cpu_registers16::AF;
        state.set_register16(cpu_registers16::SP, state.get_register16(cpu_registers16::SP) - 2);
        memory[state.get_register16(cpu_registers16::SP)] = state.get_register16(pair) & 0xFF;
        memory[state.get_register16(cpu_registers16::SP) + 1] = state.get_register16(pair) >> 8;
        _trace_stackptr16_deref();
    }

    inline void ALU_OPERATIONS_A_IMM(u8 alu) {
        u16 a = state.get_register8(cpu_registers8::A);
        u16 with = fetch();
        u16 result;

        switch (alu) {
            case 0b000: result = a + with; break;
            case 0b001: result = a + with + state.get_flag(cpu_flags::C); break;
            case 0b010: result = a - with; break;
            case 0b011: result = a - with - state.get_flag(cpu_flags::C); break;
            case 0b100: result = a & with; break;
            case 0b101: result = a ^ with; break;
            case 0b110: result = a | with; break;
            case 0b111: 
                result = a - with;
                state.set_if_flag(cpu_flags::C, result & 0xFF00);
                state.set_Z_S_P_AC_flags(result, a);
                return;
        }

        state.set_register8(cpu_registers8::A, result);
        state.set_if_flag(cpu_flags::C, result & 0xFF00);
        state.set_Z_S_P_AC_flags(result, a);
    }

    inline void RST(u8 n) {
        PUSH(cpu_registers16::PC);
        state.set_register16(cpu_registers16::PC, n * 8);
    }

    inline void RETURN() { POP(cpu_registers16::PC); }

    inline void CALL() {
        u16 lo = fetch();
        u16 hi = fetch();
        PUSH(cpu_registers16::PC);
        state.set_register16(cpu_registers16::PC, (hi << 8) | lo);
    }

    inline void OUT() { throw std::runtime_error("OUT not implemented."); }

    inline void IN() { throw std::runtime_error("IN not implemented."); }

    inline void XTHL() {
        u16 stackmem_lo = memory[state.get_register16(cpu_registers16::SP)];
        u16 stackmem_hi = memory[state.get_register16(cpu_registers16::SP) + 1];
        u16 hl = state.get_register16(cpu_registers16::HL);
        state.set_register16(cpu_registers16::HL, stackmem_hi << 8 | stackmem_lo);
        memory[state.get_register16(cpu_registers16::SP)] = hl & 0xFF;
        memory[state.get_register16(cpu_registers16::SP) + 1] = hl >> 8;
    }

    inline void PCHL() { state.set_register16(cpu_registers16::PC, state.get_register16(cpu_registers16::HL)); }

    inline void XCHG() {
        u16 de = state.get_register16(cpu_registers16::DE);
        state.set_register16(cpu_registers16::DE, state.get_register16(cpu_registers16::HL));
        state.set_register16(cpu_registers16::HL, de);
    }

    inline void DI() { throw std::runtime_error("DI not implemented."); }

    inline void SPHL() { state.set_register16(cpu_registers16::SP, state.get_register16(cpu_registers16::HL)); }

    inline void EI() { throw std::runtime_error("EI not implemented."); }

    /// \}

    void step();

    void load(std::vector<u8>::iterator begin, std::vector<u8>::iterator end, usize offset = 0);
    void load_state(const cpu_state& new_state);
    cpu_state save_state() const;
    bool is_halted() const;

    cpu() : state(), memory({}), halted(false) {}
};

#endif