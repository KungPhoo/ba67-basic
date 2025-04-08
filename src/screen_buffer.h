#pragma once
#include <array>
#include <atomic>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct ScreenInfo {
    static const size_t charsX = 40, charsY = 25;
    static const size_t charPixX = 8 /*don't change!*/, charPixY = 8 /*8 or 16*/;
    static const size_t pixX = charsX * charPixX;
    static const size_t pixY = charsY * charPixY;
};

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
                    setMulti(i * 2, bytes[i] & 0x0f);
                    setMulti(i * 2 + 1, bytes[i] >> 4);
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
        if (n >= ScreenInfo::charPixY * ScreenInfo::charPixY)
            throw std::out_of_range("Pixel index out of range");
        size_t byteIndex = n / 2;
        bool highNibble  = (n % 2 == 0);
        return highNibble ? (bits[byteIndex] >> 4) : (bits[byteIndex] & 0x0F);
    }

    // Multicolor: Set the nth pixel (0-based index) to value (4-bit)
    inline void setMulti(size_t n, uint8_t value) {
        if (n >= ScreenInfo::charPixY * ScreenInfo::charPixY)
            throw std::out_of_range("Pixel index out of range");
        if (value > 0xF)
            throw std::invalid_argument("Pixel value must be 4-bit (0-15)");

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

class Sprite {
public:
    std::array<char32_t, 6> charmap = { 0, 0, 0, 0, 0, 0 };
    int64_t x = 0, y = 0;
    uint8_t color = 1;

    bool stretchX = false, stretchY = false;
    bool enabled = false;
};

class ScreenBitmap {
public:
    ScreenBitmap() {
        pixelsRGB.resize(ScreenInfo::pixX * ScreenInfo::pixY);
        pixelsPal.resize(ScreenInfo::pixX * ScreenInfo::pixY);
    }
    // pixels in AABBGGRR little endian format.
    // 80x25 chars, each 8x16 pixels = 640x400 pixels
    std::vector<uint32_t> pixelsRGB;
    std::vector<uint8_t> pixelsPal; // pixel index in colour palette
    void clear() {
        for (auto& p : pixelsRGB) {
            p = 0;
        }
        for (auto& p : pixelsPal) {
            p = 0;
        }
    }
};
class CharMap {
public:
    CharMap();
    std::array<CharBitmap, 128> ascii;
    std::map<char32_t, CharBitmap> unicode;

    const CharBitmap& operator[](char32_t c) const {
        if (c < 128) {
            return ascii[c];
        }
        auto it = unicode.find(c);
        if (it == unicode.end()) {
            return unicode.begin()->second;
        }
        return it->second;
    }
    CharBitmap& at(char32_t c) {
        if (c < 128) {
            return ascii[c];
        }
        auto it = unicode.find(c);
        return unicode[c];
    }
};

class ScreenBuffer {
public:
    ScreenBuffer();

    // Custom swap function
    friend void swap(ScreenBuffer& lhs, ScreenBuffer& rhs) noexcept {
        using std::swap;

        // Swap everything except screenBitmap
        swap(lhs.palette, rhs.palette);
        swap(lhs.sprites, rhs.sprites);
        swap(lhs.lines, rhs.lines);
        swap(lhs.color, rhs.color);
        swap(lhs.borderColor, rhs.borderColor);
        swap(lhs.cursor, rhs.cursor);
    }

    // Move constructor
    ScreenBuffer(ScreenBuffer&& other) noexcept
        : palette(std::move(other.palette))
        , sprites(std::move(other.sprites))
        , lines(std::move(other.lines))
        , color(other.color)
        , borderColor(other.borderColor)
        , cursor(other.cursor) {
        // Note: screenBitmap is not moved
    }

    // Move assignment operator
    ScreenBuffer& operator=(ScreenBuffer&& other) noexcept {
        if (this != &other) {
            swap(*this, other);
        }
        return *this;
    }

    // zero based cursor position
    struct Cursor {
        size_t x, y;
        bool operator==(const Cursor& c) const { return x == c.x && y == c.y; }
        bool operator!=(const Cursor& c) const { return x != c.x || y != c.y; }
        bool operator<(const Cursor& c) const {
            if (y < c.y) {
                return true;
            }
            if (y == c.y && x < c.x) {
                return true;
            }
            return false;
        }
    };

    ScreenBitmap screenBitmap;
    std::array<uint32_t, 16> palette; // AABBGGRR little endian format
    std::array<Sprite, 256> sprites;

    void clear();

    // character size of screen
    static const size_t width  = ScreenInfo::charsX;
    static const size_t height = ScreenInfo::charsY;

    // at current cursor position
    void putC(char32_t c);
    void defineChar(char32_t codePoint, const CharBitmap& bits);
    const CharBitmap& getCharDefinition(char32_t codePoint) const;

    void deleteChar();
    void backspaceChar();
    void insertSpace();

    // Cursor <-> Position
    Cursor getCursorPos() const;
    // Cursor getCursorAtPos(size_t pos) const;
    // size_t getPosAtCursor(Cursor crsr);

    // these return position in buffer
    const Cursor& setCursorPos(Cursor crsr);
    const Cursor& moveCursorPos(int dx, int dy);
    Cursor getStartOfLineAt(Cursor crsr);
    Cursor getEndOfLineAt(Cursor crsr);

    // buffer to print to a console
    // void getPrintBuffer(std::u32string& chars, std::string& colors) const;

    // puffer colour indices per pixel
    void updateScreenPixelsPalette();
    // buffer to draw on a bitmap
    void updateScreenBitmap();

    std::u32string getSelectedText(Cursor start, Cursor end) const;

    // set the color index [0..15]
    void setColors(uint8_t text, uint8_t back);
    void setTextColor(int index);
    void setBackgroundColor(int index);
    void setBorderColor(int index) { borderColor = (index & 0x0f); }
    void inverseColours(); // interchange text and background colors

    inline int getTextColor() const { return color & 0x0f; }
    inline int getBackgroundColor() const { return (color >> 4) & 0x0f; }
    inline int getBorderColor() const { return (borderColor) & 0x0f; }
    void defineColor(size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff);
    void resetDefaultColors();

    void resetCharmap(char32_t from = 0, char32_t to = 127);

    // thread save copy of all needed information to draw the final pixels
    // the pixels are not copied. You must call updateScreenXXXX() manually, afterwards.
    static void copyWithLock(ScreenBuffer& dst, const ScreenBuffer& src);

    std::atomic<bool> dirtyFlag = true; // must be drawn to screen? Must be cleared manually.

    struct SChar {
        char32_t ch = U' ';
        uint8_t col = 1;
    };
    struct Line {
        void clear() {
            for (auto& c : cols) {
                c.ch  = '\0';
                c.col = 1;
            };
            wrapps = false;
        }
        std::vector<SChar> cols;
        bool wrapps = false; // this line wraps with the next line as one string
    };
    const std::vector<std::shared_ptr<Line>> getLineBuffer() const { return lines; }

protected:
    mutable std::mutex lock;

    void resize(size_t w, size_t h);
    std::vector<std::shared_ptr<Line>> lines;
    Cursor cursor;

    static void deepCopyLines(std::vector<std::shared_ptr<Line>>& dest, const std::vector<std::shared_ptr<Line>>& src);

    // std::u32string buffer;
    // mutable std::basic_string<SChar> buffer;
    uint8_t color; // color&0x0f = foreground, color>>4 = background
    uint8_t borderColor;
    // size_t cursorPos;
    // size_t cursorPos(){return

    void dropFirstLine();
    void manageOverflow();

    // draw character at given character position
    // colText and colBack are the AABBGGRR values of the pixels.
    void drawCharPal(size_t x, size_t y, char32_t ch, uint8_t colText, uint8_t colBack);
    void drawSprPal(int64_t x, int64_t y, char32_t chimg, int8_t color);
    void drawLineContinuationPal(size_t yline);
};