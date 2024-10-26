#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

#include "typedef.hpp"
#include "cpu.hpp"

constexpr static const usize TESTS_N = 4;
constexpr static const char* TESTFILE[TESTS_N] = { "cpudiag.bin", "test.com", "8080pre.com", "diag2.com" };
constexpr static const char* PASSED[TESTS_N] = { "ok_cpudiag.txt", "ok_test.txt", "ok_8080pre.txt", "ok_diag2.txt" };
constexpr static const char* OUTPUT = "out.txt";

using cpu_t = cpu<std::array<u8, 65536>>;

inline std::vector<u8> read_bin(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + std::string(filename));
    return std::vector<u8>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

inline void test(cpu_t& emu, const char* prg, const char* ok) {
    std::vector<u8> programv = read_bin(prg);
    std::vector<u8> okv = read_bin(ok);

    REQUIRE(!programv.empty());
    REQUIRE(!okv.empty());

    emu.load(programv.begin(), programv.end(), 0x100, true);
    emu.do_pseudo_bdos(true);
    emu.set_pseudo_bdos_redirect(OUTPUT);
    while (!emu.is_halted())
        emu.step();
    emu.reset_pseudo_bdos_redirect();

    std::vector<u8> outputv = read_bin(OUTPUT);

    REQUIRE(!outputv.empty());

    std::string outputs(outputv.begin(), outputv.end());
    std::string oks(okv.begin(), okv.end());

    REQUIRE(oks == outputs);

    emu.clear();
}

TEST_CASE("CPU running various diagnostics", "[cpu]") {
    cpu_t emu({0});

    for (usize i = 0; i < TESTS_N; ++i)
        SECTION("Running " + std::string(TESTFILE[i]))
            test(emu, TESTFILE[i], PASSED[i]);
}
