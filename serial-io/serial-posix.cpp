#if defined(__linux__) || defined(__APPLE__)
#include "serial-posix.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
#include <cstring>
#include <sys/ioctl.h>

#include <filesystem>
#include <fstream>

static speed_t getBaud(int baud) {
    switch (baud) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default: return B115200;
    }
}


bool Serial::Impl::open(std::string port, int baudrate) {
    close();
    auto pipeAt = port.find('|');
    if (pipeAt != std::string::npos) {
        port = port.substr(0, pipeAt);
    }

    fd = ::open(port.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) return false;

    termios tty{};
    if (tcgetattr(fd, &tty) != 0) return false;

    cfmakeraw(&tty);
    cfsetispeed(&tty, getBaud(baudrate));
    cfsetospeed(&tty, getBaud(baudrate));

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS;

    return tcsetattr(fd, TCSANOW, &tty) == 0;
}

void Serial::Impl::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

bool Serial::Impl::write(const std::vector<uint8_t>& data) {
    return ::write(fd, data.data(), data.size()) == (ssize_t)data.size();
}

int Serial::Impl::available() {
    int bytes = 0;
    if (ioctl(fd, FIONREAD, &bytes) == -1)
        return 0;

    return bytes;
}
std::vector<uint8_t> Serial::Impl::read(int maxBytes, int timeoutMs) {
    std::vector<uint8_t> buf(maxBytes);

    if (timeoutMs >= 0) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);

        timeval tv{};
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int rv = select(fd + 1, &set, NULL, NULL, &tv);
        if (rv <= 0) return {};
    }

    int n = ::read(fd, buf.data(), maxBytes);
    if (n <= 0) return {};

    buf.resize(n);
    return buf;
}

// Port listing
std::vector<std::string> Serial::listPorts() {
    std::vector<std::string> out;

    const char* path = "/dev";
    DIR* dir = opendir(path);
    if (!dir) return out;

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string name = ent->d_name;

        if (name.find("tty") == 0 || name.find("rfcomm") == 0) {
            out.push_back("/dev/" + name);
        }
    }

    closedir(dir);
    return out;
}



static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::string s;
    std::getline(f, s);
    return s;
}

std::vector<std::string> listPortsLinux() {
    std::vector<std::string> out;

    for (auto& p : std::filesystem::directory_iterator("/sys/class/tty")) {
        std::string dev = p.path().filename().string();

        if (dev.find("tty") != 0 && dev.find("rfcomm") != 0) { continue; }

        std::string base = p.path().string();

        std::string product = readFile(base + "/device/../product");
        std::string manufacturer = readFile(base + "/device/../manufacturer");

        if (!product.empty() || !manufacturer.empty()) {
            std::string pi = std::string("/dev/") + dev + "|" + manufacturer + " " + product;
            out.push_back(pi);
        } else {
            out.push_back(std::string("/dev/") + dev);
        }
    }

    return out;
}

bool Serial::isSerialPath(const std::string& path) {
    return path.starts_with("/dev/tty") || path.starts_with("/dev/rfcomm");
}


#endif