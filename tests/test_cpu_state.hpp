#include <catch2/catch_test_macros.hpp>

#include "cpu_state.hpp"

TEST_CASE("CPU state checks", "[cpu_state]") {
    cpu_state state;

    SECTION("Check state is generally zeroed, but not on bit 1 of F, on construction.") {
        
        REQUIRE(state.get_register16(cpu_registers16::AF) == 0x0002);
        REQUIRE(state.get_register16(cpu_registers16::BC) == 0x0000);
        REQUIRE(state.get_register16(cpu_registers16::DE) == 0x0000);
        REQUIRE(state.get_register16(cpu_registers16::HL) == 0x0000);
        REQUIRE(state.get_register16(cpu_registers16::SP) == 0x0000);
        REQUIRE(state.get_register16(cpu_registers16::PC) == 0x0000);
    }

    SECTION("Testing get and set working in-between 8 and 16 bit requests.") {
                
        // Get and set on 8 bit registers
        state.set_register8(cpu_registers8::B, 0x12);
        state.set_register8(cpu_registers8::C, 0x34);
        REQUIRE(state.get_register8(cpu_registers8::B) == 0x12);
        REQUIRE(state.get_register8(cpu_registers8::C) == 0x34);

        // Get a 16 bit register set by two 8 bit setters
        REQUIRE(state.get_register16(cpu_registers16::BC) == 0x1234);

        // Get two 8 bit registers set by a 16 bit setter
        state.set_register16(cpu_registers16::BC, 0xABCD);
        REQUIRE(state.get_register8(cpu_registers8::B) == 0xAB);
        REQUIRE(state.get_register8(cpu_registers8::C) == 0xCD);

        // Get and set on 16 bit registers
        state.set_register16(cpu_registers16::BC, 0x05AF);
        REQUIRE(state.get_register16(cpu_registers16::BC) == 0x05AF);
    }

    SECTION("Testing get and set not modifying unrelated register space.") {
                        
        // Set all register pairs to a known value using 16 bit setters
        state.set_register16(cpu_registers16::AF, 0x0000);
        state.set_register16(cpu_registers16::BC, 0x5555);
        state.set_register16(cpu_registers16::DE, 0xAAAA);
        state.set_register16(cpu_registers16::HL, 0xFFFF);
        state.set_register16(cpu_registers16::SP, 0x0000);
        state.set_register16(cpu_registers16::PC, 0x5555);

        // Set the high half of all register pairs to a known value using 8 bit setters
        state.set_register8(cpu_registers8::A, 0x12);
        state.set_register8(cpu_registers8::B, 0x34);
        state.set_register8(cpu_registers8::D, 0x56);
        state.set_register8(cpu_registers8::H, 0x78);
        state.set_register8(cpu_registers8::HIGH_SP, 0x9A);
        state.set_register8(cpu_registers8::HIGH_PC, 0xBC);

        // Check that the low half of all register pairs is still the same
        REQUIRE(state.get_register16(cpu_registers16::AF) == 0x1202); // F will have 0x02 set!
        REQUIRE(state.get_register16(cpu_registers16::BC) == 0x3455);
        REQUIRE(state.get_register16(cpu_registers16::DE) == 0x56AA);
        REQUIRE(state.get_register16(cpu_registers16::HL) == 0x78FF);
        REQUIRE(state.get_register16(cpu_registers16::SP) == 0x9A00);
        REQUIRE(state.get_register16(cpu_registers16::PC) == 0xBC55);

        // Set the low half of all register pairs to a known value using 8 bit setters
        state.set_register8(cpu_registers8::F, 0xFE);
        state.set_register8(cpu_registers8::C, 0xDC);
        state.set_register8(cpu_registers8::E, 0xBA);
        state.set_register8(cpu_registers8::L, 0x98);
        state.set_register8(cpu_registers8::LOW_SP, 0x76);
        state.set_register8(cpu_registers8::LOW_PC, 0x54);

        // Check that the high half of all register pairs is still the same
        REQUIRE(state.get_register16(cpu_registers16::AF) == 0x12D6); // F will have bits 3 and 5 unset!
        REQUIRE(state.get_register16(cpu_registers16::BC) == 0x34DC);
        REQUIRE(state.get_register16(cpu_registers16::DE) == 0x56BA);
        REQUIRE(state.get_register16(cpu_registers16::HL) == 0x7898);
        REQUIRE(state.get_register16(cpu_registers16::SP) == 0x9A76);
        REQUIRE(state.get_register16(cpu_registers16::PC) == 0xBC54);
    }

    SECTION("Testing getting, setting, unsetting flags.") {
        
        // Set all flags to 1 via set_register8
        state.set_register8(cpu_registers8::F, 0xFF);

        // Check that all flags are set via get_flag
        REQUIRE(state.get_flag(cpu_flags::C));
        REQUIRE(state.get_flag(cpu_flags::P));
        REQUIRE(state.get_flag(cpu_flags::AC));
        REQUIRE(state.get_flag(cpu_flags::Z));
        REQUIRE(state.get_flag(cpu_flags::S));

        // Check that the unchanging bits are correct
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b11010111);

        // Unset all flags via unset_flag
        state.unset_flag(cpu_flags::C);
        state.unset_flag(cpu_flags::P);
        state.unset_flag(cpu_flags::AC);
        state.unset_flag(cpu_flags::Z);
        state.unset_flag(cpu_flags::S);

        // Check that all flags are unset via get_register8
        REQUIRE(!state.get_flag(cpu_flags::C));
        REQUIRE(!state.get_flag(cpu_flags::P));
        REQUIRE(!state.get_flag(cpu_flags::AC));
        REQUIRE(!state.get_flag(cpu_flags::Z));
        REQUIRE(!state.get_flag(cpu_flags::S));

        // Check that the unchanging bits are correct
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b00000010);

        // Set all flags to 1 via set_flag
        state.set_flag(cpu_flags::C);
        state.set_flag(cpu_flags::P);
        state.set_flag(cpu_flags::AC);
        state.set_flag(cpu_flags::Z);
        state.set_flag(cpu_flags::S);

        // Check that all flags are set via get_register8
        REQUIRE(state.get_register8(cpu_registers8::F) == 0xD7);
    }

    SECTION("Testing incrementing.") {
                
        // Set the C register to a known value, and zero B
        state.set_register8(cpu_registers8::C, 0xFF);
        state.set_register8(cpu_registers8::B, 0x00);

        // Check that the register is the same when getting
        REQUIRE(state.get_then_inc_register8(cpu_registers8::C) == 0xFF);

        // Check that the register is incremented and hasn't leaked an overflow bit
        REQUIRE(state.get_register8(cpu_registers8::B) == 0x00);
        REQUIRE(state.get_register8(cpu_registers8::C) == 0x00);

        // Set the PC register pair to a known value, and zero all others
        state.set_register16(cpu_registers16::PC, 0xFFFF);
        state.set_register16(cpu_registers16::AF, 0x0000);
        state.set_register16(cpu_registers16::BC, 0x0000);
        state.set_register16(cpu_registers16::DE, 0x0000);
        state.set_register16(cpu_registers16::HL, 0x0000);
        state.set_register16(cpu_registers16::SP, 0x0000);

        // Check that the register pair is the same when getting
        REQUIRE(state.get_then_inc_register16(cpu_registers16::PC) == 0xFFFF);

        // Check that the register pair is incremented and hasn't leaked an overflow bit
        REQUIRE(state.get_register16(cpu_registers16::AF) == 0x0002); // Account for F having bit 1 set
        REQUIRE(state.get_register16(cpu_registers16::BC) == 0x0000);
        REQUIRE(state.get_register16(cpu_registers16::DE) == 0x0000);
        REQUIRE(state.get_register16(cpu_registers16::HL) == 0x0000);
        REQUIRE(state.get_register16(cpu_registers16::SP) == 0x0000);
        REQUIRE(state.get_register16(cpu_registers16::PC) == 0x0000);

        // Increment all 8 bit registers
        state.inc_register8(cpu_registers8::A);
        state.inc_register8(cpu_registers8::F);
        state.inc_register8(cpu_registers8::B);
        state.inc_register8(cpu_registers8::C);
        state.inc_register8(cpu_registers8::D);
        state.inc_register8(cpu_registers8::E);
        state.inc_register8(cpu_registers8::H);
        state.inc_register8(cpu_registers8::L);
        state.inc_register8(cpu_registers8::HIGH_SP);
        state.inc_register8(cpu_registers8::LOW_SP);
        state.inc_register8(cpu_registers8::HIGH_PC);
        state.inc_register8(cpu_registers8::LOW_PC);

        // Check that all registers are incremented
        REQUIRE(state.get_register16(cpu_registers16::AF) == 0x0103); // Account for F having bit 1 set
        REQUIRE(state.get_register16(cpu_registers16::BC) == 0x0101);
        REQUIRE(state.get_register16(cpu_registers16::DE) == 0x0101);
        REQUIRE(state.get_register16(cpu_registers16::HL) == 0x0101);
        REQUIRE(state.get_register16(cpu_registers16::SP) == 0x0101);
        REQUIRE(state.get_register16(cpu_registers16::PC) == 0x0101);
    }

    SECTION("Testing flag setting based on condition.") {
        
        // Set all flags to 0 via set_register8
        state.set_register8(cpu_registers8::F, 0x02);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b00000010);

        // Check flags one by one, on toggle on
        state.set_if_flag(cpu_flags::C, true);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b00000011);
        state.set_if_flag(cpu_flags::P, true);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b00000111);
        state.set_if_flag(cpu_flags::AC, true);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b00010111);
        state.set_if_flag(cpu_flags::Z, true);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b01010111);
        state.set_if_flag(cpu_flags::S, true);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b11010111);

        // Check flags one by one, on toggle off
        state.set_if_flag(cpu_flags::C, false);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b11010110);
        state.set_if_flag(cpu_flags::P, false);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b11010010);
        state.set_if_flag(cpu_flags::AC, false);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b11000010);
        state.set_if_flag(cpu_flags::Z, false);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b10000010);
        state.set_if_flag(cpu_flags::S, false);
        REQUIRE(state.get_register8(cpu_registers8::F) == 0b00000010);
    }

    SECTION("Checking flag setting toggled on set_Z_S_P_flags.") {
        
        state.set_Z_S_P_flags(0x00);
        REQUIRE((state.get_flag(cpu_flags::P) and state.get_flag(cpu_flags::Z) and !state.get_flag(cpu_flags::S)));
        state.set_Z_S_P_flags(0x80);
        REQUIRE((!state.get_flag(cpu_flags::P) and !state.get_flag(cpu_flags::Z) and state.get_flag(cpu_flags::S)));
        state.set_Z_S_P_flags(0x55);
        REQUIRE((state.get_flag(cpu_flags::P) and !state.get_flag(cpu_flags::Z) and !state.get_flag(cpu_flags::S)));
        state.set_Z_S_P_flags(0xAA);
        REQUIRE((state.get_flag(cpu_flags::P) and !state.get_flag(cpu_flags::Z) and state.get_flag(cpu_flags::S)));
    }
}
