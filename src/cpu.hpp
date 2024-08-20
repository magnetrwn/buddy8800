#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <cstdio>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "cpu_state.hpp"
#include "typedef.hpp"
#include "util.hpp"

/**
 * @brief Represents the CPU of the emulator.
 *
 * This is the 8080 CPU emulator and interpreter. It abstracts the CPU functionality into straightforward methods
 * and presents a way to step and interact with state. Being an interpreter, each opcode starting from the program
 * counter address in memory is fetched and executed according to what it represents and including further
 * fetches to opcode operands. The API also provides public access to execute(), should the user want to manually
 * ask for instructions.
 *
 * For a use case example you can look at `ŧests/test_cpu.hpp`.
 */
class cpu {
private:
    cpu_state state;
    std::array<u8, 0x10000> memory;
    bool just_booted; // turn this into "wait"
    bool halted;
    bool do_handle_bdos;
    bool interrupts_enabled;
    
    util::print_helper printer;

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

    bool resolve_flag_cond(u8 cc);
    void handle_bdos();

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
        state.inc_register8(reg);
        u8 value = state.get_register8(reg);
        state.set_if_flag(cpu_flags::AC, (value ^ (value - 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void _INR_M() { 
        u8 value = ++memory[state.get_register16(cpu_registers16::HL)];
        state.set_if_flag(cpu_flags::AC, (value ^ (value - 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void DCR(cpu_registers8 reg) {
        if (is_memref(reg)) return _DCR_M();
        u8 value = state.get_register8(reg);
        state.set_register8(reg, --value);
        state.set_if_flag(cpu_flags::AC, ~(value ^ (value + 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void _DCR_M() {
        u8 value = --memory[state.get_register16(cpu_registers16::HL)];
        state.set_if_flag(cpu_flags::AC, ~(value ^ (value + 1)) & 0x10);
        state.set_Z_S_P_flags(value);
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
        u16 a = state.get_register8(cpu_registers8::A);
        a += 0x06 * ((a & 0x0F) > 0x09 or state.get_flag(cpu_flags::AC));
        a += 0x60 * ((a & 0xF0) > 0x90 or state.get_flag(cpu_flags::C));
        state.set_if_flag(cpu_flags::AC, (a ^ state.get_register8(cpu_registers8::A)) & 0x10);
        state.set_if_flag(cpu_flags::C, a & 0x100);
        state.set_register8(cpu_registers8::A, a);
        state.set_Z_S_P_flags(a);
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
        bool is_compare = false;
        bool is_borrow = false;

        switch (alu) {
            case 0b000: result = a + with; break;
            case 0b001: result = a + with + state.get_flag(cpu_flags::C); break;
            case 0b010: result = a - with; is_borrow = true; break;
            case 0b011: result = a - with - state.get_flag(cpu_flags::C); is_borrow = true; break;
            case 0b100: result = a & with; break;
            case 0b101: result = a ^ with; break;
            case 0b110: result = a | with; break;
            case 0b111: 
                is_borrow = true;
                is_compare = true;
                result = a - with;
        }

        if (!is_compare)
            state.set_register8(cpu_registers8::A, result);
        
        if (is_borrow)
            state.set_if_flag(cpu_flags::AC, (a & 0x0F) >= (with & 0x0F));
        else
            state.set_if_flag(cpu_flags::AC, (a & 0x0F) + (with & 0x0F) > 0x0F);

        state.set_if_flag(cpu_flags::C, result & 0x0100);
        state.set_Z_S_P_flags(result);
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
        bool is_compare = false;
        bool is_borrow = false;

        switch (alu) {
            case 0b000: result = a + with; break;
            case 0b001: result = a + with + state.get_flag(cpu_flags::C); break;
            case 0b010: result = a - with; is_borrow = true; break;
            case 0b011: result = a - with - state.get_flag(cpu_flags::C); is_borrow = true; break;
            case 0b100: result = a & with; break;
            case 0b101: result = a ^ with; break;
            case 0b110: result = a | with; break;
            case 0b111: 
                is_borrow = true;
                is_compare = true;
                result = a - with;
        }

        if (!is_compare)
            state.set_register8(cpu_registers8::A, result);
        
        if (is_borrow)
            state.set_if_flag(cpu_flags::AC, (a & 0x0F) >= (with & 0x0F));
        else
            state.set_if_flag(cpu_flags::AC, (a & 0x0F) + (with & 0x0F) > 0x0F);

        state.set_if_flag(cpu_flags::C, result & 0x0100);
        state.set_Z_S_P_flags(result);
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

    inline void DI() { interrupts_enabled = false; }

    inline void SPHL() { state.set_register16(cpu_registers16::SP, state.get_register16(cpu_registers16::HL)); }

    inline void EI() { interrupts_enabled = true; }

    /// \}

public:
    void step();
    void execute(u8 opcode);

    void load(std::vector<u8>::iterator begin, std::vector<u8>::iterator end, usize offset = 0, bool auto_reset_vector = false);
    void load_state(const cpu_state& new_state);
    cpu_state save_state() const;
    bool is_halted() const;

    void set_handle_bdos(bool should);

    void set_printer_to_file(const char* filename);
    void reset_printer();

    void clear();

    cpu() : state(), memory({}), just_booted(true), halted(false), do_handle_bdos(false), 
            interrupts_enabled(true), printer(std::cout) {}
};

#endif