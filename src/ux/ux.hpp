#ifndef UX_HPP_
#define UX_HPP_

#include "cpu.hpp"
#include "bus.hpp"
#include "card.hpp"

struct terminal_emulator_ux {
public:
    inline int main(int argc, char** argv) {
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
        cardbus.insert(new ram_card(0x0000, 65536), 4);
        //cardbus.insert(new serial_card(0x0010), 0, true);
        cardbus.print_mmap();

        cpu processor(cardbus);
        processor.load(cpudiag.begin(), cpudiag.end(), 0x100, true);
        processor.do_pseudo_bdos(true);

        std::cout << "\x1B[33;01m-:-:-:-:- emulator start -:-:-:-:-\x1B[0m" << std::endl;

        while (!processor.is_halted()) {
            processor.step();
            cardbus.refresh();
        }

        std::cout << "\x1B[33;01m\n-:-:-:-:- emulator end -:-:-:-:-\x1B[0m" << std::endl;
        
        return 0;
    }
};

#endif