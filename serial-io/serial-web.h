#ifdef __EMSCRIPTEN__
#include "serial-io.h"

struct Serial::Impl {
    bool open(std::string, int);
    void close() ;
    bool write(const std::vector<uint8_t>&);
    int available();
    std::vector<uint8_t> read(int, int);
};

#endif