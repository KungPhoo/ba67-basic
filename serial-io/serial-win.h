#ifdef _WIN32
#include "serial-io.h"
#include <Windows.h>

struct Serial::Impl {
    HANDLE h = INVALID_HANDLE_VALUE;

    bool open(std::string port, int baudrate) ;

    void close() ;

    bool write(const std::vector<uint8_t>& data);
    int available();
    std::vector<uint8_t> read(int maxBytes, int);
};

#endif