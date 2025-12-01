#include "screen_buffer.h"
#include "font.h"
#include <algorithm>
#include <stdexcept>
#include <cstring>

static CharMap& charMap() {
    static CharMap c;
    return c;
}


// ------------------------------------------------------------------


/*
 * TODO: C64 can PRINT about 4-12 chars per screen update
 *       with every putC, advance a counter. After 10, update
 *       the pixelsPal
 *       after 11 draw sprites
 *       12 pixelBitmap
 *       then set a flag "ready to present" on next presentScreen
 *       there, present and possibly wait until 1/60th sec is over
 * */

// SCNCLR
void ScreenBuffer::clear() {
    dirtyFlag = true;
    size_t n  = ScreenInfo::charsX * ScreenInfo::charsY; // 80!x25 width * height;
    auto col  = currentColor();
    for (size_t i = 0; i < n; ++i) {
        charRam[i] = U' ';
        colRam[i]  = col;
    }
    memset(lineLinkTable, 0, sizeof(lineLinkTable[0]) * height);
    setCursorPos({ 0, 0 });
    overflowTop.clear();
    overflowBottom.clear();
}

// blank out current line, reset line link table flag
void ScreenBuffer::cleanCurrentLine() {
    dirtyFlag = true;
    size_t y  = getCursorPos().y;
    if (y < height) {
        auto* p = charRam + y * width;
        for (size_t x = 0; x < width; ++x) {
            *p++ = U' ';
        }
        auto col = currentColor();
        auto* c  = colRam + y * width;
        for (size_t x = 0; x < width; ++x) {
            *c++ = col;
        }
        makeRowOwner(y);
    }
}

void ScreenBuffer::moveCursorPos(int dx, int dy) {
    dirtyFlag       = true;
    MEMCELL* newPos = getCursorPtr() + (dy * width + dx);
    while (newPos < charRam) {
        scrollDownOne(); // moves cursor one down, so movement must be done
        newPos = getCursorPtr() + (dy * width + dx);
    }

    if (newPos >= charRam + width * height) {
        scrollUpOne(); // moves cursor down
        newPos = getCursorPtr() + (dy * width + dx);
    }

    setCursorPtr(newPos);
}

ScreenBuffer::Cursor ScreenBuffer::getStartOfLineAt(Cursor crsr) {
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

ScreenBuffer::Cursor ScreenBuffer::getEndOfLineAt(Cursor crsr) {
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

ScreenBuffer::ScreenBuffer() {
    charMap(); // load bitmaps
    palette = {};
}

void ScreenBuffer::initMemory(MEMCELL* mem) {
    memory        = mem;
    lineLinkTable = mem + krnl.LDTB1;
    charRam       = mem + krnl.CHARRAM;
    colRam        = mem + krnl.COLRAM;

    setCursorPos({ 0, 0 });
    resize(40, 25);

    resetDefaultColors();
    setColors(1, 0);
    setBorderColor(1);

    overflowTop.reserve(0x10000);
    overflowBottom.reserve(0x10000);
}


void ScreenBuffer::updateScreenPixelsPalette(bool highlightCursor, std::vector<uint8_t>& pixelsPal) {
    // chars
    MEMCELL* visibleCursorPosition = getCursorPtr();
    if (!highlightCursor || !isCursorActive()) {
        visibleCursorPosition = nullptr;
    }

    size_t numChars = width * height;
    pixelsPal.resize(numChars * ScreenInfo::charPixX * ScreenInfo::charPixY);

    MEMCELL *ch = charRam, *end = charRam + numChars;
    MEMCELL* co = colRam;
    size_t x = 0, y = 0;
    while (ch != end) {
        drawCharPal(pixelsPal,
                    x * ScreenInfo::charPixX,
                    y * ScreenInfo::charPixY,
                    *ch,
                    (*co) & 0x0f,
                    ((*co) >> 4) & 0x0f,
                    ch == visibleCursorPosition);
        if (x + 1 == width) {
            if (isContinuationRow(y + 1)) {
                drawLineContinuationPal(pixelsPal, y);
            }
        }
        ++ch;
        ++co;
        ++x;

        if (x == width) {
            x = 0;
            ++y;
        }
    }

    // sprites
    for (auto& sp : sprites) {
        if (!sp.enabled) {
            continue;
        }

        // C64/C128 sprite coordinates are relative to the screen - 0,0 is in the border
        const int sprbx = 25;
        const int sprby = 50;
        drawSprPal(pixelsPal, sp.x + 0 - sprbx, sp.y + sprby, sp.charmap[0], sp.color);
        drawSprPal(pixelsPal, sp.x + 8 - sprbx, sp.y + sprby, sp.charmap[1], sp.color);
        drawSprPal(pixelsPal, sp.x + 16 - sprbx, sp.y + sprby, sp.charmap[2], sp.color);
        drawSprPal(pixelsPal, sp.x + 0 - sprbx, sp.y + 8 + sprby, sp.charmap[3], sp.color);
        drawSprPal(pixelsPal, sp.x + 8 - sprbx, sp.y + 8 + sprby, sp.charmap[4], sp.color);
        drawSprPal(pixelsPal, sp.x + 16 - sprbx, sp.y + 8 + sprby, sp.charmap[5], sp.color);
    }
}

// update the RGB buffer
void ScreenBuffer::updateScreenBitmap(std::vector<uint8_t>& pixelsPal, std::vector<uint32_t>& pixelsRGB) {
    size_t n = pixelsRGB.size();
    size_t p = 0, irgb = 0;
    uint8_t* pal  = &pixelsPal[0];
    uint32_t* rgb = &pixelsRGB[0];
    while (n > 0) {
        --n;
        *rgb++ = palette[*pal++];
        ++p;
        if (p == 8) {
            p = 0;
        }
    }
}

std::u32string ScreenBuffer::getSelectedText(Cursor start, Cursor end) const {
    const MEMCELL* cstart = ptrAt(start.y, start.x);
    const MEMCELL* cend   = ptrAt(end.y, end.x);
    if (cend < cstart) {
        std::swap(cstart, cend);
    }

    std::u32string str;
    auto trim = [&str]() {
        while (!str.empty() && str.back() == U' ') {
            str.pop_back();
        }
    };

    str.reserve(cend - cstart);
    while (cstart != cend) {
        str.push_back(*cstart++);
        if (colOf(cstart) == 0 && !isContinuationRow(rowOf(cstart))) {
            trim();
            str += U"\n";
        }
    }
    str.push_back(*cend);
    trim();
    return str;
}



void ScreenBuffer::defineColor(size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (index > 15) {
        return;
    }
    palette[index] = b | (g << 8) | (r << 16) | (a << 24);
    dirtyFlag      = true;
}

void ScreenBuffer::resetDefaultColors() {
    // C64 color palette             COLOR-ID (BASIC command)
    defineColor(0, 0x00, 0x00, 0x00); //  1 Black
    defineColor(1, 0xFF, 0xFF, 0xFF); //  2 White
    defineColor(2, 0x96, 0x28, 0x2e); //  3 Red
    defineColor(3, 0x5b, 0xd6, 0xce); //  4 Cyan
    defineColor(4, 0x9f, 0x2d, 0xad); //  5 Purple
    defineColor(5, 0x41, 0xb9, 0x36); //  6 Green
    defineColor(6, 0x27, 0x24, 0xc4); //  7 Blue
    defineColor(7, 0xef, 0xf3, 0x47); //  8 Yellow
    defineColor(8, 0x9f, 0x48, 0x15); //  9 Orange
    defineColor(9, 0x5e, 0x35, 0x00); //  0 Brown
    defineColor(10, 0xda, 0x5f, 0x66); // 11 Light Red
    defineColor(11, 0x47, 0x47, 0x47); // 12 Dark Gray
    defineColor(12, 0x78, 0x78, 0x78); // 13 Medium Gray
    defineColor(13, 0x91, 0xff, 0x84); // 14 Light Green
    defineColor(14, 0x68, 0x64, 0xff); // 15 Light Blue
    defineColor(15, 0xae, 0xae, 0xae); // 16 Light Gray
}

void ScreenBuffer::resetCharmap(char32_t from, char32_t to) {
    charMap().init(from, to);
    charMap().createColorControlCodes();
}
void ScreenBuffer::defineChar(char32_t codePoint, const CharBitmap& bits) {
    dirtyFlag               = true;
    charMap().at(codePoint) = bits;
}

const CharBitmap& ScreenBuffer::getCharDefinition(char32_t codePoint) const {
    return charMap()[codePoint];
}

void ScreenBuffer::putC(char32_t c) {
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

    MEMCELL* pCh = getCursorPtr();
    *pCh         = c;
    // *cursorPosition = c;
    assertCursor();
    if (colRam) {
        colRam[idxOf(pCh)] = currentColor();
    }

    if (getCursorPos().x + 1 < width) {
        moveCursorPos(1, 0);
        // cursorPosition++;
        assertCursor();
    } else {
        softWrapToNextRow();
    }
}

void ScreenBuffer::deleteChar() {
    if (!charRam) {
        return;
    }

    dirtyFlag = true;

    auto crsr = getCursorPos();
    // size_t r = rowOf(cursorPosition);
    // size_t c = colOf(cursorPosition);

    while (true) {
        size_t srcR = crsr.y, srcC = crsr.x + 1;
        if (srcC >= width) {
            if (!rowContinues(srcR)) {
                break;
            }
            srcR++;
            srcC = 0;
        }
        MEMCELL ch = charAt(srcR, srcC);
        MEMCELL co = colorAt(srcR, srcC);
        setAt(crsr.y, crsr.x, ch, co);

        crsr.x++;
        if (crsr.x >= width) {
            if (!rowContinues(crsr.y)) {
                break;
            }
            crsr.y++;
            crsr.x = 0;
        }
    }
    setAt(crsr.y, crsr.x, blankChar, currentColor());
}

void ScreenBuffer::backspaceChar() {
    if (getCursorPtr() <= charRam) {
        return;
    }
    moveCursorPos(-1, 0);
    deleteChar();
}

// insert one space character at the cursor position
void ScreenBuffer::insertSpace() {
    if (!charRam) {
        return;
    }

    dirtyFlag = true;

    assertCursor();
    size_t tailR, tailC;
    findTailCell(getCursorPos().y, tailR, tailC);
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

        if (r == getCursorPos().y && c == getCursorPos().x) {
            break;
        }
        if (c > 0) {
            c--;
        } else {
            r--;
            c = width - 1;
        }
    }
    setAt(getCursorPos().y, getCursorPos().x, blankChar, currentColor());
}

// print '\n'
void ScreenBuffer::hardNewline() {
    size_t r = getCursorPos().y;
    // in case we're at x=0, the newline just breaks continuation of the line
    size_t c = getCursorPos().x;
    if (c == 0 && isContinuationRow(r)) {
        makeRowOwner(r);
        return;
    }

    if (r + 1 >= height) {
        scrollUpOne();
    }

    setCursorPos({ 0, std::min(r + 1, height - 1) });
    // cursorPosition = ptrAt(std::min(r + 1, height - 1), 0);
    assertCursor();
    makeRowOwner(getCursorPos().y);
}

// typed character at last screen character.
void ScreenBuffer::softWrapToNextRow() {
    size_t r = getCursorPos().y;
    if (r + 1 >= height) {
        scrollUpOne();
    }

    setCursorPos({ 0, std::min(r + 1, height - 1) });
    // cursorPosition = ptrAt(std::min(r + 1, height - 1), 0);
    assertCursor();
    makeRowContinuation(getCursorPos().y);
}

void ScreenBuffer::findTailCell(size_t startR, size_t& outR, size_t& outC) {
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

void ScreenBuffer::ensureOneCellAtTail(size_t tailR, size_t tailC) {
    if (tailC + 1 < width) {
        return;
    }
    if (tailR + 1 >= height) {
        scrollUpOne();
        --tailR;
    }
    // makeRowContinuation(std::min(tailR + 1, height - 1));
    // setAt(std::min(tailR + 1, height - 1), 0, blankChar, currentColor());

    // 1. Scroll everything down by one row (bottom row drops off)
    for (size_t r = height - 1; r > tailR + 1; --r) {
        // move characters
        for (size_t c = 0; c < width; ++c) {
            setAt(r, c, charAt(r - 1, c), colorAt(r - 1, c));
        }
        lineLinkTable[r] = lineLinkTable[r - 1]; // copy link flag
    }

    // 2. Clear the newly opened row just after tailR
    for (size_t c = 0; c < width; ++c) {
        setAt(tailR + 1, c, blankChar, currentColor());
    }

    // 3. Mark continuation flag so this row belongs to same logical line
    makeRowContinuation(tailR + 1);
}

size_t ScreenBuffer::lastUsedColumn(size_t r) const {
    for (size_t c = width; c-- > 0;) {
        if (charAt(r, c) != blankChar) {
            return c;
        }
    }
    return 0;
}

void ScreenBuffer::scrollUpOne() {
    const size_t rowCellCount = width;

    // keep in overflow buffer
    LineOverflow ovr;
    ovr.line.resize(width);
    for (size_t c = 0; c < width; ++c) {
        ovr.line[c] = charRam[c];
    }
    ovr.lineLink = lineLinkTable[0];
    overflowTop.push_back(ovr);

    ++scrollCount;
    std::memmove(charRam, charRam + rowCellCount, (height - 1) * rowCellCount * sizeof(MEMCELL));
    if (colRam) {
        std::memmove(colRam, colRam + rowCellCount, (height - 1) * rowCellCount * sizeof(MEMCELL));
    }
    MEMCELL curCol = currentColor();
    for (size_t c = 0; c < width; ++c) {
        if (!overflowBottom.empty() && c < overflowBottom.back().line.size()) {
            charRam[(height - 1) * width + c] = overflowBottom.back().line[c];
        } else {
            charRam[(height - 1) * width + c] = blankChar;
        }

        if (colRam) {
            colRam[(height - 1) * width + c] = curCol;
        }
    }

    for (size_t r = 0; r + 1 < height; ++r) {
        lineLinkTable[r] = lineLinkTable[r + 1];
    }
    makeRowOwner(height - 1);
    if (!overflowBottom.empty()) {
        lineLinkTable[height - 1] = overflowBottom.back().lineLink;
        overflowBottom.pop_back();
    }

    auto crsr  = getCursorPos();
    size_t col = std::min(crsr.x, width - 1);

    setCursorPos({ col, crsr.y - 1 });
    // cursorPosition = ptrAt(height - 1, col);
    assertCursor();
}

void ScreenBuffer::scrollDownOne() {
    const size_t rowCellCount = width;
    // keep in overflow buffer
    LineOverflow ovr;
    ovr.line.resize(width);
    for (size_t c = 0; c < width; ++c) {
        ovr.line[c] = charRam[c + ((height - 1) * rowCellCount)];
    }
    ovr.lineLink = lineLinkTable[height - 1];
    overflowBottom.push_back(ovr);

    ++scrollCount;
    std::memmove(charRam + rowCellCount, charRam, (height - 1) * rowCellCount * sizeof(MEMCELL));
    if (colRam) {
        std::memmove(colRam + rowCellCount, colRam, (height - 1) * rowCellCount * sizeof(MEMCELL));
    }
    MEMCELL curCol = currentColor();
    for (size_t c = 0; c < width; ++c) {
        if (!overflowTop.empty() && c < overflowTop.back().line.size()) {
            charRam[c] = overflowTop.back().line[c];
        } else {
            charRam[c] = blankChar;
        }
        if (colRam) {
            colRam[c] = curCol;
        }
    }

    for (size_t r = 0; r + 1 < height; ++r) {
        lineLinkTable[r] = lineLinkTable[r + 1];
    }
    makeRowOwner(height - 1);
    if (!overflowTop.empty()) {
        lineLinkTable[0] = overflowTop.back().lineLink;
        overflowTop.pop_back();
    }

    auto crsr  = getCursorPos();
    size_t col = std::min(crsr.x, width - 1);

    setCursorPos({ col, crsr.y + 1 });
    // cursorPosition = ptrAt(height - 1, col);
    assertCursor();
}

void ScreenBuffer::clearHistory() {
    overflowTop.clear();
    overflowBottom.clear();
}

// draw character pixels at given pixel position
void ScreenBuffer::drawCharPal(std::vector<uint8_t>& pixels, size_t x, size_t y, char32_t ch, uint8_t colIxText, uint8_t colIxBack, bool inverse) {
    const CharBitmap& img = charMap()[ch];

    // Calculate starting pixel index in the pixels array
    size_t pixelX = x; // * ScreenInfo::charPixX;
    size_t pixelY = y; // * ScreenInfo::charPixY;

    size_t scPixW = width * ScreenInfo::charPixX;
    // size_t scPixH = height*ScreenInfo::charPixY;

    if (img.isMono) {
        if (inverse) {
            std::swap(colIxText, colIxBack);
        }

        for (size_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.bits[row]; // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (size_t col = 0; col < ScreenInfo::charPixX; ++col) {
                bool isSet         = (pixelRow >> (7 - col)) & 1; // Extract pixel bit
                size_t pixelIndex  = (pixelY + row) * scPixW + (pixelX + col);
                pixels[pixelIndex] = isSet ? colIxText : colIxBack;
            }
        }
    } else {
        size_t nth = 0;
        for (size_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.bits[row]; // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (size_t col = 0; col < ScreenInfo::charPixX; ++col) {
                size_t pixelIndex = (pixelY + row) * scPixW + (pixelX + col);
                uint8_t colIx     = img.multi(nth++);
                if (inverse) {
                    colIx = buddyColor(colIx);
                }
                pixels[pixelIndex] = colIx;
            }
        }
    }
}

void ScreenBuffer::drawSprPal(std::vector<uint8_t>& pixels, int64_t x, int64_t y, char32_t chimg, int8_t color) {
    if (chimg == 0) {
        return;
    }
    const CharBitmap& img = charMap()[chimg];

    int64_t scPixW = width * ScreenInfo::charPixX;
    int64_t scPixH = height * ScreenInfo::charPixY;

    if (img.isMono) {
        for (int64_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.bits[row]; // Each byte represents 8 pixels in a row
            for (int64_t col = 0; col < ScreenInfo::charPixX; ++col) {
                bool isSet = (pixelRow >> (7 - col)) & 1; // Extract pixel bit
                if (!isSet) {
                    continue;
                }
                int64_t px = x + col;
                int64_t py = y + row;
                if (px < 0 || py < 0 || px >= scPixW || py >= scPixH) {
                    continue;
                }
                size_t pixelIndex  = py * scPixW + px;
                pixels[pixelIndex] = color;
            }
        }
    } else {
        size_t nth = 0;
        for (int64_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.bits[row]; // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (int64_t col = 0; col < ScreenInfo::charPixX; ++col) {
                uint8_t pxcol = img.multi(nth++);
                if (pxcol == color) {
                    continue;
                }

                int64_t px = x + col;
                int64_t py = y + row;
                if (px < 0 || py < 0 || px >= scPixW || py >= scPixH) {
                    continue;
                }
                size_t pixelIndex  = py * scPixW + px;
                pixels[pixelIndex] = pxcol;
            }
        }
    }
}

// indicate that the line wraps
void ScreenBuffer::drawLineContinuationPal(std::vector<uint8_t>& pixels, size_t yline) {
    size_t scPixW = width * ScreenInfo::charPixX;
    size_t pixelY = yline * ScreenInfo::charPixY;
    for (size_t row = 0; row < ScreenInfo::charPixY; ++row) {
        for (size_t col = 0; col < ScreenInfo::charPixX; ++col) {
            size_t pixelX     = (width - 1) * ScreenInfo::charPixX + col;
            size_t pixelIndex = (pixelY + row) * scPixW + pixelX;
            uint8_t& pcol     = pixels.at(pixelIndex);
            pcol              = buddyColor(pcol);
        }
    }
}



uint8_t ScreenBuffer::buddyColor(uint8_t colorindex) {
    //  0 Black
    //  1 White
    //  2 Red
    //  3 Cyan
    //  4 Purple
    //  5 Green
    //  6 Blue
    //  7 Yellow
    //  8 Orange
    //  9 Brown
    // 10 Light Red
    // 11 Dark Gray
    // 12 Medium Gray
    // 13 Light Green
    // 14 Light Blue
    // 15 Light Gray
    //                               0    1  2   3   4   5   6  7  8  9 10  11  12 13 14 15
    const uint8_t buddyIndex[16] = { 11, 15, 8, 13, 10, 13, 14, 8, 7, 8, 1, 12, 15, 7, 1, 1 };
    return buddyIndex[colorindex & 0x0f];
}

#if 0
void ScreenBuffer::copyWithLock(ScreenBuffer& dst, const ScreenBuffer& src) {
    src.lock.lock();
    dst.lock.lock();
    dst.dirtyFlag = true;

    ScreenBuffer::deepCopyLines(dst, src);

    dst.borderColor    = src.borderColor;
    dst.reverse        = src.reverse;
    dst.textColor      = src.textColor;
    dst.height         = src.height;
    dst.width          = src.width;
    dst.cursorPosition = src.cursorPosition;
    dst.cursorActive   = src.cursorActive;
    dst.palette        = src.palette;
    dst.sprites        = src.sprites;
    dst.windowPixels   = src.windowPixels;

    dst.lock.unlock();
    src.lock.unlock();
}


void ScreenBuffer::deepCopyLines(ScreenBuffer& dest, const ScreenBuffer& src) {
    memcpy(dest.charRam, src.charRam, sizeof(charRam[0]) * src.width * src.height);
    memcpy(dest.colRam, src.colRam, sizeof(colRam[0]) * src.width * src.height);
    memcpy(dest.lineLinkTable, src.lineLinkTable, sizeof(lineLinkTable[0]) * src.height);
}
#endif
