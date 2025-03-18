#include "screen_buffer.h"
#include "font.h"
#include <stdexcept>
#include <algorithm>

CharMap::CharMap()
    : ascii{}, unicode{} {
    Font::createCharmap(*this);
}

static CharMap& charMap() {
    static CharMap c;
    return c;
}

ScreenBuffer::ScreenBuffer()
    : cursor{} {
    resize(width, height);

    resetDefaultColors();
    setColors(1, 0);
    borderColor = 1;

    charMap();  // load bitmaps
}

void ScreenBuffer::clear() {
    cursor = {};
    for (auto& ln : lines)
    {
        for (auto& c : ln->cols)
        {
            c.ch = U'\0';
            c.col = color;
        }
        ln->wrapps = false;
    }
}

void ScreenBuffer::putC(char32_t c) {
    dirtyFlag = true;

    if (c == U'\b')
    {
        backspaceChar();
        return;
    }

    if (c == U'\n')
    {
        cursor.y++;
        cursor.x = 0;
        manageOverflow();
        return;
    }

    SChar sc;
    sc.ch = c;
    sc.col = color;

    lines[cursor.y]->cols[cursor.x] = sc;
    ++cursor.x;

    if (cursor.x >= width)
    {
        lines[cursor.y]->wrapps = true;
        cursor.y++;
        cursor.x = 0;
        manageOverflow();
    }
}

void ScreenBuffer::defineChar(char32_t codePoint, const CharBitmap& bits) {
    dirtyFlag = true;
    charMap()[codePoint] = bits;
}

void ScreenBuffer::deleteChar() {
    dirtyFlag = true;

    lines[height - 1]->wrapps = false;

    size_t xstart = cursor.x;
    for (size_t y = cursor.y; y < height; ++y)
    {
        auto ln = lines[y];
        for (size_t x = xstart; x + 1 < width; ++x)
        {
            ln->cols[x] = ln->cols[x + 1];
            if (ln->cols[x].ch == U'\0') { return; }
        }
        if (ln->wrapps)
        {
            ln->cols[width - 1] = lines[cursor.y + 1]->cols[0];
            if (ln->cols[width - 1].ch == U'\0')
            {
                ln->wrapps = false;
                return;
            }
            xstart = 0;
        }
        else
        {
            ln->cols[width - 1] = {U'\0', color};
            break;  // y
        }
    }
}

void ScreenBuffer::backspaceChar() {
    if (cursor.x == 0 && cursor.y == 0) { return; }
    moveCursorPos(-1, 0);
    deleteChar();
}

void ScreenBuffer::insertSpace() {
    dirtyFlag = true;
    SChar sc{' ', color}, nxt;

    Cursor cr = cursor;
    while (cr.y < height)
    {
        nxt = lines[cr.y]->cols[cr.x];
        lines[cr.y]->cols[cr.x] = sc;
        if (nxt.ch == U'\0') { break; }

        if (cr.x + 1 < width)
        {
            ++cr.x;
        }
        else
        {
            lines[cr.y]->wrapps = true;
            if (lines[cr.y]->wrapps)
            {
                cr.x = 0;
                ++cr.y;
                if (cr.y >= height) { return; }
            }
            else { break; }
        }
        sc = nxt;
    }
}

ScreenBuffer::Cursor ScreenBuffer::getCursorPos() const {
    return cursor;
}

const ScreenBuffer::Cursor& ScreenBuffer::setCursorPos(Cursor crsr) {
    cursor = crsr;
    if (cursor.x >= width)
    {
        cursor.x = 0;
        ++cursor.y;
    }
    while (cursor.y >= height) { dropFirstLine(); }

    dirtyFlag = true;
    return cursor;
}

const ScreenBuffer::Cursor& ScreenBuffer::moveCursorPos(int dx, int dy) {
    // vertical
    if (dy < 0 && cursor.y < -dy) { cursor.y = 0; }
    else
    {
        cursor.y += dy;
    }

    // left
    while (dx < 0 && cursor.x < -dx)
    {
        ++dx;
        if (cursor.x > 0) { --cursor.x; }
        else
        {
            if (cursor.y > 0)
            {
                cursor.x = width - 1;
                --cursor.y;
            }
        }
    }
    // right
    cursor.x += dx;
    if (cursor.x >= width)
    {
        cursor.x = 0;
        ++cursor.y;
    }

    // down
    while (cursor.y >= height) { dropFirstLine(); }

    // ensure there's no '\0' left of the cursor
    auto& ln = lines[cursor.y];
    for (size_t x = 0; x < cursor.x; ++x)
    {
        if (ln->cols[x].ch == '\0')
        {
            ln->cols[x].ch = U' ';
            ln->cols[x].col = color;
        }
    }

    dirtyFlag = true;
    return cursor;
}

ScreenBuffer::Cursor ScreenBuffer::getStartOfLineAt(Cursor crsr) {
    Cursor cr(cursor);
    cr.x = 0;
    while (cr.y > 0 && lines[cr.y - 1]->wrapps) { --cr.y; }

    return cr;
}

ScreenBuffer::Cursor ScreenBuffer::getEndOfLineAt(Cursor crsr) {
    lines[height - 1]->wrapps = false;
    for (;;)
    {
        auto& sc = lines[crsr.y]->cols[crsr.x];
        if (sc.ch == U'\0') { return crsr; }
        if (crsr.x + 1 >= width)
        {
            if (lines[crsr.y]->wrapps)
            {
                if (crsr.y + 1 >= height) { return crsr; }
                crsr.x = 0;
                ++crsr.y;
            }
        }
        ++crsr.x;
    }
    return {width - 1, height - 1};
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
    // static std::u32string chars;
    // static std::string colors;
    // getPrintBuffer(chars, colors);
    // #pragma omp for
    // for (int i = 0; i < chars.length(); ++i) {
    //     size_t x = i % width;
    //     size_t y = i / width;
    //     char32_t unicodeCodepoint = chars[i];
    //     uint8_t color = colors[i];
    //     drawCharPal(x * ScreenInfo::charPixX, y * ScreenInfo::charPixY, unicodeCodepoint, color & 0x0f, (color >> 4) & 0x0f);
    // }

#pragma omp for
    for (int y = 0; y < int(height); ++y)
    {
        uint8_t cl = 1, lastColor = 1;
        auto& ln = lines[y];
        lastColor = this->color;
        for (size_t x = 0; x < width; ++x)
        {
            auto sc = ln->cols[x];
            if (sc.ch == U'\0')
            {
                sc.ch = U' ';
                sc.col = lastColor;
            }
            else
            {
                sc.col = sc.col;
                lastColor = sc.col;
            }
            drawCharPal(x * ScreenInfo::charPixX, y * ScreenInfo::charPixY, sc.ch, sc.col & 0x0f, (sc.col >> 4) & 0x0f);
        }
#if _DEBUG
        if (ln->wrapps)
        {
            drawCharPal(ScreenInfo::pixX - ScreenInfo::charPixX, y * ScreenInfo::charPixY, U'~', 7, 0);
        }
#endif
    }

    // sprites
    for (auto& sp : sprites)
    {
        if (!sp.enabled) { continue; }

        drawSprPal(sp.x + 0, sp.y, sp.charmap[0], sp.color);
        drawSprPal(sp.x + 8, sp.y, sp.charmap[1], sp.color);
        drawSprPal(sp.x + 16, sp.y, sp.charmap[2], sp.color);
        drawSprPal(sp.x + 0, sp.y + 8, sp.charmap[3], sp.color);
        drawSprPal(sp.x + 8, sp.y + 8, sp.charmap[4], sp.color);
        drawSprPal(sp.x + 16, sp.y + 8, sp.charmap[5], sp.color);
    }

    // size_t x = 0, y = 0;
    // int32_t ctext = 1, cback = 0;
    // for (size_t ic = 0; ; ++ic) {
    //     size_t i = ic; if (ic >= chars.length()) { i = chars.length() - 1; }
    //     if (x == width) {
    //         x = 0;
    //         ++y;
    //         if (y > height) {
    //             break;
    //         }
    //     }
    //
    //     char32_t unicodeCodepoint = chars[i];
    //
    //     uint8_t color = colors[i];
    //     drawCharPal(x, y, unicodeCodepoint, color & 0x0f, (color >> 4) & 0x0f);
    //     ++x;
    // }
}

// update the RGB buffer
// call this after updateScreenPixels()
void ScreenBuffer::updateScreenBitmap() {
    size_t n = screenBitmap.pixelsRGB.size();
    size_t p = 0, irgb = 0;
    uint8_t* pal = &screenBitmap.pixelsPal[0];
    uint32_t* rgb = &screenBitmap.pixelsRGB[0];
    while (n > 0)
    {
        --n;
        *rgb++ = palette[*pal++];
        ++p;
        if (p == 8) { p = 0; }
    }
}

std::u32string ScreenBuffer::getSelectedText(Cursor start, Cursor end) const {
    if (end < start) { std::swap(start, end); }
    std::u32string mid;

    size_t startx = start.x, endx = width - 1;
    if (end.y == start.y) { endx = end.x; }
    for (size_t y = start.y; y <= end.y; ++y)
    {
        for (size_t x = startx; x <= endx; ++x)
        {
            auto& sc = lines[y]->cols[x];
            if (sc.ch == U'\0')
            {
                startx = 0;
                if (y + 1 == end.y) { endx = end.x; }
                break;
            }
#if _DEBUG
            // inverse colors for debugging
            sc.col = (sc.col << 4) | (sc.col >> 4);
#endif
            mid += sc.ch;
        }
    }

    while (mid.length() > 0 && mid[0] == '\0')
    {
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
    if (index > 15) { return; }
    palette[index] = r | (g << 8) | (b << 16) | (a << 24);
    dirtyFlag = true;
}

void ScreenBuffer::resetDefaultColors() {
    // C64 color palette
    defineColor(0, 0x00, 0x00, 0x00);   // Black
    defineColor(1, 0xFF, 0xFF, 0xFF);   // White
    defineColor(2, 0x96, 0x28, 0x2e);   // Red
    defineColor(3, 0x9f, 0x2d, 0xad);   // Cyan
    defineColor(4, 0x5b, 0xd6, 0xce);   // Purple
    defineColor(5, 0x41, 0xb9, 0x36);   // Green
    defineColor(6, 0x27, 0x24, 0xc4);   // Blue
    defineColor(7, 0xef, 0xf3, 0x47);   // Yellow
    defineColor(8, 0x9f, 0x48, 0x15);   // Orange
    defineColor(9, 0x5e, 0x35, 0x00);   // Brown
    defineColor(10, 0xda, 0x5f, 0x66);  // Light Red
    defineColor(11, 0x47, 0x47, 0x47);  // Dark Gray
    defineColor(12, 0x78, 0x78, 0x78);  // Medium Gray
    defineColor(13, 0x91, 0xff, 0x84);  // Light Green
    defineColor(14, 0x68, 0x64, 0xff);  // Light Blue
    defineColor(15, 0xae, 0xae, 0xae);  // Light Gray
}

void ScreenBuffer::resetCharmap(char32_t from, char32_t to) {
    Font::createCharmap(charMap(), from, to);
}

void ScreenBuffer::copyWithLock(ScreenBuffer& dst, const ScreenBuffer& src) {
    src.lock.lock();
    dst.lock.lock();
    dst.dirtyFlag = true;

    ScreenBuffer::deepCopyLines(dst.lines, src.lines);

    dst.borderColor = src.borderColor;
    dst.color = src.color;
    // dst.height = src.height;
    // dst.width = src.width;
    dst.cursor = src.cursor;
    dst.palette = src.palette;
    dst.sprites = src.sprites;

    dst.lock.unlock();
    src.lock.unlock();
}

void ScreenBuffer::resize(size_t w, size_t h) {
    lines.resize(h);
    for (size_t y = 0; y < h; ++y)
    {
        if (lines[y] == nullptr)
        {
            lines[y] = std::make_shared<Line>();
        }
        lines[y]->cols.resize(w, {U'\0', color});
    }
    // width=w; height=h;
}

void ScreenBuffer::deepCopyLines(std::vector<std::shared_ptr<Line>>& dest, const std::vector<std::shared_ptr<Line>>& src) {
    // Resize destination to match source
    if (dest.size() > src.size())
    {
        dest.resize(src.size());  // Shrink if needed
    }

    for (size_t i = 0; i < src.size(); ++i)
    {
        if (i >= dest.size())
        {
            // Create new Line if dest is smaller
            dest.push_back(std::make_shared<Line>());
        }
        if (!dest[i])
        {
            // Allocate if it's nullptr
            dest[i] = std::make_shared<Line>();
        }

        // Copy contents from src[i] to dest[i] (deep copy)
        dest[i]->cols = src[i]->cols;  // Copies the vector
        dest[i]->wrapps = src[i]->wrapps;
    }
}

void ScreenBuffer::dropFirstLine() {
    dirtyFlag = true;
    for (size_t y = 1; y < height; ++y)
    {
        std::swap(lines[y - 1], lines[y]);
    }
    lines[height - 1]->clear();

    if (cursor.y > 0) { --cursor.y; }
}

void ScreenBuffer::manageOverflow() {
    while (cursor.y >= height) { dropFirstLine(); }
}

// draw character pixels at given pixel position
void ScreenBuffer::drawCharPal(size_t x, size_t y, char32_t ch, uint8_t colIxText, uint8_t colIxBack) {
    const CharBitmap& img = charMap()[ch];

    // Calculate starting pixel index in the pixel array
    size_t pixelX = x;  // * ScreenInfo::charPixX;
    size_t pixelY = y;  // * ScreenInfo::charPixY;
    auto& pixels = screenBitmap.pixelsPal;

    if (img.isMono)
    {
        for (size_t row = 0; row < ScreenInfo::charPixY; ++row)
        {
            uint8_t pixelRow = img.monoPixels[row];  // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (size_t col = 0; col < ScreenInfo::charPixX; ++col)
            {
                bool isSet = (pixelRow >> (7 - col)) & 1;  // Extract pixel bit
                size_t pixelIndex = (pixelY + row) * (ScreenInfo::pixX) + (pixelX + col);
                pixels[pixelIndex] = isSet ? colIxText : colIxBack;
            }
        }
    }
    else
    {
        size_t nth = 0;
        for (size_t row = 0; row < ScreenInfo::charPixY; ++row)
        {
            uint8_t pixelRow = img.monoPixels[row];  // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (size_t col = 0; col < ScreenInfo::charPixX; ++col)
            {
                size_t pixelIndex = (pixelY + row) * (ScreenInfo::pixX) + (pixelX + col);
                pixels[pixelIndex] = img.multi(nth++);
            }
        }
    }
}

void ScreenBuffer::drawSprPal(int64_t x, int64_t y, char32_t chimg, int8_t color) {
    if (chimg == 0) { return; }
    const CharBitmap& img = charMap()[chimg];

    auto& pixels = screenBitmap.pixelsPal;

    if (img.isMono)
    {
        for (int64_t row = 0; row < ScreenInfo::charPixY; ++row)
        {
            uint8_t pixelRow = img.monoPixels[row];  // Each byte represents 8 pixels in a row
            for (int64_t col = 0; col < ScreenInfo::charPixX; ++col)
            {
                bool isSet = (pixelRow >> (7 - col)) & 1;  // Extract pixel bit
                if (!isSet) { continue; }
                int64_t px = x + col;
                int64_t py = y + row;
                if (px < 0 || py < 0 || px >= ScreenInfo::pixX || py >= ScreenInfo::pixY) { continue; }
                size_t pixelIndex = py * ScreenInfo::pixX + px;
                pixels[pixelIndex] = color;
            }
        }
    }
    else
    {
        size_t nth = 0;
        for (int64_t row = 0; row < ScreenInfo::charPixY; ++row)
        {
            uint8_t pixelRow = img.monoPixels[row];  // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (int64_t col = 0; col < ScreenInfo::charPixX; ++col)
            {
                uint8_t pxcol = img.multi(nth++);
                if (pxcol == color) { continue; }

                int64_t px = x + col;
                int64_t py = y + row;
                if (px < 0 || py < 0 || px >= ScreenInfo::pixX || py >= ScreenInfo::pixY) { continue; }
                size_t pixelIndex = py * ScreenInfo::pixX + px;
                pixels[pixelIndex] = pxcol;
            }
        }
    }
}
