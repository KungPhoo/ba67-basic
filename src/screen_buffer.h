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

#if 0
class ScreenBitmap {
public:
    ScreenBitmap() {
        pixelsRGB.resize(ScreenInfo::pixX * ScreenInfo::pixY);
        pixelsPal.resize(ScreenInfo::pixX * ScreenInfo::pixY);
    }
    // pixels in AABBGGRR little endian format.
    // Allocates 80x25 chars, each 8x16 pixels = 640x400 pixels
    // but currently used are ScreenInfo::pixX x screen.height*charPixX
    std::vector<uint32_t> pixelsRGB; // final screen, rgb values
    std::vector<uint8_t> pixelsPal; // final screen, pixel index in color palette
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
    void setCursorActive(bool active) { cursorActive = active; }
    bool isCursorActive() const { return cursorActive; }

    // these return position in buffer
    const Cursor& setCursorPos(Cursor crsr);
    const Cursor& moveCursorPos(int dx, int dy);
    Cursor getStartOfLineAt(Cursor crsr);
    Cursor getEndOfLineAt(Cursor crsr);

    // buffer to print to a console
    // void getPrintBuffer(std::u32string& chars, std::string& colors) const;

    // puffer color indices per pixel
    void updateScreenPixelsPalette(bool highlightCursor);
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
        void trim() {
            for (size_t n = cols.size(); n > 0;) {
                --n;
                if (cols[n].ch != '\0' && cols[n].ch != ' ') {
                    break;
                }
                cols[n].ch = '\0';
            }
        }
        std::vector<SChar> cols;
        bool wrapps = false; // this line wraps with the next line as one string
    };
    const std::vector<std::shared_ptr<Line>> getLineBuffer() const { return lines; }

    // std::array<char32_t, 80 * 25 * 2> memory;
    // char32_t* charRam; // pointer to screen ram $0400 (specified in $00f3..$00f4)
    // char32_t* colRam; // pointers to color ram $d800-$dbe7
    // char32_t* llT; // line link table $d9-$f1 on C64, other on C128


    // #error do it this way:
    // C64: line link table at $D9–$F1
    //      byte & x80: this line belongs to the line before.
    //      screen at $0400 (can be set with pokes)
    //      if you scroll, do so until the 1st line does not have the high bit set!

protected:
    mutable std::mutex lock;

    void resize(size_t w, size_t h);
    std::vector<std::shared_ptr<Line>> lines;
    Cursor cursor;

    static void deepCopyLines(std::vector<std::shared_ptr<Line>>& dest, const std::vector<std::shared_ptr<Line>>& src);

    inline uint8_t color() const { return reverse ? (((textColor & 0xf) << 4) | ((textColor >> 4) & 0x0f)) : textColor; }
    uint8_t textColor; // color&0x0f = foreground, color>>4 = background
    uint8_t borderColor;
    bool reverse      = false; // reverse the color?
    bool cursorActive = false;


    void dropFirstLine();
    void manageOverflow();

    // draw character at given character position
    // colText and colBack are the AABBGGRR values of the pixels.
    void drawCharPal(size_t x, size_t y, char32_t ch, uint8_t colText, uint8_t colBack, bool inverse);
    void drawSprPal(int64_t x, int64_t y, char32_t chimg, int8_t color);
    void drawLineContinuationPal(size_t yline);
    uint8_t buddyColor(uint8_t colorindex);
};
#endif

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
    void clear() {
        dirtyFlag = true;
        size_t n  = width * height;
        for (size_t i = 0; i < n; ++i) {
            charRam[i] = U' ';
            colRam[i]  = currentColor();
        }
        memset(lineLinkTable, 0, sizeof(lineLinkTable[0]) * height);
        cursorPosition = charRam;
    }

    // blank out current line, reset line link table flag
    void cleanCurrentLine() {
        dirtyFlag = true;
        size_t y  = rowOf(cursorPosition);
        if (y < height) {
            auto* p = charRam + y * width;
            for (size_t x = 0; x < width; ++x) {
                *p++ = U' ';
            }
            auto cl = currentColor();
            auto* c = colRam + y * width;
            for (size_t x = 0; x < width; ++x) {
                *c++ = cl;
            }
            if (lineLinkTable) {
                lineLinkTable[y] = 0;
            }
        }
    }


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
    void moveCursorPos(int dx, int dy) {
        dirtyFlag = true;
        cursorPosition += (dy * width + dx);
    }

    Cursor getStartOfLineAt(Cursor crsr) {
        if (!charRam || !lineLinkTable) {
            return crsr;
        }

        size_t r = crsr.y;
        // walk up while this row is a continuation
        while (r > 0 && isContinuationRow(r)) {
            r--;
        }
        // column 0
        return Cursor(0, r);
    }

    Cursor getEndOfLineAt(Cursor crsr) {
        if (!charRam || !lineLinkTable) {
            return crsr;
        }

        size_t r = crsr.y;
        // walk down through continuation rows
        while (r + 1 < height && isContinuationRow(r + 1)) {
            r++;
        }

        // now r = last physical row of this logical line
        // scan right-to-left for last non-blank character
        size_t c = width;
        while (c-- > 0) {
            if (charAt(r, c) != blankChar) {
                break;
            }
        }
        // if whole row blank, c will become size_t(-1) -> clamp to 0
        if (c == size_t(-1)) {
            c = 0;
        }

        return Cursor(c, r);
    }

    // buffer to print to a console
    // void getPrintBuffer(std::u32string& chars, std::string& colors) const;

    // puffer color indices per pixel
    void updateScreenPixelsPalette(bool highlightCursor, std::vector<uint8_t>& pixelsPal);
    // buffer to draw on a bitmap.
    void updateScreenBitmap(std::vector<uint8_t>& pixelsPal, std::vector<uint32_t>& pixelsRGB);

    std::u32string getSelectedText(Cursor start, Cursor end) const {
        const MEMCELL* cstart = ptrAt(start.y, start.x);
        const MEMCELL* cend   = ptrAt(end.y, end.x);
        if (cend < cstart) {
            std::swap(cstart, cend);
        }
        std::u32string str;
        str.reserve(cend - cstart);
        while (cstart != cend) {
            str.push_back(*cstart++);
        }
        str.push_back(*cend);

        return str;
    }

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


protected:
    // static void deepCopyLines(ScreenBuffer& dest, const ScreenBuffer& src);



    // -----------------------------------new
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

    void assertCursor() const {
        if (cursorPosition >= charRam && cursorPosition < charRam + width * height) {
        } else {
            int pause = 1;
        }
    }
    void putC(char32_t c) {
        if (!charRam || !lineLinkTable) {
            return;
        }

        dirtyFlag = true;

        if (c == U'\r' || c == U'\n') {
            hardNewline();
            return;
        }
        if (c == U'\b') {
            backspaceChar();
            return;
        }
        // if (c < 0x20) return;

        *cursorPosition = c;
        assertCursor();
        if (colRam) {
            colRam[idxOf(cursorPosition)] = currentColor();
        }

        if (colOf(cursorPosition) + 1 < width) {
            cursorPosition++;
            assertCursor();
        } else {
            softWrapToNextRow();
        }
    }

    void deleteChar() {
        if (!charRam) {
            return;
        }

        dirtyFlag = true;

        size_t r = rowOf(cursorPosition);
        size_t c = colOf(cursorPosition);

        while (true) {
            size_t srcR = r, srcC = c + 1;
            if (srcC >= width) {
                if (!rowContinues(srcR)) {
                    break;
                }
                srcR++;
                srcC = 0;
            }
            MEMCELL ch = charAt(srcR, srcC);
            MEMCELL co = colorAt(srcR, srcC);
            setAt(r, c, ch, co);

            c++;
            if (c >= width) {
                if (!rowContinues(r)) {
                    break;
                }
                r++;
                c = 0;
            }
        }
        setAt(r, c, blankChar, currentColor());
    }

    // void backspaceChar() {
    //     if (!charRam)
    //         return;
    //
    //     dirtyFlag = true;
    //
    //     if (colOf(cursorPosition) == 0 && isContinuationRow(rowOf(cursorPosition))) {
    //         size_t prevR   = rowOf(cursorPosition) - 1;
    //         cursorPosition = ptrAt(prevR, lastUsedColumn(prevR));
    //     } else if (colOf(cursorPosition) > 0) {
    //         cursorPosition--;
    //     } else
    //         return;
    //
    //     assertCursor();
    //     deleteChar();
    // }

    void backspaceChar() {
        if (cursorPosition <= charRam) {
            return;
        }
        moveCursorPos(-1, 0);
        deleteChar();
    }


    void insertSpace() {
        if (!charRam) {
            return;
        }

        dirtyFlag = true;

        assertCursor();
        size_t tailR, tailC;
        findTailCell(rowOf(cursorPosition), tailR, tailC);
        ensureOneCellAtTail(tailR, tailC);

        size_t r = tailR, c = tailC;
        while (true) {
            size_t dstR = r, dstC = c + 1;
            if (dstC >= width) {
                dstR++;
                dstC = 0;
            }

            MEMCELL ch = charAt(r, c);
            MEMCELL co = colorAt(r, c);
            setAt(dstR, dstC, ch, co);

            if (r == rowOf(cursorPosition) && c == colOf(cursorPosition)) {
                break;
            }
            if (c > 0) {
                c--;
            } else {
                r--;
                c = width - 1;
            }
        }
        setAt(rowOf(cursorPosition), colOf(cursorPosition), blankChar, currentColor());
    }

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

    void hardNewline() {
        size_t r = rowOf(cursorPosition);
        // in case we're at x=0, the neline just breaks continuation of the line
        size_t c = colOf(cursorPosition);
        if (c == 0 && isContinuationRow(r)) {
            makeRowOwner(r);
            return;
        }

        if (r + 1 >= height) {
            scrollUpOne();
        }


        cursorPosition = ptrAt(std::min(r + 1, height - 1), 0);
        assertCursor();
        makeRowOwner(rowOf(cursorPosition));
    }

    void softWrapToNextRow() {
        size_t r = rowOf(cursorPosition);
        if (r + 1 >= height) {
            scrollUpOne();
        }
        cursorPosition = ptrAt(std::min(r + 1, height - 1), 0);
        assertCursor();
        makeRowContinuation(rowOf(cursorPosition));
    }

    void findTailCell(size_t startR, size_t& outR, size_t& outC) {
        size_t r = startR;
        while (rowContinues(r) && r + 1 < height) {
            r++;
        }
        for (size_t rr = r + 1; rr-- > startR;) {
            for (size_t cc = width; cc-- > 0;) {
                if (charAt(rr, cc) != blankChar) {
                    outR = rr;
                    outC = cc;
                    return;
                }
            }
        }
        outR = startR;
        outC = size_t(-1);
    }

    void ensureOneCellAtTail(size_t tailR, size_t tailC) {
        if (tailC + 1 < width) {
            return;
        }
        if (tailR + 1 >= height) {
            scrollUpOne();
        }
        makeRowContinuation(std::min(tailR + 1, height - 1));
        setAt(std::min(tailR + 1, height - 1), 0, blankChar, currentColor());
    }

    size_t lastUsedColumn(size_t r) const {
        for (size_t c = width; c-- > 0;) {
            if (charAt(r, c) != blankChar) {
                return c;
            }
        }
        return 0;
    }

    void scrollUpOne() {
        ++scrollCount;
        size_t rowBytes = width;
        std::memmove(charRam, charRam + rowBytes, (height - 1) * rowBytes * sizeof(MEMCELL));
        if (colRam) {
            std::memmove(colRam, colRam + rowBytes, (height - 1) * rowBytes * sizeof(MEMCELL));
        }
        MEMCELL curCol = currentColor();
        for (size_t c = 0; c < width; ++c) {
            charRam[(height - 1) * width + c] = blankChar;
            if (colRam) {
                colRam[(height - 1) * width + c] = curCol;
            }
        }
        for (size_t r = 0; r + 1 < height; ++r) {
            lineLinkTable[r] = lineLinkTable[r + 1];
        }
        makeRowOwner(height - 1);
        size_t col     = std::min(colOf(cursorPosition), width - 1);
        cursorPosition = ptrAt(height - 1, col);
        assertCursor();
    }





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
