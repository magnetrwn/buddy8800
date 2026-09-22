#include "app/system_config.hpp"
#include "app/binary_file.hpp"
#include "core/cards/ram_card.hpp"
#include "core/cards/rom_card.hpp"
#include "core/cards/serial_card.hpp"
#include <stdexcept>

namespace {

template <typename Memory>
std::unique_ptr<card> make_memory(const buddy8800::app::card_config& config) {
    if (config.load.empty())
        return std::make_unique<Memory>(config.at, config.range);
    const auto bytes = buddy8800::app::read_binary(config.load);
    return std::make_unique<Memory>(config.at, bytes.begin(), bytes.end(), config.range);
}

std::unique_ptr<card> make_card(const buddy8800::app::card_config& config) {
    if (config.type == "ram")
        return make_memory<ram_card>(config);
    if (config.type == "rom")
        return make_memory<rom_card>(config);
    if (config.type == "serial") {
        if (!config.load.empty() || config.range != 0)
            throw std::invalid_argument("Serial cards do not accept load or range");
        return std::make_unique<serial_card>(config.at);
    }
    throw std::invalid_argument("Unknown card type: " + config.type);
}

}

system_config::system_config(const char* filename) : system_config(buddy8800::app::read_config(filename)) {
}

system_config::system_config(const buddy8800::app::machine_config& config)
    : start_pc(config.start_pc), do_pseudo_bdos(config.pseudo_bdos) {
    cards.reserve(config.cards.size());
    for (const auto& entry : config.cards) {
        auto device = make_card(entry);
        cardbus.insert(device.get(), entry.slot, entry.allow_collision);
        cards.push_back(std::move(device));
    }
}
