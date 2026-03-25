// Full C64 6502 Emulator with all documented opcodes

// https://www.masswerk.at/6502/6502_instruction_set.html

#include "cpu-6502.h"
#include <functional>
#include <map>
#include <cstring>
#include <bitset>

void CPU6502::reset() {
    cpuJam = false;
    A = X = Y         = 0;
    SP                = 0xFF;
    PC                = readWord(0xFFFC); // Reset vector
    P                 = 0x24; // IRQ disabled by default
    cycleCount        = 0;
    lastCycleSnapshot = 0;
    heatmap.clear();
}

bool CPU6502::sys(uint16_t address) {
    RAM[0x01FF] = 0x08; // High byte of 0x080C
    RAM[0x01FE] = 0x0C; // Low byte of 0x080C
    SP          = 0xFD; // Stack pointer (points to last used position)
    setPC(address);
    return true;
}


enum Opcode : uint8_t {
    // incrementing
    // Register increments/decrements
    DEX = 0xCA,
    DEY = 0x88,
    INX = 0xE8,
    INY = 0xC8,

    // Memory increments
    INC_ZP   = 0xE6,
    INC_ZPX  = 0xF6,
    INC_ABS  = 0xEE,
    INC_ABSX = 0xFE,

    // Memory decrements
    DEC_ZP   = 0xC6,
    DEC_ZPX  = 0xD6,
    DEC_ABS  = 0xCE,
    DEC_ABSX = 0xDE,

    // Load/Store
    LDA_IMM  = 0xA9,
    LDA_ZP   = 0xA5,
    LDA_ZPX  = 0xB5,
    LDA_ABS  = 0xAD,
    LDA_ABSX = 0xBD,
    LDA_ABSY = 0xB9,
    LDA_INDX = 0xA1,
    LDA_INDY = 0xB1,
    LDX_IMM  = 0xA2,
    LDX_ZP   = 0xA6,
    LDX_ZPY  = 0xB6,
    LDX_ABS  = 0xAE,
    LDX_ABSY = 0xBE,
    LDY_IMM  = 0xA0,
    LDY_ZP   = 0xA4,
    LDY_ZPX  = 0xB4,
    LDY_ABS  = 0xAC,
    LDY_ABSX = 0xBC,

    STA_ZP   = 0x85,
    STA_ZPX  = 0x95,
    STA_ABS  = 0x8D,
    STA_ABSX = 0x9D,
    STA_ABSY = 0x99,
    STA_INDX = 0x81,
    STA_INDY = 0x91,
    STX_ZP   = 0x86,
    STX_ZPY  = 0x96,
    STX_ABS  = 0x8E,
    STY_ZP   = 0x84,
    STY_ZPX  = 0x94,
    STY_ABS  = 0x8C,

    // Jumps & Subroutines
    JMP_ABS = 0x4C,
    JMP_IND = 0x6C,
    JSR     = 0x20,
    RTS     = 0x60,
    BRK     = 0x00,
    RTI     = 0x40,
    NOP     = 0xEA,

    // Logical
    BIT_ZP  = 0x24,
    BIT_ABS = 0x2C,

    // AND (Accumulator AND memory)
    AND_IMM  = 0x29, // Immediate
    AND_ZP   = 0x25, // Zero Page
    AND_ZPX  = 0x35, // Zero Page,X
    AND_ABS  = 0x2D, // Absolute
    AND_ABSX = 0x3D, // Absolute,X
    AND_ABSY = 0x39, // Absolute,Y
    AND_INDX = 0x21, // (Zero Page,X)
    AND_INDY = 0x31, // (Zero Page),Y

    // ORA (Accumulator OR memory)
    ORA_IMM  = 0x09, // Immediate
    ORA_ZP   = 0x05, // Zero Page
    ORA_ZPX  = 0x15, // Zero Page,X
    ORA_ABS  = 0x0D, // Absolute
    ORA_ABSX = 0x1D, // Absolute,X
    ORA_ABSY = 0x19, // Absolute,Y
    ORA_INDX = 0x01, // (Zero Page,X)
    ORA_INDY = 0x11, // (Zero Page),Y

    // EOR (Exclusive OR with Accumulator)
    EOR_IMM  = 0x49, // Immediate
    EOR_ZP   = 0x45, // Zero Page
    EOR_ZPX  = 0x55, // Zero Page,X
    EOR_ABS  = 0x4D, // Absolute
    EOR_ABSX = 0x5D, // Absolute,X
    EOR_ABSY = 0x59, // Absolute,Y
    EOR_INDX = 0x41, // (Zero Page,X)
    EOR_INDY = 0x51, // (Zero Page),Y

    // add with carry
    ADC_IMM  = 0x69,
    ADC_ZP   = 0x65,
    ADC_ZPX  = 0x75,
    ADC_ABS  = 0x6D,
    ADC_ABSX = 0x7D,
    ADC_ABSY = 0x79,
    ADC_INDX = 0x61,
    ADC_INDY = 0x71,

    // subtract with carry
    SBC_IMM  = 0xE9,
    SBC_ZP   = 0xE5,
    SBC_ZPX  = 0xF5,
    SBC_ABS  = 0xED,
    SBC_ABSX = 0xFD,
    SBC_ABSY = 0xF9,
    SBC_INDX = 0xE1,
    SBC_INDY = 0xF1,


    // Comparison
    // CMP (Compare A)
    CMP_IMM  = 0xC9,
    CMP_ZP   = 0xC5,
    CMP_ZPX  = 0xD5,
    CMP_ABS  = 0xCD,
    CMP_ABSX = 0xDD,
    CMP_ABSY = 0xD9,
    CMP_INDX = 0xC1, // (zp,X)
    CMP_INDY = 0xD1, // (zp),Y

    // CPX (Compare X)
    CPX_IMM = 0xE0,
    CPX_ZP  = 0xE4,
    CPX_ABS = 0xEC,

    // CPY (Compare Y)
    CPY_IMM = 0xC0,
    CPY_ZP  = 0xC4,
    CPY_ABS = 0xCC,



    // Shifts
    // ASL (Arithmetic Shift Left)
    ASL_ACC  = 0x0A,
    ASL_ZP   = 0x06,
    ASL_ZPX  = 0x16,
    ASL_ABS  = 0x0E,
    ASL_ABSX = 0x1E,

    // LSR (Logical Shift Right)
    LSR_ACC  = 0x4A,
    LSR_ZP   = 0x46,
    LSR_ZPX  = 0x56,
    LSR_ABS  = 0x4E,
    LSR_ABSX = 0x5E,

    // ROL (Rotate Left)
    ROL_ACC  = 0x2A,
    ROL_ZP   = 0x26,
    ROL_ZPX  = 0x36,
    ROL_ABS  = 0x2E,
    ROL_ABSX = 0x3E,

    // ROR (Rotate Right)
    ROR_ACC  = 0x6A,
    ROR_ZP   = 0x66,
    ROR_ZPX  = 0x76,
    ROR_ABS  = 0x6E,
    ROR_ABSX = 0x7E,
    // Branches
    BCC = 0x90,
    BCS = 0xB0,
    BMI = 0x30,
    BPL = 0x10,
    BVC = 0x50,
    BVS = 0x70,
    BEQ = 0xF0,
    BNE = 0xD0,

    // Flag ops
    CLC = 0x18,
    SEC = 0x38,
    CLI = 0x58,
    SEI = 0x78,
    CLV = 0xB8,
    CLD = 0xD8,
    SED = 0xF8,

    // Stack ops
    PHA = 0x48,
    PHP = 0x08,
    PLA = 0x68,
    PLP = 0x28,

    // TRANSFER INSTRUCTIONS
    TAX = 0xAA,
    TAY = 0xA8,
    TXA = 0x8A,
    TYA = 0x98,
    TSX = 0xBA,
    TXS = 0x9A
};

const char* OpCodeName(uint8_t op) {
    static const char* nm[256] = {};
    if (nm[0] == nullptr) {
        nm[0x00] = "BRK";
        nm[0x01] = "ORA_INDX";
        nm[0x05] = "ORA_ZP";
        nm[0x06] = "ASL_ZP";
        nm[0x08] = "PHP";
        nm[0x09] = "ORA_IMM";
        nm[0x0A] = "ASL_ACC";
        nm[0x0D] = "ORA_ABS";
        nm[0x0E] = "ASL_ABS";
        nm[0x10] = "BPL";
        nm[0x11] = "ORA_INDY";
        nm[0x15] = "ORA_ZPX";
        nm[0x16] = "ASL_ZPX";
        nm[0x18] = "CLC";
        nm[0x19] = "ORA_ABSY";
        nm[0x1D] = "ORA_ABSX";
        nm[0x1E] = "ASL_ABSX";
        nm[0x20] = "JSR";
        nm[0x21] = "AND_INDX";
        nm[0x24] = "BIT_ZP";
        nm[0x25] = "AND_ZP";
        nm[0x26] = "ROL_ZP";
        nm[0x28] = "PLP";
        nm[0x29] = "AND_IMM";
        nm[0x2A] = "ROL_ACC";
        nm[0x2C] = "BIT_ABS";
        nm[0x2D] = "AND_ABS";
        nm[0x2E] = "ROL_ABS";
        nm[0x30] = "BMI";
        nm[0x31] = "AND_INDY";
        nm[0x35] = "AND_ZPX";
        nm[0x36] = "ROL_ZPX";
        nm[0x38] = "SEC";
        nm[0x39] = "AND_ABSY";
        nm[0x3D] = "AND_ABSX";
        nm[0x3E] = "ROL_ABSX";
        nm[0x40] = "RTI";
        nm[0x41] = "EOR_INDX";
        nm[0x45] = "EOR_ZP";
        nm[0x46] = "LSR_ZP";
        nm[0x48] = "PHA";
        nm[0x49] = "EOR_IMM";
        nm[0x4A] = "LSR_ACC";
        nm[0x4C] = "JMP_ABS";
        nm[0x4D] = "EOR_ABS";
        nm[0x4E] = "LSR_ABS";
        nm[0x50] = "BVC";
        nm[0x51] = "EOR_INDY";
        nm[0x55] = "EOR_ZPX";
        nm[0x56] = "LSR_ZPX";
        nm[0x58] = "CLI";
        nm[0x59] = "EOR_ABSY";
        nm[0x5D] = "EOR_ABSX";
        nm[0x5E] = "LSR_ABSX";
        nm[0x60] = "RTS";
        nm[0x61] = "ADC_INDX";
        nm[0x65] = "ADC_ZP";
        nm[0x66] = "ROR_ZP";
        nm[0x68] = "PLA";
        nm[0x69] = "ADC_IMM";
        nm[0x6A] = "ROR_ACC";
        nm[0x6C] = "JMP_IND";
        nm[0x6D] = "ADC_ABS";
        nm[0x6E] = "ROR_ABS";
        nm[0x70] = "BVS";
        nm[0x71] = "ADC_INDY";
        nm[0x75] = "ADC_ZPX";
        nm[0x76] = "ROR_ZPX";
        nm[0x78] = "SEI";
        nm[0x79] = "ADC_ABSY";
        nm[0x7D] = "ADC_ABSX";
        nm[0x7E] = "ROR_ABSX";
        nm[0x81] = "STA_INDX";
        nm[0x84] = "STY_ZP";
        nm[0x85] = "STA_ZP";
        nm[0x86] = "STX_ZP";
        nm[0x88] = "DEY";
        nm[0x8A] = "TXA";
        nm[0x8C] = "STY_ABS";
        nm[0x8D] = "STA_ABS";
        nm[0x8E] = "STX_ABS";
        nm[0x90] = "BCC";
        nm[0x91] = "STA_INDY";
        nm[0x94] = "STY_ZPX";
        nm[0x95] = "STA_ZPX";
        nm[0x96] = "STX_ZPY";
        nm[0x98] = "TYA";
        nm[0x99] = "STA_ABSY";
        nm[0x9A] = "TXS";
        nm[0x9D] = "STA_ABSX";
        nm[0xA0] = "LDY_IMM";
        nm[0xA1] = "LDA_INDX";
        nm[0xA2] = "LDX_IMM";
        nm[0xA4] = "LDY_ZP";
        nm[0xA5] = "LDA_ZP";
        nm[0xA6] = "LDX_ZP";
        nm[0xA8] = "TAY";
        nm[0xA9] = "LDA_IMM";
        nm[0xAA] = "TAX";
        nm[0xAC] = "LDY_ABS";
        nm[0xAD] = "LDA_ABS";
        nm[0xAE] = "LDX_ABS";
        nm[0xB0] = "BCS";
        nm[0xB1] = "LDA_INDY";
        nm[0xB4] = "LDY_ZPX";
        nm[0xB5] = "LDA_ZPX";
        nm[0xB6] = "LDX_ZPY";
        nm[0xB8] = "CLV";
        nm[0xB9] = "LDA_ABSY";
        nm[0xBA] = "TSX";
        nm[0xBC] = "LDY_ABSX";
        nm[0xBD] = "LDA_ABSX";
        nm[0xBE] = "LDX_ABSY";
        nm[0xC0] = "CPY_IMM";
        nm[0xC1] = "CMP_INDX";
        nm[0xC4] = "CPY_ZP";
        nm[0xC5] = "CMP_ZP";
        nm[0xC6] = "DEC_ZP";
        nm[0xC8] = "INY";
        nm[0xC9] = "CMP_IMM";
        nm[0xCA] = "DEX";
        nm[0xCC] = "CPY_ABS";
        nm[0xCD] = "CMP_ABS";
        nm[0xCE] = "DEC_ABS";
        nm[0xD0] = "BNE";
        nm[0xD1] = "CMP_INDY";
        nm[0xD5] = "CMP_ZPX";
        nm[0xD6] = "DEC_ZPX";
        nm[0xD8] = "CLD";
        nm[0xD9] = "CMP_ABSY";
        nm[0xDD] = "CMP_ABSX";
        nm[0xDE] = "DEC_ABSX";
        nm[0xE0] = "CPX_IMM";
        nm[0xE1] = "SBC_INDX";
        nm[0xE4] = "CPX_ZP";
        nm[0xE5] = "SBC_ZP";
        nm[0xE6] = "INC_ZP";
        nm[0xE8] = "INX";
        nm[0xE9] = "SBC_IMM";
        nm[0xEA] = "NOP";
        nm[0xEC] = "CPX_ABS";
        nm[0xED] = "SBC_ABS";
        nm[0xEE] = "INC_ABS";
        nm[0xF0] = "BEQ";
        nm[0xF1] = "SBC_INDY";
        nm[0xF5] = "SBC_ZPX";
        nm[0xF6] = "INC_ZPX";
        nm[0xF8] = "SED";
        nm[0xF9] = "SBC_ABSY";
        nm[0xFD] = "SBC_ABSX";
        nm[0xFE] = "INC_ABSX";
    }

    const char* c = nm[op];
    if (c == nullptr) {
        c = "????";
    }
    return c;
}

const char* kernalRoutineName(uint16_t PC) {
    static std::map<uint16_t, std::string> jt;
    if (jt.empty()) {
        jt[0x0073] = "BASIC_GET_NEXT_TEXT_CHARACTER";

        // $A000...$BFFF BASIC
        jt[0xA43A] = "BASIC_ERR_MSG";
        jt[0xB78B] = "BASIC_ASC";
        jt[0xB79B] = "BASIC_ASC(get byte to X)";
        jt[0xB7AD] = "BASIC_VAL";


        // $E000...$FFFF kernal routines
        jt[0xE500] = "IOBASE";
        jt[0xE505] = "SCREEN";
        jt[0xE50A] = "PLOT";
        jt[0xE716] = "PUT_ONE_CHAR_TO_SCREEN";
        jt[0xE9C5] = "MOVE_ONE_LINE";
        jt[0xE9F0] = "SETPNT to line X";
        jt[0xE9FF] = "CLEAR_SCREEN_LINE_X";
        jt[0xEA13] = "PUT_CHAR_A_COL_X";
        jt[0xEA31] = "IRQ_MAIN";
        jt[0xEA87] = "SCNKEY";
        jt[0xED09] = "TALK";
        jt[0xED0C] = "LISTEN";
        jt[0xEDB9] = "SECOND";
        jt[0xEDC7] = "TKSA";
        jt[0xEDDD] = "CIOUT";
        jt[0xEDEF] = "UNTLK";
        jt[0xEDFE] = "UNLSN";
        jt[0xEE13] = "ACPTR";
        jt[0xF13E] = "GETIN";
        jt[0xF157] = "CHRIN";
        jt[0xF1CA] = "CHROUT";
        jt[0xF20E] = "CHKIN ";
        jt[0xF250] = "CHKOUT";
        jt[0xF291] = "CLOSE";
        jt[0xF32F] = "CLALL";
        jt[0xF333] = "CLRCHN";
        jt[0xF34A] = "OPEN";
        jt[0xF49E] = "LOAD";
        jt[0xF5DD] = "SAVE";
        jt[0xF69B] = "UDTIM";
        jt[0xF6DD] = "RDTIM";
        jt[0xF6E4] = "SETTIM";
        jt[0xF6ED] = "STOP";
        jt[0xFD15] = "RESTOR";
        jt[0xFD1A] = "VECTOR";
        jt[0xFD50] = "RAMTAS";
        jt[0xFDA3] = "IOINIT";
        jt[0xFDF9] = "SETNAM";
        jt[0xFE00] = "SETLFS";
        jt[0xFE07] = "READST";
        jt[0xFE18] = "SETMSG";
        jt[0xFE21] = "SETTMO";
        jt[0xFE25] = "MEMTOP";
        jt[0xFE34] = "MEMBOT";
        jt[0xFF5B] = "CINT";

        // jump table
        jt[0xFF81] = "CINT"; // init screen editor
        jt[0xFF84] = "IOINT"; // init input/output
        jt[0xFF87] = "RAMTAS"; // init RAM, tape screen
        jt[0xFF8A] = "RESTOR"; // restore default I/O vector
        jt[0xFF8D] = "VECTOR"; // read/set I/O vector
        jt[0xFF90] = "SETMSG"; // control KERNAL messages
        jt[0xFF93] = "SECOND"; // send SA after LISTEN
        jt[0xFF96] = "TKSA"; // send SA after TALK
        jt[0xFF99] = "MEMTOP"; // read/set top of memory
        jt[0xFF9C] = "MEMBOT"; // read/set bottom of memory
        jt[0xFF9F] = "SCNKEY"; // scan keyboard
        jt[0xFFA2] = "SETTMO"; // set IEEE timeout
        jt[0xFFA5] = "ACPTR"; // input byte from serial bus
        jt[0xFFA8] = "CIOUT"; // output byte to serial bus
        jt[0xFFAB] = "UNTALK"; // command serial bus UNTALK
        jt[0xFFAE] = "UNLSN"; // command serial bus UNLSN
        jt[0xFFB1] = "LISTEN"; // command serial bus LISTEN
        jt[0xFFB4] = "TALK"; // command serial bus TALK
        jt[0xFFB7] = "READST"; // read I/O status word
        jt[0xFFBA] = "SETLFS"; // set logical file parameters
        jt[0xFFBD] = "SETNAM"; // set filename
        jt[0xFFC0] = "OPEN"; // open file
        jt[0xFFC3] = "CLOSE"; // close file
        jt[0xFFC6] = "CHKIN"; // prepare channel for input
        jt[0xFFC9] = "CHKOUT"; // prepare channel for output
        jt[0xFFCC] = "CLRCHN"; // close all I/O
        jt[0xFFCF] = "CHRIN"; // input byte from channel
        jt[0xFFD2] = "CHROUT"; // output byte to channel
        jt[0xFFD5] = "LOAD"; // load from serial device
        jt[0xFFD8] = "SAVE"; // save to serial device
        jt[0xFFDB] = "SETTIM"; // set realtime clock
        jt[0xFFDE] = "RDTIM"; // read realtime clock
        jt[0xFFE1] = "STOP"; // check <STOP> key
        jt[0xFFE4] = "GETIN"; // get input from keyboard
        jt[0xFFE7] = "CLALL"; // close all files and channels
        jt[0xFFEA] = "UDTIM"; // increment realtime clock
        jt[0xFFED] = "SCREEN"; // return screen organisation
        jt[0xFFF0] = "PLOT"; // read/set cursor X/Y position
        jt[0xFFF3] = "IOBASE"; // return IOBASE address
    }
    if (jt.find(PC) != jt.end()) {
        return jt[PC].c_str();
    }
    return nullptr;
}


const CPU6502::OpCodeInfo& CPU6502::getOpcodeInfo(uint8_t op) {
    static CPU6502::OpCodeInfo info[256] = {};
    if (info[0].mnemonic == nullptr || info[0].length != 1) {
        auto add = [&](int op, const char* mn, AddrMode ops, int c, int cycles) {
            auto& iop    = info[op];
            iop.mnemonic = mn;
            iop.admode   = ops;
            iop.length   = uint8_t(c);
            iop.cycles   = uint8_t(cycles);
        };
        add(0x00, "BRK", IMP, 1, 7);
        add(0x01, "ORA", INDX, 2, 6);
        add(0x05, "ORA", ZPG, 2, 3);
        add(0x06, "ASL", ZPG, 2, 5);
        add(0x08, "PHP", IMP, 1, 3);
        add(0x09, "ORA", IMM, 2, 2);
        add(0x0a, "ASL", ACC, 1, 2);
        add(0x0d, "ORA", ABS, 3, 4);
        add(0x0e, "ASL", ABS, 3, 6);
        add(0x10, "BPL", REL, 2, 2);
        add(0x11, "ORA", INDY, 2, 5);
        add(0x15, "ORA", ZPX, 2, 4);
        add(0x16, "ASL", ZPX, 2, 6);
        add(0x18, "CLC", IMP, 1, 2);
        add(0x19, "ORA", ABSY, 3, 4);
        add(0x1d, "ORA", ABSX, 3, 4);
        add(0x1e, "ASL", ABSX, 3, 7);
        add(0x20, "JSR", ABS, 3, 6);
        add(0x21, "AND", INDX, 2, 6);
        add(0x24, "BIT", ZPG, 2, 3);
        add(0x25, "AND", ZPG, 2, 3);
        add(0x26, "ROL", ZPG, 2, 5);
        add(0x28, "PLP", IMP, 1, 4);
        add(0x29, "AND", IMM, 2, 2);
        add(0x2a, "ROL", ACC, 1, 2);
        add(0x2c, "BIT", ABS, 3, 4);
        add(0x2d, "AND", ABS, 3, 4);
        add(0x2e, "ROL", ABS, 3, 6);
        add(0x30, "BMI", REL, 2, 2);
        add(0x31, "AND", INDY, 2, 5);
        add(0x35, "AND", ZPX, 2, 4);
        add(0x36, "ROL", ZPX, 2, 6);
        add(0x38, "SEC", IMP, 1, 2);
        add(0x39, "AND", ABSY, 3, 4);
        add(0x3d, "AND", ABSX, 3, 4);
        add(0x3e, "ROL", ABSX, 3, 7);
        add(0x40, "RTI", IMP, 1, 6);
        add(0x41, "EOR", INDX, 2, 6);
        add(0x45, "EOR", ZPG, 2, 3);
        add(0x46, "LSR", ZPG, 2, 5);
        add(0x48, "PHA", IMP, 1, 3);
        add(0x49, "EOR", IMM, 2, 2);
        add(0x4a, "LSR", ACC, 1, 2);
        add(0x4c, "JMP", ABS, 3, 3);
        add(0x4d, "EOR", ABS, 3, 4);
        add(0x4e, "LSR", ABS, 3, 6);
        add(0x50, "BVC", REL, 2, 2);
        add(0x51, "EOR", INDY, 2, 5);
        add(0x55, "EOR", ZPX, 2, 4);
        add(0x56, "LSR", ZPX, 2, 6);
        add(0x58, "CLI", IMP, 1, 2);
        add(0x59, "EOR", ABSY, 3, 4);
        add(0x5d, "EOR", ABSX, 3, 4);
        add(0x5e, "LSR", ABSX, 3, 7);
        add(0x60, "RTS", IMP, 1, 6);
        add(0x61, "ADC", INDX, 2, 6);
        add(0x65, "ADC", ZPG, 2, 3);
        add(0x66, "ROR", ZPG, 2, 5);
        add(0x68, "PLA", IMP, 1, 4);
        add(0x69, "ADC", IMM, 2, 2);
        add(0x6a, "ROR", ACC, 1, 2);
        add(0x6c, "JMP", IND, 3, 5);
        add(0x6d, "ADC", ABS, 3, 4);
        add(0x6e, "ROR", ABS, 3, 6);
        add(0x70, "BVS", REL, 2, 2);
        add(0x71, "ADC", INDY, 2, 5);
        add(0x75, "ADC", ZPX, 2, 4);
        add(0x76, "ROR", ZPX, 2, 6);
        add(0x78, "SEI", IMP, 1, 2);
        add(0x79, "ADC", ABSY, 3, 4);
        add(0x7d, "ADC", ABSX, 3, 4);
        add(0x7e, "ROR", ABSX, 3, 7);
        add(0x81, "STA", INDX, 2, 6);
        add(0x84, "STY", ZPG, 2, 3);
        add(0x85, "STA", ZPG, 2, 3);
        add(0x86, "STX", ZPG, 2, 3);
        add(0x88, "DEY", IMP, 1, 2);
        add(0x8a, "TXA", IMP, 1, 2);
        add(0x8c, "STY", ABS, 3, 4);
        add(0x8d, "STA", ABS, 3, 4);
        add(0x8e, "STX", ABS, 3, 4);
        add(0x90, "BCC", REL, 2, 2);
        add(0x91, "STA", INDY, 2, 6);
        add(0x94, "STY", ZPX, 2, 4);
        add(0x95, "STA", ZPX, 2, 4);
        add(0x96, "STX", ZPY, 2, 4);
        add(0x98, "TYA", IMP, 1, 2);
        add(0x99, "STA", ABSY, 3, 5);
        add(0x9a, "TXS", IMP, 1, 2);
        add(0x9d, "STA", ABSX, 3, 5);
        add(0xa0, "LDY", IMM, 2, 2);
        add(0xa1, "LDA", INDX, 2, 6);
        add(0xa2, "LDX", IMM, 2, 2);
        add(0xa4, "LDY", ZPG, 2, 3);
        add(0xa5, "LDA", ZPG, 2, 3);
        add(0xa6, "LDX", ZPG, 2, 3);
        add(0xa8, "TAY", IMP, 1, 2);
        add(0xa9, "LDA", IMM, 2, 2);
        add(0xaa, "TAX", IMP, 1, 2);
        add(0xac, "LDY", ABS, 3, 4);
        add(0xad, "LDA", ABS, 3, 4);
        add(0xae, "LDX", ABS, 3, 4);
        add(0xb0, "BCS", REL, 2, 2);
        add(0xb1, "LDA", INDY, 2, 5);
        add(0xb4, "LDY", ZPX, 2, 4);
        add(0xb5, "LDA", ZPX, 2, 4);
        add(0xb6, "LDX", ZPY, 2, 4);
        add(0xb8, "CLV", IMP, 1, 2);
        add(0xb9, "LDA", ABSY, 3, 4);
        add(0xba, "TSX", IMP, 1, 2);
        add(0xbc, "LDY", ABSX, 3, 4);
        add(0xbd, "LDA", ABSX, 3, 4);
        add(0xbe, "LDX", ABSY, 3, 4);
        add(0xc0, "CPY", IMM, 2, 2);
        add(0xc1, "CMP", INDX, 2, 6);
        add(0xc4, "CPY", ZPG, 2, 3);
        add(0xc5, "CMP", ZPG, 2, 3);
        add(0xc6, "DEC", ZPG, 2, 5);
        add(0xc8, "INY", IMP, 1, 2);
        add(0xc9, "CMP", IMM, 2, 2);
        add(0xca, "DEX", IMP, 1, 2);
        add(0xcc, "CPY", ABS, 3, 4);
        add(0xcd, "CMP", ABS, 3, 4);
        add(0xce, "DEC", ABS, 3, 6);
        add(0xd0, "BNE", REL, 2, 2);
        add(0xd1, "CMP", INDY, 2, 5);
        add(0xd5, "CMP", ZPX, 2, 4);
        add(0xd6, "DEC", ZPX, 2, 6);
        add(0xd8, "CLD", IMP, 1, 2);
        add(0xd9, "CMP", ABSY, 3, 4);
        add(0xdd, "CMP", ABSX, 3, 4);
        add(0xde, "DEC", ABSX, 3, 7);
        add(0xe0, "CPX", IMM, 2, 2);
        add(0xe1, "SBC", INDX, 2, 6);
        add(0xe4, "CPX", ZPG, 2, 3);
        add(0xe5, "SBC", ZPG, 2, 3);
        add(0xe6, "INC", ZPG, 2, 5);
        add(0xe8, "INX", IMP, 1, 2);
        add(0xe9, "SBC", IMM, 2, 2);
        add(0xea, "NOP", IMP, 1, 2);
        add(0xec, "CPX", ABS, 3, 4);
        add(0xed, "SBC", ABS, 3, 4);
        add(0xee, "INC", ABS, 3, 6);
        add(0xf0, "BEQ", REL, 2, 2);
        add(0xf1, "SBC", INDY, 2, 5);
        add(0xf5, "SBC", ZPX, 2, 4);
        add(0xf6, "INC", ZPX, 2, 6);
        add(0xf8, "SED", IMP, 1, 2);
        add(0xf9, "SBC", ABSY, 3, 4);
        add(0xfd, "SBC", ABSX, 3, 4);
        add(0xfe, "INC", ABSX, 3, 7);

        for (int i = 0; i <= 0xff; ++i) {
            if (info[i].mnemonic == nullptr) {
                add(i, "???", INVALID, 1, 1);
            }
        }
    }
    return info[op & 0xff];
}

void CPU6502::updateRasterBeam() {
    size_t rasterFromCycle = cycleCount / 64; // approximately
    rasterFromCycle %= 312;

    if (rasterFromCycle == vic_current_raster) {
        return;
    }

    // this is what the CPU reads at krnl.VIC_RASTER
    vic_current_raster = uint16_t(rasterFromCycle);

    // this is what the CPU writes to krnl.VIC_RASTER
    uint16_t rasterTarget = RAM[krnl.VIC_RASTER]
                          | ((RAM[krnl.VIC_CTRL1] & 0x80) << 1);
    if (rasterTarget == vic_current_raster) {
        RAM[krnl.VIC_IRQ] |= 0x01; // set raster IRQ flag
        updateVicIrq();
    }
}

uint8_t CPU6502::fetchOperand(AddrMode mode) {
    switch (mode) {
    case IMM: return fetchByte();
    case ZPG: {
        uint8_t addr = fetchByte();
        return readByte(addr);
    }
    case ZPX: {
        uint8_t addr = (fetchByte() + X) & 0xFF;
        return readByte(addr);
    }
    case ZPY: {
        uint8_t addr = (fetchByte() + Y) & 0xFF;
        return readByte(addr);
    }
    case ABS: {
        uint16_t addr = fetchWord();
        return readByte(addr);
    }
    case ABSX: {
        uint16_t addr = (fetchWord() + X) & 0xFFFF;
        return readByte(addr);
    }
    case ABSY: {
        uint16_t addr = (fetchWord() + Y) & 0xFFFF;
        return readByte(addr);
    }
    case INDX: {
        uint8_t zp    = (fetchByte() + X) & 0xFF;
        uint16_t addr = readByte(zp) | (readByte(zp + 1) << 8);
        return readByte(addr);
    }
    case INDY: {
        uint8_t zp     = fetchByte();
        uint16_t addr  = readByte(zp) | (readByte(zp + 1) << 8);
        uint16_t addrY = (addr + Y) & 0xFFFF;
        return readByte(addrY);
    }
    default: return 0;
    }
}


// void CPU6502::brk() {
//     PC++;
//     pushWord(PC);
//     push(P | PF_BREAK);
//     P |= PF_INTERRUPT;
//     setPC(readWord(0xFFFE));
// }


void CPU6502::handle_interrupt(uint16_t vector, bool is_brk) {
    pushWord(PC);
    // push((PC >> 8) & 0xFF);
    // push(PC & 0xFF);

    uint8_t status = P;
    if (is_brk) {
        status |= PF_BREAK; // B flag set
    } else {
        status &= ~PF_BREAK; // B flag clear
    }

    push(status);

    P |= PF_INTERRUPT; // Set I flag
    PC = readWord(vector);
}


bool CPU6502::executeNext() {
    if (PC == 0x080C) {
        return false; // back to BASIC
    }


    if (PC == 0xe769) {
        int pause = 1;
    }


    updateRasterBeam();

    const uint16_t NMI_VECTOR = 0xFFFA;
    const uint16_t IRQ_VECTOR = 0xFFFE;
    if (nmiPending) {
        handle_interrupt(NMI_VECTOR, false);
    } else if (irqPending && !(P & PF_INTERRUPT)) {
        handle_interrupt(IRQ_VECTOR, false);
    }

    // pre C++20 versions (no template, but costly)
    // static auto executeOp = [this]<typename Op>(AddrMode mode, Op&& op) ;
    // static auto executeRMW = [this](AddrMode mode, std::function<uint8_t(uint8_t)> op) ;
    static auto executeOp = [this]<typename Op>(AddrMode mode, Op&& op) {
        op(fetchOperand(mode));
    };

    static auto executeRMW = [this]<typename Op>(AddrMode mode, Op&& op) {
        if (mode == IMM) {
            return; // Not used for RMW
        }
        if (mode == ACC) {
            A = op(A);
        } else {
            uint16_t addr = 0;
            switch (mode) {
            case ZPG:  addr = fetchByte(); break;
            case ZPX:  addr = (fetchByte() + X) & 0xFF; break;
            case ABS:  addr = fetchWord(); break;
            case ABSX: addr = (fetchWord() + X) & 0xFFFF; break;
            default:   return;
            }
            setByte(addr, op(readByte(addr)));
        }
    };

    static auto AND = [this](uint8_t value) {
        A &= value;
        setZN(A);
    };

    static auto ORA = [this](uint8_t value) {
        A |= value;
        setZN(A);
    };
    static auto EOR = [this](uint8_t value) {
        A ^= value;
        setZN(A);
    };
    static auto ADC = [this](uint8_t value) {
        uint16_t sum = A + value + (P & PF_CARRY ? 1 : 0);

        // Overflow is always binary
        setFlag(PF_OVERFLOW, (~(A ^ value) & (A ^ sum)) & 0x80);

        if (P & PF_DECIMAL) {
            uint16_t adj = sum;

            if ((adj & 0x0F) > 0x09) {
                adj += 0x06;
            }

            if ((adj & 0xF0) > 0x90) {
                adj += 0x60;
            }

            setFlag(PF_CARRY, adj > 0xFF);
            A = adj & 0xFF;
        } else {
            setFlag(PF_CARRY, sum > 0xFF);
            A = sum & 0xFF;
        }

        setZN(A);
    };
    static auto SBC = [this](uint8_t value) {
        uint16_t diff = A - value - (P & PF_CARRY ? 0 : 1);

        // Overflow is always binary
        setFlag(PF_OVERFLOW, ((A ^ diff) & (A ^ value)) & 0x80);

        if (P & PF_DECIMAL) {
            uint16_t adj = diff;

            if ((adj & 0x0F) > 0x09) {
                adj -= 0x06;
            }

            if ((adj & 0xF0) > 0x90) {
                adj -= 0x60;
            }

            setFlag(PF_CARRY, diff < 0x100); // no borrow
            A = adj & 0xFF;
        } else {
            setFlag(PF_CARRY, diff < 0x100);
            A = diff & 0xFF;
        }

        setZN(A);
    };
    static auto CMP = [this](uint8_t value) {
        uint8_t result = A - value;
        setFlag(PF_CARRY, A >= value);
        setFlag(PF_ZERO, result == 0);
        setFlag(PF_NEGATIVE, result & 0x80);
    };
    static auto CPX = [this](uint8_t value) {
        uint8_t result = X - value;
        setFlag(PF_CARRY, X >= value);
        setFlag(PF_ZERO, result == 0);
        setFlag(PF_NEGATIVE, result & 0x80);
    };

    static auto CPY = [this](uint8_t value) {
        uint8_t result = Y - value;
        setFlag(PF_CARRY, Y >= value);
        setFlag(PF_ZERO, result == 0);
        setFlag(PF_NEGATIVE, result & 0x80);
    };

    static auto ASL = [this](uint8_t value) -> uint8_t {
        setFlag(PF_CARRY, value & 0x80);
        value <<= 1;
        setZN(value);
        return value;
    };

    static auto LSR = [this](uint8_t value) -> uint8_t {
        setFlag(PF_CARRY, value & 0x01);
        value >>= 1;
        setZN(value);
        return value;
    };
    static auto ROL = [this](uint8_t value) -> uint8_t {
        uint8_t oldCarry = (P & PF_CARRY) ? 1 : 0;
        setFlag(PF_CARRY, value & 0x80);
        value = (value << 1) | oldCarry;
        setZN(value);
        return value;
    };
    static auto ROR = [this](uint8_t value) -> uint8_t {
        uint8_t oldCarry = (P & PF_CARRY) ? 0x80 : 0;
        setFlag(PF_CARRY, value & 0x01);
        value = (value >> 1) | oldCarry;
        setZN(value);
        return value;
    };

    static auto LDA = [this](uint8_t value) {
        A = value;
        setZN(value);
    };
    static auto LDX = [this](uint8_t value) {
        X = value;
        setZN(value);
    };
    static auto LDY = [this](uint8_t value) {
        Y = value;
        setZN(value);
    };

    opcode = fetchByte();

    auto& info = getOpcodeInfo(opcode);
    cycleCount += info.cycles;

    // when in BASIC ROM, sample heat-map
    if (PC >= 0xA000 && PC <= 0xBFFF) {
        updateBasicHeatmap();
    }


    switch (opcode) {
    case LDA_IMM:  executeOp(IMM, LDA); break;
    case LDA_ZP:   executeOp(ZPG, LDA); break;
    case LDA_ZPX:  executeOp(ZPX, LDA); break;
    case LDA_ABS:  executeOp(ABS, LDA); break;
    case LDA_ABSX: executeOp(ABSX, LDA); break;
    case LDA_ABSY: executeOp(ABSY, LDA); break;
    case LDA_INDX: executeOp(INDX, LDA); break;
    case LDA_INDY: executeOp(INDY, LDA); break;

    case LDX_IMM:  executeOp(IMM, LDX); break;
    case LDX_ZP:   executeOp(ZPG, LDX); break;
    case LDX_ZPY:  executeOp(ZPY, LDX); break;
    case LDX_ABS:  executeOp(ABS, LDX); break;
    case LDX_ABSY: executeOp(ABSY, LDX); break;

    case LDY_IMM:  executeOp(IMM, LDY); break;
    case LDY_ZP:   executeOp(ZPG, LDY); break;
    case LDY_ZPX:  executeOp(ZPX, LDY); break;
    case LDY_ABS:  executeOp(ABS, LDY); break;
    case LDY_ABSX: executeOp(ABSX, LDY); break;

    case ADC_IMM:  executeOp(IMM, ADC); break;
    case ADC_ZP:   executeOp(ZPG, ADC); break;
    case ADC_ZPX:  executeOp(ZPX, ADC); break;
    case ADC_ABS:  executeOp(ABS, ADC); break;
    case ADC_ABSX: executeOp(ABSX, ADC); break;
    case ADC_ABSY: executeOp(ABSY, ADC); break;
    case ADC_INDX: executeOp(INDX, ADC); break;
    case ADC_INDY: executeOp(INDY, ADC); break;

    case SBC_IMM:  executeOp(IMM, SBC); break;
    case SBC_ZP:   executeOp(ZPG, SBC); break;
    case SBC_ZPX:  executeOp(ZPX, SBC); break;
    case SBC_ABS:  executeOp(ABS, SBC); break;
    case SBC_ABSX: executeOp(ABSX, SBC); break;
    case SBC_ABSY: executeOp(ABSY, SBC); break;
    case SBC_INDX: executeOp(INDX, SBC); break;
    case SBC_INDY: executeOp(INDY, SBC); break;

    case AND_IMM:  executeOp(IMM, AND); break;
    case AND_ZP:   executeOp(ZPG, AND); break;
    case AND_ZPX:  executeOp(ZPX, AND); break;
    case AND_ABS:  executeOp(ABS, AND); break;
    case AND_ABSX: executeOp(ABSX, AND); break;
    case AND_ABSY: executeOp(ABSY, AND); break;
    case AND_INDX: executeOp(INDX, AND); break;
    case AND_INDY: executeOp(INDY, AND); break;

    case ORA_IMM:  executeOp(IMM, ORA); break;
    case ORA_ZP:   executeOp(ZPG, ORA); break;
    case ORA_ZPX:  executeOp(ZPX, ORA); break;
    case ORA_ABS:  executeOp(ABS, ORA); break;
    case ORA_ABSX: executeOp(ABSX, ORA); break;
    case ORA_ABSY: executeOp(ABSY, ORA); break;
    case ORA_INDX: executeOp(INDX, ORA); break;
    case ORA_INDY: executeOp(INDY, ORA); break;

    case EOR_IMM:  executeOp(IMM, EOR); break;
    case EOR_ZP:   executeOp(ZPG, EOR); break;
    case EOR_ZPX:  executeOp(ZPX, EOR); break;
    case EOR_ABS:  executeOp(ABS, EOR); break;
    case EOR_ABSX: executeOp(ABSX, EOR); break;
    case EOR_ABSY: executeOp(ABSY, EOR); break;
    case EOR_INDX: executeOp(INDX, EOR); break;
    case EOR_INDY: executeOp(INDY, EOR); break;

    case CMP_IMM:  executeOp(IMM, CMP); break;
    case CMP_ZP:   executeOp(ZPG, CMP); break;
    case CMP_ZPX:  executeOp(ZPX, CMP); break;
    case CMP_ABS:  executeOp(ABS, CMP); break;
    case CMP_ABSX: executeOp(ABSX, CMP); break;
    case CMP_ABSY: executeOp(ABSY, CMP); break;
    case CMP_INDX: executeOp(INDX, CMP); break;
    case CMP_INDY: executeOp(INDY, CMP); break;

    case CPX_IMM: executeOp(IMM, CPX); break;
    case CPX_ZP:  executeOp(ZPG, CPX); break;
    case CPX_ABS: executeOp(ABS, CPX); break;

    case CPY_IMM: executeOp(IMM, CPY); break;
    case CPY_ZP:  executeOp(ZPG, CPY); break;
    case CPY_ABS: executeOp(ABS, CPY); break;

    case ASL_ACC:  executeRMW(ACC, ASL); break;
    case ASL_ZP:   executeRMW(ZPG, ASL); break;
    case ASL_ZPX:  executeRMW(ZPX, ASL); break;
    case ASL_ABS:  executeRMW(ABS, ASL); break;
    case ASL_ABSX: executeRMW(ABSX, ASL); break;

    case LSR_ACC:  executeRMW(ACC, LSR); break;
    case LSR_ZP:   executeRMW(ZPG, LSR); break;
    case LSR_ZPX:  executeRMW(ZPX, LSR); break;
    case LSR_ABS:  executeRMW(ABS, LSR); break;
    case LSR_ABSX: executeRMW(ABSX, LSR); break;

    case ROL_ACC:  executeRMW(ACC, ROL); break;
    case ROL_ZP:   executeRMW(ZPG, ROL); break;
    case ROL_ZPX:  executeRMW(ZPX, ROL); break;
    case ROL_ABS:  executeRMW(ABS, ROL); break;
    case ROL_ABSX: executeRMW(ABSX, ROL); break;

    case ROR_ACC:  executeRMW(ACC, ROR); break;
    case ROR_ZP:   executeRMW(ZPG, ROR); break;
    case ROR_ZPX:  executeRMW(ZPX, ROR); break;
    case ROR_ABS:  executeRMW(ABS, ROR); break;
    case ROR_ABSX: executeRMW(ABSX, ROR); break;

    case BRK:
        ++PC; // padding byte
        handle_interrupt(0xFFFE, true);
        // return false; /*stop asm*/
        break;

        // Register increments/decrements
    case DEX:
        --X;
        setZN(X);
        break;

    case DEY:
        --Y;
        setZN(Y);
        break;

    case INX:
        ++X;
        setZN(X);
        break;

    case INY:
        ++Y;
        setZN(Y);
        break;

    // Memory increments
    case INC_ZP: {
        uint8_t addr = fetchByte();
        setByte(addr, 1 + readByte(addr));
        setZN(readByte(addr));
        break;
    }

    case INC_ZPX: {
        uint8_t addr = (fetchByte() + X) & 0xFF;
        setByte(addr, 1 + readByte(addr));
        setZN(readByte(addr));
        break;
    }

    case INC_ABS: {
        uint16_t addr = fetchWord();
        setByte(addr, 1 + readByte(addr));
        setZN(readByte(addr));
        break;
    }

    case INC_ABSX: {
        uint16_t addr = (fetchWord() + X) & 0xFFFF;
        setByte(addr, 1 + readByte(addr));
        setZN(readByte(addr));
        break;
    }

    // Memory decrements
    case DEC_ZP: {
        uint8_t addr = fetchByte();
        setByte(addr, readByte(addr) - 1);
        setZN(readByte(addr));
        break;
    }

    case DEC_ZPX: {
        uint8_t addr = (fetchByte() + X) & 0xFF;
        setByte(addr, readByte(addr) - 1);
        setZN(readByte(addr));
        break;
    }

    case DEC_ABS: {
        uint16_t addr = fetchWord();
        setByte(addr, readByte(addr) - 1);
        setZN(readByte(addr));
        break;
    }

    case DEC_ABSX: {
        uint16_t addr = (fetchWord() + X) & 0xFFFF;
        setByte(addr, readByte(addr) - 1);
        setZN(readByte(addr));
        break;
    }

    // STORE INSTRUCTIONS
    case STA_ZP:   setByte(fetchByte(), A); break;
    case STA_ZPX:  setByte(fetchByte() + X, A); break;
    case STA_ABS:  setByte(fetchWord(), A); break;
    case STA_ABSX: setByte(fetchWord() + X, A); break;
    case STA_ABSY: setByte(fetchWord() + Y, A); break;
    case STA_INDX: {
        uint8_t zpAddr = (fetchByte() + X) & 0xFF;
        uint16_t addr  = readByte(zpAddr) | (readByte(zpAddr + 1) << 8);
        setByte(addr, A);
        break;
    }
    case STA_INDY: {
        uint8_t zpAddr = fetchByte();
        uint16_t addr  = readByte(zpAddr) | (readByte(zpAddr + 1) << 8);
        setByte(addr + Y, A);
        break;
    }

    case STX_ZP:  setByte(fetchByte(), X); break;
    case STX_ZPY: setByte((fetchByte() + Y) & 0xFF, X); break;
    case STX_ABS: setByte(fetchWord(), X); break;

    case STY_ZP:  setByte(fetchByte(), Y); break;
    case STY_ZPX: setByte((fetchByte() + X) & 0xFF, Y); break;
    case STY_ABS:
        setByte(fetchWord(), Y);
        break;

        // JUMPS
    case RTS:
        rts();
        break;
    case RTI:
        P = pop();
        setPC(popWord());
        break;
    case JSR: {
        uint16_t addr = fetchWord();
        pushWord(PC - 1);
        setPC(addr);
        break;
    }
    case JMP_ABS: setPC(fetchWord()); break;
    case JMP_IND: {
        uint16_t addr = fetchWord();
        uint8_t lo    = readByte(addr);
        uint8_t hi    = readByte((addr & 0xFF00) | ((addr + 1) & 0x00FF));
        setPC((hi << 8) | lo);
        break;
    }

    case BIT_ZP: {
        uint8_t val = readByte(fetchByte());
        P           = (P & ~(PF_ZERO | PF_NEGATIVE | PF_OVERFLOW)) | ((A & val) == 0 ? PF_ZERO : 0) | (val & 0xC0);
        break;
    }
    case BIT_ABS: {
        uint16_t addr = fetchWord();
        uint8_t val   = readByte(addr);
        P             = (P & ~(PF_ZERO | PF_NEGATIVE | PF_OVERFLOW))
          | ((A & val) == 0 ? PF_ZERO : 0)
          | (val & 0xC0); // Copy bits 7 (N) and 6 (V) from memory
        break;
    }

        // Branches
    case BEQ: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (P & PF_ZERO) {
            branchTo(PC + offset);
        }
        break;
    }
    case BNE: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (!(P & PF_ZERO)) {
            branchTo(PC + offset);
        }
        break;
    }
    case BCC: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (!(P & PF_CARRY)) {
            branchTo(PC + offset);
        }
        break;
    }
    case BCS: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (P & PF_CARRY) {
            branchTo(PC + o);
        }
        break;
    }
    case BMI: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (P & PF_NEGATIVE) {
            branchTo(PC + offset);
        }
        break;
    }
    case BPL: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (!(P & PF_NEGATIVE)) {
            branchTo(PC + offset);
        }
        break;
    }
    case BVC: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (!(P & PF_OVERFLOW)) {
            branchTo(PC + offset);
        }
        break;
    }
    case BVS: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (P & PF_OVERFLOW) {
            branchTo(PC + offset);
        }
        break;
    }
    // Flag ops
    case CLC: P &= ~PF_CARRY; break;
    case SEC: P |= PF_CARRY; break;
    case CLI: P &= ~PF_INTERRUPT; break;
    case SEI: P |= PF_INTERRUPT; break;
    case CLV: P &= ~PF_OVERFLOW; break;
    case CLD: P &= ~PF_DECIMAL; break;
    case SED:
        P |= PF_DECIMAL;
        break;

        // Stack ops
    case PHA: push(A); break;
    case PHP:
        push(P | 0x30); // set B (0x10) and unused (0x20)
        break;
    case PLA:
        A = pop();
        setZN(A);
        break;
    case PLP:
        P = (pop() & 0xEF) | 0x20; // clear B, force unused bit
        break;



        // TRANSFER INSTRUCTIONS
    case TAX:
        X = A;
        setZN(X);
        break;

    case TAY:
        Y = A;
        setZN(Y);
        break;

    case TXA:
        A = X;
        setZN(A);
        break;

    case TYA:
        A = Y;
        setZN(A);
        break;

    case TSX:
        X = SP;
        setZN(X);
        break;

    case TXS:
        SP = X;
        // Note: TXS does NOT affect flags
        break;

        // NOP
    case NOP: break;

    default:
        --PC;
        cpuJam = true;
        return false;
        break;
    }

    // check if we hit a breakpoint for next instruction
    auto it = breakpoints.find(PC);
    if (it != breakpoints.end() && it->second.onExec) {
        breakPointHit = true;
    }

    return true;
}

void CPU6502::rts() { setPC(popWord() + 1); }


void CPU6502::setZN(uint8_t value) {
    if (value == 0) {
        P |= PF_ZERO;
    } else {
        P &= ~PF_ZERO;
    }
    if (value & 0x80) {
        P |= PF_NEGATIVE;
    } else {
        P &= ~PF_NEGATIVE;
    }
}

void CPU6502::printState() {
    const char* kernal = kernalRoutineName(PC);
    if (kernal != nullptr) {
        printf("---%s---\n", kernal);
    }
    // disassemble
    //           CRANK     PLOT            jsr chrget      put X as row    CHRGET+PRINTC
    // if (PC == 0x033e || PC == 0xe50a || PC == 0xb79b || PC == 0xe88c || PC == 0xab13
    //
    //     || PC == 0x0341 || PC == 0xAB16 || PC == 0xAB19) {
    //     int pause = 1;
    // }
    printf("%.4X  %s %s\n", int(PC), disassemble(PC).c_str(), registers().c_str());

    // 079 sets A to 37 ('7' - 1st parameter ASCII)
}



static std::string hex8(uint8_t i) {
    const char* hx = "0123456789ABCDEF?";
    char buf[3];

    buf[0] = hx[i >> 4];
    buf[1] = hx[i & 0x0f];
    buf[2] = '\0';
    return std::string(buf);
}
static std::string hex16(uint16_t i) {
    return hex8(i >> 8) + hex8(i & 0xff);
}

std::string CPU6502::disassemble(uint16_t address, bool showBytes) {

    uint8_t op = readByte(address);
    auto info  = getOpcodeInfo(op);

    std::string out;


    uint8_t lo = readByte(address + 1);
    uint8_t hi = readByte(address + 2);
    uint16_t w = lo | (hi << 8);

    if (showBytes) {
        switch (info.length) {
        case 1:
            out += hex8(op) + "        ";
            break;
        case 2:
            out += hex8(op) + " " + hex8(lo) + "     ";
            break;
        case 3:
            out += hex8(op) + " " + hex8(lo) + " " + hex8(hi) + "  ";
            break;
        default:
            out += hex8(op) + "!!      ";
            break;
        }
    }


    out += info.mnemonic;
    out += ' ';
    switch (info.admode) {
    case IMP: break;
    case ACC:
        out += 'A';
        break;

    case IMM:
        out += "#$" + hex8(lo);
        break;

    case ZPG:
        out += "$" + hex8(lo);
        break;

    case ZPX:
        out += "$" + hex8(lo) + ",X";
        break;

    case ZPY:
        out += "$" + hex8(lo) + ",Y";
        break;

    case ABS:
        if (hi == 0) {
            out += ">$" + hex16(w); // > force re-assembly with ABS not ZPG
        } else {
            out += "$" + hex16(w);
        }
        break;

    case ABSX:
        if (hi == 0) {
            out += ">$" + hex16(w) + ",X";
        } else {
            out += "$" + hex16(w) + ",X";
        }
        break;

    case ABSY:
        if (hi == 0) {
            out += ">$" + hex16(w) + ",Y";
        } else {
            out += "$" + hex16(w) + ",Y";
        }
        break;

    case IND:
        out += "($" + hex16(w) + ")";
        break;

    case INDX:
        out += "($" + hex8(lo) + ",X)";
        break;

    case INDY:
        out += "($" + hex8(lo) + "),Y";
        break;

    case REL: {
        int8_t off   = (int8_t)lo;
        uint16_t dst = address + 2 + off;
        out += "$" + hex16(dst);
        break;
    }

    default:
        break;
    }




#if _DEBUG
    // assemble the disassembly to RAM and verify it
    if (info.admode != INVALID) {
        uint8_t org[3] = { uint8_t(RAM[address]),
                           uint8_t(RAM[address + 1]),
                           uint8_t(RAM[address + 2]) };
        for (int i = 0; i < info.length; ++i) {
            RAM[address + i] = 0xdc;
        }
        std::string scode = out.substr(showBytes ? 10 : 0);
        const char* code  = scode.c_str();
        assemble(code, address); // always in RAM

        for (int i = 0; i < info.length; ++i) {
            uint8_t b = readByte(address + i);
            uint8_t m = RAM[address + i];

            if (b != m) {
                assemble(code, address);
                throw "Fix assembler";
            }
            RAM[address + i] = org[i];
        }
    }
#endif

    return out;
}


static bool isBranch(const char* m) {
    static const char* b[] = {
        "BPL", "BMI", "BVC", "BVS", "BCC", "BCS", "BNE", "BEQ"
    };
    for (auto s : b) {
        if (!strncmp(m, s, 3)) {
            return true;
        }
    }
    return false;
}

static void skipWS(const char*& s) {
    while (*s && *s <= ' ') {
        ++s;
    }
}

static bool parseNumber(const char*& s, uint16_t& v) {
    int base = 10;
    if (*s == '$') {
        base = 16;
        ++s;
    }
    char* e;
    v = uint16_t(strtoul(s, &e, base));
    if (e == s) {
        return false;
    }
    s = e;
    skipWS(s);
    return true;
}


CPU6502::AddrMode CPU6502::parseOperand(
    const char*& s,
    const char* mnemonic,
    uint16_t& value,
    int16_t pc) {
    skipWS(s);
    if (!*s) {
        return IMP;
    }

    if (*s == 'A') {
        return ACC;
    }

    if (*s == '#') {
        ++s;
        return parseNumber(s, value) ? IMM : INVALID;
    }

    bool indirect = false;
    if (*s == '(') {
        indirect = true;
        ++s;
    }

    bool forceAbs = false;
    if (*s == '>') {
        forceAbs = true;
        ++s;
    }

    if (!parseNumber(s, value)) {
        return INVALID;
    }

    if (indirect) {
        if (*s == ',') {
            ++s;
            if (*s++ != 'X') {
                return INVALID;
            }
            skipWS(s);
            if (*s++ != ')') {
                return INVALID;
            }
            return INDX;
        }
        if (*s++ != ')') {
            return INVALID;
        }
        skipWS(s);
        if (*s == ',') {
            ++s;
            if (*s++ != 'Y') {
                return INVALID;
            }
            return INDY;
        }
        return IND; // JMP only (validated later)
    }

    if (isBranch(mnemonic)) {
        int16_t off = value - (pc + 2);
        if (off < -128 || off > 127) {
            return INVALID;
        }
        value = uint8_t(off);
        return REL;
    }

    AddrMode m = ABS;
    if (!forceAbs && value <= 0xff) {
        // does the command support a ZPG version?
        for (int i = 0; i < 256; ++i) {
            auto info = getOpcodeInfo(uint8_t(i));
            if (info.admode == ZPG && strncmp(mnemonic, info.mnemonic, 3) == 0) {
                m = ZPG;
                break;
            }
        }
    }


    if (*s == ',') {
        ++s;
        if (*s == 'X') {
            return m == ZPG ? ZPX : ABSX;
        }
        if (*s == 'Y') {
            return m == ZPG ? ZPY : ABSY;
        }
        return INVALID;
    }

    return m;
}

// returns number of bytes written for operand
int CPU6502::emitBytes(CPU6502::AddrMode m, uint16_t v, uint8_t* out) {
    switch (m) {
    case IMP:
    case ACC: return 0;
    case IMM:
    case ZPG:
    case ZPX:
    case ZPY:
    case REL:
    case INDX:
    case INDY:
        out[0] = v & 0xff;
        return 1;
    case ABS:
    case ABSX:
    case ABSY:
    case IND:
        out[0] = v & 0xff;
        out[1] = v >> 8;
        return 2;
    default: return -1;
    }
}

// input must be uppercase
int16_t CPU6502::assemble(const char* line, int16_t addr) {

    const char* s = line;
    skipWS(s);
    const char* mnemonic = s;
    s += 3;

    uint16_t value = 0;
    AddrMode mode  = parseOperand(s, mnemonic, value, addr);
    if (mode == INVALID) {
        return 0;
    }

    uint8_t data[2];
    int len = emitBytes(mode, value, data);
    if (len < 0) {
        return 0;
    }

    for (int op = 0; op < 256; ++op) {
        auto info = getOpcodeInfo(op);
        if (strncmp(info.mnemonic, mnemonic, 3)) {
            continue;
        }
        if (info.admode != mode) {
            continue;
        }
        if (info.length != 1 + len) {
            continue;
        }

        setByte(addr++, op);
        for (int i = 0; i < len; ++i) {
            setByte(addr++, data[i]);
        }
        return addr;
    }
    return 0;
}


std::string CPU6502::registers() {
    static char buf[256];
    memset(buf, 0, sizeof(buf));

    sprintf(buf, "PC:%.4X   A:%.2X X:%.2X Y:%.2X   P:%.2X SP:%.2X",
            int(PC),
            int(A), int(X), int(Y), int(P), int(SP));
    return std::string(buf);
}

void CPU6502::printHeatMap() {

    uint16_t ptr = 0x0801;
    auto getword = [&]() -> uint16_t {uint16_t m = RAM[ptr] + (RAM[ptr+1]<<8); ptr+=2; return m; };


    std::bitset<0x10000> usedLines;
    for (;;) {
        uint16_t nxt = getword();
        if (nxt == 0) {
            break;
        }
        uint16_t line = getword();
        usedLines.set(line);
        ptr = nxt;
    }

redo:
    for (auto it = heatmap.begin(); it != heatmap.end(); ++it) {
        if (!usedLines.test(it->first)) {
            heatmap.erase(it);
            goto redo;
        }
    }

    if (heatmap.empty()) {
        return;
    }
    size_t greatest = 1;
    for (auto& h : heatmap) {
        if (h.second > greatest) {
            greatest = h.second;
        }
    }
    printf("BASIC HeatMap:\n");
    printf("  LINE   CYCLES                    HEAT     CODE\n");


    ptr = 0x0801;
    for (;;) {
        uint16_t nxt = getword();
        if (nxt == 0) {
            break;
        }
        uint16_t line = getword();
        size_t cycles = heatmap[line];
        printf("%6d %8d ", int(line), int(cycles));

        size_t bar = (cycles * 24 + 1) / greatest;
        for (size_t i = 0; i < 24; ++i) {
            if (i < bar) {
                printf("#");
            } else {
                printf(" ");
            }
        }

        bool printed = true;
        while (RAM[ptr] != 0 && ptr < 0xA000) {
            printed = true;
            switch (RAM[ptr]) {
            case 0x80: printf("%-8s", "END"); break;
            case 0x81: printf("%-8s", "FOR"); break;
            case 0x82: printf("%-8s", "NEXT"); break;
            case 0x83: printf("%-8s", "DATA"); break;
            case 0x85: printf("%-8s", "INPUT"); break;
            case 0x86: printf("%-8s", "DIM"); break;
            case 0x87: printf("%-8s", "READ"); break;
            case 0x88: printf("%-8s", "LET"); break;
            case 0x89: printf("%-8s", "GOTO"); break;
            case 0x8A: printf("%-8s", "RUN"); break;
            case 0x8B: printf("%-8s", "IF"); break;
            case 0x8D: printf("%-8s", "GOSUB"); break;
            case 0x8E: printf("%-8s", "RETURN"); break;
            case 0x8F: printf("%-8s", "REM"); break;
            case 0x91: printf("%-8s", "ON"); break;
            case 0x96: printf("%-8s", "DEF"); break;
            case 0x97: printf("%-8s", "POKE"); break;
            case 0x98: printf("%-8s", "PRINT#"); break;
            case 0x99: printf("%-8s", "PRINT"); break;
            case 0x9F: printf("%-8s", "OPEN"); break;
            case 0xA0: printf("%-8s", "CLOSE"); break;
            case 0xA1: printf("%-8s", "GET"); break;

            default: printed = false;
            }
            ++ptr;
            if (printed) {
                break;
            }
        }

        ptr = nxt;
        // while (RAM[ptr] != 0 && ptr < 0xA000) {
        //     ++ptr;
        // }
        printf("\n");
    }
}


void CPU6502::updateBasicHeatmap() {
    static uint16_t lastLine = 0;
    uint16_t currentLine     = RAM[0x39] | (RAM[0x3A] << 8);

    if (currentLine != lastLine) {
        // attribute elapsed cycles to previous line
        if (lastLine != 0) {
            heatmap[lastLine] += (cycleCount - lastCycleSnapshot);
        }

        lastLine          = currentLine;
        lastCycleSnapshot = cycleCount;
    }
}