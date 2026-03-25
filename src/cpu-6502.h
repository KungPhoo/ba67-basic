// Full C64 6502 Emulator with all documented opcodes
#include <cstdint>
#include <string>
#include <unordered_map>
#include "kernal.h"
#include "rom.h"

class CPU6502 : public RomImage {
public:
    // RomImage mem;

    // Registers
    uint8_t A = 0, X = 0, Y = 0, SP = 0xFF, P = 0x24;
    uint16_t PC = 0;
    // MEMCELL* memory = nullptr;
    uint8_t opcode;
    bool cpuJam        = false;
    bool enableHeatmap = true; // collect BASIC line cycle count

    size_t cycleCount        = 0;
    size_t lastCycleSnapshot = 0;
    std::unordered_map<uint16_t, size_t> heatmap; // CPU cycles per BASIC line

    // VIC2 raster position
    // STA $D012 - Interrupt me at this line
    // LDA $D012 - Where is the beam right now?
    // Same address. Two roles.
    uint16_t vic_current_raster; // internal (not directly writable)

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
        uint8_t length;
        uint8_t cycles;
    };
    static const OpCodeInfo& getOpcodeInfo(uint8_t op);

    struct BreakPoint {
        bool stop   = true; // otherwise just trace
        bool onRead = false, onWrite = false, onExec = false;
    };

    std::unordered_map<uint16_t, BreakPoint> breakpoints;
    bool breakPointHit = false; // last operation triggered a breakpoint
    bool nmiPending    = false;
    bool irqPending    = false;

    // use this to set a byte with the CPU
    // -trigger breakpoints
    inline void setByte(uint16_t addr, uint8_t byte) {
        if (!breakpoints.empty()) {
            auto it = breakpoints.find(PC);
            if (it != breakpoints.end() && it->second.onRead) {
                breakPointHit = true;
            }
        }


        switch (addr) {
        case krnl.COLOR: // BACKGROUND_COLOR_ALSO_SEE_HERE
            byte |= (IO[krnl.VIC_BKGND] << 4); // poke new foreground and current background as text color
            RAM[addr] = byte;
            break;
        case krnl.VIC_IRQ:
            RAM[addr] &= ~byte;
            updateVicIrq();
            break;
        case krnl.VIC_IRQ_ENA:
            RAM[addr] = byte;
            break;
        // case krnl.VIC_RASTER; // set raster interrupt trigger
        default:
            if (addr >= 0xD000 && addr <= 0xDFFF) {
                MEMCELL port = RAM[1];
                bool LORAM   = (port & 1) != 0;
                bool HIRAM   = (port & 2) != 0;
                bool CHAREN  = (port & 4) != 0;
                if (CHAREN && (HIRAM || LORAM)) {
                    IO[addr] = byte;
                    return;
                }
            }
            RAM[addr] = byte;
        }
    }

    // use this to get a byte with the CPU
    // -bank switching
    // -VIC registers
    // -trigger breakpoints
    inline uint8_t readByte(uint16_t addr) {
        if (!breakpoints.empty()) {
            auto it = breakpoints.find(PC);
            if (it != breakpoints.end() && it->second.onRead) {
                breakPointHit = true;
            }
        }
        switch (addr) {
        case krnl.VIC_RASTER: return uint8_t(vic_current_raster & 0xff);
        case krnl.STKEY:      {
            // when STOP key was pressed, report that, but reset the flag
            uint8_t b = RAM[addr];
            RAM[addr] = 0xff;
            return b;
        }
        }
        return uint8_t(readBankedMem(addr));
    }

private:
    void updateRasterBeam();

    uint8_t fetchOperand(AddrMode mode);
    void handle_interrupt(uint16_t vector, bool is_brk);
    void push(uint8_t value) {
        RAM[0x0100 + SP--] = value;
    }
    void pushWord(uint16_t value) {
        push(value >> 8);
        push(value & 0xFF);
    }
    uint8_t pop() { return uint8_t(RAM[0x0100 + ++SP]); }
    uint16_t popWord() {
        uint8_t lo = pop();
        uint8_t hi = pop();
        return (hi << 8) | lo;
    }

    uint8_t fetchByte() {
        return readByte(PC++);
    }
    uint16_t fetchWord() {
        uint8_t lo = fetchByte();
        uint8_t hi = fetchByte();
        return (hi << 8) | lo;
    }
    uint16_t readWord(uint16_t addr) { return readByte(addr) | (readByte(addr + 1) << 8); }


    inline void setPC(uint16_t addr) {
        PC = addr;
#if _DEBUG
        printState();
#endif
    }

    inline void branchTo(uint16_t addr) {
        ++cycleCount;
        // check if page boundary crossed
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycleCount++; // extra cycle on page cross
        }
        PC = addr;
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
    void printState();

public:
    void updateVicIrq() {
        // interrupt_status & vic.interrupt_enable) != 0;
        irqPending = (RAM[krnl.VIC_IRQ] & RAM[krnl.VIC_IRQ_ENA]) != 0;
    }

    std::string disassemble(uint16_t address, bool showBytes = true);

    static AddrMode parseOperand(const char*& s,
                                 const char* mnemonic,
                                 uint16_t& value,
                                 int16_t pc);

    int emitBytes(AddrMode m, uint16_t v, uint8_t* out);

    int16_t assemble(const char* line, int16_t addr);
    std::string registers();

    void printHeatMap();

private:
    void updateBasicHeatmap();
};
