﻿#include "basic.h"
#include "about.h"
#include "help.h"
#include "os.h"
#include "string_helper.h"
#include "unicode.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <regex>

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
    auto lines   = StringHelper::split(about::text(), "\r\n");
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
    basic->currentModule().autoNumbering = basic->valueToInt(values[0]);
}

void cmdCHAR(Basic* basic, const std::vector<Basic::Value>& values) {
    int color = 0, x = 0, y = 0;
    std::string text;
    bool inverse = false;

    if (values.size() < 5) {
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
        basic->os->screen.inverseColours();
    }
    if (color > 0) {
        if (color > 16) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }

        int old = basic->os->screen.getTextColor();
        basic->os->screen.setTextColor(color - 1);
        color = old;
    }
    /// basic->printUtf8String(text.c_str());
    const char* utf8 = text.c_str();
    while (*utf8 != '\0') {
        basic->os->screen.putC(Unicode::parseNextUtf8(utf8));
    }
    if (color > 0) {
        // restore old color
        basic->os->screen.setTextColor(color);
    }

    if (inverse) {
        basic->os->screen.inverseColours();
    }
    basic->os->presentScreen();
}

void cmdCHDIR(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string dir = basic->valueToString(values[0]);
    dir             = basic->findFirstFileNameWildcard(dir, true);

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
    }

    if (iarg == 8 + 1 && ScreenInfo::charPixY == 8) {
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
    if (Unicode::utf8StrLen(str.c_str()) > 6) {
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

    std::string sline;
    std::u32string u32;
    for (auto& ln : basic->currentModule().listing) {
        if (ln.first < 0) {
            continue;
        }
        Unicode::toU32String(Unicode::toLowerAscii(ln.second.c_str()).c_str(), u32);

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

        basic->printUtf8String(sline);
        basic->handleEscapeKey(true);
    }
}

void cmdLOAD(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string path = basic->valueToString(values[0]);
    if (!basic->fileExists(path, true)) {
        throw Basic::Error(Basic::ErrorId::FILE_NOT_FOUND);
    }

    basic->printUtf8String("LOADING         \n");
    if (!basic->loadProgram(path)) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
    }
    basic->currentModule().setProgramCounterToEnd();
    basic->currentModule().filenameQSAVE = path;
}

void cmdSAVE(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    std::string filename = basic->valueToString(values[0]);
    if (basic->fileExists(filename, false)) {
        basic->printUtf8String("FILE EXISTS. OVERWRITE (Y/N)? ");
        std::string yesno = basic->inputLine(false);
        if (yesno.length() == 0 || Unicode::toUpperAscii(yesno[0]) != u'Y') {
            return;
        }
    }

    basic->printUtf8String("SAVING          \n");
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
    basic->printUtf8String("SAVE \"");
    basic->printUtf8String(filename);
    basic->printUtf8String("\" \n");
    cmdSAVE(basic, { filename });
}

void cmdOPEN(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    int64_t ifile = basic->valueToInt(values[0]);
    if (ifile < 1 || size_t(ifile) >= basic->openFiles.size()) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    if (basic->openFiles[ifile].pfile != nullptr) {
        {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
        }
    }

    const char* iomode = "r";
    std::string path   = basic->valueToString(values[2]); // "name*, R", "name, W"
    size_t comma       = path.rfind(',');
    if (comma != std::string::npos) {
        std::string strmode = path.substr(comma + 1);
        path                = path.substr(0, comma);
        StringHelper::trimRight(path, " ");
        StringHelper::trimLeft(strmode, " ");
        if (strmode.empty()) {
            throw Basic::Error(Basic::ErrorId::SYNTAX);
        }
        if (strmode[0] == 'r' || strmode[0] == 'R') {
            iomode = "r";
            path   = basic->findFirstFileNameWildcard(path);
        } else if (strmode[0] == 'w' || strmode[0] == 'W') {
            iomode = "w";
        } else {
            throw Basic::Error(Basic::ErrorId::INTERNAL);
        }
    }

    FILE* pf = basic->fopenUtf8(path, iomode);
    if (pf == nullptr) {
        throw Basic::Error(Basic::ErrorId::FILE_NOT_FOUND);
    }
    basic->openFiles[ifile].pfile = pf;
}

void cmdCLOSE(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    int64_t ifile = basic->valueToInt(values[0]);
    if (ifile < 1 || size_t(ifile) >= basic->openFiles.size()) {
        throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
    }
    if (basic->openFiles[ifile].pfile == nullptr) {
        {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_DEVICE);
        }
    }

    fclose(basic->openFiles[ifile].pfile);
    basic->openFiles[ifile].pfile = nullptr;
}

void cmdRENUMBER(Basic* basic, const std::vector<Basic::Value>& values) {
    // RENUMBER newstart, step, startfromold

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

    std::map<int, std::string> newListing;
    std::map<int, int> lineMapping;
    int newLineNumber = newstart;

    int lastLineNumber = -1;

    auto& listing = basic->currentModule().listing;
    // Create mapping of old line numbers to new ones
    for (auto& [oldLine, code] : listing) {
        if (oldLine >= 0 && oldLine >= startFromLine) {
            if (lastLineNumber > newLineNumber) {
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
    for (auto& [oldLine, code] : listing) {
        std::string updatedCode;
        std::sregex_iterator it(code.begin(), code.end(), lineRefRegex);
        std::sregex_iterator end;
        size_t lastPos = 0;

        for (; it != end; ++it) {
            updatedCode += code.substr(lastPos, it->position() - lastPos);
            updatedCode += it->str(1) + " ";

            // int oldTarget = std::stoi(it->str(2));
            std::string str   = it->str(2);
            const char* pnum  = str.c_str();
            int64_t oldTarget = 0;
            if (basic->parseInt(pnum, &oldTarget)) {
                updatedCode += std::to_string(lineMapping[int(oldTarget)]);
                lastPos = it->position() + it->length();
            }
        }
        updatedCode += code.substr(lastPos);

        newListing[lineMapping[oldLine]] = updatedCode;
    }
    listing = std::move(newListing);

    basic->currentModule().setProgramCounterToEnd();
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

void cmdSYS(Basic* basic, const std::vector<Basic::Value>& values) {
    if (!basic->valueIsString(values[0])) {
        throw Basic::Error(Basic::ErrorId::TYPE_MISMATCH);
    }
    if (values.size() != 1) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    std::string cmd = basic->valueToString(values[0]);

    basic->os->systemCall(cmd);
    // system(cmd.c_str());
}

void cmdCATALOG(Basic* basic, const std::vector<Basic::Value>& values) {
    if (values.size() != 0) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }

    auto files = basic->os->listCurrentDirectory();
    std::vector<std::string> sizestrings(files.size());
    size_t maxsizelen = 5; // leave space to write CHDIR before a directory name
    for (size_t i = 0; i < files.size(); ++i) {
        sizestrings[i] = files[i].isDirectory ? std::string("DIR") : basic->valueToString(int64_t(files[i].filesize));
        maxsizelen     = std::max(maxsizelen, sizestrings[i].length());
    }

    std::string dirname = basic->os->getCurrentDirectory();
    if (dirname.length() + 7 > basic->os->screen.width) {
        size_t len = basic->os->screen.width - 7;
        dirname    = dirname.substr(dirname.length() - len);
    }
    basic->printUtf8String("###");
    basic->printUtf8String(dirname);
    basic->printUtf8String("###\n");
    std::string str;
    for (size_t i = 0; i < files.size(); ++i) {
        auto& f = files[i].name;
        str     = sizestrings[i];
        while (str.length() < maxsizelen) {
            str.insert(str.begin(), ' ');
        }
        str += " \"" + f + "\"";
        if (basic->os->screen.width > str.length()) {
            str += std::string(basic->os->screen.width - str.length(), ' ');
        }
        str += "\n";

        basic->printUtf8String(str);
        basic->handleEscapeKey(true);
        basic->os->delay(50);
    }
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

    static const char* digits = "0123456789ABCDEF";
    size_t hex_len            = sizeof(n) << 1;
    std::string rc(hex_len, '0');
    for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
        rc[i] = digits[(n >> j) & 0x0f];
    return rc;
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
    const auto& state = basic->os->getGamepadState(int(port - 1));

    int64_t joy = 0;

    auto setJoy = [&joy, &state](int8_t xIs, int8_t yIs, int64_t ijoy) {
        if (state.dpad.x == xIs && state.dpad.y == yIs) {
            joy = ijoy;
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
    if (state.buttons[0] || state.buttons[1]) {
        joy += 128;
    }
    if (state.buttons[2] || state.buttons[3]) {
        joy += 256;
    }
    return joy;
}

Basic::Value fktLEFT$(Basic* basic, const std::vector<Basic::Value>& args) {
    if (args.size() != 3) {
        throw Basic::Error(Basic::ErrorId::ARGUMENT_COUNT);
    }
    size_t length = basic->valueToInt(args[2]);
    return Unicode::substr(basic->valueToString(args[0]), 0, length);
}

Basic::Value fktMID$(Basic* basic, const std::vector<Basic::Value>& args) {
    // str$, start, [len]
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
    size_t length   = Unicode::utf8StrLen(str.c_str());
    if (right >= length) {
        return str;
    }

    return Unicode::substr(str, length - right);
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

void Basic::Array::dim(size_t i0, size_t i1, size_t i2, size_t i3) {
    dim({ i0, i1, i2, i3 });
}

void Basic::Array::dim(const ArrayIndex& ai) {
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
    } catch (...) {
        throw Error(ErrorId::ILLEGAL_QUANTITY);
    }
}

Basic::Value& Basic::Array::at(const Basic::ArrayIndex& ix) {
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

Basic::Basic(Os* os, SoundSystem* ss) {
    Module mainmodule;
    modules[""] = mainmodule;
    moduleVariableStack.push_back(modules.begin()); // create a module with no name and push it on the stack
    moduleListingStack.push_back(modules.begin());

    this->os             = os;
    std::string charLogo = Unicode::toUtf8String(U"🌈");
    cmdCHARDEF(this, {
                         Value(charLogo),
                         Value(int64_t(0x2222222222222222LL)), // red
                         Value(int64_t(0x8888888888888888LL)), // orange
                         Value(int64_t(0x8888888888888888LL)), // orange
                         Value(int64_t(0x7777777777777777LL)), // yellow
                         Value(int64_t(0x7777777777777777LL)), // yellow
                         Value(int64_t(0xddddddddddddddddLL)), // green
                         Value(int64_t(0xddddddddddddddddLL)), // green
                         Value(int64_t(0xeeeeeeeeeeeeeeeeLL)), // blue
                     });

    os->init(this, ss);
    os->screen.setColors(13, 11);
    os->screen.setBorderColor(13);

    keyShortcuts[1 - 1] = "\"CHDIR\"";
    keyShortcuts[2 - 1] = "\"LOAD \"";
    keyShortcuts[3 - 1] = "\"CATALOG \"+CHR$(13)";
    keyShortcuts[4 - 1] = "\"SCNCLR \"+CHR$(13)";
    keyShortcuts[5 - 1] = "\"SAVE \"";
    keyShortcuts[6 - 1] = "\"RUN \"+CHR$(13)";
    keyShortcuts[7 - 1] = "\"LIST \"+CHR$(13)";

    // hard coded keywords
    keywords = { "ON", "GOTO", "GOSUB", "RETURN", "IF", "THEN", "LET", "FOR", "TO", "NEXT", "STEP", "RCHARDEF", "READ", "DATA", "RESTORE", "END", "RUN", "DIM", "PRINT", "?", "GET", "HELP", "INPUT", "REM", "CLR", "SCNCLR", "NEW", "LIST", "MODULE", "KEY", "GETKEY", "DEF", "FN", "DELETE", "USING" };

    // commands
    commands.insert({
        { "ABOUT", cmdABOUT },
        { "AUTO", cmdAUTO },
        { "CHAR", cmdCHAR },
        { "CHDIR", cmdCHDIR },
        { "COLOR", cmdCOLOR },
        { "CATALOG", cmdCATALOG },
        { "CHARDEF", cmdCHARDEF },
        { "SPRDEF", cmdSPRDEF },
        { "SPRITE", cmdSPRITE },
        { "MOVSPR", cmdMOVSPR },
        { "QUIT", cmdQUIT },
        { "FIND", cmdFIND },
        { "LOAD", cmdLOAD },
        { "OPEN", cmdOPEN },
        { "CLOSE", cmdCLOSE },
        { "SAVE", cmdSAVE },
        { "QSAVE", cmdQSAVE },
        { "RENUMBER", cmdRENUMBER },
        { "SYS", cmdSYS },
        { "PLAY", cmdPLAY },
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
    // args contain comma opertors!
    functions.insert({
        { "ABS", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return fabs(basic->valueToDouble(args[0])); } },
        { "ASC", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); std::string s = Basic::valueToString(args[0]); const char* p = s.c_str(); return (int64_t)Unicode::parseNextUtf8(p); } },
        { "ATN", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return atan(basic->valueToDouble(args[0])); } },
        { "CHR$", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); std::string s; Unicode::appendAsUtf8(s,  char32_t(basic->valueToInt(args[0]))); return s; } },
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
        { "LEN", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return (int64_t)Unicode::utf8StrLen(basic->valueToString(args[0]).c_str()); } },
        { "LOG", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return log(basic->valueToDouble(args[0])); } },
        { "MOD", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 3); auto div = basic->valueToInt(args[2]); if (div == 0) { throw Error(ErrorId::ILLEGAL_QUANTITY); }return basic->valueToInt(args[0]) % div; } },
        { "MID$", fktMID$ },
        { "PEEK", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return int64_t(memory[basic->valueToInt(args[0])]); } },
        { "POS", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return int64_t(basic->os->screen.getCursorPos().x); } },
        { "POSY", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return int64_t(basic->os->screen.getCursorPos().y); } },
        { "RIGHT$", fktRIGHT$ },
        { "RND", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return double(rand()) / double(RAND_MAX); } },
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
        { "VAL", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 1); return basic->valueToDouble(args[0]); } },
        { "XOR", [&](Basic* basic, const std::vector<Basic::Value>& args) -> Basic::Value { nargs(args, 3); return basic->valueToInt(args[0]) ^ basic->valueToInt(args[2]); } }
    });

#if 0
    printUtf8String("0123456789012345678901234567890123456789\n111111111x111111111x111111111x111111111x\n222222222x222222222x222222222x222222222x");
    printUtf8String("0123456789012345678901234567890123456789\n111111111x111111111x111111111x111111111x\n222222222x222222222x222222222x222222222x");
    size_t p = os.screen.setCursorPos({0, 2});
    size_t p2 = os.screen.getPosAtCursor({0,2});

    os.screen.setTextColor(1); os.screen.putC('1'); os.presentScreen();
    os.screen.setTextColor(2); os.screen.putC('2'); os.presentScreen();
    os.screen.setTextColor(3); os.screen.putC('3'); os.presentScreen();
    os.screen.setTextColor(4); os.screen.putC('4'); os.presentScreen();
    os.screen.setTextColor(5); os.screen.putC('5'); os.presentScreen();
    os.screen.setTextColor(6); os.screen.putC('6'); os.presentScreen();
    os.screen.setTextColor(7); os.screen.putC('7'); os.presentScreen();
    os.screen.putC('\n');
#endif

    size_t centerx = 3;
    printUtf8String("\n");
    printUtf8String(
        std::string(centerx, ' ') + std::string(" ****    BA67 BASIC") + charLogo + " V" + version() + ("   ****\n"));

    std::string strmem = " " + valueToString(int64_t(os->getFreeMemoryInBytes())) + std::string(" BASIC BYTES FREE");
    while (strmem.length() < 31) {
        strmem.insert(strmem.begin(), ' ');
    }
    printUtf8String(
        std::string(centerx, ' ') + strmem + "\n" + std::string(centerx, ' ') + "       (C)2025 DREAM DESIGN.\n");
}

#if 0
std::string Basic::version() {
    return "1.07";
    char mnames[] = {"JanFebMarAprMayJunJulAugSepOctNovDec" };
    // char today[16];
    char temp[8];
    int day, month, year;
    const char* today = __DATE__;
    // strcpy(today, __DATE__); // Date of compilation
    for (day = 0; day < 3; day++) { temp[day] = today[day]; }
    temp[3] = '\0';

    month = int(strstr(mnames, temp) - mnames) / 3 + 1;
    day = atoi(&today[4]);
    year = atoi(&today[strlen(today) - 5]);

    //  Today yyyy.mmdd
    //  version=(double)year + (double)month/100.0 + (double) day/10000.0;

    tm when;
    time_t now, start_year;
    // tm Xmas={ 0, 0, 12, 25, 11, 93 };
    memset(&when, 0, sizeof(tm));
    when.tm_hour = 12;
    when.tm_mday = day;
    when.tm_mon = month - 1;
    when.tm_year = year - 1900;
    now = mktime(&when);

    when.tm_mday = 1;
    when.tm_mon = 0;
    start_year = mktime(&when);

    // number of days in this year
    int64_t d = int64_t(ceil(difftime(now, start_year) / 86400.0 + 0.0001));  // Secs to days, 1st day = #1

    std::string version = valueToString(int64_t(year - 2025 + 1)) + ".";

    d = d / 4;  // max. number is 91
    if (d < 10) { version += '0'; }
    version += valueToString(d);
    return version;
}
#endif

int Basic::colorForModule(const std::string& str) const {
    int col = 0;
    int i   = 0;
    for (auto& m : modules) {
        if (m.first == str) {
            col = i;
            break;
        }
        ++i;
    }
    std::vector<int> colsToUse = { 13, 7, 14, 3, 1, 10 };

    return colsToUse[col % colsToUse.size()];
}

// Represent value as string

inline std::string Basic::valueToString(const Value& v) {
    if (auto s = std::get_if<std::string>(&v))
        return *s;
    if (auto i = std::get_if<int64_t>(&v))
        return std::to_string(*i);
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
    if (auto i = std::get_if<int64_t>(&v))
        return (double)(*i);
    if (auto d = std::get_if<double>(&v))
        return (*d);
    if (auto s = std::get_if<std::string>(&v)) {
        const char* str = s->c_str();
        double d        = 0;
        if (parseDouble(str, &d) && *str == '\0') {
            return d;
        }
    }
    throw Error(ErrorId::TYPE_MISMATCH);
}

inline int64_t Basic::valueToInt(const Value& v) {
    if (auto i = std::get_if<int64_t>(&v))
        return (*i);
    if (auto d = std::get_if<double>(&v))
        return (int)(*d);
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
    if (auto i = std::get_if<Basic::Operator>(&v))
        return true;
    return false;
}

bool Basic::valueIsString(const Value& v) {
    if (auto s = std::get_if<std::string>(&v)) {
        return true;
    }
    return false;
}

inline bool Basic::isEndOfWord(char c) {
    return (
        (c > 0 && c < '0') // operators
        || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ':' || c == ';' || c == '\"' || c == '\0');
}

inline const char* Basic::skipWhite(const char*& str) {
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') {
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

bool Basic::parseFileHandle(const char*& str, std::string* number) {
    skipWhite(str);
    char* endInt;
    if (*str == '#') {
        int64_t i = strtoll(str + 1, &endInt, 10);
        if (number) {
            std::string s((const char*)(str) + 1, (const char*)(endInt));
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
    int base = 10;
    if (str.starts_with("$")) {
        return static_cast<int64_t>(std::stoull(str.substr(1), nullptr, 16));
    } else {
        return std::stoll(str);
    }
}

inline bool Basic::parseKeyword(const char*& str, std::string* keyword) {
    skipWhite(str);
    for (auto& k : keywords) {
        if (strncmp(str, k.c_str(), k.length()) == 0 && isEndOfWord(str[k.length()])) {
            if (keyword) {
                *keyword = std::string(str, str + k.length());
                str += k.length();
                return true;
            }
        }
    }
    return false;
}

inline bool Basic::parseCommand(const char*& str, std::string* command) {
    skipWhite(str);
    for (auto& k : commands) {
        if (strncmp(str, k.first.c_str(), k.first.length()) == 0 && isEndOfWord(str[k.first.length()]) /*this differs from MS BASIC*/
        ) {
            if (command) {
                *command = std::string(str, str + k.first.length());
                str += k.first.length();
                return true;
            }
        }
    }
    return false;
}

inline bool Basic::parseString(const char*& str, std::string* stringUnquoted) {
    skipWhite(str);
    if (*str == '\"' || *str == '\'') {
        // char quoteChar = *str;
        const char* end = strchr(str + 1, *str /* quoteChar */);
        if (end == nullptr) {
            throw Error(ErrorId::SYNTAX);
        }
        if (stringUnquoted) {
            *stringUnquoted = std::string(str + 1, end);
        }
        str = end + 1;
        return true;
    }
    return false;
}

inline bool Basic::parseOperator(const char*& str, std::string* op) {
    skipWhite(str);
    const char* start = str;

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
        *op = std::string(start, str);
        return true;
    }
    return false;
}

inline bool Basic::parseIdentifier(const char*& str, std::string* identifier) {
    skipWhite(str);
    // start with alpha
    if ((*str >= 'A' && *str <= 'Z') || (*str >= 'a' && *str <= 'z')) {
        const char* pend = str;
        // alpha-numeric
        while (isalnum(*pend)) {
            ++pend;
        }

        // type postfix
        if (*pend == '$' || *pend == '%') {
            ++pend;
        }

        if (identifier) {
            *identifier = std::string(str, pend);
        }
        str = pend;
        return true;
    }
    return false;
}

std::vector<Basic::Token> Basic::tokenize(ProgramCounter* pProgramCounter) {
    std::vector<Token> tokens;
    if (pProgramCounter == nullptr) {
        pProgramCounter = &programCounter();
    }
    std::string& line = pProgramCounter->line->second;

    const char* pline = line.c_str();
    const char* pc    = pline;

    size_t xpos = pProgramCounter->position;
    while (xpos != 0) {
        if (*pc != '\0') {
            ++pc;
        }
        --xpos;
    }

    // debug(">>"); debug(pc); debug("\n");

    double d;
    int64_t i;
    std::string str, str2;

    while (*pc != '\0') {
        skipWhite(pc);
        const char* pcBeforeParse = pc;
        if (parseKeyword(pc, &str)) {
            tokens.push_back({ TokenType::KEYWORD, str });
            if (str.length() == 3 && str == "REM") { // REM is special. SYS is not!
                tokens.push_back({ TokenType::STRING, pc });
                while (*pc != '\0') {
                    ++pc;
                }
            }
        } else if (parseCommand(pc, &str)) {
            tokens.push_back({ TokenType::COMMAND, str });
            // if (str.length() == 3 && str == "SYS") {
            //     skipWhite(pc);
            //     if (*pc != '\"') {
            //         tokens.push_back({TokenType::STRING, pc});
            //         while (*pc != '\0') { ++pc; }
            //     }
            // }
        } else if (parseString(pc, &str)) {
            tokens.push_back({ TokenType::STRING, str });
        } else if (parseOperator(pc, &str)) {
            tokens.push_back({ TokenType::OPERATOR, str });
        } else if (parseInt(pc, &i)) {
            tokens.push_back({ TokenType::INTEGER, std::string(pcBeforeParse, pc) });
        } else if (parseDouble(pc, &d)) {
            tokens.push_back({ TokenType::NUMBER, std::string(pcBeforeParse, pc) });
        } else if (*pc == '(' || *pc == ')') {
            tokens.push_back({ TokenType::PARENTHESIS, std::string(pc, pc + 1) });
            ++pc;
        } else if (*pc == ',') {
            tokens.push_back({ TokenType::COMMA, std::string(",") });
            ++pc;
        } else if (parseFileHandle(pc, &str)) {
            tokens.push_back({ TokenType::FILEHANDLE, str });
        } else if (parseIdentifier(pc, &str)) {
            skipWhite(pc);
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
            throw Error(ErrorId::SYNTAX);
        }
    }

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

    // move to end of statement
    pProgramCounter->position = pc - pline;

#if 0
    debug("TK:");
    for (auto& t : tokens) {
        debug(t.value.c_str()); debug(" ");
    }
    debug("\n");
#endif

    if (!moduleVariableStack.back()->second.fastMode) {
        int delay = int(15 * tokens.size());
        os->delay(delay);
    }

    return tokens;
}

// Just tokenize a string without harming the program
std::vector<Basic::Token> Basic::tokenize(const std::string& code) {
    Module m;
    m.listing[0]          = code;
    m.programCounter.line = m.listing.begin();
    return tokenize(&m.programCounter);
}

// Operator precedence
inline int Basic::precedence(const std::string& op) {
    const int line0 = __LINE__;
    if (op == ";") {
        return __LINE__ - line0;
    } // ; is string concatenation operator
    if (op == "AND" || op == "OR") {
        return __LINE__ - line0;
    }
    if (op == "<" || op == ">" || op == "<>") {
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
    } // unaray operator
    return 0;
}

Basic::Value Basic::evaluateDefFnCall(Basic::FunctionDefinition& fn, const std::vector<Basic::Value>& arguments) {
    auto& mod = currentModule();

    if (1 + arguments.size() / 2 != fn.parameters.size()) {
        throw Error(ErrorId::ARGUMENT_COUNT);
    }

    // Create a local token list with argument substitution
    std::vector<Token> substitutedBody = fn.body;
    for (size_t i = 0; i < fn.parameters.size(); ++i) {
        TokenType valType = TokenType::NUMBER;
        char c            = valuePostfix(fn.parameters[i]);
        if (c == '$') {
            valType = TokenType::STRING;
        } else if (c == '%') {
            valType = TokenType::INTEGER;
        }

        for (auto& token : substitutedBody) {
            if (token.type == TokenType::IDENTIFIER && token.value == fn.parameters[i].value) {
                token.type  = valType;
                token.value = valueToString(arguments[i * 2 /* every other arg is the comma operator*/]);
            }
        }
    }

    // Evaluate the substituted expression
    return evaluateExpression(substitutedBody, 0)[0];
}

// Evaluate expression using Shunting-Yard algorithm
// put endPtr to the next token to process
// put start to the open brace '(' - endPtr will point to the item after the matching close brace
// put start to inside the brace '(' - endPtr will point to mathcing the closing brace
std::vector<Basic::Value> Basic::evaluateExpression(const std::vector<Token>& tokens, size_t start, size_t* ptrEnd) {
    std::vector<Value> output;

    std::vector<Value> values;
    std::vector<std::string> ops;

    auto applyOpSS = [](const std::string& a, const std::string& b, const std::string& op) -> Value {
        if (op == "+")
            return a + b;
        if (op == "=")
            return a == b ? int64_t(-1) : int64_t(0);
        if (op == ">")
            return a > b ? int64_t(-1) : int64_t(0);
        if (op == "<")
            return a < b ? int64_t(-1) : int64_t(0);
        if (op == ">=")
            return a >= b ? int64_t(-1) : int64_t(0);
        if (op == "<=")
            return a <= b ? int64_t(-1) : int64_t(0);
        if (op == "<>")
            return a != b ? int64_t(-1) : int64_t(0);
        throw Error(ErrorId::SYNTAX);
    };
    auto applyOpDD = [](double a, double b, const std::string& op) -> Value {
        if (op == "+")
            return a + b;
        if (op == "-")
            return a - b;
        if (op == "*")
            return a * b;
        if (op == "/")
            return a / b;
        if (op == "=")
            return a == b ? int64_t(-1) : int64_t(0);
        if (op == ">")
            return a > b ? int64_t(-1) : int64_t(0);
        if (op == "<")
            return a < b ? int64_t(-1) : int64_t(0);
        if (op == ">=")
            return a >= b ? int64_t(-1) : int64_t(0);
        if (op == "<=")
            return a <= b ? int64_t(-1) : int64_t(0);
        if (op == "<>")
            return a != b ? int64_t(-1) : int64_t(0);
        if (op == "^")
            return pow(a, b);
        if (op == "AND")
            return int64_t(a) & int64_t(b);
        if (op == "OR")
            return int64_t(a) | int64_t(b);
        if (op == "NOT")
            return ~int64_t(b);
        throw Error(ErrorId::SYNTAX);
    };
    auto applyOpII = [](int64_t a, int64_t b, const std::string& op) -> Value {
        if (op == "+")
            return a + b;
        if (op == "-")
            return a - b;
        if (op == "*")
            return a * b;
        if (op == "/")
            return double(a) / double(b);
        if (op == "=")
            return a == b ? int64_t(-1) : int64_t(0);
        if (op == ">")
            return a > b ? int64_t(-1) : int64_t(0);
        if (op == "<")
            return a < b ? int64_t(-1) : int64_t(0);
        if (op == ">=")
            return a >= b ? int64_t(-1) : int64_t(0);
        if (op == "<=")
            return a <= b ? int64_t(-1) : int64_t(0);
        if (op == "<>")
            return a != b ? int64_t(-1) : int64_t(0);
        if (op == "^")
            return int64_t(pow(a, b));
        if (op == "AND")
            return int64_t(a) & int(b);
        if (op == "OR")
            return int64_t(a) | int(b);
        if (op == "NOT")
            return ~int64_t(b);
        throw Error(ErrorId::SYNTAX);
    };
    auto applyOpDI = [](double a, int64_t b, const std::string& op) -> Value {
        if (op == "+")
            return a + double(b);
        if (op == "-")
            return a - double(b);
        if (op == "*")
            return a * double(b);
        if (op == "/")
            return a / double(b);
        if (op == "=")
            return a == double(b) ? int64_t(-1) : int64_t(0);
        if (op == ">")
            return a > b ? int64_t(-1) : int64_t(0);
        if (op == "<")
            return a < double(b) ? int64_t(-1) : int64_t(0);
        if (op == ">=")
            return a >= double(b) ? int64_t(-1) : int64_t(0);
        if (op == "<=")
            return a <= double(b) ? int64_t(-1) : int64_t(0);
        if (op == "<>")
            return a != double(b) ? int64_t(-1) : int64_t(0);
        if (op == "^")
            return pow(double(a), double(b));
        if (op == "AND")
            return int64_t(a) & int64_t(b);
        if (op == "OR")
            return int64_t(a) | int64_t(b);
        if (op == "NOT")
            return ~int64_t(b);
        throw Error(ErrorId::SYNTAX);
    };
    auto applyOpID = [](int64_t a, double b, const std::string& op) -> Value {
        if (op == "+")
            return double(a) + b;
        if (op == "-")
            return double(a) - b;
        if (op == "*")
            return double(a) * b;
        if (op == "/")
            return double(a) / b;
        if (op == "=")
            return double(a) == b ? int64_t(-1) : int64_t(0);
        if (op == ">")
            return double(a) > b ? int64_t(-1) : int64_t(0);
        if (op == "<")
            return double(a) < b ? int64_t(-1) : int64_t(0);
        if (op == ">=")
            return double(a) >= b ? int64_t(-1) : int64_t(0);
        if (op == "<=")
            return double(a) <= b ? int64_t(-1) : int64_t(0);
        if (op == "<>")
            return double(a) != b ? int64_t(-1) : int64_t(0);
        if (op == "^")
            return pow(double(a), double(b));
        if (op == "AND")
            return int64_t(a) & int64_t(b);
        if (op == "OR")
            return int64_t(a) | int64_t(b);
        if (op == "NOT")
            return ~int64_t(b);
        throw Error(ErrorId::SYNTAX);
    };

    // take operation from stack, execute it and put the result on the value stack
    auto applyOp = [&]() -> void {
        // debug("______________\n");
        // for (auto& v : values) {
        //     debug(valueToString(v).c_str()); debug("\n");
        // }
        // debug(ops.back().c_str()); debug("\n");

        std::string op = ops.back();
        ops.pop_back();

        // just pop unary + operator: a * +b
        if (op == "u+") {
            return;
        }

        Value a;
        Value b = values.back();
        values.pop_back();

        if (op == "u-") // unary neagtive sign operator
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
        if (toki.type == TokenType::KEYWORD && toki.value == "FN") {
            continue;
        }

        if (ptrEnd != nullptr) {
            *ptrEnd = i + 1;
        }

        if (toki.type == TokenType::NUMBER) {
            values.push_back(atof(toki.value.c_str()));
        } else if (toki.type == TokenType::INTEGER) {
            values.push_back(strToInt(toki.value));
        } else if (toki.type == TokenType::STRING) {
            values.push_back(toki.value);
        } else if (toki.type == TokenType::IDENTIFIER) {
            // function or array
            size_t end  = i;
            Value* pval = findLeftValue(currentModule(), tokens, i, &end);
            if (pval != nullptr) {
                values.push_back(*pval);
                if (end > i) {
                    i = end - 1;
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
                        *ptrEnd = i;
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
                std::string fname;
                if (prevToki.type == TokenType::KEYWORD && prevToki.value == "FN") {
                    fname = "FN" + toki.value;
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
                // Value val2 = values.back(); values.pop_back();
                // Value val1 = values.back(); values.pop_back();
                // std::string op = ops.back(); ops.pop_back();
                applyOp();
            }
            if (!values.empty()) {
                output.push_back(values.back());
                values.pop_back();
            }
            output.push_back(Operator(toki.value));

            values.clear();
            ops.clear();
            continue;
        } else if (toki.type == TokenType::UNARY_OPERATOR) {
            ops.push_back("u" + toki.value);
        } else if (toki.type == TokenType::OPERATOR) {
            while (!ops.empty() && precedence(ops.back()) >= precedence(toki.value)) {
                // Value val2 = values.back(); values.pop_back();
                // Value val1 = values.back(); values.pop_back();
                // std::string op = ops.back(); ops.pop_back();
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
                // Value val2 = values.back(); values.pop_back();
                // Value val1 = values.back(); values.pop_back();
                // std::string op = ops.back(); ops.pop_back();
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
        // Value val2 = values.back(); values.pop_back();
        // Value val1 = values.back(); values.pop_back();
        // std::string op = ops.back(); ops.pop_back();
        applyOp();
    }

    while (ops.size()) {
        values.push_back(Operator(ops.back()));
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
    mod.variables.clear();
    mod.arrays.clear();
    mod.listing.clear();
    memory = {};
    mod.gosubStack.clear();
    mod.forStack.clear();
    mod.setProgramCounterToEnd();
}

void Basic::doEND() {
    EndAndPopModule();
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
        pval = findLeftValue(currentModule(), tokens, i, &end);
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

        switch (valuePostfix(tokens[ivarname])) {
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
        // debug("::"); debug(valueToString(*pval).c_str()); debug("\n");
    } else {
        throw Error(ErrorId::SYNTAX);
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
    currentModule().restoreDataPosition();
    auto& listing = currentListing();
    for (auto it = listing.begin(); it != listing.end(); ++it) {
        if (it->first >= runFrom) {
            programCounter().line     = it;
            programCounter().position = 0;
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
    const char* name = "";
    auto it          = modules.end();
    if (tokens.size() > 1) {
        name = tokens[1].value.c_str();
        it   = modules.find(tokens[1].value);
    }
    if (it == modules.end()) {
        modules[name] = {};
        it            = modules.find(name);
    }
    moduleVariableStack.push_back(it);

    // debug("MODULE VARIABLES "); debug(name.c_str()); debug("\n");
}

void Basic::doPrintValue(Value& v) {
    if (valueIsOperator(v)) {
        if (auto op = std::get_if<Basic::Operator>(&v)) {
            if (op->value == ",") {
                if (currentFileNo = 00) {
                    auto crsr = os->screen.getCursorPos();
                    os->screen.setCursorPos({ (crsr.x / 10 + 1) * 10, crsr.y });
                } else {
                    printUtf8String("    ");
                }
            } else if (op->value == ";") {
                // do nothing
            }
        }
    } else {
        if (valueIsString(v)) {
            printUtf8String(Basic::valueToString(v));
        } else {
            printUtf8String((" " + Basic::valueToString(v) + " "));
        }
    }
}

void Basic::handlePRINT(std::vector<Token>& tokens) {
    if (tokens.size() < 2) {
        printUtf8String("\n");
        return;
    }

    if (tokens[1].type == TokenType::FILEHANDLE) {
        currentFileNo = strToInt(tokens[1].value);
        tokens.erase(tokens.begin() + 1);
    }

    if (tokens[1].type == TokenType::KEYWORD && tokens[1].value == "USING") {
        handlePRINT_USING(tokens);
        return;
    }

    std::string prt;
    bool forceNewline = true;
    size_t istart     = 1;
    for (;;) {
        auto vals = evaluateExpression(tokens, istart, &istart);
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

    // if (forcePlusSign) {
    //     oss << std::showpos;
    // } else {
    //     oss << std::noshowpos;
    // }

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
        auto vals = evaluateExpression(tokens, istart, &istart);
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
    printUtf8String(output);
}

void Basic::handleGET(const std::vector<Token>& tokens, bool waitForKeypress) {
    bool firstInput = true;
    for (size_t itk = 1; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (tk.type != TokenType::COMMA) {
            Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
            if (pval == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }

            Os::KeyPress key {};
            if (waitForKeypress) {
                key = os->getFromKeyboardBuffer();
                handleEscapeKey();
            } else {
                if (os->keyboardBufferHasData()) {
                    key = os->getFromKeyboardBuffer();
                }
            }

            std::string str;
            if (key.printable) {
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
                    break; // del   20 14
                case uint32_t(Os::KeyConstant::ESCAPE):
                    Unicode::appendAsUtf8(str, 0x001b);
                    break; // ESC   27 1b
                case 0x00a3:
                    Unicode::appendAsUtf8(str, 0x005c);
                    break; // pound unicode 163
                }
            }
            switch (valuePostfix(tk)) {
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
}

void Basic::handleINPUT(const std::vector<Token>& tokens) {
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
                    // printUtf8String("\n");

                    switch (valuePostfix(tk)) {
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
                } catch (Error e) {
                    if (e.ID == ErrorId::BREAK) {
                        throw e;
                    }

                    printUtf8String("? REDO FROM START \n");
                }
            }
        }
    }
}

// DIM statement (array declaration)
inline void Basic::handleDIM(const std::vector<Token>& tokens) {
    if (tokens.size() < 4)
        return;
    if (tokens[2].type != TokenType::PARENTHESIS || tokens.back().type != TokenType::PARENTHESIS) {
        throw Error(ErrorId::SYNTAX);
    }

    // DIM a(b,c), d(e,f)
    auto& arrays = currentModule().arrays;
    size_t end   = 0;
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].type != TokenType::IDENTIFIER) {
            throw Error(ErrorId::SYNTAX);
        }
        std::string varName = tokens[i].value;
        ++i;
        if (tokens[i].type != TokenType::PARENTHESIS || tokens[i].value != "(") {
            throw Error(ErrorId::SYNTAX);
        }

        auto vals   = evaluateExpression(tokens, i + 1, &end);
        auto bounds = indexFromValues(vals);
        arrays[varName].dim(bounds);
        if (end > i) {
            i = end - 1;
        }
        if (tokens[i + 1].type != TokenType::PARENTHESIS) {
            throw Error(ErrorId::SYNTAX);
        }

        // optinally more arrays
        if (i + 2 < tokens.size() && tokens[i + 2].type != TokenType::COMMA) {
            throw Error(ErrorId::SYNTAX);
        } else {
            i += 2;
        }
    }
}

void Basic::handleLIST(const std::vector<Token>& tokens) {
    int from = 0;
    int to   = 0x7ffffff;
    if (tokens.size() > 1) {
        from = int(valueToInt(tokens[1].value));
        if (from < 0) {
            from = 0;
        }
    }
    if (tokens.size() > 3 && tokens[2].type == TokenType::OPERATOR) {
        to = int(valueToInt(tokens[3].value));
        if (to < from) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }
    }

    std::string sline;
    for (auto& ln : currentModule().listing) {
        if (ln.first < from) {
            continue;
        }
        if (ln.first > to) {
            break;
        }

        sline = std::to_string(ln.first);
        for (size_t s = sline.length(); s < 3; ++s) {
            sline += ' ';
        }
        sline += ' ';
        sline += ln.second;
        sline += '\n';

        printUtf8String(sline);
        handleEscapeKey(true);
        os->delay(50);
    }
}

void Basic::handleDELETE(const std::vector<Token>& tokens) {
    int from = 0;
    int to   = 0x7ffffff;
    if (tokens.size() > 1) {
        from = int(valueToInt(tokens[1].value));
        if (from < 0) {
            from = 0;
        }
    }
    if (tokens.size() > 3 && tokens[2].type == TokenType::OPERATOR) {
        to = int(valueToInt(tokens[3].value));
        if (to < from) {
            throw Basic::Error(Basic::ErrorId::ILLEGAL_QUANTITY);
        }
    }

    auto& lst = currentModule().listing;
    for (auto ln = lst.cbegin(); ln != lst.cend() /* not hoisted */; /* no increment */) {
        if (ln->first >= from && ln->first <= to) {
            lst.erase(ln++); // or "ln = m.erase(it)" since C++11
        } else {
            ++ln;
        }
    }
}

void Basic::handleKEY(const std::vector<Token>& tokens) {
    if (tokens.size() == 1) {
        for (size_t k = 0; k < keyShortcuts.size(); ++k) {
            std::string s = "KEY " + valueToString(int64_t(1 + k)) + "," + keyShortcuts[k] + " \n";
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
            str += "\"" + tokens[i].value + "\"";
        } else {
            str += tokens[i].value;
        }
    }
}

void Basic::handleRCHARDEF(const std::vector<Token>& tokens) {
    int iarg = 0; // nth argument to assign

    // iarg                 0   1   2
    // RCHARDEF ichar, ismono, b1, b2, b3, b4, b5, b6, b7, b8

    CharBitmap bmp;
    for (size_t itk = 1; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (itk == 1) {
            std::string str;
            if (tk.type == TokenType::STRING) {
                str = tk.value;
            } else if (tk.type == TokenType::IDENTIFIER) {
                Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
                if (pval == nullptr) {
                    throw Error(ErrorId::SYNTAX);
                }
                str = valueToString(*pval);
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

            switch (valuePostfix(tk)) {
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

char Basic::valuePostfix(const Token& t) const {
    if (t.value.ends_with('$')) {
        return '$';
    }
    if (t.value.ends_with('%')) {
        return '%';
    }
    if (t.type == TokenType::INTEGER) {
        return '%';
    }
    if (t.type == TokenType::NUMBER) {
        return '#';
    }
    if (t.type == TokenType::STRING) {
        return '$';
    }
    return '#';
}

// put endPtr to the next token to process
// returns nullptr on error
Basic::Value* Basic::findLeftValue(Module& module, const std::vector<Token>& tokens, size_t start, size_t* endPtr) {
    size_t i = start;
    if (tokens.size() > i + 2 && tokens[i + 1].type == TokenType::PARENTHESIS && tokens[i + 1].value == "(") {
        auto& arrays = module.arrays;
        auto arrit   = arrays.find(tokens[i].value);
        if (arrit == arrays.end()) {
            return nullptr; // might be a function call
        }
        // parse the comma separated list of arguments
        size_t end = i;
        auto args  = evaluateExpression(tokens, i + 2, &end);
        if (tokens[end].value != ")") {
            throw Error(ErrorId::SYNTAX);
        }
        i = end + 1; // after brace

        if (endPtr) {
            *endPtr = i;
        }
        if (arrit != arrays.end()) {
            // array(index)
            auto arridx = this->indexFromValues(args);
            return &arrit->second.at(arridx);
        } else {
            return nullptr;
        }
    }

    if (tokens[i].type == TokenType::IDENTIFIER) {
        auto& variables = module.variables;
        auto varit      = variables.find(tokens[i].value);
        if (varit == variables.end()) {
            // create new variable
            switch (valuePostfix(tokens[i])) {
            case '%':
                variables[tokens[i].value] = 0LL;
                break;
            case '$':
                variables[tokens[i].value] = "";
                break;
            default:
            case '#':
                variables[tokens[i].value] = 0.0;
                break;
            }
            varit = variables.find(tokens[i].value);
        }
        if (endPtr) {
            *endPtr = 1 + start;
        }

        return &varit->second;
    }

    return nullptr;
}

void Basic::doGOTO(int line, bool isGoSub) {
    // are we in not the same module as the programCounter?
    // then, push programCounter to stack and goto the module code
    if (moduleVariableStack.size() != moduleListingStack.size()) {
        moduleListingStack.push_back(moduleVariableStack.back());
        ASSERT(moduleVariableStack.size() != moduleListingStack.size());
        currentModule().setProgramCounterToEnd();
    }

    if (isGoSub) {
        currentModule().gosubStack.push_back(programCounter());
    }
    // TODO make sure you're in the module code now
    auto& listing = currentListing();
    auto it       = listing.find(line);
    if (it == listing.end()) {
        throw Error(ErrorId::UNDEFD_STATEMENT);
    }
    programCounter().line     = it;
    programCounter().position = 0;
}

void Basic::handleGOTO(std::vector<Token>& tokens) {
    auto values = evaluateExpression(tokens, 1);
    if (values.size() != 1) {
        throw Error(ErrorId::SYNTAX);
    }
    int64_t line = valueToInt(values.back());
    tokens.clear();
    doGOTO(int(line), false);
}

void Basic::handleGOSUB(std::vector<Token>& tokens) {
    auto values = evaluateExpression(tokens, 1);
    if (values.size() != 1) {
        throw Error(ErrorId::SYNTAX);
    }
    int64_t line = valueToInt(values.back());
    tokens.clear();
    doGOTO(int(line), true);
}

void Basic::handleONGOTO(std::vector<Token>& tokens) {
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

void Basic::handleHELP(std::vector<Token>& tokens) {
    std::string cmd;
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (i > 1) {
            cmd += " ";
        }
        cmd += tokens[i].value;
    }
    cmd             = Unicode::toUpperAscii(cmd.c_str());
    std::string usg = Help::getUsage(cmd) + " \n";
    printUtf8String(usg);
}

void Basic::handleDEFFN(std::vector<Token>& tokens) {
    std::string fname;
    if (!tokens.empty() && tokens.front().value == "DEF") {
        tokens.erase(tokens.begin(), tokens.begin() + 1);
    }
    if (!tokens.empty() && tokens.front().value == "DEFFN") {
        tokens.erase(tokens.begin(), tokens.begin() + 1);
        fname = "FN";
    }
    if (!tokens.empty() && tokens.front().value == "FN") {
        tokens.erase(tokens.begin(), tokens.begin() + 1);
        fname = "FN";
    }

    // tokens =   0    1  2  3  4  5+
    //         { name, (, X, ), =, code...}
    if (tokens.size() < 5 || tokens[1].type != TokenType::PARENTHESIS || tokens[2].type != TokenType::IDENTIFIER) {
        throw Error(ErrorId::SYNTAX);
    }
    fname += tokens[0].value;

    FunctionDefinition def;
    size_t i;
    size_t iStartBody = 0;
    for (i = 2; i + 3 < tokens.size(); i += 2) {
        if (tokens[i].type != TokenType::IDENTIFIER) {
            throw Error(ErrorId::SYNTAX);
        }
        def.parameters.push_back(tokens[i]);
        if (tokens[i + 1].type != TokenType::COMMA && tokens[i + 1].type != TokenType::PARENTHESIS) {
            throw Error(ErrorId::SYNTAX);
        }

        if (tokens[i + 1].type == TokenType::PARENTHESIS) {
            if (tokens[i + 2].value != "=") {
                throw Error(ErrorId::SYNTAX);
            }
            iStartBody = i + 3; // drop args, ), =
            break;
        }
    }

    if (tokens.empty()) {
        throw Error(ErrorId::SYNTAX);
    }

    // store the function definition:
    tokens.erase(tokens.begin(), tokens.begin() + iStartBody);

    def.body                             = tokens;
    currentModule().functionTable[fname] = def;
}

// Handle FOR statement
void Basic::handleFOR(const std::vector<Token>& tokens) {
    if (tokens.size() < 6 || tokens[2].value != "=")
        return;
    std::string varName = tokens[1].value;
    auto values         = evaluateExpression(tokens, 3);
    int64_t start       = valueToInt(values.back());

    auto& modl = currentModule();

    modl.variables[varName] = values.back();

    int64_t toEnd = start;
    int64_t step  = 1;
    for (size_t i = 4; i < tokens.size(); ++i) {
        if (tokens[i].value == "TO") {
            auto values = evaluateExpression(tokens, i + 1);
            toEnd       = valueToInt(values.back());
        }
        if (tokens[i].value == "STEP") {
            auto values = evaluateExpression(tokens, i + 1);
            step        = valueToInt(values.back());
        }
    }
    modl.variables[varName] = int64_t(start);
    modl.forStack.push_back({ varName, start, toEnd, step, modl.programCounter.line->first /*line*/, modl.programCounter.position });
}

// Handle NEXT statement
void Basic::handleNEXT(const std::vector<Token>& tokens) {
    // TODO how to deal with modules here?
    auto& modl = currentModule();
    if (modl.forStack.empty())
        return;

    ForLoop loop    = modl.forStack.back();
    Value& v        = modl.variables[loop.varName];
    int64_t loopVar = valueToInt(v) + loop.step;
    v               = loopVar;

    // debug("next "); debug(loop.varName.c_str()); debug("="); debug(valueToString(v).c_str()); debug("\n");

    if ((loop.step > 0 && loopVar <= loop.end) || (loop.step < 0 && loopVar >= loop.end)) {
        modl.programCounter.line = modl.listing.find(loop.line_number);
        if (modl.programCounter.line == modl.listing.end()) {
            throw Error(ErrorId::INTERNAL);
        }
        modl.programCounter.position = loop.char_position;
    } else {
        modl.forStack.pop_back();
    }
}

void Basic::handleIFTHEN(std::vector<Token>& tokens) {
    size_t endtok = 0;
    auto values   = evaluateExpression(tokens, 1, &endtok);
    if (values.size() != 1) {
        throw Error(ErrorId::SYNTAX);
    }
    if (valueToInt(values[0]) != 0 && endtok > 0) {
        tokens.erase(tokens.begin(), tokens.begin() + (endtok));

        // THEN 110
        if (tokens.size() > 1 && tokens[1].type == TokenType::INTEGER) {
            tokens[0].type  = TokenType::KEYWORD;
            tokens[0].value = "GOTO";
        }

        if (tokens.begin()->value == "THEN") {
            // THEN PRINT ...
            tokens.erase(tokens.begin(), tokens.begin() + 1);
        }
        executeTokens(tokens);
    } else { // skip rest of the line CBM BASIC style. Dartmouth would evaluate the next command.
        programCounter().line++;
        programCounter().position = 0;
    }
}

Basic::Value Basic::readNextData() {
    auto& cm = currentModule();

    std::vector<Basic::Token> tokens;
    for (;;) {
        ProgramCounter pcOfThisData = cm.readDataPosition;
        try {
            tokens = tokenize(&cm.readDataPosition);
        } catch (...) {
            throw ErrorId::INTERNAL;
        }

        if (tokens.empty()) {
            ++cm.readDataPosition.line;
            cm.readDataPosition.position = 0;
            cm.readDataIndex             = 0;
            if (cm.readDataPosition.line == cm.listing.end()) {
                throw Error(ErrorId::OUT_OF_DATA);
            }
            continue;
        } else if (tokens[0].value == "DATA") {
            auto pcAfterThisData = cm.readDataPosition; // where to read next DATA
            cm.readDataPosition  = pcOfThisData; // rewind to this DATA for more values

            int nthData = 0;
            for (auto& t : tokens) {
                if (t.type == TokenType::STRING || t.type == TokenType::IDENTIFIER) {
                    if (nthData == cm.readDataIndex) {
                        ++cm.readDataIndex;
                        return t.value;
                    }
                    ++nthData;
                }
                if (t.type == TokenType::INTEGER) {
                    if (nthData == cm.readDataIndex) {
                        ++cm.readDataIndex;
                        return strToInt(t.value);
                    }
                    ++nthData;
                }
                if (t.type == TokenType::NUMBER) {
                    if (nthData == cm.readDataIndex) {
                        ++cm.readDataIndex;
                        return atof(t.value.c_str());
                    }
                    ++nthData;
                }
            }

            // end of this DATA - read from next DATA
            ASSERT(nthData == cm.readDataIndex);

            cm.readDataPosition = pcAfterThisData;
            cm.readDataIndex    = 0;
        }
    }
    return Value();
}

void Basic::handleREAD(std::vector<Token>& tokens) {
    for (size_t itk = 1; itk < tokens.size(); ++itk) {
        auto& tk = tokens[itk];
        if (tk.type == TokenType::IDENTIFIER) {
            Value* pval = findLeftValue(currentModule(), tokens, itk, &itk);
            if (pval == nullptr) {
                throw Error(ErrorId::SYNTAX);
            }

            Value val = readNextData();
            switch (valuePostfix(tk)) {
            case '$':
                if (!valueIsString(val)) {
                    throw Error(ErrorId::TYPE_MISMATCH);
                }
                break;
            default:
            case '%':
            case '#':
                if (valueIsString(val)) {
                    throw Error(ErrorId::TYPE_MISMATCH);
                }
                break;
            }
            *pval = val;
        }
    }
}

void Basic::handleRESTORE(std::vector<Token>& tokens) {
    auto args = evaluateExpression(tokens, 1);
    auto& cm  = currentModule();
    if (args.empty()) {
        cm.restoreDataPosition();
    } else {
        int line = int(valueToInt(args[0]));
        for (auto it = cm.listing.begin(); it != cm.listing.end(); ++it) {
            if (it->first == line) {
                cm.readDataPosition.line     = it;
                cm.readDataPosition.position = 0;
                cm.readDataIndex             = 0;
                return;
            }
        }
        throw Error(ErrorId::UNDEFD_STATEMENT);
    }
}

// void Basic::handleDATA(std::vector<Token>& tokens) {}

void Basic::updateConstantVariables() {
    auto& vars = currentModule().variables;
    vars["TI"] = Basic::Value(int64_t(os->tick() * 60LL) / 1000LL);
}

void Basic::executeTokens(std::vector<Token>& tokens) {
    auto& modl = currentModule();
    if (tokens[0].type == TokenType::COMMAND) {
        auto cmd = commands.find(tokens[0].value);
        cmd->second(this, evaluateExpression(tokens, 1));
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
            if (modl.gosubStack.empty()) {
                throw Error(ErrorId::RETURN_WITHOUT_GOSUB);
            }
            programCounter() = modl.gosubStack.back();
            modl.gosubStack.pop_back();
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
        } else if (tokens[0].value == "GETKEY") {
            handleGET(tokens, true);
        } else if (tokens[0].value == "ON") {
            handleONGOTO(tokens);
        } else if (tokens[0].value == "REM") {
        } else if (tokens[0].value == "CLR") {
            modl.variables.clear();
            modl.arrays.clear();
            modl.forStack.clear();
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
        } else {
            throw Error(ErrorId::UNIMPLEMENTED_COMMAND);
        }
    } else {
        throw Error(ErrorId::SYNTAX);
    }
}

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

void Basic::printUtf8String(const char* utf8) {
    if (currentFileNo == 0) {
        while (*utf8 != '\0') {
            os->screen.putC(Unicode::parseNextUtf8(utf8));
        }
        os->presentScreen();
    } else {
        FILE* pf = openFiles[currentFileNo].pfile;
        if (pf == nullptr) {
            currentFileNo = 0;
            throw Error(ErrorId::ILLEGAL_DEVICE);
        } else {
            fprintf(pf, "%s", utf8);
            fflush(pf);
        }
    }
}

std::string Basic::inputLine(bool allowVertical) {
    if (os->settings.demoMode) {
        os->delay(2000);
    }

    isCursorActive     = true;
    bool movedVertical = false;

    auto typeString = [&](const std::string& s) {
        auto tok  = tokenize(s);
        auto vals = evaluateExpression(tok, 0);
        if (vals.empty()) {
            return;
        }
        for (auto& c : valueToString(vals[0])) {
            os->putToKeyboardBuffer(c);
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

    for (;;) {
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
                if (!isSelecting) {
                    isSelecting    = true;
                    startSelection = os->screen.getCursorPos();
                }
            } else {
                isSelecting = false;
            }
        };

        if (!key.printable) {
            if (key.holdCtrl) {
                if (key.code == U'C') {
                    auto str32            = os->screen.getSelectedText(startSelection, os->screen.getCursorPos());
                    std::string clipboard = Unicode::toUtf8String(str32.c_str());
                    if (clipboard.length() > 0) {
                        printf("copy %s\n", clipboard.c_str());
                        os->setClipboardData(clipboard);
                    }
                } else if (key.code == U'V') {
                    std::string clipboard = os->getClipboardData();
                    printf("pasted %s\n", clipboard.c_str());
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
                } else if (key.code == U'.' || key.code == 190) {
                    auto emoji = os->emojiPicker();
                    for (auto e : emoji) {
                        os->screen.putC(e);
                    }
                }
                continue;
            } // ctrl

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
                    insertMode = !insertMode;
                } else {
                    os->screen.insertSpace();
                }
                continue;
            case uint32_t(Os::KeyConstant::CRSR_LEFT):
                startSel();
                os->screen.moveCursorPos(-1, 0);
                continue;
            case uint32_t(Os::KeyConstant::CRSR_RIGHT):
                startSel();
                os->screen.moveCursorPos(1, 0);
                continue;
            case uint32_t(Os::KeyConstant::CRSR_UP):
                startSel();
                os->screen.moveCursorPos(0, -1);
                movedVertical = true;
                continue;
            case uint32_t(Os::KeyConstant::CRSR_DOWN):
                startSel();
                os->screen.moveCursorPos(0, 1);
                movedVertical = true;
                continue;
            case uint32_t(Os::KeyConstant::HOME):
                startSel();
                os->screen.setCursorPos({ 0, os->screen.getCursorPos().y });
                continue;
            case uint32_t(Os::KeyConstant::END):
                startSel();
                {
                    auto crsr = os->screen.getEndOfLineAt(os->screen.getCursorPos()); // that's the '\n' character
                    os->screen.setCursorPos(crsr);
                }
                continue;
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

    auto crsr = os->screen.getCursorPos();
    ScreenBuffer::Cursor istart {}, iend {};

    std::u32string screenchars;
    if (cursorAtStartOfInput.y != crsr.y) {
        // Moved a line up
        startCrsr   = crsr;
        startCrsr.x = 0;
        istart      = os->screen.getStartOfLineAt(startCrsr);
        iend        = os->screen.getEndOfLineAt(crsr); // that's the '\n' character

        crsr   = iend;
        crsr.x = 0;
        crsr.y++;
        screenchars = os->screen.getSelectedText(istart, iend);
        os->screen.setCursorPos(crsr);
    } else {
        istart      = (startCrsr);
        iend        = (crsr);
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

    isCursorActive = false;
    return str;
}

// List all variables and their values

inline void Basic::dumpVariables() {
    std::cout << "MODULE " << moduleVariableStack.back()->first << std::endl;
    if (currentModule().variables.empty() && currentModule().arrays.empty()) {
        return;
    }

    std::cout << "\n    vars:" << std::endl;
    for (auto& v : currentModule().variables) {
        std::cout << "    " << v.first << " = " << valueToString(v.second) << std::endl;
    }
    for (auto& v : currentModule().arrays) {
        std::cout << "    " << v.first << "()" << std::endl;
    }
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

    for (auto& s : os->screen.sprites) {
        s.enabled = false;
    }

    if (os->screen.getCursorPos().x != 0) {
        printUtf8String("\n");
    }
    // clear keyboard buffer (ESC key mainly)
    // while (os->keyboardBufferHasData()) {
    //     os->getFromKeyboardBuffer();
    // }
}

Basic::ParseStatus Basic::parseInput(const char* pline) {
    // program a line
    const char* pc            = pline;
    ParseStatus executeStatus = ParseStatus::PS_EXECUTED;

    try {
        int64_t n = 0;
        if (parseInt(pc, &n)) {
            // debug("program a line\n");
            if (n < 0) {
                throw Error(ErrorId::SYNTAX);
            }

            auto& cm = currentModule();

            cm.listing[int(n)]       = skipWhite(pc);
            cm.lastEnteredLineNumber = n;

            cm.programCounter.line     = cm.listing.find(int(n));
            cm.programCounter.position = 0;

            while (!tokenize().empty()) { } // check sanity, but don't execute

            cm.setProgramCounterToEnd();
            return ParseStatus::PS_PROGRAMMED;
        } else {
            // Immediate mode
            // debug("program line -1\n");
            auto& cm       = currentModule();
            cm.listing[-2] = skipWhite(pc);
            cm.listing[-1] = "REM LINE -1 ENDS THE IMMEDIATE MODE EXECUTION";

            if (*pc == '\0') {
                executeStatus = ParseStatus::PS_IDLE;
            }

            if (moduleListingStack.size() < moduleVariableStack.size()) {
                // in immediate mode, the code stack must euqal the variable stack
                moduleListingStack.push_back(moduleVariableStack.back());
            }
            programCounter().line     = currentListing().begin();
            programCounter().position = 0;
        }

        // execute listing code
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
                    printUtf8String("[" + valueToString(line) + "]");
                }
            }

            updateConstantVariables();

            // debug("MODULE VARS "); debug(moduleVariableStack.back()->first.c_str()); debug("\n");
            // debug("MODULE CODE "); debug(moduleListingStack.back()->first.c_str()); debug("\n");

#if 0
            debug("                                     LINE ");
            debug(valueToString(int64_t(programCounter().line->first)).c_str());
            debug("\n");
#endif

            std::vector<Token> tokens = tokenize();
            if (tokens.empty()) { // next line
                auto& pc = programCounter();
                pc.line++;
                pc.position = 0;

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
            executeTokens(tokens);
            os->updateKeyboardBuffer();
            os->presentScreen();
            handleEscapeKey();
        }
    } catch (Error e) {
        currentFileNo = 0;
        // dumpVariables();

        std::string& line = programCounter().line->second;
        int iline         = programCounter().line->first;
        if (iline >= 0) {
            restoreColorsAndCursor(true);
            std::string msg = "?" + errorMessages[e.ID] + " IN " + valueToString(iline);
            msg += std::string(os->screen.width - msg.length() - 1, ' ') + "\n";
            printUtf8String(msg);

            if (e.ID != ErrorId::BREAK && iline >= 0) {
                parseInput((std::string("LIST ") + valueToString(iline) + " - " + valueToString(iline)).c_str());
            }
        } else {
            printUtf8String("?" + errorMessages[e.ID] + " \n");
        }
        return ParseStatus::PS_ERROR;
    }

    return executeStatus;
}

void Basic::handleEscapeKey(bool allowPauseWithShift) {
    // break with escape
    if (os->isKeyPressed(Os::KeyConstant::ESCAPE)) {
        restoreColorsAndCursor(true);
        throw Error(ErrorId::BREAK);
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
    }
}

// Basic interpreter loop
void Basic::runInterpreter() {
    currentModule().forStack.clear();
    currentModule().gosubStack.clear();

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
                size_t y = os->screen.getCursorPos().y;
                os->screen.setCursorPos({ 0, y - 1 });
            }

            // wait for ESC release
            while (os->isKeyPressed(Os::KeyConstant::ESCAPE)) {
                os->updateKeyboardBuffer();
                os->delay(100);
                // clear keyboard buffer
                while (os->keyboardBufferHasData()) {
                    os->getFromKeyboardBuffer();
                }
            }
        }
        // debug("MODULE VARS "); debug(moduleVariableStack.back()->first.c_str()); debug("\n");
        // debug("MODULE CODE "); debug(moduleListingStack.back()->first.c_str()); debug("\n");

        // std::getline(std::cin, line);
        try {
            line = inputLine(true);
        } catch (Error) {
            currentFileNo = 0;
            status        = ParseStatus::PS_ERROR;
            continue;
        }
        uppercaseProgram(line);

        status = parseInput(line.c_str());
    }
}

void Basic::waitForKeypress() {
    for (;;) {
        this->handleEscapeKey();
        if (os->keyboardBufferHasData()) {
            break;
        }
        os->delay(100);
        os->updateKeyboardBuffer();
    }
}

std::string Basic::findFirstFileNameWildcard(std::string filenameUtf8, bool isDirectory) {
    if (filenameUtf8.find('*') == std::string::npos && filenameUtf8.find('?') == std::string::npos) {
        return filenameUtf8;
    }
    std::string cd = os->getCurrentDirectory();

    std::string fixedDirs;
    for (;;) {
        size_t endOfDir = filenameUtf8.find('/');
        if (endOfDir == std::string::npos) {
            break;
        }

        std::string folder = findFirstFileNameWildcard(filenameUtf8.substr(0, endOfDir), true);
        fixedDirs += folder;
        fixedDirs += '/';
        filenameUtf8 = filenameUtf8.substr(endOfDir + 1);

        if (!os->setCurrentDirectory(folder)) {
            break;
        }
    }

    std::string outpath = fixedDirs + filenameUtf8;

    // list and search
    std::u32string fileu32;
    if (Unicode::toU32String(filenameUtf8.c_str(), fileu32)) {
        auto files = os->listCurrentDirectory();
        for (auto& f : files) {
            std::u32string fu32;
            if (f.isDirectory == isDirectory && Unicode::toU32String(f.name.c_str(), fu32)) {
                if (Unicode::wildcardMatch(fu32.c_str(), fileu32.c_str())) {
                    outpath = fixedDirs + f.name;
                    break;
                }
            }
        }
    }

    os->setCurrentDirectory(cd);
    return outpath;
}

FILE* Basic::fopenUtf8(const std::string& filenameUtf8, const char* mode) {
    FILE* file = nullptr;

    if (mode[0] == 'w') {
#ifdef _WIN32
        std::u16string u16;
        Unicode::toU16String(filenameUtf8.c_str(), u16);
        file = _wfsopen(reinterpret_cast<const wchar_t*>(u16.c_str()), L"wb", _SH_DENYNO); // allow shared reading - even if some editor has the file open
#else
        file = fopen(filenameUtf8.c_str(), "wb");
#endif
    } else if (mode[0] == 'r') {
#ifdef _WIN32
        std::u16string u16;
        Unicode::toU16String(filenameUtf8.c_str(), u16);
        file = _wfsopen(reinterpret_cast<const wchar_t*>(u16.c_str()), L"rb", _SH_DENYNO); // allow shared reading - even if some editor has the file open
#else
        file = fopen(filenameUtf8.c_str(), "rb");
#endif
    } else {
        throw std::runtime_error("fopenUtf8 - only r and w supported");
    }
    return file;
}

bool Basic::loadProgram(const char* filenameUtf8) {
    std::string path(filenameUtf8);
    return loadProgram(path);
}

#if defined(_WIN32) || defined(_WIN64)
    /* We are on Windows */
    #define strtok_r strtok_s
#endif

bool Basic::loadProgram(std::string& inOutFilenameUtf8) {
    std::string foundname = findFirstFileNameWildcard(inOutFilenameUtf8);

    if (foundname != inOutFilenameUtf8) {
        printUtf8String("FOUND " + foundname + " \n");
        inOutFilenameUtf8 = foundname;
    }

    FILE* file = fopenUtf8(foundname, "rb");
    if (file == nullptr) {
        return false;
    }

    doNEW();

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);

    // strip optional utf8 BOM
    uint8_t bom[4] = { 0, 0, 0, 0 };
    fseek(file, 0, SEEK_SET);
    if (length > 2) {
        auto nr = fread(bom, 3, 1, file);
    }
    if (bom[0] == 0xef && bom[1] == 0xbb && bom[2] == 0xbf) {
        fseek(file, 3, SEEK_SET);
        length -= 3;
    } else {
        fseek(file, 0, SEEK_SET);
    }

    char* buff = new char[length + 1];
    if (buff == nullptr) {
        fclose(file);
        return false;
    }

    fread(buff, length, 1, file);
    buff[length] = '\0';
    fclose(file);
    file = nullptr;

    // Establish string and get the first token:
    char* next_token = nullptr;
    char* line       = strtok_r(buff, "\r\n", &next_token);

    std::string str;
    auto& listmodule = moduleListingStack.back()->second;
    int iline        = 0;
    bool rv          = true;
    while (line != NULL) {
        ++iline;
        str = line;
        uppercaseProgram(str);

        const char* pc = str.c_str();
        int64_t n      = 0;
        if (parseInt(pc, &n) && n >= 0) {
            iline                      = int(n);
            listmodule.listing[int(n)] = skipWhite(pc);
            // while (!tokenize().empty()) {} // check sanity, but don't execute
        } else {
            listmodule.listing[iline] = skipWhite(pc);
        }
        line = strtok_r(NULL, "\r\n", &next_token);
    }
    listmodule.setProgramCounterToEnd();

    delete[] buff;
    buff = nullptr;

    return rv;
}

bool Basic::saveProgram(std::string filenameUtf8) {
    FILE* file = fopenUtf8(filenameUtf8, "wb");
    if (file == nullptr) {
        return false;
    }

    for (auto& ln : currentModule().listing) {
        if (ln.first < 0) {
            continue;
        }
        fprintf(file, "%d %s\r\n", ln.first, ln.second.c_str());
    }
    fclose(file);
    file = nullptr;
    return true;
}

#include <filesystem>
bool Basic::fileExists(const std::string& filenameUtf8, bool allowWildCard) {
    if (allowWildCard) {
        std::filesystem::path p(findFirstFileNameWildcard(filenameUtf8));
        return std::filesystem::exists(p);
    }
    std::filesystem::path p(filenameUtf8);
    return std::filesystem::exists(p);
}
