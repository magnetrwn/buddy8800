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

        //     ..DDDSSS
        case 0b01000000:
        case 0b01000001:
        case 0b01000010:
        case 0b01000011:
        case 0b01000100:
        case 0b01000101:
        case 0b01000111:
        case 0b01001000:
        case 0b01001001:
        case 0b01001010:
        case 0b01001011:
        case 0b01001100:
        case 0b01001101:
        case 0b01001111:
        case 0b01010000:
        case 0b01010001:
        case 0b01010010:
        case 0b01010011:
        case 0b01010100:
        case 0b01010101:
        case 0b01010111:
        case 0b01011000:
        case 0b01011001:
        case 0b01011010:
        case 0b01011011:
        case 0b01011100:
        case 0b01011101:
        case 0b01011111:
        case 0b01100000:
        case 0b01100001:
        case 0b01100010:
        case 0b01100011:
        case 0b01100100:
        case 0b01100101:
        case 0b01100111:
        case 0b01101000:
        case 0b01101001:
        case 0b01101010:
        case 0b01101011:
        case 0b01101100:
        case 0b01101101:
        case 0b01101111:
        case 0b01111000:
        case 0b01111001:
        case 0b01111010:
        case 0b01111011:
        case 0b01111100:
        case 0b01111101:
        case 0b01111111: _trace<1>(opcode); MOV(reg_dst_sel, cpu_reg8_decode[opcode & 0b111]); _trace_state(); break;
        case 0b01000110:
        case 0b01001110:
        case 0b01010110:
        case 0b01011110:
        case 0b01100110:
        case 0b01101110:
        case 0b01111110: _trace<1>(opcode); MOV_FROM_M(reg_dst_sel); _trace_state(); _trace_reg16_deref(cpu_registers16::HL); break;
        case 0b01110000:
        case 0b01110001:
        case 0b01110010:
        case 0b01110011:
        case 0b01110100:
        case 0b01110101:
        case 0b01110111: _trace<1>(opcode); MOV_TO_M(cpu_reg8_decode[opcode & 0b111]); _trace_state(); _trace_reg16_deref(cpu_registers16::HL); break;
        
        case 0b01110110: _trace<1>(opcode); HLT(); break;

        

        default:         _trace_error(opcode); break;
    }
}

void cpu::step() { 
    if (halted) 
        return; 
    execute(fetch()); 
}

void cpu::load(std::vector<u8>::iterator begin, std::vector<u8>::iterator end, usize offset) {
    usize dist = std::distance(begin, end);

    if (dist > memory.size() - offset)
        throw std::out_of_range("Not enough space in emulated memory.");

    std::copy(begin, end, memory.begin() + offset);
}

void cpu::load_state(const cpu_state& new_state) {
    state = new_state;
}

cpu_state cpu::save_state() const {
    return state;
}