#pragma once
#include <cstdint>
class RomImage {
public:
    static const uint8_t* BASIC_V2();
    static const uint8_t* KERNAL_C64();
    static const uint8_t* LOW_RAM();
};