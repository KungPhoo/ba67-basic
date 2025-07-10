#include "prg_tool.h"
#include <cstdint>
#include <sstream>
#include <map>
#include "petscii.h"
#include "unicode.h"
#include "string_helper.h"

static uint16_t getword(const uint8_t*& p) {
    int a = *p++;
    int b = *p++;
    return uint16_t(a | (b << 8));
}

static const char* gettoken(const uint8_t*& prg) {
    static std::map<int, const char*> tokens = {
        { 0x80,      "END" },
        { 0x81,      "FOR" },
        { 0x82,     "NEXT" },
        { 0x83,     "DATA" },
        { 0x84,   "INPUT#" },
        { 0x85,    "INPUT" },
        { 0x86,      "DIM" },
        { 0x87,     "READ" },
        { 0x88,      "LET" },
        { 0x89,     "GOTO" },
        { 0x8A,      "RUN" },
        { 0x8B,       "IF" },
        { 0x8C,  "RESTORE" },
        { 0x8D,    "GOSUB" },
        { 0x8E,   "RETURN" },
        { 0x8F,      "REM" },
        { 0x90,     "STOP" },
        { 0x91,       "ON" },
        { 0x92,     "WAIT" },
        { 0x93,     "LOAD" },
        { 0x94,     "SAVE" },
        { 0x95,   "VERIFY" },
        { 0x96,      "DEF" },
        { 0x97,     "POKE" },
        { 0x98,   "PRINT#" },
        { 0x99,    "PRINT" },
        { 0x9A,     "CONT" },
        { 0x9B,     "LIST" },
        { 0x9C,      "CLR" },
        { 0x9D,      "CMD" },
        { 0x9E,      "SYS" },
        { 0x9F,     "OPEN" },
        { 0xA0,    "CLOSE" },
        { 0xA1,      "GET" },
        { 0xA2,      "NEW" },
        { 0xA3,     "TAB(" },
        { 0xA4,       "TO" },
        { 0xA5,       "FN" },
        { 0xA6,     "SPC(" },
        { 0xA7,     "THEN" },
        { 0xA8,      "NOT" },
        { 0xA9,     "STEP" },
        { 0xAA,        "+" },
        { 0xAB,        "-" },
        { 0xAC,        "*" },
        { 0xAD,        "/" },
        { 0xAE,     "\x5e" }, // power symbol
        { 0xAF,      "AND" },
        { 0xB0,       "OR" },
        { 0xB1,        ">" },
        { 0xB2,        "=" },
        { 0xB3,        "<" },
        { 0xB4,      "SGN" },
        { 0xB5,      "INT" },
        { 0xB6,      "ABS" },
        { 0xB7,      "USR" },
        { 0xB8,      "FRE" },
        { 0xB9,      "POS" },
        { 0xBA,      "SQR" },
        { 0xBB,      "RND" },
        { 0xBC,      "LOG" },
        { 0xBD,      "EXP" },
        { 0xBE,      "COS" },
        { 0xBF,      "SIN" },
        { 0xC0,      "TAN" },
        { 0xC1,      "ATN" },
        { 0xC2,     "PEEK" },
        { 0xC3,      "LEN" },
        { 0xC4,     "STR$" },
        { 0xC5,      "VAL" },
        { 0xC6,      "ASC" },
        { 0xC7,     "CHR$" },
        { 0xC8,    "LEFT$" },
        { 0xC9,   "RIGHT$" },
        { 0xCA,     "MID$" },
        { 0xCB,       "GO" },

        // Commodore 128, BASIC V7.0
        { 0xcc,      "RGR" },
        { 0xcd,      "RCL" },
        // 0xce     PREFIX
        { 0xcf,      "JOY" },
        { 0xd0,     "RDOT" },
        { 0xd1,      "DEC" },
        { 0xd2,     "HEX$" },
        { 0xd3,     "ERR$" },
        { 0xd4,    "INSTR" },
        { 0xd5,     "ELSE" },
        { 0xd6,   "RESUME" },
        { 0xd7,     "TRAP" },
        { 0xd8,     "TRON" },
        { 0xd9,    "TROFF" },
        { 0xda,    "SOUND" },
        { 0xdb,      "VOL" },
        { 0xdc,     "AUTO" },
        { 0xdd,    "PUDEF" },
        { 0xde,  "GRAPHIC" },
        { 0xdf,    "PAINT" },
        { 0xe0,     "CHAR" },
        { 0xe1,      "BOX" },
        { 0xe2,   "CIRCLE" },
        { 0xe3,   "GSHAPE" },
        { 0xe4,   "SSHAPE" },
        { 0xe5,     "DRAW" },
        { 0xe6,   "LOCATE" },
        { 0xe7,    "COLOR" },
        { 0xe8,   "SCNCLR" },
        { 0xe9,    "SCALE" },
        { 0xea,     "HELP" },
        { 0xeb,       "DO" },
        { 0xec,     "LOOP" },
        { 0xed,     "EXIT" },
        { 0xee, "DIRECTOY" },
        { 0xef,    "DSAVE" },
        { 0xf0,    "DLOAD" },
        { 0xf1,   "HEADER" },
        { 0xf2,  "SCRATCH" },
        { 0xf3,  "COLLECT" },
        { 0xf4,     "COPY" },
        { 0xf5,   "RENAME" },
        { 0xf6,   "BACKUP" },
        { 0xf7,   "DELETE" },
        { 0xf8, "RENUMBER" },
        { 0xf9,      "KEY" },
        { 0xfa,  "MONITOR" },
        { 0xfb,    "USING" },
        { 0xfc,    "UNTIL" },
        { 0xfd,    "WHILE" },
        // 0xfe PREFIX
    };

    // with 0xfe PREFIX:
    static std::map<int, const char*> tokensExtFE = {
        { 0x1f, "STASH" },
        { 0x21, "FETCH" },
        { 0x23,  "SWAP" },
    };


    if (*prg == 0xfe) {
        ++prg;
        auto it = tokensExtFE.find(int(*prg));
        if (it != tokensExtFE.end()) {
            return it->second;
        }
    }
    auto it = tokens.find(int(*prg));
    if (it != tokens.end()) {
        return it->second;
    }
    return "?UNDEFINED?";
}

std::string PrgTool::PRGtoBASIC(const uint8_t* prgBytes) {


    uint16_t startAddress = getword(prgBytes);
    if (startAddress != 0x0801) {
        return "?BAD START ADDRESS ERROR";
    }

    std::stringstream str;
    const uint8_t* p = prgBytes;
    for (;;) {
        uint16_t nextLineNo = getword(p);
        if (nextLineNo == 0) {
            break;
        }
        uint16_t lineNo = getword(p);
        str << lineNo << ' ';

        bool isData   = false;
        bool quote    = false;
        bool hasSpace = true;
        for (; *p != '\0'; ++p) {

            if (StringHelper::strncmp((const char*)(p), "GRO", 3) == 0) {
                int asd = 0;
            }


            uint8_t c = *p;

            if (!quote && c == ':') {
                isData = false;
            }

            if (c == 0x22) {
                if (quote) {
                    str << "\" ";
                } else {
                    str << " \"";
                }
                quote    = !quote;
                hasSpace = true;
            } else if (!isData && !quote && (c & 0x80)) {
                bool isGO         = (*p == 0xcb);
                const char* token = gettoken(p);
                if (StringHelper::strcmp(token, "DATA") == 0) {
                    isData = true;
                }

                if (!hasSpace && token[0] >= 'A' && token[0] <= 'Z') {
                    str << ' ';
                }

                str << token;
                char lastChar = token[StringHelper::strlen(token) - 1];
                if (lastChar >= 'A' && lastChar <= 'Z' && !isGO) {
                    str << " ";
                }
                hasSpace = true;
            } else if (quote || isData) {
                if (c == 0xcc) {
                    int pause = 1;
                }
                char32_t s32[2] = { PETSCII::toUnicode(c), 0 };
                str << Unicode::toUtf8String(s32);
            } else {
                str << c;
                hasSpace = (c == ' ');
            }
        }
        ++p; // line end
        str << '\n';
    }
    return str.str();
}
