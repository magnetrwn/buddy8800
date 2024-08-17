#include "cpu.hpp"

//#define OPRANGE(opstart, opend, opshft, opvlen)

void cpu::_trace_state() {
        printf("\x1B[44;01mA: %02hhX BC: %04hX DE: %04hX HL: %04hX \x1B[0m\n",
            state.get_register8(cpu_registers8::A),
            state.get_register16(cpu_registers16::BC),
            state.get_register16(cpu_registers16::DE),
            state.get_register16(cpu_registers16::HL));
        printf("\x1B[44;01m SP: %04hX PC: %04hX F: %c %c %c %c %c  \x1B[0m\n",
            state.get_register16(cpu_registers16::SP),
            state.get_register16(cpu_registers16::PC),
            state.get_flag(cpu_flags::S) ? 'S' : '/',
            state.get_flag(cpu_flags::Z) ? 'Z' : '/',
            state.get_flag(cpu_flags::AC) ? 'A' : '/',
            state.get_flag(cpu_flags::P) ? 'P' : '/',
            state.get_flag(cpu_flags::C) ? 'C' : '/');
    }

void cpu::_trace_reg16_deref(cpu_registers16 reg) {
    printf("\x1B[42;01m(%s: %04hX): %02hhX                   \x1B[0m\n",
        (reg == cpu_registers16::AF) ? "AF" : 
            (reg == cpu_registers16::BC) ? "BC" : 
            (reg == cpu_registers16::DE) ? "DE" : 
            (reg == cpu_registers16::HL) ? "HL" : 
            (reg == cpu_registers16::SP) ? "SP" : 
            "PC",
        state.get_register16(reg),
        memory[state.get_register16(reg)]);
}

void cpu::_trace_mem16_deref() {
    printf("\x1B[43;01m(%02hhX%02hhX): %02hhX                   \x1B[0m\n",
        memory[pc()],
        memory[pc() + 1],
        memory[(memory[pc() + 1] << 8) | memory[pc()]]);
}

void cpu::_trace_error(u8 opc) {
    printf("%04hX    %02hhX      \t \x1B[31;01mUNKNOWN\x1B[0m\n", pc() - 1, opc);
}

void cpu::execute(u8 opcode) {
    cpu_registers16 pair_sel = static_cast<cpu_registers16>((((opcode & 0b00110000) >> 4) & 0b11) + 1);
    cpu_registers8 dst_sel = cpu_reg8_decode[((opcode & 0b00111000) >> 3) & 0b111];
    cpu_registers8 src_sel = cpu_reg8_decode[opcode & 0b111];

    switch (opcode) {
        case 0b00000000: _trace<1>(opcode); NOP(); 
        break;
        
        //     ..RP....
        case 0b00000001: 
        case 0b00010001:
        case 0b00100001:
        case 0b00110001: _trace<3>(opcode); LXI(pair_sel); _trace_state(); 
        break;

        //     ..RP....
        case 0b00000010:
        case 0b00010010: _trace<1>(opcode); STAX(pair_sel); _trace_reg16_deref(pair_sel); 
        break;

        //     ..RP....
        case 0b00000011:
        case 0b00010011:
        case 0b00100011:
        case 0b00110011: _trace<1>(opcode); INX(pair_sel); _trace_state(); 
        break;

        //     ..DDD...
        case 0b00000100:
        case 0b00001100:
        case 0b00010100:
        case 0b00011100:
        case 0b00100100:
        case 0b00101100:
        case 0b00111100: 
        case 0b00110100: _trace<1>(opcode); INR(dst_sel); _trace_state(); 
                         if (is_memref(dst_sel)) _trace_reg16_deref(cpu_registers16::HL); 
        break;

        //     ..DDD...
        case 0b00000101:
        case 0b00001101:
        case 0b00010101:
        case 0b00011101:
        case 0b00100101:
        case 0b00101101:
        case 0b00111101: 
        case 0b00110101: _trace<1>(opcode); DCR(dst_sel); _trace_state(); 
                         if (is_memref(dst_sel)) _trace_reg16_deref(cpu_registers16::HL); 
        break;

        //     ..DDD...
        case 0b00000110:
        case 0b00001110:
        case 0b00010110:
        case 0b00011110:
        case 0b00100110:
        case 0b00101110:
        case 0b00111110: 
        case 0b00110110: _trace<2>(opcode); MVI(dst_sel); _trace_state(); 
                         if (is_memref(dst_sel)) _trace_reg16_deref(cpu_registers16::HL);          
        break;
        
        //     ..RP....
        case 0b00001001:
        case 0b00011001:
        case 0b00101001:
        case 0b00111001: _trace<1>(opcode); DAD(pair_sel); _trace_state();
        break;

        //     ..RP....
        case 0b00001010:
        case 0b00011010: _trace<1>(opcode); LDAX(pair_sel); _trace_state(); _trace_reg16_deref(pair_sel); 
        break;

        //     ..RP....
        case 0b00001011:
        case 0b00011011:
        case 0b00101011:
        case 0b00111011: _trace<1>(opcode); DCX(pair_sel); _trace_state(); 
        break;

        case 0b00000111: _trace<1>(opcode); RLC(); _trace_state(); 
        break;

        case 0b00001111: _trace<1>(opcode); RRC(); _trace_state(); 
        break;

        case 0b00010111: _trace<1>(opcode); RAL(); _trace_state(); 
        break;

        case 0b00011111: _trace<1>(opcode); RAR(); _trace_state(); 
        break;

        case 0b00100010: _trace<3>(opcode); SHLD(); _trace_state(); _trace_mem16_deref(); 
        break;

        case 0b00100111: _trace<1>(opcode); DAA(); _trace_state(); 
        break;

        case 0b00101010: _trace<3>(opcode); LHLD(); _trace_state(); _trace_mem16_deref(); 
        break;

        case 0b00101111: _trace<1>(opcode); CMA(); _trace_state(); 
        break;

        case 0b00110010: _trace<3>(opcode); STA(); _trace_state(); _trace_mem16_deref(); 
        break;

        case 0b00110111: _trace<1>(opcode); STC(); _trace_state(); 
        break;

        case 0b00111010: _trace<3>(opcode); LDA(); _trace_state(); _trace_mem16_deref(); 
        break;

        case 0b00111111: _trace<1>(opcode); CMC(); _trace_state(); 
        break;

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
        case 0b01111111:
        case 0b01000110:
        case 0b01001110:
        case 0b01010110:
        case 0b01011110:
        case 0b01100110:
        case 0b01101110:
        case 0b01111110:
        case 0b01110000:
        case 0b01110001:
        case 0b01110010:
        case 0b01110011:
        case 0b01110100:
        case 0b01110101:
        case 0b01110111: _trace<1>(opcode); MOV(dst_sel, src_sel); _trace_state();
                         if (is_memref(dst_sel) or is_memref(src_sel)) _trace_reg16_deref(cpu_registers16::HL); 
        break;
        
        case 0b01110110: _trace<1>(opcode); HLT(); 
        break;

        //     ..ALUSSS
        case 0b10000000:
        case 0b10000001:
        case 0b10000010:
        case 0b10000011:
        case 0b10000100:
        case 0b10000101:
        case 0b10000111:
        case 0b10001000:
        case 0b10001001:
        case 0b10001010:
        case 0b10001011:
        case 0b10001100:
        case 0b10001101:
        case 0b10001111:
        case 0b10010000:
        case 0b10010001:
        case 0b10010010:
        case 0b10010011:
        case 0b10010100:
        case 0b10010101:
        case 0b10010111:
        case 0b10011000:
        case 0b10011001:
        case 0b10011010:
        case 0b10011011:
        case 0b10011100:
        case 0b10011101:
        case 0b10011111:
        case 0b10100000:
        case 0b10100001:
        case 0b10100010:
        case 0b10100011:
        case 0b10100100:
        case 0b10100101:
        case 0b10100111:
        case 0b10101000:
        case 0b10101001:
        case 0b10101010:
        case 0b10101011:
        case 0b10101100:
        case 0b10101101:
        case 0b10101111:
        case 0b10111000:
        case 0b10111001:
        case 0b10111010:
        case 0b10111011:
        case 0b10111100:
        case 0b10111101:
        case 0b10111111:
        case 0b10000110:
        case 0b10001110:
        case 0b10010110:
        case 0b10011110:
        case 0b10100110:
        case 0b10101110:
        case 0b10111110:
        case 0b10110000:
        case 0b10110001:
        case 0b10110010:
        case 0b10110011:
        case 0b10110100:
        case 0b10110101:
        case 0b10110111:
        case 0b10110110: _trace<1>(opcode); ALU_OPERATIONS_A(src_sel, opcode & 0b111);
        break;

        //     ..CCC....
        case 0b11000000:
        case 0b11001000:
        case 0b11010000:
        case 0b11011000:
        case 0b11100000:
        case 0b11101000:
        case 0b11110000:
        case 0b11111000: _trace<1>(opcode); RETURN_ON((opcode & 0b00111000) >> 3); _trace_state();

        default:         _trace_error(opcode); 
        break;
    }
}

bool cpu::resolve_flag_cond(u8 cc) {
    switch (cc) {
        case 0b000: return !state.get_flag(cpu_flags::Z);
        case 0b001: return state.get_flag(cpu_flags::Z); 
        case 0b010: return !state.get_flag(cpu_flags::C);
        case 0b011: return state.get_flag(cpu_flags::C); 
        case 0b100: return !state.get_flag(cpu_flags::P);
        case 0b101: return state.get_flag(cpu_flags::P); 
        case 0b110: return !state.get_flag(cpu_flags::S);
        case 0b111: return state.get_flag(cpu_flags::S);
        default: return false;
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