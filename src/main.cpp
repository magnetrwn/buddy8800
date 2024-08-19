#include "cpu.hpp"

int main() {
    cpu processor;

    std::ifstream file("tests/res/cassette-tape-stub.bin", std::ios::binary);
    if (!file)
        return 1;

    std::vector<u8> cpudiag { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    processor.load(cpudiag.begin(), cpudiag.end(), 0x0);

    while (!processor.is_halted())
        processor.step();

    return 0;
}
