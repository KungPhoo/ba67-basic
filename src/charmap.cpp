#include "charmap.h"
#include <cstdint>
// #include <cstring>
#include "font-bitmap-data.h"

CharMap::CharMap()
    : ascii {} {
    unicode = new std::unordered_map<char32_t, CharBitmap>();
    init();
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

#ifdef _DEBUG
    #include <sstream>
    #include <fstream>
    #include <iostream>
    #include <iomanip>
class BDFExport {
    // Write one byte as two hex digits
    std::string toHex(uint8_t byte) {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << int(byte);
        return oss.str();
    }

public:
    void writeBDFunscaled(const FontDataBits::DataStruct* bits, const std::string& filename) {
        std::ofstream out(filename);
        if (!out) {
            std::cerr << "Cannot open file: " << filename << "\n";
            return;
        }

        // BDF Header
        // https://en.wikipedia.org/wiki/Glyph_Bitmap_Distribution_Format
        out << "STARTFONT 2.1\n"
            << "FONT -BA67-BA67-medium-upright-normal--16-160-75-75-monospaced-16-ISO10646-1\n"
            << "SIZE 16 75 75\n"
            << "FONTBOUNDINGBOX 8 16 0 -2\n"
            << "STARTPROPERTIES 2\n"
            << "FONT_ASCENT 14\n"
            << "FONT_DESCENT 2\n"
            << "ENDPROPERTIES\n";

        // Count glyphs
        int count = 0;
        for (const auto* ptr = bits; ptr->c != 0xffffffff; ++ptr) {
            ++count;
        }

        out << "CHARS " << count << "\n";

        // Write glyphs
        for (const auto* ptr = bits; ptr->c != 0xffffffff; ++ptr) {
            char32_t codepoint = ptr->c;
            const uint8_t* bmp = ptr->b;

            out << "STARTCHAR U+" << std::hex << std::uppercase << int(codepoint) << "\n";
            out << "ENCODING " << std::dec << int(codepoint) << "\n";
            out << "SWIDTH 500 0\n"; // 500 / 1000 point = half width
            out << "DWIDTH 8 0\n"; // 8 px width
            out << "BBX 8 16 0 -2\n"; // width 8, height 16, y-offset -2
            out << "BITMAP\n";

            // Scale to 8x16 by repeating each 8-bit row
            for (int i = 0; i < 8; ++i) {
                std::string line = toHex(bmp[i]);
                out << line << "\n"
                    << line << "\n"; // repeat line
            }

            out << "ENDCHAR\n";
        }

        out << "ENDFONT\n";
        out.close();
        std::cout << "Wrote: " << filename << "\n";
    }



    void writeBDF(const FontDataBits::DataStruct* bits,
                  const std::string& filename,
                  int scalex = 1, int scaley = 1) {
        std::ofstream out(filename);
        if (!out) {
            std::cerr << "Cannot open file: " << filename << "\n";
            return;
        }

        const int glyphW = 8 * scalex;
        const int glyphH = 8 * scaley;

        // BDF Header
        out << "STARTFONT 2.1\n"
            << "FONT -BA67-BA67-medium-upright-normal--" << glyphH
            << "-" << glyphH * 10 << "-75-75-monospaced-" << glyphW
            << "-ISO10646-1\n"
            << "SIZE " << glyphH << " 75 75\n"
            << "FONTBOUNDINGBOX " << glyphW << " " << glyphH << " 0 0\n"
            << "STARTPROPERTIES 2\n"
            << "FONT_ASCENT " << glyphH - 2 << "\n"
            << "FONT_DESCENT 2\n"
            << "ENDPROPERTIES\n";

        // Count glyphs
        int count = 0;
        for (const auto* ptr = bits; ptr->c != 0xffffffff; ++ptr)
            ++count;
        out << "CHARS " << count << "\n";

        // Glyphs
        for (const auto* ptr = bits; ptr->c != 0xffffffff; ++ptr) {
            char32_t codepoint = ptr->c;
            const uint8_t* bmp = ptr->b;

            out << "STARTCHAR U+" << std::hex << std::uppercase << int(codepoint) << "\n";
            out << "ENCODING " << std::dec << int(codepoint) << "\n";
            out << "SWIDTH " << (glyphW * 1000 / glyphH) << " 0\n"; // rough scaling
            out << "DWIDTH " << glyphW << " 0\n";
            out << "BBX " << glyphW << " " << glyphH << " 0 0\n";
            out << "BITMAP\n";

            // Scale rows and columns
            for (int y = 0; y < 8; ++y) {
                // expand one row horizontally
                std::vector<uint8_t> expanded(glyphW / 8, 0);
                for (int x = 0; x < 8; ++x) {
                    bool on = (bmp[y] >> (7 - x)) & 1;
                    if (on) {
                        int start = x * scalex;
                        for (int sx = 0; sx < scalex; ++sx) {
                            int px = start + sx;
                            expanded[px / 8] |= (0x80 >> (px % 8));
                        }
                    }
                }

                // write this row "scale" times (vertical scaling)
                for (int sy = 0; sy < scaley; ++sy) {
                    for (uint8_t b : expanded)
                        out << toHex(b);
                    out << "\n";
                }
            }

            out << "ENDCHAR\n";
        }

        out << "ENDFONT\n";
        out.close();
        std::cout << "Wrote: " << filename << " with scale " << scalex << "x" << scaley << "x\n";
    }
};
#endif


void CharMap::init(char32_t from, char32_t to) {

    std::unordered_map<char32_t, char32_t> synonyms = {
        { 0x203e,   0xaf },
        { 0x2044,   0x2f },
        { 0x220a, 0x2208 },
        { 0x20b7, 0x20b6 },
        { 0x20bb, 0x20b6 },
        { 0x20bc, 0x20b6 },
        { 0x20bd, 0x20b6 },
        { 0x20be, 0x20b6 },
        { 0x20bf, 0x20b6 },
        { 0x20c0, 0x20b6 },
        { 0x20c1, 0x20b6 },
        { 0x20c2, 0x20b6 },
        { 0x20c3, 0x20b6 },
        { 0x20c4, 0x20b6 },
        { 0x20c5, 0x20b6 },
        { 0x20c6, 0x20b6 },
        { 0x20c7, 0x20b6 },
        { 0x20c8, 0x20b6 },
        { 0x20c9, 0x20b6 },
        { 0x20ca, 0x20b6 },
        { 0x20cb, 0x20b6 },
        { 0x20cc, 0x20b6 },
        { 0x20cd, 0x20b6 },
        { 0x20ce, 0x20b6 },
        { 0x2211,  0x3a3 },
        { 0x2212,   0x2d },
        { 0x2215,   0x2f },
        { 0x2217,   0x2a },
        { 0x22c2, 0x2229 },
        { 0x22c3, 0x222a },
        { 0x2303,   0x5e },
        { 0x231c, 0x250c },
        { 0x23ce, 0x21b5 },
        { 0x2ba0, 0x21b5 },
        // { 0x2713, 0x23b7 },
        { 0x2e28, 0x2985 },
        { 0x2e29, 0x2986 },
        { 0x2e40,   0x3d },
        // CJK and Katagana
        { 0x3008, 0x2329 },
        { 0x3009, 0x232a },
        { 0x30a0,   0x3d },
        { 0x30fb,   0xb7 },
        { 0x301c,   0x7e },
        { 0x301d, 0x2036 },
        { 0x301e, 0x2033 },
        { 0x3036, 0x3012 },
        { 0x30a1, 0x30a2 },
        { 0x30a3, 0x30a4 },
        { 0x30a5, 0x30a6 },
        { 0x30a7, 0x30a8 },
        { 0x30a9, 0x30aa },
        { 0x30c3, 0x30c4 },
        { 0x30e3, 0x30e4 },
        { 0x30e5, 0x30e6 },
        { 0x30e7, 0x30e8 },
        { 0x30ee, 0x30ef },
        { 0x30f5, 0x30ab },
        { 0x30f6, 0x30b1 },
        { 0x31f0, 0x30af },
        { 0x31f1, 0x30b7 },
        { 0x31f2, 0x30b9 },
        { 0x31f3, 0x30c8 },
        { 0x31f4, 0x30cc },
        { 0x31f5, 0x30cf },
        { 0x31f6, 0x30d2 },
        { 0x31f7, 0x30d5 },
        { 0x31f8, 0x30d8 },
        { 0x31f9, 0x30db },
        { 0x31fa, 0x30e0 },
        { 0x31fb, 0x30e9 },
        { 0x31fc, 0x30ea },
        { 0x31fd, 0x30eb },
        { 0x31fe, 0x30ec },
        { 0x31ff, 0x30ed },
        { 0x3192, 0x4e00 },
        { 0x3193, 0x4e8c },
        { 0x3194, 0x4e09 },
        { 0x3195, 0x56db },
        { 0x3196, 0x4e0a },
        { 0x3197, 0x4e2d },
        { 0x3198, 0x4e0b },
        { 0x3199, 0x7532 },
        { 0x319a, 0x4e59 },
        { 0x319b, 0x4e19 },
        { 0x319c, 0x4e01 },
        { 0x319d, 0x5929 },
        { 0x319e, 0x5730 },
        { 0x319f, 0x4eba },
        // small forms
        { 0xfe50,   0x2c },
        { 0xfe51, 0x3001 },
        { 0xfe52,   0x2e },
        { 0xfe54,   0x3b },
        { 0xfe55,   0x3a },
        { 0xfe56,   0x3f },
        { 0xfe57,   0x21 },
        { 0xfe58, 0x2014 },
        { 0xfe59,   0x28 },
        { 0xfe5a,   0x29 },
        { 0xfe5b,   0x7b },
        { 0xfe5c,   0x7d },
        { 0xfe5d, 0x3014 },
        { 0xfe5e, 0x3015 },
        { 0xfe5f,   0x23 },
        { 0xfe60,   0x26 },
        { 0xfe61,   0x2a },
        { 0xfe62,   0x2b },
        { 0xfe63,   0x2d },
        { 0xfe64,   0x3c },
        { 0xfe65,   0x3e },
        { 0xfe66,   0x3d },
        { 0xfe68,   0x5c },
        { 0xfe69,   0x24 },
        { 0xfe6a,   0x25 },
        { 0xfe6b,   0x40 },
        // full- and halfwidth
        { 0xff81, 0x30c1 },
        { 0xff82, 0x30c4 },
        { 0xff83, 0x30c6 },
        { 0xff84, 0x30c8 },
        { 0xff85, 0x30ca },
        { 0xff86, 0x30cb },
        { 0xff87, 0x30cc },
        { 0xff88, 0x30cd },
        { 0xff89, 0x30ce },
        { 0xff8a, 0x30cf },
        { 0xff8b, 0x30d2 },
        { 0xff8c, 0x30d5 },
        { 0xff8d, 0x30d8 },
        { 0xff8e, 0x30db },
        { 0xff8f, 0x30de },
        { 0xff90, 0x30df },
        { 0xff91, 0x30e0 },
        { 0xff92, 0x30e1 },
        { 0xff93, 0x30e2 },
        { 0xff94, 0x30e4 },
        { 0xff95, 0x30e6 },
        { 0xff96, 0x30e8 },
        { 0xff97, 0x30e9 },
        { 0xff98, 0x30ea },
        { 0xff99, 0x30eb },
        { 0xff9a, 0x30ec },
        { 0xff9c, 0x30ef },
        { 0xff66, 0x30f2 },
        { 0xff67, 0x30a2 },
        { 0xff68, 0x30a4 },
        { 0xff69, 0x30a6 },
        { 0xff6a, 0x30a8 },
        { 0xff6b, 0x30aa },
        { 0xff76, 0x30ab },
        { 0xff77, 0x30ad },
        { 0xff78, 0x30af },
        { 0xff79, 0x30b1 },
        { 0xff9e, 0x309b },
        { 0xff7d, 0x30b9 },
        { 0xff9f, 0x309c },
        { 0xff80, 0x30bf },
        { 0xff9b, 0x30ed },
        { 0xff9d, 0x30f3 },
        { 0xff7a, 0x30b3 },
        { 0xff7b, 0x30b5 },
        { 0xff7c, 0x30b7 },
        { 0xff7e, 0x30bb },
        { 0xff7f, 0x30bd },
        { 0xff6d, 0x30e6 },
        { 0xff6e, 0x30e8 },
        { 0xff6f, 0x30e9 },
        { 0xff71, 0x30a2 },
        { 0xff72, 0x30a4 },
        { 0xff73, 0x30a6 },
        { 0xff74, 0x30a8 },
        { 0xff75, 0x30aa },
        { 0xff65,   0xb7 },
        { 0xff70, 0x30fc },
        { 0xff5f, 0x2985 },
        { 0xff60, 0x2986 },
        { 0xff61, 0x3002 },
        { 0xff62, 0x300c },
        { 0xff63, 0x300d },
        { 0xff64, 0x3001 },
        { 0xff1d,   0x3d },
        { 0xff01,   0x21 },
        { 0xff02,   0x22 },
        { 0xff03,   0x23 },
        { 0xff04,   0x24 },
        { 0xff05,   0x25 },
        { 0xff06,   0x26 },
        { 0xff07,   0x27 },
        { 0xff08,   0x28 },
        { 0xff09,   0x29 },
        { 0xff0a,   0x2a },
        { 0xff0b,   0x2b },
        { 0xff0c,   0x2c },
        { 0xff0d,   0x2d },
        { 0xff0e,   0x2e },
        { 0xff0f,   0x2f },
        { 0xff10,   0x30 },
        { 0xff11,   0x31 },
        { 0xff12,   0x32 },
        { 0xff13,   0x33 },
        { 0xff14,   0x34 },
        { 0xff15,   0x35 },
        { 0xff16,   0x36 },
        { 0xff17,   0x37 },
        { 0xff18,   0x38 },
        { 0xff19,   0x39 },
        { 0xff1a,   0x3a },
        { 0xff1b,   0x3b },
        { 0xff1c,   0x3c },
        { 0xff1e,   0x3e },
        { 0xff1f,   0x3f },
        { 0xff20,   0x40 },
        { 0xff21,   0x41 },
        { 0xff22,   0x42 },
        { 0xff23,   0x43 },
        { 0xff24,   0x44 },
        { 0xff25,   0x45 },
        { 0xff26,   0x46 },
        { 0xff27,   0x47 },
        { 0xff28,   0x48 },
        { 0xff29,   0x49 },
        { 0xff2a,   0x4a },
        { 0xff2b,   0x4b },
        { 0xff2c,   0x4c },
        { 0xff2d,   0x4d },
        { 0xff2e,   0x4e },
        { 0xff2f,   0x4f },
        { 0xff30,   0x50 },
        { 0xff31,   0x51 },
        { 0xff32,   0x52 },
        { 0xff33,   0x53 },
        { 0xff34,   0x54 },
        { 0xff35,   0x55 },
        { 0xff36,   0x56 },
        { 0xff37,   0x57 },
        { 0xff38,   0x58 },
        { 0xff39,   0x59 },
        { 0xff3a,   0x5a },
        { 0xff3b,   0x5b },
        { 0xff3c,   0x5c },
        { 0xff3d,   0x5d },
        { 0xff3e,   0x5e },
        { 0xff3f,   0x5f },
        { 0xff40,   0x60 },
        { 0xff41,   0x61 },
        { 0xff42,   0x62 },
        { 0xff43,   0x63 },
        { 0xff44,   0x64 },
        { 0xff45,   0x65 },
        { 0xff46,   0x66 },
        { 0xff47,   0x67 },
        { 0xff48,   0x68 },
        { 0xff49,   0x69 },
        { 0xff4a,   0x6a },
        { 0xff4b,   0x6b },
        { 0xff4c,   0x6c },
        { 0xff4d,   0x6d },
        { 0xff4e,   0x6e },
        { 0xff4f,   0x6f },
        { 0xff50,   0x70 },
        { 0xff51,   0x71 },
        { 0xff52,   0x72 },
        { 0xff53,   0x73 },
        { 0xff54,   0x74 },
        { 0xff55,   0x75 },
        { 0xff56,   0x76 },
        { 0xff57,   0x77 },
        { 0xff58,   0x78 },
        { 0xff59,   0x79 },
        { 0xff5a,   0x7a },
        { 0xff5b,   0x7b },
        { 0xff5c,   0x7c },
        { 0xff5d,   0x7d },
        { 0xff5e,   0x7e },
        { 0xffe0,   0xa2 },
        { 0xffe1,   0xa3 },
        { 0xffe2,   0xac },
        { 0xffe3,   0xaf },
        { 0xffe4,   0xa6 },
        { 0xffe5,   0xa5 },
        { 0xffe8, 0x2502 },
        { 0xffe9, 0x2190 },
        { 0xffea, 0x2191 },
        { 0xffeb, 0x2192 },
        { 0xffec, 0x2193 },
        // arrows
        { 0x21fe, 0x2192 },
        { 0x27f6, 0x2192 },
        { 0x290f, 0x21e2 },
        { 0x2911, 0x21e2 },
        { 0x290d, 0x21e2 },
        { 0x21fd, 0x2190 },
        { 0x27f5, 0x2190 },
        { 0x290e, 0x21e0 },
        { 0x290c, 0x21e0 },
        { 0x21b2, 0x21b5 },
        { 0x21a9, 0x2b90 },
        // WS -1, no-width},-2, no-width-no-breaking
        { 0x180e,     -1 },
        { 0x2000,   0x20 },
        { 0x2001,   0x20 },
        { 0x2002,   0x20 },
        { 0x2003,   0x20 },
        { 0x2004,   0x20 },
        { 0x2005,   0x20 },
        { 0x2006,   0x20 },
        { 0x2007,   0x20 },
        { 0x2008,   0x20 },
        { 0x2009,   0x20 },
        { 0x200a,   0x20 },
        { 0x200b,     -1 },
        { 0x200c,     -1 },
        { 0x200d,     -1 },
        { 0x2060,     -2 },
        { 0x202f,   0x20 },
        { 0x205f,   0x20 },
        { 0x3000,   0x20 },
        { 0xfeff,     -2 }
    };
    std::unordered_map<char32_t, char32_t> substitutes = {
        { 0x1f00,  0x3b1 },
        { 0x1f01,  0x3b1 },
        { 0x1f02,  0x3b1 },
        { 0x1f03,  0x3b1 },
        { 0x1f04,  0x3ac },
        { 0x1f05,  0x3ac },
        { 0x1f06,  0x3b1 },
        { 0x1f07,  0x3b1 },
        { 0x1f08,  0x391 },
        { 0x1f09,  0x391 },
        { 0x1f0a,  0x391 },
        { 0x1f0b,  0x391 },
        { 0x1f0c,  0x386 },
        { 0x1f0d,  0x386 },
        { 0x1f0e,  0x391 },
        { 0x1f0f,  0x391 },
        { 0x1f10,  0x3b5 },
        { 0x1f11,  0x3b5 },
        { 0x1f12,  0x3b5 },
        { 0x1f13,  0x3b5 },
        { 0x1f14,  0x3ad },
        { 0x1f15,  0x3ad },
        { 0x1f18,  0x395 },
        { 0x1f19,  0x395 },
        { 0x1f1a,  0x395 },
        { 0x1f1b,  0x395 },
        { 0x1f1c,  0x388 },
        { 0x1f1d,  0x388 },
        { 0x1f20,  0x3b7 },
        { 0x1f21,  0x3b7 },
        { 0x1f22,  0x3b7 },
        { 0x1f23,  0x3b7 },
        { 0x1f24,  0x3ae },
        { 0x1f25,  0x3ae },
        { 0x1f26,  0x3b7 },
        { 0x1f27,  0x3b7 },
        { 0x1f28,  0x397 },
        { 0x1f29,  0x397 },
        { 0x1f2a,  0x397 },
        { 0x1f2b,  0x397 },
        { 0x1f2c,  0x389 },
        { 0x1f2d,  0x389 },
        { 0x1f2e,  0x397 },
        { 0x1f2f,  0x397 },
        { 0x1f30,  0x3b9 },
        { 0x1f31,  0x3b9 },
        { 0x1f32,  0x3b9 },
        { 0x1f33,  0x3b9 },
        { 0x1f34,  0x3af },
        { 0x1f35,  0x3af },
        { 0x1f36,  0x3b9 },
        { 0x1f37,  0x3b9 },
        { 0x1f38,  0x399 },
        { 0x1f39,  0x399 },
        { 0x1f3a,  0x399 },
        { 0x1f3b,  0x399 },
        { 0x1f3c,  0x38a },
        { 0x1f3d,  0x38a },
        { 0x1f3e,  0x399 },
        { 0x1f3f,  0x399 },
        { 0x1f40,  0x3bf },
        { 0x1f41,  0x3bf },
        { 0x1f42,  0x3bf },
        { 0x1f43,  0x3bf },
        { 0x1f44,  0x3cc },
        { 0x1f45,  0x3cc },
        { 0x1f48,  0x39f },
        { 0x1f49,  0x39f },
        { 0x1f4a,  0x39f },
        { 0x1f4b,  0x39f },
        { 0x1f4c,  0x38c },
        { 0x1f4d,  0x38c },
        { 0x1f50,  0x3c5 },
        { 0x1f51,  0x3c5 },
        { 0x1f52,  0x3c5 },
        { 0x1f53,  0x3c5 },
        { 0x1f54,  0x3ac },
        { 0x1f55,  0x3ac },
        { 0x1f56,  0x3c5 },
        { 0x1f57,  0x3c5 },
        { 0x1f59,  0x3a5 },
        { 0x1f5b,  0x3a5 },
        { 0x1f5d,  0x38e },
        { 0x1f5f,  0x3a5 },
        { 0x1f60,  0x3c9 },
        { 0x1f61,  0x3c9 },
        { 0x1f62,  0x3c9 },
        { 0x1f63,  0x3c9 },
        { 0x1f64,  0x3ce },
        { 0x1f65,  0x3ce },
        { 0x1f66,  0x3c9 },
        { 0x1f67,  0x3c9 },
        { 0x1f68,  0x3a9 },
        { 0x1f69,  0x3a9 },
        { 0x1f6a,  0x3a9 },
        { 0x1f6b,  0x3a9 },
        { 0x1f6c,  0x38f },
        { 0x1f6d,  0x38f },
        { 0x1f6e,  0x3a9 },
        { 0x1f6f,  0x3a9 },
        { 0x1f70,  0x3b1 },
        { 0x1f71,  0x3ac },
        { 0x1f72,  0x3b5 },
        { 0x1f73,  0x3ad },
        { 0x1f74,  0x3b7 },
        { 0x1f75,  0x3ae },
        { 0x1f76,  0x3b9 },
        { 0x1f77,  0x3af },
        { 0x1f78,  0x3bf },
        { 0x1f79,  0x3cc },
        { 0x1f7a,  0x3c5 },
        { 0x1f7b,  0x3cd },
        { 0x1f7c,  0x3c9 },
        { 0x1f7d,  0x3ce },
        { 0x1f80,  0x3b1 },
        { 0x1f81,  0x3b1 },
        { 0x1f82,  0x3b1 },
        { 0x1f83,  0x3b1 },
        { 0x1f84,  0x3ac },
        { 0x1f85,  0x3ac },
        { 0x1f86,  0x3b1 },
        { 0x1f87,  0x3b1 },
        { 0x1f88,  0x391 },
        { 0x1f89,  0x391 },
        { 0x1f8a,  0x391 },
        { 0x1f8b,  0x391 },
        { 0x1f8c,  0x386 },
        { 0x1f8d,  0x386 },
        { 0x1f8e,  0x391 },
        { 0x1f8f,  0x391 },
        { 0x1f90,  0x3b7 },
        { 0x1f91,  0x3b7 },
        { 0x1f92,  0x3b7 },
        { 0x1f93,  0x3b7 },
        { 0x1f94,  0x3ae },
        { 0x1f95,  0x3ae },
        { 0x1f96,  0x3b7 },
        { 0x1f97,  0x3b7 },
        { 0x1f98,  0x397 },
        { 0x1f99,  0x397 },
        { 0x1f9a,  0x397 },
        { 0x1f9b,  0x397 },
        { 0x1f9c,  0x389 },
        { 0x1f9d,  0x389 },
        { 0x1f9e,  0x397 },
        { 0x1f9f,  0x397 },
        { 0x1fa0,  0x3bf },
        { 0x1fa1,  0x3bf },
        { 0x1fa2,  0x3bf },
        { 0x1fa3,  0x3bf },
        { 0x1fa4,  0x3cc },
        { 0x1fa5,  0x3cc },
        { 0x1fa6,  0x3bf },
        { 0x1fa7,  0x3bf },
        { 0x1fa8,  0x39f },
        { 0x1fa9,  0x39f },
        { 0x1faa,  0x39f },
        { 0x1fab,  0x39f },
        { 0x1fac,  0x38c },
        { 0x1fad,  0x38c },
        { 0x1fae,  0x39f },
        { 0x1faf,  0x39f },
        { 0x1fb0,  0x3b1 },
        { 0x1fb1,  0x3b1 },
        { 0x1fb2,  0x3b1 },
        { 0x1fb3,  0x3b1 },
        { 0x1fb4,  0x3ac },
        { 0x1fb6,  0x3b1 },
        { 0x1fb7,  0x3b1 },
        { 0x1fb8,  0x391 },
        { 0x1fb9,  0x391 },
        { 0x1fba,  0x391 },
        { 0x1fbb,  0x386 },
        { 0x1fbc,  0x391 },
        { 0x1fbd, 0x2019 },
        { 0x1fbe,  0x37a },
        { 0x1fbf, 0x2019 },
        { 0x1fc0,  0x2dc },
        { 0x1fc1,  0x2dc },
        { 0x1fc2,  0x3b7 },
        { 0x1fc3,  0x3b7 },
        { 0x1fc4,  0x3ae },
        { 0x1fc6,  0x3b7 },
        { 0x1fc7,  0x3b7 },
        { 0x1fc8,  0x395 },
        { 0x1fc9,  0x388 },
        { 0x1fca,  0x397 },
        { 0x1fcb,  0x389 },
        { 0x1fcc,  0x397 },
        { 0x1fcd,  0x2bd },
        { 0x1fce,  0x384 },
        { 0x1fcf,  0x2dc },
        { 0x1fd0,  0x3b9 },
        { 0x1fd1,  0x3b9 },
        { 0x1fd2,  0x3b9 },
        { 0x1fd3,  0x3af },
        { 0x1fd6,  0x3b9 },
        { 0x1fd7,  0x3b9 },
        { 0x1fd8,  0x399 },
        { 0x1fd9,  0x399 },
        { 0x1fda,  0x399 },
        { 0x1fdb,  0x38a },
        { 0x1fdd,  0x2bd },
        { 0x1fde,  0x2bc },
        { 0x1fdf,  0x2dc },
        { 0x1fe0,  0x3c5 },
        { 0x1fe1,  0x3c5 },
        { 0x1fe2,  0x3c5 },
        { 0x1fe3,  0x3ac },
        { 0x1fe4,  0x3c1 },
        { 0x1fe5,  0x3c1 },
        { 0x1fe6,  0x3c5 },
        { 0x1fe7,  0x3c5 },
        { 0x1fe8,  0x3a5 },
        { 0x1fe9,  0x3a5 },
        { 0x1fea,  0x3a5 },
        { 0x1feb,  0x38e },
        { 0x1fec,  0x3a1 },
        { 0x1fed,  0x2bd },
        { 0x1fee,  0x344 },
        { 0x1fef,  0x2bd },
        { 0x1ff2,  0x3c9 },
        { 0x1ff3,  0x3c9 },
        { 0x1ff4,  0x3ce },
        { 0x1ff6,  0x3c9 },
        { 0x1ff7,  0x3c9 },
        { 0x1ff8,  0x3a9 },
        { 0x1ff9,  0x3a9 },
        { 0x1ffa,  0x3a9 },
        { 0x1ffb,  0x3a9 },
        { 0x1ffc,  0x38f },
        { 0x1ffd,  0x2bc },
        { 0x1ffe,  0x2bc },
        { 0x2160,   0x49 },
        { 0x2164,   0x56 },
        { 0x2169,   0x58 },
        { 0x216c,   0x4c },
        { 0x216d,   0x43 },
        { 0x216e,   0x44 },
        { 0x216f,   0x4d },
        { 0x2170,   0x69 },
        { 0x2174,   0x76 },
        { 0x2179,   0x78 },
        { 0x217c,   0x6c },
        { 0x217d,   0x63 },
        { 0x217e,   0x64 },
        { 0x217f,   0x6d },
        { 0x2186, 0x2193 },
        { 0x2469, 0x24ea },
        { 0x260f, 0x260e },
        { 0x2613,   0xd7 },
        // { 0x2713, 0x23b7 },
        { 0x279b, 0x2192 },
        { 0x279d, 0x2192 },
        { 0x2710, 0x270f },
        { 0x2715,   0xd7 },
        { 0x2716,   0xd7 },
        { 0x2717,   0xd7 },
        { 0x2718,   0xd7 },
        { 0x2719,   0x2b },
        { 0x271a,   0x2b },
        { 0x271b,   0x2b },
        { 0x271c,   0x2b },
        { 0x2729, 0x22c6 },
        { 0x272b, 0x22c6 },
        { 0x272c, 0x22c6 },
        { 0x272d, 0x22c6 },
        { 0x272e, 0x22c6 },
        { 0x272f, 0x22c6 },
        { 0x2731, 0x2055 },
        { 0x2732, 0x2055 },
        { 0x2733, 0x2055 },
        { 0x2734, 0x2055 },
        { 0x2735, 0x2055 },
        { 0x2736, 0x2055 },
        { 0x2764, 0x2665 },
        { 0x274c,   0xd7 },
        { 0x2501, 0x2500 },
        { 0x2503, 0x2502 },
        { 0x250f, 0x250c },
        { 0x2513, 0x2510 },
        { 0x2517, 0x2514 },
        { 0x251b, 0x2518 },
        { 0x2523, 0x251c },
        { 0x252b, 0x2524 },
        { 0x2533, 0x252c },
        { 0x253b, 0x2534 },
        { 0x254b, 0x253c },
        { 0x2578, 0x2574 },
        { 0x2579, 0x2575 },
        { 0x257a, 0x2576 },
        { 0x257b, 0x2577 },
        { 0x2747, 0x2055 },
        { 0x2748, 0x2055 },
        { 0x2749, 0x2055 },
        { 0x274a, 0x2055 },
        { 0x274b, 0x2055 },
        { 0x274d, 0x25cb },
        { 0x274f, 0x25a1 },
        { 0x2750, 0x25a1 },
        { 0x2751, 0x25a1 },
        { 0x2752, 0x25a1 }
    };

    FontDataBits::DataStruct* pData = FontDataBits::getBits();

#if defined(_DEBUG) && defined(_WIN32)
    // export font
    #if 0
    BDFExport bdf;
    
    bdf.writeBDF(pData, "C:\\Temp\\ba67.bdf", 8, 16);
    bdf.writeBDF(pData, "C:\\Temp\\ba67-square.bdf", 8, 8);
    #endif
#endif

    while (pData->c != 0xffffffff) {
        CharBitmap bmp(pData->b, 8);
        if (pData->c < ascii.size()) {
            std::swap(ascii[pData->c], bmp);
        } else {
            if (pData->c >= from && pData->c <= to) {
                this->at(pData->c) = bmp;
            }
        }
        ++pData;
    }


#if 0
    for (auto& c : synonyms) {
        if (c.second < 0) {
            continue;
        }
        if (c.first >= from && c.first <= to) {
            this->at(c.first) = (*this)[c.second];
        }
    }
    for (auto& c : substitutes) {
        if (c.second < 0) {
            continue;
        }
        if (c.first >= from && c.first <= to) {
            this->at(c.first) = (*this)[c.second];
        }
    }
#endif
}
