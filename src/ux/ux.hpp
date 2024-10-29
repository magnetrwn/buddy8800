#ifndef UX_HPP_
#define UX_HPP_

#include <iostream>
#include <fstream>
#include <vector>

#include "cpu.hpp"
#include "bus.hpp"
#include "card.hpp"

class emulator {
private:
    bus cardbus;
    cpu<bus&> processor;
    std::vector<u8> load_rom_vec;

    bool check_argc_argv([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
        return argc == 2;
    }

    bool check_and_load_rom(char** argv) {
        std::ifstream load_rom(argv[1], std::ios::binary);

        if (!load_rom)
            return false;

        load_rom_vec = std::vector<u8>(std::istreambuf_iterator<char>(load_rom), {});
        return true;
    }

    void setup() {
        cardbus.insert(new ram_card(0x0000, 65536), 4);
        //cardbus.insert(new serial_card(0x0010), 0, true);
        cardbus.print_mmap();

        processor.load(load_rom_vec.begin(), load_rom_vec.end(), 0x100, true);
        processor.do_pseudo_bdos(true);
    }

    void run() {
        while (!processor.is_halted()) {
            processor.step();
            cardbus.refresh();
        }
    }

public:
    int main(int argc, char** argv) {
        if (!check_argc_argv(argc, argv)) {
            std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
            return 1;
        }

        if (!check_and_load_rom(argv)) {
            std::cerr << "File failure!" << std::endl;
            return 1;
        }

        std::cout << "\x1B[33;01m-:-:-:-:- emulator setup -:-:-:-:-\x1B[0m" << std::endl;

        setup();

        std::cout << "\x1B[33;01m-:-:-:-:- emulator run -:-:-:-:-\x1B[0m" << std::endl;

        run();

        std::cout << "\x1B[33;01m\n-:-:-:-:- emulator end -:-:-:-:-\x1B[0m" << std::endl;
        
        return 0;
    }

    emulator() : processor(cardbus) {}
};

#endif