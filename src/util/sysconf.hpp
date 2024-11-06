#ifndef SYSCONF_HPP_
#define SYSCONF_HPP_

#include <vector>
#include <string>
#include <stdexcept>
#include <toml.hpp>

#include "bus.hpp"
#include "card.hpp"
#include "typedef.hpp"

/**
 * @brief A class to manage objects that should be setup by reading a TOML configuration file.
 *
 * This class holds and owns a vector of pointers to card objects and a bus object. It will handle
 * de/allocating memory and the bus, while allowing to get references to both the bus and the cards.
 *
 * It also holds other emulator configuration details.
 *
 * @note For syntax information check the TOML configuration file comments.
 */
class system_config {
private:
    bus cardbus;
    std::vector<card*> cards;
    u16 start_pc;
    bool do_pseudo_bdos;

    inline card* create_card(const std::string& type, u16 at, usize range, const std::string& load) {
        card* cardptr = nullptr;
        std::ifstream load_file;
        std::vector<u8> load_file_vec;

        if (load.empty() and range == 0 and (type == "ram" or type == "rom"))
            throw std::runtime_error("Config has data card with no range or load. You need at least one of the two.");

        if (!load.empty()) {
            load_file = std::ifstream(load, std::ios::binary);

            if (!load_file)
                throw std::runtime_error("Could not open file: " + std::string(load));

            load_file_vec.assign(std::istreambuf_iterator<char>(load_file), {});

            if (load_file_vec.empty())
                throw std::runtime_error("File is empty or could not be read: " + std::string(load));
        }

        if (type == "ram")
            if (load.empty())
                cardptr = new ram_card(at, range);
            else
                cardptr = new ram_card(at, load_file_vec.begin(), load_file_vec.end());
        else if (type == "rom")
            if (load.empty())
                cardptr = new rom_card(at, range);
            else
                cardptr = new rom_card(at, load_file_vec.begin(), load_file_vec.end());
        else if (type == "serial")
            cardptr = new serial_card(at);
        else
            throw std::runtime_error("Config has unknown card type: " + type);

        return cardptr;
    }

    inline void insert_card(card* card, usize slot, bool let_collide) {
        cards.push_back(card);
        cardbus.insert(card, slot, let_collide);
    }

public:
    /// @brief Construct a new system config object by reading a TOML configuration file.
    /// @param filename The path to the file.
    system_config(const char* filename) {
        auto parser = toml::parse(filename);
        auto emulator = toml::find<toml::value>(parser, "emulator");
        auto cards = toml::find<std::vector<toml::value>>(parser, "card");

        for (const auto& card : cards) {
            insert_card(
                create_card(
                    toml::find<std::string>(card, "type"),
                    toml::find<u16>(card, "at"),
                    toml::find_or<usize>(card, "range", 0),
                    toml::find_or<std::string>(card, "load", "")
                ),
                toml::find<usize>(card, "slot"),
                toml::find_or<bool>(card, "let_collide", false)
            );
        }

        start_pc = toml::find_or<u16>(emulator, "start_with_pc_at", 0);
        do_pseudo_bdos = toml::find_or<bool>(emulator, "pseudo_bdos_enabled", false);
    }

    /// @brief Free all memory on destruction.
    ~system_config() { for (card* card : cards) delete card; }

    /// @brief Get a const reference to the vector of card pointers.
    inline const std::vector<card*>& get_cards_vec() const { return cards; }

    /// @brief Get a reference to the bus object.
    inline bus& get_bus() { return cardbus; }

    /// @brief Get whether pseudo BDOS is enabled.
    inline bool get_do_pseudo_bdos() const { return do_pseudo_bdos; }

    /// @brief Get the starting value of PC.
    inline u16 get_start_pc() const { return start_pc; }
};

#endif