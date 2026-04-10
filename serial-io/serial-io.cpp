#include "serial-io.h"
#include "serial-posix.h"
#include "serial-web.h"
#include "serial-win.h"

Serial::Serial(): impl(nullptr) {}
Serial::~Serial() { close(); }

bool Serial::open(std::string port, int baudrate) {
    close();
    impl = new Impl();
    if (!impl->open(port, baudrate)) {
        delete impl;
        impl = nullptr;
        return false;
    }
    return true;
}

void Serial::close() {
    if (impl) {
        impl->close();
        delete impl;
        impl = nullptr;
    }
}

bool Serial::write(const std::vector<uint8_t>& data) {
    if (!impl) return false;
    return impl->write(data);
}

int Serial::available() {
    if (!impl) return 0;
    return impl->available();
}


std::vector<uint8_t> Serial::read(int maxBytes, int timeoutMs) {
    if (!impl) return {};
    return impl->read(maxBytes, timeoutMs);
}

bool Serial::isOpen() const {
    return impl != nullptr;
}

