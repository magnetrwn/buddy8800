#ifndef BUDDY8800_SRC_APP_PROGRAM_LOADER_HPP_
#define BUDDY8800_SRC_APP_PROGRAM_LOADER_HPP_

#include <vector>
#include "app/options.hpp"
class emulator;

namespace buddy8800::app {

/// @brief Load requested binaries into the machine in command-line order.
void load_programs(emulator& machine, const std::vector<program_load>& programs);

}

#endif
