#ifndef CARD_HPP_
#define CARD_HPP_

#include <array>
#include <stdexcept>
#include <iostream>

#include "typedef.hpp"
#include "pty.hpp"

template <usize start_adr, usize adr_range>
class card {
public:
    constexpr inline bool in_range(u16 adr) const {return adr >= start_adr and adr <= (start_adr + adr_range); }
    constexpr inline void set_bit(u16 adr, u8 bitmask) { write(adr, read(adr) | bitmask); }
    constexpr inline void unset_bit(u16 adr, u8 bitmask) { write(adr, read(adr) & ~bitmask); }
    constexpr inline void set_if_bit(u16 adr, u8 bitmask, bool value) { value ? set_bit(adr, bitmask) : unset_bit(adr, bitmask); }

    virtual u8 read(u16 adr) = 0;

    virtual void write(u16 adr, u8 byte) = 0;
    virtual void write_lock() = 0;
    virtual void write_unlock() = 0;

    virtual void refresh() = 0;

    virtual void clear() = 0;

    virtual ~card() = default;
};

/**
 * @warning Out of range addresses are not checked, they should be checked by the bus instead, to avoid calling in_range() twice.
 */
template <usize start_adr, usize capacity, bool construct_then_write_lock>
class data_card : public card<start_adr, capacity> {
private:
    std::array<u8, capacity> data;
    bool write_locked;

public:
    data_card(bool lock = construct_then_write_lock) : write_locked(lock) {}

    template <typename iter_t>
    data_card(iter_t begin, iter_t end, bool lock = construct_then_write_lock) : write_locked(lock) {
        std::copy(begin, end, data.begin());
    }

    inline u8 read(u16 adr) override {
        return data[adr - start_adr];
    }

    inline void write(u16 adr, u8 byte) override {
        if (!write_locked)
            data[adr - start_adr] = byte;
    }

    inline void write_lock() override {
        write_locked = true;
    }

    inline void write_unlock() override {
        write_locked = false;
    }

    inline void clear() override {
        if (!write_locked)
            data.fill(0x00);
    }
};

template <usize start_adr, usize capacity>
using ram_card = data_card<start_adr, capacity, false>;

template <usize start_adr, usize capacity>
using rom_card = data_card<start_adr, capacity, true>;

/**
 * Based on the MC6850 ACIA.
 */
enum serial_register {
    TX_DATA, RX_DATA, /*CONTROL,*/ STATUS
};

/**
 * Based on the MC6850 ACIA.
 */
enum serial_status_flags {
    RDRF = 0x01, TDRE = 0x02, DCD = 0x04, CTS = 0x08, FE = 0x10, OVRN = 0x20, PE = 0x40, IRQ = 0x80
};

/**
 * Based on the MC6850 ACIA.
 * @warning Out of range addresses are not checked, they should be checked by the bus instead, to avoid calling in_range() twice.
 */
template <usize start_adr, usize base_clock = 19200>
class serial_card : public card<start_adr, 4> {
private:
    pty serial;
    std::array<u8, 3> registers;
    bool rts;

    constexpr u8 TX_DATA() const { return registers[static_cast<usize>(serial_register::TX_DATA)]; }
    constexpr u8 RX_DATA() const { return registers[static_cast<usize>(serial_register::RX_DATA)]; }
    //constexpr u8 CONTROL() const { return registers[static_cast<usize>(serial_register::CONTROL)]; }
    constexpr u8 STATUS() const { return registers[static_cast<usize>(serial_register::STATUS)]; }
    constexpr void TX_DATA(u8 value) { registers[static_cast<usize>(serial_register::TX_DATA)] = value; }
    constexpr void RX_DATA(u8 value) { registers[static_cast<usize>(serial_register::RX_DATA)] = value; }
    //constexpr void CONTROL(u8 value) { registers[static_cast<usize>(serial_register::CONTROL)] = value; }
    constexpr void STATUS(u8 value) { registers[static_cast<usize>(serial_register::STATUS)] = value; }

    constexpr bool RDRF() const { return STATUS() & static_cast<u8>(serial_status_flags::RDRF); }
    constexpr bool TDRE() const { return STATUS() & static_cast<u8>(serial_status_flags::TDRE); }
    //constexpr bool DCD() const { return STATUS() & static_cast<u8>(serial_status_flags::DCD); }
    //constexpr bool CTS() const { return STATUS() & static_cast<u8>(serial_status_flags::CTS); }
    //constexpr bool FE() const { return STATUS() & static_cast<u8>(serial_status_flags::FE); }
    //constexpr bool OVRN() const { return STATUS() & static_cast<u8>(serial_status_flags::OVRN); }
    //constexpr bool PE() const { return STATUS() & static_cast<u8>(serial_status_flags::PE); }
    constexpr bool IRQ() const { return STATUS() & static_cast<u8>(serial_status_flags::IRQ); }
    constexpr void RDRF(bool value) { this->set_if_bit(start_adr + static_cast<usize>(serial_register::STATUS), static_cast<u8>(serial_status_flags::RDRF), value); }
    constexpr void TDRE(bool value) { this->set_if_bit(start_adr + static_cast<usize>(serial_register::STATUS), static_cast<u8>(serial_status_flags::TDRE), value); }
    //constexpr void DCD(bool value) { this->set_if_bit(start_adr + static_cast<usize>(serial_register::STATUS), static_cast<u8>(serial_status_flags::DCD), value); }
    //constexpr void CTS(bool value) { this->set_if_bit(start_adr + static_cast<usize>(serial_register::STATUS), static_cast<u8>(serial_status_flags::CTS), value); }
    //constexpr void FE(bool value) { this->set_if_bit(start_adr + static_cast<usize>(serial_register::STATUS), static_cast<u8>(serial_status_flags::FE), value); }
    //constexpr void OVRN(bool value) { this->set_if_bit(start_adr + static_cast<usize>(serial_register::STATUS), static_cast<u8>(serial_status_flags::OVRN), value); }
    //constexpr void PE(bool value) { this->set_if_bit(start_adr + static_cast<usize>(serial_register::STATUS), static_cast<u8>(serial_status_flags::PE), value); }
    constexpr void IRQ(bool value) { this->set_if_bit(start_adr + static_cast<usize>(serial_register::STATUS), static_cast<u8>(serial_status_flags::IRQ), value); }

    constexpr bool RTS() const { return rts; }
    constexpr void RTS(bool value) { rts = value; }

    constexpr void reset() {
        registers.fill(0x00);
        serial.set_baud_rate(base_clock);
        TDRE(true);
        RTS(true);
    }

public:
    serial_card() {
        serial.open();
        std::cout << "Serial card '0x" << std::hex << start_adr << "' available on '" << serial.name() << "'." << std::endl;
        reset();
    }

    inline void refresh() override {
        if (!RDRF() and serial.poll()) {
            RX_DATA(serial.getch());
            RDRF(true);
        }

        if (!TDRE()) {
            serial.send(reinterpret_cast<const char*>(&TX_DATA()), 1);
            TDRE(true);
        }
    }

    inline u8 read(u16 adr) override {
        switch (adr) {
            case 0x00: return 0xFF; /// TX_DATA is write-only
            case 0x01: return RX_DATA();
            case 0x02: return 0xFF; /// CONTROL is write-only
            case 0x03: return STATUS();
        }
    }

    inline void write(u16 adr, u8 byte) override {
        switch (adr) {
            case 0x00: TX_DATA(byte); TDRE(false); break;

            case 0x01: break; /// RX_DATA is read-only

            case 0x02:
                /// Counter Divide select bits
                switch (byte & 0b00000011) {
                    //     ......DD
                    case 0b00000000: serial.set_baud_rate(base_clock); break;
                    case 0b00000001: serial.set_baud_rate(base_clock >> 4); break;
                    case 0b00000010: serial.set_baud_rate(base_clock >> 6); break;
                    case 0b00000011: reset(); break;
                }
                /// Word Select bits
                switch (byte & 0b00011100) {
                    //     ...WWW..
                    case 0b00000000: serial.setup(7, pty_parity::EVEN, 2); break;
                    case 0b00000100: serial.setup(7, pty_parity::ODD, 2); break;
                    case 0b00001000: serial.setup(7, pty_parity::EVEN, 1); break;
                    case 0b00001100: serial.setup(7, pty_parity::ODD, 1); break;
                    case 0b00010000: serial.setup(8, pty_parity::NONE, 2); break;
                    case 0b00010100: serial.setup(8, pty_parity::NONE, 1); break;
                    case 0b00011000: serial.setup(8, pty_parity::EVEN, 1); break;
                    case 0b00011100: serial.setup(8, pty_parity::ODD, 1); break;
                }
                /// Transmit Control bits
                switch (byte & 0b01100000) {
                    //     .CC.....
                    case 0b00000000: RTS(true); break;
                    case 0b00100000: RTS(true); break;
                    case 0b01000000: RTS(false); break;
                    case 0b01100000: RTS(true); serial.send_break(); break;
                }
                /// Interrupt Enable bit
                switch (byte & 0b10000000) {
                    //     I.......
                    case 0b00000000: IRQ(false); break;
                    case 0b10000000: IRQ(true); break;
                }
                break;

            case 0x03: break; /// STATUS is read-only
        }
    }
};

#endif