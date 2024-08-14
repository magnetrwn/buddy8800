#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>

#include "cpu_state.hpp"
#include "typedef.hpp"

class cpu {
private:
    cpu_state state;
    std::array<u8, 0x10000> memory; // TODO: just temporary
    std::array<void (cpu::*)(), 256> decode_table;

    constexpr u16 pc() const { return state.get_register16<cpu_registers16::PC>(); }

    template <usize amount = 1>
    constexpr void next_pc() { state.set_register16<cpu_registers16::PC>(pc() + amount); }

    constexpr u8 fetch() { next_pc(); return memory[pc() - 1]; }

    template <void (cpu::*FN)(cpu_state&)>
    constexpr void decode() {
        (this->*FN)(state);
    }

    /*template <void (cpu::*FN)(cpu_state&, u8)>
    constexpr void decode() {
        u8 param1 = fetch();
        (this->*FN)(state, param1);
    }

    template <void (cpu::*FN)(cpu_state&, u16)>
    constexpr void decode() {
        u16 param1 = fetch();
        u16 param2 = fetch();
        (this->*FN)(state, (param2 << 8) | param1);
    }

    template <void (cpu::*FN)(cpu_state&, u8, u8)>
    constexpr void decode() {
        u8 param1 = fetch();
        u8 param2 = fetch();
        (this->*FN)(state, param1, param2);
    }*/

public:
    /// @brief Processor instructions.
    /// \{

    static void _NOP([[maybe_unused]] cpu_state& state) {};
    static void _TEST1([[maybe_unused]] cpu_state& state, [[maybe_unused]] u8 test1) {};
    static void _TEST2([[maybe_unused]] cpu_state& state, [[maybe_unused]] u8 test1, [[maybe_unused]] u8 test2) {};

    /// \}

    constexpr void step() {
        u8 opcode = fetch();
        (this->*decode_table[opcode])();
    }

    cpu() : state(), memory({}) {
        decode_table.fill(decode<&cpu::_NOP>);
    }
};

#endif