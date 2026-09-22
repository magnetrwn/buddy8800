#include "app/program_loader.hpp"
#include "app/binary_file.hpp"
#include "app/emulator.hpp"

namespace buddy8800::app {

void load_programs(emulator& machine, const std::vector<program_load>& programs) {
    for (usize i = 0; i < programs.size(); ++i) {
        const auto& program = programs[i];
        machine.load_program(read_binary(program.filename), program.address, i == 0 && program.address > 2);
    }
    machine.start();
}

}
