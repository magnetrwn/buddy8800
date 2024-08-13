#include <catch2/catch_test_macros.hpp>

#include "typedef.hpp"
#include "cpu.hpp"

TEST_CASE("cpu_state") {
    cpu_state state;

    SECTION("Testing get and set working in-between 8 and 16 bit requests.") {
        // Get and set on 8 bit registers
        state.set_register8<A>(0x12);
        state.set_register8<F>(0x34);
        REQUIRE(state.get_register8<A>() == 0x12);
        REQUIRE(state.get_register8<F>() == 0x34);

        // Get a 16 bit register set by two 8 bit setters
        REQUIRE(state.get_register16<AF>() == 0x3412);

        // Get two 8 bit registers set by a 16 bit setter
        state.set_register16<AF>(0xABCD);
        REQUIRE(state.get_register8<A>() == 0xCD);
        REQUIRE(state.get_register8<F>() == 0xAB);

        // Get and set on 16 bit registers
        state.set_register16<AF>(0x05AF);
        REQUIRE(state.get_register16<AF>() == 0x05AF);
    }

    SECTION("Testing get and set not modifying unrelated register space.") {
        // Set all register pairs to a known value using 16 bit setters
        state.set_register16<AF>(0x0000);
        state.set_register16<BC>(0x5555);
        state.set_register16<DE>(0xAAAA);
        state.set_register16<HL>(0xFFFF);
        state.set_register16<SP>(0x0000);
        state.set_register16<PC>(0x5555);

        // Set the high half of all register pairs to a known value using 8 bit setters
        state.set_register8<A>(0x12);
        state.set_register8<B>(0x34);
        state.set_register8<D>(0x56);
        state.set_register8<H>(0x78);
        state.set_register8<HIGH_SP>(0x9A);
        state.set_register8<HIGH_PC>(0xBC);

        // Check that the low half of all register pairs is still the same
        REQUIRE(state.get_register16<AF>() == 0x0012);
        REQUIRE(state.get_register16<BC>() == 0x5534);
        REQUIRE(state.get_register16<DE>() == 0xAA56);
        REQUIRE(state.get_register16<HL>() == 0xFF78);
        REQUIRE(state.get_register16<SP>() == 0x009A);
        REQUIRE(state.get_register16<PC>() == 0x55BC);

        // Set the low half of all register pairs to a known value using 8 bit setters
        state.set_register8<F>(0xFE);
        state.set_register8<C>(0xDC);
        state.set_register8<E>(0xBA);
        state.set_register8<L>(0x98);
        state.set_register8<LOW_SP>(0x76);
        state.set_register8<LOW_PC>(0x54);

        // Check that the high half of all register pairs is still the same
        REQUIRE(state.get_register16<AF>() == 0xFE12);
        REQUIRE(state.get_register16<BC>() == 0xDC34);
        REQUIRE(state.get_register16<DE>() == 0xBA56);
        REQUIRE(state.get_register16<HL>() == 0x9878);
        REQUIRE(state.get_register16<SP>() == 0x769A);
        REQUIRE(state.get_register16<PC>() == 0x54BC);
    }
}