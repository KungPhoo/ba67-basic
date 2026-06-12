#include "screen_buffer.h"
#include <cmath>
#include "font.h"
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include "unicode.h"

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
    // if width is 80, this will overwrite the C64 BASIC program memory!
    size_t n = width * height; // 80!x25 width * height;
    auto col = currentColor();
    for (size_t i = 0; i < n; ++i) {
        charRam[i] = U' ';
        colRam[i]  = col;
    }

    for (size_t i = 0; i < height; ++i) {
        makeRowOwner(i);
    }
    rebuildLineLinkAddresses();

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

// get start of logical line from cursor position
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

// get end of logical line from cursor position
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

void ScreenBuffer::initMemory(RomImage& image) {
    memory             = &image.RAM[0];
    lineLinkTable      = &memory[krnl.LDTB1];
    size_t charAddress = memory[krnl.HIBASE] << 8;
    charRam            = memory + charAddress;
    VIC                = &image.IO[0];
    colRam             = &VIC[0xD800]; // VIC II memory in IO space

    // clear sets the line-link-table
    // for (size_t i = 0; i < height; ++i) {
    //     lineLinkTable[i] = ((charAddress + i * width) >> 8) | 0x80;
    // }

    setCursorPos({ 0, 0 });
    setSize(40, 25);

    resetDefaultColors();
    setColors(1, 0);
    setBorderColor(1);
    setReverseMode(false);

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
            if (rowContinues(y)) {
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
        drawSprPal(pixelsPal, sp.x + 0 - sprbx, sp.y - sprby, sp.charmap[0], sp.color);
        drawSprPal(pixelsPal, sp.x + 8 - sprbx, sp.y - sprby, sp.charmap[1], sp.color);
        drawSprPal(pixelsPal, sp.x + 16 - sprbx, sp.y - sprby, sp.charmap[2], sp.color);
        drawSprPal(pixelsPal, sp.x + 0 - sprbx, sp.y + 8 - sprby, sp.charmap[3], sp.color);
        drawSprPal(pixelsPal, sp.x + 8 - sprbx, sp.y + 8 - sprby, sp.charmap[4], sp.color);
        drawSprPal(pixelsPal, sp.x + 16 - sprbx, sp.y + 8 - sprby, sp.charmap[5], sp.color);
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


// in case you re-mapped the Unicode characters, provide the mapping
// as an argument here. Otherwise just pass an empty array.
// Characters > 127 are mapped, then.
std::string& ScreenBuffer::updateScreenTerminal(std::function<char32_t(char32_t)> mapping) {
    // uses ESC-codes for Windows and Liunx: https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
#define ESC "\x1b"


    static std::string buffer;
    buffer.clear();

    static std::vector<std::string> colFg, colBg;


    if (colFg.empty()) {


        #if 0 // redefine the 16 colors
        // this does not work. My Linux console changes either foreground
        // or background color based on this to have better contrast.
        // Clever, but unwanted.
        static char tmp[1024];
        // [c64] = console color index
        // 13 is unused. It's a bright purple, but we overwrite with brown.
        // 14 (light cyan) use reused as a light gray.
        static const std::vector<int> c64term = { 0, 15, 1, 6, 5, 2, 4, 11,    3, 13 /*lt purple=brown*/, 9, 8, 7, 10, 12, 14 /*lt.cyan=lt gray*/ };
        for (int i = 0; i < 16; ++i) {
            uint32_t c = palette[i];
            int r = (c >> 16) & 0xff;
            int g = (c >> 8) & 0xff;
            int b = (c >> 0) & 0xff;
            printf(ESC "]P%.1X%.2X%.2X%.2X\n", c64term[i], r, g, b);
        }
        // https://en.wikipedia.org/wiki/ANSI_escape_code
        // only two "gray" in this mode
        colFg = {
            ESC "[30m", //  1 black
            ESC "[97m", //  2 white
            ESC "[31m", //  3 red
            ESC "[36m", //  4 cyan
            ESC "[35m", //  5 purple
            ESC "[32m", //  6 green
            ESC "[34m", //  7 blue
            ESC "[93m", //  8 yellow
            ESC "[33m", //  9 orange
            ESC "[95m", // 10 *brown
            ESC "[91m", // 11 light red
            ESC "[90m", // 12 dark gray
            ESC "[37m", // 13 medium gray
            ESC "[92m", // 14 light green
            ESC "[94m", // 15 light blue
            ESC "[96m"  // 16 *light gray
        };
        colBg = {
            ESC "[40m", // black
            ESC "[107m", // white
            ESC "[41m", // red
            ESC "[46m", // cyan
            ESC "[45m", // purple
            ESC "[42m", // green
            ESC "[44m", // blue
            ESC "[103m", // yellow
            ESC "[43m", // orange
            ESC "[105m", // *brown
            ESC "[101m", // light red
            ESC "[100m", // dark gray
            ESC "[47m", // medium gray
            ESC "[102m", // light green
            ESC "[104m", // light blue
            ESC "[106m" // *light gray
        };

        #else // use default colors


        // "xterm-256color" or "linux" for 16 colors
        bool has256Colors = true;
#if !defined(_WIN32)
        if (getenv("TERM") != nullptr) {
            std::string term = getenv("TERM");
            if (term.find("256color") == std::string::npos) {
                has256Colors = false;
            }
        }
#endif

        if (has256Colors) {
            for (int i = 0; i < 16; ++i) {
                int r  = (palette[i] >> 16) & 0xff;
                int g  = (palette[i] >> 8) & 0xff;
                int b  = (palette[i] >> 0) & 0xff;
                int id = 16 + 36 * (r / 51) + 6 * (g / 51) + (b / 51);

                if (r == g && r == b) {
                    id = (r / 11) + 232; // grayscales
                }

                char bf[256];
                sprintf(bf, ESC "[38;5;%dm", id);
                colFg.push_back(bf);
                sprintf(bf, ESC "[48;5;%dm", id);
                colBg.push_back(bf);
            }
        } else {
            // https://en.wikipedia.org/wiki/ANSI_escape_code
            // only two "gray" in this mode
            colFg = {
                ESC "[30m", // black
                ESC "[97m", // white
                ESC "[31m", // red
                ESC "[36m", // cyan
                ESC "[35m", // purple
                ESC "[32m", // green
                ESC "[34m", // blue
                ESC "[93m", // yellow
                ESC "[33m", // orange
                ESC "[31m", // *brown
                ESC "[91m", // light red
                ESC "[90m", // dark gray
                ESC "[37m", // medium gray
                ESC "[92m", // light green
                ESC "[94m", // light blue
                ESC "[37m"  // light gray
            };
            colBg = {
                ESC "[40m", // black
                ESC "[107m", // white
                ESC "[41m", // red
                ESC "[46m", // cyan
                ESC "[45m", // purple
                ESC "[42m", // green
                ESC "[44m", // blue
                ESC "[103m", // yellow
                ESC "[43m", // orange
                ESC "[41m", // *brown
                ESC "[101m", // light red
                ESC "[100m", // dark gray
                ESC "[47m", // medium gray
                ESC "[102m", // light green
                ESC "[104m", // light blue
                ESC "[47m" // light gray
            };
        }

        #endif
    }

    MEMCELL currentColor = MEMCELL(0xffff);

    const auto* memCh = charRam;
    const auto* memCl = colRam;

    buffer += ESC "[?25l"; // hide cursor
    // buffer += ESC "[2J"; // erase screen
    buffer += ESC "[H"; // home
    for (size_t y = 0; y < height; ++y) {
        if (y > 0) {
            // print newline - but in black
            // currentColor = 0;
            // buffer += colFg[1];
            // buffer += colBg[1];
            buffer += "\n";
        }
        // buffer += ESC "[0K"; // erase from cursor to end of line
        const auto* lnCh = memCh + y * width;
        const auto* lnCo = memCl + y * width;

        for (size_t x = 0; x < width; ++x) {
            char32_t ch = char32_t(lnCh[x]);
            if (ch >= 0 && ch < 0x20) {
                ch = U' ';
            }

            MEMCELL cl = lnCo[x];
            if (cl != currentColor) {
                currentColor = cl;
                size_t fg    = cl & 0x0f;
                size_t bg    = (cl >> 4) & 0x0f;
                buffer += colFg[fg];
                buffer += colBg[bg];
            }

            // apply mapping to Linux console glyphs
            if (ch > 127 && mapping) {
                // HACK: Linux only supports Unicode mapping in 16 bits.
                //       Just use the final 3 bytes and put them im "Private Use Area Block" U+E000..U+F8FF
                if (ch > 0xFFFF) {
                    ch = (ch & 0x07FF) + 0xE000;
                }

                ch = mapping(ch);
            }
            Unicode::appendAsUtf8(buffer, ch);
        }
    }


    char bf[1024];
    auto crsr = getCursorPos();
    sprintf(bf, ESC "[%d;%dH", int(crsr.y + 1), int(crsr.x + 1));
    buffer += bf;

    buffer += ESC "[?25h"; // show cursor


#undef ESC
    return buffer;
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


#if 0
    // very vibrant
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
#elif 0
    // http://unusedino.de/ec64/technical/misc/vic656x/colors/
    // this seems more realistic - but include Hannover line effect
    defineColor(0, 0x00, 0x00, 0x00); //  1 Black
    defineColor(1, 0xFF, 0xFF, 0xFF); //  2 White
    defineColor(2, 0x68, 0x37, 0x2B); //  3 Red
    defineColor(3, 0x70, 0xA4, 0xB2); //  4 Cyan
    defineColor(4, 0x6F, 0x3D, 0x86); //  5 Purple
    defineColor(5, 0x58, 0x8D, 0x43); //  6 Green
    defineColor(6, 0x35, 0x28, 0x79); //  7 Blue
    defineColor(7, 0xB8, 0xC7, 0x6F); //  8 Yellow
    defineColor(8, 0x6F, 0x4F, 0x25); //  9 Orange
    defineColor(9, 0x43, 0x39, 0x00); //  0 Brown
    defineColor(10, 0x9A, 0x67, 0x59); // 11 Light Red
    defineColor(11, 0x44, 0x44, 0x44); // 12 Dark Gray
    defineColor(12, 0x6C, 0x6C, 0x6C); // 13 Medium Gray
    defineColor(13, 0x9A, 0xD2, 0x84); // 14 Light Green
    defineColor(14, 0x6C, 0x5E, 0xB5); // 15 Light Blue
    defineColor(15, 0x95, 0x95, 0x95); // 16 Light Gray
#else
    // https://www.colodore.com/ from https://www.pepto.de/projects/colorvic/
    // defineColor(0, 0x00, 0x00, 0x00); //  1 Black
    // defineColor(1, 0xFF, 0xFF, 0xFF); //  2 White
    // defineColor(2, 0x81, 0x33, 0x38); //  3 Red
    // defineColor(3, 0x75, 0xce, 0xc8); //  4 Cyan
    // defineColor(4, 0x8e, 0x3c, 0x97); //  5 Purple
    // defineColor(5, 0x56, 0xac, 0x4d); //  6 Green
    // defineColor(6, 0x2e, 0x2c, 0x9b); //  7 Blue
    // defineColor(7, 0xed, 0xf1, 0x71); //  8 Yellow
    // defineColor(8, 0x8e, 0x50, 0x29); //  9 Orange
    // defineColor(9, 0x55, 0x38, 0x00); //  0 Brown
    // defineColor(10, 0xc4, 0x6c, 0x71); // 11 Light Red
    // defineColor(11, 0x4a, 0x4a, 0x4a); // 12 Dark Gray
    // defineColor(12, 0x7b, 0x7b, 0x7b); // 13 Medium Gray
    // defineColor(13, 0xa9, 0xff, 0x9f); // 14 Light Green
    // defineColor(14, 0x70, 0x6d, 0xeb); // 15 Light Blue
    // defineColor(15, 0xb2, 0xb2, 0xb2); // 16 Light Gray


    // from colodore.com
    #if 1

    auto gamma_pepto = [](double value) -> double {
        double gammasrc = 2.8; // PAL
        double gammatgt = 2.2; // sRGB
        value           = std::min(std::max(value, 0.0), 255.0);

        // reverse gamma correction of source
        double factor = std::pow(255.0, 1.0 - gammasrc);
        value         = std::min(std::max(factor * std::pow(value, gammasrc), 0.0), 255.0);

        // apply gamma correction for target
        factor = std::pow(255.0, 1.0 - 1.0 / gammatgt);
        value  = std::min(std::max(factor * pow(value, 1 / gammatgt), 0.0), 255.0);
        return value;
    };

    auto yuv2r = [](double y, double v) -> uint8_t {
        return uint8_t(std::min(std::max(y + 1.140 * v, 0.0), 255.0));
    };

    auto yuv2g = [](double y, double u, double v) -> uint8_t {
        return uint8_t(std::min(std::max(y - 0.396 * u - 0.581 * v, 0.0), 255.0));
    };

    auto yuv2b = [](double y, double u) -> uint8_t {
        return uint8_t(std::min(std::max(y + 2.029 * u, 0.0), 255.0));
    };

    auto luma = [](double input) {
        double factor = (256.0 / 32.0);

        return input * factor;
    };

    auto angle = [](double input, double phs) {
        double factor = 360.0 / 16.0;
        double degree = 3.14158 / 180.0;
        double rotate = factor / 2.0 + phs;

        return (input * factor + rotate) * degree;
    };
    auto crop = [](double c) -> uint8_t {
        int i = int(c + 0.499999);
        if (i < 0) {
            return 0;
        }
        if (i > 255) {
            return 255;
        }
        return i & 0xff;
    };
    std::vector<double> lumas(16);
    lumas[0]  = 0; // Black
    lumas[1]  = 32; // White
    lumas[2]  = 10; // Red
    lumas[3]  = 20; // Cyan
    lumas[4]  = 12; // Purple
    lumas[5]  = 16; // Green
    lumas[6]  = 8; // Blue
    lumas[7]  = 24; // Yellow
    lumas[8]  = lumas[4]; // Orange
    lumas[9]  = lumas[6]; // Brown
    lumas[10] = lumas[5]; // Light Red
    lumas[11] = lumas[2]; // Dark Grey
    lumas[12] = 15; // Grey
    lumas[13] = lumas[7]; // Light Green
    lumas[14] = lumas[12]; // Light Blue
    lumas[15] = lumas[3]; // Light Grey


    std::vector<double> angles(16);

    angles[0]  = -1; // Black
    angles[1]  = -1; // White
    angles[2]  = 4; // Red
    angles[3]  = 4 + 8; // Cyan
    angles[4]  = 2; // Purple
    angles[5]  = 2 + 8; // Green
    angles[6]  = 7 + 8; // Blue
    angles[7]  = 7; // Yellow
    angles[8]  = 5; // Orange
    angles[9]  = 6; // Brown
    angles[10] = angles[2]; // Light Red
    angles[11] = -1; // Dark Grey
    angles[12] = -1; // Grey
    angles[13] = angles[5]; // Light Green
    angles[14] = angles[6]; // Light Blue
    angles[15] = -1; // Light Grey

    //  This is how I remember them (without the CRT emulation)
    double bri = 50.0; // brightness
    double con = 100.0; // contrast
    double sat = 75.0 / 1.25; // saturation /1.25. Max 0.80

    bool invert  = false;
    double phase = 0.0;

    for (int i = 0; i < 16; i++) {
        double y = luma(lumas[i]);
        double u = 0, v = 0;

        if (angles[i] == -1) {
            u = 0;
            v = 0;
        } else {
            u = sat * cos(angle(angles[i], phase));
            v = sat * sin(angle(angles[i], phase)) * ((invert == true) ? -1 : 1);
        }

        y *= con / 100.0;
        u *= con / 100.0;
        v *= con / 100.0;
        y += bri; // apply brightness and contrast

        double r = gamma_pepto(yuv2r(y, v));
        double g = gamma_pepto(yuv2g(y, u, v));
        double b = gamma_pepto(yuv2b(y, u));

        defineColor(i, crop(r), crop(g), crop(b));
    }


    #endif

#endif
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

void ScreenBuffer::chrout(char32_t c) {
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
    auto crsr = getCursorPos();
    findTailCell(crsr.y, tailR, tailC);
    if (tailC < crsr.x) {
        tailC = crsr.x;
    }
    ensureOneCellAtTail(tailR, tailC);
    crsr = getCursorPos(); // again. might have scrolled

    size_t r = tailR, c = tailC;
    while (true) {
        size_t dstR = r, dstC = c + 1;
        if (dstC >= width) {
            dstR++;
            dstC = 0;
        }

#if DEBUG
        if (c > width || r > height) {
            throw "(c,r) is out of screen memory";
        }
#endif

        MEMCELL ch = charAt(r, c);
        MEMCELL co = colorAt(r, c);
        setAt(dstR, dstC, ch, co);

        if (r == crsr.y && c == crsr.x) {
            break;
        }
        if (c > 0) {
            c--;
        } else {
            if (r > 0) {
                r--;
            }
            c = width - 1;
        }
    }
    setAt(crsr.y, crsr.x, blankChar, currentColor());
}

// rebuild the line link table's low bits that indicate the
// address of the screen buffer.
// Keep the continuation bit as it was.
void ScreenBuffer::rebuildLineLinkAddresses() {
    size_t charAddress = size_t(memory[krnl.HIBASE]) << 8;

    const size_t llheight = 25; // hard-coded 25 lines in the line link table

    for (size_t i = 0; i < llheight; ++i) {
        MEMCELL hi       = lineLinkTable[i] & 0x80;
        MEMCELL value    = MEMCELL(((charAddress + i * width) >> 8) | 0x80);
        lineLinkTable[i] = (value & 0x7f) | hi;
    }
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

// find the end of the logical line for a given physical line index
void ScreenBuffer::findTailCell(size_t startR, size_t& outR, size_t& outC) {
    size_t r = startR;
    while (rowContinues(r) && r + 1 < height) {
        r++;
    }


    for (size_t rr = r; rr + 1 > startR; --rr) {
        for (size_t cc = width; cc-- > 0;) {
            if (charAt(rr, cc) != blankChar) {
                outR = rr;
                outC = cc;
                return;
            }
        }
    }
    outR = startR;
    outC = width - 1;
}

void ScreenBuffer::ensureOneCellAtTail(size_t tailR, size_t tailC) {
    if (tailC + 1 < width) {
        return;
    }
    if (tailR + 1 >= height) {
        scrollUpOne();
        --tailR;
    }

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
    rebuildLineLinkAddresses();
}

size_t ScreenBuffer::lastUsedColumn(size_t r) const {
    for (size_t c = width; c-- > 0;) {
        if (charAt(r, c) != blankChar) {
            return c;
        }
    }
    return 0;
}


// lineIndex (base0) is the new line that will be inserted.
void ScreenBuffer::insertNewEmptyLine(int lineIndex) {
    if (lineIndex < 0 || lineIndex >= height) {
        return;
    }
    auto cr = getCursorPos();
    while (lineIndex < height - 1 && isContinuationRow(lineIndex + 1)) {
        scrollUpOne();
    }

    const size_t rowCellCount = width;

    // keep top line in overflow buffer
    LineOverflow ovr;
    ovr.setWidth(width);
    for (size_t c = 0; c < width; ++c) {
        ovr.characters[c] = charRam[c];
        if (colRam) {
            ovr.colors[c] = colRam[c];
        }
    }
    ovr.lineLink = lineLinkTable[0];
    overflowTop.push_back(ovr);

    ++scrollCount;
    std::memmove(charRam, charRam + rowCellCount, (lineIndex)*rowCellCount * sizeof(MEMCELL));
    if (colRam) {
        std::memmove(colRam, colRam + rowCellCount, (lineIndex)*rowCellCount * sizeof(MEMCELL));
    }
    for (size_t r = 0; r < lineIndex; ++r) {
        lineLinkTable[r] = lineLinkTable[r + 1];
    }

    makeRowOwner(lineIndex);
    setCursorPos(Cursor { 0, size_t(lineIndex) });
    cleanCurrentLine();
    setCursorPos(cr);
    rebuildLineLinkAddresses();
}

void ScreenBuffer::scrollUpOne() {
    const size_t rowCellCount = width;

    // keep top line in overflow buffer
    LineOverflow ovr;
    ovr.setWidth(width);
    for (size_t c = 0; c < width; ++c) {
        ovr.characters[c] = charRam[c];
        if (colRam) {
            ovr.colors[c] = colRam[c];
        }
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
        const size_t index = (height - 1) * width + c;
        if (!overflowBottom.empty() && c < overflowBottom.back().width()) {
            charRam[index] = overflowBottom.back().characters[c];
            if (colRam) {
                colRam[index] = overflowBottom.back().colors[c];
            }
        } else {
            charRam[index] = blankChar;
            if (colRam) {
                colRam[index] = curCol;
            }
        }
    }

    for (size_t r = 0; r + 1 < height; ++r) {
        lineLinkTable[r] = lineLinkTable[r + 1];
    }

    if (!overflowBottom.empty()) {
        lineLinkTable[height - 1] = overflowBottom.back().lineLink;
        overflowBottom.pop_back();
    }
    rebuildLineLinkAddresses();

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
    ovr.setWidth(width);
    for (size_t c = 0; c < width; ++c) {
        const size_t index = c + ((height - 1) * rowCellCount);
        ovr.characters[c]  = charRam[index];
        if (colRam) {
            ovr.colors[c] = colRam[index];
        }
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
        if (!overflowTop.empty() && c < overflowTop.back().width()) {
            charRam[c] = overflowTop.back().characters[c];
            if (colRam) {
                colRam[c] = overflowTop.back().colors[c];
            }
        } else {
            charRam[c] = blankChar;
            if (colRam) {
                colRam[c] = curCol;
            }
        }
    }

    for (size_t r = height - 1; r > 0; --r) {
        lineLinkTable[r] = lineLinkTable[r - 1];
    }

    if (!overflowTop.empty()) {
        lineLinkTable[0] = overflowTop.back().lineLink;
        overflowTop.pop_back();
    }
    rebuildLineLinkAddresses();

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
    const uint8_t buddyIndex[16] = { 11, 15, 8, 13, 10, 13, 14, 1, 7, 8, 7, 12, 15, 7, 13, 1 };
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
