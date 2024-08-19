#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <fstream>

#include "cpu.hpp"

TEST_CASE("Testing the CPU itself.", "[cpu]") {
    constexpr static const char* CPUDIAG = "cpudiag.bin";
    constexpr static const char* PASSED = "passed.txt";
    constexpr static const char* OUTPUT = "out.txt";

    cpu emu;

    SECTION("Running cpudiag.bin.") {
        std::ifstream program(CPUDIAG, std::ios::binary);
        REQUIRE(program.is_open());
        std::vector<u8> programv { std::istreambuf_iterator<char>(program), std::istreambuf_iterator<char>() };

        std::ifstream passed_run(PASSED, std::ios::binary);
        REQUIRE(passed_run.is_open());
        std::vector<u8> passed_runv { std::istreambuf_iterator<char>(passed_run), std::istreambuf_iterator<char>() };

        emu.load(programv.begin(), programv.end(), 0x100);
        emu.set_printer_to_file(OUTPUT);
        while (!emu.is_halted())
            emu.step();
        emu.reset_printer();

        std::ifstream current_run(OUTPUT, std::ios::binary);
        REQUIRE(current_run.is_open());
        std::vector<u8> current_runv { std::istreambuf_iterator<char>(current_run), std::istreambuf_iterator<char>() };

        REQUIRE(current_runv == passed_runv);
    }
}
