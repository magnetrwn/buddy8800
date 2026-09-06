#include <catch2/catch_test_macros.hpp>

#include "typedef.hpp"
#include "bus.hpp"

TEST_CASE("Check bus with RAM and ROM cards", "[bus]") {    
    std::array<u8, 1024> pattern_1k;
    pattern_1k.fill(0x5A);

    bus cardbus;
    rom_card rom0(0x0000, 1024, 0x5A);
    ram_card ram0(0x0400, 4096);
    rom_card rom1(0x1400, 11264, 0x5A);
    ram_card ram1(0x4000, 1024);
    rom_card overlay(0x4100, pattern_1k.begin(), pattern_1k.end());
    REQUIRE_NOTHROW(cardbus.insert(&rom0, 4));
    REQUIRE_NOTHROW(cardbus.insert(&ram0, 3));
    REQUIRE_NOTHROW(cardbus.insert(&rom1, 2));
    REQUIRE_NOTHROW(cardbus.insert(&ram1, 1));
    REQUIRE_THROWS_AS(cardbus.insert(&overlay, 0), std::invalid_argument);
    REQUIRE_NOTHROW(cardbus.insert(&overlay, 0, true));

    // Memory map:
    // 0x0000 to 0x03ff: r, filled with 0x5A
    // 0x0400 to 0x13ff: rw, bad (filled with BAD_U8)
    // 0x1400 to 0x3fff: r, filled with 0x5A
    // 0x4000 to 0x43ff: rw, bad (filled with BAD_U8)
    // 0x4100 to 0x44ff: r, bad (overlapping the previous area, but in slot 0, resulting in writing to both but only reading
    //                           back from this ROM card!)

    SECTION("Untouched read test") {
        REQUIRE(cardbus.read(0x0000) == 0x5A);
        REQUIRE(cardbus.read(0x03fe) == 0x5A);
        REQUIRE(cardbus.read(0x0400) == BAD_U8);
        REQUIRE(cardbus.read(0x13ff) == BAD_U8);
        REQUIRE(cardbus.read(0x1400) == 0x5A);
        REQUIRE(cardbus.read(0x3fff) == 0x5A);
        REQUIRE(cardbus.read(0x4000) == BAD_U8);
        REQUIRE(cardbus.read(0x43ff) == 0x5A);
        REQUIRE(cardbus.read(0x4100) == 0x5A);
        REQUIRE(cardbus.read(0x44ff) == 0x5A);
        REQUIRE(cardbus.read(0x4500) == BAD_U8);
    }

    SECTION("Write locking and slot priority test") {
        cardbus.write(0x0000, 0x99);
        cardbus.write(0x03ff, 0x99);
        REQUIRE(cardbus.read(0x0000) == 0x5A);
        REQUIRE(cardbus.read(0x03ff) == 0x5A);

        cardbus.write(0x0400, 0x88);
        cardbus.write(0x13ff, 0x88);
        REQUIRE(cardbus.read(0x0400) == 0x88);
        REQUIRE(cardbus.read(0x13ff) == 0x88);

        cardbus.write(0x1400, 0x77);
        cardbus.write(0x3fff, 0x77);
        REQUIRE(cardbus.read(0x1400) == 0x5A);
        REQUIRE(cardbus.read(0x3fff) == 0x5A);

        cardbus.write(0x4000, 0x66);
        cardbus.write(0x43ff, 0x66);
        REQUIRE(cardbus.read(0x4000) == 0x66);
        REQUIRE(cardbus.read(0x43ff) == 0x5A);

        cardbus.write(0x4100, 0x55);
        cardbus.write(0x44ff, 0x55);
        REQUIRE(cardbus.read(0x4100) == 0x5A);
        REQUIRE(cardbus.read(0x44ff) == 0x5A);
    }

    SECTION("Checking r/w (both forced and not) while identifying memory areas") {
        for (usize i = 0; i < 0x4500; ++i) {
            u8 slot = cardbus.get_slot_by_adr(i);

            if (slot == 255)
                continue;

            cardbus.write(i, 0x77);

            if (slot == 4 or slot == 2 or slot == 0)
                REQUIRE(cardbus.read(i) == 0x5A);
            else
                REQUIRE(cardbus.read(i) == 0x77);

            cardbus.write_force(i, 0xAA);

            REQUIRE(cardbus.read(i) == 0xAA);
        }
    }
}

TEST_CASE("Memory boundaries and clearing preserve card storage", "[bus]") {
    bus cardbus;
    ram_card ram(0, 65536, 0);
    rom_card rom(0xF800, 1031, 0x5A);
    REQUIRE_THROWS_AS(cardbus.insert(&ram, 18), std::out_of_range);
    REQUIRE_THROWS_AS(cardbus.remove(18), std::out_of_range);
    REQUIRE_THROWS_AS(ram_card(0xFFFF, 2), std::out_of_range);
    REQUIRE_THROWS_AS(ram_card(0, 0), std::out_of_range);
    REQUIRE_NOTHROW(cardbus.insert(&rom, 0));
    REQUIRE_NOTHROW(cardbus.insert(&ram, 17, true));
    REQUIRE(rom.in_range(0xFC06));
    REQUIRE_FALSE(rom.in_range(0xFC07));
    cardbus.write(0xFFFF, 0x11);
    cardbus.clear();
    REQUIRE(cardbus.read(0xFFFF) == BAD_U8);
    REQUIRE(cardbus.read(0xF800) == 0x5A);
    cardbus.write(0xFFFF, 0x22);
    REQUIRE(cardbus.read(0xFFFF) == 0x22);
}
