#pragma once
#include <cstdint>
class RomImage {
public:
    static void PatchROM();

    static uint8_t* BASIC_V2();
    static uint8_t* KERNAL_C64();
    static const uint8_t* LOW_RAM();

    static uint8_t* ROM(size_t address);
};