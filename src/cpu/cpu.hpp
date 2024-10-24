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
#include "bus.hpp"

/**
 * @brief Represents the CPU of the emulator.
 *
 * This is the 8080 CPU emulator and interpreter. It abstracts the CPU functionality into straightforward methods
 * and presents a way to step and interact with state. Being an interpreter, each opcode starting from the program
 * counter address in cardbus is fetched and executed according to what it represents and including further
 * fetches to opcode operands. The API also provides public access to execute(), should the user want to manually
 * ask for instructions.
 *
 * For a use case example you can look at `ŧests/test_cpu.hpp`.
 */
class cpu {
private:
    cpu_state state;
    bus& cardbus; // <-- You can in-place an array of memory, since it's conveniently using the [] operator!
    bool just_booted;
    bool halted;
    bool do_handle_bdos;
    bool interrupts_enabled;
    
    util::print_helper printer;

    inline u8 fetch() { return cardbus[state.get_then_inc_register16(cpu_registers16::PC)]; }
    inline u16 fetch2() { u16 lo = fetch(); u16 hi = fetch(); return (hi << 8) | lo; }

    template <usize ops>
    void _trace([[maybe_unused]] u8 opc) {
        #if defined ENABLE_TRACE or defined ENABLE_TRACE_ESSENTIAL
        if constexpr (ops == 1)
            printf("%04hX    %02hhX      \t %s\n", state.PC() - 1, opc, util::get_opcode_str(opc));
        else if constexpr (ops == 2)
            printf("%04hX    %02hhX %02hhX   \t %s\n", state.PC() - 1, opc, cardbus[state.PC()], util::get_opcode_str(opc));
        else if constexpr (ops == 3)
            printf("%04hX    %02hhX %02hhX %02hhX\t %s\n", state.PC() - 1, opc, cardbus[state.PC()], cardbus[state.PC() + 1], util::get_opcode_str(opc));
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
    /// \{

    inline void NOP() {}

    inline void LXI(cpu_registers16 pair) { state.set_register16(pair, fetch2()); }

    inline void STAX(cpu_registers16 pair) { cardbus[state.get_register16(pair)] = state.A(); }

    inline void INX(cpu_registers16 pair) { state.inc_register16(pair); }

    inline void INR(cpu_registers8 reg) {
        if (is_memref(reg)) return _INR_M();
        state.inc_register8(reg);
        u8 value = state.get_register8(reg);
        state.flgAC((value ^ (value - 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void _INR_M() { 
        u8 value = ++cardbus[state.HL()];
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
        u8 value = --cardbus[state.HL()];
        state.flgAC(~(value ^ (value + 1)) & 0x10);
        state.set_Z_S_P_flags(value);
    }

    inline void MVI(cpu_registers8 reg) { if (is_memref(reg)) return _MVI_M(); state.set_register8(reg, fetch()); }

    inline void _MVI_M() { cardbus[state.HL()] = fetch(); }

    inline void DAD(cpu_registers16 pair) {
        u32 hl = state.HL();
        u32 pval = state.get_register16(pair);
        u32 result = hl + pval;
        state.flgC(result & 0xFFFF0000);
        state.HL(result);
    }

    inline void LDAX(cpu_registers16 pair) { state.A(cardbus[state.get_register16(pair)]); }

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

    inline void SHLD() { u16 adr = fetch2(); cardbus[adr] = state.L(); cardbus[adr + 1] = state.H(); }

    inline void DAA() {
        u16 a = state.A();
        a += 0x06 * ((a & 0x0F) > 0x09 or state.flgAC());
        a += 0x60 * ((a & 0xF0) > 0x90 or state.flgC());
        state.flgAC((a ^ state.A()) & 0x10);
        state.flgC(a & 0x100);
        state.A(a);
        state.set_Z_S_P_flags(a);
    }

    inline void LHLD() { u16 adr = fetch2(); state.L(cardbus[adr]); state.H(cardbus[adr + 1]); }

    inline void CMA() { state.A(~state.A()); }

    inline void STA() { cardbus[fetch2()] = state.A(); }

    inline void STC() { state.flgC(true); }

    inline void LDA() { state.A(cardbus[fetch2()]); }

    inline void CMC() { state.flgC(!state.flgC()); }

    inline void MOV(cpu_registers8 dst, cpu_registers8 src) { 
        if (is_memref(src)) return _MOV_FROM_M(dst); 
        if (is_memref(dst)) return _MOV_TO_M(src); 
        state.set_register8(dst, state.get_register8(src)); 
    }

    inline void _MOV_FROM_M(cpu_registers8 dst) { state.set_register8(dst, cardbus[state.HL()]); }

    inline void _MOV_TO_M(cpu_registers8 src) { cardbus[state.HL()] = state.get_register8(src); }

    inline void HLT() { halted = true; }

    inline void _ALU(cpu_registers8 src, u8 alu, bool is_immediate) {
        u16 a = state.A();
        u16 with = (is_immediate) ? fetch() : ((is_memref(src)) ? cardbus[state.HL()] : state.get_register8(src));
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
                state.flgAC((a | with) & 0x08);
                break;
            case 0b101:
            case 0b110:
                result = (alu == 0b101) ? (a ^ with) : (a | with);
                state.flgAC(false);
                break;
            case 0b111: 
                result = a - with;
                state.flgAC((a & 0x0F) >= (with & 0x0F));
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
        u16 lo = cardbus[state.SP()];
        u16 hi = cardbus[state.SP() + 1];
        state.set_register16(pair, (hi << 8) | lo);
        state.SP(state.SP() + 2);
    }

    inline void JUMP_ON(u8 cc) { if (resolve_flag_cond(cc)) return JMP(); fetch2(); }

    inline void JMP() { state.PC(fetch2()); }

    inline void CALL_ON(u8 cc) { if (resolve_flag_cond(cc)) return CALL(); fetch2(); }

    inline void PUSH(cpu_registers16 pair) {
        if (pair == cpu_registers16::SP) pair = cpu_registers16::AF;
        state.SP(state.SP() - 2);
        cardbus[state.SP()] = state.get_register16(pair) & 0xFF;
        cardbus[state.SP() + 1] = state.get_register16(pair) >> 8;
        //_trace_stackptr16_deref();
    }

    inline void RST(u8 n) { PUSH(cpu_registers16::PC); state.PC(n * 8); }

    inline void RETURN() { POP(cpu_registers16::PC); }

    inline void CALL() { u16 adr = fetch2(); PUSH(cpu_registers16::PC); state.PC(adr); }

    inline void OUT() { throw std::runtime_error("OUT not implemented."); }

    inline void IN() { throw std::runtime_error("IN not implemented."); }

    inline void XTHL() {
        u16 stackmem_lo = cardbus[state.SP()];
        u16 stackmem_hi = cardbus[state.SP() + 1];
        u16 hl = state.HL();
        state.HL((stackmem_hi << 8) | stackmem_lo);
        cardbus[state.SP()] = hl & 0xFF;
        cardbus[state.SP() + 1] = hl >> 8;
    }

    inline void PCHL() { state.PC(state.HL()); }

    inline void XCHG() { u16 de = state.DE(); state.DE(state.HL()); state.HL(de); }

    inline void DI() { interrupts_enabled = false; }

    inline void SPHL() { state.SP(state.HL()); }

    inline void EI() { interrupts_enabled = true; }

    /// \}

public:
    /// @name CPU API methods.
    /// \{

    /**
     * @brief Steps the CPU by one instruction (and its operands).
     *
     * Calling the step method will fetch the next instruction opcode from cardbus and execute it. Internally, on every
     * fetch the PC is incremented, and each instruction is also responsible for fetching its operands, so a step is
     * effectively a full instruction step.
     *
     * If the CPU is set to handle BDOS calls, it will provide some pseudo BDOS functionality, currently only for printing
     * characters and strings to wherever the printer is set to, stdout by default.
     *
     * @note If the CPU is halted, this method will return immediately.
     * @par
     * @note Internally, this method is calling `execute(fetch())`.
     */
    void step();

    /**
     * @brief Executes a single opcode.
     * @param opcode The opcode to execute.
     *
     * This method is a comprehensive jump table for all the opcodes the 8080 CPU can execute. It effectively groups the
     * decode and execute phases of the CPU into a single method, and is used by the step method to execute instructions.
     *
     * @todo Currently, this method is useless for instructions with operands, since their implementation always fetches from
     * the address bus the CPU is aware of.
     */
    void execute(u8 opcode);

    /**
     * @brief Loads data into cardbus.
     * @param begin Vector iterator to the beginning of data.
     * @param end Vector iterator to the end of data.
     * @param offset Offset to try to start loading at.
     * @param auto_reset_vector Whether the zero page should point to the start of loaded data automatically.
     * @throw `std::out_of_range` if data won't fit in cardbus.
     * @throw `std::out_of_range` if enabled auto reset vector will overwrite the first few bytes loaded.
     *
     * This method can be useful to load programs, libraries, or data into the emulator's cardbus. It will copy the data
     * at the specified offset, check if it will fit, and if auto_reset_vector is true, it will set the zero page (the first
     * 3 bytes of cardbus, specifically a jump instruction and a 2 byte argument) to point to the start of the loaded data.
     */
    void load(std::vector<u8>::iterator begin, std::vector<u8>::iterator end, usize offset = 0, bool auto_reset_vector = false);

    /**
     * @brief Load CPU state.
     * @param new_state The state to load.
     *
     * This method can set a new state to the CPU, including all registers and flags of it.
     */
    void load_state(const cpu_state& new_state);

    /**
     * @brief Save CPU state.
     * @return The current state of the CPU.
     *
     * This method will return the current state of the CPU, including all registers and flags of it.
     */
    cpu_state save_state() const;

    /**
     * @brief Check if the CPU is halted.
     * @return True if the CPU is halted, false otherwise.
     */
    bool is_halted() const;

    void clear();

    /// \}
    /// @name Test related methods.
    /// \{

    /**
     * @brief Set the CPU to resolve calls to BDOS internally.
     * @param should Whether the CPU should handle BDOS calls.
     *
     * This method enables pseudo BDOS functionality by the CPU itself. This can be essential for testing the CPU,
     * as most diagnostic programs expect the system to be able to print messages.
     */
    void do_pseudo_bdos(bool should);

    /**
     * @brief Redirect pseudo BDOS print routines to a file.
     * @param filename The name of the file to print to.
     */
    void set_pseudo_bdos_redirect(const char* filename);

    /**
     * @brief Redirect pseudo BDOS print routines back to stdout.
     */
    void reset_pseudo_bdos_redirect();

    /// \}

    cpu(bus& cardbus) : state(), cardbus(cardbus), just_booted(true), halted(false), do_handle_bdos(false), 
            interrupts_enabled(true), printer(std::cout) {}
};

#endif