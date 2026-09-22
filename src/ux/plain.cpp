#include "ux/frontend.hpp"
#include "ux/machine_view.hpp"
#include "app/emulator.hpp"
#include <iostream>

namespace buddy8800::ux {

int run_plain(emulator& machine, const volatile std::sig_atomic_t& stop) {
    std::cout << "\x1B[33;01m-:-:-:-:- emulator setup -:-:-:-:-\x1B[0m\n" << std::endl;
    std::cout << format_machine(machine.devices(), true);
    std::cout << "\x1B[33;01m-:-:-:-:- emulator run -:-:-:-:-\x1B[0m" << std::endl;
    machine.run(stop);
    std::cout << "\x1B[33;01m\n-:-:-:-:- emulator end -:-:-:-:-\x1B[0m" << std::endl;
    return 0;
}

}
