#ifndef BUS_HPP_
#define BUS_HPP_

#include <array>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "typedef.hpp"
#include "card.hpp"
#include "util.hpp"

/**
 * @brief Represents the S-100 bus of the emulator.
 *
 * This class holds the necessary structures and methods to handle inter-operation of the Intel 8080 CPU
 * with any assortment of cards interacting through a data, address, and control bus according to the S-100
 * specs, including interrupt vectors.
 *
 * A design choice was made to try to stay as close as possible as an indexable array of memory with the bus, so that it can
 * be easily replaced by a simple `std::array<u8, 65536>`. This is why the `read()` and `write()` methods are called by an
 * additional middle class, `bus_index_iface`, which acts as a proxy to address locations and call reads and writes by the
 * cast and assignment operators.
 *
 * @note Conflicting address ranges are allowed but must be explicitly toggled by an `insert()`. Make sure you actually want
 * a conflict, since cards have an IOR/IOW signal that can be used to determine if the cards belong to memory or I/O addressable
 * ranges, which can overlap but be separated by the IOR/IOW signal being set or not.
 * @par
 * @note The CPU, while in reality is placed on a card, it's not here. The bus acts as glue between the plentitude
 * of cards and the CPU itself.
 * @warning This class does not own the cards, and it's up to other parts of the emulator to manage the card memory, such as
 * `system_config`.
 * @par
 * @warning The `size()` method returns the maximum number of addressable locations on the bus (65536), not the number of cards.
 * While this might look misleading, it's actually an attempt at keeping the same interface as there would be if the instanced bus
 * were to be replaced by a normal `std::array<u8, 65536>`.
 */
class bus {
private:

    /**
     * @brief Proxy class to index the bus.
     *
     * This class puts itself in-between each index to the bus. It allows for a more natural way to read and write to the bus,
     * by calling the `read()` and `write()` methods via cast and assignment operators. There are also increment and decrement
     * operators for convenience, which will read the value, increment or decrement it, and write it back to the bus. This could
     * have interesting effects on non-memory devices.
     */
    class bus_index_iface {
    private:
        bus& bus_ref;
        u16 adr;

    public:
        /// @brief Casts a indexed location on the bus to a u8.
        inline operator u8() const { return bus_ref.read(adr); }
        
        /// @brief Prefix operator, increments indexed bus location.
        /// @note The double read in this method is due to possibly interfacing with MMIO that doesn't behave like a simple value.
        inline u8 operator++() {
            u8 value = bus_ref.read(adr);
            bus_ref.write(adr, value + 1);
            return bus_ref.read(adr);
        }

        /// @brief Prefix operator, decrements indexed bus location.
        /// @note The double read in this method is due to possibly interfacing with MMIO that doesn't behave like a simple value.
        inline u8 operator--() {
            u8 value = bus_ref.read(adr);
            bus_ref.write(adr, value - 1);
            return bus_ref.read(adr);
        }

        /// @brief Postfix operator, increments indexed bus location.
        inline u8 operator++(int) {
            u8 value = bus_ref.read(adr);
            bus_ref.write(adr, value + 1);
            return value;
        }

        /// @brief Postfix operator, decrements indexed bus location.
        inline u8 operator--(int) {
            u8 value = bus_ref.read(adr);
            bus_ref.write(adr, value - 1);
            return value;
        }

        /// @brief Assignment operator, writes a byte to the indexed bus location.
        inline bus_index_iface& operator=(u8 byte) { bus_ref.write(adr, byte); return *this; }
        
        /// @brief Assignment operator, writes a byte read from another indexed bus location.
        /// @note This method prevents self-assignment.
        inline bus_index_iface& operator=(const bus_index_iface& other) {
            if (this != &other)
                bus_ref.write(adr, other.bus_ref.read(other.adr));

            return *this;
        }

        bus_index_iface(bus& b, u16 adr) : bus_ref(b), adr(adr) {}
    };

    /// @todo MAX_BUS_CARDS might be a bit hard to find, and doesn't exactly serve a purpose other than hard limit.
    static constexpr usize MAX_BUS_CARDS = 18;
    static constexpr card* NO_CARD = nullptr;

    std::array<card*, MAX_BUS_CARDS> cards;
    std::array<bool, MAX_BUS_CARDS> ignore_conflicts;

    inline bool test_for_bus_conflict(card* card) const {
        for (usize i = 0; i < MAX_BUS_CARDS; ++i)
            if (!ignore_conflicts[i] and cards[i] != NO_CARD
            and cards[i]->is_io() == card->is_io()
            and (cards[i]->in_range(card->identify().start_adr)
                 or (card->in_range(cards[i]->identify().start_adr))
                )
            )
                return true;

        return false;
    }

public:
    /**
     * @brief Inserts a card into a slot on the bus.
     * @param card A pointer to the card to insert.
     * @param slot The slot number to insert the card into, counting from 0 to MAX_BUS_CARDS - 1.
     * @param allow_conflict Whether to allow bus conflicts (run a conflict check) or not.
     * @throws std::invalid_argument if the card is nullptr.
     * @throws std::out_of_range if the slot is out of range.
     * @throws std::invalid_argument if the slot is already occupied.
     * @throws std::invalid_argument if a bus conflict is detected and allow_conflict is false.
     * @warning This method allows a bus conflict, but in such event, only the earlier slot card will be able
     * to be read from!
     */
    inline void insert(card* card, usize slot, bool allow_conflict = false) {
        if (!card)
            throw std::invalid_argument("cannot insert nullptr");
        if (slot > MAX_BUS_CARDS)
            throw std::out_of_range("slot out of range");
        if (cards[slot] != NO_CARD)
            throw std::invalid_argument("slot already occupied");
        if (!allow_conflict and test_for_bus_conflict(card))
            throw std::invalid_argument("bus conflict detected");

        cards[slot] = card;
        ignore_conflicts[slot] = allow_conflict;
    }

    /**
     * @brief Removes a card from a slot on the bus.
     * @param slot The slot number to remove the card from.
     * @throws std::out_of_range if the slot is out of range.
     */
    inline void remove(usize slot) {
        if (slot >= MAX_BUS_CARDS)
            throw std::out_of_range("slot out of range");

        cards[slot] = NO_CARD;
        ignore_conflicts[slot] = false;
    }

    /**
     * @brief Returns the maximum number of addressable locations on the bus.
     * @return The maximum number of addressable locations on the bus.
     * @warning This method returns a fixed value of max addressable locations on the bus (65536), not the number of cards!
     */
    constexpr inline usize size() const { return 65536; }

    /**
     * @brief Reads a byte from the bus.
     * @param adr The address to read from.
     * @param ior Whether the IOR signal is set or not.
     * @return The byte read from the first valid card on the bus.
     * @warning This method will return only the first valid card slot that is in range of the address. Be mindful of allowed collision ranges.
     */
    inline u8 read(u16 adr, bool ior = false) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr) and ior == card->is_io())
                return card->read(adr);

        return BAD_U8;
    }

    /**
     * @brief Writes a byte to the bus.
     * @param adr The address to write to.
     * @param byte The byte to write.
     * @param iow Whether the IOW signal is set or not.
     * @note This method will write to all cards in range of the address. Be mindful of allowed collision ranges.
     */
    inline void write(u16 adr, u8 byte, bool iow = false) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr) and iow == card->is_io())
                card->write(adr, byte);
    }

    /**
     * @brief Writes a byte to the bus, without considering write lock.
     * @param adr The address to write to.
     * @param byte The byte to write.
     * @param iow Whether the IOW signal is set or not.
     * @note This method will write to all cards in range of the address.
     */
    inline void write_force(u16 adr, u8 byte, bool iow = false) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr) and iow == card->is_io())
                card->write_force(adr, byte);
    }

    /**
     * @brief Indexes the bus.
     * @param adr The address to index.
     * @return A proxy object to index the bus.
     */
    inline bus_index_iface operator[](u16 adr) { return bus_index_iface(*this, adr); }

    /**
     * @brief A shortcut to read the bus using an index directly without a proxy object.
     * @param adr The address to index.
     * @return The byte read from the bus.
     */
    //inline u8 operator[](u16 adr) const { return this->read(adr); }

    /**
     * @brief Checks if an IRQ is raised.
     * @return True if an IRQ is raised, false otherwise.
     * @note This method should be a condition for a loop at the end of an emulation cycle, so that all IRQs can be
     * handled if occasionally concurrent.
     */
    inline bool is_irq() const {
        for (card* card : cards)
            if (card != NO_CARD and card->is_irq())
                return true;

        return false;
    }

    /**
     * @brief Gets the IRQ instruction (and possible operands).
     * @return An instruction according to the type of interrupt the device uses to send.
     * @throws std::runtime_error if no IRQ is raised.
     *
     * When the 8080 accepts an interrupt request, it will look for an instruction on the data bus to run. Most commonly
     * this is either a `RST` or a `CALL` instruction. While `RST` has a bunch of fixed offsets, `CALL` instead makes the
     * 8080 run an extra two fetches for a 16-bit address to jump to. Because of this, an array of 3 bytes has to be returned
     * in case of the latter, otherwise they would be just 0x00.
     *
     * @note This method should be called after `is_irq()` returns true.
     * @par
     * @note The order of returned interrupts is preferential to the order of cards on the bus. This is because many S-100
     * systems used simple daisy-chaining for IRQs, and the closest slot to raise an IRQ concurrent to another would be the 
     * first to be serviced.
     * @todo Verify this last claim (we may need an 8259 PIC instead...).
     */
    inline std::array<u8, 3> get_irq() {
        for (card* card : cards)
            if (card != NO_CARD and card->is_irq())
                return card->get_irq();

        throw std::runtime_error("tried get_irq() while none was raised");
    }

    /**
     * @brief Returns a detailed map of the bus.
     * @return A std::string with details about the bus devices.
     *
     * This method will return a very long std::string containing the address map of the bus, showing the start 
     * and end addresses of each card, along with the card's type (name) and additional details.
     *
     * @note The output for each device is formatted as follows:
     * ```
     * slot: [is-i/o] start-address-hex/address-range: card-type, card-details
     * ```
     */
    inline std::string bus_map_s() const {
        static constexpr usize PAD_ADR_RANGE_SLEN = 12;

        std::stringstream ss;

        for (usize i = 0; i < MAX_BUS_CARDS; ++i)
            if (cards[i] != NO_CARD) {
                const card_identify ident = cards[i]->identify();

                std::string adr_range_verbose = (
                    util::to_hex_s(ident.start_adr, cards[i]->is_io() ? 2 : 4) + "/" + std::to_string(ident.adr_range)
                );

                if (adr_range_verbose.size() < PAD_ADR_RANGE_SLEN)
                    adr_range_verbose.resize(PAD_ADR_RANGE_SLEN, ' ');

                ss << "Slot " << std::setw(2) << i << ": " 
                   << (cards[i]->is_io() ? "\x1B[45;01mI/O" : "\x1B[47;01mMEM") << "\x1B[0m "
                   << adr_range_verbose << ": " 
                   << "\x1B[01m" << ident.name << "\x1B[0m" << (*ident.detail ? ", " : "") << (*ident.detail ? ident.detail : "")
                   << std::endl;
            }

        return ss.str();
    }

    /**
     * @brief Get in which slot a card is according to an address.
     * @returns The slot closest that accepts the address in its range, 255 if none.
     */
    inline u8 get_slot_by_adr(u16 adr) const {
        for (usize i = 0; i < MAX_BUS_CARDS; ++i)
            if (cards[i] != NO_CARD and cards[i]->in_range(adr))
                return i;

        return 255;
    }

    /**
     * @brief Clears all cards on the bus.
     *
     * This method will call each card's possibly distinct way to clear, which will depend on the type of
     * card, some will clear data, others might reset configuration settings.
     */
    inline void clear() {
        for (card* card : cards)
            if (card != NO_CARD)
                card->clear();
    }

    bus() : cards({ NO_CARD }), ignore_conflicts({ false }) {}
};

#endif