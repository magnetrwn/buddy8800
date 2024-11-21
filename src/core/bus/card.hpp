#ifndef CARD_HPP_
#define CARD_HPP_

#include <array>
#include <cstring>
#include <vector>

#include "typedef.hpp"
#include "pty.hpp"
#include "util.hpp"
#include "defines.hpp"

/**
 * @brief Holds information that can be used to identify a card.
 *
 * This struct groups the start address, address range, a name C-string and details C-string of a card.
 * Its main application is to have an instance of it returned by the `identify()` method of a card.
 */
struct card_identify {
    u16 start_adr;
    usize adr_range;
    const char* name;
    const char* detail;

    card_identify() : start_adr(0xFF), adr_range(0), name("unknown"), detail("") {};
    card_identify(u16 start_adr, usize adr_range, const char* name) : start_adr(start_adr), adr_range(adr_range), name(name), detail("") {};
    card_identify(u16 start_adr, usize adr_range, const char* name, const char* detail) : start_adr(start_adr), adr_range(adr_range), name(name), detail(detail) {};
};

/**
 * @brief Base class for all cards.
 *
 * This class is the base class for all cards, it provides a set of methods that should be implemented by all cards that
 * want to interface with the bus class. It also provides some prepared methods that can be used by the bus or the card
 * itself to handle some common operations.
 */
class card {
protected:
    bool write_locked = false;
    bool irq_raised = false;

public:
    /// @name Commonly used methods.
    /// \{

    /// @brief Check if the card is write-locked.
    bool is_w_locked() const { return write_locked; }

    /// @brief Lock the card for writing.
    void w_lock() { write_locked = true; }

    /// @brief Unlock the card for writing.
    void w_unlock() { write_locked = false; }

    /// @brief Check if the card has an IRQ raised.
    /// @warning Always use this before calling `get_irq()`.
    bool is_irq() const { return irq_raised; }

    /// @brief Raise or clear the IRQ trigger.
    void raise_irq(bool value) { irq_raised = value; }

    /// \}
    /// @name Abstract methods.
    /// \{

    /**
     * @brief Check if an address on the bus is in the card's range.
     * @param adr The address to check.
     * @returns True if the address is in the card's range, false otherwise.
     * @note This method should always be used when interacting with cards from the bus, to avoid out of range accesses.
     * It is not used by the cards themselves to avoid double checking.
     */ 
    virtual bool in_range(u16 adr) const = 0;

    /**
     * @brief Get information about the card.
     * @returns A card_identify struct with lengthy details about the card.
     */
    virtual card_identify identify() = 0;

    /**
     * @brief Read a byte from the card.
     * @param adr The address to read from.
     * @returns The byte read from the card.
     */
    virtual u8 read(u16 adr) = 0;

    /**
     * @brief Write a byte to the card.
     * @param adr The address to write to.
     * @param byte The byte to write.
     */
    virtual void write(u16 adr, u8 byte) = 0;

    /**
     * @brief Write a byte to the card regardless of write lock.
     * @param adr The address to write to.
     * @param byte The byte to write.
     */
    virtual void write_force(u16 adr, u8 byte) = 0;

    /// @brief Check if the card is an I/O card.
    /// @returns False on a memory card, true on an I/O card.
    virtual bool is_io() const = 0;

    /// @brief Get the IRQ instruction (and possible operands).
    /// @see bus::get_irq()
    virtual std::array<u8, 3> get_irq() = 0;

    /// @brief Clears the card data or configuration.
    virtual void clear() = 0;

    /// \}

    virtual ~card() = default;
};

/**
 * @brief A card that holds a fixed amount of data.
 * @param start_adr The starting address of the card.
 * @param capacity The size in bytes of the card starting from the start address.
 * @param construct_then_write_lock Whether the card should be write-locked after construction.
 *
 * This class can be used to create cards that hold a fixed amount of data. It allows copying data to the
 * card immediately after construction, and can be write-locked after construction if needed. Write locking 
 * can also be toggled at any time.
 *
 * When setting up the card by copying data to it, the capacity of it will be determined by the size of the
 * provided data. This might be a bit unrealistic in address range.
 *
 * @note For convenience, use the aliases `ram_card` and `rom_card` instead of this class.
 * @warning Out of range addresses are not checked, they should be checked by the bus instead, to avoid 
 * calling in_range() twice.
 * @see ram_card, rom_card
 */
template <bool construct_then_write_lock>
class data_card : public card {
private:
    const u16 start_adr;
    const usize capacity;
    std::vector<u8> data;

    /*static constexpr usize next_pow2_to_v(usize v) {
        usize pw = 1;
        while (pw < v)
            pw <<= 1;
        return pw;
    }*/

public:
    /**
     * @brief Construct a card with a fixed capacity and simple one byte fill.
     * @param start_adr The starting address of the card.
     * @param capacity The size in bytes of the card starting from the start address.
     * @param fill The byte to fill the card with, default is BAD_U8.
     * @param lock Whether the card should be write-locked after construction.
     */
    data_card(u16 start_adr, usize capacity, u8 fill = BAD_U8, bool lock = construct_then_write_lock) 
        : start_adr(start_adr), capacity(capacity) { 

        data.resize(capacity, fill);
        this->write_locked = lock;
    }
    
    /**
     * @brief Construct a card and copy data from an iterator pair immediately.
     * @param start_adr The starting address of the card.
     * @param begin The iterator to the beginning of the data. It must be a container of u8 type.
     * @param end The iterator to the end of the data. It must be a container of u8 type.
     * @param capacity The size in bytes of the card starting from the start address. Zero (or default) to autodetect from container size.
     * @param lock Whether the card should be write-locked after construction.
     */
    template <typename T, T_ITERATOR_SFINAE>
    data_card(u16 start_adr, T begin, T end, usize capacity = 0, bool lock = construct_then_write_lock) 
        : start_adr(start_adr), 
          capacity(
            (capacity == 0)
                ? static_cast<usize>(std::distance(begin, end))
                : capacity
            ) {

        static_assert(std::is_same_v<typename std::iterator_traits<T>::value_type, u8>, "Iterator value type must be u8.");

        if (static_cast<usize>(std::distance(begin, end)) > this->capacity)
            throw std::out_of_range("Binary data exceeds card capacity.");

        data.resize(this->capacity, BAD_U8);
        std::copy(begin, end, data.begin());
        this->write_locked = lock;
    }

    /// @brief Check if an address on the bus is in the card's range.
    bool in_range(u16 adr) const override { return adr >= start_adr and adr < (start_adr + capacity); }

    /// @brief Get information about the data card.
    card_identify identify() override { return { start_adr, capacity, (this->write_locked ? "rom area" : "ram area") }; }

    /// @brief Read a byte from the data card.
    u8 read(u16 adr) override { return data[adr - start_adr]; }

    /// @brief Write a byte to the data card.
    void write(u16 adr, u8 byte) override {
        if (!this->write_locked)
            data[adr - start_adr] = byte;
    }

    /// @brief Write a byte to the data card regardless of write lock.
    void write_force(u16 adr, u8 byte) override { data[adr - start_adr] = byte; }

    /// @brief Check if the card is an I/O card.
    bool is_io() const override { return false; }

    /// @brief Clear the data card.
    void clear() override {
        if (!this->write_locked)
            data.clear();
    }

    /// @name Unused methods.
    /// \{

    std::array<u8, 3> get_irq() override { return { BAD_U8, BAD_U8, BAD_U8 }; }

    /// \}
};

/// @brief A card that holds random access memory.
using ram_card = data_card<false>;

/// @brief A card that holds read-only memory.
using rom_card = data_card<true>;

/// @brief Enum of the main registers of the MC6850 ACIA (UART).
enum class serial_register {
    TX_DATA, RX_DATA, CONTROL, STATUS
};

/// @brief Enum of bitmasks of the status register bits of the MC6850 ACIA (UART).
enum class serial_status_flags {
    RDRF = 0x01, TDRE = 0x02, DCD = 0x04, CTS = 0x08, FE = 0x10, OVRN = 0x20, PE = 0x40, IRQ = 0x80
};

/// @brief The number of I/O addresses of the MC6850 ACIA (UART).
constexpr static u16 SERIAL_IO_ADDRESSES = 2;

/// @brief The base clock speed of the MC6850 ACIA (UART).
constexpr static usize SERIAL_BASE_CLOCK = 19200;

/**
 * @brief A card that emulates a 6850 ACIA.
 * @param start_adr The starting address of the card.
 * @param base_clock The base clock speed of the UART (it can be further divided), default is SERIAL_BASE_CLOCK.
 *
 * This card handles interaction with a pseudo-terminal connected to the card UART. Emulation follows the Motorola 6850 ACIA
 * (Asynchronous Communications Interface Adapter) specifications, but quite simplified. The card has 4 I/O addresses that
 * correspond to the TX_DATA (write-only), RX_DATA (read-only), CONTROL (write-only) and STATUS (read-only) registers of the
 * UART. The card is also able to trigger IRQ according to different conditions.
 *
 * @note The state of I/O devices is updated upon each read or write operation, instead of running a refresh cycle as previously done.
 * @par
 * @note To mimic the partial address decode behavior, while the IN and OUT instructions of the 8080 duplicate the argument byte on
 * the address bus, the decoder only looks at the lower 8 bits, effectively creating 255 mirrors of the card in the address space.
 * This is expected behavior.
 * @warning Out of range addresses are not checked, they should be checked by the bus instead, to avoid calling in_range() twice.
 */
class serial_card : public card {
private:
    constexpr static usize MAX_SERIAL_DETAIL_LENGTH = 64;

    const u16 start_adr;
    const usize base_clock;

    pty serial;
    std::array<u8, 4> registers;
    usize divide_by;
    char detail[MAX_SERIAL_DETAIL_LENGTH];
    bool rts;

    constexpr u8 TX_DATA() const { return registers[static_cast<usize>(serial_register::TX_DATA)]; }
    constexpr u8 RX_DATA() const { return registers[static_cast<usize>(serial_register::RX_DATA)]; }
    constexpr u8 CONTROL() const { return registers[static_cast<usize>(serial_register::CONTROL)]; }
    constexpr u8 STATUS() const { return registers[static_cast<usize>(serial_register::STATUS)]; }
    
    constexpr void TX_DATA(u8 value) { registers[static_cast<usize>(serial_register::TX_DATA)] = value; }
    constexpr void RX_DATA(u8 value) { registers[static_cast<usize>(serial_register::RX_DATA)] = value; }
    constexpr void CONTROL(u8 value) { registers[static_cast<usize>(serial_register::CONTROL)] = value; }
    constexpr void STATUS(u8 value) { registers[static_cast<usize>(serial_register::STATUS)] = value; }

    constexpr bool RDRF() const { return STATUS() & static_cast<u8>(serial_status_flags::RDRF); }
    constexpr bool TDRE() const { return STATUS() & static_cast<u8>(serial_status_flags::TDRE); }
    constexpr bool DCD() const { return STATUS() & static_cast<u8>(serial_status_flags::DCD); }
    constexpr bool CTS() const { return STATUS() & static_cast<u8>(serial_status_flags::CTS); }
    constexpr bool FE() const { return STATUS() & static_cast<u8>(serial_status_flags::FE); }
    constexpr bool OVRN() const { return STATUS() & static_cast<u8>(serial_status_flags::OVRN); }
    constexpr bool PE() const { return STATUS() & static_cast<u8>(serial_status_flags::PE); }
    constexpr bool IRQ() const { return STATUS() & static_cast<u8>(serial_status_flags::IRQ); }

    constexpr void RDRF(bool value) { value ? STATUS(STATUS() | static_cast<u8>(serial_status_flags::RDRF)) : STATUS(STATUS() & ~static_cast<u8>(serial_status_flags::RDRF)); }
    constexpr void TDRE(bool value) { value ? STATUS(STATUS() | static_cast<u8>(serial_status_flags::TDRE)) : STATUS(STATUS() & ~static_cast<u8>(serial_status_flags::TDRE)); }
    constexpr void DCD(bool value) { value ? STATUS(STATUS() | static_cast<u8>(serial_status_flags::DCD)) : STATUS(STATUS() & ~static_cast<u8>(serial_status_flags::DCD)); }
    constexpr void CTS(bool value) { value ? STATUS(STATUS() | static_cast<u8>(serial_status_flags::CTS)) : STATUS(STATUS() & ~static_cast<u8>(serial_status_flags::CTS)); }
    constexpr void FE(bool value) { value ? STATUS(STATUS() | static_cast<u8>(serial_status_flags::FE)) : STATUS(STATUS() & ~static_cast<u8>(serial_status_flags::FE)); }
    constexpr void OVRN(bool value) { value ? STATUS(STATUS() | static_cast<u8>(serial_status_flags::OVRN)) : STATUS(STATUS() & ~static_cast<u8>(serial_status_flags::OVRN)); }
    constexpr void PE(bool value) { value ? STATUS(STATUS() | static_cast<u8>(serial_status_flags::PE)) : STATUS(STATUS() & ~static_cast<u8>(serial_status_flags::PE)); }
    constexpr void IRQ(bool value) { value ? STATUS(STATUS() | static_cast<u8>(serial_status_flags::IRQ)) : STATUS(STATUS() & ~static_cast<u8>(serial_status_flags::IRQ)); }

    constexpr bool RTS() const { return rts; }
    constexpr void RTS(bool value) { rts = value; }

    void reset() {
        registers.fill(0x00);
        divide_by = 4;
        serial.set_baud_rate(base_clock >> divide_by);
        CONTROL(0b10010101);
        TDRE(true);
        RTS(true);
    }

public:
    serial_card(u16 start_adr, usize base_clock = SERIAL_BASE_CLOCK) 
        : start_adr(start_adr), base_clock(base_clock) { serial.open(); reset(); }

    /// @brief Check if an address on the bus is in the card's range.
    bool in_range(u16 adr) const override { return (adr & 0xFF) >= start_adr and (adr & 0xFF) < (start_adr + SERIAL_IO_ADDRESSES); }

    /// @brief Get information about the serial card.
    /// @note The detail contains the base clock, control register (hex) and the pseudo-terminal name.
    card_identify identify() override {
        std::snprintf(
            detail, sizeof(detail), 
            "baud: %lu, ctrl: %s, pty: '%s'", 
            base_clock >> divide_by, util::to_hex_s(static_cast<usize>(CONTROL()), 2).c_str(), serial.name()
        );

        return { start_adr, SERIAL_IO_ADDRESSES, "serial uart", detail };
    }

    /// @brief Read a byte from the serial registers.
    /// @returns The byte read from the serial registers, or BAD_U8 if the address is invalid (which should be prevented by `in_range()`).
    u8 read(u16 adr) override {
        if (!RDRF() and serial.poll()) {
            RX_DATA(serial.getch());
            RDRF(true);
        }

        if ((adr & 0xFF) == start_adr)
            return STATUS();
        else if ((adr & 0xFF) == start_adr + 1)
            return RX_DATA();

        return BAD_U8;
    }

    /// @brief Write a byte to the serial registers.
    /// @note This method will also handle the UART configuration by writing to the CONTROL register.
    void write(u16 adr, u8 byte) override {
        if ((adr & 0xFF) == start_adr) {
            // Counter Divide select bits
            switch (byte & 0b00000011) {
                //     ......DD
                case 0b00000000: divide_by = 1; serial.set_baud_rate(base_clock >> divide_by); break;
                case 0b00000001: divide_by = 4; serial.set_baud_rate(base_clock >> divide_by); break;
                case 0b00000010: divide_by = 6; serial.set_baud_rate(base_clock >> divide_by); break;
                case 0b00000011: reset(); break;
            }
            // Word Select bits
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
            // Transmit Control bits (TODO: missing interrupt controls)
            switch (byte & 0b01100000) {
                //     .CC.....
                case 0b00000000: RTS(true); break;
                case 0b00100000: RTS(true); break;
                case 0b01000000: RTS(false); break;
                case 0b01100000: RTS(true); serial.send_break(); break;
            }
            // Interrupt Enable bit (TODO: probably wrong behavior)
            switch (byte & 0b10000000) {
                //     I.......
                case 0b00000000: IRQ(false); break;
                case 0b10000000: IRQ(true); break;
            }

            CONTROL(byte);
        }

        else if ((adr & 0xFF) == start_adr + 1) {
            TX_DATA(byte); 
            TDRE(false);
        }

        if (!TDRE()) {
            serial.putch(TX_DATA());
            TDRE(true);
        }
    }

    /// @brief Check if the card is an I/O card.
    bool is_io() const override { return true; }

    /// @brief Clear the serial card state and configuration.
    void clear() override { reset(); }

    /// @name Unused methods.
    /// \{

    void write_force(u16 adr, u8 byte) override { write(adr, byte); }
    std::array<u8, 3> get_irq() override { return { BAD_U8, BAD_U8, BAD_U8 }; }

    /// \}
};

#endif