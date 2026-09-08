#ifndef UX_HPP_
#define UX_HPP_

#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <csignal>

#include "cpu.hpp"
#include "bus.hpp"
#include "card.hpp"
#include "sysconf.hpp"

class emulator {
private:
    system_config conf;
    bus& cardbus;
    cpu<bus&> processor;

public:
    void setup(int argc, char** argv) {
        if (argc < 1 or !(argc & 1)) 
            throw std::invalid_argument("Invalid number of arguments. Provide pairs of ROM/data files and integer load addresses.");

        processor.do_pseudo_bdos(conf.get_do_pseudo_bdos());
        std::vector<u8> load_rom_vec;

        // The arguments come in pairs of filename and location to load the ROM at.
        for (int i = 1; i < argc; i += 2) {
            std::ifstream load_rom(argv[i], std::ios::binary);

            if (!load_rom)
                throw std::runtime_error("Could not open file: " + std::string(argv[i]));

            load_rom_vec.assign(std::istreambuf_iterator<char>(load_rom), {});

            // The first ROM is the one that will have the reset vector jump to.
            const std::string address(argv[i + 1]);
            usize consumed = 0;
            const auto offset = std::stoul(address, &consumed, 0);
            if (consumed != address.size() || offset > 65535 || address.front() == '-')
                throw std::invalid_argument("Invalid load address: " + address);
            if (load_rom.bad() || load_rom_vec.empty())
                throw std::runtime_error("Empty or unreadable ROM file");
            processor.load(load_rom_vec.begin(), load_rom_vec.end(), offset, i == 1 && offset > 2);
        }

        processor.set_pc(conf.get_start_pc());
    }

    void run(const volatile std::sig_atomic_t& stop) {
        while (!stop && !processor.is_halted()) {
            step<false>();
        }
    }

    template <bool report_trace = true>
    void step() {
        if (processor.is_halted()) return;
        processor.step<report_trace>();
        if (cardbus.is_irq()) processor.interrupt<report_trace>(cardbus.get_irq());
    }
    bool halted() const { return processor.is_halted(); }
    cpu_state state() const { return processor.save_state(); }
    void trace(std::function<void(u16, const std::array<u8, 3>&, usize)> observer) {
        processor.set_trace_observer(std::move(observer));
    }

    std::string info() const { return cardbus.bus_map_s(); }

    emulator(const char* config_filename) 
        : conf(config_filename), 
          cardbus(conf.get_bus()), 
          processor(cardbus, conf.get_start_pc() == 0x0000) {}
};

#ifndef DISABLE_TRACE
int run_tui(emulator& emu, const volatile std::sig_atomic_t& stop);
#endif

struct terminal_ux {
    emulator emu;

    int main(int argc, char** argv, const volatile std::sig_atomic_t& stop) {
        emu.setup(argc, argv);
#ifndef DISABLE_TRACE
        if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))
            return run_tui(emu, stop);
#endif
        std::cout << "\x1B[33;01m-:-:-:-:- emulator setup -:-:-:-:-\x1B[0m\n" << std::endl;

        std::cout << emu.info();

        std::cout << "\x1B[33;01m-:-:-:-:- emulator run -:-:-:-:-\x1B[0m" << std::endl;

        emu.run(stop);

        std::cout << "\x1B[33;01m\n-:-:-:-:- emulator end -:-:-:-:-\x1B[0m" << std::endl;
        
        return 0;
    }

    terminal_ux(const char* config_filename) : emu(config_filename) {}
};

#endif
