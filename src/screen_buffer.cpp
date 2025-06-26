#include "screen_buffer.h"
#include "font.h"
#include <algorithm>
#include <stdexcept>

CharMap::CharMap()
    : ascii {} {
    unicode = new std::unordered_map<char32_t, CharBitmap>();
    Font::createCharmap(*this);
    createColorControlCodes();
}

void CharMap::createColorControlCodes() {
    // create multicolor "change color" characters
    std::array<char32_t, 16> ctrls = {
        0x90, // Black
        0x05, // White
        0x1c, // Red
        0x9f, // Cyan
        0x9c, // Purple
        0x1e, // Green
        0x1f, // Blue
        0x9e, // Yellow
        0x81, // Orange
        0x95, // Brown
        0x96, // Light Red
        0x97, // Dark Gray
        0x98, // Medium Gray
        0x99, // Light Green
        0x9a, // Light Blue
        0x9b // Light Gray
    };

    uint8_t cc = 0;
    for (char32_t c : ctrls) {
        uint8_t bb = 11;
        uint8_t ff = 1;
        uint8_t mm = 15;
        if (cc == 15) {
            mm = 1;
        }
        CharBitmap& bm = at(c);
        uint8_t cols[] = {
            bb, ff, ff, ff, ff, ff, ff, bb,
            ff, cc, cc, cc, cc, cc, cc, ff,
            ff, ff, cc, cc, cc, ff, ff, ff,
            ff, mm, cc, mm, cc, mm, mm, ff,
            ff, mm, mm, mm, cc, mm, mm, ff,
            ff, mm, mm, mm, mm, mm, mm, ff,
            ff, mm, mm, mm, mm, mm, mm, ff,
            bb, ff, ff, ff, ff, ff, ff, bb
        };
        bm.isMono = false;
        for (int i = 0; i < 64; ++i) {
            bm.setMulti(i, cols[i]);
        }
        ++cc;
    }
}

static CharMap& charMap() {
    static CharMap c;
    return c;
}

ScreenBuffer::ScreenBuffer()
    : cursor {} {
    resize(maxWidth, maxHeight);

    resetDefaultColors();
    setColors(1, 0);
    borderColor = 1;

    charMap(); // load bitmaps
}

void ScreenBuffer::clear() {
    cursor = {};
    for (auto& ln : lines) {
        for (auto& c : ln->cols) {
            c.ch  = U'\0';
            c.col = color;
        }
        ln->wrapps = false;
    }
}

void ScreenBuffer::putC(char32_t c) {
    dirtyFlag = true;

    if (c == U'\b') {
        backspaceChar();
        return;
    }

    if (c == U'\n') {
        // hard terminate the current line to avoid trailing spaces
        // when inserting a character
        lines[cursor.y]->cols[cursor.x].ch = U'\0';

        // here's the magic in printing 40 chars per line w/o wrapping
        // print 40 chars, then print '\n'.
        if (cursor.y > 0 && cursor.x == 0 && lines[cursor.y - 1]->wrapps) {
            lines[cursor.y - 1]->wrapps = false;
            return;
        }

        cursor.y++;
        cursor.x = 0;
        manageOverflow();
        return;
    }

    SChar sc;
    sc.ch  = c;
    sc.col = color;

    lines[cursor.y]->cols[cursor.x] = sc;
    ++cursor.x;

    if (cursor.x >= width) {
        lines[cursor.y]->wrapps = true;
        cursor.y++;
        cursor.x = 0;
        manageOverflow();
    }
}

void ScreenBuffer::defineChar(char32_t codePoint, const CharBitmap& bits) {
    dirtyFlag               = true;
    charMap().at(codePoint) = bits;
}

const CharBitmap& ScreenBuffer::getCharDefinition(char32_t codePoint) const {
    return charMap()[codePoint];
}

void ScreenBuffer::deleteChar() {
    dirtyFlag = true;

    lines[height - 1]->wrapps = false;

    size_t xstart = cursor.x;
    for (size_t y = cursor.y; y < height; ++y) {
        auto ln = lines[y];
        for (size_t x = xstart; x + 1 < width; ++x) {
            ln->cols[x] = ln->cols[x + 1];
            if (ln->cols[x].ch == U'\0') {
                return;
            }
        }
        if (ln->wrapps) {
            ln->cols[width - 1] = lines[cursor.y + 1]->cols[0];
            if (ln->cols[width - 1].ch == U'\0') {
                ln->wrapps = false;
                return;
            }
            xstart = 0;
        } else {
            ln->cols[width - 1] = { U'\0', color };
            break; // y
        }
    }
}

void ScreenBuffer::backspaceChar() {
    if (cursor.x == 0 && cursor.y == 0) {
        return;
    }
    moveCursorPos(-1, 0);
    deleteChar();
}

void ScreenBuffer::insertSpace() {
    dirtyFlag = true;
    SChar sc { ' ', color }, nxt;

    Cursor cr = cursor;
    while (cr.y < height) {
        nxt                     = lines[cr.y]->cols[cr.x];
        lines[cr.y]->cols[cr.x] = sc;
        if (nxt.ch == U'\0') {
            break;
        }

        if (cr.x + 1 < width) {
            ++cr.x;
        } else {
            lines[cr.y]->wrapps = true;
            if (lines[cr.y]->wrapps) {
                cr.x = 0;
                ++cr.y;
                if (cr.y >= height) {
                    return;
                }
            } else {
                break;
            }
        }
        sc = nxt;
    }
}

void ScreenBuffer::cleanCurrentLine() {
    size_t y = cursor.y;
    if (y < height) {
        lines[y]->clear();
    }
}

ScreenBuffer::Cursor ScreenBuffer::getCursorPos() const {
    return cursor;
}

const ScreenBuffer::Cursor& ScreenBuffer::setCursorPos(Cursor crsr) {
    cursor = crsr;
    if (cursor.x >= width) {
        cursor.x = 0;
        ++cursor.y;
    }
    while (cursor.y >= height) {
        dropFirstLine();
    }

    dirtyFlag = true;
    return cursor;
}

const ScreenBuffer::Cursor& ScreenBuffer::moveCursorPos(int dx, int dy) {
    // vertical
    if (dy < 0 && cursor.y < -dy) {
        cursor.y = 0;
    } else {
        cursor.y += dy;
    }

    // left
    while (dx < 0 && cursor.x < -dx) {
        ++dx;
        if (cursor.x > 0) {
            --cursor.x;
        } else {
            if (cursor.y > 0) {
                cursor.x = width - 1;
                --cursor.y;
            }
        }
    }
    // right
    cursor.x += dx;
    if (cursor.x >= width) {
        cursor.x = 0;
        ++cursor.y;
    }

    // down
    while (cursor.y >= height) {
        dropFirstLine();
    }

    // ensure there's no '\0' left of the cursor
    auto& ln = lines[cursor.y];
    for (size_t x = 0; x < cursor.x; ++x) {
        if (ln->cols[x].ch == '\0') {
            ln->cols[x].ch  = U' ';
            ln->cols[x].col = color;
        }
    }

    dirtyFlag = true;
    return cursor;
}

ScreenBuffer::Cursor ScreenBuffer::getStartOfLineAt(Cursor crsr) {
    Cursor cr(cursor);
    cr.x = 0;
    while (cr.y > 0 && lines[cr.y - 1]->wrapps) {
        --cr.y;
    }

    return cr;
}

ScreenBuffer::Cursor ScreenBuffer::getEndOfLineAt(Cursor crsr) {
    Cursor lastNonSpace       = crsr;
    lines[height - 1]->wrapps = false;
    for (;;) {
        if (crsr.x + 1 >= width) {
            if (lines[crsr.y]->wrapps) {
                if (crsr.y + 1 >= height) {
                    return lastNonSpace;
                }
                crsr.x = 0;
                ++crsr.y;
            } else {
                crsr.x = width - 1;
                return lastNonSpace;
            }
        }
        auto& sc = lines[crsr.y]->cols[crsr.x];
        if (sc.ch == U'\0') {
            return lastNonSpace;
        }

        if (sc.ch != U' ' && sc.ch != U'\r' && sc.ch != U'\n' && sc.ch != U'\0') {
            lastNonSpace = crsr;
        }

        ++crsr.x;
    }
    return lastNonSpace; // {width - 1, height - 1};
}

// get buffer as a width * height character buffer
/*
void ScreenBuffer::getPrintBuffer(std::u32string& chars, std::string& colors) const {
    chars.clear(); colors.clear();
    chars.resize(width * height); colors.resize(width * height);
    char lastColor = char(color);

    size_t pos = 0;
    for (size_t y = 0; y < height; ++y) {
        bool endReached = false;
        for (size_t x = 0; x < width; ++x) {
            auto& sc = lines[y]->cols[x];
            if (sc.ch == U'\0') {
                chars[pos] = U' ';
                colors[pos] = lastColor;
            } else {
                chars[pos] = sc.ch;
                colors[pos] = sc.col;
                lastColor = sc.col;
            }
            ++pos;
        }
    }
}
*/

void ScreenBuffer::updateScreenPixelsPalette() {
#pragma omp for
    for (int y = 0; y < int(height); ++y) {
        uint8_t cl = 1, lastColor = 1;
        auto& ln  = lines[y];
        lastColor = this->color;
        for (size_t x = 0; x < width; ++x) {
            auto sc = ln->cols[x];
            if (sc.ch == U'\0') {
                sc.ch  = U' ';
                sc.col = this->color;
            } else {
                sc.col    = sc.col;
                lastColor = sc.col;
            }
            drawCharPal(x * ScreenInfo::charPixX, y * ScreenInfo::charPixY, sc.ch, sc.col & 0x0f, (sc.col >> 4) & 0x0f);
        }
        if (ln->wrapps) {
            drawLineContinuationPal(y);
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
        drawSprPal(sp.x + 0 - sprbx, sp.y + sprby, sp.charmap[0], sp.color);
        drawSprPal(sp.x + 8 - sprbx, sp.y + sprby, sp.charmap[1], sp.color);
        drawSprPal(sp.x + 16 - sprbx, sp.y + sprby, sp.charmap[2], sp.color);
        drawSprPal(sp.x + 0 - sprbx, sp.y + 8 + sprby, sp.charmap[3], sp.color);
        drawSprPal(sp.x + 8 - sprbx, sp.y + 8 + sprby, sp.charmap[4], sp.color);
        drawSprPal(sp.x + 16 - sprbx, sp.y + 8 + sprby, sp.charmap[5], sp.color);
    }
}

// update the RGB buffer
// call this after updateScreenPixels()
void ScreenBuffer::updateScreenBitmap() {
    size_t n = screenBitmap.pixelsRGB.size();
    size_t p = 0, irgb = 0;
    uint8_t* pal  = &screenBitmap.pixelsPal[0];
    uint32_t* rgb = &screenBitmap.pixelsRGB[0];
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
    if (end < start) {
        std::swap(start, end);
    }
    std::u32string mid;

    size_t startx = start.x, endx = width - 1;
    if (end.y == start.y) {
        endx = end.x;
    }
    for (size_t y = start.y; y <= end.y; ++y) {
        for (size_t x = startx; x <= endx; ++x) {
            auto& sc = lines[y]->cols[x];
            if (sc.ch == U'\0') {
                startx = 0;
                if (y + 1 == end.y) {
                    endx = end.x;
                }
                break;
            }
#if 0 //  _DEBUG
      // inverse colors for debugging
            sc.col = (sc.col << 4) | (sc.col >> 4);
#endif
            mid += sc.ch;
        }
    }

    while (mid.length() > 0 && mid[0] == '\0') {
        mid.erase(mid.begin());
    }

    return mid;
}

void ScreenBuffer::setColors(uint8_t text, uint8_t back) {
    color = (text & 0x0f) | ((back << 4) & 0xf0);
}

void ScreenBuffer::setTextColor(int index) {
    setColors(index, color >> 4);
}

void ScreenBuffer::setBackgroundColor(int index) {
    setColors(color & 0x0f, index);
}

void ScreenBuffer::inverseColours() {
    color = ((color & 0xf) << 4) | ((color >> 4) & 0x0f);
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
    Font::createCharmap(charMap(), from, to);
    charMap().createColorControlCodes();
}

void ScreenBuffer::copyWithLock(ScreenBuffer& dst, const ScreenBuffer& src) {
    src.lock.lock();
    dst.lock.lock();
    dst.dirtyFlag = true;

    ScreenBuffer::deepCopyLines(dst.lines, src.lines);

    dst.borderColor  = src.borderColor;
    dst.color        = src.color;
    dst.height       = src.height;
    dst.width        = src.width;
    dst.cursor       = src.cursor;
    dst.palette      = src.palette;
    dst.sprites      = src.sprites;
    dst.windowPixels = src.windowPixels;

    dst.lock.unlock();
    src.lock.unlock();
}

void ScreenBuffer::resize(size_t w, size_t h) {
    lines.resize(h);
    for (size_t y = 0; y < h; ++y) {
        if (lines[y] == nullptr) {
            lines[y] = std::make_shared<Line>();
        }
        lines[y]->cols.resize(w + 1, { U'\0', color });
    }
    // width=w; height=h;
}

void ScreenBuffer::deepCopyLines(std::vector<std::shared_ptr<Line>>& dest, const std::vector<std::shared_ptr<Line>>& src) {
    // Resize destination to match source
    if (dest.size() > src.size()) {
        dest.resize(src.size()); // Shrink if needed
    }

    for (size_t i = 0; i < src.size(); ++i) {
        if (i >= dest.size()) {
            // Create new Line if dest is smaller
            dest.push_back(std::make_shared<Line>());
        }
        if (!dest[i]) {
            // Allocate if it's nullptr
            dest[i] = std::make_shared<Line>();
        }

        // Copy contents from src[i] to dest[i] (deep copy)
        dest[i]->cols   = src[i]->cols; // Copies the vector
        dest[i]->wrapps = src[i]->wrapps;
    }
}

void ScreenBuffer::dropFirstLine() {
    dirtyFlag = true;
    for (size_t y = 1; y < height; ++y) {
        std::swap(lines[y - 1], lines[y]);
    }
    lines[height - 1]->clear();

    if (cursor.y > 0) {
        --cursor.y;
    }
    ++scrollCount;
}

void ScreenBuffer::manageOverflow() {
    while (cursor.y >= height) {
        dropFirstLine();
    }
}

// draw character pixels at given pixel position
void ScreenBuffer::drawCharPal(size_t x, size_t y, char32_t ch, uint8_t colIxText, uint8_t colIxBack) {
    const CharBitmap& img = charMap()[ch];

    // Calculate starting pixel index in the pixel array
    size_t pixelX = x; // * ScreenInfo::charPixX;
    size_t pixelY = y; // * ScreenInfo::charPixY;
    auto& pixels  = screenBitmap.pixelsPal;

    if (img.isMono) {
        for (size_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.bits[row]; // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (size_t col = 0; col < ScreenInfo::charPixX; ++col) {
                bool isSet         = (pixelRow >> (7 - col)) & 1; // Extract pixel bit
                size_t pixelIndex  = (pixelY + row) * (ScreenInfo::pixX) + (pixelX + col);
                pixels[pixelIndex] = isSet ? colIxText : colIxBack;
            }
        }
    } else {
        size_t nth = 0;
        for (size_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.bits[row]; // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (size_t col = 0; col < ScreenInfo::charPixX; ++col) {
                size_t pixelIndex  = (pixelY + row) * (ScreenInfo::pixX) + (pixelX + col);
                pixels[pixelIndex] = img.multi(nth++);
            }
        }
    }
}

void ScreenBuffer::drawSprPal(int64_t x, int64_t y, char32_t chimg, int8_t color) {
    if (chimg == 0) {
        return;
    }
    const CharBitmap& img = charMap()[chimg];

    auto& pixels = screenBitmap.pixelsPal;

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
                if (px < 0 || py < 0 || px >= ScreenInfo::pixX || py >= ScreenInfo::pixY) {
                    continue;
                }
                size_t pixelIndex  = py * ScreenInfo::pixX + px;
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
                if (px < 0 || py < 0 || px >= ScreenInfo::pixX || py >= ScreenInfo::pixY) {
                    continue;
                }
                size_t pixelIndex  = py * ScreenInfo::pixX + px;
                pixels[pixelIndex] = pxcol;
            }
        }
    }
}

// indicate that the line wraps
void ScreenBuffer::drawLineContinuationPal(size_t yline) {
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

    auto& pixels  = screenBitmap.pixelsPal;
    size_t pixelY = yline * ScreenInfo::charPixY;
    for (size_t row = 0; row < ScreenInfo::charPixY; ++row) {
        for (size_t col = 0; col < ScreenInfo::charPixX; ++col) {
            size_t pixelX     = width * ScreenInfo::charPixX - ScreenInfo::charPixX + col;
            size_t pixelIndex = (pixelY + row) * (ScreenInfo::pixX) + pixelX;
            uint8_t& pcol     = pixels.at(pixelIndex);
            pcol              = buddyIndex[pcol];
        }
    }
}
