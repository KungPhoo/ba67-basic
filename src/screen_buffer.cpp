#include "screen_buffer.h"
#include "font.h"
#include <algorithm>
#include <stdexcept>


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
    size_t n  = width * height;
    for (size_t i = 0; i < n; ++i) {
        charRam[i] = U' ';
        colRam[i]  = currentColor();
    }
    memset(lineLinkTable, 0, sizeof(lineLinkTable[0]) * height);
    cursorPosition = charRam;
}

// blank out current line, reset line link table flag
void ScreenBuffer::cleanCurrentLine() {
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

void ScreenBuffer::moveCursorPos(int dx, int dy) {
    dirtyFlag       = true;
    MEMCELL* newPos = cursorPosition + (dy * width + dx);
    if (newPos < charRam) {
        auto crsr = getCursorPos();
        crsr.y    = 0;
        setCursorPos(crsr);
        return;
    }

    if (newPos >= charRam + width * height) {
        scrollUpOne();
        cursorPosition = newPos;
        auto crsr      = getCursorPos();
        crsr.y         = height - 1;
        setCursorPos(crsr);
        return;
    }
    cursorPosition = newPos;
}

ScreenBuffer::ScreenBuffer() {
    resize(40, 25);

    resetDefaultColors();
    setColors(1, 0);
    borderColor = 1;

    charMap(); // load bitmaps
    // resetCharmap();

    // clear();
}

void ScreenBuffer::updateScreenPixelsPalette(bool highlightCursor, std::vector<uint8_t>& pixelsPal) {
    // chars
    MEMCELL* visibleCursorPosition = cursorPosition;
    if (!highlightCursor || !cursorActive) {
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
            if (lineLinkTable[y + 1] & 0x80) {
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


void ScreenBuffer::setColors(uint8_t text, uint8_t back) {
    textColor = (text & 0x0f) | ((back << 4) & 0xf0);
}

void ScreenBuffer::setTextColor(int index) {
    setColors(index, textColor >> 4);
}

void ScreenBuffer::setBackgroundColor(int index) {
    setColors(textColor & 0x0f, index);
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
