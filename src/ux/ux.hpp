#ifndef UX_HPP_
#define UX_HPP_

#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#include "cpu.hpp"
#include "bus.hpp"
#include "card.hpp"
#include "sysconf.hpp"

class emulator {
private:
    system_config hw;
    bus& cardbus;
    cpu<bus&> processor;
    std::vector<u8> load_rom_vec;

public:
    void setup(int argc, char** argv) {
        if (argc < 3 or !(argc & 1)) 
            throw std::invalid_argument("Invalid number of arguments. Provide pairs of ROM/data files and integer load addresses.");

        cardbus.print_mmap();
        processor.do_pseudo_bdos(hw.get_do_pseudo_bdos());
        load_rom_vec.reserve(cardbus.size());

        // The arguments come in pairs of filename and location to load the ROM at.
        for (int i = 1; i < argc; i += 2) {
            std::ifstream load_rom(argv[i], std::ios::binary);

            if (!load_rom)
                throw std::runtime_error("Could not open file: " + std::string(argv[i]));

            load_rom_vec.assign(std::istreambuf_iterator<char>(load_rom), {});

            // The first ROM is the one that will have the reset vector jump to.
            processor.load(load_rom_vec.begin(), load_rom_vec.end(), std::stoul(argv[i + 1], nullptr, 0), i == 1);
        }
    }

    void run() {
        while (!processor.is_halted()) {
            processor.step();
            cardbus.refresh();
        }
    }

    emulator(const char* config_filename) : hw(config_filename), cardbus(hw.get_bus()), processor(cardbus) {}
};

struct terminal_ux {
    emulator emu;

    int main(int argc, char** argv) {
        std::cout << "\x1B[33;01m-:-:-:-:- emulator setup -:-:-:-:-\x1B[0m" << std::endl;

        emu.setup(argc, argv);

        std::cout << "\x1B[33;01m-:-:-:-:- emulator run -:-:-:-:-\x1B[0m" << std::endl;

        emu.run();

        std::cout << "\x1B[33;01m\n-:-:-:-:- emulator end -:-:-:-:-\x1B[0m" << std::endl;
        
        return 0;
    }

    terminal_ux(const char* config_filename) : emu(config_filename) {}
};

#endif