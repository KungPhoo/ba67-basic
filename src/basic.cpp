#include "basic.h"
#include "about.h"
#include "help.h"
#include "os.h"
#include "string_helper.h"
#include "unicode.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <regex>
#include <variant>
#include "prg_tool.h"
#include "minifetch.h"
#include "petscii.h"
#include "control_characters.h"

#include "rawconfig.h"
#include "rom.h"


// TODO try 10 DIM JU(13): FOR I = 0 TO 13 : READ JU(I) : NEXT : PRINT JU(I): DATA 5, 4, 3, 2, 1, 1, 0, 0, -1, -1, -2, -3, -4, -5





#if defined(__cplusplus) && __cplusplus > 202002L
    #include <format>
#else
    #include <iomanip> // std::setprecision()
#endif

#define ASSERT(a)                                         \
    {                                                     \
        if (!(a)) {                                       \
            throw Basic::Error(Basic::ErrorId::INTERNAL); \
        }                                                 \
    }

// Built In Commands
void cmdABOUT(Basic* basic, const std::vector<Basic::Value>&) {

    std::string raw = about::text();
    StringHelper::replace(raw, "{VERSION}", Basic::version());
    StringHelper::replace(raw, "{BUILD}", Basic::buildVersion());

    const char* utf8 = raw.c_str();
    for (;;) {
        char32_t c = Unicode::parseNextUtf8(utf8);
        if (c == 0) {
            break;
        }


        basic->os->screen.setTextColor(1); // white
        basic->os->screen.putC(c);
        basic->os->presentScreen();
        basic->os->delay(16);
        basic->os->screen.setTextColor(13); // lt green
        basic->handleEscapeKey();
        if (c != U'\r' && c != '\n') {
            basic->os->screen.moveCursorPos(-1, 0);
            basic->os->screen.putC(c);
        }
    }

    return;



    auto lines   = StringHelper::split(raw, "\r\n");
    int ntoPause = 23;
    for (auto& ln : lines) {
        if (ln == ".") {
            ln.clear();
        }
        basic->printUtf8String(ln + "\n");
        if (--ntoPause < 0) {
            basic->os->presentScreen();
            ntoPause = 23;
            basic->waitForKeypress();
        }
    }
}

void cmdAUTO(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.empty() || basic->valueToInt(values[0]) < 1) {
        basic->currentModule().autoNumbering = 0;
    }
    basic->currentModule().autoNumbering = int32_t(basic->valueToInt(values[0]));
}


void bakePRGtoC64(Basic* basic) {
    std::string all;
    for (auto& ln : basic->currentModule().listing) {
        if (ln.first < 0) {
            continue;
        }
        all += std::to_string(ln.first) + " " + ln.second.code + "\n";
    }

    std::vector<std::pair<int, std::string>> errorDetails;
    auto prg = PrgTool::BASICtoPRG(all.c_str(), &errorDetails);

    size_t address = krnl.BASICCODE;
    for (size_t i = 2; i < prg.size(); ++i) {
        basic->memory[address++] = prg[i]; // skip 2 byte loading address header
        if (address >= krnl.BASICEND) {
            throw Basic::ErrorId::OUT_OF_MEMORY;
        }
    }
    basic->printUtf8String("BAKE TO $" + StringHelper::int2hex(krnl.BASICCODE, true, 2) + "-$" + StringHelper::int2hex(address, true, 2) + "\n");

    // BASIC warm-start clears the variables and BASIC program here
    basic->memory[0xA4ED + 0] = 0xEA; // NOP
    basic->memory[0xA4ED + 1] = 0xEA; // NOP
    basic->memory[0xA4ED + 2] = 0xEA; // NOP
}



void cmdBAKE(Basic* basic, const std::vector<Basic::Value>& values) {

    bool bakeToC64 = false;
    if (values.size() > 0 && basic->valueIsInt(values[0]) && basic->valueToInt(values[0]) == 64) {
        bakeToC64 = true;
    }


    basic->currentModule().forceTokenizing();
    auto& listing = basic->currentModule().listing;

    std::map<std::string, int> labelMap;
    std::regex labelRegex(R"(REM\s+--(\w+)--)");
    std::regex gotoLabelRegex(R"(\b(GOTO|GOSUB)\s+(\d*)([A-Z_]+)\b)");

    // First pass: collect labels
    for (const auto& [lineNumber, prgline] : listing) {
        std::smatch match;
        if (std::regex_search(prgline.code, match, labelRegex)) {
            std::string label = match[1];
            labelMap[label]   = lineNumber;
        }
    }

    // Second pass: rewrite GOTO/GOSUB statements
    for (auto itr = listing.begin(); itr != listing.end(); ++itr) { // auto& [lineNumber, line] : listing) {
        int lineNumber    = itr->first;
        std::string& line = itr->second;

        std::smatch match;
        std::string updatedLine = line;
        size_t offset           = 0;
        // Use regex iterator to replace multiple label refs in one line
        auto begin = std::sregex_iterator(line.begin(), line.end(), gotoLabelRegex);
        auto end   = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            size_t matchPos = it->position() + offset;
            size_t matchLen = it->length();

            bool inString  = false;
            bool inComment = false;
            for (size_t i = 0; i < matchPos; ++i) {
                if (updatedLine[i] == '\"') {
                    inString = !inString;
                }
                if (!inString && updatedLine.substr(i, 3) == "REM") {
                    inComment = true;
                    break;
                }
            }
            if (inString || inComment) {
                continue;
            }


            std::string command     = (*it)[1];
            std::string numberPart  = (*it)[2];
            std::string label       = (*it)[3];
            std::string matchedText = it->str(); // The exact text matched in the line

            if (labelMap.count(label)) {
                std::string newToken = command + " " + std::to_string(labelMap[label]) + label;
                updatedLine.replace(it->position() + offset, matchedText.length(), newToken);
                offset += newToken.length() - matchedText.length();
            } else {
                basic->programCounter().line   = itr;
                basic->programCounter().cmdpos = 0;
                throw Basic::Error(Basic::ErrorId::UNDEFD_STATEMENT);
            }
        }
        line = updatedLine;
    }

    if (bakeToC64) {
        bakePRGtoC64(basic);
    }
}

void cmdCHAR(Basic* basic, const std::vector<Basic::Value>& values) {
    auto curPos = basic->os->screen.getCursorPos();
    int color = 0, x = int(curPos.x), y = int(curPos.y);
    std::string text;
    bool inverse = false;

    if (values.size() < 4) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int ipara = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        auto& v = values[i];
        if (basic->valueIsOperator(v)) {
            ++ipara;
            continue;
        }
        switch (ipara) {
        case 0:
            color = int(Basic::valueToInt(v));
            break;
        case 1:
            x = int(Basic::valueToInt(v));
            break;
        case 2:
            y = int(Basic::valueToInt(v));
            break;
        case 3:
            text = Basic::valueToString(v);
            break;
        case 4:
            inverse = Basic::valueToInt(v) != 0;
            break;
        default:
            throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
        }
    }

    int screenwidth = int(basic->os->screen.width);
    while (x > screenwidth) {
        ++y;
        x -= screenwidth;
    }

    if (x < 0 || y < 0 || y > basic->os->screen.width) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    ScreenBuffer::Cursor cr;
    cr.x = x;
    cr.y = y;
    basic->os->screen.setCursorPos(cr);
    if (inverse) {
        basic->os->screen.setReverseMode(true);
    }
    if (color > 0) {
        if (color > 16) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }

        int old = basic->os->screen.getTextColor();
        basic->os->screen.setTextColor(color - 1);
        color = old;
    }

    basic->printUtf8String(text, true /* apply control characters */);
    // const char* utf8 = text.c_str();
    // while (*utf8 != '\0') {
    //     basic->os->screen.putC(Unicode::parseNextUtf8(utf8));
    // }

    // No. The set color remains
    // if (color > 0) {
    //     // restore old color
    //     basic->os->screen.setTextColor(color);
    // }

    // inverse flag is only for this string
    if (inverse) {
        basic->os->screen.setReverseMode(false);
    }
    basic->os->presentScreen();
}

void cmdCHDIR(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string dir = basic->valueToString(values[0]);
    dir             = basic->os->findFirstFileNameWildcard(dir, true);

    if (!basic->os->setCurrentDirectory(dir)) {
        throw Basic::Error(Basic::ErrorId::FILE_NOT_FOUND);
    }
}

void cmdCOLOR(Basic* basic, const std::vector<Basic::Value>& values) {
    int colorsource = 1, color = 1;

    if (values.size() < 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    int ipara = 0;
    uint8_t red, green, blue;
    for (size_t i = 0; i < values.size(); ++i) {
        auto& v = values[i];
        if (basic->valueIsOperator(v)) {
            ++ipara;
            continue;
        }
        switch (ipara) {
        case 0:
            colorsource = int(Basic::valueToInt(v));
            break;
        case 1:
            color = int(Basic::valueToInt(v));
            red   = uint8_t(color);
            break;
        case 2: /*optional brightness*/
            green = uint8_t(Basic::valueToInt(v));
            break;
        case 3: /*optional brightness*/
            blue = uint8_t(Basic::valueToInt(v));
            break;
        default:
            throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
        }
    }

    if (ipara == 3) {
        color = colorsource;
        if (color > 16 || color < 1) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }
        basic->os->screen.defineColor(color - 1, red, green, blue);
    } else {
        if (color > 16 || color < 1) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }
        switch (colorsource) {
        case 0:
        case 6:
            basic->os->screen.setBackgroundColor(color - 1);
            break;
        case 1:
        case 5:
            basic->os->screen.setTextColor(color - 1);
            break;
        case 4:
            basic->os->setBorderColor(color - 1);
            break;
        }
    }
}

void cmdCHARDEF(Basic* basic, const std::vector<Basic::Value>& values) {
    // TODO #error ARM is big endian?
    char32_t codePoint = 0;
    uint64_t bytes8;
    static std::array<uint8_t, 32> bytes;
    static std::array<uint32_t, 16> dwords;
    size_t iarg = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        if (iarg > 16) {
            throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
        }
        const Basic::Value& v = values[i];
        if (basic->valueIsOperator(v)) {
            continue;
        }
        if (iarg == 0) {
            std::string s = Basic::valueToString(v);
            if (!s.empty()) {
                const char* pc = s.c_str();
                codePoint      = Unicode::parseNextUtf8(pc);
                ;
            }
        } else {
            bytes8           = uint64_t(Basic::valueToInt(v));
            bytes[iarg - 1]  = (uint8_t(bytes8));
            dwords[iarg - 1] = (uint32_t(bytes8));
        }
        ++iarg;
    }

    if (iarg != 1 + 1 && iarg != 8 + 1 && iarg != 16 + 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    // one 64 bit integer - 8x8 mono
    if (iarg == 1 + 1) {
        dwords[0] = 0;
        for (size_t i = 0; i < 8; ++i) {
            bytes[7 - i] = uint8_t((bytes8 >> (8 * i)) & 0xff);
        }
        iarg = 9;
    } else if (iarg == 8 + 1 && ScreenInfo::charPixY == 8) {
        bool isMulti = false;
        for (size_t i = 0; i < iarg - 1; ++i) {
            if (dwords[i] > 0xff) {
                isMulti = true;
                break;
            }
        }
        if (isMulti) {
            size_t ib = 0;
            for (size_t i = 0; i < iarg - 1; ++i) {
                bytes[ib++] = (dwords[i] & 0xff000000) >> 24;
                bytes[ib++] = (dwords[i] & 0x00ff0000) >> 16;
                bytes[ib++] = (dwords[i] & 0x0000ff00) >> 8;
                bytes[ib++] = (dwords[i] & 0x000000ff) >> 0;
            }
            iarg = 33;
        }
    }

    basic->os->screen.defineChar(codePoint, CharBitmap(&bytes[0], iarg - 1));
}

void cmdSPRDEF(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int id = int(basic->valueToInt(values[0]));
    if (id < 1 || id > basic->os->screen.sprites.size()) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    std::string str = basic->valueToString(values[2]);
    if (Unicode::utf8StrLen(str) > 6) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }

    auto& sprite   = basic->os->screen.sprites[id];
    const char* pc = str.c_str();
    for (size_t i = 0; i < 6; ++i) {
        char32_t c        = Unicode::parseNextUtf8(pc);
        sprite.charmap[i] = c;
    }
    if (Unicode::parseNextUtf8(pc) != 0) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }

    basic->os->screen.dirtyFlag = true;
}

void cmdSPRITE(Basic* basic, const std::vector<Basic::Value>& values) {
    // `SPRITE nr, on, color, prio, x2, y2`
    Sprite* sprite = nullptr;

    int ipara = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        auto& v = values[i];
        if (basic->valueIsOperator(v)) {
            ++ipara;
            continue;
        }
        int64_t iv = Basic::valueToInt(v);
        switch (ipara) {
        case 0:
            if (iv < 1 || iv > int64_t(basic->os->screen.sprites.size())) {
                throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
            }
            sprite = &basic->os->screen.sprites[iv];
            break;
        case 1: sprite->enabled = (iv != 0); break;
        case 2: sprite->color = iv & 0x0f; break;
        case 3: break; // prio
        case 4: sprite->stretchX = (iv != 0); break;
        case 5: sprite->stretchY = (iv != 0); break;
        default:
            throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
        }
    }
    basic->os->screen.dirtyFlag = true;
}

void cmdMOVSPR(Basic* basic, const std::vector<Basic::Value>& values) {
    Sprite* sprite = nullptr;

    int ipara = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        auto& v = values[i];
        if (basic->valueIsOperator(v)) {
            ++ipara;
            continue;
        }
        int64_t iv = Basic::valueToInt(v);
        switch (ipara) {
        case 0:
            if (iv < 1 || iv > int64_t(basic->os->screen.sprites.size())) {
                throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
            }
            sprite = &basic->os->screen.sprites[iv];
            break;
        case 1:  sprite->x = iv; break;
        case 2:  sprite->y = iv; break;
        default: throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
        }
    }
    basic->os->screen.dirtyFlag = true;
}
void cmdMONITOR(Basic* basic, const std::vector<Basic::Value>& values) {
    basic->monitor();
}

void cmdQUIT(Basic* basic, const std::vector<Basic::Value>& values) {
    int code = 0;
    if (values.size() == 1) {
        code = int(Basic::valueToInt(values[0]));
    }
    exit(code);
}

void cmdFIND(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    std::u32string find32;
    Unicode::toU32String(
        Unicode::toLowerAscii(
            ("*" + basic->valueToString(values[0]) + "*").c_str())
            .c_str(),
        find32);

    basic->os->delay(1000);
    std::string sline;
    std::u32string u32;
    for (auto& ln : basic->currentModule().listing) {
        if (ln.first < 0) {
            continue;
        }
        Unicode::toU32String(Unicode::toLowerAscii(ln.second.code.c_str()).c_str(), u32);

        if (!Unicode::wildcardMatch(u32.c_str(), find32.c_str())) {
            continue;
        }

        sline = std::to_string(ln.first);

        for (size_t s = sline.length(); s < 3; ++s) {
            sline += ' ';
        }
        sline += ' ';
        sline += ln.second;
        sline += '\n';

        basic->os->screen.cleanCurrentLine();
        basic->printUtf8String(sline);
        basic->handleEscapeKey(true);
        basic->os->delay(50);
    }
}

void cmdSYS(Basic* basic, const std::vector<Basic::Value>& values);
void cmdGO(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t where = int(Basic::valueToInt(values[0]));

    if (where == 64) {
        if (basic->AreYouSureQuestion()) {

            // GO 64
            // prepare the original C64 font
            // copy the BA67 font to the CHARROM memory.
            // --this is never used. BA67 takes the PRINT-ing role--
            for (size_t i = 0; i < 0x100; ++i) {
                char32_t unicode = PETSCII::toUnicode(uint8_t(i));

                const CharBitmap& bmp = basic->os->screen.getCharDefinition(unicode);
                basic->os->screen.defineChar(char32_t(i), bmp);
            }

            // basic->os->screen.setBackgroundColor(6);
            // basic->os->screen.setTextColor(14);
            // basic->os->screen.setBorderColor(14);
            // basic->os->screen.clear();
            // basic->printUtf8String("\n    **** COMMODORE 64 BASIC V2 ****     "
            //                        "\n                                        "
            //                        "\n 64K RAM SYSTEM - BA67 EMULATION MODE   "
            //                        "\n");
            // cmdSYS(basic, { 0xA474 }); // print BASIC "READY." and go

            cmdSYS(basic, { 0xFCE2 }); // 64738 system cold start
            basic->restoreColorsAndCursor(true);
        }
    } else {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
}

void cmdGRAPHIC(Basic* basic, const std::vector<Basic::Value>& values) {
    int code = 0;
    if (values.size() == 1) {
        code = int(Basic::valueToInt(values[0]));
        if (code == 5) {
            basic->os->screen.setSize(80, 25);
        } else {
            basic->os->screen.setSize(40, 25);
        }
    }
}


void cmdLOAD(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string path = basic->valueToString(values[0]);
    // if (!basic->fileExists(path, true)) {
    //     throw Basic::Error(Basic::ErrorId::FILE_NOT_FOUND);
    // }

    basic->os->screen.cleanCurrentLine();
    basic->printUtf8String("LOADING\n");
    if (!basic->loadProgram(path)) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
    }
    basic->currentModule().setProgramCounterToEnd();
    basic->currentModule().forceTokenizing();
    basic->currentModule().filenameQSAVE = path;
}

void cmdSAVE(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string filename = basic->valueToString(values[0]);
    if (basic->fileExists(filename, false)) {
        basic->os->screen.cleanCurrentLine();
        basic->printUtf8String("FILE EXISTS. OVERWRITE (Y/N)?");
        std::string yesno = basic->inputLine(false);
        if (yesno.length() == 0 || Unicode::toUpperAscii(yesno[0]) != u'Y') {
            return;
        }
    }

    basic->os->screen.cleanCurrentLine();
    basic->printUtf8String("SAVING\n");
    if (!basic->saveProgram(filename)) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
    }

    basic->currentModule().filenameQSAVE = filename;
}


void cmdQSAVE(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 0) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string filename = basic->currentModule().filenameQSAVE;
    if (filename.empty()) {
        throw Basic::Error(Basic::ErrorId::VARIABLE_UNDEFINED);
    }
    basic->os->screen.cleanCurrentLine();
    basic->printUtf8String("SAVE \"");
    basic->printUtf8String(filename);
    basic->printUtf8String("\" \n");
    cmdSAVE(basic, { filename });
}


void cmdBLOAD(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string path    = basic->valueToString(values[0]);
    size_t startAddress = size_t(basic->valueToInt(values[2]));


    FilePtr file(basic->os);
    file.open(path, "rb");
    if (!file) {
        basic->memory[krnl.STATUS] = Basic::FS_ERROR_READ;
        throw Basic::Error(Basic::ErrorId::FILE_NOT_FOUND);
    }
    auto bytes = file.readAll();
    file.close();

    for (size_t i = 0; i < bytes.size(); ++i) {
        if (startAddress < 0 || startAddress >= basic->memory.size()) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }
        basic->memory[startAddress++] = bytes[i];
    }
}

void cmdBSAVE(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 5) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string path    = basic->valueToString(values[0]);
    size_t startAddress = size_t(basic->valueToInt(values[2]));
    size_t endAddress   = size_t(basic->valueToInt(values[4]));

    if (startAddress < 0 || startAddress >= endAddress || endAddress > basic->memory.size()) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }

    FilePtr file(basic->os);
    file.open(path, "wb");
    if (!file) {
        basic->memory[krnl.STATUS] = Basic::FS_ERROR_WRITE;
        throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
    }
    for (size_t i = startAddress; i < endAddress; ++i) {
        uint8_t b = uint8_t(basic->memory[i] & 0xff);
        file.write(&b, 1);
    }
    file.close();
}


void cmdSCRATCH(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    std::string filename = basic->valueToString(values[0]);

    for (int ifile = 0; ifile < 999; ++ifile) {
        std::string fn = basic->os->findFirstFileNameWildcard(filename);
        if (!basic->fileExists(fn, false)) {
            if (ifile == 0) {
                throw Basic::Error(Basic::ErrorId::FILE_NOT_FOUND);
            }
            break;
        }

        if (basic->currentModule().isInDirectMode()) {
            basic->os->screen.cleanCurrentLine();
            basic->printUtf8String("SCRATCH ");
            basic->printUtf8String(fn);
            if (!basic->AreYouSureQuestion()) {
                break;
            }
        }

        if (!basic->os->scratchFile(fn)) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
        }
    }
}

// TODO: OPEN no, drive(8,9,...), !!15!!: direct mode "S:file" = scratch
void cmdOPEN(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() < 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    std::vector<int64_t> intparams = { 0, 1, 0 };
    std::string path;

    size_t iint = 0;
    for (auto& v : values) {
        if (basic->valueIsString(v)) {
            path = basic->valueToString(v);
            break;
        } else if (basic->valueIsInt(v) || basic->valueIsDouble(v)) {
            if (iint == 3) {
                throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
            }
            intparams[iint++] = basic->valueToInt(v);
        }
    }

    int64_t ifile     = intparams[0];
    int64_t device    = intparams[1];
    int64_t secondary = intparams[2];

    if (ifile < 1 || size_t(ifile) >= basic->fileHandles.size()) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    // file slot is already open
    if (basic->fileHandles[ifile]) {
        basic->memory[krnl.STATUS] = Basic::FS_DEVICE_ERROR;
        throw Basic::Error(Basic::ErrorId::FILE_OPEN);
    }

    // printer -> stdout and stderr
    switch (device) {
    case 0: // Inter Process Communication
    {
#if defined(__EMSCRIPTEN__)
        throw Basic::Error(Basic::ErrorId::UNIMPLEMENTED_COMMAND);
#endif

        IPC::Options opt = {};

        std::string cmdpath(path);
        if (secondary < 0) {
            opt.mode = IPC::IPC_MODE::PIPE;
            cmdpath  = basic->os->findFirstFileNameWildcard(cmdpath);
        } else {
            opt.mode     = IPC::IPC_MODE::TCP;
            opt.hostname = cmdpath;
            opt.port     = 6502;
            if (secondary > 0) {
                opt.port = secondary & 0xffff;
            }
        }


        basic->fileHandles[ifile].openIPC(opt);
        break;
    }
    case 4:  basic->fileHandles[ifile].openStdIn(); break;
    case 5:  basic->fileHandles[ifile].openStdErr(); break;
    case 6:  basic->fileHandles[ifile].openStdOut(); break;
    default: {
        // assume disk device
        const char* iomode = "r";

        std::string filepath(path);
        // std::string filepath   = basic->valueToString(values[2]); // "name*, R", "name, W"


        std::string fileExt = "";
        for (int iComma = 0; iComma < 2; ++iComma) {
            size_t comma = filepath.rfind(',');
            if (comma == std::string::npos) {
                break;
            }

            std::string strpara = filepath.substr(comma + 1);
            filepath            = filepath.substr(0, comma);
            StringHelper::trimRight(filepath, " ");
            StringHelper::trimLeft(strpara, " ");
            if (strpara.empty()) {
                throw Basic::Error(Basic::ErrorId::SYNTAX);
            }

            switch (iComma) {
            case 1: // TYPE PRG,SEQ,USR,REL
            {
                char t = char(Unicode::toUpperAscii(char32_t(strpara[0])));
                switch (t) {
                case 'P': fileExt = ".PRG"; break;
                case 'S': fileExt = ".SEQ"; break;
                case 'U': fileExt = ".USR"; break;
                case 'R': fileExt = ".REL"; break;
                default:
                    throw Basic::Error(Basic::ErrorId::SYNTAX);
                }

                filepath += fileExt;
                break;
            }
            case 0: // MODE R/W/A/M
                if (strpara[0] == 'r' || strpara[0] == 'R') {
                    iomode = "r";
                } else if (strpara[0] == 'w' || strpara[0] == 'W') {
                    iomode = "w";
                } else {
                    basic->memory[krnl.STATUS] = Basic::FS_DEVICE_ERROR;
                    throw Basic::Error(Basic::ErrorId::INTERNAL);
                }
                break;
            }
        }

        if (iomode[0] == 'r' || iomode[0] == 'a') {
            filepath = basic->os->findFirstFileNameWildcard(filepath);
        }

        basic->fileHandles[ifile].open(filepath, iomode);
        break;
    } // default
    case 15:
        throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
        break;
    } // switch


    if (!basic->fileHandles[ifile]) {
        basic->memory[krnl.STATUS] = Basic::FS_DEVICE_ERROR;
        throw Basic::Error(Basic::ErrorId::FILE_NOT_OPEN);
    }
    basic->memory[krnl.STATUS] = Basic::FS_OK;
}

void cmdCLOSE(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    int64_t ifile = basic->valueToInt(values[0]);
    if (ifile < 1 || size_t(ifile) >= basic->fileHandles.size()) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    if (!basic->fileHandles[ifile]) {
        basic->memory[krnl.STATUS] = Basic::FS_DEVICE_ERROR;
        throw Basic::Error(Basic::ErrorId::FILE_NOT_OPEN);
    }

    basic->memory[krnl.STATUS] = Basic::FS_OK;

    if (!basic->fileHandles[ifile].close()) {
        basic->memory[krnl.STATUS] = Basic::FS_DEVICE_ERROR;
        basic->printUtf8String(basic->fileHandles[ifile].status());
        throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
    }

    // TODO currentFileNo should not be a member variable at all
    if (basic->currentFileNo == ifile) {
        basic->currentFileNo = 0;
    }
}

void cmdRENUMBER(Basic* basic, const std::vector<Basic::Value>& values) {
    // RENUMBER new_start, step, start_from_old

    int newstart      = 10;
    int step          = 10;
    int startFromLine = 0;
    int milestone     = 0;
    std::map<int, int> renum;
    if (values.size() > 0) {
        newstart = int(basic->valueToInt(values[0]));
    }
    if (values.size() > 2) {
        step = int(basic->valueToInt(values[2]));
    }
    if (values.size() > 4) {
        startFromLine = int(basic->valueToInt(values[4]));
    }
    if (values.size() > 6) {
        milestone = int(basic->valueToInt(values[6]));
    }

    std::map<int, Basic::ProgramLine> newListing;
    std::map<int, int> lineMapping;
    int newLineNumber = newstart;

    int lastLineNumber = -1;

    auto& listing = basic->currentModule().listing;
    // Create mapping of old line numbers to new ones
    for (auto itlst = listing.begin(); itlst != listing.end(); ++itlst) {
        auto oldLine = itlst->first;
        auto& code   = itlst->second;
        if (oldLine >= 0 && oldLine >= startFromLine) {
            if (lastLineNumber > newLineNumber) {
                basic->os->screen.cleanCurrentLine();
                basic->printUtf8String("LINE NUMBERS WOULD OVERLAP! \n");
                throw Basic::Error(Basic::ErrorId::UNDEFD_STATEMENT);
            }

            if (oldLine > newLineNumber && milestone != 0 && (oldLine % milestone) == 0) {
                newLineNumber = oldLine;
            }

            lineMapping[oldLine] = newLineNumber;
            newLineNumber += step;
        } else {
            lineMapping[oldLine] = oldLine;
            lastLineNumber       = oldLine;
        }
    }

    // Update the lines and renumber references in GOTO, GOSUB, THEN
    std::regex lineRefRegex("\\b(GOTO|GOSUB|THEN)\\s+(\\d+)");
    for (auto itlst = listing.begin(); itlst != listing.end(); ++itlst) {
        auto oldLine  = itlst->first;
        auto& prgline = itlst->second;
        std::string updatedCode;
        std::sregex_iterator it(prgline.code.begin(), prgline.code.end(), lineRefRegex);
        std::sregex_iterator end;
        size_t lastPos = 0;

        for (; it != end; ++it) {
            updatedCode += prgline.code.substr(lastPos, it->position() - lastPos);
            updatedCode += it->str(1) + " ";

            std::string str   = it->str(2);
            const char* pnum  = str.c_str();
            int64_t oldTarget = 0;
            if (basic->parseInt(pnum, &oldTarget)) {
                updatedCode += std::to_string(lineMapping[int(oldTarget)]);
                lastPos = it->position() + it->length();
            }
        }
        updatedCode += prgline.code.substr(lastPos);

        newListing[lineMapping[oldLine]].code = updatedCode;
    }
    listing = std::move(newListing);

    basic->currentModule().setProgramCounterToEnd();
    basic->currentModule().forceTokenizing();
}

void cmdREMODEL(Basic* basic, const std::vector<Basic::Value>& values) {
    auto& opt = basic->options;
    if (values.empty()) {
        // sort alphabetically, here
        basic->printUtf8String("REMODEL \"COLORLIST\", " + basic->valueToString(opt.colorzizeListing) + "\n");
        basic->printUtf8String("REMODEL \"SPACING\", " + basic->valueToString(opt.spacingRequired) + "\n");
        basic->printUtf8String("REMODEL \"UPPERCASE\", " + basic->valueToString(opt.uppercaseInput) + "\n");
        basic->printUtf8String("REMODEL \"ZERODOT\", " + basic->valueToString(opt.dotAsZero) + "\n");
        // basic->printUtf8String("REMODEL \"\", " + basic->valueToString(0));
        return;
    }

    if (values.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    std::string key = basic->valueToString(values[0]);
    int64_t val     = basic->valueToInt(values[2]);
    if (key == "SPACING") {
        opt.spacingRequired = (val != 0);
    } else if (key == "ZERODOT") {
        opt.dotAsZero = (val != 0);
    } else if (key == "UPPERCASE") {
        opt.uppercaseInput = (val != 0);
    } else if (key == "COLORLIST") {
        opt.colorzizeListing = (val != 0);
    } else {
        throw Basic::Error(Basic::ErrorId::UNDEFD_STATEMENT);
    }
}

void cmdPLAY(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    if (!basic->valueIsString(values[0])) {
        throw Basic::Error(Basic::ErrorId::TYPE_MISMATCH);
    }

    std::string cmd = basic->valueToString(values[0]);

    basic->os->soundSystem().PLAY(cmd);
}

void cmdPOKE(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t address = basic->valueToInt(values[0]);
    int64_t value   = basic->valueToInt(values[2]);
    if (address < 0 || address >= int64_t(basic->memory.size())) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }

    MEMCELL v = MEMCELL(uint32_t(value) & 0xffffffff);
    if (address == krnl.COLOR) { // BACKGROUND_COLOR_ALSO_SEE_HERE
        v |= (basic->memory[krnl.VIC_BKGND] << 4);
    }

    basic->memory[address]      = v;
    basic->os->screen.dirtyFlag = true;
}


void cmdSOUND(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    if (!basic->valueIsString(values[2])) {
        throw Basic::Error(Basic::ErrorId::TYPE_MISMATCH);
    }

    int voice       = int(basic->valueToInt(values[0]));
    std::string cmd = basic->valueToString(values[2]);

    basic->os->soundSystem().SOUND(voice, cmd);
}

void runAssemblerCode(Basic* basic) {
    int counter       = 0;
    basic->cpu.cpuJam = false;
    uint16_t raster   = 0;
    bool stoppedByEsc = false;

#if _DEBUG
    printf("---Run Machine Code---\n");
#endif

    auto& cpu = basic->cpu;
    for (;;) {
        auto PC = cpu.PC;

        if (cpu.breakPointHit) {
            basic->monitor();
        }

        switch (PC) {
        case 0xE5C6:
            // BASIF has decreased the keyboard buffer count
            // clear kb buffer, because BASIC might have changed the NDX value.
            // we would duplicate the input character otherwise.
            while (basic->os->keyboardBufferHasData()) {
                basic->os->getFromKeyboardBuffer();
            }
            break;

        case 0XF142: // CHIN - get character from keyboard
            basic->os->updateEvents(); // pokes to keyboard buffer
            //            if (basic->os->peekKeyboardBuffer().code != 0) {
            //                cpu.A = uint8_t(basic->os->getFromKeyboardBuffer().code);
            //            }
            //            cpu.rts();
            //
            break;
        case 0xFFD5: // LOAD (vector)
        case 0xF49E: // LOAD implementation
        {
            // LOAD RAM FUNCTION
            // LOADS FROM CASSETTE 1 OR 2, OR
            // SERIAL BUS DEVICES >= 4 TO 31
            // AS DETERMINED BY CONTENTS OF
            // VARIABLE FA.VERIFY FLAG IN.A
            // ALT LOAD IF SA = 0, NORMAL SA = 1
            // .X, .Y LOAD ADDRESS IF SA     = 0
            // .A = 0 PERFORMS LOAD, <> IS VERIFY
            // HIGH LOAD RETURN IN X, Y.

            uint8_t fnlen = cpu.memory[0xB7];
            uint8_t dev1  = cpu.memory[0xBA]; // primary device number   ,!8!, 1
            uint8_t dev2  = cpu.memory[0xB9]; // secondary device number , 8 ,!1!

            // see set filename: FDF9
            uint16_t fnaddr = (cpu.memory[0xBB] & 0xff) | ((cpu.memory[0xBC] & 0xff) << 8);
            if (fnaddr + fnlen >= 0x10000) {
                throw Basic::ErrorId::OUT_OF_MEMORY;
            }

            std::string fname;
            for (size_t i = 0; i < fnlen; ++i) {
                auto c = cpu.memory[fnaddr + i];
                if (c == 0) {
                    break;
                }
                fname += c;
            }

            fname = basic->os->findFirstFileNameWildcard(fname);
            if (!basic->os->doesFileExist(fname)) {
                cpu.memory[krnl.STATUS] = Basic::FS_ERROR_READ;
                cpu.rts();
                break;
            }

            if (cpu.A == 0) {
                FilePtr pf(basic->os);
                pf.open(fname, "rb");
                auto bytes = pf.readAll();
                pf.close();

                //
                uint16_t addr = (cpu.X & 0xff) | ((cpu.Y & 0xff) << 8);
                if (dev2 == 0) {
                    // use 2 byte header of PRG file
                    if (bytes.size() >= 2) {
                        addr = (bytes[1] << 8) | bytes[0];
                        bytes.erase(bytes.begin(), bytes.begin() + 2);
                    }
                }

#if _DEBUG
                basic->printUtf8String("LOADING $" + StringHelper::int2hex(addr, true, 2) + "-$" + StringHelper::int2hex(addr + bytes.size(), true, 2) + "\n");
#endif

                for (size_t i = 0; i < bytes.size(); ++i) {
                    cpu.memory[addr] = bytes[i];
                    ++addr;
                }
                cpu.X            = addr | 0xff;
                cpu.Y            = addr >> 8;
                cpu.memory[0xAE] = cpu.X;
                cpu.memory[0xAF] = cpu.Y;
                cpu.clearFlag(CPU6502::PF_CARRY);
            }
            cpu.rts();
            break;
        }


            // case 0xFFD8: // SAVE implementation
            //     cpu.rts();
            //     break;
        }

        if (!cpu.executeNext()) {
            break;
        }

        if (++counter == 0xff) {
            counter = 0;
            basic->os->updateEvents();
            basic->os->presentScreen();


            // basic->handleEscapeKey();
            //
            // break with escape
            if (basic->os->isKeyPressed(Os::KeyConstant::ESCAPE)) {
                stoppedByEsc = true;
                break;
            }
            if (basic->os->isKeyPressed(Os::KeyConstant::PAUSE)) {
                basic->monitor();
            }

            // overwrite the jiffy clock
            int64_t TI                   = (basic->os->tick() * 60LL) / 1000LL;
            basic->memory[krnl.TIME + 2] = TI & 0xff;
            basic->memory[krnl.TIME + 1] = (TI << 8) & 0xff;
            basic->memory[krnl.TIME + 0] = (TI << 16) & 0xff;

            ++raster;
            if (raster > 312) {
                raster = 0;
            }
            basic->memory[krnl.VIC_RASTER] = raster & 0xff;

            // high bit in CTRL1 is used for lines 256..312
            if (raster > 255) {
                basic->memory[krnl.VIC_CTRL1] |= 0x80;
            } else {
                basic->memory[krnl.VIC_CTRL1] &= 0x7f;
            }
        }
    }
    if (cpu.cpuJam || stoppedByEsc) {
        std::string err = (cpu.cpuJam ? ("CPU JAM\n" + cpu.registers()) : "EXIT TO BA67")
                        + "\n";
        basic->restoreColorsAndCursor(false);
        basic->printUtf8String(err);
        throw Basic::Error(Basic::ErrorId::INTERNAL);
    }
}

void cmdSYS(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() == 0) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }



    // address specified
    if (!basic->valueIsString(values[0])) {
        int64_t address = basic->valueToInt(values[0]);

        if (values.size() > 1) {
            // you can use BASIC arguments after SYS in your machine code.
            // TXTPTR    $7A/$7B    Pointer to current position in BASIC program text
            // GETARG  (JSR $B79B)  uses $7A/$7B to know where it currently is in the BASIC text.
            // 10 PRINT PEEK(122),PEEK(123) prints 13,8 which is $080d - printed, seems correct!
            // BUF       $0200      89 bytes is the text input buffer.
            //                      Robin/8-bit-show-and-tell puts code at $0202.

            MEMCELL* ptr = &basic->memory[krnl.BUF];
            *ptr++       = U'\0';
            *ptr++       = U'\0';
            // *ptr++       = U'S';
            // *ptr++       = U'Y';
            // *ptr++       = U'S';
            // *ptr++       = U' ';
            for (size_t i = 1 /*skip address*/; i < values.size(); ++i) {
                if (basic->valueIsOperator(values[i])) {
                    *ptr++ = U',';
                    continue;
                }
                if (basic->valueIsString(values[i])) {
                    *ptr++ = U'\"';
                }
                std::string str = basic->valueToString(values[i]);
                for (char c : str) {
                    *ptr++ = c;
                }
                if (basic->valueIsString(values[i])) {
                    *ptr++ = U'\"';
                }
            }
            *ptr++ = U'\r';
            *ptr   = U'\0';

            basic->memory[krnl.TXTPTR + 0] = (2 + krnl.BUF) & 0xff;
            basic->memory[krnl.TXTPTR + 1] = ((2 + krnl.BUF) >> 8) & 0xff;


            // disable testing STOP key
            // basic->memory[0x0328] = 0xed;
            // basic->memory[0x0329] = 0xf6;
            basic->cpu.A = basic->memory[0x030C + 0]; // #780
            basic->cpu.X = basic->memory[0x030C + 1];
            basic->cpu.Y = basic->memory[0x030C + 2];
            basic->cpu.P = basic->memory[0x030C + 3];

            // address = krnl.NEWSTT;
        }



        basic->cpu.memory = &basic->memory[0];
        if (address < 0 || address >= int64_t(basic->memory.size())) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }
        if (!basic->cpu.sys(uint16_t(address & 0xffff))) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }

        runAssemblerCode(basic);

        // read registers back into memory
        basic->memory[0x030C + 0] = basic->cpu.A;
        basic->memory[0x030C + 1] = basic->cpu.X;
        basic->memory[0x030C + 2] = basic->cpu.Y;
        basic->memory[0x030C + 3] = basic->cpu.P;

        return;
    }

    // command specified
    if (!basic->valueIsString(values[0])) {
        throw Basic::Error(Basic::ErrorId::TYPE_MISMATCH);
    }
    std::string cmd = basic->valueToString(values[0]);

    basic->os->systemCall(cmd);
    // system(cmd.c_str());
}

void cmdCATALOG(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() > 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    std::string filter = "";
    if (values.size() > 0) {
        filter = basic->valueToString(values[0]);
    }

    // file size, 4 characters wide
    auto niceSize = [&basic](uint64_t s) -> std::string {
        static const char* pfx = "BKMGTPEZ????";
        const char* p          = pfx;
        while (s > 1024) {
            s /= 1024;
            ++p;
        }
        std::string str(basic->valueToString(int64_t(s)));
        str += *p;
        while (str.length() < 4) {
            str.insert(str.begin(), ' ');
        }
        return str;
    };

    uint64_t totalBytes = 0;
    auto files          = basic->os->listCurrentDirectory();
    std::vector<std::string> sizestrings(files.size());
    for (size_t i = 0; i < files.size(); ++i) {
        if (files[i].isDirectory) {
            sizestrings[i] = std::string("DIR ");
        } else {
            sizestrings[i] = niceSize(files[i].filesize);
            totalBytes += files[i].filesize;
        }
    }

    std::string dirname = basic->os->getCurrentDirectory();
    if (dirname.length() + 7 > basic->os->screen.width) {
        size_t len = basic->os->screen.width - 7;
        dirname    = dirname.substr(dirname.length() - len);
    }
    if (basic->os->dirIsInCloud()) {
        dirname = std::string((const char*)(u8"\u2601 "));
        dirname += basic->os->cloudUrl;
    }
    basic->os->screen.cleanCurrentLine();
    basic->printUtf8String((const char*)(u8"╔══"));
    basic->printUtf8String(dirname);
    basic->printUtf8String((const char*)(u8"═══\n"));
    std::string str;

    std::u32string filter32;
    Unicode::toU32String(filter.c_str(), filter32);

    std::string lockSymbol = basic->os->lockSymbol();
    for (size_t i = 0; i < files.size(); ++i) {
        auto f = files[i].name;

        if (!filter.empty()) {
            std::u32string f32;
            Unicode::toU32String(f.c_str(), f32);
            if (!Unicode::wildcardMatchNoCase(f32.c_str(), filter32.c_str())) {
                continue;
            }
        }

        if (files[i].isLocked) {
            f += lockSymbol;
        }

        // 5 chars+space for file size - space for CHDIR command
        str = (const char*)(u8"║") + sizestrings[i] + " \"" + f + "\"\n";
        basic->os->screen.cleanCurrentLine();
        basic->printUtf8String(str);
        basic->handleEscapeKey(true);
        basic->os->delay(50);
    }
    basic->os->screen.cleanCurrentLine();
    basic->printUtf8String(std::string((const char*)(u8"╚══Σ")) + niceSize(totalBytes) + (const char*)(u8"═══"));
}

void cmdCLOUD(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() == 0) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    basic->os->cloudUser = basic->valueToString(values[0]);
    if (values.size() > 2) {
        basic->os->cloudUrl = basic->valueToString(values[2]);
    }
}


void cmdCONT(Basic* basic, const std::vector<Basic::Value>& values) {
    auto& modl = basic->currentModule();
    auto ln    = modl.listing.find(modl.lineNumberForCONT);
    if (ln != modl.listing.end()) {
        modl.programCounter.line   = ln;
        modl.programCounter.cmdpos = modl.tokenIndexForCONT;
    }
}

Basic::Value fktCHR$(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    char32_t i = char32_t(basic->valueToInt(args[0]));
    std::string s;
    if (i == 0) {
        s.push_back(0);
    } else {
        Unicode::appendAsUtf8(s, i);
    }
    return s;
}

Basic::Value fktDEC(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    return basic->strToInt("$" + basic->valueToString(args[0]));
}

Basic::Value fktHEX$(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t n = basic->valueToInt(args[0]);

    return StringHelper::int2hex(n);
}

Basic::Value fktINSTR(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() < 3 || args.size() > 5) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    size_t startOffset = 1;
    if (args.size() == 5) {
        startOffset = basic->valueToInt(args[6]);
    }

    if (startOffset < 1) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }

    auto pos = Unicode::strstr(basic->valueToString(args[0]), basic->valueToString(args[2]), startOffset - 1);
    // INSTR is on index, base 1.
    if (pos == std::string::npos) {
        return 0;
    }
    return int64_t(pos + 1);
}

Basic::Value fktJOY(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t port      = basic->valueToInt(args[0]);
    const auto* state = &basic->os->getGamepadState(int(port - 1));

#if defined(_WIN32)
    // On Windows we have 4 XInput controllers and a bunch of DirectInput controllers
    if (!state->connected && port < 4) {
        state = &basic->os->getGamepadState(int(port - 1 + 4));
    }
#endif

    int64_t joy = 0;

    auto setJoy = [&joy, &state](int8_t xIs, int8_t yIs, int64_t ijoy) {
        if (state->dpad.x == xIs && state->dpad.y == yIs) {
            joy = ijoy;
        } else {
            const double deadzone = 0.7;
            auto toDigital        = [](float f) -> int8_t { return std::abs(f) > 0.7f ? (f < 0.0f ? -1 : 1) : 0; };
            if (toDigital(state->analogLeft.x) == xIs && toDigital(state->analogLeft.y) == yIs) {
                joy = ijoy;
            }
        }
    };
    setJoy(0, -1, 1);
    setJoy(1, -1, 2);
    setJoy(1, 0, 3);
    setJoy(1, 1, 4);
    setJoy(0, 1, 5);
    setJoy(-1, 1, 6);
    setJoy(-1, 0, 7);
    setJoy(-1, -1, 8);
    if (state->buttons[0] || state->buttons[2] || state->buttons[4]) {
        joy += 128;
    }
    if (state->buttons[1] || state->buttons[3] || state->buttons[5]) {
        joy += 256;
    }
    return joy;
}

Basic::Value fktMAX(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() == 0) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    bool gotOne = false;
    Basic::Value ret;
    for (auto& a : args) {
        if (basic->valueIsOperator(a)) {
            continue;
        }
        if (!gotOne) {
            gotOne = true;
            ret    = a;
        }

        if (basic->valueIsInt(a) && basic->valueIsInt(ret)) {
            if (basic->valueToInt(a) > basic->valueToInt(ret)) {
                ret = a;
            }
        } else {
            if (basic->valueToDouble(a) > basic->valueToDouble(ret)) {
                ret = a;
            }
        }
    }

    if (!gotOne) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    return ret;
}

Basic::Value fktMIN(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() == 0) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    bool gotOne = false;
    Basic::Value ret;
    for (auto& a : args) {
        if (basic->valueIsOperator(a)) {
            continue;
        }
        if (!gotOne) {
            gotOne = true;
            ret    = a;
        }

        if (basic->valueIsInt(a) && basic->valueIsInt(ret)) {
            if (basic->valueToInt(a) < basic->valueToInt(ret)) {
                ret = a;
            }
        } else {
            if (basic->valueToDouble(a) < basic->valueToDouble(ret)) {
                ret = a;
            }
        }
    }

    if (!gotOne) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    return ret;
}


Basic::Value fktLEFT$(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    size_t length = basic->valueToInt(args[2]);
    return Unicode::substr(basic->valueToString(args[0]), 0, length);
}

Basic::Value fktMID$(Basic* basic, const std::vector<Basic::Value>& args) {
    // str$, start, [length]
    if (args.size() < 3 || args.size() > 5) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t start = basic->valueToInt(args[2]); // base 1
    if (start < 1) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    --start; // base 0

    size_t length = ~size_t(0);
    if (args.size() == 5) {
        length = basic->valueToInt(args[4]);
    }
    return Unicode::substr(basic->valueToString(args[0]), start, length);
}

Basic::Value fktRIGHT$(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    size_t right    = basic->valueToInt(args[2]);
    std::string str = basic->valueToString(args[0]);
    size_t length   = Unicode::utf8StrLen(str);
    if (right >= length) {
        return str;
    }
    return Unicode::substr(str, length - right);
}

Basic::Value fktRND(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t n = basic->valueToInt(args[0]);
    if (n < 0) {
        srand(int(-n));
    } else if (n == 0) {
        srand(int(basic->os->tick()) + rand());
    }
    return double(rand()) / double(RAND_MAX);
}


Basic::Value fktLCASE$(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    return Unicode::toLower(basic->valueToString(args[0]).c_str());
}
Basic::Value fktUCASE$(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    return Unicode::toUpper(basic->valueToString(args[0]).c_str());
}

Basic::Value fktTAB(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t tab = basic->valueToInt(args[0]);
    auto crsr   = basic->os->screen.getCursorPos();
    if (int64_t(crsr.x) < tab) {
        basic->os->screen.setCursorPos({ size_t(tab), crsr.y });
    }
    return std::string();
}
Basic::Value fktSPC(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t tab = basic->valueToInt(args[0]);
    auto crsr   = basic->os->screen.getCursorPos();
    if (int64_t(crsr.x) < tab) {
        basic->os->screen.setCursorPos({ size_t(tab), crsr.y });
    }
    return std::string();
}


Basic::Value fktPEN(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    int64_t index = basic->valueToInt(args[0]);
    auto mouse    = basic->os->getMouseStatus();
    switch (index) {
    case 0:
    case 2: return { int64_t(mouse.x) };
    case 1:
    case 3: return { int64_t(mouse.y) };
    case 4: return { int64_t(mouse.buttonBits) };
    }
    throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
}

Basic::Value fktPETSCII$(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string str;
    int64_t index = basic->valueToInt(args[0]);
    if (index >= 0 && index <= 0xff) {
        Unicode::appendAsUtf8(str, PETSCII::realPETSCIItoUnicode(index & 0xff, false));
    } else if (index < 0 && index >= -0xff) {
        Unicode::appendAsUtf8(str, PETSCII::realPETSCIItoUnicode(-index & 0xff, false));
    } else {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    return Basic::Value(str);
}



void Basic::Array::dim(Value init, size_t i0, size_t i1, size_t i2, size_t i3) {
    dim(init, { i0, i1, i2, i3 });
}

void Basic::Array::dim(Value init, const ArrayIndex& ai) {
    setIsDictionary(false);
    this->bounds.index = ai.index;
    size_t elems       = 1 + ai.index[0]; // add the end element, too. dim a(5) = 0..5
    size_t old         = elems;
    try {
        for (size_t n = 1; n < ai.index.size(); ++n) {
            if (ai.index[n] != 0) {
                elems *= (1 + ai.index[n]);
            }
            if (elems < old) {
                throw Error(ErrorId::ILLEGAL_QUANTITY);
            }
            old = elems;
        }

        data.clear();
        data.resize(elems);
        for (auto& v : data) {
            v = init;
        }
    } catch (...) {
        throw Error(ErrorId::ILLEGAL_QUANTITY);
    }
}

Basic::Value& Basic::Array::at(const Basic::ArrayIndex& ix) {
    if (isDictionary) {
        throw Error(ErrorId::TYPE_MISMATCH);
    }
    size_t i   = 0;
    size_t blk = 1;

    for (size_t n = 0; n < bounds.index.size(); ++n) {
        size_t ixn = ix.index[n];
        if (blk == 0 && ixn > 0) {
            throw Error(ErrorId::BAD_SUBSCRIPT);
        } // DIM a(5): a(1,1) = 1
        i += ixn * blk;
        blk *= bounds.index[n];
    }
    if (i < data.size()) {
        return data[i];
    }
    throw Error(ErrorId::BAD_SUBSCRIPT); // DIM a(5): a(6) = 1: REM a is defined (0..5)
}


Basic::Options Basic::options; // static instance

inline const std::string Basic::buildVersion() { return __DATE__; }

Basic::Basic(Os* os, SoundSystem* ss) {
    memory.resize(0x20000);
    cpu.memory = &memory[0];

    static auto memcellcpy8 = [](MEMCELL* dst, const uint8_t* src, size_t count) {
        while (count != 0) {
            --count;
            *dst++ = *src++;
        }
    };
    auto memcellcpycell = [this](size_t dst, size_t src, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            memory[dst + i] = memory[src + i];
        }
    };

    memcellcpy8(&memory[0xA000], RomImage::BASIC_V2(), 0x2000);
    memcellcpy8(&memory[0xE000], RomImage::KERNAL_C64(), 0x2000);
    memcellcpy8(&memory[0x0000], RomImage::LOW_RAM(), 0x1000);

    // copy the BA67 font to the CHARROM memory.
    // --this is never used. BA67 takes the PRINT-ing role--
    for (size_t i = 0; i < 0x100; ++i) {
        // char32_t unicode = PETSCII::toUnicode(uint8_t(i));

        auto& bmp = os->screen.getCharDefinition(char32_t(i));
        for (size_t y = 0; y < 8; ++y) {
            // uppercase char set
            // memory[0xD000 + i * 8 + y] = bmp.bits[y];

            // lowercase char set (same, but reversed)
            memory[0xD800 + i * 8 + y] = ~bmp.bits[y];
        }
    }

    // start C64 BASIC with SYS $FCE2 or GO 64
    memory[0xE739 + 1] = 0xff; // don't convert PETSCII to screen code
    memory[0xE73D + 1] = 0xff; // don't convert PETSCII to screen code

    memory[0xE640 + 0] = 0x4c; // JMP $E654 skips conversion of screen code to PETSCII
    memory[0xE640 + 1] = 0x54;
    memory[0xE640 + 2] = 0xe6;

    // return from a BASIC error back to BA67.
    // memory[0xA46C] = 0x00; // brk instead of going to C64 editor


    // COMMODORE BASIC V2 -> COMMODORE+BA67  V2
    memory[0xE48A] = '+';
    // BA..
    memory[0xE48D] = '6'; // make sure $E48D is '6' as stated in the readme.md
    memory[0xE48E] = '7';
    memory[0xE48F] = ' ';

    // set screen page to $0400
    memory[krnl.HIBASE] = 0x04;

#if 0
    // CHRGET: copy BASIC's MOVCHG to zero page
    memcellcpycell(0x0073, 0xE3A2, 24);

    // prepare BASIC Indirect Vectors
    uint8_t basic_ptr[] = { 0x8b, 0xe3, 0x83, 0xa4, 0x7c, 0xa5, 0x1a, 0xa7, 0xe4, 0xa7, 0x86, 0xae };
    memcellcpy8(&memory[0x0300], &basic_ptr[0], 12);


    // copy and prepare Kernal Indirect Vectors
    memcellcpycell(0x0314, 0xFD30, 34);
#endif


    memory[krnl.DFLTI] = 0; // input = keyboard
    memory[krnl.DFLTO] = 3; // ouput = screen



    // ST
    memory[krnl.STATUS] = Basic::FS_OK;


#if 0 // defined(_DEBUG)
    auto savemem = [&](size_t from, size_t len, std::string path) {
        std::vector<uint8_t> by(len);
        for (size_t n = 0; n < len; ++n) {
            by[n] = uint8_t(memory[from + n]);
        }
        FilePtr p(os);
        p.open(path, "wb");
        p.write(&by[0], len);
        p.close();
    };
    savemem(0xA000, 0x2000, "C:\\Temp\\basic");
    savemem(0xE000, 0x2000, "C:\\Temp\\kernal");
    savemem(0xD000, 0x1000, "C:\\Temp\\chargen");
#endif


    os->screen.initMemory(&memory[0]);

    for (int i = 0; i < 256; ++i) {
        fileHandles.emplace_back(FilePtr(os));
    }

    Module mainmodule;
    modules[""] = mainmodule;
    moduleVariableStack.push_back(modules.begin()); // create a module with no name and push it on the stack
    moduleListingStack.push_back(modules.begin());

    this->os             = os;
    time0                = os->tick();
    std::string charLogo = Unicode::toUtf8String(U"🌈"); // rainbow - 1F308
    cmdCHARDEF(this, {
                         Value(charLogo),
                         Value(int64_t(0xbbbbb222LL)), // red
                         Value(int64_t(0xbbb22888LL)), // orange
                         Value(int64_t(0xbb288888LL)), // orange
                         Value(int64_t(0xb2888777LL)), // yellow
                         Value(int64_t(0xb2887777LL)), // yellow
                         Value(int64_t(0x22877dddLL)), // green
                         Value(int64_t(0x287dddeeLL)), // green
                         Value(int64_t(0x287ddeeeLL)), // blue
                     });

    os->init(this, ss);
    os->screen.setColors(13, 11);
    os->screen.setBorderColor(13);
    os->screen.clear();

    keyShortcuts[1 - 1] = "\"CHDIR\"";
    keyShortcuts[2 - 1] = "\"LOAD \"";
    keyShortcuts[3 - 1] = "\"CATALOG \""; // +CHR$(13)
    keyShortcuts[4 - 1] = "\"SCNCLR \"+CHR$(13)";
    keyShortcuts[5 - 1] = "\"SAVE \"";
    keyShortcuts[6 - 1] = "\"RUN \"+CHR$(13)";
    keyShortcuts[7 - 1] = "\"LIST \"+CHR$(13)";
    keyShortcuts[9 - 1] = "\"CHDIR \"+CHR$(34)+\"CLOUD\"+CHR$(34)+CHR$(13)+\"CATALOG\"+CHR$(13)";

    // hard coded keywords
    // common words first, slow keywords to the back
    keywords = { "GOTO", "GOSUB", "RETURN", "IF", "THEN", "FOR", "TO", "NEXT", "STEP", "LET",
                 "PRINT", "?", "GET", "FN", "FNEND",
                 "REM", "RCHARDEF", "READ", "DATA", "RESTORE",
                 "END", "RUN", "DIM", "NETGET", "HELP", "INPUT", "CLR", "ON", "SCNCLR",
                 "NEW", "LIST", "MODULE", "KEY", "GETKEY", "DEF", "DELETE", "USING",
                 "DUMP" };

    // commands
    commands.insert({
        { "ABOUT", cmdABOUT },
        { "AUTO", cmdAUTO },
        { "BAKE", cmdBAKE },
        { "CHAR", cmdCHAR },
        { "CHDIR", cmdCHDIR },
        { "COLOR", cmdCOLOR },
        { "CATALOG", cmdCATALOG },
        { "CONT", cmdCONT },
        { "CLOUD", cmdCLOUD },
        { "CHARDEF", cmdCHARDEF },
        { "SPRDEF", cmdSPRDEF },
        { "SPRITE", cmdSPRITE },
        { "MOVSPR", cmdMOVSPR },
        { "MONITOR", cmdMONITOR },
        { "QUIT", cmdQUIT },
        { "FIND", cmdFIND },
        { "GO", cmdGO },
        { "GRAPHIC", cmdGRAPHIC },
        { "LOAD", cmdLOAD },
        { "OPEN", cmdOPEN },
        { "CLOSE", cmdCLOSE },
        { "SAVE", cmdSAVE },
        { "QSAVE", cmdQSAVE },
        { "BLOAD", cmdBLOAD },
        { "BSAVE", cmdBSAVE },
        { "RENUMBER", cmdRENUMBER },
        { "SCRATCH", cmdSCRATCH },
        { "SYS", cmdSYS },
        { "PLAY", cmdPLAY },
        { "POKE", cmdPOKE },
        { "REMODEL", cmdREMODEL },
        { "SOUND", cmdSOUND },
        { "STOP", [&](Basic* basic, const std::vector<Basic::Value>&) { throw Error(ErrorId::BREAK); } },
        { "SLOW", [&](Basic* basic, const std::vector<Basic::Value>&) { basic->moduleVariableStack.back()->second.fastMode = false; } },
        { "FAST", [&](Basic* basic, const std::vector<Basic::Value>&) { basic->moduleVariableStack.back()->second.fastMode = true; } },
        { "TRON", [&](Basic* basic, const std::vector<Basic::Value>&) { basic->moduleVariableStack.back()->second.traceOn = true; } },
        { "TROFF", [&](Basic* basic, const std::vector<Basic::Value>&) { basic->moduleVariableStack.back()->second.traceOn = false; } },
    });
    // commands["PRINT"] = cmdPRINT;
    // // commands["INPUT"] = cmdINPUT;
    // commands["EXIT"] = cmdEXIT;
    // commands["LIST"] = cmdLIST;

    // throw if the argument count does not match
    auto nargs = [](const std::vector<Basic::Value>& a, size_t n) {if (a.size() != n) { throw Error(ErrorId::ARGUMENT_COUNT); } };

    // functions
    // functions["SIN"] = fktSIN;
    // args contain comma operators!
    functions.insert({
        { "ABS", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return fabs(basic->valueToDouble(args[0])); } },
        { "ASC", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); std::string s = Basic::valueToString(args[0]); const char* p = s.c_str(); return (int64_t)Unicode::parseNextUtf8(p); } },
        { "ATN", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return atan(basic->valueToDouble(args[0])); } },
        { "CHR$", fktCHR$ },
        { "COS", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return cos(basic->valueToDouble(args[0])); } },
        { "DEC", fktDEC },
        { "EXP", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return exp(basic->valueToDouble(args[0])); } },
        { "FRE", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return (int64_t)basic->os->getFreeMemoryInBytes(); } },
        { "HEX$", fktHEX$ },
        { "INT", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return basic->valueToInt(args[0]); } },
        { "INSTR", fktINSTR },
        { "JOY", fktJOY },
        { "LEFT$", fktLEFT$ },
        { "LCASE$", fktLCASE$ },
        { "UCASE$", fktUCASE$ },
        { "LEN", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return (int64_t)Unicode::utf8StrLen(basic->valueToString(args[0])); } },
        { "LOG", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return log(basic->valueToDouble(args[0])); } },
        { "MOD", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 3); auto div = basic->valueToInt(args[2]); if (div == 0) { throw Error(ErrorId::ILLEGAL_QUANTITY); }return basic->valueToInt(args[0]) % div; } },
        { "MAX", fktMAX },
        { "MIN", fktMIN },
        { "MID$", fktMID$ },
        { "PEEK", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return int64_t(memory[basic->valueToInt(args[0])]); } },
        { "PEN", fktPEN },
        { "PETSCII$", fktPETSCII$ },
        { "POS", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return int64_t(basic->os->screen.getCursorPos().x); } },
        { "POSY", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return int64_t(basic->os->screen.getCursorPos().y); } },
        { "RIGHT$", fktRIGHT$ },
        { "RND", fktRND },
        { "SGN", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value {
             nargs(args, 1);
             double d = basic->valueToDouble(args[0]);
             return (d == 0.0) ? 0.0 : ((d < 0.0) ? -1.0 : 1.0);
         } },
        { "SIN", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return sin(basic->valueToDouble(args[0])); } },

        { "SPC", fktSPC },
        { "SQR", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return sqrt(basic->valueToDouble(args[0])); } },
        { "STR$", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return basic->valueToString(args[0]); } },
        { "TAB", fktTAB },
        { "TAN", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return tan(basic->valueToDouble(args[0])); } },
        { "VAL", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return basic->valueToDoubleOrZero(args[0]); } },
        { "XOR", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 3); return basic->valueToInt(args[0]) ^ basic->valueToInt(args[2]); } }
    });


    size_t centerx = 3;
    printUtf8String("\n");
    printUtf8String(
        std::string(centerx, ' ') + std::string(" ****    BA67 BASIC") + charLogo + " V" + version() + ("   ****\n"));

    std::string strmem = "   " + valueToString(int64_t(os->getFreeMemoryInBytes()) / (1024 * 1024)) + std::string(" BASIC MBYTES FREE");
    while (strmem.length() < 31) {
        strmem.insert(strmem.begin(), ' ');
    }
    printUtf8String(
        std::string(centerx, ' ') + strmem + "\n" + std::string(centerx, ' ') + "       (C)2025 DREAM DESIGN.\n");
}

// color index (base 0) for highlighting current module
uint8_t Basic::colorForModule(const std::string& str) const {

    // main module: light green
    if (str.empty()) {
        return 13;
    }

    // others: guess a color by the name
    uint8_t col = 0;
    for (auto c : str) {
        col += c;
        ++col;
    }

    std::vector<uint8_t> colsToUse = { /* 13,*/ 7, 14, 3, 1, 10 };
    return colsToUse[col % colsToUse.size()];
}


void Basic::storeProgramCounterForCont() {
    Basic::Module& modl    = moduleListingStack.back()->second;
    modl.tokenIndexForCONT = 0;
    modl.lineNumberForCONT = 0;
    auto& pc               = programCounter();
    if (pc.line != moduleListingStack.back()->second.listing.end()) {
        modl.tokenIndexForCONT = pc.cmdpos;
        modl.lineNumberForCONT = pc.line->first;
    }
}

// Represent value as string
inline std::string Basic::valueToString(const Value& v) {
    if (auto s = std::get_if<std::string>(&v)) {
        return *s;
    }
    if (auto i = std::get_if<int64_t>(&v)) {
        return std::to_string(*i);
    }
    if (auto d = std::get_if<double>(&v)) {
#if defined(__cplusplus) && __cplusplus > 202002L
        return return std::format("{:.9g}", *d);
#else
        std::ostringstream oss;
        oss << std::setprecision(9) << std::noshowpoint << *d;
        return oss.str();
#endif
        return std::to_string(*d);
    }
    throw Error(ErrorId::TYPE_MISMATCH);
}

inline double Basic::valueToDouble(const Value& v) {
    if (auto i = std::get_if<int64_t>(&v)) {
        return (double)(*i);
    }
    if (auto d = std::get_if<double>(&v)) {
        return (*d);
    }
    if (auto s = std::get_if<std::string>(&v)) {
        const char* str = s->c_str();
        double d        = 0;
        if (parseDouble(str, &d) && *str == '\0') {
            return d;
        }
    }
    throw Error(ErrorId::TYPE_MISMATCH);
}
inline double Basic::valueToDoubleOrZero(const Value& v) {
    if (auto i = std::get_if<int64_t>(&v)) {
        return (double)(*i);
    }
    if (auto d = std::get_if<double>(&v)) {
        return (*d);
    }
    if (auto s = std::get_if<std::string>(&v)) {
        const char* str = s->c_str();
        double d        = 0;
        if (parseDouble(str, &d) && *str == '\0') {
            return d;
        }
    }
    return 0.0; // VAL("X") instead of throw Error(ErrorId::TYPE_MISMATCH);
}

inline int64_t Basic::valueToInt(const Value& v) {
    if (auto i = std::get_if<int64_t>(&v)) {
        return (*i);
    }
    if (auto d = std::get_if<double>(&v)) {
        return (int)(*d);
    }
    if (auto s = std::get_if<std::string>(&v)) {
        const char* str = s->c_str();
        int64_t i       = 0;
        if (parseInt(str, &i) && *str == '\0') {
            return i;
        }
    }
    throw Error(ErrorId::TYPE_MISMATCH);
}

bool Basic::valueIsOperator(const Value& v) {
    if (auto i = std::get_if<Basic::Operator>(&v)) {
        return true;
    }
    return false;
}

bool Basic::valueIsString(const Value& v) {
    if (auto s = std::get_if<std::string>(&v)) {
        return true;
    }
    return false;
}
bool Basic::valueIsInt(const Value& v) {
    if (auto s = std::get_if<int64_t>(&v)) {
        return true;
    }
    return false;
}
bool Basic::valueIsDouble(const Value& v) {
    if (auto s = std::get_if<double>(&v)) {
        return true;
    }
    return false;
}

inline bool Basic::isEndOfWord(char c) {
    return (
        (c >= 0 && c < '0') // operators, space, newline, tab, quotes, braces
        || (c >= ':' && c <= '@') // colons, comparators
        || (c >= '{' && c <= 0x7f) // curly braces, or-operator
        // || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ':' || c == ';' || c == '\"' || c == '\0'
    );
}

inline const char* Basic::skipWhite(const char*& str) {
    while (isWhiteSpace(*str)) {
        ++str;
    }
    return str;
}

// positive double value
inline bool Basic::parseDouble(const char*& str, double* number) {
    skipWhite(str);

    char* end;
    double d = strtod(str, &end);
    if (number) {
        *number = d;
    }
    if (end > str) {
        str = end;
        return true;
    }

    if (options.dotAsZero && *str == '.') {
        ++str;
        if (number) {
            *number = 0.0;
        }
        return true;
    }

    return false;
}

// positive int - not a double! "1.23" returns false
inline bool Basic::parseInt(const char*& str, int64_t* number) {
    skipWhite(str);
    char* endInt;

    int base = 10;
    if (*str == '$') {
        ++str;
        base = 16;
    }

    int64_t i = strtoll(str, &endInt, base);
    if (number) {
        *number = i;
    }

    if (endInt == str) {
        return false;
    }

    // might be a double?
    if (base == 10) {
        const char* endDbl = str;
        if (!parseDouble(endDbl)) {
            throw Error(ErrorId::SYNTAX);
        }
        if (endDbl > endInt) {
            return false;
        }
    }
    str = endInt;
    return true;
}

bool Basic::parseFileHandle(const char*& str, std::string_view* number) {
    skipWhite(str);
    char* endInt;
    if (*str == '#') {
        int64_t i = strtoll(str + 1, &endInt, 10);
        if (number) {
            std::string_view s((const char*)(str) + 1, (const char*)(endInt));
            *number = s;
        }
        if (endInt == str + 1) {
            return false;
        }
        str = endInt;
        return true;
    }
    return false;
}

int64_t Basic::strToInt(const std::string& str) {
    return strToInt(std::string_view(str));
}
int64_t Basic::strToInt(const std::string_view& str) {
    if (str[0] == '$' && str[1] != '\0') { // str.starts_with("$") && str.length() > 1
        return StringHelper::strtoi64_hex(str.substr(1));
    } else if (str[0] != '\0') { // str.length() > 0
        return StringHelper::strtoi64(str);
    }
    return 0;
}

bool Basic::parseKeyword(const char*& str, std::string_view* keyword) {
    skipWhite(str);
    for (auto& k : keywords) {
        if (StringHelper::strncmp(str, k.c_str(), k.length()) == 0
            && (isEndOfWord(str[k.length()]) || !options.spacingRequired)) {
            if (keyword != nullptr) {
                *keyword = std::string_view(str, str + k.length());
            }
            str += k.length();
            return true;
        }
    }
    return false;
}

bool Basic::parseCommand(const char*& str, std::string_view* command) {
    skipWhite(str);
    for (auto& k : commands) {
        if (StringHelper::strncmp(str, k.first.c_str(), k.first.length()) == 0
            && (isEndOfWord(str[k.first.length()]) || !options.spacingRequired) /*this differs from MS BASIC*/
        ) {
            if (command != nullptr) {
                *command = std::string_view(str, str + k.first.length());
            }
            str += k.first.length();
            return true;
        }
    }
    return false;
}
bool Basic::parseFunction(const char*& str, std::string_view* function) {
    skipWhite(str);
    for (auto& k : functions) {
        if (StringHelper::strncmp(str, k.first.c_str(), k.first.length()) == 0
            && (isEndOfWord(str[k.first.length()])) /*this differs from MS BASIC*/
        ) {
            const char* fndBrace = str + k.first.length();
            skipWhite(fndBrace);
            if (*fndBrace == '(') {
                if (function != nullptr) {
                    *function = std::string_view(str, str + k.first.length());
                }
                str = fndBrace; // str += k.first.length();
            }
            return true;
        }
    }
    return false;
}

bool Basic::parseString(const char*& str, std::string_view* stringUnquoted) {
    skipWhite(str);
    if (*str == '\"' || *str == '\'') {
        // char quoteChar = *str;
        const char* end = StringHelper::strchr(str + 1, *str /* quoteChar */);
        if (end == nullptr) {
            throw Error(ErrorId::SYNTAX);
        }
        if (stringUnquoted) {
            *stringUnquoted = std::string_view(str + 1, end);
        }
        str = end + 1;
        return true;
    }
    return false;
}

bool Basic::parseOperator(const char*& str, std::string_view* op) {
    skipWhite(str);
    const char* start = str;

    if (!options.spacingRequired) {
        if (StringHelper::strncmp(str, "AND", 3) == 0) {
            *op = std::string_view(str, str + 3);
            str += 3;
            return true;
        }
        if (StringHelper::strncmp(str, "NOT", 3) == 0) {
            *op = std::string_view(str, str + 3);
            str += 3;
            return true;
        }
        if (StringHelper::strncmp(str, "OR", 2) == 0) {
            *op = std::string_view(str, str + 2);
            str += 2;
            return true;
        }
    }


    if (parseIdentifier(str, op)) {
        if (*op == "AND" || *op == "OR" || *op == "NOT") {
            return true;
        }
        str = start;
    }

    while (*str == '=' || *str == '+' || *str == '-' || *str == '*' || *str == '/' || *str == '<' || *str == '>' || *str == '^' || *str == ';') {
        if (*str == '-' && str > start) {
            break;
        } // a=-2
        ++str;
    }
    if (str > start && op) {
        *op = std::string_view(start, str);
        return true;
    }
    return false;
}

inline bool Basic::parseIdentifier(const char*& str, std::string_view* identifier) {
    skipWhite(str);
    // start with alpha or Unicode
    if ((*str >= 'A' && *str <= 'Z') || (*str >= 'a' && *str <= 'z')
        /* || *str > 192 || *str < 0 */
    ) {
        const char* pend = str;

        if (!options.spacingRequired) {
            while (!isEndOfWord(*pend)) {
                const char* test = pend;
                if (parseKeyword(test) || parseCommand(test)) {
                    break;
                }
                ++pend;
            }
        } else {
            // alpha-numeric
            // while (isalnum(*pend)) {
            while (!isEndOfWord(*pend)) {
                ++pend;
            }
        }


        // type postfix
        if (*pend == '$' || *pend == '%') {
            ++pend;
        }

        if (identifier) {
            *identifier = std::string_view(str, pend);
        }
        str = pend;
        return true;
    }
    return false;
}


// parse next command, advance the program counter
// void Basic::tokenizeNextCommand(ProgramCounter* pProgramCounter) {
//     if (pProgramCounter == nullptr) {
//         pProgramCounter = &programCounter();
//     }
//     std::string& line = pProgramCounter->line->second;
//
//     const char* pline = line.c_str();
//     const char* pc    = pline;
//
//     size_t xpos = pProgramCounter->cmdpos;
//     while (xpos != 0) {
//         if (*pc != '\0') {
//             ++pc;
//         }
//         --xpos;
//     }
//
//
//     pc = tokenizeNextCommand(pc, pProgramCounter->line->second.tokens);
//
//     // move to end of statement
//     pProgramCounter->position = pc - pline;
//
// #if 0
//     debug("TK:");
//     for (auto& t : tokens) {
//         debug(t.value.c_str()); debug(" ");
//     }
//     debug("\n");
// #endif
// }

// parse next command, advance the program code pointer
const char* Basic::tokenizeNextCommand(const char* pc, std::vector<Basic::Token>& tokens) {
    tokens.clear();

    double d;
    int64_t i;
    std::string_view str;

    bool rememberNoSpace = options.spacingRequired;

    while (*pc != '\0') {
        skipWhite(pc);
        const char* pcBeforeParse = pc;
        if (parseKeyword(pc, &str)) {
            tokens.push_back({ TokenType::KEYWORD, str });
            if (str.length() == 3 && str == "REM") { // REM is special. SYS is not! (we need variables in SYS)
                tokens.push_back({ TokenType::STRING, pc });
                while (*pc != '\0') {
                    ++pc;
                }
            }
            if (rememberNoSpace && str == "DATA") { // in DATA, don't split unquoted OTTO to OT TO
                options.spacingRequired = true;
            }
        } else if (parseCommand(pc, &str)) {
            tokens.push_back({ TokenType::COMMAND, str });
        } else if (parseString(pc, &str)) {
            tokens.push_back({ TokenType::STRING, str });
        } else if (parseOperator(pc, &str)) {
            tokens.push_back({ TokenType::OPERATOR, str });
        } else if (parseInt(pc, &i)) {
            tokens.push_back({ TokenType::INTEGER, std::string_view(pcBeforeParse, pc) });
        } else if (parseDouble(pc, &d)) {
            tokens.push_back({ TokenType::NUMBER, std::string_view(pcBeforeParse, pc) });
        } else if (*pc == '(' || *pc == ')') {
            tokens.push_back({ TokenType::PARENTHESIS, std::string_view(pc, pc + 1) });
            ++pc;
        } else if (*pc == ',') {
            tokens.push_back({ TokenType::COMMA, std::string_view(pc, pc + 1) });
            ++pc;
        } else if (parseFileHandle(pc, &str)) {
            tokens.push_back({ TokenType::FILEHANDLE, str });
        } else if (parseIdentifier(pc, &str)) { // TODO parse function before identifier
            skipWhite(pc);
            std::string_view str2;
            const char* pcdot = pc;
            if (*pcdot++ == '.' && parseIdentifier(pcdot, &str2)) {
                tokens.push_back({ TokenType::MODULE, str });
                tokens.push_back({ TokenType::IDENTIFIER, str2 });
                pc = pcdot;
            } else {
                tokens.push_back({ TokenType::IDENTIFIER, str });
            }
        } else if (*pc == ':') {
            ++pc;
            break; // end of command
        } else if (*pc != '\0') {
            options.spacingRequired = rememberNoSpace;
            throw Error(ErrorId::SYNTAX);
        }
    }
    options.spacingRequired = rememberNoSpace;

    // fix negative unary operators
    // a + -5
    for (size_t i = 1; i + 1 < tokens.size(); ++i) {
        auto& toki  = tokens[i];
        auto& tok_1 = tokens[i - 1];

        if (toki.type == TokenType::OPERATOR && toki.value == "NOT") {
            toki.type = TokenType::UNARY_OPERATOR;
        }

        if (toki.type == TokenType::OPERATOR && (tok_1.type == TokenType::OPERATOR || tok_1.type == TokenType::KEYWORD || tok_1.type == TokenType::UNARY_OPERATOR || (tok_1.type == TokenType::PARENTHESIS && tok_1.value == "("))) {
            toki.type = TokenType::UNARY_OPERATOR;
        }
    }
    return pc;
}

// tokenize the line, creating string_views to the line's code.
void Basic::tokenizeLine(ProgramLine& line) {
    line.tokens.clear();
    const char* pc = line.code.c_str();
    while (*pc != '\0') {
        line.tokens.resize(line.tokens.size() + 1);
        pc = tokenizeNextCommand(pc, line.tokens.back());
        if (line.tokens.back().empty()) {
            line.tokens.pop_back();
            // pc += strlen(pc);
            break;
        }
    }
}

// Just tokenizeNextCommand a string without harming the program
// std::vector<std::vector<Basic::Token>> Basic::tokenizeNextCommand(const std::string& code) {
//     std::vector<std::vector<Token>> output;
//     Module m;
//     m.listing[0]          = code;
//     m.programCounter.line = m.listing.begin();
//     for (;;) {
//         auto tok = tokenizeNextCommand(&m.programCounter);
//         if (tok.empty()) {
//             break;
//         }
//         output.emplace_back(tok);
//     }
//     return output;
// }

// Operator precedence
inline int Basic::precedence(const std::string_view& op) {
    const int line0 = __LINE__;
    if (op == ";") {
        return __LINE__ - line0;
    } // ; is string concatenation operator
    if (op == "AND" || op == "OR") {
        return __LINE__ - line0;
    }
    if (op == "<" || op == ">" || op == "<>" || op == "<=" || op == ">=") {
        return __LINE__ - line0;
    }
    if (op == "=") {
        return __LINE__ - line0;
    }
    if (op == "+" || op == "-") {
        return __LINE__ - line0;
    }
    if (op == "*" || op == "/") {
        return __LINE__ - line0;
    }
    if (op == "^") {
        return __LINE__ - line0;
    }
    if (op[0] == 'u' || op == "NOT") {
        return __LINE__ - line0;
    } // unary operator
    return 0;
}

Basic::Value Basic::evaluateDefFnCall(Basic::FunctionDefinition& fn, const std::vector<Basic::Value>& arguments) {
    auto& mod = currentModule();

    // arguments are comma separated
    if (1 + arguments.size() != fn.parameters.size() * 2) {
        throw Error(ErrorId::ARGUMENT_COUNT);
    }

    // multiline FN body
    if (fn.body.empty()) {
        std::vector<Value> functionCallVariableStack;

        for (size_t i = 0; i < fn.parameters.size(); ++i) {
            auto& para = fn.parameters[i];
            auto varIt = mod.findOrCreateVariable(para.value);
            functionCallVariableStack.push_back(varIt->second);

            varIt->second = arguments[i * 2 /* skip comma*/];
        }

        // GOSUB but FNEND instead of RETURN
        auto pc = mod.programCounter;
        // set PC to DEF FN code
        executeCommands(("GOTO " + valueToString(fn.gotoLine)).c_str());
        // run
        runToEnd();
        // reset PC to FN call
        mod.programCounter = pc;

        // extract variable of function name
        auto varIt   = mod.findOrCreateVariable(fn.fnName);
        Value retval = varIt->second;
        // remove the temporary FNX variable
        mod.variables.erase(varIt);

        // restore argument parameters
        for (auto& para : fn.parameters) {
            varIt         = mod.findOrCreateVariable(para.value);
            varIt->second = functionCallVariableStack.back();
            functionCallVariableStack.pop_back();
        }
        return retval;
    }

    // inline function body

    // Create a local token list with argument substitution
    std::vector<Token> substitutedBody = fn.body;
    std::vector<std::string> argumentStrings; // tokens use string_view!
    argumentStrings.reserve(fn.parameters.size());

    for (size_t i = 0; i < fn.parameters.size(); ++i) {
        TokenType valType = TokenType::NUMBER;
        char c            = fn.parameters[i].valuePostfix();
        if (c == '$') {
            valType = TokenType::STRING;
        } else if (c == '%') {
            valType = TokenType::INTEGER;
        }

        // replace the variable tokens with the passed arguments
        for (auto& token : substitutedBody) {
            if (token.type == TokenType::IDENTIFIER && token.value == fn.parameters[i].value) {
                argumentStrings.emplace_back(valueToString(arguments[i * 2 /* every other arg is the comma operator*/]));
                token.type  = valType;
                token.value = argumentStrings.back();
            }
        }
    }

    // Evaluate the substituted expression
    return evaluateExpression(substitutedBody, 0)[0];
}

// Evaluate expression using Shunting-Yard algorithm
// put endPtr to the next token to process
// put start to the open brace '(' - endPtr will point to the item after the matching close brace
// put start to inside the brace '(' - endPtr will point to matching the closing brace
// function calls itself recursively.
std::vector<Basic::Value> Basic::evaluateExpression(const std::vector<Token>& tokens, size_t start, size_t* ptrEnd, bool breakEarly) {
    std::vector<Value> output;
    output.reserve(tokens.size());
    std::vector<Value> values;
    values.reserve(tokens.size());
    std::vector<std::string_view> ops;
    ops.reserve(tokens.size());

    // static string pointers for string_view of unary operators
    static std::vector<std::string> unaryOperators;
    if (unaryOperators.empty()) {
        unaryOperators.reserve(0x100);
        for (size_t i = 0; i < 256; ++i) {
            unaryOperators.push_back("u");
        }
        unaryOperators[size_t('-')] = "u-";
        unaryOperators[size_t('+')] = "u+";
    }


    auto applyOpSS = [](const std::string_view& a, const std::string_view& b, const std::string_view& op) -> Value {
        if (op == "+") {
            return std::string(a) + std::string(b);
        }
        if (op == "=") {
            return a == b ? int64_t(-1) : int64_t(0);
        }
        if (op == ">") {
            return a > b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<") {
            return a < b ? int64_t(-1) : int64_t(0);
        }
        if (op == ">=") {
            return a >= b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<=") {
            return a <= b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<>") {
            return a != b ? int64_t(-1) : int64_t(0);
        }
        throw Error(ErrorId::SYNTAX);
    };
    auto applyOpDD = [](double a, double b, const std::string_view& op) -> Value {
        if (op == "+") {
            return a + b;
        }
        if (op == "-") {
            return a - b;
        }
        if (op == "*") {
            return a * b;
        }
        if (op == "/") {
            return a / b;
        }
        if (op == "=") {
            return fabs(a - b) < Basic::eps ? int64_t(-1) : int64_t(0);
        }
        if (op == ">") {
            return a > b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<") {
            return a < b ? int64_t(-1) : int64_t(0);
        }
        if (op == ">=") {
            return a >= b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<=") {
            return a <= b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<>") {
            return a != b ? int64_t(-1) : int64_t(0);
        }
        if (op == "^") {
            return pow(a, b);
        }
        if (op == "AND") {
            return int64_t(a) & int64_t(b);
        }
        if (op == "OR") {
            return int64_t(a) | int64_t(b);
        }
        if (op == "NOT") {
            return ~int64_t(b);
        }
        throw Error(ErrorId::SYNTAX);
    };
    auto applyOpII = [](int64_t a, int64_t b, const std::string_view& op) -> Value {
        if (op == "+") {
            return a + b;
        }
        if (op == "-") {
            return a - b;
        }
        if (op == "*") {
            return a * b;
        }
        if (op == "/") {
            return double(a) / double(b);
        }
        if (op == "=") {
            return a == b ? int64_t(-1) : int64_t(0);
        }
        if (op == ">") {
            return a > b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<") {
            return a < b ? int64_t(-1) : int64_t(0);
        }
        if (op == ">=") {
            return a >= b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<=") {
            return a <= b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<>") {
            return a != b ? int64_t(-1) : int64_t(0);
        }
        if (op == "^") {
            return int64_t(pow(a, b));
        }
        if (op == "AND") {
            return int64_t(a) & int(b);
        }
        if (op == "OR") {
            return int64_t(a) | int(b);
        }
        if (op == "NOT") {
            return ~int64_t(b);
        }
        throw Error(ErrorId::SYNTAX);
    };
    auto applyOpDI = [](double a, int64_t b, const std::string_view& op) -> Value {
        if (op == "+") {
            return a + double(b);
        }
        if (op == "-") {
            return a - double(b);
        }
        if (op == "*") {
            return a * double(b);
        }
        if (op == "/") {
            return a / double(b);
        }
        if (op == "=") {
            return fabs(a - double(b)) < Basic::eps ? int64_t(-1) : int64_t(0);
        }
        if (op == ">") {
            return a > b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<") {
            return a < double(b) ? int64_t(-1) : int64_t(0);
        }
        if (op == ">=") {
            return a >= double(b) ? int64_t(-1) : int64_t(0);
        }
        if (op == "<=") {
            return a <= double(b) ? int64_t(-1) : int64_t(0);
        }
        if (op == "<>") {
            return a != double(b) ? int64_t(-1) : int64_t(0);
        }
        if (op == "^") {
            return pow(double(a), double(b));
        }
        if (op == "AND") {
            return int64_t(a) & int64_t(b);
        }
        if (op == "OR") {
            return int64_t(a) | int64_t(b);
        }
        if (op == "NOT") {
            return ~int64_t(b);
        }
        throw Error(ErrorId::SYNTAX);
    };
    auto applyOpID = [](int64_t a, double b, const std::string_view& op) -> Value {
        if (op == "+") {
            return double(a) + b;
        }
        if (op == "-") {
            return double(a) - b;
        }
        if (op == "*") {
            return double(a) * b;
        }
        if (op == "/") {
            return double(a) / b;
        }
        if (op == "=") {
            return fabs(double(a) - b) < Basic::eps ? int64_t(-1) : int64_t(0);
        }
        if (op == ">") {
            return double(a) > b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<") {
            return double(a) < b ? int64_t(-1) : int64_t(0);
        }
        if (op == ">=") {
            return double(a) >= b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<=") {
            return double(a) <= b ? int64_t(-1) : int64_t(0);
        }
        if (op == "<>") {
            return double(a) != b ? int64_t(-1) : int64_t(0);
        }
        if (op == "^") {
            return pow(double(a), double(b));
        }
        if (op == "AND") {
            return int64_t(a) & int64_t(b);
        }
        if (op == "OR") {
            return int64_t(a) | int64_t(b);
        }
        if (op == "NOT") {
            return ~int64_t(b);
        }
        throw Error(ErrorId::SYNTAX);
    };

    // take operation from stack, execute it and put the result on the value stack
    auto applyOp = [&]() -> void {
        // printf("______________\n");
        // for (auto& v : values) {
        //     printf(valueToString(v).c_str());
        //     printf("\n");
        // }
        // printf(ops.back().c_str());
        // printf("\n");

        std::string_view op = ops.back();
        ops.pop_back();

        // just pop unary + operator: a * +b
        if (op == "u+") {
            return;
        }

        Value a;
        Value b = values.back();
        values.pop_back();

        if (op == "u-") // unary negative sign operator
        {
            op = "-";
            a  = Value(0.0);
        } else {
            if (values.empty()) {
                throw Error(ErrorId::SYNTAX);
            }
            a = values.back();
            values.pop_back();
        }
        // debug(valueToString(a).c_str()); debug(op.c_str()); debug(valueToString(b).c_str()); debug("\n");
        const double* da = get_if<double>(&a);
        const double* db = get_if<double>(&b);
        if (da && db) {
            values.push_back(applyOpDD(*da, *db, op));
            return;
        }
        const int64_t* ia = get_if<int64_t>(&a);
        const int64_t* ib = get_if<int64_t>(&b);
        if (ia && ib) {
            values.push_back(applyOpII(*ia, *ib, op));
            return;
        }
        if (da && ib) {
            values.push_back(applyOpDI(*da, *ib, op));
            return;
        }
        if (ia && db) {
            values.push_back(applyOpID(*ia, *db, op));
            return;
        }

        const std::string* sa = get_if<std::string>(&a);
        const std::string* sb = get_if<std::string>(&b);
        if (sa && sb) {
            values.push_back(applyOpSS(*sa, *sb, op));
            return;
        }
        if (sa) {
            values.push_back(applyOpSS(*sa, valueToString(b), op));
            return;
        }
        if (sb) {
            values.push_back(applyOpSS(valueToString(a), *sb, op));
            return;
        }

        throw Error(ErrorId::SYNTAX);
    };

    int openBraceCount = 0;
    for (size_t i = start; i < tokens.size(); i++) {
        // DEF FN A(X)=1 is stored as FNA()
        // you can call both: b=FN A(1) and FNA(1)!
        auto& toki = tokens[i];


        // PRINT "X" TAB(8) "Y" must return after each single piece
        // but PRINT 1+1 must be evaluated
        if (breakEarly && !values.empty() && ops.empty() && toki.type != TokenType::OPERATOR) {
            return values;
        }





        if (toki.type == TokenType::KEYWORD && toki.value == "FN") {
            continue;
        }

        if (ptrEnd != nullptr) {
            *ptrEnd = i + 1;
        }

        if (toki.type == TokenType::NUMBER) {
            values.push_back(atof(toki.value.data()));
        } else if (toki.type == TokenType::INTEGER) {
            values.push_back(strToInt(toki.value));
        } else if (toki.type == TokenType::STRING) {
            values.push_back(std::string(toki.value));
        } else if (toki.type == TokenType::IDENTIFIER) {
            // TODO A(5)=10 will automatically DIM A(10) in CBM BASIC.
            // function or array
            size_t end  = i;
            Value* pval = findLeftValue(currentModule(), tokens, i, &end);
            if (pval != nullptr) {
                values.push_back(*pval);
                if (end > i) {
                    i = end - 1;
                    if (ptrEnd != nullptr) {
                        *ptrEnd = i + 1;
                    }
                }
            } else {
                auto& prevToki = tokens[i > start ? i - 1 : i];

                // parse the comma separated list of arguments
                size_t end = i;
                auto args  = evaluateExpression(tokens, i + 2, &end);
                if (end < tokens.size() && tokens[end].value != ")") {
                    throw Error(ErrorId::SYNTAX);
                }
                if (end > i) {
                    i = end;
                    if (ptrEnd != nullptr) {
                        *ptrEnd = i + 1;
                    }
                }

                // built-in function
                auto fktit = functions.find(toki.value);
                if (fktit != functions.end()) {
                    // function(args)
                    auto retval = fktit->second(this, args); // call function
                    values.push_back(retval);
                    continue;
                }

                // See if it's a DEF FN
                static std::string strFN("FN");
                std::string fname;
                if (prevToki.type == TokenType::KEYWORD && prevToki.value == "FN") {
                    fname = strFN + std::string(toki.value);
                } else {
                    fname = toki.value;
                }
                auto defit = currentModule().functionTable.find(fname);
                if (defit != currentModule().functionTable.end()) {
                    auto retval = evaluateDefFnCall(defit->second, args);
                    values.push_back(retval);
                    continue;
                }
                throw Error(ErrorId::VARIABLE_UNDEFINED); // BASIC V7 would create this variable with a value of 0.0!
            }
        } else if (toki.type == TokenType::MODULE) {
            auto modit = modules.find(toki.value);
            if (modit == modules.end()) {
                throw Error(ErrorId::UNDEFD_MODULE);
            }
            ++i;
            size_t end  = i;
            Value* pval = findLeftValue(modit->second, tokens, i, &end);
            if (pval != nullptr) {
                values.push_back(*pval);
                if (end > i) {
                    i = end - 1;
                }
            }
        } else if (toki.type == TokenType::COMMA || (toki.type == TokenType::OPERATOR && toki.value == ";")) {
            while (!ops.empty() && precedence(ops.back()) >= precedence(toki.value)) {
                if (ops.back() == "(") {
                    ops.pop_back();
                    continue;
                }
                applyOp();
            }
            if (!values.empty()) {
                output.push_back(values.back());
                values.pop_back();
            }
            output.push_back(Operator(std::string(toki.value)));

            values.clear();
            ops.clear();
            continue;
        } else if (toki.type == TokenType::UNARY_OPERATOR) {
            // ops.push_back(std::string("u") + std::string(toki.value));
            ops.push_back(unaryOperators[(unsigned char)toki.value[0]]);

        } else if (toki.type == TokenType::OPERATOR) {
            while (!ops.empty() && precedence(ops.back()) >= precedence(toki.value)) {
                applyOp();
            }
            ops.push_back(toki.value);
        } else if (toki.value == "(") {
            ++openBraceCount;
            ops.push_back(toki.value);
        } else if (toki.value == ")") {
            if (ptrEnd != nullptr) {
                *ptrEnd = 1 + i;
            }

            while (!ops.empty() && ops.back() != "(") {
                applyOp();
            }

            if (!ops.empty()) {
                ops.pop_back(); // Remove "("
            }
            if (toki.type == TokenType::PARENTHESIS) {
                --openBraceCount;

                if (openBraceCount < 0) {
                    if (ptrEnd != nullptr) {
                        *ptrEnd = *ptrEnd - 1; /* put back ')' */
                    }
                    break;
                }
            }
        } else if (tokens[i].type == TokenType::FILEHANDLE) {
            throw Error(ErrorId::SYNTAX);
        } else if (tokens[i].type == TokenType::KEYWORD) {
            /* TO, STEP, ...*/
            if (ptrEnd != nullptr) {
                *ptrEnd = *ptrEnd - 1; /* put back ')' */
            }
            break;
        }
    }
    while (!ops.empty() && values.size() > 0) {
        applyOp();
    }

    while (ops.size()) {
        values.push_back(Operator(std::string(ops.back())));
        ops.pop_back();
    }
    if (!values.empty()) {
        for (auto& v : values) {
            output.push_back(v);
        }
    }
    return output;
}

Basic::ArrayIndex Basic::indexFromValues(const std::vector<Basic::Value>& vals) {
    ArrayIndex ai = {};
    size_t i      = 0;
    for (auto& v : vals) {
        if (valueIsOperator(v)) {
            continue;
        }
        int64_t iv = valueToInt(v);
        if (iv < 0) {
            throw Error(ErrorId::BAD_SUBSCRIPT);
        }
        ai.index[i++] = iv;
    }
    return ai;
}

void Basic::EndAndPopModule() {
    // debug("POPPING MODULE STACK\n");
    // we are running module's code
    if (moduleVariableStack.size() == moduleListingStack.size()) {
        moduleListingStack.back()->second.setProgramCounterToEnd();
        if (moduleListingStack.size() > 1) {
            moduleListingStack.pop_back();
        }
    }

    if (moduleVariableStack.size() > 1) {
        moduleVariableStack.pop_back();
    }
}

void Basic::doNEW() {
    auto& mod = currentModule();
    mod.filenameQSAVE.clear();
    mod.variables.clear();
    mod.arrays.clear();
    mod.listing.clear();
    // memory = {};
    mod.loopStack.clear();
    mod.setProgramCounterToEnd();
    mod.forceTokenizing();
}

void Basic::doEND() {
    EndAndPopModule();
}

void Basic::handleCLR() {
    auto& modl = currentModule();
    modl.variables.clear();
    modl.arrays.clear();
    modl.loopStack.clear();
}

// LET statement (assignment)
inline void Basic::handleLET(const std::vector<Token>& tokens) {
    size_t i = 0;
    if (tokens[0].value == "LET") {
        ++i;
    }
    size_t ivarname = i;
    size_t end      = i;

    Value* pval = nullptr;
    if (tokens[i].type == TokenType::MODULE) {
        auto modit = modules.find(tokens[i].value);
        if (modit == modules.end()) {
            throw Error(ErrorId::UNDEFD_MODULE);
        }
        ++i;
        pval = findLeftValue(modit->second, tokens, i, &end);
    } else {
        pval = findLeftValue(currentModule(), tokens, i, &end, true /*allow DIM of unused variables */);
    }

    if (pval != nullptr && end < tokens.size()) {
        i = end;
        if (tokens[i].value != "=") {
            throw Error(ErrorId::SYNTAX);
        }
        auto values = evaluateExpression(tokens, i + 1);
        if (values.size() != 1) {
            throw Error(ErrorId::SYNTAX);
        }

        switch (tokens[ivarname].valuePostfix()) {
        case '%':
            *pval = valueToInt(values[0]);
            break;
        case '$':
            *pval = values[0];
            break;
        default:
        case '#':
            *pval = valueToDouble(values[0]);
            break;
        }
    } else {
        throw Error(ErrorId::SYNTAX);
    }

    if (tokens[ivarname].value == "TI$") {
        std::string ti$ = valueToString(*pval);
        if (ti$.length() != 6) {
            throw Error(ErrorId::ILLEGAL_QUANTITY);
        }
        int64_t secs   = ((ti$[5] - '0') + (ti$[4] - '0') * 10) % 60;
        int64_t mins   = ((ti$[3] - '0') + (ti$[2] - '0') * 10) % 60;
        int64_t hours  = ((ti$[1] - '0') + (ti$[0] - '0') * 10) % 24;
        int64_t tiTick = secs * 1000 + mins * 60000 + hours * 60 * 60000;
        // #errormmins and hours not working
        // TI = ((tick()-time0)*60) / 1000
        time0 = os->tick() - tiTick;
    }
}

void Basic::handleRUN(const std::vector<Token>& tokens) {
    currentModule().autoNumbering = 0;


    int runFrom = 0;

    if (tokens.size() > 2) {
        auto vals = evaluateExpression(tokens, 1);
        runFrom   = int(valueToInt(vals[0]));
        if (runFrom < 0) {
            throw Error(ErrorId::UNDEFD_STATEMENT);
        }
    }

    // are we in not the same module as the programCounter?
    // then, push programCounter to stack and run the module code
    if (moduleVariableStack.size() != moduleListingStack.size()) {
        moduleListingStack.push_back(moduleVariableStack.back());
        currentModule().setProgramCounterToEnd();

        // debug("MODULE LISTING "); debug(moduleListingStack.back()->first.c_str()); debug("\n");
    }

    os->screen.clearHistory();

    handleCLR();
    currentModule().forceTokenizing();
    currentModule().restoreDataPosition();
    auto& listing = currentListing();
    for (auto it = listing.begin(); it != listing.end(); ++it) {
        if (it->first >= runFrom) {
            programCounter().line   = it;
            programCounter().cmdpos = 0;
            return;
        }
    }

    if (runFrom > 0) {
        throw Error(ErrorId::UNDEFD_STATEMENT); // run 999999
    }

    // no listing in module - exit to calling module
    doEND();
}

void Basic::handleMODULE(const std::vector<Token>& tokens) {
    std::string name = "";
    auto it          = modules.end();

    bool direct = currentModule().isInDirectMode();

    if (tokens.size() > 1) {
        name = std::string(tokens[1].value);
        Unicode::toUpper(name);
        it = modules.find(name);
    }
    if (it == modules.end()) {
        modules[name] = {};
        it            = modules.find(name);
    }
    moduleVariableStack.push_back(it);

    if (direct) {
        printUtf8String("MODULE IS " + name);
    }

    // debug("MODULE VARIABLES "); debug(name.c_str()); debug("\n");
}

void Basic::doPrintValue(Value& v) {
    // if (valueIsOperator(v)) {
    if (auto op = std::get_if<Basic::Operator>(&v)) {
        if (op->value == ",") {
            if (currentFileNo == 0) {
                auto crsr = os->screen.getCursorPos();
                crsr      = { (crsr.x / 10 + 1) * 10, crsr.y };
                os->screen.setCursorPos(crsr);
            } else {
                printUtf8String("    ");
            }
        } else if (op->value == ";") {
            // do nothing
        }
        // }
    } else {
        if (valueIsString(v)) {
            printUtf8String(Basic::valueToString(v), true /* apply control characters */, true /*even in quotes*/);
        } else {
            printUtf8String((" " + Basic::valueToString(v) + " "));
        }
    }
}

void Basic::handlePRINT(const std::vector<Token>& tokens) {
    size_t istart = 1;

    currentFileNo = 0;
    if (tokens.size() > 1 && tokens[1].type == TokenType::FILEHANDLE) {
        currentFileNo = strToInt(tokens[1].value);

        if (tokens.size() > 2 && tokens[2].type != TokenType::COMMA) {
            throw Basic::Error(Basic::ErrorId::SYNTAX);
        }

        // tokens.erase(tokens.begin() + 1);
        istart += 2; // handle and comma
    }

    if (tokens.size() < 1 + istart) {
        printUtf8String("\n");
        return;
    }

    if (tokens[1].type == TokenType::KEYWORD && tokens[1].value == "USING") {
        handlePRINT_USING(tokens);
        return;
    }

    std::string prt;
    bool forceNewline = true;
    for (;;) {
        auto vals = evaluateExpression(tokens, istart, &istart, true);
        if (vals.empty()) {
            break;
        }
        for (auto& v : vals) {
            doPrintValue(v);
            forceNewline = !valueIsOperator(v);
        }
    }
    if (forceNewline) {
        printUtf8String("\n");
    }
    currentFileNo = 0;
}

static std::string printUsing(const std::string& format, std::string value) {
    std::ostringstream oss;
    size_t fieldWidth = format.length();
    if (fieldWidth == 0) {
        throw Basic::Error(Basic::ErrorId::SYNTAX);
    }
    if (fieldWidth < value.length()) {
        value = value.substr(0, fieldWidth);
    }
    bool hasEqual = format.find('=') != std::string::npos;
    bool hasRight = format.find('>') != std::string::npos;
    if (hasEqual) {
        if (hasRight) {
            throw Basic::Error(Basic::ErrorId::SYNTAX);
        }
        size_t padLeft  = (fieldWidth - value.length()) / 2;
        size_t padRight = (fieldWidth - value.length() - padLeft);
        oss << std::string(padLeft, ' ') << value << std::string(padRight, ' ');
    } else if (hasRight) {
        oss << std::setw(fieldWidth) << std::setfill(' ') << std::right << value;
    } else {
        oss << std::setw(fieldWidth) << std::setfill(' ') << std::left << value;
    }
    return oss.str();
}

static std::string printUsing(const std::string& format, double value) {
    struct CustomThousandsSep : std::numpunct<char> {
    protected:
        char do_thousands_sep() const override { return ','; }
        std::string do_grouping() const override { return "\3"; }
    };

    // handle sign separately
    bool forcePlusSign = false;
    size_t signPos     = std::string::npos;
    size_t p           = format.find('+');
    if (p != std::string::npos) {
        signPos       = p;
        forcePlusSign = true;
    }
    p = format.find('-');
    if (p != std::string::npos) {
        signPos = p;
        if (forcePlusSign) {
            throw Basic::Error(Basic::ErrorId::SYNTAX);
        }
    }
    char sign = (value == 0) ? ' ' : ((value < 0) ? '-' : '+');
    if (signPos == std::string::npos) {
        sign = '*'; // never printed
    } else {
        value = fabs(value);
    }

    std::ostringstream oss;
    bool useExponential = format.find("^^^^") != std::string::npos;
    bool useDollar      = format.find('$') != std::string::npos;
    bool useComma       = format.find(',') != std::string::npos;
    size_t decimalPos   = format.find('.');
    size_t firstHash    = format.find('#');
    if (firstHash == std::string::npos) {
        throw Basic::Error(Basic::ErrorId::SYNTAX);
    }

    size_t fieldWidth          = format.length();
    size_t integerPartWidth    = (decimalPos != std::string::npos) ? decimalPos : fieldWidth;
    size_t fractionalPartWidth = (decimalPos != std::string::npos) ? (fieldWidth - decimalPos - 1) : 0;
    size_t hashBeforeDecimal   = std::count(format.begin(), format.begin() + integerPartWidth, '#');
    size_t hashAfterDecimal    = std::count(format.begin() + integerPartWidth, format.end(), '#');

    if (useExponential) {
        oss << std::scientific << std::setprecision(hashAfterDecimal);
    } else {
        oss << std::fixed << std::setprecision(hashAfterDecimal);
    }

    if (useDollar) {
        oss << "$";
    }
    if (signPos != std::string::npos && signPos <= firstHash) {
        if (forcePlusSign || sign == '-') {
            oss << sign;
            ++fieldWidth;
        }
    }

    if (useComma) {
        oss.imbue(std::locale(oss.getloc(), new CustomThousandsSep));
    }

    oss << std::setw(hashBeforeDecimal + hashAfterDecimal + (decimalPos != std::string::npos ? 1 : 0)) << std::setfill(' ') << std::right << value;

    if (signPos != std::string::npos && signPos > firstHash) {
        if (forcePlusSign || sign == '-') {
            oss << sign;
            ++fieldWidth;
        }
    }

    std::string ret = oss.str();
    if (ret.length() > fieldWidth) {
        ret = std::string(fieldWidth, '*');
    }
    return ret;
}

void Basic::handlePRINT_USING(const std::vector<Token>& tokens) {
    // tokens: 0     1     2       3  4      5 6
    //         PRINT USING FORMAT  ;  VALUE [, VALUE]

    std::string output;
    bool forceNewline = true;
    std::string format;
    size_t istart = 2;
    for (int irun = 0;; ++irun) {
        auto vals = evaluateExpression(tokens, istart, &istart, true);
        if (vals.empty()) {
            break;
        }
        if (irun == 0) {
            format = valueToString(vals[0]);
            vals.erase(vals.begin());
        }

        for (auto& v : vals) {
            forceNewline = true;
            if (valueIsOperator(v)) {
                forceNewline = false;
                continue;
            }

            auto isFormatChar = [](char c) { return c == '#' || c == '.' || c == ',' || c == '^' || c == '=' || c == '>' || c == '+' || c == '-' || c == '$'; };
            while (format.length() > 0 && !isFormatChar(format[0])) {
                output += format[0];
                format.erase(0, 1);
            }
            std::string subformat;
            while (format.length() > 0 && isFormatChar(format[0])) {
                subformat += format[0];
                format.erase(0, 1);
            }

            if (subformat.length() > 0) {
                if (valueIsString(v)) {
                    output += printUsing(subformat, valueToString(v));
                } else {
                    output += printUsing(subformat, valueToDouble(v));
                }
            } else {
                doPrintValue(v);
            }
        }
    }

    output += format;

    if (forceNewline) {
        output += '\n';
    }
    printUtf8String(output, true /* apply control characters */, true /* even in quotes*/);
}

void Basic::handleGET(const std::vector<Token>& tokens, bool waitForKeypress) {

    size_t istart = 1;
    currentFileNo = 0;
    if (tokens.size() > 1 && tokens[1].type == TokenType::FILEHANDLE) {
        currentFileNo = strToInt(tokens[1].value);

        if (tokens.size() > 2 && tokens[2].type != TokenType::COMMA) {
            throw Basic::Error(Basic::ErrorId::SYNTAX);
        }

        istart += 2; // handle and comma
    }

    if (currentFileNo != 0) {
        memory[krnl.STATUS] = Basic::FS_OK;
    }

    bool firstInput = true;
    std::string str;
    for (size_t itk = istart; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (tk.type != TokenType::COMMA) {
            Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
            if (pval == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }

            str.clear();

            if (currentFileNo == 0) {
                Os::KeyPress key {};
                if (waitForKeypress) {
                    key = os->getFromKeyboardBuffer();
                    handleEscapeKey();
                } else {
                    if (os->keyboardBufferHasData()) {
                        key = os->getFromKeyboardBuffer();
                    }
                }
                if (key.printable) {
                    if (options.uppercaseInput) {
                        key.code = Unicode::toUpper(key.code);
                    }
                    Unicode::appendAsUtf8(str, key.code);
                } else {
                    // https://www.commodore.ca/manuals/128_system_guide/app-i.htm
                    switch (key.code) {
                    case uint32_t(Os::KeyConstant::CRSR_LEFT):
                        Unicode::appendAsUtf8(str, 0x009d);
                        break; // L    157 9d
                    case uint32_t(Os::KeyConstant::CRSR_RIGHT):
                        Unicode::appendAsUtf8(str, 0x001d);
                        break; // R     29 1d
                    case uint32_t(Os::KeyConstant::CRSR_UP):
                        Unicode::appendAsUtf8(str, 0x0091);
                        break; // U    145 91
                    case uint32_t(Os::KeyConstant::CRSR_DOWN):
                        Unicode::appendAsUtf8(str, 0x0011);
                        break; // D     17 11
                    case uint32_t(Os::KeyConstant::F1):
                        Unicode::appendAsUtf8(str, 0x0085);
                        break; // F1   133 85
                    case uint32_t(Os::KeyConstant::F2):
                        Unicode::appendAsUtf8(str, 0x0086);
                        break;
                    case uint32_t(Os::KeyConstant::F3):
                        Unicode::appendAsUtf8(str, 0x0087);
                        break;
                    case uint32_t(Os::KeyConstant::F4):
                        Unicode::appendAsUtf8(str, 0x0088);
                        break;
                    case uint32_t(Os::KeyConstant::F5):
                        Unicode::appendAsUtf8(str, 0x0089);
                        break;
                    case uint32_t(Os::KeyConstant::F6):
                        Unicode::appendAsUtf8(str, 0x008a);
                        break;
                    case uint32_t(Os::KeyConstant::F7):
                        Unicode::appendAsUtf8(str, 0x008b);
                        break;
                    case uint32_t(Os::KeyConstant::F8):
                        Unicode::appendAsUtf8(str, 0x008c);
                        break; // F8   140 8c
                    case uint32_t(Os::KeyConstant::HOME):
                        Unicode::appendAsUtf8(str, 0x0093);
                        break; // home 147 93
                    case uint32_t(Os::KeyConstant::DEL):
                        Unicode::appendAsUtf8(str, 0x0014);
                        break; // delete   20 14
                    case uint32_t(Os::KeyConstant::ESCAPE):
                        Unicode::appendAsUtf8(str, 0x001b);
                        break; // ESC   27 1b
                    case 0x00a3:
                        Unicode::appendAsUtf8(str, 0x005c);
                        break; // pound Unicode 163
                    }
                }

            } else { // read from file
                auto& fp = fileHandles[currentFileNo];
                if (!fp) {
                    throw Error(ErrorId::FILE_NOT_OPEN);
                }
                char buffer[4] = { 0, 0, 0, 0 };
                if (fp.read(&buffer[0], 1) == 1) {
                    str = buffer;
                } else {
                    memory[krnl.STATUS] = Basic::FS_EOF;
                }
            }

            switch (tk.valuePostfix()) {
            case '%':
                *pval = valueToInt(str);
                break;
            case '$':
                *pval = str;
                break;
            default:
            case '#':
                *pval = valueToDouble(str);
                break;
            }
        }
    }
    currentFileNo = 0;
}

// INPUT from file
void Basic::handleINPUTFile(const std::vector<Token>& tokens) {
    currentFileNo = 0;
    currentFileNo = strToInt(tokens[1].value);

    if (tokens.size() > 2 && tokens[2].type != TokenType::COMMA) {
        throw Basic::Error(Basic::ErrorId::SYNTAX);
    }

    size_t istart = 3; // INPUT handle and comma

    auto& fp = fileHandles[currentFileNo];
    if (!fp) {
        memory[krnl.STATUS] = Basic::FS_ERROR_READ;
        throw Error(ErrorId::FILE_NOT_OPEN);
    }

    if ((memory[krnl.STATUS] & Basic::FS_EOF) == 0) {
        memory[krnl.STATUS] = Basic::FS_OK;
    }

    for (size_t itk = istart; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (tk.type != TokenType::COMMA && tk.type != TokenType::OPERATOR) {
            Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
            if (pval == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }

            char firstChar = '\0';
            std::string s;
            if (memory[krnl.STATUS] & Basic::FS_EOF) {
                // CBM BASIC sets ST to EOF after the last read.
                // if there is no more data, it adds the ERROR_READ flag
                // the last read string is repeatedly returned, though.
                // I decided to return an empty string instead.
                memory[krnl.STATUS] |= Basic::FS_ERROR_READ;
            } else {
                for (;;) {
                    char buffer[4] = { 0, 0, 0, 0 };
                    if (!(fp.read(&buffer[0], 1) == 1)) {
                        memory[krnl.STATUS] |= Basic::FS_EOF;
                        buffer[0] = '\0';
                    }

                    if (s.empty()) {
                        firstChar = buffer[0];
                        if (firstChar == '\"') {
                            continue;
                        }
                    }

                    if (
                        firstChar != '\"' && (buffer[0] == ',' || buffer[0] == ';' || buffer[0] == ':')
                        || buffer[0] == '\0'
                        || buffer[0] == '\r'
                        || buffer[0] == '\n') {
                        break;
                    }
                    s += buffer;
                }
                // trim closing quotes when started with quotes
                if (firstChar == '\"' && s.ends_with("\"")) {
                    s = s.substr(0, s.length() - 1);
                }
            }

            switch (tk.valuePostfix()) {
            case '%':
                *pval = valueToInt(s);
                break;
            case '$':
                *pval = s;
                break;
            default:
            case '#':
                *pval = valueToDouble(s);
                break;
            }
        }
    }
    currentFileNo = 0;
}

void Basic::handleINPUT(const std::vector<Token>& tokens) {
    if (tokens.size() > 1 && tokens[1].type == TokenType::FILEHANDLE) {
        handleINPUTFile(tokens);
        return;
    }

    bool firstInput = true;
    for (size_t itk = 1; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (itk == 1 && tk.type == TokenType::STRING) {
            printUtf8String(tk.value);
            if (itk + 1 < tokens.size() && tokens[itk + 1].value != ";") {
                printUtf8String("\n");
            }
        } else if (tk.type != TokenType::COMMA && tk.type != TokenType::OPERATOR) {
            Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
            if (pval == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }

            bool badinput = true;
            while (badinput) {
                try {
                    if (firstInput) {
                        printUtf8String("? ");
                        firstInput = false;
                    } else {
                        printUtf8String("?? ");
                    }
                    std::string s = inputLine(false);

                    if (options.uppercaseInput) {
                        s = Unicode::toUpper(s.c_str());
                    }

                    // printUtf8String("\n");

                    switch (tk.valuePostfix()) {
                    case '%':
                        *pval = valueToInt(s);
                        break;
                    case '$':
                        *pval = s;
                        break;
                    default:
                    case '#':
                        *pval = valueToDouble(s);
                        break;
                    }
                    badinput = false;
                } catch (const Error& e) {
                    if (e.ID == ErrorId::BREAK) {
                        throw e;
                    }

                    printUtf8String("? REDO FROM START \n");
                }
            }
        }
    }
}

void Basic::handleNETGET(const std::vector<Token>& tokens) {
    if (tokens.size() != 4) {
        throw Error(ErrorId::ARGUMENT_COUNT);
    }

    size_t starttok  = 1;
    auto firstValues = evaluateExpression(tokens, starttok, &starttok, true);
    if (firstValues.size() != 1) {
        throw Error(ErrorId::SYNTAX);
    }

    MiniFetch fetch;
    fetch.request.fillServerFromUrl(valueToString(firstValues[0]));
    auto response = fetch.fetch();


    bool firstInput = true;
    for (size_t itk = starttok; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (tk.type != TokenType::COMMA) {
            Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
            if (pval == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }

            switch (tk.valuePostfix()) {
            case '$':
                *pval = response.toString();
                break;
            default:
                throw Error(ErrorId::TYPE_MISMATCH);
                break;
            }
        }
    }
}



// DIM statement (array declaration)
inline void Basic::handleDIM(const std::vector<Token>& tokens) {
    if (tokens.size() < 2) {
        return;
    }
    // if (tokens[2].type != TokenType::PARENTHESIS || tokens.back().type != TokenType::PARENTHESIS) {
    //     throw Error(ErrorId::SYNTAX);
    // }

    // DIM a(b,c), d(e,f)
    auto& arrays        = currentModule().arrays;
    char varnamePostfix = '#';
    size_t end          = 0;
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].type != TokenType::IDENTIFIER) {
            throw Error(ErrorId::SYNTAX);
        }
        std::string varName(tokens[i].value);
        varnamePostfix = tokens[i].valuePostfix();
        ++i;
        if (i >= tokens.size() || tokens[i].type != TokenType::PARENTHESIS || tokens[i].value != "(") {
            Value* value = findLeftValue(currentModule(), tokens, i - 1, &i);
            if (value == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }
            if (valueIsString(*value)) {
                *value = "";
            } else if (valueToInt(*value)) {
                *value = 0;
            } else {
                *value = 0.0;
            }
            continue;
        }

        auto vals = evaluateExpression(tokens, i + 1, &end);

        // DIM a$() -> dictionary
        if (vals.empty()) {
            arrays[varName].setIsDictionary(true);
        } else {
            Value init;
            switch (varnamePostfix) {
            case '#': init = 0.0; break;
            case '%': init = int64_t(0); break;
            case '$': init = std::string(); break;
            }

            auto bounds = indexFromValues(vals);
            arrays[varName].dim(init, bounds);
            if (end > i) {
                i = end - 1;
            }
        }


        if (tokens[i + 1].type != TokenType::PARENTHESIS) {
            throw Error(ErrorId::SYNTAX);
        }

        // optionally more arrays
        if (i + 2 < tokens.size() && tokens[i + 2].type != TokenType::COMMA) {
            throw Error(ErrorId::SYNTAX);
        } else {
            i += 2;
        }
    }
}

void Basic::handleLIST(const std::vector<Token>& tokens) {
    static int lastFrom = 0, lastTo = 0x7fffffff;
    int from = 0;
    int to   = 0x7ffffff;

    if (tokens.size() == 2) {
        if (tokens[1].type == TokenType::KEYWORD && tokens[1].value == "MODULE") {
            // LIST MODULE
            for (auto& lmd : modules) {
                os->screen.cleanCurrentLine();
                printUtf8String("MODULE \"" + lmd.first + "\"\n");

                handleEscapeKey(true);
                os->delay(50);
            }
            return;
        }
        if (tokens[1].type == TokenType::COMMAND && tokens[1].value == "BAKE") {
            // LIST BAKE
            executeCommands("FIND \"REM*--*--*\"");
            return;
        }
    }


    if (tokens.size() == 1) {
        // list all
    } else if (tokens.size() == 2 && tokens[1].type != TokenType::OPERATOR) {
        // LIST 10
        from = to = int(strToInt(tokens[1].value));
    } else if (tokens.size() == 4 && tokens[2].type == TokenType::OPERATOR) {
        // LIST 10-20
        from = int(strToInt(tokens[1].value));
        to   = int(strToInt(tokens[3].value));
        if (to < from) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }
    } else if (tokens.size() == 3 && tokens[1].type == TokenType::UNARY_OPERATOR) {
        // LIST -20
        to = int(strToInt(tokens[2].value));
    } else if (tokens.size() == 3 && tokens[2].type == TokenType::OPERATOR) {
        // LIST 20-
        from = int(strToInt(tokens[1].value));
    } else if (tokens.size() == 2 && tokens[1].type == TokenType::OPERATOR) {
        // LIST -
        from = lastFrom;
        to   = lastTo;
    } else {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    lastFrom = from;
    lastTo   = to;

    std::string sline;


    std::string moduleName = moduleListingStack.back()->first;
    uint8_t colForMod      = colorForModule(moduleName);
    std::string colLN;
    Unicode::appendAsUtf8(colLN, ControlCharacters::textColor1_White);
    std::string colKW;
    Unicode::appendAsUtf8(colKW, ControlCharacters::charForColor(colForMod));
    std::string colQU;
    Unicode::appendAsUtf8(colQU, ControlCharacters::textColor15_Light_Gray);
    std::string colREM;
    Unicode::appendAsUtf8(colREM, ControlCharacters::charForColor(ScreenBuffer::buddyColor(colForMod)));

    // now, that's a hack
    if (!options.colorzizeListing) {
        colKW.clear();
        colLN.clear();
        colQU.clear();
    }

    auto& module = currentModule();
    for (auto& ln : module.listing) {
        if (ln.first < from) {
            continue;
        }
        if (ln.first > to) {
            break;
        }

        // line number
        sline = colLN + std::to_string(ln.first);
        for (size_t s = sline.length(); s < 3; ++s) {
            sline += ' ';
        }
        sline += ' ';

        bool inQuotes = false, inQuotes2 = false;
        for (const char* ptr = ln.second.code.c_str(); *ptr != '\0'; ++ptr) {
            char c = *ptr;
            if (!inQuotes && c == '\"') {
                sline += colQU;
            }
            if (!inQuotes2 && c == '\'') {
                sline += colQU;
            }

            const char* peek = ptr;
            std::string_view range;
            if (!inQuotes
                && !inQuotes2
                && !isWhiteSpace(*ptr)
                && (parseCommand(peek, &range)
                    || parseKeyword(peek, &range)
                    || parseFunction(peek, &range))) {
                sline += colKW;
                sline += range;
                ptr += range.length();

                if (range == "REM") {
                    sline += colREM;
                    sline += ptr;
                    sline += colLN;
                    break;
                }

                sline += colLN;
                if (range.length() != 0) {
                    --ptr; // for loop increments
                }
                continue;
            }

            sline += c;
            if (inQuotes && c == '\"') {
                sline += colLN;
            }
            if (inQuotes2 && c == '\'') {
                sline += colLN;
            }
            if (c == '\"') {
                inQuotes = !inQuotes;
            }
            if (c == '\'') {
                inQuotes2 = !inQuotes;
            }
        }
        sline += colKW;

        // sline += ln.second;
        sline += '\n';
        os->screen.cleanCurrentLine();
        printUtf8String(sline, true /*ctrl*/, false /*but not in quotes*/);

        handleEscapeKey(true);
        os->delay(50);
    }
}

void Basic::handleDELETE(const std::vector<Token>& tokens) {
    int from = 0;
    int to   = 0x7ffffff;
    if (tokens.size() > 1) {
        from = int(strToInt(tokens[1].value));
        if (from < 0) {
            from = 0;
        }
    }
    if (tokens.size() > 3 && tokens[2].type == TokenType::OPERATOR) {
        to = int(strToInt(tokens[3].value));
        if (to < from) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }
    }

    auto& lst = currentModule().listing;
    for (auto ln = lst.cbegin(); ln != lst.cend() /* not hoisted */; /* no increment */) {
        if (ln->first >= from && ln->first <= to) {
            lst.erase(ln++);
        } else {
            ++ln;
        }
    }
    currentModule().forceTokenizing();
}

// List all variables and their values
void Basic::handleDUMP(const std::vector<Token>& tokens) {
    std::set<std::string> varnames;
    for (auto& tok : tokens) {
        if (tok.type != TokenType::IDENTIFIER) {
            continue;
        }
        varnames.insert(std::string(tok.value));
    }



    std::ostringstream oss;
    oss << "MODULE " << moduleVariableStack.back()->first << std::endl;
    if (currentModule().variables.empty() && currentModule().arrays.empty()) {
        return;
    }

    for (auto& v : currentModule().variables) {
        if (!varnames.empty() && !varnames.contains(v.first)) {
            continue;
        }
        oss << v.first << " = " << valueToString(v.second) << std::endl;
    }
    for (auto& v : currentModule().arrays) {
        auto& arr = v.second;
        if (!varnames.empty() && !varnames.contains(v.first)) {
            continue;
        }

        if (arr.isDictionary) {
            int count = 0;
            for (auto& p : arr.dict) {
                oss << v.first << "(" << valueToString(p.first) << ") = " << valueToString(p.second) << std::endl;
                if (++count > 9) {
                    oss << "..." << std::endl;
                    break;
                }
            }

            continue;
        }

        switch (arr.bounds.dimensions()) {
        case 1:
            oss << v.first << "(" << arr.bounds.index[0] << " )" << std::endl;
            for (size_t i = 0; i < std::min(size_t(10), arr.bounds.index[0]); ++i) {
                oss << "    (" << int(i) << ") = " << valueToString(arr.at(ArrayIndex(i))) << std::endl;
            }
            if (arr.bounds.index[0] >= 10) {
                oss << "    (" << int(arr.bounds.index[0]) << ") = " << valueToString(arr.at(arr.bounds.index[0])) << std::endl;
            }
            break;
        case 2: oss << v.first << "(" << arr.bounds.index[0] << ", " << arr.bounds.index[1] << " )" << std::endl; break;
        case 3: oss << v.first << "(" << arr.bounds.index[0] << ", " << arr.bounds.index[1] << ", " << arr.bounds.index[2] << " )" << std::endl; break;
        case 4: oss << v.first << "(" << arr.bounds.index[0] << ", " << arr.bounds.index[1] << ", " << arr.bounds.index[2] << ", " << arr.bounds.index[3] << " )" << std::endl; break;
        }
    }

    // print and allow shift to slowdown
    auto lines = StringHelper::split(oss.str(), "\r\n");
    std::string sline;
    for (auto& ln : lines) {
        os->screen.cleanCurrentLine();
        printUtf8String(ln + "\n");
        handleEscapeKey(true);
        os->delay(50);
    }
}

void Basic::handleKEY(const std::vector<Token>& tokens) {
    if (tokens.size() == 1) {
        for (size_t k = 0; k < keyShortcuts.size(); ++k) {
            std::string s = "KEY " + valueToString(int64_t(1 + k)) + "," + keyShortcuts[k] + " \n";
            os->screen.cleanCurrentLine();
            printUtf8String(s);
        }
        return;
    }
    if (tokens.size() < 4) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    size_t k = strToInt(tokens[1].value) - 1;
    if (k >= keyShortcuts.size()) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    auto& str = keyShortcuts[k];
    str       = "";
    for (size_t i = 3; i < tokens.size(); ++i) {
        if (tokens[i].type == TokenType::STRING) {
            str += "\"" + std::string(tokens[i].value) + "\"";
        } else {
            str += tokens[i].value;
        }
    }
}

void Basic::handleRCHARDEF(const std::vector<Token>& tokens) {
    int iarg = 0; // nth argument to assign

    // i_arg         0        1   2
    // RCHARDEF i_char, is_mono, b1, b2, b3, b4, b5, b6, b7, b8

    CharBitmap bmp;
    for (size_t itk = 1; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (itk == 1) {
            std::string str;
            if (tk.type == TokenType::STRING) {
                str = tk.value;
            } else if (tk.type == TokenType::IDENTIFIER) {
                Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
                if (pval != nullptr) {
                    str = valueToString(*pval);
                } else {
                    // RCHARDEF CHR$(1+1), mono, a,b,c,d,e,f,g,h
                    throw Error(ErrorId::FORMULA_TOO_COMPLEX);
                    // auto values = evaluateExpression(tokens, itk + 1, &itk);
                    // if (vals.size() != 1) {
                    //     throw Error(ErrorId::SYNTAX);
                    // }
                    // str = valueToString(values[0]);
                }
            } else {
                throw Error(ErrorId::FORMULA_TOO_COMPLEX);
            }
            if (str.empty()) {
                throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
            }
            const char* ptr = str.c_str();
            char32_t ichar  = Unicode::parseNextUtf8(ptr);
            bmp             = os->screen.getCharDefinition(ichar);
            continue;
        }
        if (iarg > 8) {
            throw Error(ErrorId::ARGUMENT_COUNT);
        }
        if (tk.type != TokenType::COMMA) {
            Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
            if (pval == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }
            int64_t v = 0;
            if (iarg == 0) {
                v = bmp.isMono ? -1 : 1;
            } else {
                if (bmp.isMono) {
                    v = bmp.bits[iarg - 1];
                } else {
                    size_t n = (iarg - 1) * 8;
                    for (size_t i = 0; i < 8; ++i) {
                        size_t nibble = bmp.multi(n + i);
                        ;
                        v <<= 4;
                        v += nibble;
                    }
                }
            }

            switch (tk.valuePostfix()) {
            case '%':
                *pval = v;
                break;
            case '$':
                *pval = valueToString(v);
                break;
            default:
            case '#':
                *pval = double(v);
                break;
            }
            ++iarg;
        }
    }
}


// put endPtr to the next token to process
// returns nullptr on error
Basic::Value* Basic::findLeftValue(Module& module, const std::vector<Token>& tokens, size_t start, size_t* endPtr, bool allowDimArray) {
    size_t i = start;
    if (tokens.size() > i + 2 && tokens[i + 1].type == TokenType::PARENTHESIS && tokens[i + 1].value == "(") {
        auto& arrays = module.arrays;
        auto arrit   = arrays.find(tokens[i].value);
        if (arrit == arrays.end()) {
            if (allowDimArray) {
                executeCommands(("DIM " + std::string(tokens[0].value) + "(10)").c_str());
                // auto dimtok = tokenizeNextCommand(cmd);
                // handleDIM(dimtok[0]);

                allowDimArray = false;
                return findLeftValue(module, tokens, start, endPtr, false);
            }
            return nullptr; // might be a function call
        }
        // parse the comma separated list of arguments
        size_t end = i;
        auto args  = evaluateExpression(tokens, i + 2, &end);
        if (end >= tokens.size() || tokens[end].value != ")") {
            throw Error(ErrorId::SYNTAX);
        }
        i = end + 1; // after brace

        if (endPtr) {
            *endPtr = i;
        }
        if (arrit != arrays.end()) {
            auto& arr = arrit->second;

            if (arr.isDictionary) {
                if (args.size() != 1) {
                    throw Error(ErrorId::BAD_SUBSCRIPT);
                }
                return &arr.atKey(args[0]);
            } else {
                // array(index)
                auto arridx = this->indexFromValues(args);
                return &arr.at(arridx);
            }
        } else {
            return nullptr;
        }
    }

    if (tokens[i].type == TokenType::IDENTIFIER) {

        if (endPtr) {
            *endPtr = 1 + start;
        }

        const std::string_view variableName = tokens[i].value;
        static const std::string_view svTI("TI");
        static const std::string_view svTI$("TI$");
        static const std::string_view svST("ST");
        static const std::string_view svSTATUS("STATUS");
        if (variableName == svTI || variableName == svTI$) { //  TI or TI$
            int64_t ti_ms = os->tick() - time0;
            int64_t ti    = int64_t((ti_ms) * 60LL) / 1000LL; // jiffies
            TIvariable    = Basic::Value(ti);

            if (variableName == svTI$) {
                int64_t secs  = ti_ms / 1000;
                int64_t mins  = secs / 60;
                int64_t hours = mins / 60;

                secs %= 60;
                mins %= 60;
                hours %= 24;

                auto str2 = [](int64_t i) -> std::string {
                    static const char* nums = "0123456789";
                    char buf[3]             = { nums[(i / 10) % 10], nums[i % 10], '\0' };
                    return { buf };
                };
                TI$variable = str2(hours) + str2(mins) + str2(secs);
                return &TI$variable;
            }
            // #error not working ti$="000100":?ti$

            return &TIvariable; // jiffies (1/60th seconds)
        } else if (variableName == svST || variableName == svSTATUS) {
            STvariable = int64_t(memory[krnl.STATUS]);
            return &STvariable;
        }

        auto varit = module.findOrCreateVariable(variableName);
        return &varit->second;
    }

    return nullptr;
}

void Basic::doGOTO(int line, bool isGoSub) {

    if (line < 0) {
        int pase = 1;
    }

    // are we in not the same module as the programCounter?
    // then, push programCounter to stack and goto the module code
    if (moduleVariableStack.size() != moduleListingStack.size()) {
        moduleListingStack.push_back(moduleVariableStack.back());
        ASSERT(moduleVariableStack.size() != moduleListingStack.size());
        currentModule().setProgramCounterToEnd();
    }

    if (isGoSub) {
        LoopItem loop;
        loop.varName = "GOSUB";
        loop.jump    = programCounter();
        currentModule().loopStack.push_back(loop);
    }
    // TODO make sure you're in the module code now
    auto& listing = currentListing();
    auto it       = listing.find(line);
    if (it == listing.end()) {
        throw Error(ErrorId::UNDEFD_STATEMENT);
    }
    programCounter().line   = it;
    programCounter().cmdpos = 0;
}

void Basic::handleGOTO(const std::vector<Token>& tokens) {
    auto values = evaluateExpression(tokens, 1);
    if (values.size() == 0 || values.size() > 2) {
        throw Error(ErrorId::SYNTAX);
    }
    int64_t line = valueToInt(values.front());
    // TODO why? tokens.clear();
    doGOTO(int(line), false);
}

void Basic::handleGOSUB(const std::vector<Token>& tokens) {
    auto values = evaluateExpression(tokens, 1);
    if (values.size() == 0 || values.size() > 2) {
        throw Error(ErrorId::SYNTAX);
    }
    int64_t line = valueToInt(values.front());
    // TODO why ? tokens.clear();
    doGOTO(int(line), true);
}

void Basic::handleONGOTO(const std::vector<Token>& tokens) {
    // ON expression GOSUB/GOTO line1, line2
    size_t i   = 1;
    int64_t on = valueToInt(evaluateExpression(tokens, i, &i)[0]);

    auto lines = evaluateExpression(tokens, i + 1);
    if (tokens[i].value == "GOTO") {
        doGOTO(int(valueToInt(lines[(on - 1) * 2])), false); // 1=[0], 2=[2], 3=[4]
    } else if (tokens[i].value == "GOSUB") {
        doGOTO(int(valueToInt(lines[(on - 1) * 2])), true); // 1=[0], 2=[2], 3=[4]
    } else {
        throw Error(ErrorId::SYNTAX);
    }
}

void Basic::handleHELP(const std::vector<Token>& tokens) {
    std::string cmd;
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (i > 1) {
            cmd += " ";
        }
        cmd += tokens[i].value;
    }
    cmd             = Unicode::toUpperAscii(cmd.c_str());
    std::string usg = Help::getUsage(cmd) + " \n";
    os->screen.cleanCurrentLine();
    printUtf8String(usg);
}

void Basic::handleDEFFN(const std::vector<Token>& intokens) {
    std::string fname;

    if (intokens.size() < 8) {
        throw Error(ErrorId::SYNTAX);
    }

    // tokens:
    // 0   12     23 3 4 45 56 67   <- tokens
    // DEF FNname (  X   )  =  Y
    //     0      1  2   3  4  5   <- bodyTokens

    size_t startParsing = 2;
    if (intokens[1].value == "FN") {
        ++startParsing;
        fname = "FN" + std::string(intokens[2].value);
    } else {
        fname = intokens[1].value;
    }

    auto& module = currentModule();

    FunctionDefinition& def = module.functionTable[fname];
    def.clear();
    def.fnName = fname;

    def.lineCopy = fname;
    for (size_t i = startParsing; i < intokens.size(); ++i) {
        def.lineCopy += " ";
        def.lineCopy += std::string(intokens[i].value);
    }

    std::vector<Token>& bodyTokens = def.body;
    tokenizeNextCommand(def.lineCopy.c_str(), def.body);

    // tokens =   0    1  2  ... 3  4  5+
    //         { name, (, X, ... ), =, code...}
    if (def.body.size() < 3 || def.body[1].type != TokenType::PARENTHESIS
        || def.body[2].type != TokenType::IDENTIFIER) {
        throw Error(ErrorId::SYNTAX);
    }

    size_t i;
    size_t iStartBody = 0;
    for (i = 2; i + 1 < def.body.size(); i += 2) {
        if (def.body[i].type != TokenType::IDENTIFIER) {
            throw Error(ErrorId::SYNTAX);
        }
        def.parameters.push_back(def.body[i]);
        if (def.body[i + 1].type != TokenType::COMMA
            && def.body[i + 1].type != TokenType::PARENTHESIS) {
            throw Error(ErrorId::SYNTAX);
        }

        if (def.body[i + 1].type == TokenType::PARENTHESIS) {
            if (i + 2 < def.body.size() && def.body[i + 2].value == "=") {
                // definition continues with equals (=) sign:

                // store the function definition:
                iStartBody = i + 3; // drop args, ), =
                def.body.erase(def.body.begin(), def.body.begin() + iStartBody);

                if (def.body.empty()) {
                    throw Error(ErrorId::SYNTAX);
                }

            } else {
                // no equals (=) after closing brace
                // BA67 allows Dartmouth BASIC's multiline DEF FN
                def.body.clear();

                // skip DEF FN line
                module.programCounter.line++;
                def.gotoLine = module.programCounter.line->first;

                // skip to find FNEND
                for (;;) {
                    if (module.programCounter.line == module.listing.end()) {
                        throw Error(ErrorId::DEF_WITHOUT_FNEND);
                    }
                    const char* code = module.programCounter.line->second.code.c_str();
                    skipWhite(code);
                    if (strncmp(code, "FNEND", 5) == 0) {
                        module.programCounter.line++;
                        module.programCounter.cmdpos = 0;
                        break;
                    }
                    module.programCounter.line++;
                }
            }
            break;
        }
    }

    // module.functionTable[fname] = def;
}

// Handle FOR statement
void Basic::handleFOR(const std::vector<Token>& tokens) {
    if (tokens.size() < 6 || tokens[2].value != "=") {
        return;
    }
    std::string varName(tokens[1].value); // TODO why the copy for std::unordered_map::operator[]
    auto values = evaluateExpression(tokens, 3);
    if (values.size() != 1) {
        throw Error(ErrorId::SYNTAX);
    }
    int64_t start = valueToInt(values.back());

    auto& modl = currentModule();

    modl.variables[varName] = values.back();

    int64_t toEnd = start;
    int64_t step  = 1;
    for (size_t i = 4; i < tokens.size(); ++i) {
        if (tokens[i].value == "TO") {
            auto values = evaluateExpression(tokens, i + 1);
            if (values.size() != 1) {
                throw Error(ErrorId::SYNTAX);
            }
            toEnd = valueToInt(values.back());
        }
        if (tokens[i].value == "STEP") {
            auto values = evaluateExpression(tokens, i + 1);
            if (values.size() != 1) {
                throw Error(ErrorId::SYNTAX);
            }
            step = valueToInt(values.back());
        }
    }
    modl.variables[varName] = int64_t(start);

    // re-use the same loop variable if it's on the stack.
    Basic::LoopItem loopParam { Basic::LoopItem::FORNEXT, varName, start, toEnd, step, modl.programCounter };

    for (int i = int(modl.loopStack.size()) - 1; i >= 0; --i) {
        auto& it = modl.loopStack[i];
        if (it.type != Basic::LoopItem::FORNEXT) {
            break;
        }
        if (modl.loopStack[i].varName == varName) {
            // pop any inner loops and this loop
            while (modl.loopStack.size() > i) {
                modl.loopStack.pop_back();
            }
            break;
        }
    }
    modl.loopStack.push_back(loopParam);
}

// Handle NEXT statement
/*
C64 BASIC deals with information about FOR-NEXT loops on the return stack in one of four ways:

    - When the FOR-NEXT loop terminates normally, the information is removed from the return stack.
    - When a program encounters a NEXT statement with an explicit loop variable, all inner loops are
      cancelled and their information removed from the return stack.
    - All FOR-NEXT loops commenced within a subroutine are terminated and their information removed
      from the return stack when the program encounters a RETURN statement.
    - If a FOR statement is encountered, any existing loop using the same variable name along with
      any subsequent unfinished FOR-NEXT loops are terminated and their information removed from
      the return stack.

Examples:
// NEXT searches for the matching FOR, but not beyond a GOSUB!
10 FOR I=1 TO 5
20 GOSUB 100
30 NEXT I
100 NEXT I : REM "?NEXT WITHOUT FOR"


// FOR searches for an already open for with that variable, but stops at a gosub
10 FOR I = 1 TO 3
20 PRINT "OUTER FOR I"
30 GOSUB 100
40 PRINT "BACK FROM GOSUB"
50 NEXT I
60 END
100 FOR I = 1 TO 1
110 PRINT "INNER FOR I"
120 NEXT I
130 RETURN
RUN
OUTER FOR I
INNER FOR I
BACK FROM GOSUB
OUTER FOR I
INNER FOR I
BACK FROM GOSUB
...
*/
void Basic::handleNEXT(const std::vector<Token>& tokens) {
    auto& modl = currentModule();

    std::string varName;
    if (tokens.size() > 1) {
        varName = tokens[1].value;

        for (int i = int(modl.loopStack.size()) - 1; i >= 0; --i) {
            auto& it = modl.loopStack[i];
            if (it.type != Basic::LoopItem::FORNEXT) {
                break;
            }
            if (modl.loopStack[i].varName == varName) {
                // pop any inner loops
                while (modl.loopStack.size() > i + 1) {
                    modl.loopStack.pop_back();
                }
                break;
            }
        }
    }
    if (modl.loopStack.empty()) {
        throw Error(ErrorId::NEXT_WITHOUT_FOR);
    }
    if (!varName.empty() && modl.loopStack.back().varName != varName) {
        throw Error(ErrorId::NEXT_WITHOUT_FOR);
    }


    const auto& loop = modl.loopStack.back();
    Value& v         = modl.variables[loop.varName];
    int64_t loopVar  = valueToInt(v) + loop.step;
    v                = loopVar;

    // debug("next "); debug(loop.varName.c_str()); debug("="); debug(valueToString(v).c_str()); debug("\n");

    if ((loop.step > 0 && loopVar <= loop.end) || (loop.step < 0 && loopVar >= loop.end)) {
        modl.programCounter = loop.jump;
    } else {
        modl.loopStack.pop_back();
    }
}

void Basic::handleIFTHEN(const std::vector<Token>& tokens) {
    size_t endtok = 0;
    auto values   = evaluateExpression(tokens, 1, &endtok);
    if (values.size() != 1) {
        throw Error(ErrorId::SYNTAX);
    }
    if (valueToInt(values[0]) != 0 && endtok > 0) {
        auto copyTok = tokens; // TODO you can do better than this
        copyTok.erase(copyTok.begin(), copyTok.begin() + (endtok));

        // THEN 110
        if (copyTok.size() > 1 && copyTok[1].type == TokenType::INTEGER) {
            copyTok[0].type  = TokenType::KEYWORD;
            copyTok[0].value = "GOTO";
        }

        if (copyTok.begin()->value == "THEN") {
            // THEN PRINT ...
            copyTok.erase(copyTok.begin(), copyTok.begin() + 1);
        }
        executeParsedTokens(copyTok);
    } else { // skip rest of the line CBM BASIC style. Dartmouth would evaluate the next command.
        programCounter().line++;
        programCounter().cmdpos = 0;
    }
}

void Basic::readNextData(Basic::Value* pval, char valuePostfix) {
    auto& cm = currentModule();

    // std::vector<Basic::Token> tokens;
    for (;;) {
        // try {
        //     tokens = tokenizeNextCommand(&cm.readDataPosition);
        // } catch (...) {
        //     // that's a syntax error in a line above the DATA
        //     // throw Error(ErrorId::INTERNAL);
        //     tokens = {};
        // }


        ProgramCounter& pcOfThisData = cm.readDataPosition;
        ensureTokenized(cm, pcOfThisData);
        if (pcOfThisData.cmdpos >= pcOfThisData.line->second.tokens.size()) {
            ++pcOfThisData.line;
            pcOfThisData.cmdpos = 0;
            cm.readDataIndex    = 0;
            if (pcOfThisData.line == cm.listing.end()) {
                throw Error(ErrorId::OUT_OF_DATA);
            }
            continue;
        }

        auto& tokens = pcOfThisData.line->second.tokens[pcOfThisData.cmdpos];

        if (tokens.empty()) {
            // TODO can't be.
            cm.readDataPosition.cmdpos++;
            continue;
        }

        // if (tokens.empty()) {
        //     ++cm.readDataPosition.line;
        //     cm.readDataPosition.cmdpos = 0;
        //     cm.readDataIndex           = 0;
        //     if (cm.readDataPosition.line == cm.listing.end()) {
        //         throw Error(ErrorId::OUT_OF_DATA);
        //     }
        //     continue;
        // } else

        if (tokens[0].value == "DATA") {
            auto pcAfterThisData = cm.readDataPosition; // where to read next DATA
            pcAfterThisData.cmdpos++;

            cm.readDataPosition = pcOfThisData; // rewind to this DATA for more values

            // this is a hack. When the last data is a comma, there is
            // one empty data piece at the end of that line - which is not tokenized!
            // This way, we add an empty item.
            if (tokens.back().type == TokenType::COMMA) {
                Token t {};
                if (valuePostfix == '$') {
                    t.type  = TokenType::STRING;
                    t.value = "";
                } else {
                    t.type  = TokenType::INTEGER;
                    t.value = "0";
                }
                tokens.push_back(t);
            }


            bool previousWasComma = false;
            int nthData           = 0;
            bool unaryMinus       = false;
            for (size_t itok = 1 /* skip "DATA"*/; itok < tokens.size(); ++itok) {
                if (nthData == cm.readDataIndex && tokens[itok].type == TokenType::OPERATOR) {
                    if (tokens[itok].value == "-") {
                        unaryMinus = true;
                        ++itok;
                    }
                }

                auto& t = tokens[itok];
                if (nthData == cm.readDataIndex) {
                    ++cm.readDataIndex;
                    bool skipComma = true;
                    switch (t.type) {
                    case TokenType::STRING:
                    case TokenType::IDENTIFIER:
                        if (valuePostfix != '$') {
                            throw Error(ErrorId::TYPE_MISMATCH);
                        }
                        *pval = std::string(t.value);
                        break;
                    case TokenType::INTEGER:
                        if (valuePostfix == '$') {
                            throw Error(ErrorId::TYPE_MISMATCH);
                        }
                        *pval = unaryMinus ? -strToInt(t.value) : strToInt(t.value);
                        break;
                    case TokenType::NUMBER:
                        if (valuePostfix == '$') {
                            throw Error(ErrorId::TYPE_MISMATCH);
                        }
                        *pval = unaryMinus ? -atof(t.value.data()) : atof(t.value.data());
                        break;
                    case TokenType::COMMA:
                        skipComma = false;
                        // empty data statement
                        if (valuePostfix == '$') {
                            *pval = std::string("");
                        } else {
                            *pval = int64_t(0);
                        }
                        break;
                    default: break;
                    }

                    // skip following comma
                    if (skipComma && itok + 1 < tokens.size() && tokens[itok + 1].type == TokenType::COMMA) {
                        if (unaryMinus) {
                            ++cm.readDataIndex;
                        }
                        ++cm.readDataIndex;
                    }
                    return;
                }
                ++nthData;
            }




            // end of this DATA - read from next DATA
            ASSERT(nthData == cm.readDataIndex);

            cm.readDataPosition = pcAfterThisData;
            cm.readDataIndex    = 0;
        } else {
            ++cm.readDataPosition.cmdpos;
        }
    }


    throw Error(ErrorId::INTERNAL);
}

void Basic::handleREAD(const std::vector<Token>& tokens) {
    for (size_t itk = 1; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (tk.type == TokenType::IDENTIFIER) {
            Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
            if (pval == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }
            readNextData(pval, tk.valuePostfix());

            // std::string dbg = valueToString(programCounter().line->first) + ": " + valueToString(*pval) + "\n";
            // printUtf8String(dbg);
        }
    }
}

void Basic::handleRESTORE(const std::vector<Token>& tokens) {
    auto args = evaluateExpression(tokens, 1);
    auto& cm  = currentModule();
    if (args.empty()) {
        cm.restoreDataPosition();
    } else {
        int line = int(valueToInt(args[0]));
        for (auto it = cm.listing.begin(); it != cm.listing.end(); ++it) {
            if (it->first == line) {
                cm.readDataPosition.line   = it;
                cm.readDataPosition.cmdpos = 0;
                cm.readDataIndex           = 0;
                return;
            }
        }
        throw Error(ErrorId::UNDEFD_STATEMENT);
    }
}

// void Basic::handleDATA(std::vector<Token>& tokens) {}


void Basic::ensureTokenized(Basic::Module& module, Basic::ProgramCounter& pc) {
    if (pc.line != module.listing.end() && pc.line->second.tokens.empty()) {
        tokenizeLine(pc.line->second);
    }
}

// better call execute(ProgramLine&)
void Basic::executeParsedTokens(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        return;
    }

    if (!moduleVariableStack.back()->second.fastMode) {
        static int tokenDelay = 0;
        tokenDelay += int(tokens.size());
        const int delayThreshold = 20;
        if (tokenDelay > delayThreshold) {
            tokenDelay -= delayThreshold;
            os->delay(5);
        }
    }

    auto& modl = currentModule();
    if (tokens[0].type == TokenType::COMMAND) {
        auto cmd    = commands.find(tokens[0].value);
        auto values = evaluateExpression(tokens, 1);
        cmd->second(this, values);
    } else if (tokens[0].value == "LET") {
        /*tokens.erase(tokens.begin());*/ handleLET(tokens);
    } else if (tokens.size() > 2 && tokens[0].type == TokenType::IDENTIFIER) {
        handleLET(tokens);
    } else if (tokens.size() > 3 && tokens[0].type == TokenType::MODULE) {
        handleLET(tokens);
    } else if (tokens[0].value == "DIM") {
        handleDIM(tokens);
    } else if (tokens[0].type == TokenType::KEYWORD) {
        if (tokens[0].value == "GOTO") {
            handleGOTO(tokens);
        } else if (tokens[0].value == "GOSUB") {
            handleGOSUB(tokens);
        } else if (tokens[0].value == "RETURN") {
            ASSERT(moduleListingStack.size() == moduleVariableStack.size());

            // pop inner loops
            while (!modl.loopStack.empty() && modl.loopStack.back().type != LoopItem::GOSUB) {
                modl.loopStack.pop_back();
            }
            if (modl.loopStack.empty()) {
                throw Error(ErrorId::RETURN_WITHOUT_GOSUB);
            }
            programCounter() = modl.loopStack.back().jump;
            modl.loopStack.pop_back();
        } else if (tokens[0].value == "FNEND") {
            ASSERT(moduleListingStack.size() == moduleVariableStack.size());
            modl.setProgramCounterToEnd(); // return to function call
        } else if (tokens[0].value == "FOR") {
            handleFOR(tokens);
        } else if (tokens[0].value == "NEXT") {
            handleNEXT(tokens);
        } else if (tokens[0].value == "IF") {
            handleIFTHEN(tokens);
        } else if (tokens[0].value == "DIM") {
            handleDIM(tokens);
        } else if (tokens[0].value == "RUN") {
            handleRUN(tokens);
        } else if (tokens[0].value == "MODULE") {
            handleMODULE(tokens);
        } else if (tokens[0].value == "PRINT" || tokens[0].value == "?") {
            handlePRINT(tokens);
        } else if (tokens[0].value == "INPUT") {
            handleINPUT(tokens);
        } else if (tokens[0].value == "GET") {
            handleGET(tokens, false);
        } else if (tokens[0].value == "NETGET") {
            handleNETGET(tokens);
        } else if (tokens[0].value == "GETKEY") {
            handleGET(tokens, true);
        } else if (tokens[0].value == "ON") {
            handleONGOTO(tokens);
        } else if (tokens[0].value == "REM") {
        } else if (tokens[0].value == "CLR") {
            handleCLR();
        } else if (tokens[0].value == "SCNCLR") {
            os->screen.clear();
            os->presentScreen();
        } else if (tokens[0].value == "NEW") {
            doNEW();
        } else if (tokens[0].value == "LIST") {
            handleLIST(tokens);
        } else if (tokens[0].value == "READ") {
            handleREAD(tokens);
        } else if (tokens[0].value == "RESTORE") {
            handleRESTORE(tokens);
        } else if (tokens[0].value == "DATA") {
        } else if (tokens[0].value == "KEY") {
            handleKEY(tokens);
        } else if (tokens[0].value == "RCHARDEF") {
            handleRCHARDEF(tokens);
        } else if (tokens[0].value == "DEF") {
            handleDEFFN(tokens);
        } else if (tokens[0].value == "END") {
            doEND();
        } else if (tokens[0].value == "HELP") {
            handleHELP(tokens);
        } else if (tokens[0].value == "DELETE") {
            handleDELETE(tokens);
        } else if (tokens[0].value == "DUMP") {
            handleDUMP(tokens);
        } else {
            throw Error(ErrorId::UNIMPLEMENTED_COMMAND);
        }
    } else {

        if (tokens.size() > 1 && tokens[0].value == "READY") {
            throw Error(ErrorId::READY_COMMAND);
        }

        throw Error(ErrorId::SYNTAX);
    }
}

// void Basic::execute(ProgramLine& line, bool mustTokenize) {
//     if (mustTokenize) {
//         tokenizeLine(line);
//     }
//     for (auto& toks : line.tokens) {
//         executeParsedTokens(toks);
//     }
// }

void Basic::uppercaseProgram(std::string& codeline) {
    char32_t quotes = U'\0';
    std::u32string u32;
    if (Unicode::toU32String(codeline.c_str(), u32)) {
        for (size_t i = 0; i < u32.length(); ++i) {
            char32_t c = u32[i];
            if (c == U'\"' || c == U'\'') {
                if (quotes == '\0') {
                    quotes = c;
                } else if (c == quotes) {
                    quotes = '\0';
                }
            }
            if (quotes == U'\0') {
                u32.at(i) = Unicode::toUpperAscii(c);
            }
        }
        codeline = Unicode::toUtf8String(u32.c_str());
    } else {
        throw Error(ErrorId::SYNTAX);
    }
}





/*
        { 0x9a,  color (light blue)
        { 0x9b,  color (light gray)
        { 0x9f,  color (cyan)
* */
void Basic::printUtf8String(const char* utf8, const char* pend, bool applyCtrlCodes, bool ctrlInQuotes) {
    bool quotes1 = false, quotes2 = false;

    if (pend == nullptr) {
        pend = utf8;
        while (*pend != '\0') {
            ++pend;
        }
    }

    if (currentFileNo == 0) {
        if (applyCtrlCodes) {
            while (utf8 < pend) {
                char32_t c = Unicode::parseNextUtf8(utf8);

                if (!ctrlInQuotes) {
                    if (c == '\"') {
                        quotes1 = !quotes1;
                    }
                    if (c == '\'') {
                        quotes1 = !quotes1;
                    }
                }

                if (!quotes1 && !quotes2) {
                    switch (c) {
                    case ControlCharacters::cursorDown /*0x11*/:              os->screen.moveCursorPos(0, 1); break; // cursor down
                    case ControlCharacters::cursorRight /*0x1d*/:             os->screen.moveCursorPos(1, 0); break; // cursor right
                    case ControlCharacters::cursorUp /*0x91*/:                os->screen.moveCursorPos(0, -1); break; // cursor up
                    case ControlCharacters::cursorLeft /*0x9d*/:              os->screen.moveCursorPos(-1, 0); break; // cursor left
                    case ControlCharacters::cursorHome /*0x13*/:              os->screen.setCursorPos({ 0, 0 }); break; // home
                    case ControlCharacters::backspaceChar /*0x14*/:           os->screen.backspaceChar(); break; // delete
                    case ControlCharacters::clearScreen /*0x93*/:             os->screen.clear(); break; // clear
                    case ControlCharacters::reverseModeOn /*0x12*/:           os->screen.setReverseMode(true); break; // reverse on
                    case ControlCharacters::reverseModeOff /*0x92*/:          os->screen.setReverseMode(false); break; // reverse off
                    case ControlCharacters::textColor0_Black /*0x90*/:        os->screen.setTextColor(0); break; // Black
                    case ControlCharacters::textColor1_White /*0x05*/:        os->screen.setTextColor(1); break; // White
                    case ControlCharacters::textColor2_Red /*0x1c*/:          os->screen.setTextColor(2); break; // Red
                    case ControlCharacters::textColor3_Cyan /*0x9f*/:         os->screen.setTextColor(3); break; // Cyan
                    case ControlCharacters::textColor4_Purple /*0x9c*/:       os->screen.setTextColor(4); break; // Purple
                    case ControlCharacters::textColor5_Green /*0x1e*/:        os->screen.setTextColor(5); break; // Green
                    case ControlCharacters::textColor6_Blue /*0x1f*/:         os->screen.setTextColor(6); break; // Blue
                    case ControlCharacters::textColor7_Yellow /*0x9e*/:       os->screen.setTextColor(7); break; // Yellow
                    case ControlCharacters::textColor8_Orange /*0x81*/:       os->screen.setTextColor(8); break; // Orange
                    case ControlCharacters::textColor9_Brown /*0x95*/:        os->screen.setTextColor(9); break; // Brown
                    case ControlCharacters::textColor10_Light_Red /*0x96*/:   os->screen.setTextColor(10); break; // Light Red
                    case ControlCharacters::textColor11_Dark_Gray /*0x97*/:   os->screen.setTextColor(11); break; // Dark Gray
                    case ControlCharacters::textColor12_Medium_Gray /*0x98*/: os->screen.setTextColor(12); break; // Medium Gray
                    case ControlCharacters::textColor13_Light_Green /*0x99*/: os->screen.setTextColor(13); break; // Light Green
                    case ControlCharacters::textColor14_Light_Blue /*0x9a*/:  os->screen.setTextColor(14); break; // Light Blue
                    case ControlCharacters::textColor15_Light_Gray /*0x9b*/:  os->screen.setTextColor(15); break; // Light Gray
                    default:
                        os->screen.putC(c);
                    }
                } else {
                    os->screen.putC(c);
                }
            }
        } else {
            while (utf8 < pend) {
                os->screen.putC(Unicode::parseNextUtf8(utf8));
            }
        }

        os->presentScreen();
    } else {
        auto& pf = fileHandles[currentFileNo];
        if (!pf) {
            currentFileNo = 0;
            throw Error(ErrorId::FILE_NOT_OPEN);
        } else {
            pf.write(utf8, pend - utf8);
            // pf.printf("%s", utf8);
            pf.flush();
        }
    }
}

std::string Basic::inputLine(bool allowVertical) {
    if (os->settings.demoMode) {
        os->delay(2000);
    }


    bool movedVertical = false;

    // == simulate typing a string (F-key macros)
    auto typeString = [&](const std::string& s) {
        ProgramLine prgline;
        prgline.code = s;
        tokenizeLine(prgline);
        // auto toks = tokenizeNextCommand(s);
        if (prgline.tokens.empty()) {
            return;
        }
        for (auto& tok : prgline.tokens) {
            auto vals = evaluateExpression(tok, 0);
            if (vals.empty()) {
                return;
            }
            for (auto& c : valueToString(vals[0])) {
                os->putToKeyboardBuffer(c);
            }
        }
    };

    auto startCrsr = os->screen.getCursorPos();
    if (allowVertical) {
        // anything in the programmed line matters as input
        // the AUTO command will put the x position after the line number
        startCrsr.x = 0;
    }

    bool isSelecting          = false;
    auto cursorAtStartOfInput = startCrsr;
    auto startSelection       = startCrsr;
    size_t oldScrollCount     = os->screen.scrollCount;

    os->screen.setCursorActive(true);
    for (;;) {
        os->updateEvents();
        os->presentScreen();
        handleEscapeKey();

        auto key = os->getFromKeyboardBuffer();
        if (key.code < 0x20 // windows sends $09 (ESC) and $10 when I press Ctrl+C, Ctrl+V
            && key.code != U'\b' && key.code != U'\n' && key.code != U'\r') {
            continue;
        }

        // key.debug();

        // if shift is pressed and cursor gets moved, a selection is made
        auto startSel = [&]() {
            if (key.holdShift) {
                isSelecting = true;
            } else {
                isSelecting    = false;
                startSelection = os->screen.getCursorPos();
            }
        };

        if (!key.printable) {
            if (key.holdCtrl) {
                char32_t ctrlChar = 0;
                switch (key.code) {
                case U'c':
                case U'C': {
                    auto str32            = os->screen.getSelectedText(startSelection, os->screen.getCursorPos());
                    std::string clipboard = Unicode::toUtf8String(str32.c_str());
                    StringHelper::trimRight(clipboard, " \r\n\t");

                    if (clipboard.length() > 0) {
                        std::cout << "copy to clipboard: " << clipboard << std::endl;
                        os->setClipboardData(clipboard);
                    }
                    break;
                }
                case U'v':
                case U'V': {
                    std::string clipboard = os->getClipboardData();
                    std::cout << "pasted from clipboard: " << clipboard << std::endl;
                    if (clipboard.length() != 0) {
                        std::string cliptext = clipboard;

                        // remove \r, escape in string
                        StringHelper::replace(cliptext, "\r\n", "\n");
                        StringHelper::replace(cliptext, "\r", "\n");

                        Os::KeyPress kp  = {};
                        kp.printable     = true;
                        const char* utf8 = cliptext.c_str();
                        for (;;) {
                            char32_t c = Unicode::parseNextUtf8(utf8);
                            if (c == 0) {
                                break;
                            }

                            kp.code = c;
                            os->putToKeyboardBuffer(kp, false); // can copy/paste listings etc.
                        }
                    }
                    break;
                }
                case U'.':
                case 190:  {
                    auto emoji = os->emojiPicker();
                    for (auto e : emoji) {
                        os->screen.putC(e);
                    }
                    break;
                }
                case u'1':
                    if (key.holdShift) {
                        ctrlChar = 0x97; // dark gray
                    } else {
                        ctrlChar = 0x90; // black
                    }
                    break;
                case u'2':
                    if (key.holdShift) {
                        ctrlChar = 0x9b; // light gray
                    } else {
                        ctrlChar = 0x05; // white
                    }
                    break; // gray/ white
                case u'3':
                    if (key.holdShift) {
                        ctrlChar = 0x96; // pink/light red
                    } else {
                        ctrlChar = 0x1c; // red
                    }
                    break;
                case u'4':
                    if (key.holdShift) {
                        ctrlChar = 0x98; // gray
                    } else {
                        ctrlChar = 0x9f; // cyan
                    }
                    break;
                case u'5':
                    if (key.holdShift) {
                        ctrlChar = 0x81; // orange
                    } else {
                        ctrlChar = 0x9c; // purple
                    }
                    break;
                case u'6':
                    if (key.holdShift) {
                        ctrlChar = 0x99; // light green
                    } else {
                        ctrlChar = 0x1e; // green
                    }
                    break;
                case u'7':
                    if (key.holdShift) {
                        ctrlChar = 0x9a; // light blue
                    } else {
                        ctrlChar = 0x1f; // blue
                    }
                    break;
                case u'8':
                    if (key.holdShift) {
                        ctrlChar = 0x95; // brown
                    } else {
                        ctrlChar = 0x9e; // yellow
                    }
                    break;
                case u'9':
                    if (key.holdShift) {
                        // ctrlChar = 0x95; // brown
                    } else {
                        ctrlChar = 0x12; // reverse on
                    }
                    break;
                case u'0':
                    if (key.holdShift) {
                        // ctrlChar = 0x95; // brown
                    } else {
                        ctrlChar = 0x92; // reverse off
                    }
                    break;
                }

                if (ctrlChar == 0) {
                    continue;
                } else {
                    key.code      = ctrlChar;
                    key.printable = true;
                }
            } else { // not ctrl

                if (key.holdAlt) {
                    switch (key.code) {
                    case uint32_t(Os::KeyConstant::INSERT):
                        // ctrlChar = 0x94;
                        insertMode           = !insertMode;
                        os->screen.dirtyFlag = true;
                        key.code             = 0;
                        break;

                    case uint32_t(Os::KeyConstant::BACKSPACE):  key.code = 0x14; break;
                    case uint32_t(Os::KeyConstant::DEL):        key.code = 0x14; break;
                    case uint32_t(Os::KeyConstant::CRSR_LEFT):  key.code = 0x9d; break;
                    case uint32_t(Os::KeyConstant::CRSR_RIGHT): key.code = 0x1d; break;
                    case uint32_t(Os::KeyConstant::CRSR_UP):    key.code = 0x91; break;
                    case uint32_t(Os::KeyConstant::CRSR_DOWN):  key.code = 0x11; break;
                    case uint32_t(Os::KeyConstant::HOME):       key.code = 0x13; break;
                    case uint32_t(Os::KeyConstant::END):        key.code = 0x93; break; // clear
                    default:
                        // Alt and Shift+Alt produce the PETSCII characters as on the C128
                        key.code = PETSCII::unicodeFromAltKeyPress(char(key.code), key.holdShift);
                    }

                    if (key.code == 0) {
                        continue;
                    } else {
                        key.printable = true;
                    }
                } else { // not alt

                    switch (key.code) {
                    case uint32_t(Os::KeyConstant::F1):
                        typeString(keyShortcuts[0]);
                        continue;
                    case uint32_t(Os::KeyConstant::F2):
                        typeString(keyShortcuts[1]);
                        continue;
                    case uint32_t(Os::KeyConstant::F3):
                        typeString(keyShortcuts[2]);
                        continue;
                    case uint32_t(Os::KeyConstant::F4):
                        typeString(keyShortcuts[3]);
                        continue;
                    case uint32_t(Os::KeyConstant::F5):
                        typeString(keyShortcuts[4]);
                        continue;
                    case uint32_t(Os::KeyConstant::F6):
                        typeString(keyShortcuts[5]);
                        continue;
                    case uint32_t(Os::KeyConstant::F7):
                        typeString(keyShortcuts[6]);
                        continue;
                    case uint32_t(Os::KeyConstant::F8):
                        typeString(keyShortcuts[7]);
                        continue;
                    case uint32_t(Os::KeyConstant::F9):
                        typeString(keyShortcuts[8]);
                        continue;
                    case uint32_t(Os::KeyConstant::F10):
                        typeString(keyShortcuts[9]);
                        continue;
                    case uint32_t(Os::KeyConstant::F11):
                        typeString(keyShortcuts[10]);
                        continue;
                    case uint32_t(Os::KeyConstant::F12):
                        typeString(keyShortcuts[11]);
                        continue;
                    case uint32_t(Os::KeyConstant::BACKSPACE):
                        os->screen.backspaceChar();
                        continue;
                    case uint32_t(Os::KeyConstant::DEL):
                        os->screen.deleteChar();
                        continue;
                    case uint32_t(Os::KeyConstant::INSERT):
                        if (key.holdAlt) {
                            insertMode           = !insertMode;
                            os->screen.dirtyFlag = true;
                        } else {
                            os->screen.insertSpace();
                        }
                        continue;
                    case uint32_t(Os::KeyConstant::CRSR_LEFT):
                        os->screen.moveCursorPos(-1, 0);
                        startSel();
                        continue;
                    case uint32_t(Os::KeyConstant::CRSR_RIGHT):
                        os->screen.moveCursorPos(1, 0);
                        startSel();
                        continue;
                    case uint32_t(Os::KeyConstant::CRSR_UP):
                        os->screen.moveCursorPos(0, -1);
                        startSel();
                        movedVertical = true;
                        continue;
                    case uint32_t(Os::KeyConstant::CRSR_DOWN):
                        os->screen.moveCursorPos(0, 1);
                        startSel();
                        movedVertical = true;
                        continue;
                    case uint32_t(Os::KeyConstant::PG_UP):
                        for (size_t i = 1; i < os->screen.height; ++i) {
                            os->screen.moveCursorPos(0, -1);
                            os->screen.scrollDownOne();
                        }
                        startSel();
                        movedVertical = true;
                        continue;
                    case uint32_t(Os::KeyConstant::PG_DOWN):
                        for (size_t i = 1; i < os->screen.height; ++i) {
                            os->screen.moveCursorPos(0, 1);
                            os->screen.scrollUpOne();
                        }
                        startSel();
                        movedVertical = true;
                        continue;
                    case uint32_t(Os::KeyConstant::HOME):
                        os->screen.setCursorPos({ 0, os->screen.getCursorPos().y });
                        startSel();
                        continue;
                    case uint32_t(Os::KeyConstant::END): {
                        auto crsr = os->screen.getEndOfLineAt(os->screen.getCursorPos()); // that's the '\n' character
                        os->screen.setCursorPos(crsr);
                        os->screen.moveCursorPos(1, 0);
                    }
                        startSel();
                        continue;
                    case uint32_t(Os::KeyConstant::PAUSE): {
                        std::string statePath(os->getHomeDirectory() + "/quicksave.state67");
                        if (key.holdShift) {
                            loadState(statePath);
                        } else {
                            saveState(statePath);
                        }
                    }
                        continue;
                    }
                }
            }
        } // not printable

        uint32_t ch = key.code;
        if (ch == '\r' || ch == '\n') {
            break;
        }

        if (key.printable) {
            if (insertMode) {
                os->screen.insertSpace();
            }
            os->screen.putC(ch);
        }
    }
    os->screen.setCursorActive(false);


    if (oldScrollCount < os->screen.scrollCount) {
        cursorAtStartOfInput.y -= (os->screen.scrollCount - oldScrollCount);
    }


    auto crsr = os->screen.getCursorPos();
    ScreenBuffer::Cursor istart {}, iend {};

    std::u32string screenchars;
    if (cursorAtStartOfInput.y != crsr.y) {
        // Moved a line up
        startCrsr   = crsr;
        startCrsr.x = 0;
        istart      = os->screen.getStartOfLineAt(startCrsr);
        iend        = os->screen.getEndOfLineAt(crsr); // that's the '\n' character

        screenchars = os->screen.getSelectedText(istart, iend);

        crsr   = iend;
        crsr.x = 0;
        crsr.y++;
        if (crsr.y >= os->screen.height) {
            os->screen.scrollUpOne();
            --crsr.y;
        }
        os->screen.setCursorPos(crsr);
    } else {
        istart = (startCrsr);
        iend   = os->screen.getEndOfLineAt(startCrsr);
        if (iend < istart) {
            iend = istart;
        }
        screenchars = os->screen.getSelectedText(istart, iend);
        printUtf8String("\n");
    }
    if (iend < istart) {
        iend = istart;
    }

    std::string str;
    for (auto ch : screenchars) {
        Unicode::appendAsUtf8(str, ch);
    }

    if (os->settings.demoMode) {
        os->delay(1000);
    }

    return str;
}


void Basic::restoreColorsAndCursor(bool resetFont) {
    if (resetFont) {
        os->screen.resetDefaultColors();
        os->screen.resetCharmap();
        while (moduleListingStack.size() > 1) {
            moduleListingStack.pop_back();
        }
        while (moduleVariableStack.size() > 1) {
            moduleVariableStack.pop_back();
        }
    }

    os->screen.setBackgroundColor(11);
    os->screen.setTextColor(colorForModule(moduleVariableStack.back()->first));
    os->screen.setReverseMode(false);
    insertMode = false;

    for (auto& s : os->screen.sprites) {
        s.enabled = false;
    }

    if (os->screen.getCursorPos().x != 0) {
        printUtf8String("\n");
    }
}

Basic::ParseStatus Basic::parseInput(const char* pline) {
    const char* pc            = pline;
    ParseStatus executeStatus = ParseStatus::PS_EXECUTED;

    try {
        int64_t n = 0;
        if (parseInt(pc, &n)) {
            // program a line
            // debug("program a line\n");
            if (n < 0) {
                throw Error(ErrorId::SYNTAX);
            }

            auto& cm = currentModule();

            pc = skipWhite(pc);
            if (*pc == '\0') {
                auto itr = cm.listing.find(int(n));
                if (itr != cm.listing.end()) {
                    cm.listing.erase(itr);
                }
            } else {
                ProgramLine& prgline = cm.listing[int32_t(n)];

                prgline.code = pc;
                StringHelper::trimRight(prgline.code, " \r\n\t");

                tokenizeLine(prgline); // check sanity, but don't execute

                cm.lastEnteredLineNumber = int32_t(n);

                cm.programCounter.line   = cm.listing.find(int32_t(n));
                cm.programCounter.cmdpos = 0;

                // while (!tokenizeNextCommand().empty()) { } // check sanity, but don't execute
            }
            cm.setProgramCounterToEnd();
            return ParseStatus::PS_PROGRAMMED;
        } else {
            // Immediate mode command
            // debug("program line -1\n");
            auto& cm = currentModule();

            ProgramLine& prgline = cm.listing[-2];
            prgline.code         = pc;
            tokenizeLine(prgline); // check sanity, but don't execute

            ProgramLine& remline = cm.listing[-1];
            if (remline.code.empty()) {
                remline.code = "REM LINE -1 ENDS THE IMMEDIATE MODE EXECUTION";
                tokenizeLine(remline);
            }

            if (*pc == '\0') {
                executeStatus = ParseStatus::PS_IDLE;
            }

            if (moduleListingStack.size() < moduleVariableStack.size()) {
                // in immediate mode, the code stack must equal the variable stack
                moduleListingStack.push_back(moduleVariableStack.back());
            }
            programCounter().line   = currentListing().begin();
            programCounter().cmdpos = 0;
        }

        // execute listing code
        runToEnd();
    } catch (const Error& e) {
        currentFileNo = 0;
        // dumpVariables();
        storeProgramCounterForCont();
        auto& pc  = programCounter();
        int iline = 0;
        if (pc.line != currentListing().end() && pc.line->first != -1) {
            // std::string& line = pc.line->second;
            iline = pc.line->first;
        }
        if (iline >= 0) {
            restoreColorsAndCursor(true);
            std::string msg = "?" + std::string(e.what()) + " IN " + valueToString(iline) + "\n";
            // msg += std::string(os->screen.width - msg.length() - 1, ' ') ;
            os->screen.cleanCurrentLine();
            printUtf8String(msg);

            if (e.ID != ErrorId::BREAK && iline >= 0) {
                parseInput((std::string("LIST ") + valueToString(iline) + " - " + valueToString(iline)).c_str());
            }
        } else {
            os->screen.cleanCurrentLine();
            printUtf8String("?" + std::string(e.what()) + " \n");
        }
        return ParseStatus::PS_ERROR;
    }

    return executeStatus;
}

void Basic::executeCommands(const char* pline) {
    ProgramLine prgline;
    prgline.code = pline;
    tokenizeLine(prgline);
    for (auto& tok : prgline.tokens) {
        executeParsedTokens(tok);
    }
}

void Basic::handleEscapeKey(bool allowPauseWithShift) {
    // break with escape
    if (os->isKeyPressed(Os::KeyConstant::ESCAPE)) {
        restoreColorsAndCursor(true);
        throw Error(ErrorId::BREAK);
    }
    if (os->isKeyPressed(Os::KeyConstant::PAUSE) && os->isKeyPressed(Os::KeyConstant::ALT_LEFT)) {


        monitor();
    }


    // pause with scroll lock
    if (!allowPauseWithShift) {
        return;
    }
    while (
        os->isKeyPressed(Os::KeyConstant::SCROLL)
        || os->isKeyPressed(Os::KeyConstant::SHIFT_LEFT)
        || os->isKeyPressed(Os::KeyConstant::SHIFT_RIGHT)) {
        if (os->isKeyPressed(Os::KeyConstant::ESCAPE)) {
            throw Error(ErrorId::BREAK);
        }
        os->delay(30);
        os->updateEvents();
        os->presentScreen();
    }
}

// Basic interpreter loop
void Basic::runInterpreter() {
    currentModule().loopStack.clear();

    std::string line;
    ParseStatus status = ParseStatus::PS_EXECUTED;
    while (true) {
        if (status == ParseStatus::PS_PROGRAMMED) {
            auto& cm = currentModule();
            if (cm.autoNumbering > 0) {
                int64_t number = int64_t(cm.autoNumbering) + cm.lastEnteredLineNumber;

                std::string str = valueToString(number) + " ";
                if (cm.listing.find(int(number)) == cm.listing.end()) {
                    printUtf8String(str);
                } else {
                    parseInput((std::string("LIST ") + str + " - " + str).c_str());
                    os->screen.moveCursorPos(0, -1);
                }
            }
        } else {
            if (status != ParseStatus::PS_IDLE) {
                restoreColorsAndCursor(false);
                printUtf8String("\nREADY." + std::string(os->screen.width - 7, ' ') + "\n");
                printUtf8String(std::string(os->screen.width, ' ') + "\n");
                auto pos = os->screen.getCursorPos();
                size_t y = os->screen.getCursorPos().y;
                os->screen.setCursorPos({ 0, y - 1 });
            }

            // wait for ESC release
            while (os->isKeyPressed(Os::KeyConstant::ESCAPE)) {
                os->updateEvents();
                os->delay(100);
                // clear keyboard buffer
                while (os->keyboardBufferHasData()) {
                    os->getFromKeyboardBuffer();
                }
            }
        }
        // debug("MODULE VARS "); debug(moduleVariableStack.back()->first.c_str()); debug("\n");
        // debug("MODULE CODE "); debug(moduleListingStack.back()->first.c_str()); debug("\n");

        try {
            line = inputLine(true);
        } catch (const Error& e) {
            (void)e;
            currentFileNo = 0;
            status        = ParseStatus::PS_ERROR;
            continue;
        }
        uppercaseProgram(line);

        status = parseInput(line.c_str());
    }
}

// run from current program counter until END is hit
void Basic::runToEnd() {
    bool programWasRunning = false; // still in immediate mode
    int lastTraceLine      = -1;
    for (;;) {
        if (programCounter().line == currentListing().end()) {
            break;
        }

        if (moduleListingStack.back()->second.traceOn && programCounter().line->first >= 0) {
            int line = programCounter().line->first;
            if (lastTraceLine != line) {
                lastTraceLine = line;
                os->screen.cleanCurrentLine();
                printUtf8String("[" + valueToString(line) + "]");
            }
        }

        // debug("MODULE VARS "); debug(moduleVariableStack.back()->first.c_str()); debug("\n");
        // debug("MODULE CODE "); debug(moduleListingStack.back()->first.c_str()); debug("\n");

#if 0
            std::cout << "                                     LINE ";
            std::cout << valueToString(int64_t(programCounter().line->first)).c_str();
            std::cout << "\n";
#endif
        auto& pc = programCounter();

        // might need to tokenize when first encountering a loaded program line.
        ensureTokenized(moduleListingStack.back()->second, pc);
        // if (pc.line != currentListing().end() && pc.line->second.tokens.empty()) {
        //     tokenizeLine(pc.line->second);
        // }

        if (pc.cmdpos >= pc.line->second.tokens.size()) { // next line
            ++pc.line;
            pc.cmdpos = 0;
            // end of program or immediate mode
            if (pc.line == currentListing().end() || pc.line->first == -1) {
                if (programWasRunning) {
                    EndAndPopModule(); // pop module, continue with calling module
                    continue;
                }
                break; // end immediate
            }
            programWasRunning = true; // now, RUN or GOTO must have been called. We're running
            continue;
        }

        auto& tokens = pc.line->second.tokens[pc.cmdpos];

        ++pc.cmdpos; // next command

        executeParsedTokens(tokens);

        os->updateEvents();
        os->presentScreen();
        handleEscapeKey();
    }
}

void Basic::waitForKeypress() {
    for (;;) {
        this->handleEscapeKey();
        if (os->keyboardBufferHasData()) {
            break;
        }
        os->delay(100);
        os->updateEvents();
        os->presentScreen();
    }
}

bool Basic::loadProgram(const char* filenameUtf8) {
    std::string path(filenameUtf8);
    return loadProgram(path);
}


#include "font.h"
bool Basic::loadProgram(std::string& inOutFilenameUtf8) {
    std::string foundname = os->findFirstFileNameWildcard(inOutFilenameUtf8);

    if (foundname != inOutFilenameUtf8) {
        os->screen.cleanCurrentLine();
        printUtf8String("FOUND " + foundname + " \n");
        inOutFilenameUtf8 = foundname;
    }

    doNEW();
    std::vector<char> buff;

    std::string fileExt;
    if (inOutFilenameUtf8.length() > 4) {
        fileExt = Unicode::toLowerAscii(inOutFilenameUtf8.substr(inOutFilenameUtf8.length() - 4).c_str());
    }
    if (os->dirIsInD64() || (fileExt == ".prg")) {
        // load PRG
        FilePtr fprg(os);
        if (!fprg.open(foundname, "rb")) {
            return false;
        }
        auto bytes        = fprg.readAll();
        std::string basic = PrgTool::PRGtoBASIC(&bytes[0]);
        fprg.close();
        buff.resize(basic.length() + 1);
        StringHelper::memcpy(&buff[0], basic.c_str(), basic.length());
        buff[basic.length()] = '\0';

        // no need. Will ask before overwriting .prg
        // inOutFilenameUtf8 = inOutFilenameUtf8.substr(0, inOutFilenameUtf8.length() - 4) + ".bas";
    } else {
        // load BAS
        FilePtr file(os);
        file.open(foundname, "rb");
        if (!file) {
            return false;
        }
        file.seek(0, SEEK_END);
        size_t length = file.tell();

        // strip optional utf8 BOM
        uint8_t bom[4] = { 0, 0, 0, 0 };
        file.seek(0, SEEK_SET);
        if (length > 2) {
            auto nr = file.read(bom, 3);
        }
        if (bom[0] == 0xef && bom[1] == 0xbb && bom[2] == 0xbf) {
            file.seek(3, SEEK_SET);
            length -= 3;
        } else {
            file.seek(0, SEEK_SET);
        }

        buff.resize(length + 1);

        file.read(&buff[0], length);
        buff[length] = '\0';
        file.close();
    }

    // Establish string and get the first token:
    char* next_token = nullptr;
    char* line       = StringHelper::strtok_r(&buff[0], "\r\n", &next_token);

    std::string str;
    auto& listmodule  = moduleListingStack.back()->second;
    int iLastLine     = -1;
    int iline         = 0;
    bool rv           = true;
    bool escapePetcat = false;
    bool oldSpaceReq  = options.spacingRequired;
    while (line != NULL) {
        ++iline;
        str = line;
        uppercaseProgram(str);

        const char* pc = str.c_str();
        int64_t n      = 0;
        if (parseInt(pc, &n) && n >= 0) {
            iline = int(n);
            if (iline <= iLastLine) {
                os->screen.cleanCurrentLine();
                printUtf8String("PROGRAM NOT SORTED! LINE: " + valueToString(iline) + "\n");
            }

            const char* programLineCode = skipWhite(pc);

            if (iline == 1 && StringHelper::strncmp(programLineCode, "REMBA67", 7) == 0) {
                options.spacingRequired = false;
                options.dotAsZero       = true;

                if (StringHelper::strstr(programLineCode, "PETCAT") != nullptr) {
                    escapePetcat = true;
                }
            }
            ProgramLine& prgline = listmodule.listing[int(n)];
            prgline.code         = programLineCode;


            // TODO add spaces from old programs that have no spaces
            // if (!options.spacingRequired) {
            //     try {
            //         std::string reformated;
            //         auto commands = tokenizeNextCommand(programline);
            //         for (size_t icmd = 0; icmd < commands.size(); ++icmd) {
            //             if (icmd > 0) {
            //                 reformated += ": ";
            //             }
            //
            //             auto& cmdTokens = commands[icmd];
            //             if (cmdTokens.empty()) {
            //                 continue;
            //             }
            //             bool quote = true;
            //             if (cmdTokens.front().value == "REM") {
            //                 quote = false;
            //             }
            //             auto needsSpace = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); };
            //             for (auto& t : cmdTokens) {
            //
            //                 if (reformated.empty() || needsSpace(reformated.back())) {
            //                     reformated += " ";
            //                 }
            //
            //                 if (t.type == TokenType::STRING && quote) {
            //                     if (escapePetcat) {
            //                         std::string esc;
            //                         std::string strvalue = std::string(t.value); // null-terminate a copy
            //                         const char* str      = strvalue.c_str();
            //                         while (*str != '\0') {
            //                             char32_t cp = Font::parseNextPetcat(str);
            //                             Unicode::appendAsUtf8(esc, cp);
            //                         }
            //                         t.value = esc;
            //                     }
            //
            //                     reformated += "\"" + std::string(t.value) + "\"";
            //                 } else if (t.type == TokenType::NUMBER && t.value == ".") {
            //                     reformated += "0";
            //                 } else {
            //                     reformated += t.value;
            //                 }
            //             }
            //         }
            //         listmodule.listing[int(n)] = reformated;
            //     } catch (...) {
            //         int what_is_wrong = 1;
            //     }
            // }

            // while (!tokenizeNextCommand().empty()) {} // check sanity, but don't execute
        } else { // no line number
            listmodule.listing[iline].code = skipWhite(pc);
        }
        line = StringHelper::strtok_r(nullptr, "\r\n", &next_token);
    }
    listmodule.setProgramCounterToEnd();



    buff.clear();
    options.spacingRequired = oldSpaceReq;

    return rv;
}

bool Basic::saveProgram(std::string filenameUtf8) {

    std::string password;
    size_t posLock = filenameUtf8.find(",P");
    if (posLock != std::string::npos) {
        password     = filenameUtf8.substr(posLock + 2);
        filenameUtf8 = filenameUtf8.substr(0, posLock);
    }

    FilePtr file(os);
    file.setPassword(password);
    file.open(filenameUtf8, "wb");
    if (!file) {
        return false;
    }

    std::string fileExt;
    if (filenameUtf8.length() > 4) {
        fileExt = Unicode::toLowerAscii(filenameUtf8.substr(filenameUtf8.length() - 4).c_str());
    }
    if (os->dirIsInD64() || (fileExt == ".prg")) {
        std::string all;
        for (auto& ln : currentModule().listing) {
            if (ln.first < 0) {
                continue;
            }
            all += std::to_string(ln.first) + " " + ln.second.code + "\n";
        }

        std::vector<std::pair<int, std::string>> errorDetails;
        auto prg = PrgTool::BASICtoPRG(all.c_str(), &errorDetails);
        file.write(&prg[0], prg.size());

        if (!errorDetails.empty()) {
            std::string sline;
            for (auto& ln : errorDetails) {
                sline = std::to_string(ln.first);
                for (size_t s = sline.length(); s < 3; ++s) {
                    sline += ' ';
                }
                sline += ' ';
                sline += ln.second;
                sline += '\n';
                os->screen.cleanCurrentLine();
                printUtf8String(sline);

                handleEscapeKey(true);
                os->delay(50);
            }
        }

    } else {

        for (auto& ln : currentModule().listing) {
            if (ln.first < 0) {
                continue;
            }
            file.printf("%d %s\r\n", ln.first, ln.second.code.c_str());
        }
    }

    if (!file.close()) {
        os->screen.cleanCurrentLine();
        printUtf8String(file.status());
    }


    return true;
}

bool Basic::fileExists(const std::string& filenameUtf8, bool allowWildCard) {
    if (allowWildCard) {
        return os->doesFileExist(os->findFirstFileNameWildcard(filenameUtf8));
    }
    return os->doesFileExist(filenameUtf8);
}

bool Basic::saveState(std::string& filenameUtf8) {
    RawConfig cfg(os);
    cfg.set("options.spacingRequired", options.spacingRequired);
    cfg.set("options.dotAsZero", options.dotAsZero);
    cfg.set("options.uppercaseInput", options.uppercaseInput);
    cfg.set("options.colorizeListing", options.colorzizeListing);
    cfg.set("memory", &memory[0], memory.size(), sizeof(memory[0]));
    cfg.set("time0", time0);

    auto& mod = currentModule();
    cfg.set("module.fastmode", mod.fastMode);
    cfg.set("module.traceOn", mod.traceOn);
    cfg.set("module.autoNumbering", mod.autoNumbering);
    cfg.set("module.lastEnteredLineNumber", mod.lastEnteredLineNumber);
    cfg.set("module.filenameQSAVE", mod.filenameQSAVE);
    int32_t nlines = int32_t(mod.listing.size());
    cfg.set("module.linecount", nlines);
    int64_t i = 0;
    for (auto& ln : mod.listing) {
        cfg.set("lst.ln" + std::to_string(i), ln.first);
        cfg.set("lst.tx" + std::to_string(i), ln.second);
        ++i;
    }
    cfg.save(filenameUtf8.c_str());
    return false;
}

bool Basic::loadState(std::string& filenameUtf8) {
    RawConfig cfg(os);
    if (!cfg.load(filenameUtf8.c_str())) {
        return false;
    }
    cfg.get("options.spacingRequired", options.spacingRequired);
    cfg.get("options.dotAsZero", options.dotAsZero);
    cfg.get("options.uppercaseInput", options.uppercaseInput);
    cfg.get("memory", &memory[0], memory.size(), sizeof(memory[0]));
    cfg.get("time0", time0);

    auto& mod = currentModule();
    cfg.get("module.fastmode", mod.fastMode);
    cfg.get("module.traceOn", mod.traceOn);
    cfg.get("module.autoNumbering", mod.autoNumbering);
    cfg.get("module.lastEnteredLineNumber", mod.lastEnteredLineNumber);
    cfg.get("module.filenameQSAVE", mod.filenameQSAVE);

    mod.restoreDataPosition();
    mod.setProgramCounterToEnd();
    mod.listing.clear();

    int32_t nlines = 0;
    cfg.get("module.linecount", nlines);

    for (int32_t i = 0; i < nlines; ++i) {
        ProgramLine pl;
        int32_t lineNo = 0;
        cfg.get("lst.ln" + std::to_string(i), lineNo);
        cfg.get("lst.tx" + std::to_string(i), pl.code);
        mod.listing[lineNo] = pl;
    }
    mod.forceTokenizing();
    os->screen.dirtyFlag = true;
    return true;
}

Basic::Module::VariableMap::iterator Basic::Module::findOrCreateVariable(const std::string_view& variableName) {
    auto varit = variables.find(variableName);
    if (varit == variables.end()) {
        Token tok;
        tok.type  = TokenType::IDENTIFIER;
        tok.value = variableName;

        // create new variable
        switch (tok.valuePostfix()) {
        case '%':
            variables[std::string(tok.value)] = 0LL;
            break;
        case '$':
            variables[std::string(tok.value)] = "";
            break;
        default:
        case '#':
            variables[std::string(tok.value)] = 0.0;
            break;
        }
        varit = variables.find(variableName);
    }
    return varit;
}


bool Basic::AreYouSureQuestion() {
    printUtf8String("ARE YOU SURE (Y/N)?");
    std::string yesno = inputLine(false);
    if (yesno.length() > 0 && Unicode::toUpperAscii(yesno[0]) == u'Y') {
        return true;
    }
    return false;
}

// machine memory monitor
void Basic::monitor() {
    static bool inMonitor = false;

    if (inMonitor) {
        return;
    }

    cpu.breakPointHit = false;


    inMonitor      = true;
    auto& scn      = os->screen;
    auto oldScreen = scn.saveState();

    scn.setBackgroundColor(11);
    scn.setTextColor(13);
    scn.setReverseMode(false);
    bool assemblerMode  = false;
    uint16_t assembleAt = 0;
    for (;;) {
        os->updateEvents();
        os->presentScreen();
        // prompt
        scn.setCursorPos({ 0, scn.height - 1 });
        printUtf8String("\n");

        scn.cleanCurrentLine();
        if (assemblerMode) {
            printUtf8String("." + StringHelper::int2hex(assembleAt, true, 2) + "  ");
        } else {
            printUtf8String("$" + StringHelper::int2hex(cpu.PC, true, 2) + " ");
        }

        std::string cmd;
        try {
            cmd = inputLine(false);
        } catch (...) {
            break;
        }


        printUtf8String("\n");
        scn.cleanCurrentLine();
        auto args = StringHelper::split(cmd, " \t");

        // ==
        auto argi = [&args](size_t i) -> int {
            if (args.size() <= i) {
                return 0;
            }
            const char* str = args[i].c_str();
            if (*str == '$') {
                ++str;
            }
            char* endInt = nullptr;
            i            = int(strtoll(str, &endInt, 16));
            if (endInt != str) {
                return int(i);
            }
            return 0;
        };


        // ==
        auto disassemble = [&](uint16_t addr) -> uint16_t {
            auto info = CPU6502::getOpcodeInfo(cpu.memory[addr]);
            scn.cleanCurrentLine();
            printUtf8String("." + StringHelper::int2hex(addr, true, 2) + "  " + cpu.disassemble(addr) + "\n");
            addr += info.length;
            return addr;
        };

        // assembler
        if (assemblerMode) {
            if (args.empty()) {
                assemblerMode = false;
                continue;
            }
            Unicode::toUpper(cmd);
            uint16_t newAddr = cpu.assemble(cmd.c_str(), assembleAt);
            if (newAddr == 0) {
                printUtf8String("ERROR\n");
            } else {
                disassemble(assembleAt);
                assembleAt = newAddr;
            }
            continue;
        }

        // monitor
        if (args.empty()) {
            continue;
        }
        Unicode::toLower(args[0]);


        if (args[0] == "h" || args[0] == "help") {
            //              "0123456789012345678901234567890123456789"
            printUtf8String("g [address]     - continue execution\n");
            printUtf8String("x               - exit monitor\n");
            printUtf8String("z               - single step\n");
            printUtf8String("m [from] [to]   - memory display\n");
            printUtf8String("d [from] [to]   - disassemble\n");
            printUtf8String("r               - registers\n");
            printUtf8String("> address xx    - poke bytes into memory\n");
            printUtf8String("bk [l|s|x] a    - add breakpoint\n");
            printUtf8String("del [addr]      - remove breakpoints\n");
            printUtf8String("a address       - start assembler mode.\n");
        } else if (args[0] == "g") {
            // --goto--
            if (args.size() > 1) {
                cpu.PC = argi(1);
            }
            break; // monitor loop
        } else if (args[0] == "x" || args[0] == "exit") {
            break; // monitor loop
        } else if (args[0] == "z") {
            // --single step--
            cpu.breakPointHit = true;
            break; // monitor loop
        } else if (args[0] == "m") {
            // --memory--
            int from = argi(1), to = argi(2);
            if (to <= from) {
                to = from + 0x8f;
            }
            while (from < to) {
                scn.cleanCurrentLine();
                std::string str = StringHelper::int2hex(from, true, 2);
                for (int i = 0; i < 8; ++i) {
                    if ((i % 4) == 0 && i != 0) {
                        str += " ";
                    }
                    str += " " + StringHelper::int2hex(memory[from + i], true, 1);
                }

                str += " ";
                for (int i = 0; i < 8; ++i) {
                    char32_t c = memory[from + i];
                    if (c < 0x20) {
                        c = U'.';
                    }
                    Unicode::appendAsUtf8(str, c);
                }
                str += "\n";
                from += 8;
                printUtf8String(str);
            }
        } else if (args[0] == "a") {
            // --assembler mode--
            assembleAt    = argi(1);
            assemblerMode = true;
        } else if (args[0] == "d" || args[0] == "disass") {
            // --disassemble--
            int count     = 0x10000;
            uint16_t from = argi(1);
            if (from == 0) {
                from = cpu.PC;
            }
            uint16_t to = argi(2);
            if (to == 0) {
                to    = 0xffff;
                count = 20;
            }

            for (int i = 0; i < count; ++i) {
                if (from > to) {
                    break;
                }
                from = disassemble(from);
            }
        } else if (args[0][0] == '>') {
            // --poke--
            if (args[0].length() > 1) { // >0801 00 00 instead of > 0801 00 00
                args.insert(args.begin() + 1, args[0].substr(1));
            }
            int addr = argi(1);
            if (addr >= 0 && addr < memory.size()) {
                for (size_t i = 2; i < args.size(); ++i) {
                    memory[addr + i - 2] = argi(i);
                }
            }
        } else if (args[0] == "r") {
            // --registers--
            printUtf8String(cpu.registers() + "\n");
        } else if (args[0] == "break" || args[0] == "bk") {
            // --break--
            CPU6502::BreakPoint opt;
            opt.onExec = false;

            for (int i = 1; i < args.size(); ++i) {
                Unicode::toLower(args[i]);
                if (args[i][0] == 'l') {
                    opt.onRead = true;
                    continue;
                }
                if (args[i][0] == 's') {
                    opt.onWrite = true;
                    continue;
                }
                if (args[i][0] == 'x' || args[i] == "exec") {
                    opt.onExec = true;
                    continue;
                }

                int16_t addr = argi(i);
                if (!opt.onExec && !opt.onRead && !opt.onWrite) {
                    opt.onExec = true;
                }
                auto& br = cpu.breakpoints[addr];
                br.onExec |= opt.onExec;
                br.onRead |= opt.onRead;
                br.onWrite |= opt.onWrite;
            }
            // list all
            for (auto& bk : cpu.breakpoints) {
                printUtf8String("BREAK: $" + StringHelper::int2hex(bk.first, true, 2));
                if (bk.second.onExec) {
                    printUtf8String(" ON EXEC");
                }
                if (bk.second.onRead) {
                    printUtf8String(" ON LOAD");
                }
                if (bk.second.onWrite) {
                    printUtf8String(" ON STORE");
                }
                printUtf8String("\n");
            }
        } else if (args[0] == "del" || args[0] == "delete") {
            // --delete breakpoints--
            if (args.size() == 1) {
                cpu.breakpoints.clear();
            } else {
                for (size_t i = 1; i < args.size(); ++i) {
                    int addr = argi(i);
                    auto it  = cpu.breakpoints.find(addr);
                    if (it != cpu.breakpoints.end()) {
                        cpu.breakpoints.erase(it);
                    }
                }
            }
        } else {
            printUtf8String("? TYPE HELP FOR AVAILABLE COMMANDS\n");
        }
    }


    inMonitor = false;
    scn.restoreState(oldScreen);
}
