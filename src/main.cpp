#include "cpu.hpp"

int main() {
    cpu processor;

    std::ifstream file("tests/res/diag2.com", std::ios::binary);
    if (!file)
        return 1;

    std::vector<u8> cpudiag { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    processor.load(cpudiag.begin(), cpudiag.end(), 0x100, true);
    processor.set_handle_bdos(true);
    while (!processor.is_halted())
        processor.step();

    return 0;
}
