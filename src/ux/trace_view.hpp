#ifndef BUDDY8800_SRC_UX_TRACE_VIEW_HPP_
#define BUDDY8800_SRC_UX_TRACE_VIEW_HPP_

#include <deque>
#include <sstream>
#include <iomanip>
#include <array>
#include <string>
#include <utility>
#include "util/typedef.hpp"
#include "core/cpu/opcode_names.hpp"

/// @brief Format an instruction address, bytes and mnemonic.
inline std::string instruction_line(u16 address, const std::array<u8, 3>& bytes, usize size) {
    std::ostringstream line;
    line << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << address << "  ";
    for (usize i = 0; i < 3; ++i) {
        if (i < size)
            line << std::setw(2) << static_cast<unsigned>(bytes[i]) << ' ';
        else
            line << "   ";
    }
    line << " " << buddy8800::instructions::get_opcode_str(bytes[0]);
    return line.str();
}

/**
 * @brief Keep a bounded history of formatted trace lines.
 *
 * Rendering is independent of execution; slow refreshes cannot grow the queue.
 */
class trace_history {
    std::deque<std::string> lines;

public:
    static constexpr usize CAPACITY = 2048;

    /// @name History access
    /// \{

    /// @brief Append a line, discarding the oldest when full.
    void append(std::string line) {
        if (lines.size() == CAPACITY)
            lines.pop_front();
        lines.push_back(std::move(line));
    }

    /// @brief Return the number of retained lines.
    usize size() const { return lines.size(); }

    /// @brief Discard all retained lines.
    void clear() { lines.clear(); }

    /// @brief Read a retained line with bounds checking.
    const std::string& operator[](usize index) const { return lines.at(index); }

    /// \}
};
#endif
