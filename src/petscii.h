#pragma once

#include <cstdint>
#include <string>
class PETSCII {
public:
    static char32_t unicodeFromAltKeyPress(char keyChar, bool withShift);
    static char32_t toUnicode(uint8_t petscii);
    static uint8_t fromUnicode(char32_t c, uint8_t fallback = 0xff);
};