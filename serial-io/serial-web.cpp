#ifdef __EMSCRIPTEN__
#include "serial-web.h"

bool Serial::Impl::open(std::string, int) { return false; }
void Serial::Impl::close() {}
bool Serial::Impl::write(const std::vector<uint8_t>&) { return false; }
int Serial::Impl::available() { return 0; }
std::vector<uint8_t> Serial::Impl::read(int, int) { return {}; }

std::vector<std::string> Serial::listPorts() {
    return {};
}

bool Serial::isSerialPath(const std::string& path) {
    return path.starts_with("/dev/tty") || path.starts_with("/dev/rfcomm");
}

#endif

