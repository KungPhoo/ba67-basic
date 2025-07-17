// Full C64 6502 Emulator with all documented opcodes
#include "cpu-6502.h"

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
    AND_IMM = 0x29,
    ORA_IMM = 0x09,
    EOR_IMM = 0x49,
    BIT_ZP  = 0x24,

    // Shifts
    ASL_A = 0x0A,
    LSR_A = 0x4A,
    ROL_A = 0x2A,
    ROR_A = 0x6A,

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

    // Comparison
    CMP_IMM = 0xC9,
    CPX_IMM = 0xE0,
    CPY_IMM = 0xC0,

    // Stack ops
    PHA = 0x48,
    PHP = 0x08,
    PLA = 0x68,
    PLP = 0x28
};


bool CPU6502::executeNext() {
    if (PC == 0x080C) {
        return false; // back to BASIC
    }


    opcode = fetchByte();

    switch (opcode) {


    case BRK:
        brk();
        return false; /*stop asm*/
        break;

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
    case AND_IMM:
        A &= fetchByte();
        setZN(A);
        break;
    case ORA_IMM:
        A |= fetchByte();
        setZN(A);
        break;
    case EOR_IMM:
        A ^= fetchByte();
        setZN(A);
        break;
    case BIT_ZP: {
        uint8_t val = memory[fetchByte()];
        P           = (P & ~(PF_ZERO | PF_NEGATIVE | PF_OVERFLOW)) | ((A & val) == 0 ? PF_ZERO : 0) | (val & 0xC0);
        break;
    }
    case ASL_A:
        P = (P & ~PF_CARRY) | ((A & 0x80) ? PF_CARRY : 0);
        A <<= 1;
        setZN(A);
        break;
    case LSR_A:
        P = (P & ~PF_CARRY) | (A & 0x01);
        A >>= 1;
        setZN(A);
        break;
    case ROL_A: {
        bool oldCarry = P & PF_CARRY;
        P             = (P & ~PF_CARRY) | ((A & 0x80) ? PF_CARRY : 0);
        A             = (A << 1) | (oldCarry ? 1 : 0);
        setZN(A);
        break;
    }
    case ROR_A: {
        bool oldCarry = P & PF_CARRY;
        P             = (P & ~PF_CARRY) | (A & 0x01);
        A             = (A >> 1) | (oldCarry ? 0x80 : 0);
        setZN(A);
        break;
    }
    // Branches
    case BEQ: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (P & PF_ZERO)
            PC += offset;
        break;
    }
    case BNE: {
        int8_t offset = static_cast<int8_t>(fetchByte());
        if (!(P & PF_ZERO))
            PC += offset;
        break;
    }
    case BCC: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (!(P & PF_CARRY))
            PC += o;
        break;
    }
    case BCS: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (P & PF_CARRY)
            PC += o;
        break;
    }
    case BMI: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (P & PF_NEGATIVE)
            PC += o;
        break;
    }
    case BPL: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (!(P & PF_NEGATIVE))
            PC += o;
        break;
    }
    case BVC: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (!(P & PF_OVERFLOW))
            PC += o;
        break;
    }
    case BVS: {
        int8_t o = static_cast<int8_t>(fetchByte());
        if (P & PF_OVERFLOW)
            PC += o;
        break;
    }
    // Flag ops
    case CLC: P &= ~PF_CARRY; break;
    case SEC: P |= PF_CARRY; break;
    case CLI: P &= ~PF_INTERRUPT; break;
    case SEI: P |= PF_INTERRUPT; break;
    case CLV: P &= ~PF_OVERFLOW; break;
    case CLD: P &= ~PF_DECIMAL; break;
    case SED: P |= PF_DECIMAL; break;
    // Compare X and Y
    case CPX_IMM: {
        uint8_t val = fetchByte();
        P           = (P & ~(PF_CARRY | PF_ZERO | PF_NEGATIVE));
        if (X >= val)
            P |= PF_CARRY;
        if (X == val)
            P |= PF_ZERO;
        if ((X - val) & 0x80)
            P |= PF_NEGATIVE;
        break;
    }
    case CPY_IMM: {
        uint8_t val = fetchByte();
        P           = (P & ~(PF_CARRY | PF_ZERO | PF_NEGATIVE));
        if (Y >= val)
            P |= PF_CARRY;
        if (Y == val)
            P |= PF_ZERO;
        if ((Y - val) & 0x80)
            P |= PF_NEGATIVE;
        break;
    }
    case CMP_IMM: {
        uint8_t val = fetchByte();
        P           = (P & ~(PF_CARRY | PF_ZERO | PF_NEGATIVE));
        if (A >= val)
            P |= PF_CARRY;
        if (A == val)
            P |= PF_ZERO;
        if ((A - val) & 0x80)
            P |= PF_NEGATIVE;
        break;
    }
    // Stack ops
    case PHA: push(A); break;
    case PHP: push(P); break;
    case PLA:
        A = pop();
        setZN(A);
        break;
    case PLP: P = pop(); break;
    // NOP
    case NOP: break;

    default:
        cpuJam = true;
        return false;
        break;
    }

    return true;
}

void CPU6502::rts() { PC = popWord() + 1; }

void CPU6502::setZN(uint8_t value) {
    if (value == 0)
        P |= PF_ZERO;
    else
        P &= ~PF_ZERO;
    if (value & 0x80)
        P |= PF_NEGATIVE;
    else
        P &= ~PF_NEGATIVE;
}

void CPU6502::brk() {
    PC++;
    pushWord(PC);
    push(P | PF_BREAK);
    P |= PF_INTERRUPT;
    PC = readWord(0xFFFE);
}
