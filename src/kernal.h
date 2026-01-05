
// https://www.pagetable.com/c64ref/c64mem/
struct KernalAddress {
    const size_t TXTPTR = 0x007A; // 7A,7B pointer to where CBM BASIC interpreter continues execution
    const size_t STATUS = 0x0090; // Kernal I/O status for file IO

    const size_t DFLTI = 0x0099; // Default input device (0=keyboard)
    const size_t DFLTO = 0x009A; // Default output device (3=screen)

    const size_t NDX = 0x00C6; // Number of Characters in Keyboard Buffer (Queue)

    const size_t BLNSW = 0x00CC; // Cursor Blink Enable: 0=Flash Cursor
    const size_t RVS   = 0x00C7; // Reverse Mode on? 1 on, 0: off

    const size_t PNT   = 0x00D1; // 2 byte pointer to start address of screen line of cursor position
    const size_t PNTR  = 0x00D3; // Current column on current line (cursor x)
    const size_t LNMX  = 0x00D5; // max cursor x of physical screen line (39 or 79). Kernal E58C changes this when printing.
    const size_t TBLX  = 0x00D6; // Current Cursor Physical Line Number (cursor y)
    const size_t LDTB1 = 0x00D9; // Screen Line Link Table

    // const size_t COLRAM_PTR = 0x00F3; // F3,F4 pointer to color ram or current line

    const size_t KEYD   = 0x0277; // keyboard buffer [9]
    const size_t COLOR  = 0x0286; // current text foreground color
    const size_t HIBASE = 0x0288; // top page of screen memory (currently not used, but KERNAL needs it)

    const size_t CHARRAM = 0x0400; // hard coded screen character ram (should be read from $0288 HIBASE)

    const size_t BUF = 0x0200; // text input buffer for INPUT and immediate mode BASIC


    const size_t NEWSTT = 0xA7AE; // BASIC new statement fetcher

    const size_t COLRAM = 0xD800; // hard coded color ram (should be read from $00f3,$00f4, but on C64 it's hard coded)

    const size_t VIC_BORDER = 0xD020; // color of border
    const size_t VIC_BKGND  = 0xD021; // color of screen background
};

inline constexpr KernalAddress krnl {};
