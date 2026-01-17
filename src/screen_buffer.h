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
#include "kernal.h"

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
    void initMemory(MEMCELL* mem);
    const MEMCELL* getMemory() const { return &memory[0]; }

    // SCNCLR
    void clear();

    // blank out current line, reset line link table flag
    void cleanCurrentLine();


    // Cursor <-> Position
    void setCursorActive(bool active) { memory[krnl.BLNSW] = active ? 0 : 1; }
    bool isCursorActive() const { return memory[krnl.BLNSW] == 0; }

    // these return position in buffer
    inline Cursor getCursorPos() const {
        // return { colOf(cursorPosition), rowOf(cursorPosition) };
        return { memory[krnl.PNTR], memory[krnl.TBLX] };
    }
    inline void setCursorPos(Cursor crsr) {

        if (crsr.x < 0) {
            crsr.x = 0;
        } else if (crsr.x >= width) {
            crsr.x = 0;
            ++crsr.y;
        }
        if (crsr.y < 0) {
            crsr.y = 0;
        } else if (crsr.y >= height) {
            scrollUpOne();
            crsr.y = height - 1;
        }



        dirtyFlag = true;
        // cursorPosition    = ptrAt(crsr.y, crsr.x);
        memory[krnl.PNTR] = MEMCELL(crsr.x);
        memory[krnl.TBLX] = MEMCELL(crsr.y);

        size_t pnt           = (charRam + width * crsr.y) - memory;
        memory[krnl.PNT]     = pnt & 0xff;
        memory[krnl.PNT + 1] = (pnt >> 8) & 0xff;
        assertCursor();
    }
    inline void setCursorPtr(MEMCELL* pos) {
        setCursorPos({ colOf(pos), rowOf(pos) });
    }
    inline MEMCELL* getCursorPtr() {
        Cursor c = getCursorPos();
        return charRam + c.x + width * c.y;
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
    void setColors(uint8_t text, uint8_t back) {
        setTextColor(text);
        setBackgroundColor(back);
    }

    // get/set the text foreground color
    void setTextColor(int index) {
        memory[krnl.COLOR] = (index & 0x0f) | ((memory[krnl.VIC_BKGND] & 0x0f) << 4); // BACKGROUND_COLOR_ALSO_SEE_HERE
    }
    inline uint8_t getTextColor() const { return memory[krnl.COLOR] & 0x0f; }
    // get/set text background color
    // C64/C128 had no mode to display per-character background color
    // however, the high nibble at krnl.COLOR was never used.
    // BA67 uses it, but also stores the *one* background color
    // at krnl.VIC_BKGND.
    void setBackgroundColor(int index) {
        memory[krnl.VIC_BKGND] = (index & 0x0f);
        setTextColor(getTextColor()); // set background in foreground byte, too
    }
    inline uint8_t getBackgroundColor() const { return memory[krnl.VIC_BKGND] & 0x0f; }

    void setBorderColor(int index) { memory[krnl.VIC_BORDER] = (index & 0x0f); }
    uint8_t getBorderColor() const { return memory[krnl.VIC_BORDER] & 0x0f; }

    void setReverseMode(bool enable) { memory[krnl.RVS] = enable ? 1 : 0; } // reverse text and background colors
    bool getReverseMode() const { return memory[krnl.RVS] != 0; }

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

    // save/restore screen for emoji picker etc.
    struct SaveState {
        std::vector<MEMCELL> screen;
        Cursor crsr;
        uint8_t c1, c2;
        bool rvs;
    };
    SaveState saveState() const {
        SaveState s;
        s.screen.resize(width * height);
        for (size_t n = 0; n < width * height; ++n) {
            s.screen[n] = charRam[n];
        }
        s.crsr = getCursorPos();
        s.c1   = getTextColor();
        s.c2   = getBackgroundColor();
        s.rvs  = getReverseMode();

        return s;
    }
    void restoreState(const SaveState& s) {
        for (size_t n = 0; n < s.screen.size(); ++n) {
            charRam[n] = s.screen[n];
        }
        setCursorPos(s.crsr);
        setTextColor(s.c1);
        setBackgroundColor(s.c2);
        setReverseMode(s.rvs);
    }

public:
    size_t width       = 40;
    size_t height      = 25;
    size_t scrollCount = 0; // jus to indicate the screen was scrolled during input


private:
    MEMCELL* memory  = nullptr; // all memory
    MEMCELL* charRam = nullptr; // screen chars
    // !BA67 uses high/low nibble for fore/background of each char
    // colRam & 0x0f = foreground, colRam >> 4 = background
    MEMCELL* colRam = nullptr; // screen colors

    // C64 style line link table. The C128 has a complex bit mapping at $035E-$0361
    MEMCELL* lineLinkTable = nullptr; // continuation flags: 0 = owner, 0x80 = continuation

    // MEMCELL* cursorPosition = nullptr; // internal pointer, to where the cursor is (points to charRam)

    MEMCELL blankChar    = U' ';
    MEMCELL defaultColor = 1;

    struct LineOverflow {
        std::vector<MEMCELL> line; // TODO add color ram, too
        MEMCELL lineLink;
    };
    std::vector<LineOverflow> overflowTop, overflowBottom;


    inline void assertCursor() {
#if defined(_DEBUG)
        const auto* ptr = getCursorPtr();
        if (ptr >= charRam && ptr < charRam + width * height) {
        } else {
            if (ptr < charRam) {
                setCursorPtr(charRam);
            } else {
                setCursorPtr(charRam + width * height - 1);
            }
            throw "cursor is out of screen memory";
        }
#endif
    }

public:
    void setSize(size_t w, size_t h) {
        memory[krnl.LNMX] = MEMCELL(w - 1);
        width             = w;
        height            = h;
    }

    void putC(char32_t c);

    void deleteChar();

    void backspaceChar();


    void insertSpace();
    void scrollUpOne();
    void scrollDownOne();

    void clearHistory();

    inline bool isOwnerRow(size_t r) const { return (lineLinkTable[r] & 0x80) != 0; }
    inline bool isContinuationRow(size_t r) const { return (lineLinkTable[r] & 0x80) == 0; }

private:
    inline size_t idxOf(const MEMCELL* p) const { return static_cast<size_t>(p - charRam); }
    inline size_t rowOf(const MEMCELL* p) const { return idxOf(p) / width; }
    inline size_t colOf(const MEMCELL* p) const { return idxOf(p) % width; }
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

    // row 'r' is continued at 'r+1'?
    inline bool rowContinues(size_t r) const { return r + 1 < height && isContinuationRow(r + 1); }
    void makeRowOwner(size_t r) {
        lineLinkTable[r] |= 0x80; // set bit 7
    }
    void makeRowContinuation(size_t r) {
        lineLinkTable[r] &= 0x7F; // clear bit 7
    }
    void rebuildLineLinkAddresses();

    void hardNewline();

    void softWrapToNextRow();

    void findTailCell(size_t startR, size_t& outR, size_t& outC);

    void ensureOneCellAtTail(size_t tailR, size_t tailC);

    size_t lastUsedColumn(size_t r) const;


protected:
    mutable std::mutex lock;

    // get colRam value for current text and background colors
    inline MEMCELL currentColor() const {
        uint8_t tx = getTextColor();
        uint8_t bk = getBackgroundColor();
        if (getReverseMode()) {
            return MEMCELL(bk | (tx << 4));
        } else {
            return MEMCELL(tx | (bk << 4));
        }
    }

    // draw character at given character position
    // colText and colBack are the AABBGGRR values of the pixels.
    void drawCharPal(std::vector<uint8_t>& pixels, size_t x, size_t y, char32_t ch, uint8_t colText, uint8_t colBack, bool inverse);
    void drawSprPal(std::vector<uint8_t>& pixels, int64_t x, int64_t y, char32_t chimg, int8_t color);
    void drawLineContinuationPal(std::vector<uint8_t>& pixels, size_t yline);

public:
    static uint8_t buddyColor(uint8_t colorindex);
};
