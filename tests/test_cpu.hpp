#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <fstream>

#include "cpu.hpp"

TEST_CASE("Testing the CPU itself.", "[cpu]") {
    cpu emu;

    SECTION("Running cpudiag.bin.") {
        std::ifstream program("tests/res/cpudiag.bin", std::ios::binary);
        REQUIRE(program);
        std::vector<u8> programv { std::istreambuf_iterator<char>(program), std::istreambuf_iterator<char>() };

        std::ifstream passed_run("tests/res/passed.txt", std::ios::binary);
        REQUIRE(passed_run);
        std::vector<u8> passed_runv { std::istreambuf_iterator<char>(passed_run), std::istreambuf_iterator<char>() };

        emu.load(programv.begin(), programv.end(), 0x100);
        emu.set_printer_to_file("tests/res/out.txt");
        while (!emu.is_halted())
            emu.step();

        std::ifstream current_run("tests/res/out.txt", std::ios::binary);
        REQUIRE(current_run);
        std::vector<u8> current_runv { std::istreambuf_iterator<char>(current_run), std::istreambuf_iterator<char>() };

        REQUIRE(current_runv == passed_runv);
    }
}
