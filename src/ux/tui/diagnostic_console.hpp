#ifndef BUDDY8800_SRC_UX_TUI_DIAGNOSTIC_CONSOLE_HPP_
#define BUDDY8800_SRC_UX_TUI_DIAGNOSTIC_CONSOLE_HPP_

#include "ux/trace_view.hpp"
#include <ostream>
#include <streambuf>
#include <string>

namespace buddy8800::ux::tui {

/// @brief Adapt deprecated pseudo-BDOS output into bounded trace history.
class diagnostic_console : public std::streambuf {
    trace_history& history;
    std::string line;
    std::ostream output{this};
    bool enabled = true;

    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof()))
            return traits_type::not_eof(ch);
        if (!enabled)
            return ch;
        const char byte = traits_type::to_char_type(ch);
        if (byte == '\n' || line.size() == 120) {
            history.append("BDOS: " + line);
            line.clear();
        }
        if (byte != '\r' && byte != '\n')
            line += (byte >= 32 && byte < 127) ? byte : '.';
        return ch;
    }

public:
    explicit diagnostic_console(trace_history& history) : history(history) {}

    diagnostic_console(const diagnostic_console&) = delete;

    diagnostic_console& operator=(const diagnostic_console&) = delete;

    /// @name Diagnostic output capture
    /// \{

    /// @brief Borrow the diagnostic output stream.
    std::ostream& stream() { return output; }

    /// @brief Enable or suppress capture, discarding a partial line.
    void enable(bool value) {
        enabled = value;
        line.clear();
    }

    /// @brief Append any partial line to the trace history.
    void flush_pending() {
        if (!line.empty()) {
            history.append("BDOS: " + line);
            line.clear();
        }
    }

    /// \}
};

}

#endif
