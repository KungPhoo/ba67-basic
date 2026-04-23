#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

using MEMCELL = uint32_t;


class RomImage {
public:
    RomImage() {
        RAM.resize(0x00020000);
        ROM.resize(0x00010000);
        IO.resize(0x00010000);
        init();
    }

    // http://www.harries.dk/files/C64MemoryMaps.pdf
    // read from memory, take care of bank switching
    inline MEMCELL& readBankedMem(size_t address) {
        MEMCELL port = RAM[1];

        bool LORAM  = (port & 1) != 0;
        bool HIRAM  = (port & 2) != 0;
        bool CHAREN = (port & 4) != 0;
        if (address >= 0xA000 && address <= 0xBFFF && LORAM) {
            if (
                (LORAM && HIRAM && CHAREN)
                || (LORAM && HIRAM && !CHAREN) // see pdf
            ) {
                return ROM[address]; // BASIC
            }
        } else if (address >= 0xE000 && HIRAM) {
            return ROM[address]; // KERNAL
        } else if (address >= 0xD000 && address <= 0xDFFF && CHAREN) {
            if (CHAREN && (HIRAM || LORAM)) {
                return IO[address - 0xD000]; // I/O visible
            } else if (HIRAM || LORAM) {
                return ROM[address]; // character ROM
            }
        }
        return RAM[address];
    }

    std::vector<MEMCELL> RAM; // 128k RAM
    std::vector<MEMCELL> ROM; //  64k BASIC, KERNAL, CHARGEN - ROMS
    std::vector<MEMCELL> IO; //  64k VIC, ColorRAM, IO
protected:
    void init();
};
