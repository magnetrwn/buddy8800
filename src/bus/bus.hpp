#ifndef BUS_HPP_
#define BUS_HPP_

#include <array>
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
 * additional middle class, `bus_subscr_iface`, which acts as a proxy to address locations and call reads and writes by the
 * cast and assignment operators.
 *
 * @note The CPU, while in reality is placed on a card, it's not here. The bus acts as glue between the plentitude
 * of cards and the CPU itself.
 * @warning The `size()` method returns the maximum number of addressable locations on the bus (65536), not the number of cards.
 * While this might look misleading, it's actually an attempt at keeping the same interface as there would be if the instanced bus
 * were to be replaced by a normal `std::array<u8, 65536>`.
 */
class bus {
private:

    /**
     * @brief Proxy class to subscript (index) the bus.
     *
     * This class puts itself in-between each index to the bus. It allows for a more natural way to read and write to the bus,
     * by calling the `read()` and `write()` methods via cast and assignment operators. There are also increment and decrement
     * operators for convenience, which will read the value, increment or decrement it, and write it back to the bus. This could
     * have interesting effects on non-memory devices.
     */
    class bus_subscr_iface {
    private:
        bus& bus_ref;
        u16 adr;

    public:
        /// @brief Casts a subscripted location on the bus to a u8.
        inline operator u8() const { return bus_ref.read(adr); }
        
        /// @brief Prefix operator, increments subscripted bus location.
        inline u8 operator++() {
            u8 value = bus_ref.read(adr);
            bus_ref.write(adr, value + 1);
            return value + 1;
        }

        /// @brief Prefix operator, decrements subscripted bus location.
        inline u8 operator--() {
            u8 value = bus_ref.read(adr);
            bus_ref.write(adr, value - 1);
            return value - 1;
        }

        /// @brief Postfix operator, increments subscripted bus location.
        inline u8 operator++(int) {
            u8 value = bus_ref.read(adr);
            bus_ref.write(adr, value + 1);
            return value;
        }

        /// @brief Postfix operator, decrements subscripted bus location.
        inline u8 operator--(int) {
            u8 value = bus_ref.read(adr);
            bus_ref.write(adr, value - 1);
            return value;
        }

        /// @brief Assignment operator, writes a byte to the subscripted bus location.
        inline bus_subscr_iface& operator=(u8 byte) { bus_ref.write(adr, byte); return *this; }
        
        /// @brief Assignment operator, writes a byte read from another subscripted bus location.
        /// @note This method prevents self-assignment.
        inline bus_subscr_iface& operator=(const bus_subscr_iface& other) {
            if (this != &other)
                bus_ref.write(adr, other.bus_ref.read(other.adr));

            return *this;
        }

        bus_subscr_iface(bus& b, u16 adr) : bus_ref(b), adr(adr) {}
    };

    static constexpr usize MAX_BUS_CARDS = 18;
    static constexpr card* NO_CARD = nullptr;

    std::array<card*, MAX_BUS_CARDS> cards;
    std::array<bool, MAX_BUS_CARDS> ignore_conflicts;

    inline bool test_for_bus_conflict() const {
        for (usize i = 0; i < MAX_BUS_CARDS; ++i) {
            if (ignore_conflicts[i])
                continue;

            for (usize j = i + 1; j < MAX_BUS_CARDS; ++j)
                if (cards[i] != NO_CARD 
                and cards[j] != NO_CARD
                and (
                    cards[i]->in_range(cards[j]->identify().start_adr) 
                    or cards[j]->in_range(cards[i]->identify().start_adr)
                ))
                    return true;
        }

        return false;
    }

public:
    /**
     * @brief Inserts a card into a slot on the bus.
     * @param card A pointer to the card to insert.
     * @param slot The slot number to insert the card into.
     * @param allow_conflict Whether to allow bus conflicts (run a conflict check) or not.
     * @throws std::invalid_argument if the card is nullptr.
     * @throws std::out_of_range if the slot is out of range.
     * @throws std::invalid_argument if the slot is already occupied.
     * @throws std::invalid_argument if a bus conflict is detected and allow_conflict is false.
     */
    inline void insert(card* card, usize slot, bool allow_conflict = false) {
        if (!card)
            throw std::invalid_argument("cannot insert nullptr");
        if (slot >= MAX_BUS_CARDS)
            throw std::out_of_range("slot out of range");
        if (cards[slot] != NO_CARD)
            throw std::invalid_argument("slot already occupied");

        cards[slot] = card;
        ignore_conflicts[slot] = allow_conflict;

        if (!allow_conflict and test_for_bus_conflict())
            throw std::invalid_argument("bus conflict detected");
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
     * @return The byte read from the first valid card on the bus.
     * @warning This method will return only the first valid card slot that is in range of the address.
     * @todo Handle this warning.
     */
    inline u8 read(u16 adr) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr))
                return card->read(adr);

        return BAD_U8;
    }

    /**
     * @brief Writes a byte to the bus.
     * @param adr The address to write to.
     * @param byte The byte to write.
     * @note This method will write to all cards in range of the address.
     */
    inline void write(u16 adr, u8 byte) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr))
                card->write(adr, byte);
    }

    /**
     * @brief Subscripts the bus.
     * @param adr The address to subscript.
     * @return A proxy object to subscript the bus.
     */
    inline bus_subscr_iface operator[](u16 adr) { return bus_subscr_iface(*this, adr); }

    /**
     * @brief Refreshes all cards on the bus.
     *
     * This method will call each card's possibly distinct way to refresh, which is important for example to
     * let a serial card be able to poll or send new data.
     */
    inline void refresh() {
        for (card* card : cards)
            if (card != NO_CARD)
                card->refresh();
    }

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
     * @brief Gets the IRQ instruction(s).
     * @return A few instructions according to the type of interrupt the device uses to send.
     * @throws std::runtime_error if no IRQ is raised.
     *
     * When the 8080 accepts an interrupt request, it will look for an instruction on the data bus to run. Most commonly
     * this is either a `RST` or a `CALL` instruction. While `RST` has a bunch of fixed offsets, `CALL` instead makes the
     * 8080 run an extra two fetches for a 16-bit address to jump to. Because of this, an array of 3 bytes has to be returned
     * in case of the latter, otherwise they would be just 0x00.
     */
    inline std::array<u8, 3> get_irq() {
        for (card* card : cards)
            if (card != NO_CARD and card->is_irq())
                return card->get_irq();

        throw std::runtime_error("tried get_irq() while none was raised");
    }

    /**
     * @brief Prints a detailed memory map of the bus.
     *
     * This method will print the memory map of the bus, showing the start and end addresses of each card, along with
     * the card's type (name) and additional details.
     *
     * The output is formatted as follows:
     * ```
     * slot: start-address/address-range: card-type, card-details
     * ```
     */
    inline void print_mmap() {
        for (usize i = 0; i < MAX_BUS_CARDS; ++i)
            if (cards[i] != NO_CARD) {
                u16 start_adr = cards[i]->identify().start_adr;
                u16 end_adr = start_adr + cards[i]->identify().adr_range;
                const char* detail = cards[i]->identify().detail;
                std::cout 
                    << i << ": " 
                    << util::to_hex_s(start_adr) << "/" << util::to_hex_s(end_adr) << ": " 
                    << cards[i]->identify().name << (*detail ? ", " : "") << (*detail ? detail : "")
                    << std::endl;
            }
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