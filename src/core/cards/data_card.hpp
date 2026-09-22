#ifndef BUDDY8800_SRC_CORE_CARDS_DATA_CARD_HPP_
#define BUDDY8800_SRC_CORE_CARDS_DATA_CARD_HPP_

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "core/bus/card_base.hpp"
#include "util/iterator.hpp"

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

        if (capacity == 0 || capacity > 65536u - start_adr)
            throw std::out_of_range("Memory card exceeds address space or is empty");
        data.resize(capacity, fill);
        this->write_locked = lock;
    }

    /**
     * @brief Construct a card and copy data from an iterator pair immediately.
     * @param start_adr The starting address of the card.
     * @param begin The iterator to the beginning of the data. It must be a container of u8 type.
     * @param end The iterator to the end of the data. It must be a container of u8 type.
     * @param capacity The size in bytes of the card starting from the start address. Zero (or default) to
     * autodetect from container size.
     * @param lock Whether the card should be write-locked after construction.
     */
    template <typename value_type, typename = buddy8800::enable_input_iterator<value_type>>
    data_card(u16 start_adr, value_type begin, value_type end, usize capacity = 0,
              bool lock = construct_then_write_lock)
        : start_adr(start_adr),
          capacity((capacity == 0) ? static_cast<usize>(std::distance(begin, end)) : capacity) {

        static_assert(std::is_same_v<typename std::iterator_traits<value_type>::value_type, u8>,
                      "Iterator value type must be u8.");

        if (this->capacity == 0 || this->capacity > 65536u - start_adr)
            throw std::out_of_range("Memory card exceeds address space or is empty");
        if (static_cast<usize>(std::distance(begin, end)) > this->capacity)
            throw std::out_of_range("Binary data exceeds card capacity.");

        data.resize(this->capacity, BAD_U8);
        std::copy(begin, end, data.begin());
        this->write_locked = lock;
    }

    /// @brief Check if an address on the bus is in the card's range.
    bool in_range(u16 adr) const override { return adr >= start_adr and adr < (start_adr + capacity); }

    /// @brief Get information about the data card.
    card_identify identify() const override {
        return {start_adr, capacity, (this->write_locked ? "rom area" : "ram area")};
    }

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
            std::fill(data.begin(), data.end(), BAD_U8);
    }

    /// @name Unused methods.
    /// \{

    std::array<u8, 3> get_irq() override { return {BAD_U8, BAD_U8, BAD_U8}; }

    /// \}
};

#endif
