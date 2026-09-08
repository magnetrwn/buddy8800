#ifndef TRACE_VIEW_HPP_
#define TRACE_VIEW_HPP_

#include <deque>
#include <sstream>
#include <iomanip>
#include "cpu_state.hpp"
#include "util.hpp"

inline std::string instruction_line(u16 address, const std::array<u8, 3>& bytes, usize size) {
    std::ostringstream line;
    line << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << address << "  ";
    for (usize i = 0; i < 3; ++i) {
        if (i < size) line << std::setw(2) << static_cast<unsigned>(bytes[i]) << ' ';
        else line << "   ";
    }
    line << " " << util::get_opcode_str(bytes[0]);
    return line.str();
}

// Bound memory even when the guest runs indefinitely. Rendering is independent
// of instruction execution, so slow terminal refreshes cannot grow an output queue.
class trace_history {
    std::deque<std::string> lines;
public:
    static constexpr usize capacity = 2048;
    void append(std::string line) {
        if (lines.size() == capacity) lines.pop_front();
        lines.push_back(std::move(line));
    }
    usize size() const { return lines.size(); }
    void clear() { lines.clear(); }
    const std::string& operator[](usize index) const { return lines.at(index); }
};
#endif
