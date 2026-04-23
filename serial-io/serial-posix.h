#if defined(__linux__) || defined(__APPLE__)
#include "serial-io.h"

struct Serial::Impl {
    int fd = -1;
    bool open(std::string port, int baudrate) ;
    void close() ;
    bool write(const std::vector<uint8_t>& data);
    int available() ;
    std::vector<uint8_t> read(int maxBytes, int timeoutMs);
};
#endif