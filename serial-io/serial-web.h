#ifdef __EMSCRIPTEN__
#include "serial-io.h"

struct Serial::Impl {
    bool open(std::string, int) { return false; }
    void close() {}
    bool write(const std::vector<uint8_t>&) { return false; }
    std::vector<uint8_t> read(int, int) { return {}; }
};

#endif