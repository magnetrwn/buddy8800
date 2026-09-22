#ifndef BUDDY8800_SRC_CORE_CARDS_FRONT_PANEL_HPP_
#define BUDDY8800_SRC_CORE_CARDS_FRONT_PANEL_HPP_

#include "core/bus/card_base.hpp"
#include "util/format.hpp"
#include <stdexcept>

/**
 * @brief Fixed Altair front-panel sense switches, read through one I/O port.
 *
 * Bits 0..7 represent switches A8..A15 (one means up). Only the low eight
 * address bits are decoded. Writes and clear leave the startup setting intact;
 * runtime switch changes and the other front-panel controls are not modeled.
 */
class front_panel : public card {
    const u16 start_adr;
    const u8 switches;

public:
    /// @brief Configure the sense-switch port and its immutable initial byte.
    explicit front_panel(u16 start_adr = 0xFF, u8 switches = 0) : start_adr(start_adr), switches(switches) {
        if (start_adr > 0xFF)
            throw std::out_of_range("Front panel requires a port in 0..255");
    }

    /// @name Card interface
    /// \{

    /// @brief Match the configured port using the low eight address bits.
    bool in_range(u16 adr) const override { return (adr & 0xFF) == start_adr; }

    /// @brief Describe the port and switches without changing device state.
    card_identify identify() const override {
        return {start_adr, 1, "front panel",
                "upper adr: " + buddy8800::format::to_hex_s(static_cast<unsigned>(switches), 2)};
    }

    /// @brief Read the fixed sense-switch byte.
    u8 read(u16) override { return switches; }

    /// @brief Ignore guest output; sense switches are read-only.
    void write(u16, u8) override {}

    /// @brief Ignore forced writes as well as ordinary writes.
    void write_force(u16, u8) override {}

    /// @brief Identify the sense-switch register as an I/O device.
    bool is_io() const override { return true; }

    /// @brief Preserve the switch positions when the machine is cleared.
    void clear() override { raise_irq(false); }

    /// @brief Return the unused interrupt vector for this passive device.
    std::array<u8, 3> get_irq() override { return {BAD_U8, BAD_U8, BAD_U8}; }

    /// \}
};

#endif
