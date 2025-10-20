#pragma once

#include <cstdint>
#include <string>
class PETSCII {
public:
    // these use the BA67 mapping
    static char32_t unicodeFromAltKeyPress(char keyChar, bool withShift);
    static char32_t toUnicode(uint8_t petscii);
    static uint8_t fromUnicode(char32_t c, uint8_t fallback = 0xff);

    // this one would be the real mapping. PETSCII$() uses this.
    static char32_t realPETSCIItoUnicode(uint8_t petscii, bool shiftedFont);
};