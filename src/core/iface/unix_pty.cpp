#include "unix_pty.hpp"
#include <cerrno>
#include <poll.h>
#include <system_error>

namespace {
void fail(const char* operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}
void wait_for(int descriptor, short events) {
    pollfd event{descriptor, events, 0};
    int result;
    do { result = ::poll(&event, 1, -1); } while (result < 0 && errno == EINTR);
    if (result < 0) fail("poll");
    if (event.revents & (POLLERR | POLLHUP | POLLNVAL))
        throw std::runtime_error("PTY closed while waiting for I/O");
}
}

void pty::open() {
    if (master_fd >= 0) throw std::logic_error("PTY already open");
    try {
        master_fd = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
        if (master_fd < 0) fail("posix_openpt");
        if (grantpt(master_fd) < 0) fail("grantpt");
        if (unlockpt(master_fd) < 0) fail("unlockpt");
        int error = ptsname_r(master_fd, slave_device_name, MAX_SLAVE_DEVICE_NAME);
        if (error != 0) throw std::system_error(error, std::generic_category(), "ptsname_r");
        // Preserve startup output and allow clients to disconnect and reconnect.
        slave_fd = ::open(slave_device_name, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
        if (slave_fd < 0) fail("open PTY slave");
        setup(DEFAULT_DATA_BITS, DEFAULT_PARITY, DEFAULT_STOP_BITS);
        set_baud_rate(DEFAULT_BAUD_RATE);
    } catch (...) { close(); throw; }
}
const char* pty::name() const { return master_fd < 0 ? "" : slave_device_name; }
bool pty::try_getch(u8& byte) const {
    ssize_t result;
    do { result = ::read(master_fd, &byte, 1); } while (result < 0 && errno == EINTR);
    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return false;
    if (result < 0) fail("read PTY");
    return result == 1;
}
bool pty::try_putch(u8 byte) const {
    ssize_t result;
    do { result = ::write(master_fd, &byte, 1); } while (result < 0 && errno == EINTR);
    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return false;
    if (result < 0) fail("write PTY");
    return result == 1;
}
char pty::getch() const {
    u8 byte;
    while (!try_getch(byte)) wait_for(master_fd, POLLIN);
    if (echo_received_back) putch(static_cast<char>(byte));
    return static_cast<char>(byte);
}
void pty::putch(char byte) const {
    while (!try_putch(static_cast<u8>(byte))) wait_for(master_fd, POLLOUT);
}
void pty::send(const char* data) const { send(data, std::strlen(data)); }
void pty::send(const char* data, usize size) const {
    for (usize i = 0; i < size; ++i) putch(data[i]);
}
bool pty::poll() const {
    if (master_fd < 0) throw std::logic_error("PTY is closed");
    pollfd event{master_fd, POLLIN, 0};
    int result;
    do { result = ::poll(&event, 1, 0); } while (result < 0 && errno == EINTR);
    if (result < 0) fail("poll PTY");
    return result > 0 && (event.revents & POLLIN);
}
void pty::recv(char* data, usize max, char terminator) const {
    if (max == 0) throw std::invalid_argument("recv buffer must not be empty");
    if (max == 1) { data[0] = getch(); return; }
    usize count = 0;
    while (count < max - 1) {
        data[count++] = getch();
        if (data[count - 1] == terminator) break;
    }
    data[count] = '\0';
}
void pty::setup(u32 data_bits, pty_parity parity, u32 stop_bits) {
    if (data_bits < 5 || data_bits > 8 || (stop_bits != 1 && stop_bits != 2) ||
        (parity != pty_parity::NONE && parity != pty_parity::EVEN && parity != pty_parity::ODD))
        throw std::invalid_argument("Invalid serial framing");
    // PTYs transport bytes, not UART frames. Keep Linux raw and eight-bit clean.
    termios tty{};
    if (tcgetattr(slave_fd, &tty) < 0) fail("tcgetattr");
    cfmakeraw(&tty);
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;
    if (tcsetattr(slave_fd, TCSANOW, &tty) < 0) fail("tcsetattr");
}
void pty::set_baud_rate(u32 baud_rate) {
    speed_t speed;
    switch (baud_rate) {
        case 300: speed = B300; break;
        case 1200: speed = B1200; break;
        case 2400: speed = B2400; break;
        case 4800: speed = B4800; break;
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 115200: speed = B115200; break;
        default: throw std::invalid_argument("Unsupported PTY baud rate");
    }
    termios tty{};
    if (tcgetattr(slave_fd, &tty) < 0) fail("tcgetattr");
    if (cfsetospeed(&tty, speed) < 0 || cfsetispeed(&tty, speed) < 0) fail("cfsetspeed");
    if (tcsetattr(slave_fd, TCSANOW, &tty) < 0) fail("tcsetattr");
}
void pty::send_break() const {
    if (tcsendbreak(master_fd, DEFAULT_BREAK_DURATION) < 0) fail("tcsendbreak");
}
void pty::set_echo_received_back(bool should) { echo_received_back = should; }
void pty::close() {
    if (slave_fd >= 0) { ::close(slave_fd); slave_fd = -1; }
    if (master_fd >= 0) { ::close(master_fd); master_fd = -1; }
}
