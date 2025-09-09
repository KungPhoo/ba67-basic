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

using MEMCELL = uint32_t; // what a peek/poke address points to

// only access this from the main thread
class ScreenBuffer {
public:
    // zero based cursor position
    struct Cursor {
        size_t x = 0, y = 0;
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

    ScreenBuffer();

    // SCNCLR
    void clear();

    // blank out current line, reset line link table flag
    void cleanCurrentLine();


    // Cursor <-> Position
    void setCursorActive(bool active) { cursorActive = active; }
    bool isCursorActive() const { return cursorActive; }

    // these return position in buffer
    Cursor getCursorPos() const {
        return { colOf(cursorPosition), rowOf(cursorPosition) };
    }
    void setCursorPos(Cursor crsr) {
        dirtyFlag      = true;
        cursorPosition = ptrAt(crsr.y, crsr.x);
    }
    void moveCursorPos(int dx, int dy);

    Cursor getStartOfLineAt(Cursor crsr);

    Cursor getEndOfLineAt(Cursor crsr);

    // buffer to print to a console
    // void getPrintBuffer(std::u32string& chars, std::string& colors) const;

    // puffer color indices per pixel
    void updateScreenPixelsPalette(bool highlightCursor, std::vector<uint8_t>& pixelsPal);
    // buffer to draw on a bitmap.
    void updateScreenBitmap(std::vector<uint8_t>& pixelsPal, std::vector<uint32_t>& pixelsRGB);

    std::u32string getSelectedText(Cursor start, Cursor end) const;

    // set the color index [0..15]
    void setColors(uint8_t text, uint8_t back);
    void setTextColor(int index);
    void setBackgroundColor(int index);
    void setBorderColor(int index) { borderColor = (index & 0x0f); }
    void reverseMode(bool enable) { reverse = enable; } // reverse text and background colors

    inline int getTextColor() const { return currentColor() & 0x0f; }
    inline int getBackgroundColor() const { return (currentColor() >> 4) & 0x0f; }
    inline int getBorderColor() const { return (borderColor) & 0x0f; }
    void defineColor(size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff);
    void resetDefaultColors();

    void resetCharmap(char32_t from = 0, char32_t to = 127);
    void defineChar(char32_t codePoint, const CharBitmap& bits);
    const CharBitmap& getCharDefinition(char32_t codePoint) const;


    // thread save copy of all needed information to draw the final pixels
    // the pixels are not copied. You must call updateScreenXXXX() manually, afterwards.
    // static void copyWithLock(ScreenBuffer& dst, const ScreenBuffer& src);

    std::atomic<bool> dirtyFlag = true; // must be drawn to screen? Must be cleared manually.

    // ScreenBitmap screenBitmap;
    std::array<uint32_t, 16> palette; // AABBGGRR little endian format
    std::array<Sprite, 256> sprites;


    struct SaveState {
        std::vector<MEMCELL> screen;
        Cursor crsr;
    };
    SaveState saveState() const {
        SaveState s;
        s.screen.resize(width * height);
        for (size_t n = 0; n < width * height; ++n) {
            s.screen[n] = charRam[n];
        }
        s.crsr = getCursorPos();
        return s;
    }
    void restoreState(const SaveState& s) {
        for (size_t n = 0; n < s.screen.size(); ++n) {
            charRam[n] = s.screen[n];
        }
        setCursorPos(s.crsr);
    }

public:
    size_t width       = 40;
    size_t height      = 25;
    size_t scrollCount = 0; // jus to indicate the screen was scrolled during input


    // charRam; // pointer to screen ram $0400 (specified in $00f3..$00f4)
    // colRam; // pointers to color ram $d800-$dbe7
    // llT; // line link table $d9-$f1 on C64, other on C128


    MEMCELL* charRam       = nullptr; // screen chars
    MEMCELL* colRam        = nullptr; // screen colors
    MEMCELL* lineLinkTable = nullptr; // continuation flags: 0 = owner, 0x80 = continuation

    MEMCELL* cursorPosition = nullptr;

    MEMCELL blankChar    = U' ';
    MEMCELL defaultColor = 1;

    inline void assertCursor() const {
#if defined(_DEBUG)
        if (cursorPosition >= charRam && cursorPosition < charRam + width * height) {
        } else {
            throw "cursor is out of screen memory";
        }
#endif
    }
    void putC(char32_t c);

    void deleteChar();

    void backspaceChar();


    void insertSpace();

private:
    size_t idxOf(const MEMCELL* p) const { return static_cast<size_t>(p - charRam); }
    size_t rowOf(const MEMCELL* p) const { return idxOf(p) / width; }
    size_t colOf(const MEMCELL* p) const { return idxOf(p) % width; }
    const MEMCELL* ptrAt(size_t r, size_t c) const { return &charRam[r * width + c]; }
    MEMCELL* ptrAt(size_t r, size_t c) { return &charRam[r * width + c]; }
    MEMCELL charAt(size_t r, size_t c) const { return charRam[r * width + c]; }
    MEMCELL colorAt(size_t r, size_t c) const { return colRam ? colRam[r * width + c] : defaultColor; }
    void setAt(size_t r, size_t c, MEMCELL ch, MEMCELL co) {
        size_t n = r * width + c;
        if (n >= width * height) {
            int pause = 1;
        }
        charRam[n] = ch;
        colRam[n]  = co;
    }
    // MEMCELL currentColor() const { return colRam ? colRam[idxOf(cursorPosition)] : defaultColor; }

    bool isOwnerRow(size_t r) const { return (lineLinkTable[r] & 0x80) == 0; }
    bool isContinuationRow(size_t r) const { return (lineLinkTable[r] & 0x80) != 0; }
    bool rowContinues(size_t r) const { return r + 1 < height && isContinuationRow(r + 1); }
    void makeRowOwner(size_t r) { lineLinkTable[r] = 0; }
    void makeRowContinuation(size_t r) {
        lineLinkTable[r] = 0x80;
    }

    void hardNewline();

    void softWrapToNextRow();

    void findTailCell(size_t startR, size_t& outR, size_t& outC);

    void ensureOneCellAtTail(size_t tailR, size_t tailC);

    size_t lastUsedColumn(size_t r) const;

    void scrollUpOne();


protected:
    mutable std::mutex lock;

    void resize(size_t w, size_t h) {
        width  = w;
        height = h;
    }

    inline MEMCELL currentColor() const { return reverse ? (((textColor & 0xf) << 4) | ((textColor >> 4) & 0x0f)) : textColor; }
    uint8_t textColor; // color&0x0f = foreground, color>>4 = background
    uint8_t borderColor;
    bool reverse      = false; // reverse the color?
    bool cursorActive = false;

    // draw character at given character position
    // colText and colBack are the AABBGGRR values of the pixels.
    void drawCharPal(std::vector<uint8_t>& pixels, size_t x, size_t y, char32_t ch, uint8_t colText, uint8_t colBack, bool inverse);
    void drawSprPal(std::vector<uint8_t>& pixels, int64_t x, int64_t y, char32_t chimg, int8_t color);
    void drawLineContinuationPal(std::vector<uint8_t>& pixels, size_t yline);
    uint8_t buddyColor(uint8_t colorindex);
};
