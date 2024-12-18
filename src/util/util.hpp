#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "typedef.hpp"

/**
 * @brief Contains utility functions for the emulator.
 *
 * This class contains various static methods that are used throughout the emulator to provide
 * useful information and overall shorten the code.
 */
class util {
public:

    /**
     * @brief Helper class for managing print redirection.
     *
     * This class provides an object that handles printing going through it into a set destination.
     * It can be useful in context of tests, where some specific output might be useful to be redirected
     * to a text file rather than stdout.
     *
     * The temporary redirected destination is set(), then reset() to go back to the default destination.
     */
    class print_helper {
    private:
        std::ostream& by_default;
        std::ofstream file_redirect;

    public:
        /// @brief Set a redirection to file.
        void set(const char* filename) {
            file_redirect = std::ofstream(filename, std::ios::binary | std::ios::trunc);
            if (!file_redirect.is_open())
                throw std::invalid_argument("Could not open file for printer.");
        }

        /// @brief Reset and fallback to default destination.
        void reset() {
            if (file_redirect.is_open()) {
                file_redirect.flush();
                file_redirect.close();
            }
        }

        /// @brief Print data to the set destination.
        template <typename T>
        void print(const T& data) {
            if (file_redirect.is_open()) {
                file_redirect << data;
                if (file_redirect.fail())
                    throw std::runtime_error("Failed to write to file.");
            } else
                by_default << data;
        }

        /// @brief Operator overload that calls print(), matching common usage of << operator.
        template <typename T>
        print_helper& operator<<(const T& data) {
            print(data);
            return *this;
        }

        print_helper(std::ostream& by_default) : by_default(by_default) {}
        ~print_helper() { if (file_redirect.is_open()) reset(); }
    };

    /// @brief Get the absolute path of the main executable as a std::string.
    static std::string get_absolute_dir() {
        static constexpr usize MAX_BUF = 512;
        char buffer[MAX_BUF];
        ssize_t pathLength = readlink("/proc/self/exe", buffer, MAX_BUF - 1);

        if (pathLength == -1)
            throw std::runtime_error("failed to get_absolute_dir()");

        std::string path(buffer, pathLength);

        return path.substr(0, path.rfind('/') + 1);
    }

    /// @brief Get the parity of an integer type value.
    /// @note This is an alternative if __builtin_parity is not available.
    template <typename T>
    constexpr static bool get_parity(T value) {
        static_assert(std::is_integral<T>::value, "T must be an integral type");
        usize p = 0;
        for (usize i = 0; i < 8 * sizeof(T); ++i)
            p += (value >> i) & 1;
        return p & 1;
    }

    /// @brief Make an integer into an hexadecimal std::string.
    template <typename T>
    static std::string to_hex_s(T value, usize zfill = 4) {
        static_assert(std::is_integral<T>::value, "T must be an integral type");
        std::stringstream stream;
        stream << "0x" << std::setfill('0') << std::setw(zfill) << std::hex << value;
        return stream.str();
    }

    /// @brief Get the disassembled name of an opcode.
    constexpr static const char* get_opcode_str(u8 opcode) {
        switch (opcode) {
            case 0x00: return "NOP"; break;
            case 0x01: return "LXI B, D16"; break;
            case 0x02: return "STAX B"; break;
            case 0x03: return "INX B"; break;
            case 0x04: return "INR B"; break;
            case 0x05: return "DCR B"; break;
            case 0x06: return "MVI B, D8"; break;
            case 0x07: return "RLC"; break;
            case 0x09: return "DAD B"; break;
            case 0x0A: return "LDAX B"; break;
            case 0x0B: return "DCX B"; break;
            case 0x0C: return "INR C"; break;
            case 0x0D: return "DCR C"; break;
            case 0x0E: return "MVI C, D8"; break;
            case 0x0F: return "RRC"; break;
            case 0x11: return "LXI D, D16"; break;
            case 0x12: return "STAX D"; break;
            case 0x13: return "INX D"; break;
            case 0x14: return "INR D"; break;
            case 0x15: return "DCR D"; break;
            case 0x16: return "MVI D, D8"; break;
            case 0x17: return "RAL"; break;
            case 0x19: return "DAD D"; break;
            case 0x1A: return "LDAX D"; break;
            case 0x1B: return "DCX D"; break;
            case 0x1C: return "INR E"; break;
            case 0x1D: return "DCR E"; break;
            case 0x1E: return "MVI E, D8"; break;
            case 0x1F: return "RAR"; break;
            case 0x21: return "LXI H, D16"; break;
            case 0x22: return "SHLD adr"; break;
            case 0x23: return "INX H"; break;
            case 0x24: return "INR H"; break;
            case 0x25: return "DCR H"; break;
            case 0x26: return "MVI H, D8"; break;
            case 0x27: return "DAA"; break;
            case 0x29: return "DAD H"; break;
            case 0x2A: return "LHLD adr"; break;
            case 0x2B: return "DCX H"; break;
            case 0x2C: return "INR L"; break;
            case 0x2D: return "DCR L"; break;
            case 0x2E: return "MVI L, D8"; break;
            case 0x2F: return "CMA"; break;
            case 0x31: return "LXI SP, D16"; break;
            case 0x32: return "STA adr"; break;
            case 0x33: return "INX SP"; break;
            case 0x34: return "INR M"; break;
            case 0x35: return "DCR M"; break;
            case 0x36: return "MVI M, D8"; break;
            case 0x37: return "STC"; break;
            case 0x39: return "DAD SP"; break;
            case 0x3A: return "LDA adr"; break;
            case 0x3B: return "DCX SP"; break;
            case 0x3C: return "INR A"; break;
            case 0x3D: return "DCR A"; break;
            case 0x3E: return "MVI A, D8"; break;
            case 0x3F: return "CMC"; break;
            case 0x40: return "MOV B, B"; break;
            case 0x41: return "MOV B, C"; break;
            case 0x42: return "MOV B, D"; break;
            case 0x43: return "MOV B, E"; break;
            case 0x44: return "MOV B, H"; break;
            case 0x45: return "MOV B, L"; break;
            case 0x46: return "MOV B, M"; break;
            case 0x47: return "MOV B, A"; break;
            case 0x48: return "MOV C, B"; break;
            case 0x49: return "MOV C, C"; break;
            case 0x4A: return "MOV C, D"; break;
            case 0x4B: return "MOV C, E"; break;
            case 0x4C: return "MOV C, H"; break;
            case 0x4D: return "MOV C, L"; break;
            case 0x4E: return "MOV C, M"; break;
            case 0x4F: return "MOV C, A"; break;
            case 0x50: return "MOV D, B"; break;
            case 0x51: return "MOV D, C"; break;
            case 0x52: return "MOV D, D"; break;
            case 0x53: return "MOV D, E"; break;
            case 0x54: return "MOV D, H"; break;
            case 0x55: return "MOV D, L"; break;
            case 0x56: return "MOV D, M"; break;
            case 0x57: return "MOV D, A"; break;
            case 0x58: return "MOV E, B"; break;
            case 0x59: return "MOV E, C"; break;
            case 0x5A: return "MOV E, D"; break;
            case 0x5B: return "MOV E, E"; break;
            case 0x5C: return "MOV E, H"; break;
            case 0x5D: return "MOV E, L"; break;
            case 0x5E: return "MOV E, M"; break;
            case 0x5F: return "MOV E, A"; break;
            case 0x60: return "MOV H, B"; break;
            case 0x61: return "MOV H, C"; break;
            case 0x62: return "MOV H, D"; break;
            case 0x63: return "MOV H, E"; break;
            case 0x64: return "MOV H, H"; break;
            case 0x65: return "MOV H, L"; break;
            case 0x66: return "MOV H, M"; break;
            case 0x67: return "MOV H, A"; break;
            case 0x68: return "MOV L, B"; break;
            case 0x69: return "MOV L, C"; break;
            case 0x6A: return "MOV L, D"; break;
            case 0x6B: return "MOV L, E"; break;
            case 0x6C: return "MOV L, H"; break;
            case 0x6D: return "MOV L, L"; break;
            case 0x6E: return "MOV L, M"; break;
            case 0x6F: return "MOV L, A"; break;
            case 0x70: return "MOV M, B"; break;
            case 0x71: return "MOV M, C"; break;
            case 0x72: return "MOV M, D"; break;
            case 0x73: return "MOV M, E"; break;
            case 0x74: return "MOV M, H"; break;
            case 0x75: return "MOV M, L"; break;
            case 0x76: return "HLT"; break;
            case 0x77: return "MOV M, A"; break;
            case 0x78: return "MOV A, B"; break;
            case 0x79: return "MOV A, C"; break;
            case 0x7A: return "MOV A, D"; break;
            case 0x7B: return "MOV A, E"; break;
            case 0x7C: return "MOV A, H"; break;
            case 0x7D: return "MOV A, L"; break;
            case 0x7E: return "MOV A, M"; break;
            case 0x7F: return "MOV A, A"; break;
            case 0x80: return "ADD B"; break;
            case 0x81: return "ADD C"; break;
            case 0x82: return "ADD D"; break;
            case 0x83: return "ADD E"; break;
            case 0x84: return "ADD H"; break;
            case 0x85: return "ADD L"; break;
            case 0x86: return "ADD M"; break;
            case 0x87: return "ADD A"; break;
            case 0x88: return "ADC B"; break;
            case 0x89: return "ADC C"; break;
            case 0x8A: return "ADC D"; break;
            case 0x8B: return "ADC E"; break;
            case 0x8C: return "ADC H"; break;
            case 0x8D: return "ADC L"; break;
            case 0x8E: return "ADC M"; break;
            case 0x8F: return "ADC A"; break;
            case 0x90: return "SUB B"; break;
            case 0x91: return "SUB C"; break;
            case 0x92: return "SUB D"; break;
            case 0x93: return "SUB E"; break;
            case 0x94: return "SUB H"; break;
            case 0x95: return "SUB L"; break;
            case 0x96: return "SUB M"; break;
            case 0x97: return "SUB A"; break;
            case 0x98: return "SBB B"; break;
            case 0x99: return "SBB C"; break;
            case 0x9A: return "SBB D"; break;
            case 0x9B: return "SBB E"; break;
            case 0x9C: return "SBB H"; break;
            case 0x9D: return "SBB L"; break;
            case 0x9E: return "SBB M"; break;
            case 0x9F: return "SBB A"; break;
            case 0xA0: return "ANA B"; break;
            case 0xA1: return "ANA C"; break;
            case 0xA2: return "ANA D"; break;
            case 0xA3: return "ANA E"; break;
            case 0xA4: return "ANA H"; break;
            case 0xA5: return "ANA L"; break;
            case 0xA6: return "ANA M"; break;
            case 0xA7: return "ANA A"; break;
            case 0xA8: return "XRA B"; break;
            case 0xA9: return "XRA C"; break;
            case 0xAA: return "XRA D"; break;
            case 0xAB: return "XRA E"; break;
            case 0xAC: return "XRA H"; break;
            case 0xAD: return "XRA L"; break;
            case 0xAE: return "XRA M"; break;
            case 0xAF: return "XRA A"; break;
            case 0xB0: return "ORA B"; break;
            case 0xB1: return "ORA C"; break;
            case 0xB2: return "ORA D"; break;
            case 0xB3: return "ORA E"; break;
            case 0xB4: return "ORA H"; break;
            case 0xB5: return "ORA L"; break;
            case 0xB6: return "ORA M"; break;
            case 0xB7: return "ORA A"; break;
            case 0xB8: return "CMP B"; break;
            case 0xB9: return "CMP C"; break;
            case 0xBA: return "CMP D"; break;
            case 0xBB: return "CMP E"; break;
            case 0xBC: return "CMP H"; break;
            case 0xBD: return "CMP L"; break;
            case 0xBE: return "CMP M"; break;
            case 0xBF: return "CMP A"; break;
            case 0xC0: return "RNZ"; break;
            case 0xC1: return "POP B"; break;
            case 0xC2: return "JNZ adr"; break;
            case 0xC3: return "JMP adr"; break;
            case 0xC4: return "CNZ adr"; break;
            case 0xC5: return "PUSH B"; break;
            case 0xC6: return "ADI D8"; break;
            case 0xC7: return "RST 0"; break;
            case 0xC8: return "RZ"; break;
            case 0xC9: return "RET"; break;
            case 0xCA: return "JZ adr"; break;
            case 0xCB: return "JMP adr"; break;
            case 0xCC: return "CZ adr"; break;
            case 0xCD: return "CALL adr"; break;
            case 0xCE: return "ACI D8"; break;
            case 0xCF: return "RST 1"; break;
            case 0xD0: return "RNC"; break;
            case 0xD1: return "POP D"; break;
            case 0xD2: return "JNC adr"; break;
            case 0xD3: return "OUT D8"; break;
            case 0xD4: return "CNC adr"; break;
            case 0xD5: return "PUSH D"; break;
            case 0xD6: return "SUI D8"; break;
            case 0xD7: return "RST 2"; break;
            case 0xD8: return "RC"; break;
            case 0xD9: return "RET"; break;
            case 0xDA: return "JC adr"; break;
            case 0xDB: return "IN D8"; break;
            case 0xDC: return "CC adr"; break;
            case 0xDE: return "SBI D8"; break;
            case 0xDF: return "RST 3"; break;
            case 0xE0: return "RPO"; break;
            case 0xE1: return "POP H"; break;
            case 0xE2: return "JPO adr"; break;
            case 0xE3: return "XTHL"; break;
            case 0xE4: return "CPO adr"; break;
            case 0xE5: return "PUSH H"; break;
            case 0xE6: return "ANI D8"; break;
            case 0xE7: return "RST 4"; break;
            case 0xE8: return "RPE"; break;
            case 0xE9: return "PCHL"; break;
            case 0xEA: return "JPE adr"; break;
            case 0xEB: return "XCHG"; break;
            case 0xEC: return "CPE adr"; break;
            case 0xEE: return "XRI D8"; break;
            case 0xEF: return "RST 5"; break;
            case 0xF0: return "RP"; break;
            case 0xF1: return "POP PSW"; break;
            case 0xF2: return "JP adr"; break;
            case 0xF3: return "DI"; break;
            case 0xF4: return "CP adr"; break;
            case 0xF5: return "PUSH PSW"; break;
            case 0xF6: return "ORI D8"; break;
            case 0xF7: return "RST 6"; break;
            case 0xF8: return "RM"; break;
            case 0xF9: return "SPHL"; break;
            case 0xFA: return "JM adr"; break;
            case 0xFB: return "EI"; break;
            case 0xFC: return "CM adr"; break;
            case 0xFE: return "CPI D8"; break;
            case 0xFF: return "RST 7"; break;
            default: return "UNKNOWN"; break;
        }
    }
};

#endif