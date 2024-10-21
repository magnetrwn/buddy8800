#include "unix_pty.hpp"

void pty::open() {
    master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0)
        throw std::runtime_error("posix_openpt() failed");
    if (grantpt(master_fd) < 0)
        throw std::runtime_error("grantpt() failed");
    if (unlockpt(master_fd) < 0)
        throw std::runtime_error("unlockpt() failed");
    if (ptsname_r(master_fd, slave_device_name, MAX_SLAVE_DEVICE_NAME) < 0)
        throw std::runtime_error("ptsname_r() failed");

    set_baud_rate(DEFAULT_BAUD_RATE);
    setup(DEFAULT_DATA_BITS, DEFAULT_PARITY, DEFAULT_STOP_BITS);
}

const char* pty::name() const {
    if (master_fd < 0)
        return "";
    return slave_device_name;
}

void pty::send(const char* data) const {
    send(data, std::strlen(data));
}

void pty::send(const char* data, usize size) const {
    usize total_wr = 0;
    while (total_wr < size) {
        isize wr_amount = write(master_fd, data + total_wr, size - total_wr);
        if (wr_amount < 0)
            throw std::runtime_error("write() failed");
        total_wr += wr_amount;
    }
}

void pty::send_break() const {
    if (tcsendbreak(master_fd, DEFAULT_BREAK_DURATION) < 0)
        throw std::runtime_error("tcsendbreak() failed");
}

char pty::getch() const {
    char c;
    isize recv_amount = read(master_fd, &c, 1);
    if (recv_amount != 1)
        throw std::runtime_error("read() failed");
    return c;
}

bool pty::poll() const {
    struct pollfd poll_ds;
    poll_ds.fd = master_fd;
    poll_ds.events = POLLIN;

    if (::poll(&poll_ds, 1, 0) < 0 and errno != EINTR)
        throw std::runtime_error("poll() failed");

    return poll_ds.revents & POLLIN;
}

void pty::recv(char* data, usize max, char terminator) const {
    if (max == 0)
        throw std::invalid_argument("recv() buffer max must be greater than 0");

    if (max == 1) {
        data[0] = getch();
        return;
    }

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

void pty::setup(u32 data_bits, pty_parity parity, u32 stop_bits) {
    struct termios tty;
    if (tcgetattr(master_fd, &tty) != 0)
        throw std::runtime_error("tcgetattr() failed");

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

void pty::set_baud_rate(u32 baud_rate) {
    struct termios tty;
    if (tcgetattr(master_fd, &tty) != 0)
        throw std::runtime_error("tcgetattr() failed");

    cfsetospeed(&tty, baud_rate);
    cfsetispeed(&tty, baud_rate);

    if (tcsetattr(master_fd, TCSANOW, &tty) != 0)
        throw std::runtime_error("tcsetattr() failed");
}

void pty::set_echo_received_back(bool should) {
    echo_received_back = should;
}

void pty::close() {
    if (master_fd >= 0)
        ::close(master_fd);
    master_fd = -1;
}