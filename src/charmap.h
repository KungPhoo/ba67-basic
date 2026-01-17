#pragma once
#include <array>
#include <unordered_map>
#include <stdexcept>
#include "screeninfo.h"
#include <cstdint>

class CharBitmap {
public:
    CharBitmap() = default;
    // each byte is a 8 pixels line
    // construct with either 8 or 16 bytes
    CharBitmap(std::initializer_list<uint8_t> lst)
        : CharBitmap(lst.begin(), lst.size()) {
    }

    CharBitmap(const uint8_t* bytes, size_t count)
        : bits {} {
        if (ScreenInfo::charPixY == 8) {
            if (count == 8) { // 8x8 mono pixels
                isMono = true;
                for (size_t i = 0; i < 8; ++i) {
                    bits[i] = bytes[i];
                }
            } else if (count == ScreenInfo::charPixY * ScreenInfo::charPixY / 2) {
                isMono = false; // multicolor - every pixel given
                for (size_t i = 0; i < count; ++i) {
                    setMulti(i * 2, bytes[i] >> 4);
                    setMulti(i * 2 + 1, bytes[i] & 0x0f);
                }
            }
        } else {
            if (count == 8) { // mono: 8x8 every 2nd line is defined
                isMono = true;
                for (size_t i = 0; i < 8; ++i) {
                    bits[i * 2]     = bytes[i];
                    bits[i * 2 + 1] = bits[i * 2];
                }
            } else {
                isMono = true;
                if (count > ScreenInfo::charPixY) {
                    count = ScreenInfo::charPixY;
                }
                for (size_t i = 0; i < count; ++i) {
                    bits[i] = bytes[i];
                }
                for (size_t i = count; i < ScreenInfo::charPixY; ++i) {
                    bits[i] = 0;
                }
            }
        }
    }

    // Multicolor: Get the nth pixel (0-based index)
    inline uint8_t multi(size_t n) const {
        if (n >= ScreenInfo::charPixY * ScreenInfo::charPixY) {
            throw std::out_of_range("Pixel index out of range");
        }
        size_t byteIndex = n / 2;
        bool highNibble  = (n % 2 == 0);
        return highNibble ? (bits[byteIndex] >> 4) : (bits[byteIndex] & 0x0F);
    }

    // Multicolor: Set the nth pixel (0-based index) to value (4-bit)
    inline void setMulti(size_t n, uint8_t value) {
        if (n >= ScreenInfo::charPixY * ScreenInfo::charPixY) {
            throw std::out_of_range("Pixel index out of range");
        }
        if (value > 0xF) {
            throw std::invalid_argument("Pixel value must be 4-bit (0-15)");
        }

        size_t byteIndex = n / 2;
        bool highNibble  = (n % 2 == 0);

        if (highNibble) {
            bits[byteIndex] = (bits[byteIndex] & 0x0F) | (value << 4);
        } else {
            bits[byteIndex] = (bits[byteIndex] & 0xF0) | (value & 0x0F);
        }
    }

    // 8xcharPixY pixels, monochrome - or 4 bits per pixel multicolor
    std::array<uint8_t, (ScreenInfo::charPixY * ScreenInfo::charPixY + 1) / 2> bits = {}; // one byte per line
    bool isMono                                                                     = true;
};

class CharMap {
public:
    CharMap();
    ~CharMap() { delete unicode; }
    void init(char32_t from = 0, char32_t to = char32_t(-1));

    std::array<CharBitmap, 160> ascii; // ASCII including control codes up to 159/0x9f
    std::unordered_map<char32_t, CharBitmap>* unicode; // starting from Latin-1 160/0xa0

    void createColorControlCodes();

    const CharBitmap& operator[](char32_t c) const {
        if (c < 160) {
            return ascii[c];
        }
        auto it = unicode->find(c);
        if (it == unicode->end()) {
            return ascii['?'];
        }
        return it->second;
    }
    CharBitmap& at(char32_t c) {
        if (c < 160) {
            return ascii[c];
        }
        auto it = unicode->find(c);
        if (it == unicode->end()) {
            auto& ref = unicode->insert_or_assign(c, ascii[0]).first->second;
            return ref;
        }
        return it->second;
    }
};
