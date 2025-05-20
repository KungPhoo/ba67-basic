#pragma once
#include <cstdint>
class CharMap;
namespace Font {
extern void createCharmap(CharMap& charmap, char32_t from = 0, char32_t to = 0xffffffff);
extern char32_t PETSCIItoUnicode(uint8_t petscii);
extern char32_t parseNextPetcat(const char*& str);
};
