#include "app/emulator.hpp"

emulator::emulator(const char* config_filename)
    : conf(config_filename), cardbus(conf.get_bus()), processor(cardbus, conf.get_start_pc() == 0x0000) {
    processor.do_pseudo_bdos(conf.get_do_pseudo_bdos());
}

void emulator::load_program(const std::vector<u8>& bytes, u16 address, bool reset_vector) {
    processor.load(bytes.begin(), bytes.end(), address, reset_vector);
}

void emulator::start() {
    processor.set_pc(conf.get_start_pc());
}

void emulator::run(const volatile std::sig_atomic_t& stop) {
    while (!stop && !processor.is_halted())
        step<false>();
}
