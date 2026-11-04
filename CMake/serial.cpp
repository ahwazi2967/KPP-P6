#include "serial.h"
#include <libusbp.hpp>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

const uint16_t kVendorId = 0x0403;
const uint16_t kProductId = 0x6001;

std::string find_serial_port() {
    libusbp::device device = libusbp::find_device_with_vid_pid(kVendorId, kProductId);
    libusbp::serial_port port(device);
    return port.get_name();
}

int open_serial(const std::string& port_name) {
    int fd = open(port_name.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) throw std::runtime_error("Gagal membuka " + port_name);
    
    termios tty{};
    tcgetattr(fd, &tty);
    cfmakeraw(&tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    
    tty.c_cc[VTIME] = 1; // 1 decisecond (100ms) timeout
    tty.c_cc[VMIN] = 0;
    
    tcsetattr(fd, TCSANOW, &tty);
    return fd;
}

void send_serial(int fd, const std::string& data) {
    write(fd, data.data(), data.size());
}

std::string receive_serial(int fd) {
    char buf[256];
    int n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return std::string(buf);
    }
    return "";
}
