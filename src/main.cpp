#include "cpu.hpp"
#include "bus.hpp"
#include "card.hpp"

#include <immintrin.h>

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

    std::cout << "\x1B[33;01m-:-:-:-:- emulator setup -:-:-:-:-\x1B[0m" << std::endl;

    std::vector<u8> cpudiag { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

    bus cardbus;
    cardbus.insert(new ram_card<0x0000, 65535>, 0);
    cardbus.insert(new serial_card<0x0010>, 1, true);
    cardbus.print_mmap();

    cpu processor(cardbus);
    processor.load(cpudiag.begin(), cpudiag.end(), 0x100, true);
    processor.do_pseudo_bdos(true);

    std::cout << "\x1B[33;01m-:-:-:-:- emulator start -:-:-:-:-\x1B[0m" << std::endl;
    for (float i = 1; !processor.is_halted(); i = (i < 600.0f) ? (i * 1.375f) : (i + 25.0f)) {
        processor.step();
        cardbus.refresh();
        usleep(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(i))) * 1000000);
    }
    std::cout << "\x1B[33;01m\n-:-:-:-:- emulator end -:-:-:-:-\x1B[0m" << std::endl;
    
    return 0;
}
