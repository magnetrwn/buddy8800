#ifndef BUDDY8800_SRC_CORE_BUS_DEVICE_DESCRIPTION_HPP_
#define BUDDY8800_SRC_CORE_BUS_DEVICE_DESCRIPTION_HPP_

#include "core/bus/card_base.hpp"

namespace buddy8800 {

/**
 * @brief An owned device snapshot for presentation without side effects.
 *
 * Multiple memory/I/O mappings are deferred; this preserves the one-space API.
 */
struct device_description {
    usize slot;
    bool io;
    card_identify identity;
};

}

#endif
