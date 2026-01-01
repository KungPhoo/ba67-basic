// Full C64 6502 Emulator with all documented opcodes
#include <cstdint>

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

private:
    enum AddrMode {
        IMM, // Immediate
        ACC, // accumulator variants
        ZP, // Zero Page
        ZPX, // Zero Page,X
        ZPY, // Zero Page,Y
        ABS, // Absolute
        ABSX, // Absolute,X
        ABSY, // Absolute,Y
        INDX, // (Zero Page,X)
        INDY // (Zero Page),Y
    };

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
#if _DEBUG
        if (memory[PC] > 0xff) {
            int pause = 1;
        }
#endif
        return uint8_t(memory[PC++]);
    }
    uint16_t fetchWord() {
        uint8_t lo = fetchByte();
        uint8_t hi = fetchByte();
        return (hi << 8) | lo;
    }
    uint16_t readWord(uint16_t addr) { return (memory[addr] & 0xff) | ((memory[addr + 1] & 0xff) << 8); }

    inline void setByte(uint16_t addr, uint8_t byte) {
        memory[addr] = byte;
    }


    inline bool getFlag(uint8_t f) const { return (P & f) != 0; }
    inline void clearFlag(uint8_t f) { P &= ~f; }
    inline void setFlag(uint8_t f) { P |= f; }
    inline void setFlag(uint8_t f, bool cond = true) {
        if (cond) {
            P |= f; // set the bit
        } else {
            P &= ~f; // clear the bit
        }
    }
    void setZN(uint8_t value);

    void brk();
};
