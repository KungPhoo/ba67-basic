// Full C64 6502 Emulator with all documented opcodes
#include <cstdint>
#include <string>
#include <unordered_map>

using MEMCELL = uint32_t;
class CPU6502 {
public:
    // Registers
    uint8_t A = 0, X = 0, Y = 0, SP = 0xFF, P = 0x24;
    uint16_t PC     = 0;
    MEMCELL* memory = nullptr;
    uint8_t opcode;
    bool cpuJam = false;


    void reset();

    // use this to set the start address foe the
    // program
    bool sys(uint16_t address);



    // The program counter PC is the 0 based pointer in
    // memory. Call this function to execute one ASM statement.
    // -returns: false -> bad opcode. Read the opcode and PC for
    // more details.
    bool executeNext();

    void rts();

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

    inline void clearFlag(uint8_t f) { P &= ~f; }
    inline void setFlag(uint8_t f) { P |= f; }

    enum AddrMode {
        IMP,
        IMM, // Immediate
        ACC, // accumulator variants
        ZPG, // Zero Page
        ZPX, // Zero Page,X
        ZPY, // Zero Page,Y
        ABS, // Absolute
        ABSX, // Absolute,X
        ABSY, // Absolute,Y
        IND, // JMP (indirect)
        INDX, // (Zero Page,X)
        INDY, // (Zero Page),Y
        REL, // branches
        INVALID
    };
    static const char* modeStr(AddrMode m) {
        static const char* t[] = {
            "       ",
            "#      ", "A      ",
            "ZPG    ", "ZPG,X  ", "ZPG,Y  ",
            "ABS    ", "ABS,X  ", "ABS,Y  ",
            "(IND)  ", "(IND,X)", "(IND),Y",
            "REL    ", "???????"
        };
        return t[m];
    }
    struct OpCodeInfo {
        const char* mnemonic;
        AddrMode admode;
        int length;
    };
    static OpCodeInfo getOpcodeInfo(uint8_t op);

    struct BreakPoint {
        bool onRead = false, onWrite = false, onExec = true;
    };

    std::unordered_map<uint16_t, BreakPoint> breakpoints;
    bool breakPointHit = false; // last operation triggered a breakpoint

private:
    uint8_t fetchOperand(AddrMode mode);
    void push(uint8_t value) { memory[0x0100 + SP--] = value; }
    void pushWord(uint16_t value) {
        push(value >> 8);
        push(value & 0xFF);
    }
    uint8_t pop() { return uint8_t(memory[0x0100 + ++SP]); }
    uint16_t popWord() {
        uint8_t lo = pop();
        uint8_t hi = pop();
        return (hi << 8) | lo;
    }

    uint8_t fetchByte() {
        auto it = breakpoints.find(PC);
        if (it != breakpoints.end() && it->second.onRead) {
            breakPointHit = true;
        }
        return uint8_t(memory[PC++]);
    }
    uint16_t fetchWord() {
        uint8_t lo = fetchByte();
        uint8_t hi = fetchByte();
        return (hi << 8) | lo;
    }
    uint16_t readWord(uint16_t addr) { return (memory[addr] & 0xff) | ((memory[addr + 1] & 0xff) << 8); }

    inline void setByte(uint16_t addr, uint8_t byte) {
        auto it = breakpoints.find(addr);
        if (it != breakpoints.end() && it->second.onWrite) {
            breakPointHit = true;
        }

        if (addr == 0x0286) { // BACKGROUND_COLOR_ALSO_SEE_HERE
            byte |= (memory[0xD021] << 4);
        }
        memory[addr] = byte;
    }

    inline void setPC(uint16_t addr) {
        PC = addr;
#if _DEBUG
        printState();
#endif
    }


    inline bool getFlag(uint8_t f) const { return (P & f) != 0; }
    inline void setFlag(uint8_t f, bool cond = true) {
        if (cond) {
            P |= f; // set the bit
        } else {
            P &= ~f; // clear the bit
        }
    }
    void setZN(uint8_t value);

    void brk();

    void printState();

public:
    std::string disassemble(uint16_t address, bool showBytes = true);

    static AddrMode parseOperand(const char*& s,
                                 const char* mnemonic,
                                 uint16_t& value,
                                 int16_t pc);

    int emitBytes(AddrMode m, uint16_t v, uint8_t* out);

    int16_t assemble(const char* line, int16_t addr);
    std::string registers();
};
