#include "cpu.hpp"

void cpu::execute(u8 opcode) {
    switch (opcode) {
        case 0b00000000: _trace<0b00000000, 1>(); NOP(); break;

        case 0b00000001: _trace<0b00000001, 3>(); LXI<cpu_registers16::BC>(); _trace_state(); break;
        case 0b00010001: _trace<0b00010001, 3>(); LXI<cpu_registers16::DE>(); _trace_state(); break;
        case 0b00100001: _trace<0b00100001, 3>(); LXI<cpu_registers16::HL>(); _trace_state(); break;
        case 0b00110001: _trace<0b00110001, 3>(); LXI<cpu_registers16::SP>(); _trace_state(); break;

        case 0b00000010: _trace<0b00000010, 1>(); STAX<cpu_registers16::BC>(); _trace_reg16_deref<cpu_registers16::BC>(); break;
        case 0b00010010: _trace<0b00010010, 1>(); STAX<cpu_registers16::DE>(); _trace_reg16_deref<cpu_registers16::DE>(); break;
        case 0b00100010: _trace<0b00100010, 1>(); STAX<cpu_registers16::HL>(); _trace_reg16_deref<cpu_registers16::HL>(); break;
        case 0b00110010: _trace<0b00110010, 1>(); STAX<cpu_registers16::SP>(); _trace_reg16_deref<cpu_registers16::SP>(); break;

        case 0b00000011: _trace<0b00000011, 1>(); INX<cpu_registers16::BC>(); _trace_state(); break;
        case 0b00010011: _trace<0b00010011, 1>(); INX<cpu_registers16::DE>(); _trace_state(); break;
        case 0b00100011: _trace<0b00100011, 1>(); INX<cpu_registers16::HL>(); _trace_state(); break;
        case 0b00110011: _trace<0b00110011, 1>(); INX<cpu_registers16::SP>(); _trace_state(); break;
        
        default: {
            _trace_error(opcode);
            return;
        }
    }
}