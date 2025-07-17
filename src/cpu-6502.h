// Full C64 6502 Emulator with all documented opcodes
#include <cstdint>

class CPU6502 {
public:
    // Registers
    uint8_t A = 0, X = 0, Y = 0, SP = 0xFF, P = 0x24;
    uint16_t PC     = 0;
    uint8_t* memory = nullptr;
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

private:
    void push(uint8_t value) { memory[0x0100 + SP--] = value; }
    void pushWord(uint16_t value) {
        push(value >> 8);
        push(value & 0xFF);
    }
    uint8_t pop() { return memory[0x0100 + ++SP]; }
    uint16_t popWord() {
        uint8_t lo = pop();
        uint8_t hi = pop();
        return (hi << 8) | lo;
    }

    uint8_t fetchByte() { return memory[PC++]; }
    uint16_t fetchWord() {
        uint8_t lo = fetchByte();
        uint8_t hi = fetchByte();
        return (hi << 8) | lo;
    }
    uint16_t readWord(uint16_t addr) { return memory[addr] | (memory[addr + 1] << 8); }

    void setZN(uint8_t value);

    void brk();
};
