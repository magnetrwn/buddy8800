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
#include "defines.hpp"

/**
 * @brief Represents the CPU of the emulator.
 *
 * This is the 8080 CPU emulator and interpreter. It abstracts the CPU functionality into straightforward methods
 * and presents a way to step and interact with state. Being an interpreter, each opcode starting from the program
 * counter address in cardbus is fetched and executed according to what it represents and including further
 * fetches to opcode operands. The API also provides public access to execute(), should the user want to manually
 * ask for instructions.
 *
 * @note The class is completely defined in this header to allow templating the class to determine what bus interface
 * `bus_iface` to use. For example, you can:
 * - `cpu<bus&> cpu(cardbus);` or `cpu<> cpu(cardbus);` to create a standard CPU that interacts with a bus, or
 * - `cpu<std::array<u8, 65536>> cpu;` to instead have an empty array as a bus, for max performance!
 */
template <class bus_iface = bus&>
class cpu {
private:
    cpu_state state;
    bus_iface cardbus;
    bool just_booted;
    bool halted;
    bool do_handle_bdos;
    bool interrupts_enabled;
    
    util::print_helper printer;

    /* ~~~~~~~~~~~~~~~ vvv ~~~~~~~~~~~~~~ fetch ~~~~~~~~~~~~~~ vvv ~~~~~~~~~~~~~~~ */

    u8 fetch_default() { return cardbus[state.get_then_inc_register16(cpu_registers16::PC)]; }
    u16 fetch2_default()  { u16 lo = fetch_default(); u16 hi = fetch_default(); return (hi << 8) | lo; }

    u8 ext_op[2];
    bool ext_op_idx;
    u8 fetch_ext() { bool idx = ext_op_idx; ext_op_idx = !ext_op_idx; return ext_op[idx]; }
    u16 fetch2_ext() { u16 lo = fetch_ext(); u16 hi = fetch_ext(); return (hi << 8) | lo; }

    u8 (cpu::*fetch_ptr)() = &cpu::fetch_default;
    u16 (cpu::*fetch2_ptr)() = &cpu::fetch2_default;

    inline void set_fetch_ext(bool use_ext) {
        if constexpr (!std::is_same_v<bus_iface, bus&>) {
            if (use_ext)
                throw std::runtime_error("Cannot use external fetch with non-bus interface.");
            else
                return;
        } else {        
            fetch_ptr = use_ext ? &cpu::fetch_ext : &cpu::fetch_default;
            fetch2_ptr = use_ext ? &cpu::fetch2_ext : &cpu::fetch2_default;
        }
    }

    inline u8 fetch() {
        if constexpr (!std::is_same_v<bus_iface, bus&>)
            return fetch_default();
        else
            return (this->*fetch_ptr)();
    }

    inline u16 fetch2() { 
        if constexpr (!std::is_same_v<bus_iface, bus&>)
            return fetch2_default();
        else
            return (this->*fetch2_ptr)();
    }

    /* ~~~~~~~~~~~~~~~ ^^^ ~~~~~~~~~~~~~~ fetch ~~~~~~~~~~~~~~ ^^^ ~~~~~~~~~~~~~~~ */

    template <usize ops>
    constexpr void _trace([[maybe_unused]] u8 opc) {
        #if defined ENABLE_TRACE or defined ENABLE_TRACE_ESSENTIAL
        if constexpr (ops == 1)
            std::printf("%04hX    %02hhX      \t %s\n", static_cast<unsigned>(state.PC() - 1), static_cast<unsigned>(opc), util::get_opcode_str(opc));
        else if constexpr (ops == 2)
            std::printf("%04hX    %02hhX %02hhX   \t %s\n", static_cast<unsigned>(state.PC() - 1), static_cast<unsigned>(opc), static_cast<unsigned>(cardbus[state.PC()]), util::get_opcode_str(opc));
        else if constexpr (ops == 3)
            std::printf("%04hX    %02hhX %02hhX %02hhX\t %s\n", static_cast<unsigned>(state.PC() - 1), static_cast<unsigned>(opc), static_cast<unsigned>(cardbus[state.PC()]), static_cast<unsigned>(cardbus[state.PC() + 1]), util::get_opcode_str(opc));
        #endif
    }

    constexpr void _trace_state() {
        #ifdef ENABLE_TRACE
        constexpr static const char* CHG = "\x1B[43;01m";
        static cpu_state last_state = state;

        printf("\x1B[44;01mA: %s%02hhX\x1B[44;01m BC: %s%04hX\x1B[44;01m DE: %s%04hX\x1B[44;01m HL: %s%04hX\x1B[44;01m \x1B[0m\n"
            "\x1B[44;01m SP: %s%04hX\x1B[44;01m PC: %s%04hX\x1B[44;01m F: %s%c\x1B[44;01m %s%c\x1B[44;01m %s%c\x1B[44;01m %s%c\x1B[44;01m %s%c\x1B[44;01m  \x1B[0m\n",
            last_state.get_register8(cpu_registers8::A) == state.get_register8(cpu_registers8::A) ? "" : CHG,
            state.get_register8(cpu_registers8::A),
            last_state.get_register16(cpu_registers16::BC) == state.get_register16(cpu_registers16::BC) ? "" : CHG,
            state.get_register16(cpu_registers16::BC),
            last_state.get_register16(cpu_registers16::DE) == state.get_register16(cpu_registers16::DE) ? "" : CHG,
            state.get_register16(cpu_registers16::DE),
            last_state.get_register16(cpu_registers16::HL) == state.get_register16(cpu_registers16::HL) ? "" : CHG,
            state.get_register16(cpu_registers16::HL),
            last_state.get_register16(cpu_registers16::SP) == state.get_register16(cpu_registers16::SP) ? "" : CHG,
            state.get_register16(cpu_registers16::SP),
            last_state.get_register16(cpu_registers16::PC) == state.get_register16(cpu_registers16::PC) ? "" : CHG,
            state.get_register16(cpu_registers16::PC),
            last_state.get_flag(cpu_flags::S) == state.get_flag(cpu_flags::S) ? "" : CHG,
            state.get_flag(cpu_flags::S) ? 'S' : '/',
            last_state.get_flag(cpu_flags::Z) == state.get_flag(cpu_flags::Z) ? "" : CHG,
            state.get_flag(cpu_flags::Z) ? 'Z' : '/',
            last_state.get_flag(cpu_flags::AC) == state.get_flag(cpu_flags::AC) ? "" : CHG,
            state.get_flag(cpu_flags::AC) ? 'A' : '/',
            last_state.get_flag(cpu_flags::P) == state.get_flag(cpu_flags::P) ? "" : CHG,
            state.get_flag(cpu_flags::P) ? 'P' : '/',
            last_state.get_flag(cpu_flags::C) == state.get_flag(cpu_flags::C) ? "" : CHG,
            state.get_flag(cpu_flags::C) ? 'C' : '/');

        last_state = state;
        #endif
    }

    constexpr void _trace_reg16_deref([[maybe_unused]] cpu_registers16 reg) {
        #ifdef ENABLE_TRACE
        printf("\x1B[42;01m(%s: %04hX): %02hhX                   \x1B[0m\n",
            (reg == cpu_registers16::AF) ? "AF" : 
                (reg == cpu_registers16::BC) ? "BC" : 
                (reg == cpu_registers16::DE) ? "DE" : 
                (reg == cpu_registers16::HL) ? "HL" : 
                (reg == cpu_registers16::SP) ? "SP" : 
                "PC",
            state.get_register16(reg),
            cardbus[state.get_register16(reg)]);
        #endif
    }

    /*void _trace_memref16_deref() {
        #ifdef ENABLE_TRACE
        printf("\x1B[41;01m(%02hhX%02hhX): %02hhX                       \x1B[0m\n",
            cardbus[state.PC() + 1],
            cardbus[state.PC()],
            cardbus[(cardbus[state.PC() + 1] << 8) | cardbus[state.PC()]]);
        #endif
    }*/

    constexpr void _trace_stackptr16_deref() {
        #ifdef ENABLE_TRACE
        printf("\x1B[42;01m(%04hX): %02hhX (%04hX): %02hhX            \x1B[0m\n",
            state.get_register16(cpu_registers16::SP),
            cardbus[state.get_register16(cpu_registers16::SP)],
            state.get_register16(cpu_registers16::SP) + 1,
            cardbus[state.get_register16(cpu_registers16::SP) + 1]);
        #endif
    }

    constexpr void _trace_error([[maybe_unused]] u8 opc) {
        #ifdef ENABLE_TRACE
        printf("%04hX    %02hhX      \t \x1B[31;01mUNKNOWN\x1B[0m\n", state.PC() - 1, opc);
        throw std::runtime_error("Unknown opcode hit.");
        #endif
    }

    bool resolve_flag_cond(u8 cc) {
        switch (cc) {
            case 0b000: return !state.get_flag(cpu_flags::Z);
            case 0b001: return state.get_flag(cpu_flags::Z); 
            case 0b010: return !state.get_flag(cpu_flags::C);
            case 0b011: return state.get_flag(cpu_flags::C); 
            case 0b100: return !state.get_flag(cpu_flags::P);
            case 0b101: return state.get_flag(cpu_flags::P); 
            case 0b110: return !state.get_flag(cpu_flags::S);
            case 0b111: return state.get_flag(cpu_flags::S);
            default: return false;
        }
    }

    void handle_bdos() {
        if (state.PC() == 0x0000) {
            if (just_booted) {

                #ifdef ENABLE_TRACE
                puts("\x1B[47;01mBDOS 0x0000: Reset vector!       \x1B[0m");
                #endif

                just_booted = false;
                return;
            }

            #ifdef ENABLE_TRACE
            puts("\x1B[47;01mBDOS 0x0000: That's all folks!   \x1B[0m");
            #endif

            cardbus[0] = 0b01110110;
        }

        if (state.PC() == 0x0005) {
            u8 c = state.C();

            #ifdef ENABLE_TRACE
            puts("\x1B[47;01mBDOS 0x0005: Wants to print:     \x1B[0m");
            #endif

            #ifdef ENABLE_TRACE_ESSENTIAL
            puts("\x1B[33;01m");
            #endif

            if (c == 0x02)
                printer << state.E();

            else if (c == 0x09)
                for (u16 de = state.DE(); cardbus[de] != '$'; ++de)
                    printer << cardbus[de];

            else
                throw std::runtime_error("Unknown BDOS 0x0005 call parameters.");

            fetch();

            #ifdef ENABLE_TRACE
            putchar('\n');
            _trace<1>(0b11001001); 
            #endif

            #ifdef ENABLE_TRACE_ESSENTIAL
            puts("\x1B[0m");
            #endif
            
            RETURN();
        }
    }

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

    inline void OUT() { 
        if constexpr (!std::is_same_v<bus_iface, bus&>)
            throw std::runtime_error("OUT instruction can only be used with a bus interface.");
        else {
            u16 port = fetch(); port |= (port << 8); cardbus.write(port, state.A(), true);
        }
    }

    inline void IN() {
        if constexpr (!std::is_same_v<bus_iface, bus&>)
            throw std::runtime_error("IN instruction can only be used with a bus interface.");
        else {
            u16 port = fetch(); port |= (port << 8); state.A(cardbus.read(port, true));
        }
    }

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
     * @param steps The number of steps to forward the CPU by.
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
    void step(usize steps = 1) {
        for (usize i = 0; i < steps; ++i) {
            if (halted)
                return;
            if (do_handle_bdos)
                handle_bdos();
            execute(fetch());
        }
    }

    /**
     * @brief Executes a single opcode with one or two operands.
     * @param opcode The opcode to execute.
     * @param operand1 The first operand to the opcode.
     * @param operand2 The second operand to the opcode.
     *
     * This is an overload that manipulates the way the fetch cycles interact in the CPU logic: it will use
     * the operands retrieved by this method (usually to be placed by a device on the bus that called an IRQ)
     * instead of fetching them from the bus cards. In fact, this is used by the interrupt method as well.
     */
    void execute(u8 opcode, u8 operand1, u8 operand2 = 0) {
        ext_op[0] = operand1;
        ext_op[1] = operand2;
        set_fetch_ext(true);
        execute(opcode);
        set_fetch_ext(false);
    }

    /**
     * @brief Executes a single opcode.
     * @param opcode The opcode to execute.
     *
     * This method is a comprehensive jump table for all the opcodes the 8080 CPU can execute. It effectively groups the
     * decode and execute phases of the CPU into a single method, and is used by the step method to execute instructions.
     *
     * @note To allow multiple operand instructions, there is an overload of this method that takes one or two extra
     * argument bytes and temporarily redirects `fetch()` and `fetch2()` to them during the instruction cycle!
     */
    void execute(u8 opcode) {
        cpu_registers16 pair_sel = static_cast<cpu_registers16>((((opcode & 0b00110000) >> 4) & 0b11) + 1);
        cpu_registers8 dst_sel = cpu_reg8_decode[(opcode >> 3) & 0b111];
        cpu_registers8 src_sel = cpu_reg8_decode[opcode & 0b111];

        switch (opcode) {
            case 0b00000000: _trace<1>(opcode); NOP(); 
            break;
            
            //     ..RP....
            case 0b00000001: 
            case 0b00010001:
            case 0b00100001:
            case 0b00110001: _trace<3>(opcode); LXI(pair_sel); _trace_state(); 
            break;

            //     ..RP....
            case 0b00000010:
            case 0b00010010: _trace<1>(opcode); STAX(pair_sel); _trace_reg16_deref(pair_sel); 
            break;

            //     ..RP....
            case 0b00000011:
            case 0b00010011:
            case 0b00100011:
            case 0b00110011: _trace<1>(opcode); INX(pair_sel); _trace_state(); 
            break;

            //     ..DDD...
            case 0b00000100:
            case 0b00001100:
            case 0b00010100:
            case 0b00011100:
            case 0b00100100:
            case 0b00101100:
            case 0b00111100: 
            case 0b00110100: _trace<1>(opcode); INR(dst_sel); _trace_state(); 
                            if (is_memref(dst_sel)) _trace_reg16_deref(cpu_registers16::HL); 
            break;

            //     ..DDD...
            case 0b00000101:
            case 0b00001101:
            case 0b00010101:
            case 0b00011101:
            case 0b00100101:
            case 0b00101101:
            case 0b00111101: 
            case 0b00110101: _trace<1>(opcode); DCR(dst_sel); _trace_state(); 
                            if (is_memref(dst_sel)) _trace_reg16_deref(cpu_registers16::HL); 
            break;

            //     ..DDD...
            case 0b00000110:
            case 0b00001110:
            case 0b00010110:
            case 0b00011110:
            case 0b00100110:
            case 0b00101110:
            case 0b00111110: 
            case 0b00110110: _trace<2>(opcode); MVI(dst_sel); _trace_state(); 
                            if (is_memref(dst_sel)) _trace_reg16_deref(cpu_registers16::HL);          
            break;
            
            //     ..RP....
            case 0b00001001:
            case 0b00011001:
            case 0b00101001:
            case 0b00111001: _trace<1>(opcode); DAD(pair_sel); _trace_state();
            break;

            //     ..RP....
            case 0b00001010:
            case 0b00011010: _trace<1>(opcode); LDAX(pair_sel); _trace_state(); _trace_reg16_deref(pair_sel); 
            break;

            //     ..RP....
            case 0b00001011:
            case 0b00011011:
            case 0b00101011:
            case 0b00111011: _trace<1>(opcode); DCX(pair_sel); _trace_state(); 
            break;

            case 0b00000111: _trace<1>(opcode); RLC(); _trace_state(); 
            break;

            case 0b00001111: _trace<1>(opcode); RRC(); _trace_state(); 
            break;

            case 0b00010111: _trace<1>(opcode); RAL(); _trace_state(); 
            break;

            case 0b00011111: _trace<1>(opcode); RAR(); _trace_state(); 
            break;

            case 0b00100010: _trace<3>(opcode); SHLD(); _trace_state(); //_trace_mem16_deref(); 
            break;

            case 0b00100111: _trace<1>(opcode); DAA(); _trace_state(); 
            break;

            case 0b00101010: _trace<3>(opcode); LHLD(); _trace_state(); //_trace_mem16_deref(); 
            break;

            case 0b00101111: _trace<1>(opcode); CMA(); _trace_state(); 
            break;

            case 0b00110010: _trace<3>(opcode); STA(); _trace_state(); //_trace_mem16_deref(); 
            break;

            case 0b00110111: _trace<1>(opcode); STC(); _trace_state(); 
            break;

            case 0b00111010: _trace<3>(opcode); LDA(); _trace_state(); //_trace_mem16_deref(); 
            break;

            case 0b00111111: _trace<1>(opcode); CMC(); _trace_state(); 
            break;

            //     ..DDDSSS
            case 0b01000000:
            case 0b01000001:
            case 0b01000010:
            case 0b01000011:
            case 0b01000100:
            case 0b01000101:
            case 0b01000111:
            case 0b01001000:
            case 0b01001001:
            case 0b01001010:
            case 0b01001011:
            case 0b01001100:
            case 0b01001101:
            case 0b01001111:
            case 0b01010000:
            case 0b01010001:
            case 0b01010010:
            case 0b01010011:
            case 0b01010100:
            case 0b01010101:
            case 0b01010111:
            case 0b01011000:
            case 0b01011001:
            case 0b01011010:
            case 0b01011011:
            case 0b01011100:
            case 0b01011101:
            case 0b01011111:
            case 0b01100000:
            case 0b01100001:
            case 0b01100010:
            case 0b01100011:
            case 0b01100100:
            case 0b01100101:
            case 0b01100111:
            case 0b01101000:
            case 0b01101001:
            case 0b01101010:
            case 0b01101011:
            case 0b01101100:
            case 0b01101101:
            case 0b01101111:
            case 0b01111000:
            case 0b01111001:
            case 0b01111010:
            case 0b01111011:
            case 0b01111100:
            case 0b01111101:
            case 0b01111111:
            case 0b01000110:
            case 0b01001110:
            case 0b01010110:
            case 0b01011110:
            case 0b01100110:
            case 0b01101110:
            case 0b01111110:
            case 0b01110000:
            case 0b01110001:
            case 0b01110010:
            case 0b01110011:
            case 0b01110100:
            case 0b01110101:
            case 0b01110111: _trace<1>(opcode); MOV(dst_sel, src_sel); _trace_state();
                            if (is_memref(dst_sel) or is_memref(src_sel)) _trace_reg16_deref(cpu_registers16::HL); 
            break;
            
            case 0b01110110: _trace<1>(opcode); HLT(); 
            break;

            //     ..ALUSSS
            case 0b10000000:
            case 0b10000001:
            case 0b10000010:
            case 0b10000011:
            case 0b10000100:
            case 0b10000101:
            case 0b10000111:
            case 0b10001000:
            case 0b10001001:
            case 0b10001010:
            case 0b10001011:
            case 0b10001100:
            case 0b10001101:
            case 0b10001111:
            case 0b10010000:
            case 0b10010001:
            case 0b10010010:
            case 0b10010011:
            case 0b10010100:
            case 0b10010101:
            case 0b10010111:
            case 0b10011000:
            case 0b10011001:
            case 0b10011010:
            case 0b10011011:
            case 0b10011100:
            case 0b10011101:
            case 0b10011111:
            case 0b10100000:
            case 0b10100001:
            case 0b10100010:
            case 0b10100011:
            case 0b10100100:
            case 0b10100101:
            case 0b10100111:
            case 0b10101000:
            case 0b10101001:
            case 0b10101010:
            case 0b10101011:
            case 0b10101100:
            case 0b10101101:
            case 0b10101111:
            case 0b10111000:
            case 0b10111001:
            case 0b10111010:
            case 0b10111011:
            case 0b10111100:
            case 0b10111101:
            case 0b10111111:
            case 0b10000110:
            case 0b10001110:
            case 0b10010110:
            case 0b10011110:
            case 0b10100110:
            case 0b10101110:
            case 0b10111110:
            case 0b10110000:
            case 0b10110001:
            case 0b10110010:
            case 0b10110011:
            case 0b10110100:
            case 0b10110101:
            case 0b10110111:
            case 0b10110110: _trace<1>(opcode); ALU(src_sel, (opcode >> 3) & 0b111); _trace_state();
            break;

            //     ..CCC...
            case 0b11000000:
            case 0b11001000:
            case 0b11010000:
            case 0b11011000:
            case 0b11100000:
            case 0b11101000:
            case 0b11110000:
            case 0b11111000: _trace<1>(opcode); RETURN_ON((opcode >> 3) & 0b111); _trace_state();
            break;

            //     ..RP....
            case 0b11000001:
            case 0b11010001:
            case 0b11100001:
            case 0b11110001: _trace<1>(opcode); POP(pair_sel); _trace_state();
            break;

            //     ..CCC...
            case 0b11000010:
            case 0b11001010:
            case 0b11010010:
            case 0b11011010:
            case 0b11100010:
            case 0b11101010:
            case 0b11110010:
            case 0b11111010: _trace<3>(opcode); JUMP_ON((opcode >> 3) & 0b111); _trace_state();
            break;

            case 0b11000011: _trace<3>(opcode); JMP(); _trace_state();
            break;

            //     ..CCC...
            case 0b11000100:
            case 0b11001100:
            case 0b11010100:
            case 0b11011100:
            case 0b11100100:
            case 0b11101100:
            case 0b11110100:
            case 0b11111100: _trace<3>(opcode); CALL_ON((opcode & 0b00111000) >> 3); _trace_state();
            break;

            //     ..RP....
            case 0b11000101:
            case 0b11010101:
            case 0b11100101:
            case 0b11110101: _trace<1>(opcode); PUSH(pair_sel); _trace_state();
            break;

            //     ..ALU...
            case 0b11000110:
            case 0b11001110:
            case 0b11010110:
            case 0b11011110:
            case 0b11100110:
            case 0b11101110:
            case 0b11110110:
            case 0b11111110: _trace<2>(opcode); ALU_IMM((opcode >> 3) & 0b111); _trace_state();
            break;

            //     ..NNN...
            case 0b11000111:
            case 0b11001111:
            case 0b11010111:
            case 0b11011111:
            case 0b11100111:
            case 0b11101111:
            case 0b11110111:
            case 0b11111111: _trace<1>(opcode); RST((opcode >> 3) & 0b111); _trace_state();
            break;

            case 0b11001001: _trace<1>(opcode); RETURN(); _trace_state();
            break;

            case 0b11001101: _trace<3>(opcode); CALL(); _trace_state();
            break;

            case 0b11010011: _trace<2>(opcode); OUT();
            break;

            case 0b11011011: _trace<2>(opcode); IN();
            break;

            case 0b11100011: _trace<1>(opcode); XTHL(); _trace_state(); _trace_stackptr16_deref();
            break;

            case 0b11101001: _trace<1>(opcode); PCHL(); _trace_state();
            break;

            case 0b11101011: _trace<1>(opcode); XCHG(); _trace_state();
            break;

            case 0b11110011: _trace<1>(opcode); DI();
            break;

            case 0b11111001: _trace<1>(opcode); SPHL(); _trace_state();
            break;

            case 0b11111011: _trace<1>(opcode); EI();
            break;

            default:         _trace_error(opcode); 
            break;
        }
    }

    /**
     * @brief Loads data into cardbus.
     * @param begin Iterator to the beginning of data.
     * @param end Iterator to the end of data.
     * @param offset Offset to try to start loading at.
     * @param auto_reset_vector Whether the zero page should point to the start of loaded data automatically.
     * @throw `std::out_of_range` if data won't fit in cardbus.
     * @throw `std::out_of_range` if enabled auto reset vector will overwrite the first few bytes loaded.
     *
     * This method can be useful to load programs, libraries, or data into the emulator's cardbus. It will copy the data
     * at the specified offset, check if it will fit, and if auto_reset_vector is true, it will set the zero page (the first
     * 3 bytes of cardbus, specifically a jump instruction and a 2 byte argument) to point to the start of the loaded data.
     *
     * @note This method calls `write_force()` on the cards when using a bus, so it can also write to locked ROM
     * to setup the system roms.
     */
    template <typename T, T_ITERATOR_SFINAE>
    void load(T begin, T end, usize offset = 0, bool auto_reset_vector = false) {
        usize dist = std::distance(begin, end);

        if (dist > cardbus.size() - offset)
            throw std::out_of_range("Not enough space in emulated cardbus.");

        if constexpr (std::is_same_v<bus_iface, bus&>)
            for (usize i = 0; i < dist; ++i)
                cardbus.write_force(offset + i, *(begin + i));
        else
            for (usize i = 0; i < dist; ++i)
                cardbus[offset + i] = *(begin + i);

        if (auto_reset_vector) {
            if (offset <= 2)
                throw std::out_of_range("First program bytes will be overwritten by reset vector.");

            cardbus[0] = 0xC3;
            cardbus[1] = static_cast<u8>(offset & 0xFF);
            cardbus[2] = static_cast<u8>(offset >> 8);
        }
    }

    /**
     * @brief Load CPU state.
     * @param new_state The state to load.
     *
     * This method can set a new state to the CPU, including all registers and flags of it.
     */
    void load_state(const cpu_state& new_state) { state = new_state; }

    /**
     * @brief Save CPU state.
     * @return The current state of the CPU.
     *
     * This method will return the current state of the CPU, including all registers and flags of it.
     */
    cpu_state save_state() const { return state; }

    /// @brief Check if the CPU is halted.
    /// @return True if the CPU is halted, false otherwise.
    bool is_halted() const { return halted; }

    /// @brief Reset the CPU.
    void clear() {
        state = cpu_state();
        just_booted = true;
        halted = false;
    }

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
    void do_pseudo_bdos(bool should) { do_handle_bdos = should; }

    /// @brief Redirect pseudo BDOS print routines to a file.
    /// @param filename The name of the file to print to.
    void set_pseudo_bdos_redirect(const char* filename) { printer.set(filename); }

    /// @brief Redirect pseudo BDOS print routines back to stdout.
    void reset_pseudo_bdos_redirect() { printer.reset(); }

    /// \}
    /// @name Interrupt related methods.
    /// \{

    /// @brief Call an interrupt and push PC, then disable interrupts.
    /// @param inst The interrupt instruction (with optional operands) to execute out of place.
    /// @todo This allows any instruction and any retrieval of arguments, but I'm not sure if that happens other than on `CALL`.
    void interrupt(std::array<u8, 3> inst) {
        if (!interrupts_enabled)
            return;

        interrupts_enabled = false;
        PUSH(cpu_registers16::PC);
        execute(inst[0], inst[1], inst[2]);
    }

    /// \}

    cpu(bus_iface init_adr_space) : state(), cardbus(init_adr_space), just_booted(true), halted(false), do_handle_bdos(false), 
            interrupts_enabled(true), printer(std::cout), ext_op_idx(false) {}
};

#endif