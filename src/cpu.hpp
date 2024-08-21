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

    inline u16 pc() const { return state.PC(); }
    inline u8 fetch() { return memory[state.get_then_inc_register16(cpu_registers16::PC)]; }
    inline u16 fetch2() { u16 lo = fetch(); u16 hi = fetch(); return (hi << 8) | lo; }

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

    inline void LXI(cpu_registers16 pair) { state.set_register16(pair, fetch2()); }

    inline void STAX(cpu_registers16 pair) { memory[state.get_register16(pair)] = state.A(); }

    inline void INX(cpu_registers16 pair) { state.inc_register16(pair); }

    inline void INR(cpu_registers8 reg) {
        if (is_memref(reg)) return _INR_M();
        state.inc_register8(reg);
        u8 value = state.get_register8(reg);
        state.flgAC((value ^ (value - 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void _INR_M() { 
        u8 value = ++memory[state.HL()];
        state.flgAC((value ^ (value - 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void DCR(cpu_registers8 reg) {
        if (is_memref(reg)) return _DCR_M();
        u8 value = state.get_register8(reg);
        state.set_register8(reg, --value);
        state.flgAC(~(value ^ (value + 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void _DCR_M() {
        u8 value = --memory[state.HL()];
        state.flgAC(~(value ^ (value + 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void MVI(cpu_registers8 reg) { if (is_memref(reg)) return _MVI_M(); state.set_register8(reg, fetch()); }

    inline void _MVI_M() { memory[state.HL()] = fetch(); }

    inline void DAD(cpu_registers16 pair) {
        u32 hl = state.HL();
        u32 pval = state.get_register16(pair);
        u32 result = hl + pval;
        state.flgC(result & 0xFFFF0000);
        state.HL(result);
    }

    inline void LDAX(cpu_registers16 pair) { state.A(memory[state.get_register16(pair)]); }

    inline void DCX(cpu_registers16 pair) { state.set_register16(pair, state.get_register16(pair) - 1); }

    inline void RLC() { u8 a = state.A(); state.A((a << 1) | (a >> 7)); state.flgC(a & 0x80); }

    inline void RRC() { u8 a = state.A(); state.A((a >> 1) | (a << 7)); state.flgC(a & 0x01); }

    inline void RAL() {
        u8 a = state.A();
        state.A((a << 1) | state.flgC());
        state.flgC(a & 0x80);
    }

    inline void RAR() {
        u8 a = state.A();
        state.A((a >> 1) | (state.flgC() << 7));
        state.flgC(a & 0x01);
    }

    inline void SHLD() { u16 adr = fetch2(); memory[adr] = state.L(); memory[adr + 1] = state.H(); }

    inline void DAA() {
        u16 a = state.A();
        a += 0x06 * ((a & 0x0F) > 0x09 or state.flgAC());
        a += 0x60 * ((a & 0xF0) > 0x90 or state.flgC());
        state.flgAC((a ^ state.A()) & 0x10);
        state.flgC(a & 0x100);
        state.A(a);
        state.set_Z_S_P_flags(a);
    }

    inline void LHLD() { u16 adr = fetch2(); state.L(memory[adr]); state.H(memory[adr + 1]); }

    inline void CMA() { state.A(~state.A()); }

    inline void STA() { memory[fetch2()] = state.A(); }

    inline void STC() { state.flgC(true); }

    inline void LDA() { state.A(memory[fetch2()]); }

    inline void CMC() { state.flgC(!state.flgC()); }

    inline void MOV(cpu_registers8 dst, cpu_registers8 src) { 
        if (is_memref(src)) return _MOV_FROM_M(dst); 
        if (is_memref(dst)) return _MOV_TO_M(src); 
        state.set_register8(dst, state.get_register8(src)); 
    }

    inline void _MOV_FROM_M(cpu_registers8 dst) { state.set_register8(dst, memory[state.HL()]); }

    inline void _MOV_TO_M(cpu_registers8 src) { memory[state.HL()] = state.get_register8(src); }

    inline void HLT() { halted = true; }

    inline void _ALU(cpu_registers8 src, u8 alu, bool is_immediate) {
        u16 a = state.A();
        u16 with = (is_immediate) ? fetch() : ((is_memref(src)) ? memory[state.HL()] : state.get_register8(src));
        u16 result;
        
        switch (alu) {
            case 0b000: 
            case 0b001:
                result = a + with + ((alu == 0b001) ? state.flgC() : 0);
                state.flgAC(((a & 0x0F) + (with & 0x0F) + ((alu == 0b001) ? state.flgC() : 0)) > 0x0F);
                break;
            case 0b010: 
            case 0b011:
                result = a - with - ((alu == 0b011) ? state.flgC() : 0);
                state.flgAC((a & 0x0F) >= (with & 0x0F));
                break;
            case 0b100:
                result = a & with;
                // ANA (not ANI!) sets AC if bit 3 of (a | with) is 1
                if (is_immediate)
                    state.flgAC(false);
                else
                    state.flgAC((a | with) & 0x08);
                break;
            case 0b101:
            case 0b110:
                result = (alu == 0b101) ? (a ^ with) : (a | with);
                state.flgAC(false);
                break;
            case 0b111: 
                result = a - with;
                state.flgAC((a & 0x0F) < (with & 0x0F));
                state.flgC(result & 0x100);
                state.set_Z_S_P_flags(result);
                break;
            default: throw std::runtime_error("Invalid ALU operation.");
        }

        state.flgC(result & 0x100);
        state.set_Z_S_P_flags(result);
        if (alu != 0b111)
            state.A(result);
    }

    inline void ALU(cpu_registers8 src, u8 alu) { _ALU(src, alu, false); }

    inline void ALU_IMM(u8 alu) { _ALU(cpu_registers8::A /*unused*/, alu, true); }

    inline void RETURN_ON(u8 cc) { if (resolve_flag_cond(cc)) RETURN(); }

    inline void POP(cpu_registers16 pair) {
        if (pair == cpu_registers16::SP) pair = cpu_registers16::AF;
        //_trace_stackptr16_deref();
        u16 lo = memory[state.SP()];
        u16 hi = memory[state.SP() + 1];
        state.set_register16(pair, (hi << 8) | lo);
        state.SP(state.SP() + 2);
    }

    inline void JUMP_ON(u8 cc) { if (resolve_flag_cond(cc)) return JMP(); fetch2(); }

    inline void JMP() { state.PC(fetch2()); }

    inline void CALL_ON(u8 cc) { if (resolve_flag_cond(cc)) return CALL(); fetch2(); }

    inline void PUSH(cpu_registers16 pair) {
        if (pair == cpu_registers16::SP) pair = cpu_registers16::AF;
        state.SP(state.SP() - 2);
        memory[state.SP()] = state.get_register16(pair) & 0xFF;
        memory[state.SP() + 1] = state.get_register16(pair) >> 8;
        //_trace_stackptr16_deref();
    }

    inline void RST(u8 n) { PUSH(cpu_registers16::PC); state.PC(n * 8); }

    inline void RETURN() { POP(cpu_registers16::PC); }

    inline void CALL() { u16 adr = fetch2(); PUSH(cpu_registers16::PC); state.PC(adr); }

    inline void OUT() { throw std::runtime_error("OUT not implemented."); }

    inline void IN() { throw std::runtime_error("IN not implemented."); }

    inline void XTHL() {
        u16 stackmem_lo = memory[state.SP()];
        u16 stackmem_hi = memory[state.SP() + 1];
        u16 hl = state.HL();
        state.HL((stackmem_hi << 8) | stackmem_lo);
        memory[state.SP()] = hl & 0xFF;
        memory[state.SP() + 1] = hl >> 8;
    }

    inline void PCHL() { state.PC(state.HL()); }

    inline void XCHG() { u16 de = state.DE(); state.DE(state.HL()); state.HL(de); }

    inline void DI() { interrupts_enabled = false; }

    inline void SPHL() { state.SP(state.HL()); }

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