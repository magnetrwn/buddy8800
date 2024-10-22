#ifndef BUS_HPP_
#define BUS_HPP_

#include <array>
#include <stdexcept>

#include "typedef.hpp"
#include "card.hpp"
#include "util.hpp"

class bus {
private:
    static constexpr usize MAX_BUS_CARDS = 10;
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
    inline void insert(card* card, usize slot) {
        if (!card)
            throw std::invalid_argument("cannot insert nullptr");
        if (slot >= MAX_BUS_CARDS)
            throw std::out_of_range("slot out of range");
        if (cards[slot] != NO_CARD)
            throw std::invalid_argument("slot already occupied");

        cards[slot] = card;

        if (test_for_bus_conflict())
            throw std::invalid_argument("bus conflict detected");
    }

    inline void remove(usize slot) {
        if (slot >= MAX_BUS_CARDS)
            throw std::out_of_range("slot out of range");

        cards[slot] = NO_CARD;
    }

    inline u8 read(u16 adr) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr))
                return card->read(adr);

        return BAD_U8;
    }

    inline void write(u16 adr, u8 byte) {
        for (card* card : cards)
            if (card != NO_CARD and card->in_range(adr))
                card->write(adr, byte);
    }

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
                std::cout 
                    << i << ": " 
                    << util::to_hex_s(start_adr) << "-" << util::to_hex_s(end_adr) << ": " 
                    << cards[i]->identify().name << ", " << cards[i]->identify().detail 
                    << std::endl;
            }
    }

    bus() : cards({ NO_CARD }) {}
};


#endif