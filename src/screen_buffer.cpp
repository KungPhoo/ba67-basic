#include "screen_buffer.h"
#include "font.h"
#include <algorithm>

CharMap::CharMap(): ascii{}, unicode{} {
    Font::createCharmap(*this);
}


static CharMap& charMap() {
    static CharMap c;
    return c;
}

ScreenBuffer::ScreenBuffer(): cursorPos(0) {
    // C64 color palette
    defineColor(0, 0x00, 0x00, 0x00); // Black
    defineColor(1, 0xFF, 0xFF, 0xFF); // White
    defineColor(2, 0x96, 0x28, 0x2e); // Red
    defineColor(3, 0x9f, 0x2d, 0xad); // Cyan
    defineColor(4, 0x5b, 0xd6, 0xce); // Purple
    defineColor(5, 0x41, 0xb9, 0x36); // Green
    defineColor(6, 0x27, 0x24, 0xc4); // Blue
    defineColor(7, 0xef, 0xf3, 0x47); // Yellow
    defineColor(8, 0x9f, 0x48, 0x15); // Orange
    defineColor(9, 0x5e, 0x35, 0x00); // Brown
    defineColor(10, 0xda, 0x5f, 0x66); // Light Red
    defineColor(11, 0x47, 0x47, 0x47); // Dark Gray
    defineColor(12, 0x78, 0x78, 0x78); // Medium Gray
    defineColor(13, 0x91, 0xff, 0x84); // Light Green
    defineColor(14, 0x68, 0x64, 0xff); // Light Blue
    defineColor(15, 0xae, 0xae, 0xae); // Light Gray
    setColors(1, 0);
    borderColor = 1;

    charMap(); // load bitmaps
}

void ScreenBuffer::clear() {
    buffer.clear();
    cursorPos = 0;
}

void ScreenBuffer::putC(char32_t c) {
    verifyPosition();

    SChar sc;
    sc.ch = c;
    sc.col = color;

    if (c == U'\b') { backspaceChar(); return; }

    // if (c == U'\n') {
    //     // newline at last line? scroll one line up
    //     auto crsr = getCursorPos();
    //     if (crsr.y + 1 >= height) {
    //         dropFirstLine();
    //         verifyPosition();
    //         crsr.y = height - 2;
    //         setCursorPos(crsr);
    //         verifyPosition();
    //     }
    // }

    if (cursorPos >= buffer.length()) {
        buffer += sc;
        cursorPos = buffer.length();
    } else {
        if (buffer.at(cursorPos).ch == U'\n') { // append to end of current line
            if (c != '\n') {
                buffer.insert(buffer.begin() + cursorPos, sc);
            }
        } else {
            buffer.at(cursorPos) = sc; // overwrite
        }
        if (c == U'\n') {
            auto crsr = getCursorPos();
            if (crsr.x != 0) {
                setCursorPos({0, crsr.y + 1});
            }
        } else {
            ++cursorPos;
        }
    }
    verifyPosition();
    manageOverflow();
    verifyPosition();
}

void ScreenBuffer::defineChar(char32_t codePoint, const CharBitmap& bits) {
    charMap()[codePoint] = bits;
}


void ScreenBuffer::deleteChar() {
    if (cursorPos < buffer.size()) {
        buffer.erase(cursorPos, 1);
    }
}

void ScreenBuffer::backspaceChar() {
    if (cursorPos > 1) {
        buffer.erase(cursorPos - 1, 1);
        cursorPos--;
    }
}

void ScreenBuffer::insertSpace() {
    SChar sc{' ', color};
    buffer.insert(buffer.begin() + cursorPos, sc);
}

ScreenBuffer::Cursor ScreenBuffer::getCursorPos() const {
    return getCursorAtPos(cursorPos);
}

// can return y values >= screen.height!
ScreenBuffer::Cursor ScreenBuffer::getCursorAtPos(size_t pos) const {
    size_t x = 0, y = 0;
    size_t i = 0;
    for (i = 0; i < pos && i < buffer.size(); ++i) {
        bool didLineBreak = false;
        if (x >= width) { didLineBreak = true; x = 0; ++y; }
        // if (y >= height) {
        //     break;
        // }
        if (buffer[i].ch == U'\n') {
            if (didLineBreak) {
                //++x;
            } else {
                y++;
                x = 0;
            }
        } else {
            x++;
        }

    }
    if (x >= width) { x -= width; ++y; }

    // const_cast<ScreenBuffer*>(this)->verifyPosition(pos);

    return {x, y};
}

// allows y positions greater than end of screen
size_t ScreenBuffer::setCursorPos(Cursor crsr) {
    SChar nl{'\n', color};

    while (crsr.x >= width) { crsr.x -= width; ++crsr.y; }
    // if (crsr.y >= height) {
    //     crsr.y = height;
    // }
    // if (crsr.y >= height) {
    //     crsr.y = height - 1;
    //     dropFirstLine();
    //     verifyPosition();
    // }

    // fill with newlines to make sure we have this much lines
    cursorPos = buffer.length();
    auto endCrsr = getCursorAtPos(buffer.length());
    if (endCrsr.y <= crsr.y) {
        buffer.append(1 + crsr.y - endCrsr.y, nl);
    }
    size_t x = 0, y = 0;
    for (size_t i = 0; i < buffer.length(); ++i) {
        auto ch = buffer[i];
        bool didLineBreak = false;
        if (x == width) {
            didLineBreak = true;
            x = 0; ++y;
        }
        if (y > crsr.y || (x == crsr.x && y == crsr.y)) {
            cursorPos = i;
            verifyPosition();
        #if _DEBUG
            auto nowcrsr = getCursorPos();
            if (nowcrsr != crsr) { throw std::exception(); }
        #endif
            return cursorPos;
        }
        if (ch.ch == U'\n') {
            if (!didLineBreak && (y == crsr.y && x < crsr.x)) {
                // we must insert spaces to get to this position
                SChar sp{' ', color};
                buffer.insert(buffer.begin() + i, (crsr.x - x), sp);
                --i;
                continue;
            }

            if (didLineBreak) {
                int pause = 1;
                // ++x;
            } else {
                x = 0;
                ++y;
            }
        } else {
            x++;
        }

    }
    return buffer.length();
}

size_t ScreenBuffer::moveCursorPos(int dx, int dy) {
    auto crsr = getCursorPos();
    while (dx < 0 && dx > crsr.x) {
        dx += int(width);
        --dy;
    }
    crsr.x += dx;
    while (crsr.x >= width) { crsr.x -= width; ++dy; }

    if (dy < 0 && crsr.y < -dy) { crsr.y = 0; } else { crsr.y += dy; }

    size_t pos = setCursorPos(crsr);
    if (crsr.y >= height) {
        dropFirstLine();
        crsr.y = height - 1;
        return setCursorPos(crsr);
    }
    return pos;
}

size_t ScreenBuffer::getPosAtCursor(Cursor crsr) {
    Cursor save = getCursorPos();
    size_t ret = setCursorPos(crsr);
    setCursorPos(save);
    return ret;
}

size_t ScreenBuffer::getStartOfLineAt(Cursor crsr) {
    size_t start = 0;
    size_t pos = 0;
    size_t posEnd = getPosAtCursor(crsr);
    while (pos < posEnd) {
        if (buffer.at(pos).ch == U'\n') { start = pos + 1; }
        ++pos;
    }
    verifyPosition(start);
    return start;
}

size_t ScreenBuffer::getEndOfLineAt(Cursor crsr) {
    size_t pos = getPosAtCursor(crsr);
    while (pos < buffer.length() && buffer.at(pos).ch != U'\n') {
        ++pos;
    }
    if (pos >= buffer.length() || buffer.at(pos).ch != U'\n') { pos = buffer.length(); }
    verifyPosition(pos);
    return pos;
}

// get buffer as a width * height character buffer
void ScreenBuffer::getPrintBuffer(std::u32string& chars, std::string& colors) const {
    chars.reserve(width * height); colors.reserve(width * height);
    chars.clear(); colors.clear();
    char lastColor = char(color);
    size_t x = 0, y = 0;
    for (size_t pos = 0; ; ++pos) {
        const char32_t ch = buffer[pos].ch;
        if (ch == U'\0') { break; }
        if (ch == U'\n' || x == width) {
            chars += std::u32string(width - x, U' ');
            colors += std::string(width - x, char(buffer[pos].col));

            x = 0; ++y;
            if (y == height) {
                SChar sc = {' ', buffer.back().col};
                for (size_t p = pos + 1; p < buffer.size(); ++p) {
                    buffer.data()[p] = sc;
                }
                return;
            }
        }
        if (ch != U'\n') {
            chars += ch;
            lastColor = char(buffer[pos].col);
            colors += lastColor;
            x++;
        }
    }

    size_t fillUp = width * height - chars.size();
    chars += std::u32string(fillUp, ' ');
    colors += std::string(fillUp, lastColor);
}


void ScreenBuffer::updateScreenPixelsPalette() {
    static std::u32string chars;
    static std::string colors;

    getPrintBuffer(chars, colors);

#pragma omp for
    for (int i = 0; i < chars.length(); ++i) {
        size_t x = i % width;
        size_t y = i / width;
        char32_t unicodeCodepoint = chars[i];
        uint8_t color = colors[i];
        drawCharPal(x * ScreenInfo::charPixX, y * ScreenInfo::charPixY, unicodeCodepoint, color & 0x0f, (color >> 4) & 0x0f);
    }


    // sprites
    for (auto& sp : sprites) {
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
    while (n > 0) {
        --n;
        *rgb++ = palette[*pal++];
        ++p; if (p == 8) { p = 0; }
    }
}

std::u32string ScreenBuffer::getSelectedText(size_t start, size_t end) const {
    if (end < start) { std::swap(start, end); }
    std::u32string mid;

#if _DEBUG
    // inverse colors for debugging
    for (size_t i = start; i < end; ++i) {
        auto& ch = const_cast<SChar&>(buffer.at(i));
        ch.col = (ch.col << 4) | (ch.col >> 4);
    }
#endif

    auto sub = buffer.substr(start, end - start);
    mid.reserve(sub.length() + 1);
    for (auto& c : sub) {
        mid += c.ch;
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
    palette[index] = r | (g << 8) | (b << 16) | (a << 24);
}

void ScreenBuffer::resetCharmap(char32_t from, char32_t to) {
    Font::createCharmap(charMap(), from, to);
}

#if 0
void ScreenBuffer::printBuffer() const {
    size_t x = 0, y = 0;
    for (const char32_t* ch = buffer.c_str(); *ch != U'\0'; ++ch) {
        if (*ch == U'\n' || x == width) {
            std::cout << std::string(width - x, ' ') << '\n';
            x = 0; ++y;
            if (y == height) {
                return;
            }
        }
        if (*ch != U'\n') {
            std::cout << static_cast<char>(ch); // Assumes ASCII for simplicity
            x++;
        }
    }
    std::cout << std::string(width - x, ' ') << std::endl;
}
#endif

void ScreenBuffer::dropFirstLine() {
    auto crsr = getCursorPos();
    // size_t firstNewline = buffer.find(U'\n');
    size_t firstNewline = 0;
    auto* ptr = buffer.c_str();
    while (ptr->ch != '\0' && ptr->ch != '\n') { ++ptr; ++firstNewline; }

    if (firstNewline != std::u32string::npos && firstNewline < width) {
        buffer.erase(0, firstNewline + 1);
    } else {
        buffer.erase(0, width);
    }

    SChar nl{'\n', color};
    buffer += nl;
    if (crsr.y > 0) { --crsr.y; }
    setCursorPos(crsr);
}

void ScreenBuffer::manageOverflow() {
    bool didScroll;
    do {
        didScroll = false;
        auto crsr = getCursorPos();
        if (crsr.y >= height) {
            didScroll = true;
            dropFirstLine();
        }
    } while (didScroll);
}

// draw character pixels at given pixel position
void ScreenBuffer::drawCharPal(size_t x, size_t y, char32_t ch, uint8_t colIxText, uint8_t colIxBack) {
    const CharBitmap& img = charMap()[ch];

    // Calculate starting pixel index in the pixel array
    size_t pixelX = x; // * ScreenInfo::charPixX;
    size_t pixelY = y; // * ScreenInfo::charPixY;
    auto& pixels = screenBitmap.pixelsPal;

    if (img.isMono) {
        for (size_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.monoPixels[row]; // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (size_t col = 0; col < ScreenInfo::charPixX; ++col) {
                bool isSet = (pixelRow >> (7 - col)) & 1; // Extract pixel bit
                size_t pixelIndex = (pixelY + row) * (ScreenInfo::pixX)+(pixelX + col);
                pixels[pixelIndex] = isSet ? colIxText : colIxBack;
            }
        }
    } else {
        size_t nth = 0;
        for (size_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.monoPixels[row]; // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (size_t col = 0; col < ScreenInfo::charPixX; ++col) {
                size_t pixelIndex = (pixelY + row) * (ScreenInfo::pixX)+(pixelX + col);
                pixels[pixelIndex] = img.multi(nth++);
            }
        }
    }
}



void ScreenBuffer::drawSprPal(int64_t x, int64_t y, char32_t chimg, int8_t color) {
    if (chimg == 0) { return; }
    const CharBitmap& img = charMap()[chimg];

    auto& pixels = screenBitmap.pixelsPal;

    if (img.isMono) {
        for (int64_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.monoPixels[row]; // Each byte represents 8 pixels in a row
            for (int64_t col = 0; col < ScreenInfo::charPixX; ++col) {
                bool isSet = (pixelRow >> (7 - col)) & 1; // Extract pixel bit
                if (!isSet) { continue; }
                int64_t px = x + col;
                int64_t py = y + row;
                if (px < 0 || py < 0 || px >= ScreenInfo::pixX || py >= ScreenInfo::pixY) { continue; }
                size_t pixelIndex = py * ScreenInfo::pixX + px;
                pixels[pixelIndex] = color;
            }
        }
    } else {
        size_t nth = 0;
        for (int64_t row = 0; row < ScreenInfo::charPixY; ++row) {
            uint8_t pixelRow = img.monoPixels[row]; // Each byte represents 8 pixels in a row
            static_assert(ScreenInfo::charPixX == 8, "the pixel bytes are wrong, otherwise");
            for (int64_t col = 0; col < ScreenInfo::charPixX; ++col) {
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



#ifdef _DEBUG
void ScreenBuffer::verifyPosition(size_t p) {
    size_t oldCur = cursorPos;
    static bool inside = false;
    if (inside) { return; }

    // update debug string
    debugBuffer.clear();
    for (auto sc : buffer) {
        debugBuffer += char(sc.ch);
    }

    // try beyond end of buffer error
    if (p == size_t(-1)) {
        p = cursorPos;
    }

    if (p > buffer.length()) {
        throw std::exception();
    }

    // try cursor position error
    inside = true;
    auto cr = getCursorAtPos(p);
    size_t cp = getPosAtCursor(cr);
    inside = false;
    // 
    // if (cp != p && cr.y < 25) {
    //     cr = getCursorAtPos(p);
    //     cp = getPosAtCursor(cr);
    //     throw std::exception();
    // }

    cursorPos = oldCur;
}


#endif


