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

    template <u8 opc>
    constexpr void _debug_ex() {
        printf("PC: %04hX OP: %02hhX (\x1B[33m%s\x1B[0m)\n", pc() - 1, opc, util::get_opcode_str<opc>());
    }

    template <u8 opc>
    constexpr void _debug_ex(u8 oper1) {
        printf("PC: %04hX OP: %02hhX (\x1B[33m%s\x1B[0m | %02hX)\n", pc() - 2, opc, util::get_opcode_str<opc>(), oper1);
    }

    template <u8 opc>
    constexpr void _debug_ex(u8 oper1, u8 oper2) {
        printf("PC: %04hX OP: %02hhX (\x1B[33m%s\x1B[0m | %02hX %02hX)\n", pc() - 3, opc, util::get_opcode_str<opc>(), oper1, oper2);
    }

    constexpr void _debug_ex_error(u8 opc) {
        printf("PC: %04hX OP: %02hhX (\x1B[31;01mUNKNOWN\x1B[0m)\n", pc() - 1, opc);
    }

    void execute(u8 opcode);

public:
    void step() { execute(fetch()); }

    cpu() : state(), memory({}) {
        for (int i = 0; i < 30; i++)
            memory[i] = i / 3;
    }
};

#endif