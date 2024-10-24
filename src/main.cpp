#include "cpu.hpp"
#include "bus.hpp"
#include "card.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "No such file!" << std::endl;
        return 1;
    }

    std::vector<u8> cpudiag { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    bus cardbus;
    cardbus.insert(new ram_card<0x0000, 65535>, 0);
    cardbus.insert(new serial_card<0x10>, 1, true);
    cardbus.print_mmap();

    cpu processor(cardbus);
    processor.load(cpudiag.begin(), cpudiag.end(), 0x100, true);
    processor.do_pseudo_bdos(true);

    while (!processor.is_halted()) {
        processor.step();
        cardbus.refresh();
    }
    
    return 0;
}
