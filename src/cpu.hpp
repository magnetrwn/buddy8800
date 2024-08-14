#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <variant>

#include <iostream>
#include <cstdio>

#include "cpu_state.hpp"
#include "typedef.hpp"

enum class inst_operands {
    NONE, ONE_8, ONE_16, TWO_8
};

class cpu {
private:
    using instruction = std::variant<
        void (*)(cpu_state&),
        void (*)(cpu_state&, u8),
        void (*)(cpu_state&, u16),
        void (*)(cpu_state&, u8, u8)
    >;

    cpu_state state;
    std::array<u8, 0x10000> memory; // TODO: just temporary
    std::array<instruction, 0x100> decode_table;

    constexpr u16 pc() const { return state.get_register16<cpu_registers16::PC>(); }

    template <usize amount = 1>
    constexpr void next_pc() { state.set_register16<cpu_registers16::PC>(pc() + amount); }

    constexpr u8 fetch() { next_pc(); return memory[pc() - 1]; }

    constexpr void execute(const instruction& inst) {
        std::visit([this](auto&& call) {
            using inst_type = std::decay_t<decltype(call)>;

            if constexpr (std::is_same_v<inst_type, void (*)(cpu_state&)>)
                call(state);

            else if constexpr (std::is_same_v<inst_type, void (*)(cpu_state&, u8)>) {
                u8 op = fetch();
                call(state, op);
            }

            else if constexpr (std::is_same_v<inst_type, void (*)(cpu_state&, u16)>) {
                u16 op_h = fetch();
                u16 op_l = fetch();
                call(state, (op_h << 8 | op_l));
            }

            else if constexpr (std::is_same_v<inst_type, void (*)(cpu_state&, u8, u8)>) {
                u8 op_1 = fetch();
                u8 op_2 = fetch();
                call(state, op_1, op_2);
            }
        }, inst);
    }

public:
    /// @brief Processor instructions.
    /// \{

    static void TESTNOP([[maybe_unused]] cpu_state& state) { printf("_TESTNOP\n"); };
    static void TEST1([[maybe_unused]] cpu_state& state, [[maybe_unused]] u8 test1) { printf("_TEST1 %hu\n", test1); };
    static void TEST2([[maybe_unused]] cpu_state& state, [[maybe_unused]] u8 test1, [[maybe_unused]] u8 test2) { printf("_TEST2 %hu %hu\n", test1, test2); };

    /// \}

    void step() {
        auto opcode = fetch();
        std::cout << "Fetched opcode: " << static_cast<unsigned int>(opcode) << std::endl;
        execute(decode_table[opcode]);
    }


    cpu() : state(), memory({}) {
        decode_table.fill(TESTNOP);
        decode_table[1] = TEST2;
        for (int i = 0; i < 8; i++)
            memory[i] = i % 4;
    }
};

#endif