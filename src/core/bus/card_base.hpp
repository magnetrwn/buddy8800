#ifndef BUDDY8800_SRC_CORE_BUS_CARD_BASE_HPP_
#define BUDDY8800_SRC_CORE_BUS_CARD_BASE_HPP_

#include <array>
#include <string>
#include <utility>
#include "util/typedef.hpp"

/**
 * @brief Holds information that can be used to identify a card.
 *
 * This struct groups the start address, address range, a name string and details string of a card.
 * Its main application is to have an instance of it returned by the `identify()` method of a card.
 */
struct card_identify {
    u16 start_adr;
    usize adr_range;
    std::string name;
    std::string detail;

    card_identify() : start_adr(0xFF), adr_range(0), name("unknown"), detail("") {};
    card_identify(u16 start_adr, usize adr_range, std::string name)
        : start_adr(start_adr), adr_range(adr_range), name(std::move(name)), detail("") {};

    card_identify(u16 start_adr, usize adr_range, std::string name, std::string detail)
        : start_adr(start_adr), adr_range(adr_range), name(std::move(name)), detail(std::move(detail)) {}
};

/**
 * @brief Base class for all cards.
 *
 * This class is the base class for all cards, it provides a set of methods that should be implemented by all
 * cards that want to interface with the bus class. It also provides some prepared methods that can be used by
 * the bus or the card itself to handle some common operations.
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
     * @note This method should always be used when interacting with cards from the bus, to avoid out of range
     * accesses. It is not used by the cards themselves to avoid double checking.
     */
    virtual bool in_range(u16 adr) const = 0;

    /**
     * @brief Get information about the card.
     * @returns A card_identify struct with lengthy details about the card.
     */
    virtual card_identify identify() const = 0;

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

#endif
