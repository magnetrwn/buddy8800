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
    constexpr bool in_range(u16 adr) const {
        return adr >= start_adr and adr <= (start_adr + adr_range);
    }

    virtual u8 read(u16 adr) = 0;

    virtual void write(u16 adr, u8 byte) = 0;
    virtual void write_lock() = 0;
    virtual void write_unlock() = 0;

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
enum class serial_register {
    TX_DATA, RX_DATA, CONTROL, STATUS
};

/**
 * Based on the MC6850 ACIA.
 * @warning Out of range addresses are not checked, they should be checked by the bus instead, to avoid calling in_range() twice.
 */
template <usize start_adr>
class serial_card : public card<start_adr, 4> {
private:
    pty serial;
    std::array<u8, 4> registers;

public:
    serial_card() {
        serial.open();
        std::cout << "Serial card '0x" << std::hex << start_adr << "' available on '" << serial.name() << "'." << std::endl;
    }

    inline u8 read(u16 adr) override {
        if ((adr & 1) == 0)
            return 0x00;
        return registers[adr - start_adr];
    }

    inline void write(u16 adr, u8 byte) override {
        if ((adr & 1) == 1)
            return;
        registers[adr - start_adr] = byte;
    }
};

#endif