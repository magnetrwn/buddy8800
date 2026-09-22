#include "core/cards/serial_card.hpp"
#include "util/format.hpp"
#include <stdexcept>
#include <string>

void serial_card::refresh() {
    if (transmit_full && serial.try_putch(transmitted))
        transmit_full = false;
    if (!receive_full)
        receive_full = serial.try_getch(received);
}

serial_card::serial_card(u16 start_adr, usize base_clock) : start_adr(start_adr), base_clock(base_clock) {
    if (start_adr > 0xFE)
        throw std::out_of_range("Serial card requires two ports in 0..255");
    serial.open();
}

bool serial_card::in_range(u16 adr) const {
    return (adr & 0xFF) >= start_adr && (adr & 0xFF) < start_adr + SERIAL_IO_ADDRESSES;
}

card_identify serial_card::identify() const {
    const usize divisor = (control & 3) == 1 ? 16 : (control & 3) == 2 ? 64 : 1;
    // Guest clock selection does not change the raw host transport speed.
    const auto detail = "clock: " + std::to_string(base_clock) + " Hz /" + std::to_string(divisor) +
                        ", ctrl: " + buddy8800::format::to_hex_s(static_cast<unsigned>(control), 2) +
                        ", pty: '" + serial.name() + "'";
    return {start_adr, SERIAL_IO_ADDRESSES, "serial uart", detail};
}

u8 serial_card::read(u16 adr) {
    refresh();
    if ((adr & 0xFF) == start_adr)
        return (receive_full ? 0x01 : 0) | (transmit_full ? 0 : 0x02);
    if ((adr & 0xFF) == start_adr + 1) {
        const u8 byte = received;
        receive_full = false;
        return byte;
    }
    return BAD_U8;
}

void serial_card::write(u16 adr, u8 byte) {
    refresh();
    if ((adr & 0xFF) == start_adr) {
        if ((byte & 3) == 3) {
            clear();
            return;
        }
        control = byte;
    } else if ((adr & 0xFF) == start_adr + 1 && !transmit_full) {
        transmitted = byte;
        transmit_full = true;
        refresh();
    }
}

void serial_card::clear() {
    control = received = transmitted = 0;
    receive_full = transmit_full = false;
    raise_irq(false);
}
