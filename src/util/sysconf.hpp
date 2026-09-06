#ifndef SYSCONF_HPP_
#define SYSCONF_HPP_

#include <filesystem>
#include <memory>
#include <vector>
#include <toml.hpp>
#include "bus.hpp"
#include "card.hpp"

// Owns the cards; the bus only borrows their addresses.
class system_config {
    std::vector<std::unique_ptr<card>> cards;
    bus cardbus;
    u16 start_pc = 0;
    bool do_pseudo_bdos = false;

    static std::int64_t integer(const toml::value& value, const char* key,
                                std::int64_t fallback, std::int64_t maximum) {
        const auto result = toml::find_or<std::int64_t>(value, key, fallback);
        if (result < 0 || result > maximum)
            throw std::out_of_range(std::string("Config value out of range: ") + key);
        return result;
    }

public:
    explicit system_config(const char* filename) {
        const auto parser = toml::parse(filename);
        const auto settings = toml::find(parser, "emulator");
        start_pc = integer(settings, "start_with_pc_at", 0, 65535);
        do_pseudo_bdos = toml::find_or<bool>(settings, "pseudo_bdos_enabled", false);
        const auto directory = std::filesystem::absolute(filename).parent_path();
        const auto entries = toml::find(parser, "card").as_array();
        for (const auto& entry : entries) {
            const auto type = toml::find<std::string>(entry, "type");
            const auto at = integer(entry, "at", -1, 65535);
            const auto slot = integer(entry, "slot", -1, 17);
            const auto range = integer(entry, "range", 0, 65536);
            const auto load = toml::find_or<std::string>(entry, "load", "");
            std::unique_ptr<card> device;
            if (type == "serial") {
                if (!load.empty() || range != 0)
                    throw std::invalid_argument("Serial cards do not accept load or range");
                device = std::make_unique<serial_card>(at);
            } else if (type == "ram" || type == "rom") {
                std::vector<u8> bytes;
                if (!load.empty()) {
                    const auto path = directory / load;
                    std::ifstream file(path, std::ios::binary);
                    if (!file) throw std::runtime_error("Could not open file: " + path.string());
                    bytes.assign(std::istreambuf_iterator<char>(file), {});
                    if (file.bad() || bytes.empty())
                        throw std::runtime_error("Empty or unreadable file: " + path.string());
                }
                if (load.empty()) {
                    if (type == "ram") device = std::make_unique<ram_card>(at, range);
                    else device = std::make_unique<rom_card>(at, range);
                } else {
                    if (type == "ram") device = std::make_unique<ram_card>(at, bytes.begin(), bytes.end(), range);
                    else device = std::make_unique<rom_card>(at, bytes.begin(), bytes.end(), range);
                }
            } else {
                throw std::invalid_argument("Unknown card type: " + type);
            }
            cardbus.insert(device.get(), slot, toml::find_or<bool>(entry, "let_collide", false));
            cards.push_back(std::move(device));
        }
    }

    system_config(const system_config&) = delete;
    system_config& operator=(const system_config&) = delete;
    const auto& get_cards_vec() const { return cards; }
    bus& get_bus() { return cardbus; }
    bool get_do_pseudo_bdos() const { return do_pseudo_bdos; }
    u16 get_start_pc() const { return start_pc; }
};
#endif
