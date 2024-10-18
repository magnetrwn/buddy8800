#ifndef UNIX_PTY_HPP_
#define UNIX_PTY_HPP_

#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>

#include "typedef.hpp"

enum class pty_parity : u32 {
    NONE, EVEN, ODD
};

class pty {
private:
    static constexpr usize MAX_SLAVE_DEVICE_NAME = 128;

    static constexpr u32 DEFAULT_BAUD_RATE       = 300;
    static constexpr u32 DEFAULT_DATA_BITS       = 8;
    static constexpr pty_parity DEFAULT_PARITY   = pty_parity::NONE;
    static constexpr u32 DEFAULT_STOP_BITS       = 1;

    fd master_fd;
    char slave_device_name[MAX_SLAVE_DEVICE_NAME];

    bool echo_received_back;

public:
    inline void open() {
        master_fd = posix_openpt(O_RDWR | O_NOCTTY);
        if (master_fd < 0)
            throw std::runtime_error("posix_openpt() failed");
        if (grantpt(master_fd) < 0)
            throw std::runtime_error("grantpt() failed");
        if (unlockpt(master_fd) < 0)
            throw std::runtime_error("unlockpt() failed");
        if (ptsname_r(master_fd, slave_device_name, MAX_SLAVE_DEVICE_NAME) < 0)
            throw std::runtime_error("ptsname_r() failed");

        setup();
    }

    inline const char* name() const {
        if (master_fd < 0)
            return "";
        return slave_device_name;
    }

    inline void send(const char* data) {
        send(data, std::strlen(data));
    }

    inline void send(const char* data, usize size) {
        usize total_wr = 0;
        while (total_wr < size) {
            isize wr_amount = write(master_fd, data + total_wr, size - total_wr);
            if (wr_amount < 0)
                throw std::runtime_error("write() failed");
            total_wr += wr_amount;
        }
    }

    inline char getch() {
        char c;
        isize recv_amount = read(master_fd, &c, 1);
        if (recv_amount != 1)
            throw std::runtime_error("read() failed");
        return c;
    }

    inline void recv(char* data, usize max, char terminator = '\r') {
        if (max == 0)
            throw std::invalid_argument("recv() buffer max must be greater than 0");

        usize total_recv = 0;

        while ((total_recv == 0 or data[total_recv - 1] != terminator) and total_recv < max - 1) {
            isize recv_amount = read(master_fd, data + total_recv, max - total_recv - 1);

            if (recv_amount < 0)
                throw std::runtime_error("read() failed");
            else if (recv_amount == 0) // On EOF
                break;
            
            if (echo_received_back)
                send(data + total_recv, recv_amount);

            total_recv += recv_amount;
        }
        data[total_recv] = '\0';
    }

    inline void setup(u32 baud_rate = DEFAULT_BAUD_RATE, 
                      u32 data_bits = DEFAULT_DATA_BITS, 
                      pty_parity parity = DEFAULT_PARITY, 
                      u32 stop_bits = DEFAULT_STOP_BITS
    ) {
        struct termios tty;
        if (tcgetattr(master_fd, &tty) != 0)
            throw std::runtime_error("tcgetattr() failed");

        cfsetospeed(&tty, baud_rate);
        cfsetispeed(&tty, baud_rate);
        cfmakeraw(&tty);

        tty.c_cflag &= ~CSIZE;
        switch (data_bits) {
            case 5: tty.c_cflag |= CS5; break;
            case 6: tty.c_cflag |= CS6; break;
            case 7: tty.c_cflag |= CS7; break;
            case 8: tty.c_cflag |= CS8; break;
            default: throw std::invalid_argument("Invalid data_bits value");
        }

        if (parity == pty_parity::NONE)
            tty.c_cflag &= ~PARENB;
        else if (parity == pty_parity::EVEN) {
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
        } else if (parity == pty_parity::ODD) {
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
        }

        if (stop_bits == 1)
            tty.c_cflag &= ~CSTOPB;
        else if (stop_bits == 2)
            tty.c_cflag |= CSTOPB;
        else
            throw std::invalid_argument("Invalid stop_bits value");

        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cc[VMIN] = 1;
        tty.c_cc[VTIME] = 0;

        if (tcsetattr(master_fd, TCSANOW, &tty) != 0)
            throw std::runtime_error("tcsetattr() failed");
    }

    inline void set_echo_received_back(bool should) {
        echo_received_back = should;
    }

    pty() : master_fd(-1), echo_received_back(false) {};

    ~pty() {
        if (master_fd >= 0)
            close(master_fd);
    }
};

#endif
