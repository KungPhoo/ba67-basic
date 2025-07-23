#pragma once
#include <cstdint>

class FontDataBits {
public:
    struct DataStruct {
        char32_t c;
        uint8_t b[8];
    };
    static DataStruct* getBits();
};
