#include <catch2/catch_test_macros.hpp>

#include "typedef.hpp"
#include "cpu.hpp"

TEST_CASE("cpu_state") {
    cpu_state state;

    SECTION("registers") {
        // Get and set on 8 bit registers
        state.set_register8<A>(0x05);
        state.set_register8<F>(0xAF);
        REQUIRE(state.get_register8<A>() == 0x05);
        REQUIRE(state.get_register8<F>() == 0xAF);

        // Get a 16 bit register set by two 8 bit setters
        REQUIRE(state.get_register16<AF>() == 0xAF05);

        // Get two 8 bit registers set by a 16 bit setter
        state.set_register16<BC>(0xA55A);
        REQUIRE(state.get_register8<B>() == 0x5A);
        REQUIRE(state.get_register8<C>() == 0xA5);

        // Get and set on 16 bit registers
        state.set_register16<SP>(0xF0F0);
        REQUIRE(state.get_register16<SP>() == 0xF0F0);
    }
}