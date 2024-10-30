#ifndef UX_HPP_
#define UX_HPP_

#include <iostream>
#include <fstream>
#include <vector>
#include <cerrno>

#include "cpu.hpp"
#include "bus.hpp"
#include "card.hpp"

namespace errors {
    enum class errno_enum {
        SUCCESS = 0,
        FILE_NOT_FOUND = ENOENT,
        INVALID_ARGS = EINVAL
    };

    static bool is_error(errno_enum err) { return err != errno_enum::SUCCESS; }
    static int error_to_int(errno_enum err) { return static_cast<int>(err); }
    static const char* error_to_str(errno_enum err) { return strerror(error_to_int(err)); }
    
    static int pretty_error_then_ret_int(errno_enum err) { 
        std::cerr << "\x1B[31;01merror: " << error_to_str(err) << "\x1B[0m" << std::endl; 
        return error_to_int(err);
    }
};

using namespace errors;

class emulator {
private:
    bus cardbus;
    cpu<bus&> processor;
    ram_card* ram_64k;
    std::vector<u8> load_rom_vec;
    bool cards_initialized;

public:
    errno_enum setup(int argc, char** argv) {
        if (argc != 2) 
            return errno_enum::INVALID_ARGS;

        std::ifstream load_rom(argv[1], std::ios::binary);

        if (!load_rom)
            return errno_enum::FILE_NOT_FOUND;

        /// @todo: Add a way to input the size of the rom, or reserve the max memory size (temporary fix for now), to avoid reallocations.
        load_rom_vec.reserve(cardbus.size());
        load_rom_vec = std::vector<u8>(std::istreambuf_iterator<char>(load_rom), {});

        ram_64k = new ram_card(0x0000, 65536);

        cardbus.insert(ram_64k, 4);
        //cardbus.insert(new serial_card(0x0010), 0, true);
        cardbus.print_mmap();

        processor.load(load_rom_vec.begin(), load_rom_vec.end(), 0x100, true);
        processor.do_pseudo_bdos(true);

        cards_initialized = true;

        return errno_enum::SUCCESS;
    }

    void run() {
        while (!processor.is_halted()) {
            processor.step();
            cardbus.refresh();
        }
    }

    void stop() {
        processor.clear();
        cardbus.clear();
        if (cards_initialized)
            delete ram_64k;
        cards_initialized = false;
    }

    emulator() : processor(cardbus), cards_initialized(false) {}
    ~emulator() { if (cards_initialized) stop(); }
};

struct terminal_ux {
    emulator emu;

    int main(int argc, char** argv) {
        std::cout << "\x1B[33;01m-:-:-:-:- emulator setup -:-:-:-:-\x1B[0m" << std::endl;

        errno_enum err = emu.setup(argc, argv);
        if (is_error(err))
            return pretty_error_then_ret_int(err);

        std::cout << "\x1B[33;01m-:-:-:-:- emulator run -:-:-:-:-\x1B[0m" << std::endl;

        emu.run();

        std::cout << "\x1B[33;01m\n-:-:-:-:- emulator end -:-:-:-:-\x1B[0m" << std::endl;
        
        return 0;
    }
};

#endif