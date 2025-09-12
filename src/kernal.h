
// https://www.pagetable.com/c64ref/c64mem/
struct KernalAddress {
    const size_t BLNSW = 0x00CC; // Cursor Blink Enable: 0=Flash Cursor
    const size_t RVS   = 0x00C7; // Reverse Mode on? 1 on, 0: off
    const size_t PNTR  = 0x00D3; // Current column on current line (cursor x)
    const size_t LNMX  = 0x00D5; // max cursor x of physical screen line (39 or 79)
    const size_t TBLX  = 0x00D6; // Current Cursor Physical Line Number (cursor y)
    const size_t LDTB1 = 0x00D9; // Screen Line Link Table
    // const size_t COLRAM_PTR = 0x00F3; // F3,F4 pointer to color ram or current line
    const size_t COLOR = 0x0286; // current text foreground color

    const size_t CHARRAM = 0x0400; // hard coded screen character ram (should be read from $0288 HIBASE)
    const size_t COLRAM  = 0xD800; // hard coded color ram (should be read from $00f3,$00f4, but on C64 it's hard coded)

    const size_t VIC_BORDER = 0xD020; // color of border
    const size_t VIC_BKGND  = 0xD021; // color of screen background
};

inline constexpr KernalAddress krnl {};
