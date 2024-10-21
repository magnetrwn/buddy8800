#ifndef BUS_HPP_
#define BUS_HPP_

#include <array>

#include "typedef.hpp"
#include "card.hpp"

class bus {
private:
    static constexpr usize MAX_BUS_CARDS = 18;

    std::array<card*, MAX_BUS_CARDS> cards { nullptr };

public:

};


#endif