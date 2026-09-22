#include "ux/machine_view.hpp"
#include "util/format.hpp"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace buddy8800::ux {

std::string format_machine(const std::vector<device_description>& devices, bool color) {
    constexpr usize ADDRESS_COLUMN_WIDTH = 12;
    std::ostringstream text;
    for (const auto& device : devices) {
        const auto& ident = device.identity;
        auto range =
            format::to_hex_s(ident.start_adr, device.io ? 2 : 4) + "/" + std::to_string(ident.adr_range);
        if (range.size() < ADDRESS_COLUMN_WIDTH)
            range.resize(ADDRESS_COLUMN_WIDTH, ' ');
        text << "Slot " << std::setw(2) << device.slot << ": ";
        if (color)
            text << (device.io ? "\x1B[45;01m" : "\x1B[47;01m");
        text << (device.io ? "I/O" : "MEM");
        if (color)
            text << "\x1B[0m";
        text << ' ' << range << ": ";
        if (color)
            text << "\x1B[01m";
        text << ident.name;
        if (color)
            text << "\x1B[0m";
        if (!ident.detail.empty())
            text << ", " << ident.detail;
        text << '\n';
    }
    return text.str();
}

std::vector<std::string> machine_lines(const std::vector<device_description>& devices, int width) {
    if (width <= 0)
        throw std::invalid_argument("Machine view width must be positive");
    std::istringstream stream(format_machine(devices, false));
    std::vector<std::string> result;
    for (std::string line; std::getline(stream, line);) {
        while (line.size() > static_cast<usize>(width)) {
            result.push_back(line.substr(0, width));
            line.erase(0, width);
        }
        result.push_back(std::move(line));
    }
    return result;
}

}
