#ifndef BUDDY8800_SRC_CORE_CARDS_RAM_CARD_HPP_
#define BUDDY8800_SRC_CORE_CARDS_RAM_CARD_HPP_

#include "core/cards/data_card.hpp"

// The aliases retain the existing write-lock and constructor contracts.
using ram_card = data_card<false>;

#endif
