#include "app/configuration.hpp"
#include <toml.hpp>
#include <cstdint>
#include <stdexcept>

namespace buddy8800::app {

namespace {

std::int64_t integer(const toml::value& value, const char* key, std::int64_t fallback, std::int64_t maximum) {
    const auto result = value.contains(key) ? toml::find<std::int64_t>(value, key) : fallback;
    if (result < 0 || result > maximum)
        throw std::out_of_range(std::string("Config value out of range: ") + key);
    return result;
}

}

machine_config read_config(const std::filesystem::path& filename) {
    const auto parser = toml::parse(filename.string());
    const auto settings = toml::find(parser, "emulator");
    machine_config result;
    result.start_pc = integer(settings, "start_with_pc_at", 0, 65535);
    result.pseudo_bdos = toml::find_or<bool>(settings, "pseudo_bdos_enabled", false);
    const auto directory = std::filesystem::absolute(filename).parent_path();
    const auto entries = toml::find(parser, "card").as_array();
    for (const auto& entry : entries) {
        card_config config;
        config.type = toml::find<std::string>(entry, "type");
        const bool is_front_panel = config.type == "front_panel";
        config.at = integer(entry, "at", is_front_panel ? 0xFF : -1, is_front_panel ? 255 : 65535);
        config.slot = integer(entry, "slot", -1, 17);
        config.range = integer(entry, "range", 0, 65536);
        const auto load = toml::find_or<std::string>(entry, "load", "");
        if (!load.empty())
            config.load = directory / load;
        config.allow_collision = toml::find_or<bool>(entry, "let_collide", false);
        if (is_front_panel) {
            config.switches = integer(entry, "switches", 0, 255);
            if (entry.contains("load") || entry.contains("range"))
                throw std::invalid_argument("Front panel cards do not accept load or range");
        } else if (config.type == "serial") {
            if (!load.empty() || config.range != 0)
                throw std::invalid_argument("Serial cards do not accept load or range");
        } else if (config.type != "ram" && config.type != "rom") {
            throw std::invalid_argument("Unknown card type: " + config.type);
        }
        result.cards.push_back(std::move(config));
    }
    return result;
}

}
