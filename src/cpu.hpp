#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <cstdio>

#include "cpu_state.hpp"
#include "typedef.hpp"

enum class inst_operands {
    NONE, ONE_8, ONE_16, TWO_8
};

struct instruction {
    inst_operands ops;
    union {
        void (*i0)(cpu_state&);
        void (*i1)(cpu_state&, u8);
        void (*i2)(cpu_state&, u16);
        void (*i3)(cpu_state&, u8, u8);
    } function;
};

class cpu {
private:
    cpu_state state;
    std::array<u8, 0x10000> memory; // TODO: just temporary
    std::array<instruction, 256> decode_table;

    constexpr u16 pc() const { return state.get_register16<cpu_registers16::PC>(); }

    template <usize amount = 1>
    constexpr void next_pc() { state.set_register16<cpu_registers16::PC>(pc() + amount); }

    constexpr u8 fetch() { next_pc(); return memory[pc() - 1]; }

    void execute(const instruction& inst) {
        switch (inst.ops) {
            case inst_operands::NONE: {
                (*inst.function.i0)(state); 
                break;
            }
            case inst_operands::ONE_8: {
                u8 op = fetch();
                (*inst.function.i1)(state, op);
                break;
            }
            case inst_operands::ONE_16: {
                u16 op_h = fetch();
                u16 op_l = fetch();
                (*inst.function.i2)(state, op_h << 8 | op_l);
                break;
            }
            case inst_operands::TWO_8: {
                u8 op_1 = fetch();
                u8 op_2 = fetch();
                (*inst.function.i3)(state, op_1, op_2);
                break;
            }
        }
    }

public:
    /// @brief Processor instructions.
    /// \{

    #define TESTNOP instruction{ inst_operands::NONE, { .i0 = _TESTNOP }}
    static void _TESTNOP([[maybe_unused]] cpu_state& state) { printf("_TESTNOP\n"); };

    #define TEST1 instruction{ inst_operands::ONE_8, { .i1 = _TEST1 } }
    static void _TEST1([[maybe_unused]] cpu_state& state, [[maybe_unused]] u8 test1) { printf("_TEST1 %hu\n", test1); };

    #define TEST2 instruction{ inst_operands::TWO_8, { .i3 = _TEST2 } }
    static void _TEST2([[maybe_unused]] cpu_state& state, [[maybe_unused]] u8 test1, [[maybe_unused]] u8 test2) { printf("_TEST2 %hu %hu\n", test1, test2); };

    /// \}

    void step() { execute(decode_table[fetch()]); }

    cpu() : state(), memory({}) {
        decode_table.fill(TESTNOP);
        decode_table[1] = TEST2;
        for (int i = 0; i < 8; i++)
            memory[i] = i % 4;
    }
};

#endif