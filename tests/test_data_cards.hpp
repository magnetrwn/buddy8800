#include <catch2/catch_test_macros.hpp>

#include "typedef.hpp"
#include "bus.hpp"

TEST_CASE("Check bus with RAM and ROM cards", "[bus]") {    
    bus cardbus;
    REQUIRE_NOTHROW(cardbus.insert(new rom_card(0x0000, 1024, 0x5A), 4));
    REQUIRE_NOTHROW(cardbus.insert(new ram_card(0x0400, 4096), 3));
    REQUIRE_NOTHROW(cardbus.insert(new rom_card(0x1400, 11264, 0x5A), 2));
    REQUIRE_NOTHROW(cardbus.insert(new ram_card(0x4000, 1024), 1));
    REQUIRE_THROWS_AS(cardbus.insert(new rom_card(0x4100, 1024, 0x5A), 0), std::invalid_argument);
    REQUIRE_NOTHROW(cardbus.insert(new rom_card(0x4100, 1024, 0x5A), 0, true));

    // Memory map:
    // 0x0000 to 0x03ff: r, filled with 0x5A
    // 0x0400 to 0x13ff: rw, zeroed
    // 0x1400 to 0x3fff: r, filled with 0x5A
    // 0x4000 to 0x43ff: rw, zeroed
    // 0x4100 to 0x44ff: r, zeroed (overlapping the previous area, but in slot 0, resulting in writing to both but only reading
    //                              back from this ROM card!)

    SECTION("Untouched read test") {
        REQUIRE(cardbus.read(0x0000) == 0x5A);
        REQUIRE(cardbus.read(0x03fe) == 0x5A);
        REQUIRE(cardbus.read(0x0400) == 0x00);
        REQUIRE(cardbus.read(0x13ff) == 0x00);
        REQUIRE(cardbus.read(0x1400) == 0x5A);
        REQUIRE(cardbus.read(0x3fff) == 0x5A);
        REQUIRE(cardbus.read(0x4000) == 0x00);
        REQUIRE(cardbus.read(0x43ff) == 0x5A);
        REQUIRE(cardbus.read(0x4100) == 0x5A);
        REQUIRE(cardbus.read(0x44ff) == 0x5A);
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

    SECTION("Checking r/w while identifying memory areas") {
        for (usize i = 0; i < 0x4500; ++i) {
            u8 slot = cardbus.get_slot_by_adr(i);

            if (slot == 255)
                continue;

            cardbus.write(i, 0x77);

            if (slot == 4 or slot == 2 or slot == 0)
                REQUIRE(cardbus.read(i) == 0x5A);
            else
                REQUIRE(cardbus.read(i) == 0x77);
        }
    }
}
