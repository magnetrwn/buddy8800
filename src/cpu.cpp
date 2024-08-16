#include "cpu.hpp"

void cpu::execute(u8 opcode) {
    cpu_registers16 reg_pair_sel = static_cast<cpu_registers16>(((opcode >> 4) & 0b11) + 1);

    switch (opcode) {
        case 0b00000000: _trace<1>(opcode); NOP(); break;

        case 0b00000001: 
        case 0b00010001:
        case 0b00100001:
        case 0b00110001: _trace<3>(opcode); LXI(reg_pair_sel); _trace_state(); break;

        case 0b00000010:
        case 0b00010010:
        case 0b00100010:
        case 0b00110010: _trace<1>(opcode); STAX(reg_pair_sel); _trace_reg16_deref(reg_pair_sel); break;

        case 0b00000011:
        case 0b00010011:
        case 0b00100011:
        case 0b00110011: _trace<1>(opcode); INX(reg_pair_sel); _trace_state(); break;
        
        default: {
            _trace_error(opcode);
            return;
        }
    }
}