#ifndef UNIX_PTY_HPP_
#define UNIX_PTY_HPP_

#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <poll.h>

#include "typedef.hpp"

/// @brief Enumerates all possible parity modes of the serial device.
enum class pty_parity : u32 {
    NONE, EVEN, ODD
};

/**
 * @brief Represents a pseudo-terminal interface.
 *
 * This class handles a PTY interface and provides some convenience when interacting with it.
 * After construction and by calling open(), the PTY interface is opened and public methods from
 * this class can be used to handle the master file descriptor side of the PTY. The slave side
 * is supposed to be provided to the user or a process to interact with, thus the name() method
 * is available to retrieve the slave device name, but no further handling is done by this class.
 */
class pty {
private:
    static constexpr usize MAX_SLAVE_DEVICE_NAME = 64;

    static constexpr u32 DEFAULT_BAUD_RATE       = 19200;
    static constexpr u32 DEFAULT_DATA_BITS       = 8;
    static constexpr pty_parity DEFAULT_PARITY   = pty_parity::NONE;
    static constexpr u32 DEFAULT_STOP_BITS       = 1;

    static constexpr u32 DEFAULT_BREAK_DURATION  = 0; /// Will default to the termios.h default value

    fd master_fd;
    char slave_device_name[MAX_SLAVE_DEVICE_NAME];

    bool echo_received_back;

public:
    /**
     * @brief Open the PTY interface.
     * @throw `std::runtime_error` if the PTY interface could not be opened.
     * 
     * Use this method when first starting the PTY interface. It will open the master file descriptor
     * and setup various configuration flags for a bit more realism in emulating an Altair 8800 serial
     * interface.
     *
     * @note The default setup configuration is 300 baud, 8 data bits, no parity and 1 stop bit. `B300-8N1`
     */
    void open();

    /**
     * @brief Retrieve the name of the slave device.
     * @return A C-like string containing the name of the slave device.
     * 
     * This method returns the name of the slave device, such as `/dev/pts/3`. This name identifies
     * the slave side PTY interface, to which a user or process can interface with. For example, you
     * could run `screen /dev/pts/3` on your terminal to connect to the PTY slave side.
     */
    const char* name() const;

    /**
     * @brief Send data to the PTY interface master side.
     * @param data A pointer to the data to be sent.
     * @throw `std::runtime_error` if the PTY interface had an error.
     *
     * This method is a wrapper around the `send(const char* data, usize size)` method, where size simply
     * is provided by the length of the passed string using `std::strlen()`.
     *
     * @warning This method will block until all bytes of data are sent.
     */
    void send(const char* data) const;

    /**
     * @brief Send data to the PTY interface master side.
     * @param data A pointer to the data to be sent.
     * @param size The amount of bytes to be sent starting from data.
     * @throw `std::runtime_error` if the PTY interface had an error.
     *
     * This method sends data to the PTY interface master side. The slave side will be able to receive
     * this data in order. It uses the `write()` system call to send the data to the master file descriptor.
     *
     * @warning This method will block until `size` bytes are sent.
     */
    void send(const char* data, usize size) const;

    /**
     * @brief Send a break signal to the PTY interface master side.
     * @throw `std::runtime_error` if the PTY interface had an error.
     * 
     * This method sends a break signal to the PTY interface master side. It uses the `tcsendbreak()` system
     * call to make the pseudo-terminal interface run a break signal.
     *
     * Sending a break signal effectively holds the transmission line low for a considerable amount of time,
     * with no concern for framing or data bits, which is a step further than simply sending 0x00 over and over.
     */
    void send_break() const;

    /**
     * @brief Get a single byte from the PTY interface master side.
     * @return The byte read from the PTY interface.
     * @throw `std::runtime_error` if the PTY interface had an error.
     * 
     * This method reads a single byte/char from the PTY interface master side. It uses the `read()` system
     * call to read the byte from the master file descriptor, sent by the slave side.
     *
     * @warning This method will block until a byte is read.
     */
    char getch() const;

    /**
     * @brief Send a single byte to the PTY interface master side.
     * @param c The byte to be sent.
     * @throw `std::runtime_error` if the PTY interface had an error.
     *
     * This method sends a single byte/char to the PTY interface master side. It uses the `write()` system call.
     *
     * @warning This method will block until the byte is sent.
     */
    void putch(char c) const;

    /**
     * @brief Check if there is data available to be read from the PTY interface master side.
     * @return Whether there is data available to read.
     * @throw `std::runtime_error` if the PTY interface had an error.
     *
     * This method polls the PTY interface master side to check if there is data available to be read. It
     * uses the `poll()` system call to check if there is data available to be read from the master file descriptor.
     * This allows a blocking `getch()` operation to be non-blocking by employing a call to this method before reading.
     */
    bool poll() const;

    /**
     * @brief Receive data from the PTY interface master side.
     * @param data A pointer to the buffer where the data will be stored.
     * @param max The amount of bytes to be read, usually the size of the buffer.
     * @param terminator The character that will stop the reading.
     * @throw `std::runtime_error` if the PTY interface had an error.
     * @throw `std::invalid_argument` if `max` is 0.
     * 
     * This method reads data from the PTY interface master side. It will read up to `max - 1` bytes, until 
     * the terminator character is found. The received data is always null-terminated by this method, thus 
     * `max` is actually considered as `max - 1` for the amount of bytes to be read, with the last byte being
     * reserved for the null-terminator, should data fit the entire buffer.
     *
     * @note If `max` is 1, the method is a wrapper to `getch()` and will not null-terminate the buffer.
     * @warning This method will block until `max - 1` bytes are read or the terminator character is found.
     */
    void recv(char* data, usize max, char terminator = '\r') const;

    /**
     * @brief Setup the PTY interface with custom configuration.
     * @param data_bits The amount of data bits to be used.
     * @param parity The parity mode to be used.
     * @param stop_bits The amount of stop bits to be used.
     * @throw `std::runtime_error` if the PTY interface could not be configured.
     * @throw `std::invalid_argument` if an invalid setup was being configured.
     * 
     * This method sets up the PTY interface with custom configuration. Internally, this makes extensive use
     * of `termios.h` functionality and its functions to configure the PTY interface. 
     */
    void setup(u32 data_bits, pty_parity parity, u32 stop_bits);

    /**
     * @brief Set the baud rate of the PTY interface.
     * @param baud_rate The baud rate to be set.
     * @throw `std::runtime_error` if the PTY interface could not be configured.
     * 
     * This method sets the baud rate of the PTY interface. It uses the `cfsetospeed()` and `cfsetispeed()`
     * functions from `termios.h` to set the baud rate of the PTY interface.
     */
    void set_baud_rate(u32 baud_rate);

    /**
     * @brief Set whether received data should be printed back to the PTY slave side.
     * @param should Whether received data should be echoed.
     */
    void set_echo_received_back(bool should);

    
    /// @brief Close the PTY interface and free the PTY master file descriptor.
    void close();

    pty() : master_fd(-1), echo_received_back(false) {};
    ~pty() { close(); }
};

#endif
