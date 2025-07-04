#pragma once
#include <atomic>
#include <iostream>
#include <array>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "charmap.h"



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
    std::vector<uint8_t> pixelsPal; // pixel index in color palette
    void clear() {
        for (auto& p : pixelsRGB) {
            p = 0;
        }
        for (auto& p : pixelsPal) {
            p = 0;
        }
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
        swap(lhs.reverse, rhs.reverse);
        swap(lhs.textColor, rhs.textColor);
        swap(lhs.borderColor, rhs.borderColor);
        swap(lhs.cursor, rhs.cursor);
    }

    // Move constructor
    ScreenBuffer(ScreenBuffer&& other) noexcept
        : palette(std::move(other.palette))
        , sprites(std::move(other.sprites))
        , lines(std::move(other.lines))
        , reverse(other.reverse)
        , textColor(other.textColor)
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

    // maximum possible size of screen in characters
    static const size_t maxWidth  = ScreenInfo::charsX;
    static const size_t maxHeight = ScreenInfo::charsY;

    // current screen mode's screen size in characters
    size_t width       = 40;
    size_t height      = maxHeight;
    size_t scrollCount = 0;

    // pixel site of borders
    struct WindowPixels {
        int borderx = 0, bordery = 0; // pixels of borders
        int pixelscalex = 1, pixelscaley = 1; // number of screen pixels for one BA67 pixel
    } windowPixels = {};


    // at current cursor position
    void putC(char32_t c);
    void defineChar(char32_t codePoint, const CharBitmap& bits);
    const CharBitmap& getCharDefinition(char32_t codePoint) const;

    void deleteChar();
    void backspaceChar();
    void insertSpace();
    void cleanCurrentLine();

    // Cursor <-> Position
    Cursor getCursorPos() const;

    // these return position in buffer
    const Cursor& setCursorPos(Cursor crsr);
    const Cursor& moveCursorPos(int dx, int dy);
    Cursor getStartOfLineAt(Cursor crsr);
    Cursor getEndOfLineAt(Cursor crsr);

    // buffer to print to a console
    // void getPrintBuffer(std::u32string& chars, std::string& colors) const;

    // puffer color indices per pixel
    void updateScreenPixelsPalette();
    // buffer to draw on a bitmap
    void updateScreenBitmap();

    std::u32string getSelectedText(Cursor start, Cursor end) const;

    // set the color index [0..15]
    void setColors(uint8_t text, uint8_t back);
    void setTextColor(int index);
    void setBackgroundColor(int index);
    void setBorderColor(int index) { borderColor = (index & 0x0f); }
    void reverseMode(bool enable) { reverse = enable; } // reverse text and background colors

    inline int getTextColor() const { return color() & 0x0f; }
    inline int getBackgroundColor() const { return (color() >> 4) & 0x0f; }
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

    inline uint8_t color() const { return reverse ? (((textColor & 0xf) << 4) | ((textColor >> 4) & 0x0f)) : textColor; }
    uint8_t textColor; // color&0x0f = foreground, color>>4 = background
    uint8_t borderColor;
    bool reverse = false; // reverse the color?

    void dropFirstLine();
    void manageOverflow();

    // draw character at given character position
    // colText and colBack are the AABBGGRR values of the pixels.
    void drawCharPal(size_t x, size_t y, char32_t ch, uint8_t colText, uint8_t colBack);
    void drawSprPal(int64_t x, int64_t y, char32_t chimg, int8_t color);
    void drawLineContinuationPal(size_t yline);
};