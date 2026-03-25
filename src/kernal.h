#pragma once

// https://www.pagetable.com/c64ref/c64mem/
struct KernalAddress {
    const size_t TXTPTR = 0x007A; // 7A,7B pointer to where CBM BASIC interpreter continues execution
    const size_t STATUS = 0x0090; // Kernal I/O status for file IO
    const size_t STKEY = 0x0091; // STOP key pressed? 0xFF no key. 0x7F STOP was pressed. On C64 Jiffy clock sets this

    const size_t DFLTI = 0x0099; // Default input device (0=keyboard)
    const size_t DFLTO = 0x009A; // Default output device (3=screen)

    const size_t TIME = 0x00A0; // Jiffy clock - 3 bytes


    const size_t NDX = 0x00C6; // Number of Characters in Keyboard Buffer (Queue)

    const size_t BLNSW = 0x00CC; // Cursor Blink Enable: 0=Flash Cursor
    const size_t RVS   = 0x00C7; // Reverse Mode on? 1 on, 0: off

    const size_t PNT   = 0x00D1; // 2 byte pointer to start address of screen line of cursor position
    const size_t PNTR  = 0x00D3; // Current column on current line (cursor x)
    const size_t LNMX  = 0x00D5; // max cursor x of physical screen line (39 or 79). Kernal E58C changes this when printing.
    const size_t TBLX  = 0x00D6; // Current Cursor Physical Line Number (cursor y)
    const size_t LDTB1 = 0x00D9; // Screen Line Link Table

    const size_t CLRPNT = 0x00F3; // 2 byte pointer to start address of color ram line of cursor position

    // const size_t COLRAM_PTR = 0x00F3; // F3,F4 pointer to color ram or current line
    const size_t BUF = 0x0200; // text input buffer for INPUT and immediate mode BASIC

    const size_t KEYD = 0x0277; // keyboard buffer [9]

    const size_t MEMSTR_ON_RST = 0x0281; // Pointer to start of BASIC memory on reset
    const size_t MEMSIZ_ON_RST = 0x0283; // Pointer to end of BASIC memory on reset


    const size_t COLOR  = 0x0286; // current text foreground color
    const size_t HIBASE = 0x0288; // top page of screen memory (currently only used in LDTB1)

    const size_t CHARRAM = 0x0400; // hard coded screen character ram (should be read from $0288 HIBASE)

    const size_t BASICCODE = 0x0801; // BASIC program code address. TXTTAB $2B/$2C points to it

    // <-- BASIC CODE AREA -->
    const size_t BASICEND  = 0xA000; // first address that cannot be used for BASIC code anymore
    const size_t BASIC_ROM = 0xA000; // BASIC ROM address

    const size_t NEWSTT = 0xA7AE; // BASIC new statement fetcher


    const size_t VIC_CTRL1   = 0xD011; // various bits
    const size_t VIC_RASTER  = 0xD012; // raster interrupt position (when to trigger)
    const size_t VIC_IRQ     = 0xD019;
    const size_t VIC_IRQ_ENA = 0xD01A;
    const size_t VIC_BORDER  = 0xD020; // color of border
    const size_t VIC_BKGND   = 0xD021; // color of screen background

    const size_t CHARGEN = 0xD000; // char gen rom

    const size_t COLRAM = 0xD800; // hard coded color ram (should be read from $00f3,$00f4, but on C64 it's hard coded)

    const size_t KERNAL_ROM = 0xE000;

    // screen size
    const size_t LLEN   = 0xE506; // 40 columns - hardcoded
    const size_t NLINES = 0xE508; // 25 lines   - hardcoded

    const size_t REVISION = 0xFF80; // KERNAL revision byte
};

inline constexpr KernalAddress krnl {};
