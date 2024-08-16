#include "cpu.hpp"

#include <vector>
#include <fstream>

int main() {
    cpu processor;

    std::ifstream file("tests/res/cpudiag.bin", std::ios::binary);
    if (!file) {
        return 1;
    }

    std::vector<u8> cpudiag{
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    };

    processor.load(cpudiag.begin(), cpudiag.end());

    for (usize i = 0; i < 100; ++i)
        processor.step();

    return 0;
}
