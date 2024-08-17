#include "cpu.hpp"

#include <vector>
#include <fstream>

int main() {
    cpu processor(0x100);

    std::ifstream file("tests/res/cpudiag.bin", std::ios::binary);
    if (!file) {
        return 1;
    }

    std::vector<u8> cpudiag { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    processor.load(cpudiag.begin(), cpudiag.end(), 0x100);

    for (usize i = 0; i < 150; ++i)
        processor.step();

    return 0;
}
