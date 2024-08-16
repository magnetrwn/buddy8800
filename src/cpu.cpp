#include "cpu.hpp"

void cpu::execute(u8 opcode) {
    cpu_registers16 reg_pair_sel = static_cast<cpu_registers16>((((opcode & 0b00110000) >> 4) & 0b11) + 1);
    cpu_registers8 reg_dst_sel = cpu_reg8_decode[((opcode & 0b00111000) >> 3) & 0b111];

    switch (opcode) {
        case 0b00000000: _trace<1>(opcode); NOP(); break;
        
        //     ..RP....
        case 0b00000001: 
        case 0b00010001:
        case 0b00100001:
        case 0b00110001: _trace<3>(opcode); LXI(reg_pair_sel); _trace_state(); break;

        //     ..RP....
        case 0b00000010:
        case 0b00010010:
        case 0b00100010:
        case 0b00110010: _trace<1>(opcode); STAX(reg_pair_sel); _trace_reg16_deref(reg_pair_sel); break;

        //     ..RP....
        case 0b00000011:
        case 0b00010011:
        case 0b00100011:
        case 0b00110011: _trace<1>(opcode); INX(reg_pair_sel); _trace_state(); break;

        //     ..DDD...
        case 0b00000100:
        case 0b00001100:
        case 0b00010100:
        case 0b00011100:
        case 0b00100100:
        case 0b00101100:
        case 0b00111100: _trace<1>(opcode); INR(reg_dst_sel); _trace_state(); break;
        case 0b00110100: _trace<1>(opcode); INR_M(); _trace_state(); _trace_reg16_deref(cpu_registers16::HL); break;

        //     ..DDD...
        case 0b00000101:
        case 0b00001101:
        case 0b00010101:
        case 0b00011101:
        case 0b00100101:
        case 0b00101101:
        case 0b00111101: _trace<1>(opcode); DCR(reg_dst_sel); _trace_state(); break;
        case 0b00110101: _trace<1>(opcode); DCR_M(); _trace_state(); _trace_reg16_deref(cpu_registers16::HL); break;
        
        default: _trace_error(opcode); break;
    }
}