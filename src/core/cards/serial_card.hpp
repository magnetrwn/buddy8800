#ifndef BUDDY8800_SRC_CORE_CARDS_SERIAL_CARD_HPP_
#define BUDDY8800_SRC_CORE_CARDS_SERIAL_CARD_HPP_

#include "core/bus/card_base.hpp"
#include "core/iface/unix_pty.hpp"

inline constexpr u16 SERIAL_IO_ADDRESSES = 2;
inline constexpr usize SERIAL_BASE_CLOCK = 19200;

/**
 * @brief Polled 88-2SIO subset using the 6850 ACIA register layout.
 *
 * Base reads status / writes control; base+1 reads RX / writes TX. Only low
 * address bits are decoded. Timing, framing and IRQs are not emulated.
 * Transport is serviced on guest reads/writes, never while describing the card.
 */
class serial_card : public card {
    const u16 start_adr;
    const usize base_clock;
    pty serial;
    u8 control = 0;
    u8 received = 0;
    u8 transmitted = 0;
    bool receive_full = false;
    bool transmit_full = false;

    void refresh();

public:
    /// @brief Open a PTY-backed serial device at the specified base address.
    serial_card(u16 start_adr, usize base_clock = SERIAL_BASE_CLOCK);

    /// @brief Return the host PTY slave device name.
    const char* pty_name() const { return serial.name(); }

    /// @name Card interface
    /// \{

    /// @brief Check whether an address selects either serial register.
    bool in_range(u16 adr) const override;

    /// @brief Describe this device without servicing serial traffic.
    card_identify identify() const override;

    /// @brief Read status or received data.
    u8 read(u16 adr) override;

    /// @brief Write control or transmitted data.
    void write(u16 adr, u8 byte) override;

    /// @brief Identify this card as an I/O-space device.
    bool is_io() const override { return true; }

    /// @brief Reset the emulated register and buffer state.
    void clear() override;

    /// @brief Forward a forced write to the ordinary register interface.
    void write_force(u16 adr, u8 byte) override { write(adr, byte); }

    /// @brief Return the unused interrupt vector for this polled device.
    std::array<u8, 3> get_irq() override { return {BAD_U8, BAD_U8, BAD_U8}; }

    /// \}
};

#endif
