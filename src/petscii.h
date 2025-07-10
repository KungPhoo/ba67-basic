#pragma once

#include <cstdint>
class PETSCII {
public:
    static char32_t toUnicode(uint8_t petscii);
};