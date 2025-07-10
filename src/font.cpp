#include "font.h"
#include "screen_buffer.h"
#include <unordered_map>
#include "unicode.h"
#include "petscii.h"

// @preserve
// char8.js v.1.2.1, (c) 2015-2019 Norbert Landsteiner, www.masswerk.at
// Virtual character generator based on PET/MZ80 and SuperPET charsets.
// Implements:
// Latin + Latin-1 Supplement (ISO 8859-1), Latin Extended-A, Latin Extended-B,
// Hiragana, Katakana, Greek, common Cyrillic, essential Math, APL, tech,
// punctations, arrows, block elements, frames, OCR and more,
// mappings for halfwidth and fullwidth characters, spaces and punctations.
// Block elements PET bitmaps modified to match standard Unicode glyphs.
// private (0xe000-0xe017): additional PET and MZ80 glyphs.
// see https://www.masswerk.at/char8 for a full list and usage notes.
//
// v.1.1: Includes arrows and graphics for legacy computing (proposal)
// according to https://www.unicode.org/L2/L2019/19025-terminals-prop.pdf
//
// v.1.2: unescapeHTML now returns U+FFFD for unresolved numeric entities.
namespace Font {



static void fillPetCatMap(std::unordered_map<std::string, char32_t>& petcatMap) {
    static const char* digits = "0123456789abcdef";

    for (int n = 0; n < 256; ++n) {
        size_t hex_len = 2;
        std::string hex(hex_len, '0');
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            hex[i] = digits[(n >> j) & 0x0f];

        size_t dec_len = 3;
        std::string dec(dec_len, '0');
        dec[2] = '0' + (n % 10);
        dec[1] = '0' + ((n / 10) % 10);
        dec[0] = '0' + ((n / 100) % 10);


        petcatMap["{$" + hex + "}"] = PETSCII::toUnicode(n);
        petcatMap["{" + dec + "}"]  = PETSCII::toUnicode(n);
    }

    /* keys for char-codes 0xa0-0xdf */
    static const char* cbmkeys[0x40] = {
        "SHIFT-SPACE", "CBM-K", "CBM-I", "CBM-T", "CBM-@", "CBM-G", "CBM-+", "CBM-M",
        "CBM-POUND", "SHIFT-POUND", "CBM-N", "CBM-Q", "CBM-D", "CBM-Z", "CBM-S", "CBM-P",
        "CBM-A", "CBM-E", "CBM-R", "CBM-W", "CBM-H", "CBM-J", "CBM-L", "CBM-Y",
        "CBM-U", "CBM-O", "SHIFT-@", "CBM-F", "CBM-C", "CBM-X", "CBM-V", "CBM-B",
        "SHIFT-*", "SHIFT-A", "SHIFT-B", "SHIFT-C", "SHIFT-D", "SHIFT-E", "SHIFT-F", "SHIFT-G",
        "SHIFT-H", "SHIFT-I", "SHIFT-J", "SHIFT-K", "SHIFT-L", "SHIFT-M", "SHIFT-N", "SHIFT-O",
        "SHIFT-P", "SHIFT-Q", "SHIFT-R", "SHIFT-S", "SHIFT-T", "SHIFT-U", "SHIFT-V", "SHIFT-W",
        "SHIFT-X", "SHIFT-Y", "SHIFT-Z", "SHIFT-+", "CBM--", "SHIFT--", "SHIFT-^", "CBM-*"
    };
    for (int i = 0; i < sizeof(cbmkeys) / sizeof(cbmkeys[0]); ++i) {
        const char* it = cbmkeys[i];
        if (*it == '\0') {
            continue;
        }
        // these are at $a0, but also at $e0
        petcatMap["{" + std::string(it) + "}"] = PETSCII::toUnicode(i + 0xa0);
    }
    // BA67 stores the pound sign at $a3 (as does unicode)
    petcatMap["{CBM-T}"] = PETSCII::toUnicode(0xe3); // upper one eights bar
    petcatMap["{$a3}"]   = PETSCII::toUnicode(0xe3);

    petcatMap["SHIFT-^"] = PETSCII::toUnicode(0x7e); // pi
    petcatMap["$ff"]     = PETSCII::toUnicode(0x7e);

    petcatMap["CBM-*"] = PETSCII::toUnicode(0x7f); // upper right triangle
    petcatMap["$df"]   = PETSCII::toUnicode(0x7f);

    petcatMap["SHIFT-@"] = PETSCII::toUnicode(0xba); // lower right blocks
    petcatMap["$fa"]     = PETSCII::toUnicode(0xba);


    /* 0x00 - 0x1f (petcat) */
    static const char* ctrl1[0x20] = {
        "", "CTRL-A", "CTRL-B", "stop", "CTRL-D", "wht", "CTRL-F", "CTRL-G",
        "dish", "ensh", "\n", "CTRL-K", "CTRL-L", "\n", "swlc", "CTRL-O",
        "CTRL-P", "down", "rvon", "home", "del", "CTRL-U", "CTRL-V", "CTRL-W",
        "CTRL-X", "CTRL-Y", "CTRL-Z", "esc", "red", "rght", "grn", "blu"
    };
    for (int i = 0; i < sizeof(ctrl1) / sizeof(ctrl1[0]); ++i) {
        const char* it = ctrl1[i];
        if (*it == '\0') {
            continue;
        }
        petcatMap["{" + std::string(it) + "}"] = PETSCII::toUnicode(i + 0x00);
    }

    /* 0x80 - 0x9f (petcat) */
    static const char* ctrl2[0x20] = {
        "", "orng", "", "", "", "f1", "f3", "f5",
        "f7", "f2", "f4", "f6", "f8", "sret", "swuc", "",
        "blk", "up", "rvof", "clr", "inst", "brn", "lred", "gry1",
        "gry2", "lgrn", "lblu", "gry3", "pur", "left", "yel", "cyn"
    };
    for (int i = 0; i < sizeof(ctrl2) / sizeof(ctrl2[0]); ++i) {
        const char* it = ctrl2[i];
        if (*it == '\0') {
            continue;
        }
        petcatMap["{" + std::string(it) + "}"] = PETSCII::toUnicode(i + 0x80);
    }

    // BASIC 3.5 specials
    petcatMap["{dblu}"] = petcatMap["{blu}"];
    petcatMap["{flon}"] = PETSCII::toUnicode(130);
    petcatMap["{flof}"] = PETSCII::toUnicode(132);
    petcatMap["{help}"] = petcatMap["{f8}"];
    petcatMap["{pink}"] = petcatMap["{lred}"];
    petcatMap["{blgn}"] = petcatMap["{lgrn}"];

    // Original TOK64 program
    petcatMap["{RVS ON}"]     = petcatMap["{rvon}"];
    petcatMap["{reverse on}"] = petcatMap["{rvon}"];
    petcatMap["{REVERSE ON}"] = petcatMap["{rvon}"];
    petcatMap["{RVSON}"]      = petcatMap["{rvon}"];

    petcatMap["{RVS OFF}"]     = petcatMap["{rvof}"];
    petcatMap["{reverse off}"] = petcatMap["{rvof}"];
    petcatMap["{REVERSE OFF}"] = petcatMap["{rvof}"];
    petcatMap["{RVSOFF}"]      = petcatMap["{rvof}"];

    petcatMap["{clear}"]        = petcatMap["{clr}"];
    petcatMap["{right}"]        = petcatMap["{rght}"];
    petcatMap["{white}"]        = petcatMap["{wht}"];
    petcatMap["{black}"]        = petcatMap["{blk}"];
    petcatMap["{green}"]        = petcatMap["{grn}"];
    petcatMap["{blue}"]         = petcatMap["{blu}"];
    petcatMap["{orange}"]       = petcatMap["{orng}"];
    petcatMap["{shift return}"] = petcatMap["{sret}"];
    petcatMap["{upper case}"]   = petcatMap["{swuc}"];
    petcatMap["{brown}"]        = petcatMap["{brn}"];
    petcatMap["{lt.red}"]       = petcatMap["{lred}"];
    petcatMap["{lig.red}"]      = petcatMap["{lred}"];
    petcatMap["{dark gray}"]    = petcatMap["{gry1}"];
    petcatMap["{grey1}"]        = petcatMap["{gry1}"];
    petcatMap["{grey 1}"]       = petcatMap["{gry2}"];
    petcatMap["{grey2}"]        = petcatMap["{gry2}"];
    petcatMap["{grey 2}"]       = petcatMap["{gry2}"];
    petcatMap["{lt green}"]     = petcatMap["{lgrn}"];
    petcatMap["{light green}"]  = petcatMap["{lgrn}"];
    petcatMap["{lig.green}"]    = petcatMap["{lgrn}"];
    petcatMap["{light blue}"]   = petcatMap["{lblu}"];
    petcatMap["{lt.blue}"]      = petcatMap["{lblu}"];
    petcatMap["{lig.blue}"]     = petcatMap["{lblu}"];
    petcatMap["{light gray}"]   = petcatMap["{gry3}"];
    petcatMap["{grey3}"]        = petcatMap["{gry3}"];
    petcatMap["{grey 3}"]       = petcatMap["{gry3}"];
    petcatMap["{purple}"]       = petcatMap["{pur}"];
    petcatMap["{yellow}"]       = petcatMap["{yel}"];
    petcatMap["{cyan}"]         = petcatMap["{cyn}"];
    petcatMap["{CBM-^}"]        = petcatMap["{SHIFT-^}"];
    petcatMap["{space}"]        = U' ';
}

static int my_strnicmp(const char* s1, const char* s2, size_t n) {
    unsigned char c1, c2;

    while (n-- > 0) {
        c1 = *s1++;
        c2 = *s2++;

        // Convert both characters to lowercase manually
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 += 'a' - 'A';
        }
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 += 'a' - 'A';
        }

        if (c1 != c2) {
            return c1 - c2;
        }
        if (c1 == '\0') {
            return 0;
        }
    }

    return 0;
}

// parse the next character and escape the petcat {$xx} strings
// return Unicode representation
char32_t parseNextPetcat(const char*& str) {
    static std::unordered_map<std::string, char32_t> petcatMap;
    if (petcatMap.empty()) {
        fillPetCatMap(petcatMap);
    }

    if (*str == '{') {
        for (auto mp : petcatMap) {
            size_t len = mp.first.length();
            if (my_strnicmp(str, mp.first.c_str(), len) == 0) {
                str += len;
                return mp.second;
            }
        }
#if _DEBUG
        *(int*)0 = 0;
#endif
        while (*str != '}' && *str != '\0') {
            ++str;
        }
        return U'?';
    }


    char32_t ch = Unicode::parseNextUtf8(str);

    /* petcat prints lowercase characters. In that font, upper/lowercase is switched from ASCII */
    if (ch >= 'a' && ch <= 'z') {
        return char32_t(ch + 'A' - 'a');
    }
    if (ch >= 'A' && ch <= 'Z') {
        // the upper case letters are graphics characters
        // in default font. but they are repeated at $C0
        return PETSCII::toUnicode(ch + 0xc0 - 0x40);
    }

    return ch;
}



#if 0
struct petcatTest {
    petcatTest() {
        static const char* digits = "0123456789abcdef";
        std::string test          = "1REMBA67 PETCAT\n";
        for (int i = 0; i < 16; ++i) {
            char ln[128] = { 0 };
            itoa((1 + i) * 10, ln, 10);

            test += std::string(ln) + " PRINT \"";
            for (int j = 0; j < 16; ++j) {
                int n          = i * 16 + j;
                size_t hex_len = 2;
                std::string hex(hex_len, '0');
                for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
                    hex[i] = digits[(n >> j) & 0x0f];
                }

                test += "{$" + hex + "}";
            }
            test += "\"\n";
        }
    }
} petcatTttt;
#endif


} // namespace Font