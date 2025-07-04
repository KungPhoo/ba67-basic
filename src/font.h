#pragma once
#include <cstdint>
namespace Font {
extern char32_t PETSCIItoUnicode(uint8_t petscii);
extern char32_t parseNextPetcat(const char*& str);
};
