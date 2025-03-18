#pragma once
class CharMap;
namespace Font {
extern void createCharmap(CharMap& charmap, char32_t from = 0, char32_t to = 0xffffffff);
};
