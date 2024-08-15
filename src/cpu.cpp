#include "cpu.hpp"

void cpu::execute(u8 opcode) {
    switch (opcode) {
        case 0x00: {
            _debug_ex<0x00>();
            break;
        }
        case 0x01: {
            u16 lo = fetch();
            u16 hi = fetch();
            _debug_ex<0x01>(lo, hi);    
            state.set_register16<cpu_registers16::BC>((hi << 8) | lo);
            break;
        }
        case 0x02: {
            _debug_ex<0x02>();
            memory[state.get_register16<cpu_registers16::BC>()] = state.get_register8<cpu_registers8::A>();
            break;
        }
        case 0x03: {
            _debug_ex<0x03>();
            state.set_register16<cpu_registers16::BC>(state.get_register16<cpu_registers16::BC>() + 1);
            break;
        }
        default: {
            _debug_ex_error(opcode);
            break;
        }
    }
}