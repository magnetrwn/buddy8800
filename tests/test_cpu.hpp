#include <catch2/catch_test_macros.hpp>

#include "cpu.hpp"

TEST_CASE("Test Description", "[tag]") {
    REQUIRE(baz::foo(1) == -1);
}