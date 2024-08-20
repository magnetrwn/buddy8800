#include "cpu.hpp"

void cpu::_trace_state() {
    #ifdef ENABLE_TRACE
    constexpr static const char* CHG = "\x1B[43;01m";
    static cpu_state last_state = state;

    printf("\x1B[44;01mA: %s%02hhX\x1B[44;01m BC: %s%04hX\x1B[44;01m DE: %s%04hX\x1B[44;01m HL: %s%04hX\x1B[44;01m \x1B[0m\n"
           "\x1B[44;01m SP: %s%04hX\x1B[44;01m PC: %s%04hX\x1B[44;01m F: %s%c\x1B[44;01m %s%c\x1B[44;01m %s%c\x1B[44;01m %s%c\x1B[44;01m %s%c\x1B[44;01m  \x1B[0m\n",
        last_state.get_register8(cpu_registers8::A) == state.get_register8(cpu_registers8::A) ? "" : CHG,
        state.get_register8(cpu_registers8::A),
        last_state.get_register16(cpu_registers16::BC) == state.get_register16(cpu_registers16::BC) ? "" : CHG,
        state.get_register16(cpu_registers16::BC),
        last_state.get_register16(cpu_registers16::DE) == state.get_register16(cpu_registers16::DE) ? "" : CHG,
        state.get_register16(cpu_registers16::DE),
        last_state.get_register16(cpu_registers16::HL) == state.get_register16(cpu_registers16::HL) ? "" : CHG,
        state.get_register16(cpu_registers16::HL),
        last_state.get_register16(cpu_registers16::SP) == state.get_register16(cpu_registers16::SP) ? "" : CHG,
        state.get_register16(cpu_registers16::SP),
        last_state.get_register16(cpu_registers16::PC) == state.get_register16(cpu_registers16::PC) ? "" : CHG,
        state.get_register16(cpu_registers16::PC),
        last_state.get_flag(cpu_flags::S) == state.get_flag(cpu_flags::S) ? "" : CHG,
        state.get_flag(cpu_flags::S) ? 'S' : '/',
        last_state.get_flag(cpu_flags::Z) == state.get_flag(cpu_flags::Z) ? "" : CHG,
        state.get_flag(cpu_flags::Z) ? 'Z' : '/',
        last_state.get_flag(cpu_flags::AC) == state.get_flag(cpu_flags::AC) ? "" : CHG,
        state.get_flag(cpu_flags::AC) ? 'A' : '/',
        last_state.get_flag(cpu_flags::P) == state.get_flag(cpu_flags::P) ? "" : CHG,
        state.get_flag(cpu_flags::P) ? 'P' : '/',
        last_state.get_flag(cpu_flags::C) == state.get_flag(cpu_flags::C) ? "" : CHG,
        state.get_flag(cpu_flags::C) ? 'C' : '/');

    last_state = state;
    #endif
}

void cpu::_trace_reg16_deref([[maybe_unused]] cpu_registers16 reg) {
    #ifdef ENABLE_TRACE
    printf("\x1B[42;01m(%s: %04hX): %02hhX                   \x1B[0m\n",
        (reg == cpu_registers16::AF) ? "AF" : 
            (reg == cpu_registers16::BC) ? "BC" : 
            (reg == cpu_registers16::DE) ? "DE" : 
            (reg == cpu_registers16::HL) ? "HL" : 
            (reg == cpu_registers16::SP) ? "SP" : 
            "PC",
        state.get_register16(reg),
        memory[state.get_register16(reg)]);
    #endif
}

/*void cpu::_trace_memref16_deref() {
    #ifdef ENABLE_TRACE
    printf("\x1B[41;01m(%02hhX%02hhX): %02hhX                       \x1B[0m\n",
        memory[pc() + 1],
        memory[pc()],
        memory[(memory[pc() + 1] << 8) | memory[pc()]]);
    #endif
}*/

void cpu::_trace_stackptr16_deref() {
    #ifdef ENABLE_TRACE
    printf("\x1B[42;01m(%04hX): %02hhX (%04hX): %02hhX            \x1B[0m\n",
        state.get_register16(cpu_registers16::SP),
        memory[state.get_register16(cpu_registers16::SP)],
        state.get_register16(cpu_registers16::SP) + 1,
        memory[state.get_register16(cpu_registers16::SP) + 1]);
    #endif
}

void cpu::_trace_error([[maybe_unused]] u8 opc) {
    #ifdef ENABLE_TRACE
    printf("%04hX    %02hhX      \t \x1B[31;01mUNKNOWN\x1B[0m\n", pc() - 1, opc);
    throw std::runtime_error("Unknown opcode hit.");
    #endif
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

void cpu::handle_bdos() {
    if (pc() == 0x0000) {
        if (just_booted) {

            #ifdef ENABLE_TRACE
            puts("\x1B[47;01mBDOS 0x0000: Reset vector!       \x1B[0m");
            #endif

            just_booted = false;
            return;
        }

        #ifdef ENABLE_TRACE
        puts("\x1B[47;01mBDOS 0x0000: That's all folks!   \x1B[0m");
        #endif

        memory[0] = 0b01110110;
    }

    if (pc() == 0x0005) {
        u8 c = state.get_register8(cpu_registers8::C);

        #ifdef ENABLE_TRACE
        puts("\x1B[47;01mBDOS 0x0005: Wants to print:     \x1B[0m");
        #endif

        if (c == 0x02)
            printer << state.get_register8(cpu_registers8::E);

        else if (c == 0x09)
            for (u16 de = state.get_register16(cpu_registers16::DE); memory[de] != '$'; ++de)
                printer << memory[de];

        else
            throw std::runtime_error("Unknown BDOS 0x0005 call parameters.");

        fetch();

        #ifdef ENABLE_TRACE
        putchar('\n');
        _trace<1>(0b11001001); 
        #endif
        
        RETURN();
    }
}

void cpu::set_printer_to_file(const char* filename) {
    printer.set(filename);
}

void cpu::reset_printer() {
    printer.reset();
}

void cpu::set_handle_bdos(bool should) {
    do_handle_bdos = should;
}

void cpu::step() {
    if (halted)
        return;
    if (do_handle_bdos)
        handle_bdos();
    execute(fetch()); 
}

void cpu::load(std::vector<u8>::iterator begin, std::vector<u8>::iterator end, usize offset, bool auto_reset_vector) {
    usize dist = std::distance(begin, end);

    if (dist > memory.size() - offset)
        throw std::out_of_range("Not enough space in emulated memory.");

    std::copy(begin, end, memory.begin() + offset);

    if (auto_reset_vector) {
        if (offset <= 2)
            throw std::out_of_range("First program bytes will be overwritten by reset vector.");

        memory[0] = 0xC3;
        memory[1] = static_cast<u8>(offset & 0xFF);
        memory[2] = static_cast<u8>(offset >> 8);
    }
}

void cpu::load_state(const cpu_state& new_state) {
    state = new_state;
}

cpu_state cpu::save_state() const {
    return state;
}

bool cpu::is_halted() const {
    return halted;
}

void cpu::clear() {
    state = cpu_state();
    just_booted = true;
    memory.fill(0);
    halted = false;
}

void cpu::execute(u8 opcode) {
    cpu_registers16 pair_sel = static_cast<cpu_registers16>((((opcode & 0b00110000) >> 4) & 0b11) + 1);
    cpu_registers8 dst_sel = cpu_reg8_decode[(opcode >> 3) & 0b111];
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

        case 0b00100010: _trace<3>(opcode); SHLD(); _trace_state(); //_trace_mem16_deref(); 
        break;

        case 0b00100111: _trace<1>(opcode); DAA(); _trace_state(); 
        break;

        case 0b00101010: _trace<3>(opcode); LHLD(); _trace_state(); //_trace_mem16_deref(); 
        break;

        case 0b00101111: _trace<1>(opcode); CMA(); _trace_state(); 
        break;

        case 0b00110010: _trace<3>(opcode); STA(); _trace_state(); //_trace_mem16_deref(); 
        break;

        case 0b00110111: _trace<1>(opcode); STC(); _trace_state(); 
        break;

        case 0b00111010: _trace<3>(opcode); LDA(); _trace_state(); //_trace_mem16_deref(); 
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
        case 0b10110110: _trace<1>(opcode); ALU_OPERATIONS_A(src_sel, (opcode >> 3) & 0b111); _trace_state();
        break;

        //     ..CCC...
        case 0b11000000:
        case 0b11001000:
        case 0b11010000:
        case 0b11011000:
        case 0b11100000:
        case 0b11101000:
        case 0b11110000:
        case 0b11111000: _trace<1>(opcode); RETURN_ON((opcode >> 3) & 0b111); _trace_state();
        break;

        //     ..RP....
        case 0b11000001:
        case 0b11010001:
        case 0b11100001:
        case 0b11110001: _trace<1>(opcode); POP(pair_sel); _trace_state();
        break;

        //     ..CCC...
        case 0b11000010:
        case 0b11001010:
        case 0b11010010:
        case 0b11011010:
        case 0b11100010:
        case 0b11101010:
        case 0b11110010:
        case 0b11111010: _trace<3>(opcode); JUMP_ON((opcode >> 3) & 0b111); _trace_state();
        break;

        case 0b11000011: _trace<3>(opcode); JMP(); _trace_state();
        break;

        //     ..CCC...
        case 0b11000100:
        case 0b11001100:
        case 0b11010100:
        case 0b11011100:
        case 0b11100100:
        case 0b11101100:
        case 0b11110100:
        case 0b11111100: _trace<3>(opcode); CALL_ON((opcode & 0b00111000) >> 3); _trace_state();
        break;

        //     ..RP....
        case 0b11000101:
        case 0b11010101:
        case 0b11100101:
        case 0b11110101: _trace<1>(opcode); PUSH(pair_sel); _trace_state();
        break;

        //     ..ALU...
        case 0b11000110:
        case 0b11001110:
        case 0b11010110:
        case 0b11011110:
        case 0b11100110:
        case 0b11101110:
        case 0b11110110:
        case 0b11111110: _trace<2>(opcode); ALU_OPERATIONS_A_IMM((opcode >> 3) & 0b111); _trace_state();
        break;

        //     ..NNN...
        case 0b11000111:
        case 0b11001111:
        case 0b11010111:
        case 0b11011111:
        case 0b11100111:
        case 0b11101111:
        case 0b11110111:
        case 0b11111111: _trace<1>(opcode); RST((opcode >> 3) & 0b111); _trace_state();
        break;

        case 0b11001001: _trace<1>(opcode); RETURN(); _trace_state();
        break;

        case 0b11001101: _trace<3>(opcode); CALL(); _trace_state();
        break;

        case 0b11010011: _trace<2>(opcode); OUT();
        break;

        case 0b11011011: _trace<2>(opcode); IN();
        break;

        case 0b11100011: _trace<1>(opcode); XTHL(); _trace_state(); _trace_stackptr16_deref();
        break;

        case 0b11101001: _trace<1>(opcode); PCHL(); _trace_state();
        break;

        case 0b11101011: _trace<1>(opcode); XCHG(); _trace_state();
        break;

        case 0b11110011: _trace<1>(opcode); DI();
        break;

        case 0b11111001: _trace<1>(opcode); SPHL(); _trace_state();
        break;

        case 0b11111011: _trace<1>(opcode); EI();
        break;

        default:         _trace_error(opcode); 
        break;
    }
}
