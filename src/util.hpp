#ifndef UTIL_HPP_
#define UTIL_HPP_

#include "typedef.hpp"

class util {
public:
    template <u8 opcode>
    static constexpr const char* get_opcode_str() {
        if constexpr      (opcode == 0x00) { return "NOP"; }
        else if constexpr (opcode == 0x01) { return "LXI B, D16"; }
        else if constexpr (opcode == 0x02) { return "STAX B"; }
        else if constexpr (opcode == 0x03) { return "INX B"; }
        else if constexpr (opcode == 0x04) { return "INR B"; }
        else if constexpr (opcode == 0x05) { return "DCR B"; }
        else if constexpr (opcode == 0x06) { return "MVI B, D8"; }
        else if constexpr (opcode == 0x07) { return "RLC"; }
        else if constexpr (opcode == 0x09) { return "DAD B"; }
        else if constexpr (opcode == 0x0A) { return "LDAX B"; }
        else if constexpr (opcode == 0x0B) { return "DCX B"; }
        else if constexpr (opcode == 0x0C) { return "INR C"; }
        else if constexpr (opcode == 0x0D) { return "DCR C"; }
        else if constexpr (opcode == 0x0E) { return "MVI C, D8"; }
        else if constexpr (opcode == 0x0F) { return "RRC"; }
        else if constexpr (opcode == 0x11) { return "LXI D, D16"; }
        else if constexpr (opcode == 0x12) { return "STAX D"; }
        else if constexpr (opcode == 0x13) { return "INX D"; }
        else if constexpr (opcode == 0x14) { return "INR D"; }
        else if constexpr (opcode == 0x15) { return "DCR D"; }
        else if constexpr (opcode == 0x16) { return "MVI D, D8"; }
        else if constexpr (opcode == 0x17) { return "RAL"; }
        else if constexpr (opcode == 0x19) { return "DAD D"; }
        else if constexpr (opcode == 0x1A) { return "LDAX D"; }
        else if constexpr (opcode == 0x1B) { return "DCX D"; }
        else if constexpr (opcode == 0x1C) { return "INR E"; }
        else if constexpr (opcode == 0x1D) { return "DCR E"; }
        else if constexpr (opcode == 0x1E) { return "MVI E, D8"; }
        else if constexpr (opcode == 0x1F) { return "RAR"; }
        else if constexpr (opcode == 0x21) { return "LXI H, D16"; }
        else if constexpr (opcode == 0x22) { return "SHLD adr"; }
        else if constexpr (opcode == 0x23) { return "INX H"; }
        else if constexpr (opcode == 0x24) { return "INR H"; }
        else if constexpr (opcode == 0x25) { return "DCR H"; }
        else if constexpr (opcode == 0x26) { return "MVI H, D8"; }
        else if constexpr (opcode == 0x27) { return "DAA"; }
        else if constexpr (opcode == 0x29) { return "DAD H"; }
        else if constexpr (opcode == 0x2A) { return "LHLD adr"; }
        else if constexpr (opcode == 0x2B) { return "DCX H"; }
        else if constexpr (opcode == 0x2C) { return "INR L"; }
        else if constexpr (opcode == 0x2D) { return "DCR L"; }
        else if constexpr (opcode == 0x2E) { return "MVI L, D8"; }
        else if constexpr (opcode == 0x2F) { return "CMA"; }
        else if constexpr (opcode == 0x31) { return "LXI SP, D16"; }
        else if constexpr (opcode == 0x32) { return "STA adr"; }
        else if constexpr (opcode == 0x33) { return "INX SP"; }
        else if constexpr (opcode == 0x34) { return "INR M"; }
        else if constexpr (opcode == 0x35) { return "DCR M"; }
        else if constexpr (opcode == 0x36) { return "MVI M, D8"; }
        else if constexpr (opcode == 0x37) { return "STC"; }
        else if constexpr (opcode == 0x39) { return "DAD SP"; }
        else if constexpr (opcode == 0x3A) { return "LDA adr"; }
        else if constexpr (opcode == 0x3B) { return "DCX SP"; }
        else if constexpr (opcode == 0x3C) { return "INR A"; }
        else if constexpr (opcode == 0x3D) { return "DCR A"; }
        else if constexpr (opcode == 0x3E) { return "MVI A, D8"; }
        else if constexpr (opcode == 0x3F) { return "CMC"; }
        else { static_assert(false, "Unknown template parameter opcode was provided to get_opcode_str<>!"); }
    }
};

#endif