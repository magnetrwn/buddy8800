#ifndef BUDDY8800_SRC_LEGACY_DIAGNOSTIC_OUTPUT_HPP_
#define BUDDY8800_SRC_LEGACY_DIAGNOSTIC_OUTPUT_HPP_

#include <fstream>
#include <ostream>
#include <stdexcept>

// Deprecated diagnostic-only output support. Retained for the unchanged tests.
namespace buddy8800::legacy {

class print_helper {
private:
    std::ostream* by_default;
    std::ofstream file_redirect;

public:
    /// @brief Set a redirection to file.
    void set(const char* filename) {
        file_redirect = std::ofstream(filename, std::ios::binary | std::ios::trunc);
        if (!file_redirect.is_open())
            throw std::invalid_argument("Could not open file for printer.");
    }

    /// @brief Reset and fallback to default destination.
    void reset() {
        if (file_redirect.is_open()) {
            file_redirect.flush();
            file_redirect.close();
        }
    }

    /// @brief Print data to the set destination.
    template <typename value_type>
    void print(const value_type& data) {
        if (file_redirect.is_open()) {
            file_redirect << data;
            if (file_redirect.fail())
                throw std::runtime_error("Failed to write to file.");
        } else
            *by_default << data;
    }

    /// @brief Operator overload that calls print(), matching common usage of << operator.
    template <typename value_type>
    print_helper& operator<<(const value_type& data) {
        print(data);
        return *this;
    }

    // Rebind only this diagnostic printer; never replace a process-wide rdbuf.
    std::ostream& set_stream(std::ostream& stream) {
        auto& previous = *by_default;
        by_default = &stream;
        return previous;
    }

    print_helper(std::ostream& by_default) : by_default(&by_default) {}

    ~print_helper() {
        if (file_redirect.is_open())
            reset();
    }
};

}

#endif
