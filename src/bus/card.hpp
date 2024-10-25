#ifndef CARD_HPP_
#define CARD_HPP_

#include <array>
#include <cstring>

#include "typedef.hpp"
#include "pty.hpp"
#include "util.hpp"

/**
 * @brief Holds information that can be used to identify a card.
 *
 * This struct groups the start address, address range, a name C-string and details C-string of a card.
 * Its main application is to have an instance of it returned by the `identify()` method of a card.
 */
struct card_identify {
    u16 start_adr;
    u16 adr_range;
    const char* name;
    const char* detail;

    card_identify() : start_adr(0xFF), adr_range(0), name("unknown"), detail("") {};
    card_identify(u16 start_adr, u16 adr_range, const char* name) : start_adr(start_adr), adr_range(adr_range), name(name), detail("") {};
    card_identify(u16 start_adr, u16 adr_range, const char* name, const char* detail) : start_adr(start_adr), adr_range(adr_range), name(name), detail(detail) {};
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
    /// @name Bit utilities.
    /// \{

    /// @brief Set a bit in a byte on the bus.
    /// @warning This method does not check if the address is in the card's range.
    inline void set_bit(u16 adr, u8 bitmask) { write(adr, read(adr) | bitmask); }

    /// @brief Unset a bit in a byte on the bus.
    /// @warning This method does not check if the address is in the card's range.
    inline void unset_bit(u16 adr, u8 bitmask) { write(adr, read(adr) & ~bitmask); }

    /// @brief Un/set a bit in a byte on the bus by condition.
    /// @warning This method does not check if the address is in the card's range.
    inline void set_if_bit(u16 adr, u8 bitmask, bool value) { value ? set_bit(adr, bitmask) : unset_bit(adr, bitmask); }

    /// \}
    /// @name Commonly used methods.
    /// \{

    /// @brief Check if the card is write-locked.
    inline bool is_w_locked() const { return write_locked; }

    /// @brief Lock the card for writing.
    inline void w_lock() { write_locked = true; }

    /// @brief Unlock the card for writing.
    inline void w_unlock() { write_locked = false; }

    /// @brief Check if the card has an IRQ raised.
    inline bool is_irq() const { return irq_raised; }

    /// @brief Raise or clear the IRQ trigger.
    inline void raise_irq(bool value) { irq_raised = value; }

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
    virtual u8 read(u16 adr) const = 0;

    /**
     * @brief Write a byte to the card.
     * @param adr The address to write to.
     * @param byte The byte to write.
     */
    virtual void write(u16 adr, u8 byte) = 0;

    /// @brief Refresh the card to allow periodic I/O, timer or sync operation.
    /// @see bus::refresh()
    virtual void refresh() = 0;

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
 * @tparam start_adr The starting address of the card.
 * @tparam capacity The size in bytes of the card starting from the start address.
 * @tparam construct_then_write_lock Whether the card should be write-locked after construction.
 *
 * This class is a template that can be used to create cards that hold a fixed amount of data. It allows copying data to the
 * card immediately after construction, and can be write-locked after construction if needed. Write locking can also be toggled
 * at any time.
 *
 * @note For convenience, use the aliases `ram_card` and `rom_card` instead of this class.
 * @warning Out of range addresses are not checked, they should be checked by the bus instead, to avoid calling in_range() twice.
 * @see ram_card, rom_card
 */
template <u16 start_adr, u16 capacity, bool construct_then_write_lock>
class data_card : public card {
private:
    std::array<u8, capacity> data;

public:
    data_card(bool lock = construct_then_write_lock) { this->write_locked = lock; }

    /// @brief Construct a card and copy data from an iterator pair immediately.
    template <typename iter_t>
    data_card(iter_t begin, iter_t end, bool lock = construct_then_write_lock) {
        std::copy(begin, end, data.begin());
        this->write_locked = lock;
    }

    /// @brief Check if an address on the bus is in the card's range.
    inline bool in_range(u16 adr) const override { return adr >= start_adr and adr <= (start_adr + capacity); }

    /// @brief Get information about the data card.
    inline card_identify identify() override { return { start_adr, capacity, (this->write_locked ? "rom area" : "ram area") }; }

    /// @brief Read a byte from the data card.
    inline u8 read(u16 adr) const override {
        return data[adr - start_adr];
    }

    inline void write(u16 adr, u8 byte) override {
        if (!this->write_locked)
            data[adr - start_adr] = byte;
    }

    inline void clear() override {
        if (!this->write_locked)
            data.fill(0x00);
    }

    /// @note Unused methods.
    /// \{

    inline void refresh() override {}
    inline std::array<u8, 3> get_irq() override { return { BAD_U8, BAD_U8, BAD_U8 }; }

    /// \}
};

/// @brief A card that holds random access memory.
template <u16 start_adr, u16 capacity>
using ram_card = data_card<start_adr, capacity, false>;

/// @brief A card that holds read-only memory.
template <u16 start_adr, u16 capacity>
using rom_card = data_card<start_adr, capacity, true>;

/// @brief Enum of the main registers of the MC6850 ACIA (UART).
enum class serial_register {
    TX_DATA, RX_DATA, CONTROL, STATUS
};

/// @brief Enum of bitmasks of the status register bits of the MC6850 ACIA (UART).
enum class serial_status_flags {
    RDRF = 0x01, TDRE = 0x02, DCD = 0x04, CTS = 0x08, FE = 0x10, OVRN = 0x20, PE = 0x40, IRQ = 0x80
};

/// @brief The number of I/O addresses of the MC6850 ACIA (UART).
constexpr static u16 SERIAL_IO_ADDRESSES = 4;

/// @brief The base clock speed of the MC6850 ACIA (UART).
constexpr static usize SERIAL_BASE_CLOCK = 19200;

/**
 * @brief A card that emulates an MC6850 ACIA (UART), close to a MITS 88 SIOB card.
 * @tparam start_adr The starting address of the card.
 * @tparam base_clock The base clock speed of the UART (it can be further divided), default is SERIAL_BASE_CLOCK.
 *
 * This card handles interaction with a pseudo-terminal connected to the card UART. Emulation follows the Motorola 6850 ACIA
 * (Asynchronous Communications Interface Adapter) specifications, but quite simplified. The card has 4 I/O addresses that
 * correspond to the TX_DATA (write-only), RX_DATA (read-only), CONTROL (write-only) and STATUS (read-only) registers of the
 * UART. The card is also able to trigger IRQ according to different conditions.
 *
 * @note The `refresh()` method is fundamental here, as it will poll the pseudo-terminal for new data and send data to it regularly.
 * Not periodically refreshing the bus, and by consequence the card, will make the card unable to send or receive data.
 * @warning Out of range addresses are not checked, they should be checked by the bus instead, to avoid calling in_range() twice.
 */
template <u16 start_adr, usize base_clock = SERIAL_BASE_CLOCK>
class serial_card : public card {
private:
    constexpr static usize MAX_SERIAL_DETAIL_LENGTH = 64;

    pty serial;
    std::array<u8, 4> registers;
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
        serial.set_baud_rate(base_clock >> 4);
        CONTROL(0b10010101);
        TDRE(true);
        RTS(true);
    }

public:
    serial_card() { serial.open(); reset(); }

    /// @brief Check if an address on the bus is in the card's range.
    inline bool in_range(u16 adr) const override { return adr >= start_adr and adr <= (start_adr + SERIAL_IO_ADDRESSES); }

    /// @brief Get information about the serial card.
    /// @note The detail contains the base clock, control register (hex) and the pseudo-terminal name.
    inline card_identify identify() override {
        std::snprintf(
            detail, sizeof(detail), 
            "base: %lu, ctrl: %s, pty: '%s'", 
            base_clock, util::to_hex_s(static_cast<usize>(CONTROL()), 2).c_str(), serial.name()
        );

        return { start_adr, SERIAL_IO_ADDRESSES, "serial uart", detail };
    }

    /// @brief Refresh the UART for data I/O.
    inline void refresh() override {
        if (!RDRF() and serial.poll()) {
            RX_DATA(serial.getch());
            RDRF(true);
        }

        if (!TDRE()) {
            serial.putch(TX_DATA());
            TDRE(true);
        }
    }

    /// @brief Read a byte from the serial registers.
    /// @returns The byte read from the serial registers, or BAD_U8 if the address is invalid.
    inline u8 read(u16 adr) const override {
        switch (adr) {
            case 0x00: return BAD_U8; /// TX_DATA is write-only
            case 0x01: return RX_DATA();
            case 0x02: return BAD_U8; /// CONTROL is write-only
            case 0x03: return STATUS();
        }

        return BAD_U8;
    }

    /// @brief Write a byte to the serial registers.
    /// @note This method will also handle the UART configuration by writing to the CONTROL register.
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
                /// Transmit Control bits (TODO: missing interrupt controls)
                switch (byte & 0b01100000) {
                    //     .CC.....
                    case 0b00000000: RTS(true); break;
                    case 0b00100000: RTS(true); break;
                    case 0b01000000: RTS(false); break;
                    case 0b01100000: RTS(true); serial.send_break(); break;
                }
                /// Interrupt Enable bit (TODO: probably wrong behavior)
                switch (byte & 0b10000000) {
                    //     I.......
                    case 0b00000000: IRQ(false); break;
                    case 0b10000000: IRQ(true); break;
                }

                CONTROL(byte);
                break;

            case 0x03: break; /// STATUS is read-only
        }
    }

    /// @brief Clear the serial card state and configuration.
    inline void clear() override { reset(); }

    /// @note Unused methods.
    /// \{

    inline std::array<u8, 3> get_irq() override { return { BAD_U8, BAD_U8, BAD_U8 }; }

    /// \}
};

#endif