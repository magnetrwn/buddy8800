#ifndef BUS_HPP_
#define BUS_HPP_

#include <array>
#include <stdexcept>

#include "typedef.hpp"
#include "card.hpp"
#include "util.hpp"

class bus {
private:
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

    inline bool test_for_bus_conflict() const {
        for (usize i = 0; i < MAX_BUS_CARDS; ++i)
            for (usize j = i + 1; j < MAX_BUS_CARDS; ++j)
                if (cards[i] != NO_CARD 
                and cards[j] != NO_CARD 
                and (
                    cards[i]->in_range(cards[j]->identify().start_adr) 
                    or cards[j]->in_range(cards[i]->identify().start_adr)
                ))
                    return true;

        return false;
    }

public:
    inline void insert(card* card, usize slot, bool allow_conflict = false) {
        if (!card)
            throw std::invalid_argument("cannot insert nullptr");
        if (slot >= MAX_BUS_CARDS)
            throw std::out_of_range("slot out of range");
        if (cards[slot] != NO_CARD)
            throw std::invalid_argument("slot already occupied");

        cards[slot] = card;

        if (!allow_conflict and test_for_bus_conflict())
            throw std::invalid_argument("bus conflict detected");
    }

    inline void remove(usize slot) {
        if (slot >= MAX_BUS_CARDS)
            throw std::out_of_range("slot out of range");

        cards[slot] = NO_CARD;
    }

    inline usize size() const { return 65536; }

    // NOTE: stops at the first one in range,
    inline u8 read(u16 adr) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr))
                return card->read(adr);

        return BAD_U8;
    }

    // NOTE: writes to all of the ones in range, not just the first one!
    inline void write(u16 adr, u8 byte) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr))
                card->write(adr, byte);
    }

    inline bus_subscr_iface operator[](u16 adr) { return bus_subscr_iface(*this, adr); }

    inline void refresh() {
        for (card* card : cards)
            if (card != NO_CARD)
                card->refresh();
    }

    inline bool is_irq() const {
        for (card* card : cards)
            if (card != NO_CARD and card->is_irq())
                return true;

        return false;
    }

    inline std::array<u8, 3> get_irq() {
        for (card* card : cards)
            if (card != NO_CARD and card->is_irq())
                return card->get_irq();

        throw std::runtime_error("tried get_irq() while none was raised");
    }

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

    inline void clear() {
        for (card* card : cards)
            if (card != NO_CARD)
                card->clear();
    }

    bus() : cards({ NO_CARD }) {}
};

#endif