// Full C64 6502 Emulator with all documented opcodes
#include "cpu-6502.h"
#include <functional>

void CPU6502::reset() {
    cpuJam = false;
    A = X = Y = 0;
    SP        = 0xFF;
    PC        = readWord(0xFFFC); // Reset vector
    P         = 0x24; // IRQ disabled by default
}

bool CPU6502::sys(uint16_t address) {
    memory[0x01FF] = 0x08; // High byte of 0x080C
    memory[0x01FE] = 0x0C; // Low byte of 0x080C
    SP             = 0xFD; // Stack pointer (points to last used position)
    PC             = address;
    return true;
}

enum P_Flags {
    PF_CARRY     = 0x01,
    PF_ZERO      = 0x02,
    PF_INTERRUPT = 0x04,
    PF_DECIMAL   = 0x08,
    PF_BREAK     = 0x10,
    PF_UNUSED    = 0x20,
    PF_OVERFLOW  = 0x40,
    PF_NEGATIVE  = 0x80,
};


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
    BIT_ZP = 0x24,

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



uint8_t CPU6502::fetchOperand(AddrMode mode) {
    switch (mode) {
    case IMM: return fetchByte();
    case ZP:  {
        uint8_t addr = fetchByte();
        return memory[addr];
    }
    case ZPX: {
        uint8_t addr = (fetchByte() + X) & 0xFF;
        return memory[addr];
    }
    case ZPY: {
        uint8_t addr = (fetchByte() + Y) & 0xFF;
        return memory[addr];
    }
    case ABS: {
        uint16_t addr = fetchWord();
        return memory[addr];
    }
    case ABSX: {
        uint16_t addr = (fetchWord() + X) & 0xFFFF;
        return memory[addr];
    }
    case ABSY: {
        uint16_t addr = (fetchWord() + Y) & 0xFFFF;
        return memory[addr];
    }
    case INDX: {
        uint8_t zp    = (fetchByte() + X) & 0xFF;
        uint16_t addr = memory[zp] | (memory[(zp + 1) & 0xFF] << 8);
        return memory[addr];
    }
    case INDY: {
        uint8_t zp     = fetchByte();
        uint16_t addr  = memory[zp] | (memory[(zp + 1) & 0xFF] << 8);
        uint16_t addrY = (addr + Y) & 0xFFFF;
        return memory[addrY];
    }
    default: return 0;
    }
}





bool CPU6502::executeNext() {
    if (PC == 0x080C) {
        return false; // back to BASIC
    }

    auto executeOp = [this](AddrMode mode, std::function<void(uint8_t)> op) {
        uint8_t value = fetchOperand(mode);
        op(value);
    };

    auto executeRMW = [this](AddrMode mode, std::function<uint8_t(uint8_t)> op) {
        if (mode == IMM) {
            return; // Not used for RMW
        }
        if (mode == ACC) {
            A = op(A);
        } else {
            uint16_t addr = 0;
            switch (mode) {
            case ZP:   addr = fetchByte(); break;
            case ZPX:  addr = (fetchByte() + X) & 0xFF; break;
            case ABS:  addr = fetchWord(); break;
            case ABSX: addr = (fetchWord() + X) & 0xFFFF; break;
            default:   return;
            }
            uint8_t value = memory[addr];
            value         = op(value);
            memory[addr]  = value;
        }
    };


    auto AND = [this](uint8_t value) {
        A &= value;
        setZN(A);
    };

    auto ORA = [this](uint8_t value) {
        A |= value;
        setZN(A);
    };
    auto EOR = [this](uint8_t value) {
        A ^= value;
        setZN(A);
    };
    auto ADC = [this](uint8_t value) {
        uint16_t sum = A + value + (P & PF_CARRY ? 1 : 0);
        setFlag(PF_CARRY, sum > 0xFF);
        uint8_t result = sum & 0xFF;
        setFlag(PF_OVERFLOW, (~(A ^ value) & (A ^ result)) & 0x80);
        A = result;
        setZN(A);
    };
    auto SBC = [this](uint8_t value) {
        uint16_t temp = A - value - (P & PF_CARRY ? 0 : 1);
        setFlag(PF_CARRY, temp < 0x100); // set if no borrow
        uint8_t result = temp & 0xFF;
        setFlag(PF_OVERFLOW, ((A ^ result) & (A ^ value)) & 0x80);
        A = result;
        setZN(A);
    };
    auto CMP = [this](uint8_t value) {
        uint8_t result = A - value;
        setFlag(PF_CARRY, A >= value);
        setFlag(PF_ZERO, result == 0);
        setFlag(PF_NEGATIVE, result & 0x80);
    };
    auto CPX = [this](uint8_t value) {
        uint8_t result = X - value;
        setFlag(PF_CARRY, X >= value);
        setFlag(PF_ZERO, result == 0);
        setFlag(PF_NEGATIVE, result & 0x80);
    };

    auto CPY = [this](uint8_t value) {
        uint8_t result = Y - value;
        setFlag(PF_CARRY, Y >= value);
        setFlag(PF_ZERO, result == 0);
        setFlag(PF_NEGATIVE, result & 0x80);
    };

    auto ASL = [this](uint8_t value) -> uint8_t {
        setFlag(PF_CARRY, value & 0x80);
        value <<= 1;
        setZN(value);
        return value;
    };

    auto LSR = [this](uint8_t value) -> uint8_t {
        setFlag(PF_CARRY, value & 0x01);
        value >>= 1;
        setZN(value);
        return value;
    };
    auto ROL = [this](uint8_t value) -> uint8_t {
        uint8_t oldCarry = (P & PF_CARRY) ? 1 : 0;
        setFlag(PF_CARRY, value & 0x80);
        value = (value << 1) | oldCarry;
        setZN(value);
        return value;
    };
    auto ROR = [this](uint8_t value) -> uint8_t {
        uint8_t oldCarry = (P & PF_CARRY) ? 0x80 : 0;
        setFlag(PF_CARRY, value & 0x01);
        value = (value >> 1) | oldCarry;
        setZN(value);
        return value;
    };


    opcode = fetchByte();

    switch (opcode) {
    case ADC_IMM:  executeOp(IMM, ADC); break;
    case ADC_ZP:   executeOp(ZP, ADC); break;
    case ADC_ZPX:  executeOp(ZPX, ADC); break;
    case ADC_ABS:  executeOp(ABS, ADC); break;
    case ADC_ABSX: executeOp(ABSX, ADC); break;
    case ADC_ABSY: executeOp(ABSY, ADC); break;
    case ADC_INDX: executeOp(INDX, ADC); break;
    case ADC_INDY: executeOp(INDY, ADC); break;

    case SBC_IMM:  executeOp(IMM, SBC); break;
    case SBC_ZP:   executeOp(ZP, SBC); break;
    case SBC_ZPX:  executeOp(ZPX, SBC); break;
    case SBC_ABS:  executeOp(ABS, SBC); break;
    case SBC_ABSX: executeOp(ABSX, SBC); break;
    case SBC_ABSY: executeOp(ABSY, SBC); break;
    case SBC_INDX: executeOp(INDX, SBC); break;
    case SBC_INDY: executeOp(INDY, SBC); break;

    case AND_IMM:  executeOp(IMM, AND); break;
    case AND_ZP:   executeOp(ZP, AND); break;
    case AND_ZPX:  executeOp(ZPX, AND); break;
    case AND_ABS:  executeOp(ABS, AND); break;
    case AND_ABSX: executeOp(ABSX, AND); break;
    case AND_ABSY: executeOp(ABSY, AND); break;
    case AND_INDX: executeOp(INDX, AND); break;
    case AND_INDY: executeOp(INDY, AND); break;

    case ORA_IMM:  executeOp(IMM, ORA); break;
    case ORA_ZP:   executeOp(ZP, ORA); break;
    case ORA_ZPX:  executeOp(ZPX, ORA); break;
    case ORA_ABS:  executeOp(ABS, ORA); break;
    case ORA_ABSX: executeOp(ABSX, ORA); break;
    case ORA_ABSY: executeOp(ABSY, ORA); break;
    case ORA_INDX: executeOp(INDX, ORA); break;
    case ORA_INDY: executeOp(INDY, ORA); break;

    case EOR_IMM:  executeOp(IMM, EOR); break;
    case EOR_ZP:   executeOp(ZP, EOR); break;
    case EOR_ZPX:  executeOp(ZPX, EOR); break;
    case EOR_ABS:  executeOp(ABS, EOR); break;
    case EOR_ABSX: executeOp(ABSX, EOR); break;
    case EOR_ABSY: executeOp(ABSY, EOR); break;
    case EOR_INDX: executeOp(INDX, EOR); break;
    case EOR_INDY: executeOp(INDY, EOR); break;

    case CMP_IMM:  executeOp(IMM, CMP); break;
    case CMP_ZP:   executeOp(ZP, CMP); break;
    case CMP_ZPX:  executeOp(ZPX, CMP); break;
    case CMP_ABS:  executeOp(ABS, CMP); break;
    case CMP_ABSX: executeOp(ABSX, CMP); break;
    case CMP_ABSY: executeOp(ABSY, CMP); break;
    case CMP_INDX: executeOp(INDX, CMP); break;
    case CMP_INDY: executeOp(INDY, CMP); break;

    case CPX_IMM: executeOp(IMM, CPX); break;
    case CPX_ZP:  executeOp(ZP, CPX); break;
    case CPX_ABS: executeOp(ABS, CPX); break;

    case CPY_IMM: executeOp(IMM, CPY); break;
    case CPY_ZP:  executeOp(ZP, CPY); break;
    case CPY_ABS: executeOp(ABS, CPY); break;

    case ASL_ACC:  executeRMW(ACC, ASL); break;
    case ASL_ZP:   executeRMW(ZP, ASL); break;
    case ASL_ZPX:  executeRMW(ZPX, ASL); break;
    case ASL_ABS:  executeRMW(ABS, ASL); break;
    case ASL_ABSX: executeRMW(ABSX, ASL); break;

    case LSR_ACC:  executeRMW(ACC, LSR); break;
    case LSR_ZP:   executeRMW(ZP, LSR); break;
    case LSR_ZPX:  executeRMW(ZPX, LSR); break;
    case LSR_ABS:  executeRMW(ABS, LSR); break;
    case LSR_ABSX: executeRMW(ABSX, LSR); break;

    case ROL_ACC:  executeRMW(ACC, ROL); break;
    case ROL_ZP:   executeRMW(ZP, ROL); break;
    case ROL_ZPX:  executeRMW(ZPX, ROL); break;
    case ROL_ABS:  executeRMW(ABS, ROL); break;
    case ROL_ABSX: executeRMW(ABSX, ROL); break;

    case ROR_ACC:  executeRMW(ACC, ROR); break;
    case ROR_ZP:   executeRMW(ZP, ROR); break;
    case ROR_ZPX:  executeRMW(ZPX, ROR); break;
    case ROR_ABS:  executeRMW(ABS, ROR); break;
    case ROR_ABSX: executeRMW(ABSX, ROR); break;

    case BRK:
        brk();
        return false; /*stop asm*/
        break;



        // Register increments/decrements
    case DEX:
        X = X - 1;
        setZN(X);
        break;

    case DEY:
        Y = Y - 1;
        setZN(Y);
        break;

    case INX:
        X = X + 1;
        setZN(X);
        break;

    case INY:
        Y = Y + 1;
        setZN(Y);
        break;

    // Memory increments
    case INC_ZP: {
        uint8_t addr = fetchByte();
        memory[addr]++;
        setZN(memory[addr]);
        break;
    }

    case INC_ZPX: {
        uint8_t addr = (fetchByte() + X) & 0xFF;
        memory[addr]++;
        setZN(memory[addr]);
        break;
    }

    case INC_ABS: {
        uint16_t addr = fetchWord();
        memory[addr]++;
        setZN(memory[addr]);
        break;
    }

    case INC_ABSX: {
        uint16_t addr = (fetchWord() + X) & 0xFFFF;
        memory[addr]++;
        setZN(memory[addr]);
        break;
    }

    // Memory decrements
    case DEC_ZP: {
        uint8_t addr = fetchByte();
        memory[addr]--;
        setZN(memory[addr]);
        break;
    }

    case DEC_ZPX: {
        uint8_t addr = (fetchByte() + X) & 0xFF;
        memory[addr]--;
        setZN(memory[addr]);
        break;
    }

    case DEC_ABS: {
        uint16_t addr = fetchWord();
        memory[addr]--;
        setZN(memory[addr]);
        break;
    }

    case DEC_ABSX: {
        uint16_t addr = (fetchWord() + X) & 0xFFFF;
        memory[addr]--;
        setZN(memory[addr]);
        break;
    }

    // LOAD INSTRUCTIONS
    case LDA_IMM:
        A = fetchByte();
        setZN(A);
        break;
    case LDA_ZP:
        A = memory[fetchByte()];
        setZN(A);
        break;
    case LDA_ZPX:
        A = memory[(fetchByte() + X) & 0xFF];
        setZN(A);
        break;
    case LDA_ABS:
        A = memory[fetchWord()];
        setZN(A);
        break;
    case LDA_ABSX: {
        uint16_t addr = fetchWord();
        A             = memory[addr + X];
        setZN(A);
        break;
    }
    case LDA_ABSY: {
        uint16_t addr = fetchWord();
        A             = memory[addr + Y];
        setZN(A);
        break;
    }
    case LDA_INDX: {
        uint8_t zpAddr = (fetchByte() + X) & 0xFF;
        uint16_t addr  = memory[zpAddr] | (memory[(zpAddr + 1) & 0xFF] << 8);
        A              = memory[addr];
        setZN(A);
        break;
    }
    case LDA_INDY: {
        uint8_t zpAddr = fetchByte();
        uint16_t addr  = memory[zpAddr] | (memory[(zpAddr + 1) & 0xFF] << 8);
        A              = memory[addr + Y];
        setZN(A);
        break;
    }

    case LDX_IMM:
        X = fetchByte();
        setZN(X);
        break;
    case LDX_ZP:
        X = memory[fetchByte()];
        setZN(X);
        break;
    case LDX_ZPY:
        X = memory[(fetchByte() + Y) & 0xFF];
        setZN(X);
        break;
    case LDX_ABS:
        X = memory[fetchWord()];
        setZN(X);
        break;
    case LDX_ABSY: {
        uint16_t addr = fetchWord();
        X             = memory[addr + Y];
        setZN(X);
        break;
    }

    case LDY_IMM:
        Y = fetchByte();
        setZN(Y);
        break;
    case LDY_ZP:
        Y = memory[fetchByte()];
        setZN(Y);
        break;
    case LDY_ZPX:
        Y = memory[(fetchByte() + X) & 0xFF];
        setZN(Y);
        break;
    case LDY_ABS:
        Y = memory[fetchWord()];
        setZN(Y);
        break;
    case LDY_ABSX: {
        uint16_t addr = fetchWord();
        Y             = memory[addr + X];
        setZN(Y);
        break;
    }

    // STORE INSTRUCTIONS
    case STA_ZP:   memory[fetchByte()] = A; break;
    case STA_ZPX:  memory[(fetchByte() + X) & 0xFF] = A; break;
    case STA_ABS:  memory[fetchWord()] = A; break;
    case STA_ABSX: {
        uint16_t addr    = fetchWord();
        memory[addr + X] = A;
        break;
    }
    case STA_ABSY: {
        uint16_t addr    = fetchWord();
        memory[addr + Y] = A;
        break;
    }
    case STA_INDX: {
        uint8_t zpAddr = (fetchByte() + X) & 0xFF;
        uint16_t addr  = memory[zpAddr] | (memory[(zpAddr + 1) & 0xFF] << 8);
        memory[addr]   = A;
        break;
    }
    case STA_INDY: {
        uint8_t zpAddr   = fetchByte();
        uint16_t addr    = memory[zpAddr] | (memory[(zpAddr + 1) & 0xFF] << 8);
        memory[addr + Y] = A;
        break;
    }

    case STX_ZP:  memory[fetchByte()] = X; break;
    case STX_ZPY: memory[(fetchByte() + Y) & 0xFF] = X; break;
    case STX_ABS: memory[fetchWord()] = X; break;

    case STY_ZP:  memory[fetchByte()] = Y; break;
    case STY_ZPX: memory[(fetchByte() + X) & 0xFF] = Y; break;
    case STY_ABS: memory[fetchWord()] = Y; break;


    case RTS: rts(); break;
    case RTI:
        P  = pop();
        PC = popWord();
        break;
    case JSR: {
        uint16_t addr = fetchWord();
        pushWord(PC - 1);
        PC = addr;
        break;
    }
    case JMP_ABS: PC = fetchWord(); break;
    case JMP_IND: {
        uint16_t addr = fetchWord();
        uint8_t lo    = memory[addr];
        uint8_t hi    = memory[(addr & 0xFF00) | ((addr + 1) & 0x00FF)];
        PC            = (hi << 8) | lo;
        break;
    }

    case BIT_ZP: {
        uint8_t val = memory[fetchByte()];
        P           = (P & ~(PF_ZERO | PF_NEGATIVE | PF_OVERFLOW)) | ((A & val) == 0 ? PF_ZERO : 0) | (val & 0xC0);
        break;
    }



        // Branches
    case BEQ: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (P & PF_ZERO) {
            PC += offset;
        }
        break;
    }
    case BNE: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (!(P & PF_ZERO)) {
            PC += offset;
        }
        break;
    }
    case BCC: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (!(P & PF_CARRY)) {
            PC += o;
        }
        break;
    }
    case BCS: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (P & PF_CARRY) {
            PC += o;
        }
        break;
    }
    case BMI: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (P & PF_NEGATIVE) {
            PC += o;
        }
        break;
    }
    case BPL: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (!(P & PF_NEGATIVE)) {
            PC += o;
        }
        break;
    }
    case BVC: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (!(P & PF_OVERFLOW)) {
            PC += o;
        }
        break;
    }
    case BVS: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (P & PF_OVERFLOW) {
            PC += o;
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
    case PHP: push(P); break;
    case PLA:
        A = pop();
        setZN(A);
        break;
    case PLP:
        P = pop();
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

    return true;
}

void CPU6502::rts() { PC = popWord() + 1; }

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

void CPU6502::brk() {
    PC++;
    pushWord(PC);
    push(P | PF_BREAK);
    P |= PF_INTERRUPT;
    PC = readWord(0xFFFE);
}
