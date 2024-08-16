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
        case 0b00010010: _trace<1>(opcode); STAX(reg_pair_sel); _trace_reg16_deref(reg_pair_sel); break;

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

        //     ..DDD...
        case 0b00000110:
        case 0b00001110:
        case 0b00010110:
        case 0b00011110:
        case 0b00100110:
        case 0b00101110:
        case 0b00111110: _trace<2>(opcode); MVI(reg_dst_sel); _trace_state(); break;
        case 0b00110110: _trace<2>(opcode); MVI_M(); _trace_state(); _trace_reg16_deref(cpu_registers16::HL); break;
        
        //     ..RP....
        case 0b00001001:
        case 0b00011001:
        case 0b00101001:
        case 0b00111001: _trace<1>(opcode); DAD(reg_pair_sel); _trace_state(); break;

        //     ..RP....
        case 0b00001010:
        case 0b00011010: _trace<1>(opcode); LDAX(reg_pair_sel); _trace_state(); _trace_reg16_deref(reg_pair_sel); break;

        //     ..RP....
        case 0b00001011:
        case 0b00011011:
        case 0b00101011:
        case 0b00111011: _trace<1>(opcode); DCX(reg_pair_sel); _trace_state(); break;

        case 0b00000111: _trace<1>(opcode); RLC(); _trace_state(); break;

        case 0b00001111: _trace<1>(opcode); RRC(); _trace_state(); break;

        case 0b00010111: _trace<1>(opcode); RAL(); _trace_state(); break;

        case 0b00011111: _trace<1>(opcode); RAR(); _trace_state(); break;

        case 0b00100010: _trace<3>(opcode); SHLD(); _trace_state(); _trace_mem16_deref(); break;

        case 0b00100111: _trace<1>(opcode); DAA(); _trace_state(); break;

        case 0b00101010: _trace<3>(opcode); LHLD(); _trace_state(); _trace_mem16_deref(); break;

        case 0b00101111: _trace<1>(opcode); CMA(); _trace_state(); break;

        case 0b00110010: _trace<3>(opcode); STA(); _trace_state(); _trace_mem16_deref(); break;

        case 0b00110111: _trace<1>(opcode); STC(); _trace_state(); break;

        case 0b00111010: _trace<3>(opcode); LDA(); _trace_state(); _trace_mem16_deref(); break;

        case 0b00111111: _trace<1>(opcode); CMC(); _trace_state(); break;

        default:         _trace_error(opcode); break;
    }
}