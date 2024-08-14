#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <variant>

#include <cstdio>

#include "cpu_state.hpp"
#include "typedef.hpp"

class cpu {
private:
    cpu_state state;
    std::array<u8, 0x10000> memory; // TODO: just temporary
    u8 current_opcode;

    constexpr u16 pc() const { return state.get_register16<cpu_registers16::PC>(); }

    template <usize amount = 1>
    constexpr void next_pc() { state.set_register16<cpu_registers16::PC>(pc() + amount); }

    constexpr u8 fetch() { current_opcode = memory[pc()]; next_pc(); return current_opcode; }

    void execute(u8 opcode) {
        printf("PC: 0x%04hX OP: 0x%02hhX ", pc(), opcode);

        switch (opcode) {
            case 0x01: {
                printf("%hhu %hhu\n", fetch(), fetch());
                break;
            }
            default: {
                puts("");
                break;
            }
        }
    }

public:
    void step() {
        execute(fetch());
    }

    cpu() : state(), memory({}), current_opcode(0x00) {
        for (int i = 0; i < 8; i++)
            memory[i] = i % 4;
    }
};

#endif