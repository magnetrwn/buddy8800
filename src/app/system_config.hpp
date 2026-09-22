#ifndef BUDDY8800_SRC_APP_SYSTEM_CONFIG_HPP_
#define BUDDY8800_SRC_APP_SYSTEM_CONFIG_HPP_

#include <memory>
#include <vector>
#include "core/bus/bus.hpp"
#include "app/configuration.hpp"

/**
 * @brief Owns configured cards and their bus.
 *
 * Parsing lives outside this interface; the CPU and frontends borrow the bus.
 */
class system_config {
    std::vector<std::unique_ptr<card>> cards;
    bus cardbus;
    u16 start_pc = 0;
    bool do_pseudo_bdos = false;

public:
    /// @brief Parse a configuration file and construct its devices.
    explicit system_config(const char* filename);

    /// @brief Construct devices from parsed settings.
    explicit system_config(const buddy8800::app::machine_config& config);
    system_config(const system_config&) = delete;

    system_config& operator=(const system_config&) = delete;

    /// @name Configured resources and startup settings
    /// \{

    /// @brief Inspect the owned cards.
    const auto& get_cards_vec() const { return cards; }

    /// @brief Borrow the configured bus.
    bus& get_bus() { return cardbus; }

    /// @brief Report whether legacy diagnostic interception is enabled.
    /// @deprecated Diagnostic compatibility only.
    bool get_do_pseudo_bdos() const { return do_pseudo_bdos; }

    /// @brief Return the configured initial program counter.
    u16 get_start_pc() const { return start_pc; }

    /// \}
};

#endif
