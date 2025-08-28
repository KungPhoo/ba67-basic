#include "font-bitmap-data.h"

// Keep this file very simple.
// cc1plus might crash with "out of memory" on the Raspberry Pi 400, otherwise.

// set the first number Unicode code point to 0xffffffff to stop reading
// the next block are 8 bytes, top to bottom for each line of the 8x8 character.
static FontDataBits::DataStruct staticTable[] = {
    {       0x00,                { 126, 66, 66, 66, 66, 66, 126, 0 } },
    {       0x09,                         { 0, 0, 0, 0, 0, 0, 0, 0 } },

    {       0x11, { 0x00, 0x18, 0x18, 0x18, 0x7e, 0x3c, 0x18, 0x00 } }, // cursor down
    {       0x1d, { 0x00, 0x08, 0x0c, 0x7e, 0x7e, 0x0c, 0x08, 0x00 } }, // cursor right
    {       0x91, { 0x00, 0x18, 0x3c, 0x7e, 0x18, 0x18, 0x18, 0x00 } }, // cursor up
    {       0x9d, { 0x00, 0x10, 0x30, 0x7e, 0x7e, 0x30, 0x10, 0x00 } }, // cursor left

    {       0x13, { 0x00, 0x3c, 0x7e, 0xff, 0x4e, 0x4e, 0x4e, 0x00 } }, // home
    {       0x14, { 0x00, 0x3e, 0x7e, 0xfe, 0x7e, 0x3e, 0x00, 0x00 } }, // delete
    {       0x93, { 0xff, 0xcb, 0xbb, 0xbb, 0xbb, 0xbb, 0xc9, 0xff } }, // clear
    // 0x94 insert

    {       0x12, { 0xff, 0xc1, 0xbd, 0xc1, 0xed, 0xdd, 0xbd, 0xff } }, // reverse on
    {       0x92, { 0xff, 0x83, 0xbd, 0x83, 0xb7, 0xbb, 0xbd, 0xff } }, // reverse off

    {       0x05, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (white)
    {       0x1c, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (red)
    {       0x1e, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (green)
    {       0x1f, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (blue)
    {       0x81, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (orange)
    {       0x90, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (black)
    {       0x95, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (brown)
    {       0x96, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (pink/light red)
    {       0x97, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (dark gray)
    {       0x98, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (gray)
    {       0x99, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (light green)
    {       0x9a, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (light blue)
    {       0x9b, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (light gray)
    {       0x9c, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (purple)
    {       0x9e, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (yellow)
    {       0x9f, { 0x7e, 0x81, 0xff, 0x95, 0x85, 0x85, 0x81, 0x7e } }, // color (cyan)


// Basic Latin
#if 0  // char8.js (tiny adjustments) - that's the alternative C128 font
  { 0x00020, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // SPACE
  { 0x00021, { 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00 } }, // EXCLAMATION MARK
  { 0x00022, { 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // QUOTATION MARK
  { 0x00023, { 0x66, 0x66, 0xff, 0x66, 0xff, 0x66, 0x66, 0x00 } }, // NUMBER SIGN
  { 0x00024, { 0x18, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x18, 0x00 } }, // DOLLAR SIGN
  { 0x00025, { 0x62, 0x66, 0x0c, 0x18, 0x30, 0x66, 0x46, 0x00 } }, // PERCENT SIGN
  { 0x00026, { 0x3c, 0x66, 0x3c, 0x38, 0x67, 0x66, 0x3f, 0x00 } }, // AMPERSAND
  { 0x00027, { 0x06, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // APOSTROPHE
  { 0x00028, { 0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00 } }, // LEFT PARENTHESIS
  { 0x00029, { 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00 } }, // RIGHT PARENTHESIS
  { 0x0002A, { 0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00 } }, // ASTERISK
  { 0x0002B, { 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00 } }, // PLUS SIGN
  { 0x0002C, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30 } }, // COMMA
  { 0x0002D, { 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00 } }, // HYPHEN-MINUS
  { 0x0002E, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00 } }, // FULL STOP
  { 0x0002F, { 0x00, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00 } }, // SOLIDUS
  { 0x00030, { 0x3c, 0x42, 0x46, 0x5a, 0x62, 0x42, 0x3c, 0x00 } }, // DIGIT ZERO
  { 0x00031, { 0x08, 0x18, 0x28, 0x08, 0x08, 0x08, 0x3e, 0x00 } }, // DIGIT ONE
  { 0x00032, { 0x3c, 0x42, 0x02, 0x0c, 0x30, 0x40, 0x7e, 0x00 } }, // DIGIT TWO
  { 0x00033, { 0x3c, 0x42, 0x02, 0x1c, 0x02, 0x42, 0x3c, 0x00 } }, // DIGIT THREE
  { 0x00034, { 0x04, 0x0c, 0x14, 0x24, 0x7e, 0x04, 0x04, 0x00 } }, // DIGIT FOUR
  { 0x00035, { 0x7e, 0x40, 0x7c, 0x02, 0x02, 0x42, 0x3c, 0x00 } }, // DIGIT FIVE
  { 0x00036, { 0x1c, 0x20, 0x40, 0x7c, 0x42, 0x42, 0x3c, 0x00 } }, // DIGIT SIX
  { 0x00037, { 0x7e, 0x02, 0x04, 0x08, 0x10, 0x10, 0x10, 0x00 } }, // DIGIT SEVEN
  { 0x00038, { 0x3c, 0x42, 0x42, 0x3c, 0x42, 0x42, 0x3c, 0x00 } }, // DIGIT EIGHT
  { 0x00039, { 0x3c, 0x42, 0x42, 0x3e, 0x02, 0x04, 0x38, 0x00 } }, // DIGIT NINE
  { 0x0003A, { 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00 } }, // COLON
  { 0x0003B, { 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30 } }, // SEMICOLON
  { 0x0003C, { 0x0e, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0e, 0x00 } }, // LESS-THAN SIGN
  { 0x0003D, { 0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00 } }, // EQUALS SIGN
  { 0x0003E, { 0x70, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x70, 0x00 } }, // GREATER-THAN SIGN
  { 0x0003F, { 0x3c, 0x66, 0x06, 0x0c, 0x18, 0x00, 0x18, 0x00 } }, // QUESTION MARK
  { 0x00040, { 0x3c, 0x66, 0x6e, 0x6e, 0x60, 0x62, 0x3c, 0x00 } }, // COMMERCIAL AT
  { 0x00041, { 0x18, 0x24, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x00 } }, // LATIN CAPITAL LETTER A
  { 0x00042, { 0x7c, 0x22, 0x22, 0x3c, 0x22, 0x22, 0x7c, 0x00 } }, // LATIN CAPITAL LETTER B
  { 0x00043, { 0x1c, 0x22, 0x40, 0x40, 0x40, 0x22, 0x1c, 0x00 } }, // LATIN CAPITAL LETTER C
  { 0x00044, { 0x78, 0x24, 0x22, 0x22, 0x22, 0x24, 0x78, 0x00 } }, // LATIN CAPITAL LETTER D
  { 0x00045, { 0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER E
  { 0x00046, { 0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x40, 0x00 } }, // LATIN CAPITAL LETTER F
  { 0x00047, { 0x1c, 0x22, 0x40, 0x4e, 0x42, 0x22, 0x1c, 0x00 } }, // LATIN CAPITAL LETTER G
  { 0x00048, { 0x42, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x00 } }, // LATIN CAPITAL LETTER H
  { 0x00049, { 0x1c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00 } }, // LATIN CAPITAL LETTER I
  { 0x0004A, { 0x0e, 0x04, 0x04, 0x04, 0x04, 0x44, 0x38, 0x00 } }, // LATIN CAPITAL LETTER J
  { 0x0004B, { 0x42, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0x00 } }, // LATIN CAPITAL LETTER K
  { 0x0004C, { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER L
  { 0x0004D, { 0x42, 0x66, 0x5a, 0x5a, 0x42, 0x42, 0x42, 0x00 } }, // LATIN CAPITAL LETTER M
  { 0x0004E, { 0x42, 0x62, 0x52, 0x4a, 0x46, 0x42, 0x42, 0x00 } }, // LATIN CAPITAL LETTER N
  { 0x0004F, { 0x18, 0x24, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00 } }, // LATIN CAPITAL LETTER O
  { 0x00050, { 0x7c, 0x42, 0x42, 0x7c, 0x40, 0x40, 0x40, 0x00 } }, // LATIN CAPITAL LETTER P
  { 0x00051, { 0x18, 0x24, 0x42, 0x42, 0x4a, 0x24, 0x1a, 0x00 } }, // LATIN CAPITAL LETTER Q
  { 0x00052, { 0x7c, 0x42, 0x42, 0x7c, 0x48, 0x44, 0x42, 0x00 } }, // LATIN CAPITAL LETTER R
  { 0x00053, { 0x3c, 0x42, 0x40, 0x3c, 0x02, 0x42, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER S
  { 0x00054, { 0x3e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00 } }, // LATIN CAPITAL LETTER T
  { 0x00055, { 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER U
  { 0x00056, { 0x42, 0x42, 0x42, 0x24, 0x24, 0x18, 0x18, 0x00 } }, // LATIN CAPITAL LETTER V
  { 0x00057, { 0x42, 0x42, 0x42, 0x5a, 0x5a, 0x66, 0x42, 0x00 } }, // LATIN CAPITAL LETTER W
  { 0x00058, { 0x42, 0x42, 0x24, 0x18, 0x24, 0x42, 0x42, 0x00 } }, // LATIN CAPITAL LETTER X
  { 0x00059, { 0x22, 0x22, 0x22, 0x1c, 0x08, 0x08, 0x08, 0x00 } }, // LATIN CAPITAL LETTER Y
  { 0x0005A, { 0x7e, 0x02, 0x04, 0x18, 0x20, 0x40, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER Z
  { 0x0005B, { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 } }, // LEFT SQUARE BRACKET
  { 0x0005C, { 0x00, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x00 } }, // REVERSE SOLIDUS
  { 0x0005D, { 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00 } }, // RIGHT SQUARE BRACKET
  { 0x0005E, { 0x08, 0x1c, 0x36, 0x22, 0x00, 0x00, 0x00, 0x00 } }, // CIRCUMFLEX ACCENT
  { 0x0005F, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00 } }, // LOW LINE
#else  // C64 with slight improvements on m ' ´ ` ~
    {    0x00020, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // SPACE
    {    0x00021, { 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00 } }, // EXCLAMATION MARK
    {    0x00022, { 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // QUOTATION MARK
    {    0x00023, { 0x66, 0x66, 0xff, 0x66, 0xff, 0x66, 0x66, 0x00 } }, // NUMBER SIGN
    {    0x00024, { 0x18, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x18, 0x00 } }, // DOLLAR SIGN
    {    0x00025, { 0x62, 0x66, 0x0c, 0x18, 0x30, 0x66, 0x46, 0x00 } }, // PERCENT SIGN
    {    0x00026, { 0x3c, 0x66, 0x3c, 0x38, 0x67, 0x66, 0x3f, 0x00 } }, // AMPERSAND
    {    0x00027, { 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // APOSTROPHE
    {    0x00028, { 0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00 } }, // LEFT PARENTHESIS
    {    0x00029, { 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00 } }, // RIGHT PARENTHESIS
    {    0x0002A, { 0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00 } }, // ASTERISK
    {    0x0002B, { 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00 } }, // PLUS SIGN
    {    0x0002C, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30 } }, // COMMA
    {    0x0002D, { 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00 } }, // HYPHEN-MINUS
    {    0x0002E, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00 } }, // FULL STOP
    {    0x0002F, { 0x00, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00 } }, // SOLIDUS
    {    0x00030, { 0x3c, 0x66, 0x6e, 0x76, 0x66, 0x66, 0x3c, 0x00 } }, // DIGIT ZERO
    {    0x00031, { 0x18, 0x18, 0x38, 0x18, 0x18, 0x18, 0x7e, 0x00 } }, // DIGIT ONE
    {    0x00032, { 0x3c, 0x66, 0x06, 0x0c, 0x30, 0x60, 0x7e, 0x00 } }, // DIGIT TWO
    {    0x00033, { 0x3c, 0x66, 0x06, 0x1c, 0x06, 0x66, 0x3c, 0x00 } }, // DIGIT THREE
    {    0x00034, { 0x06, 0x0e, 0x1e, 0x66, 0x7f, 0x06, 0x06, 0x00 } }, // DIGIT FOUR
    {    0x00035, { 0x7e, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c, 0x00 } }, // DIGIT FIVE
    {    0x00036, { 0x3c, 0x66, 0x60, 0x7c, 0x66, 0x66, 0x3c, 0x00 } }, // DIGIT SIX
    {    0x00037, { 0x7e, 0x66, 0x0c, 0x18, 0x18, 0x18, 0x18, 0x00 } }, // DIGIT SEVEN
    {    0x00038, { 0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00 } }, // DIGIT EIGHT
    {    0x00039, { 0x3c, 0x66, 0x66, 0x3e, 0x06, 0x66, 0x3c, 0x00 } }, // DIGIT NINE
    {    0x0003A, { 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00 } }, // COLON
    {    0x0003B, { 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30 } }, // SEMICOLON
    {    0x0003C, { 0x00, 0x07, 0x1c, 0x70, 0x1c, 0x07, 0x00, 0x00 } }, // LESS-THAN SIGN
    {    0x0003D, { 0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00 } }, // EQUALS SIGN
    {    0x0003E, { 0x00, 0x70, 0x1c, 0x07, 0x1c, 0x70, 0x00, 0x00 } }, // GREATER-THAN SIGN
    {    0x0003F, { 0x3c, 0x66, 0x06, 0x0c, 0x18, 0x00, 0x18, 0x00 } }, // QUESTION MARK
    {    0x00040, { 0x3c, 0x66, 0x6e, 0x6e, 0x60, 0x62, 0x3c, 0x00 } }, // COMMERCIAL AT
    {    0x00041, { 0x18, 0x3c, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER A
    {    0x00042, { 0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00 } }, // LATIN CAPITAL LETTER B
    {    0x00043, { 0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER C
    {    0x00044, { 0x78, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00 } }, // LATIN CAPITAL LETTER D
    {    0x00045, { 0x7e, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER E
    {    0x00046, { 0x7e, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00 } }, // LATIN CAPITAL LETTER F
    {    0x00047, { 0x3c, 0x66, 0x60, 0x6e, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER G
    {    0x00048, { 0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER H
    {    0x00049, { 0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER I
    {    0x0004A, { 0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0x6c, 0x38, 0x00 } }, // LATIN CAPITAL LETTER J
    {    0x0004B, { 0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00 } }, // LATIN CAPITAL LETTER K
    {    0x0004C, { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER L
    {    0x0004D, { 0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00 } }, // LATIN CAPITAL LETTER M
    {    0x0004E, { 0x66, 0x76, 0x7e, 0x7e, 0x6e, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER N
    {    0x0004F, { 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER O
    {    0x00050, { 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x00 } }, // LATIN CAPITAL LETTER P
    {    0x00051, { 0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x0e, 0x00 } }, // LATIN CAPITAL LETTER Q
    {    0x00052, { 0x7c, 0x66, 0x66, 0x7c, 0x78, 0x6c, 0x66, 0x00 } }, // LATIN CAPITAL LETTER R
    {    0x00053, { 0x3c, 0x66, 0x60, 0x3c, 0x06, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER S
    {    0x00054, { 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 } }, // LATIN CAPITAL LETTER T
    {    0x00055, { 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER U
    {    0x00056, { 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00 } }, // LATIN CAPITAL LETTER V
    {    0x00057, { 0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00 } }, // LATIN CAPITAL LETTER W
    {    0x00058, { 0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER X
    {    0x00059, { 0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00 } }, // LATIN CAPITAL LETTER Y
    {    0x0005A, { 0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER Z
    {    0x0005B, { 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00 } }, // LEFT SQUARE BRACKET
    {    0x0005C, { 0x00, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x00 } }, // REVERSE SOLIDUS
    {    0x0005D, { 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00 } }, // RIGHT SQUARE BRACKET
    {    0x0005E, { 0x08, 0x1c, 0x36, 0x22, 0x00, 0x00, 0x00, 0x00 } }, // CIRCUMFLEX ACCENT
    {    0x0005F, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00 } }, // LOW LINE
    {    0x00060, { 0x30, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // GRAVE ACCENT
    // {    0x00061, { 0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER A
    // {    0x00062, { 0x00, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x7c, 0x00 } }, // LATIN SMALL LETTER B
    // {    0x00063, { 0x00, 0x00, 0x3c, 0x60, 0x60, 0x60, 0x3c, 0x00 } }, // LATIN SMALL LETTER C
    // {    0x00064, { 0x00, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER D
    // {    0x00065, { 0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00 } }, // LATIN SMALL LETTER E
    // {    0x00066, { 0x00, 0x0e, 0x18, 0x3e, 0x18, 0x18, 0x18, 0x00 } }, // LATIN SMALL LETTER F
    // {    0x00067, { 0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x7c } }, // LATIN SMALL LETTER G
    // {    0x00068, { 0x00, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x00 } }, // LATIN SMALL LETTER H
    // {    0x00069, { 0x00, 0x18, 0x00, 0x38, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN SMALL LETTER I
    // {    0x0006A, { 0x00, 0x06, 0x00, 0x06, 0x06, 0x06, 0x06, 0x3c } }, // LATIN SMALL LETTER J
    // {    0x0006B, { 0x00, 0x60, 0x60, 0x6c, 0x78, 0x6c, 0x66, 0x00 } }, // LATIN SMALL LETTER K
    // {    0x0006C, { 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x1c, 0x00 } }, // LATIN SMALL LETTER L
    // {    0x0006D, { 0x00, 0x00, 0x7c, 0x6a, 0x6a, 0x6a, 0x6a, 0x00 } }, // LATIN SMALL LETTER M
    // {    0x0006E, { 0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00 } }, // LATIN SMALL LETTER N
    // {    0x0006F, { 0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN SMALL LETTER O
    // {    0x00070, { 0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60 } }, // LATIN SMALL LETTER P
    // {    0x00071, { 0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x06 } }, // LATIN SMALL LETTER Q
    // {    0x00072, { 0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x00 } }, // LATIN SMALL LETTER R
    // {    0x00073, { 0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x00 } }, // LATIN SMALL LETTER S
    // {    0x00074, { 0x00, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x0e, 0x00 } }, // LATIN SMALL LETTER T
    // {    0x00075, { 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER U
    // {    0x00076, { 0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00 } }, // LATIN SMALL LETTER V
    // {    0x00077, { 0x00, 0x00, 0x63, 0x6b, 0x7f, 0x3e, 0x36, 0x00 } }, // LATIN SMALL LETTER W
    // {    0x00078, { 0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00 } }, // LATIN SMALL LETTER X
    // {    0x00079, { 0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x0c, 0x78 } }, // LATIN SMALL LETTER Y
    // {    0x0007A, { 0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00 } }, // LATIN SMALL LETTER Z

    {    0x00061, { 0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER A
    {    0x00062, { 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x00 } }, // LATIN SMALL LETTER B
    {    0x00063, { 0x00, 0x00, 0x3c, 0x60, 0x60, 0x60, 0x3c, 0x00 } }, // LATIN SMALL LETTER C
    {    0x00064, { 0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER D
    {    0x00065, { 0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00 } }, // LATIN SMALL LETTER E
    {    0x00066, { 0x0e, 0x18, 0x3e, 0x18, 0x18, 0x18, 0x18, 0x00 } }, // LATIN SMALL LETTER F
    {    0x00067, { 0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x7c } }, // LATIN SMALL LETTER G
    {    0x00068, { 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00 } }, // LATIN SMALL LETTER H
    {    0x00069, { 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN SMALL LETTER I
    {    0x0006A, { 0x06, 0x00, 0x06, 0x06, 0x06, 0x06, 0x06, 0x3c } }, // LATIN SMALL LETTER J
    {    0x0006B, { 0x60, 0x60, 0x6c, 0x6c, 0x78, 0x6c, 0x66, 0x00 } }, // LATIN SMALL LETTER K
    {    0x0006C, { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0e, 0x00 } }, // LATIN SMALL LETTER L
    {    0x0006D, { 0x00, 0x00, 0x7c, 0x6a, 0x6a, 0x6a, 0x6a, 0x00 } }, // LATIN SMALL LETTER M
    {    0x0006E, { 0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00 } }, // LATIN SMALL LETTER N
    {    0x0006F, { 0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN SMALL LETTER O
    {    0x00070, { 0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60 } }, // LATIN SMALL LETTER P
    {    0x00071, { 0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x06 } }, // LATIN SMALL LETTER Q
    {    0x00072, { 0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x00 } }, // LATIN SMALL LETTER R
    {    0x00073, { 0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x00 } }, // LATIN SMALL LETTER S
    {    0x00074, { 0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x0e, 0x00 } }, // LATIN SMALL LETTER T
    {    0x00075, { 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER U
    {    0x00076, { 0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00 } }, // LATIN SMALL LETTER V
    {    0x00077, { 0x00, 0x00, 0x63, 0x6b, 0x7f, 0x3e, 0x36, 0x00 } }, // LATIN SMALL LETTER W
    {    0x00078, { 0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00 } }, // LATIN SMALL LETTER X
    {    0x00079, { 0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x0c, 0x78 } }, // LATIN SMALL LETTER Y
    {    0x0007A, { 0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00 } }, // LATIN SMALL LETTER Z

    {    0x0007B, { 0x1e, 0x18, 0x18, 0x60, 0x18, 0x18, 0x1e, 0x00 } }, // LEFT CURLY BRACKET
    {    0x0007C, { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 } }, // VERTICAL LINE
    {    0x0007D, { 0x78, 0x18, 0x18, 0x06, 0x18, 0x18, 0x78, 0x00 } }, // RIGHT CURLY BRACKET
    {    0x0007E, { 0x00, 0x00, 0x32, 0x7e, 0x4c, 0x00, 0x00, 0x00 } }, // TILDE
#endif


/// ---------------------------- UNICODE ----------------------------///

// Latin-1

#if 0
        { 0xa0, { 0, 0, 0, 0, 0, 0, 0, 0 } },
        { 0xa1, { 0, 8, 0, 0, 8, 8, 8, 8 } },
        { 0xa2, { 0, 8, 30, 40, 40, 30, 8, 0 } },
        { 0xa3, { 24, 36, 32, 112, 32, 98, 92, 0 } },
        { 0xa4, { 0, 90, 36, 36, 90, 0, 0, 0 } },
        { 0xa5, { 34, 34, 20, 62, 8, 62, 8, 0 } },
        { 0xa6, { 8, 8, 8, 0, 0, 8, 8, 8 } },
        { 0xa7, { 60, 64, 60, 66, 60, 2, 60, 0 } },
        { 0xa8, { 36, 0, 0, 0, 0, 0, 0, 0 } },
        { 0xa9, { 28, 34, 77, 81, 77, 34, 28, 0 } },
        { 0xaa, { 24, 4, 28, 36, 26, 0, 0, 0 } },
        { 0xab, { 0, 0, 18, 36, 72, 36, 18, 0 } },
        { 0xac, { 0, 0, 0, 126, 2, 2, 0, 0 } },
        { 0xad, { 0, 0, 0, 126, 0, 0, 0, 0 } },
        { 0xae, { 28, 34, 93, 81, 81, 34, 28, 0 } },
        { 0xaf, { 126, 0, 0, 0, 0, 0, 0, 0 } },
        { 0xb0, { 24, 36, 36, 24, 0, 0, 0, 0 } },
        { 0xb1, { 8, 8, 62, 8, 8, 0, 62, 0 } },
        { 0xb2, { 24, 36, 8, 48, 60, 0, 0, 0 } },
        { 0xb3, { 60, 4, 8, 36, 24, 0, 0, 0 } },
        { 0xb4, { 4, 8, 16, 0, 0, 0, 0, 0 } },
        { 0xb5, { 0, 0, 36, 36, 36, 58, 32, 64 } },
        { 0xb6, { 62, 74, 74, 58, 10, 10, 10, 0 } },
        { 0xb7, { 0, 0, 0, 24, 24, 0, 0, 0 } },
        { 0xb8, { 0, 0, 0, 0, 0, 16, 8, 48 } },
        { 0xb9, { 8, 24, 8, 8, 28, 0, 0, 0 } },
        { 0xba, { 28, 34, 34, 34, 28, 0, 0, 0 } },
        { 0xbb, { 0, 0, 72, 36, 18, 36, 72, 0 } },
        { 0xbc, { 96, 34, 36, 9, 21, 39, 65, 0 } },
        { 0xbd, { 96, 34, 36, 11, 17, 38, 71, 0 } },
        { 0xbe, { 112, 34, 116, 9, 21, 39, 65, 0 } },
        { 0xbf, { 0, 8, 0, 8, 48, 64, 66, 60 } },
        { 0xc0, { 16, 8, 24, 36, 66, 126, 66, 0 } },
        { 0xc1, { 8, 16, 24, 36, 66, 126, 66, 0 } },
        { 0xc2, { 8, 20, 24, 36, 66, 126, 66, 0 } },
        { 0xc3, { 50, 76, 24, 36, 66, 126, 66, 0 } },
        { 0xc4, { 66, 24, 36, 66, 126, 66, 66, 0 } },
        { 0xc5, { 24, 24, 36, 66, 126, 66, 66, 0 } },
        { 0xc6, { 30, 40, 72, 126, 72, 72, 78, 0 } },
        { 0xc7, { 28, 34, 64, 64, 34, 28, 8, 48 } },
        { 0xc8, { 16, 8, 126, 64, 120, 64, 126, 0 } },
        { 0xc9, { 8, 16, 126, 64, 120, 64, 126, 0 } },
        { 0xca, { 8, 20, 126, 64, 120, 64, 126, 0 } },
        { 0xcb, { 36, 0, 126, 64, 120, 64, 126, 0 } },
        { 0xcc, { 8, 4, 28, 8, 8, 8, 28, 0 } },
        { 0xcd, { 8, 16, 28, 8, 8, 8, 28, 0 } },
        { 0xce, { 8, 20, 28, 8, 8, 8, 28, 0 } },
        { 0xcf, { 20, 0, 28, 8, 8, 8, 28, 0 } },
        { 0xd0, { 120, 36, 34, 114, 34, 36, 120, 0 } },
        { 0xd1, { 50, 76, 0, 98, 82, 74, 70, 0 } },
        { 0xd2, { 16, 8, 24, 36, 66, 36, 24, 0 } },
        { 0xd3, { 8, 16, 24, 36, 66, 36, 24, 0 } },
        { 0xd4, { 8, 20, 24, 36, 66, 36, 24, 0 } },
        { 0xd5, { 50, 76, 24, 36, 66, 36, 24, 0 } },
        { 0xd6, { 66, 24, 36, 66, 66, 36, 24, 0 } },
        { 0xd7, { 0, 34, 20, 8, 20, 34, 0, 0 } },
        { 0xd8, { 26, 36, 74, 82, 82, 36, 88, 0 } },
        { 0xd9, { 16, 8, 66, 66, 66, 66, 60, 0 } },
        { 0xda, { 8, 16, 66, 66, 66, 66, 60, 0 } },
        { 0xdb, { 8, 20, 66, 66, 66, 66, 60, 0 } },
        { 0xdc, { 36, 0, 66, 66, 66, 66, 60, 0 } },
        { 0xdd, { 4, 42, 34, 34, 28, 8, 8, 0 } },
        { 0xde, { 0, 32, 60, 34, 34, 60, 32, 0 } },
        { 0xdf, { 60, 66, 66, 76, 66, 66, 76, 64 } },
        { 0xe0, { 32, 16, 56, 4, 60, 68, 58, 0 } },
        { 0xe1, { 8, 16, 56, 4, 60, 68, 58, 0 } },
        { 0xe2, { 8, 20, 56, 4, 60, 68, 58, 0 } },
        { 0xe3, { 50, 76, 56, 4, 60, 68, 58, 0 } },
        { 0xe4, { 36, 0, 56, 4, 60, 68, 58, 0 } },
        { 0xe5, { 24, 0, 56, 4, 60, 68, 58, 0 } },
        { 0xe6, { 0, 0, 52, 10, 62, 72, 54, 0 } },
        { 0xe7, { 0, 0, 60, 66, 64, 60, 8, 48 } },
        { 0xe8, { 16, 8, 60, 66, 126, 64, 60, 0 } },
        { 0xe9, { 8, 16, 60, 66, 126, 64, 60, 0 } },
        { 0xea, { 8, 20, 60, 66, 126, 64, 60, 0 } },
        { 0xeb, { 36, 0, 60, 66, 126, 64, 60, 0 } },
        { 0xec, { 16, 8, 0, 24, 8, 8, 28, 0 } },
        { 0xed, { 8, 16, 0, 24, 8, 8, 28, 0 } },
        { 0xee, { 8, 20, 0, 24, 8, 8, 28, 0 } },
        { 0xef, { 20, 0, 24, 8, 8, 8, 28, 0 } },
        { 0xf0, { 26, 4, 18, 58, 70, 66, 60, 0 } },
        { 0xf1, { 50, 76, 0, 92, 98, 66, 66, 0 } },
        { 0xf2, { 16, 8, 60, 66, 66, 66, 60, 0 } },
        { 0xf3, { 8, 16, 60, 66, 66, 66, 60, 0 } },
        { 0xf4, { 8, 20, 60, 66, 66, 66, 60, 0 } },
        { 0xf5, { 50, 12, 60, 66, 66, 66, 60, 0 } },
        { 0xf6, { 36, 0, 60, 66, 66, 66, 60, 0 } },
        { 0xf7, { 0, 8, 0, 62, 0, 8, 0, 0 } },
        { 0xf8, { 0, 0, 60, 70, 90, 98, 60, 0 } },
        { 0xf9, { 16, 8, 66, 66, 66, 70, 58, 0 } },
        { 0xfa, { 8, 16, 66, 66, 66, 70, 58, 0 } },
        { 0xfb, { 8, 20, 66, 66, 66, 70, 58, 0 } },
        { 0xfc, { 36, 0, 66, 66, 66, 70, 58, 0 } },
        { 0xfd, { 8, 16, 66, 66, 70, 58, 2, 60 } },
        { 0xfe, { 0, 32, 32, 60, 34, 60, 32, 32 } },
        { 0xff, { 36, 0, 66, 66, 70, 58, 2, 60 } },
#else

    {    0x000A0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // NO-BREAK SPACE
    {    0x000A1, { 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 } }, // INVERTED EXCLAMATION MARK
    {    0x000A2, { 0x00, 0x08, 0x3c, 0x68, 0x68, 0x3c, 0x08, 0x00 } }, // CENT SIGN
    {    0x000A3, { 0x0c, 0x12, 0x30, 0x7c, 0x30, 0x62, 0xfc, 0x00 } }, // POUND SIGN
    {    0x000A4, { 0x00, 0x24, 0x18, 0x24, 0x24, 0x18, 0x24, 0x00 } }, // CURRENCY SIGN
    {    0x000A5, { 0x66, 0x3c, 0x18, 0x7e, 0x18, 0x7e, 0x18, 0x00 } }, // YEN SIGN
    {    0x000A6, { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 } }, // BROKEN BAR
    {    0x000A7, { 0x3c, 0x60, 0x3c, 0x66, 0x3c, 0x06, 0x3c, 0x00 } }, // SECTION SIGN
    {    0x000A8, { 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // DIAERESIS
    {    0x000A9, { 0x3c, 0x66, 0x5a, 0x52, 0x5a, 0x66, 0x3c, 0x00 } }, // COPYRIGHT SIGN
    {    0x000AA, { 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00, 0x7e, 0x00 } }, // FEMININE ORDINAL INDICATOR
    {    0x000AB, { 0x00, 0x00, 0x36, 0x6c, 0x6c, 0x36, 0x00, 0x00 } }, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    {    0x000AC, { 0x00, 0x00, 0x00, 0x7e, 0x02, 0x00, 0x00, 0x00 } }, // NOT SIGN
    {    0x000AD, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // SOFT HYPHEN
    {    0x000AE, { 0x3c, 0x46, 0x5a, 0x46, 0x5a, 0x5a, 0x3c, 0x00 } }, // REGISTERED SIGN
    {    0x000AF, { 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // MACRON
    {    0x000B0, { 0x3c, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00 } }, // DEGREE SIGN
    {    0x000B1, { 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x7e, 0x00 } }, // PLUS-MINUS SIGN
    {    0x000B2, { 0x00, 0x38, 0x0c, 0x18, 0x3c, 0x00, 0x00, 0x00 } }, // SUPERSCRIPT TWO
    {    0x000B3, { 0x00, 0x38, 0x0c, 0x18, 0x0c, 0x38, 0x00, 0x00 } }, // SUPERSCRIPT THREE
    {    0x000B4, { 0x0c, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // ACUTE ACCENT
    {    0x000B5, { 0x00, 0x00, 0x6c, 0x6c, 0x6c, 0x6c, 0x7e, 0x60 } }, // MICRO SIGN
    {    0x000B6, { 0x3e, 0x76, 0x76, 0x76, 0x36, 0x36, 0x36, 0x36 } }, // PILCROW SIGN
    {    0x000B7, { 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00 } }, // MIDDLE DOT
    {    0x000B8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x30 } }, // CEDILLA
    {    0x000B9, { 0x00, 0x18, 0x38, 0x18, 0x3c, 0x00, 0x00, 0x00 } }, // SUPERSCRIPT ONE
    {    0x000BA, { 0x3c, 0x66, 0x66, 0x3c, 0x00, 0x7e, 0x00, 0x00 } }, // MASCULINE ORDINAL INDICATOR
    {    0x000BB, { 0x00, 0x00, 0x6c, 0x36, 0x36, 0x6c, 0x00, 0x00 } }, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    {    0x000BC, { 0x40, 0x42, 0x44, 0x4a, 0x16, 0x2a, 0x4e, 0x02 } }, // VULGAR FRACTION ONE QUARTER
    {    0x000BD, { 0x40, 0x42, 0x44, 0x48, 0x14, 0x22, 0x44, 0x0e } }, // VULGAR FRACTION ONE HALF
    {    0x000BE, { 0x60, 0x32, 0x34, 0x6a, 0x16, 0x2a, 0x4e, 0x02 } }, // VULGAR FRACTION THREE QUARTERS
    {    0x000BF, { 0x18, 0x00, 0x18, 0x30, 0x60, 0x66, 0x3c, 0x00 } }, // INVERTED QUESTION MARK
    {    0x000C0, { 0x10, 0x08, 0x3c, 0x66, 0x7e, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER A WITH GRAVE
    {    0x000C1, { 0x08, 0x10, 0x3c, 0x66, 0x7e, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER A WITH ACUTE
    {    0x000C2, { 0x18, 0x00, 0x3c, 0x66, 0x7e, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    {    0x000C3, { 0x3c, 0x00, 0x3c, 0x66, 0x7e, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER A WITH TILDE
    {    0x000C4, { 0x66, 0x00, 0x3c, 0x66, 0x7e, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER A WITH DIAERESIS
    {    0x000C5, { 0x18, 0x18, 0x3c, 0x66, 0x7e, 0x66, 0x66, 0x00 } }, // LATIN CAPITAL LETTER A WITH RING ABOVE
    {    0x000C6, { 0x1e, 0x3c, 0x6c, 0x7e, 0x7c, 0x6c, 0x6e, 0x00 } }, // LATIN CAPITAL LETTER AE
    {    0x000C7, { 0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x10 } }, // LATIN CAPITAL LETTER C WITH CEDILLA
    {    0x000C8, { 0x10, 0x08, 0x7e, 0x60, 0x78, 0x60, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER E WITH GRAVE
    {    0x000C9, { 0x08, 0x10, 0x7e, 0x60, 0x78, 0x60, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER E WITH ACUTE
    {    0x000CA, { 0x18, 0x00, 0x7e, 0x60, 0x78, 0x60, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    {    0x000CB, { 0x66, 0x00, 0x7e, 0x60, 0x78, 0x60, 0x7e, 0x00 } }, // LATIN CAPITAL LETTER E WITH DIAERESIS
    {    0x000CC, { 0x10, 0x08, 0x3c, 0x18, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER I WITH GRAVE
    {    0x000CD, { 0x08, 0x10, 0x3c, 0x18, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER I WITH ACUTE
    {    0x000CE, { 0x18, 0x00, 0x3c, 0x18, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    {    0x000CF, { 0x24, 0x00, 0x3c, 0x18, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER I WITH DIAERESIS
    {    0x000D0, { 0x78, 0x6c, 0x66, 0xf6, 0x66, 0x6c, 0x78, 0x00 } }, // LATIN CAPITAL LETTER ETH
    {    0x000D1, { 0x3c, 0x00, 0x66, 0x76, 0x7e, 0x6e, 0x66, 0x00 } }, // LATIN CAPITAL LETTER N WITH TILDE
    {    0x000D2, { 0x10, 0x08, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER O WITH GRAVE
    {    0x000D3, { 0x08, 0x10, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER O WITH ACUTE
    {    0x000D4, { 0x18, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    {    0x000D5, { 0x3c, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER O WITH TILDE
    {    0x000D6, { 0x66, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER O WITH DIAERESIS
    {    0x000D7, { 0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00 } }, // MULTIPLICATION SIGN
    {    0x000D8, { 0x3e, 0x66, 0x6e, 0x7e, 0x76, 0x66, 0x7c, 0x00 } }, // LATIN CAPITAL LETTER O WITH STROKE
    {    0x000D9, { 0x10, 0x08, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER U WITH GRAVE
    {    0x000DA, { 0x08, 0x10, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER U WITH ACUTE
    {    0x000DB, { 0x18, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    {    0x000DC, { 0x66, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN CAPITAL LETTER U WITH DIAERESIS
    {    0x000DD, { 0x08, 0x10, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x00 } }, // LATIN CAPITAL LETTER Y WITH ACUTE
    {    0x000DE, { 0x60, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x60, 0x00 } }, // LATIN CAPITAL LETTER THORN
    {    0x000DF, { 0x3c, 0x66, 0x66, 0x6c, 0x66, 0x66, 0x6c, 0x60 } }, // LATIN SMALL LETTER SHARP S
    {    0x000E0, { 0x10, 0x08, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER A WITH GRAVE
    {    0x000E1, { 0x08, 0x10, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER A WITH ACUTE
    {    0x000E2, { 0x18, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER A WITH CIRCUMFLEX
    {    0x000E3, { 0x3c, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER A WITH TILDE
    {    0x000E4, { 0x66, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER A WITH DIAERESIS
    {    0x000E5, { 0x18, 0x18, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER A WITH RING ABOVE
    {    0x000E6, { 0x00, 0x3c, 0x1a, 0x3c, 0x58, 0x58, 0x3c, 0x00 } }, // LATIN SMALL LETTER AE
    {    0x000E7, { 0x00, 0x00, 0x3c, 0x60, 0x60, 0x60, 0x3c, 0x08 } }, // LATIN SMALL LETTER C WITH CEDILLA
    {    0x000E8, { 0x10, 0x08, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00 } }, // LATIN SMALL LETTER E WITH GRAVE
    {    0x000E9, { 0x08, 0x10, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00 } }, // LATIN SMALL LETTER E WITH ACUTE
    {    0x000EA, { 0x18, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00 } }, // LATIN SMALL LETTER E WITH CIRCUMFLEX
    {    0x000EB, { 0x66, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00 } }, // LATIN SMALL LETTER E WITH DIAERESIS
    {    0x000EC, { 0x10, 0x08, 0x00, 0x38, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN SMALL LETTER I WITH GRAVE
    {    0x000ED, { 0x08, 0x10, 0x00, 0x38, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN SMALL LETTER I WITH ACUTE
    {    0x000EE, { 0x00, 0x18, 0x00, 0x38, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN SMALL LETTER I WITH CIRCUMFLEX
    {    0x000EF, { 0x00, 0x28, 0x00, 0x38, 0x18, 0x18, 0x3c, 0x00 } }, // LATIN SMALL LETTER I WITH DIAERESIS
    {    0x000F0, { 0x06, 0x0f, 0x06, 0x3e, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER ETH
    {    0x000F1, { 0x00, 0x3c, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x00 } }, // LATIN SMALL LETTER N WITH TILDE
    {    0x000F2, { 0x10, 0x08, 0x00, 0x3c, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN SMALL LETTER O WITH GRAVE
    {    0x000F3, { 0x08, 0x10, 0x00, 0x3c, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN SMALL LETTER O WITH ACUTE
    {    0x000F4, { 0x00, 0x18, 0x00, 0x3c, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN SMALL LETTER O WITH CIRCUMFLEX
    {    0x000F5, { 0x00, 0x3c, 0x00, 0x3c, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN SMALL LETTER O WITH TILDE
    {    0x000F6, { 0x00, 0x66, 0x00, 0x3c, 0x66, 0x66, 0x3c, 0x00 } }, // LATIN SMALL LETTER O WITH DIAERESIS
    {    0x000F7, { 0x00, 0x18, 0x00, 0x7e, 0x00, 0x18, 0x00, 0x00 } }, // DIVISION SIGN
    {    0x000F8, { 0x00, 0x00, 0x00, 0x3e, 0x6e, 0x76, 0x7c, 0x00 } }, // LATIN SMALL LETTER O WITH STROKE
    {    0x000F9, { 0x10, 0x08, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER U WITH GRAVE
    {    0x000FA, { 0x08, 0x10, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER U WITH ACUTE
    {    0x000FB, { 0x00, 0x18, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER U WITH CIRCUMFLEX
    {    0x000FC, { 0x00, 0x66, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x00 } }, // LATIN SMALL LETTER U WITH DIAERESIS
    {    0x000FD, { 0x08, 0x10, 0x66, 0x66, 0x66, 0x3e, 0x0c, 0x38 } }, // LATIN SMALL LETTER Y WITH ACUTE
    {    0x000FE, { 0x00, 0x60, 0x60, 0x78, 0x6c, 0x78, 0x60, 0x60 } }, // LATIN SMALL LETTER THORN
    {    0x000FF, { 0x00, 0x66, 0x00, 0x66, 0x66, 0x3e, 0x0c, 0x38 } }, // LATIN SMALL LETTER Y WITH DIAERESIS
#endif



    // Latin Extended-A
    {      0x100,                  { 60, 0, 24, 36, 66, 126, 66, 0 } },
    {      0x101,                    { 60, 0, 56, 4, 60, 68, 58, 0 } },
    {      0x102,                 { 36, 24, 24, 36, 66, 126, 66, 0 } },
    {      0x103,                   { 36, 24, 56, 4, 60, 68, 58, 0 } },
    {      0x104,                 { 24, 36, 66, 126, 66, 66, 68, 3 } },
    {      0x105,                     { 0, 0, 56, 4, 60, 68, 58, 3 } },
    {      0x106,                   { 4, 28, 34, 64, 64, 34, 28, 0 } },
    {      0x107,                    { 4, 8, 60, 66, 64, 66, 60, 0 } },
    {      0x108,                   { 8, 28, 34, 64, 64, 34, 28, 0 } },
    {      0x109,                   { 8, 20, 60, 66, 64, 66, 60, 0 } },
    {      0x10a,                   { 8, 28, 34, 64, 64, 34, 28, 0 } },
    {      0x10b,                    { 8, 0, 60, 66, 64, 66, 60, 0 } },
    {      0x10c,                  { 20, 28, 34, 64, 64, 34, 28, 0 } },
    {      0x10d,                   { 20, 8, 60, 66, 64, 66, 60, 0 } },
    {      0x10e,                { 20, 120, 36, 34, 34, 36, 120, 0 } },
    {      0x10f,                    { 5, 5, 52, 76, 68, 76, 52, 0 } },
    {      0x110,               { 120, 36, 34, 114, 34, 36, 120, 0 } },
    {      0x111,                    { 2, 7, 58, 70, 66, 70, 58, 0 } },
    {      0x112,                { 60, 0, 126, 64, 120, 64, 126, 0 } },
    {      0x113,                  { 60, 0, 60, 66, 126, 64, 60, 0 } },
    {      0x114,               { 36, 24, 126, 64, 120, 64, 126, 0 } },
    {      0x115,                 { 36, 24, 60, 66, 126, 64, 60, 0 } },
    {      0x116,                 { 8, 0, 126, 64, 120, 64, 126, 0 } },
    {      0x117,                   { 8, 0, 60, 66, 126, 64, 60, 0 } },
    {      0x118,               { 126, 64, 64, 120, 64, 64, 126, 6 } },
    {      0x119,                  { 0, 0, 60, 66, 126, 64, 60, 12 } },
    {      0x11a,                { 20, 8, 126, 64, 120, 64, 126, 0 } },
    {      0x11b,                  { 20, 8, 60, 66, 126, 64, 60, 0 } },
    {      0x11c,                   { 8, 28, 34, 64, 78, 34, 28, 0 } },
    {      0x11d,                   { 8, 20, 58, 70, 70, 58, 2, 60 } },
    {      0x11e,                  { 20, 28, 34, 64, 78, 34, 28, 0 } },
    {      0x11f,                   { 20, 8, 58, 70, 70, 58, 2, 60 } },
    {      0x120,                   { 8, 28, 34, 64, 78, 34, 28, 0 } },
    {      0x121,                    { 8, 0, 58, 70, 70, 58, 2, 60 } },
    {      0x122,                  { 28, 34, 64, 78, 66, 34, 28, 8 } },
    {      0x123,                    { 4, 8, 58, 70, 70, 58, 2, 60 } },
    {      0x124,                  { 8, 20, 66, 66, 126, 66, 66, 0 } },
    {      0x125,                   { 8, 84, 64, 92, 98, 66, 66, 0 } },
    {      0x126,                 { 0, 66, 255, 66, 126, 66, 66, 0 } },
    {      0x127,                 { 64, 224, 64, 92, 98, 66, 66, 0 } },
    {      0x128,                     { 50, 76, 28, 8, 8, 8, 28, 0 } },
    {      0x129,                     { 50, 76, 0, 24, 8, 8, 28, 0 } },
    {      0x12a,                      { 28, 0, 28, 8, 8, 8, 28, 0 } },
    {      0x12b,                      { 28, 0, 0, 24, 8, 8, 28, 0 } },
    {      0x12c,                     { 36, 24, 28, 8, 8, 8, 28, 0 } },
    {      0x12d,                     { 36, 24, 0, 24, 8, 8, 28, 0 } },
    {      0x12e,                      { 28, 8, 8, 8, 8, 8, 16, 12 } },
    {      0x12f,                      { 8, 0, 24, 8, 8, 8, 16, 12 } },
    {      0x130,                        { 8, 0, 8, 8, 8, 8, 28, 0 } },
    {      0x131,                       { 0, 0, 24, 8, 8, 8, 28, 0 } },
    {      0x132,                { 238, 66, 66, 66, 66, 74, 228, 0 } },
    {      0x133,                { 34, 0, 102, 34, 34, 34, 114, 28 } },
    {      0x134,                      { 4, 10, 4, 4, 4, 68, 56, 0 } },
    {      0x135,                     { 4, 10, 0, 12, 4, 4, 68, 56 } },
    {      0x136,                { 66, 68, 72, 112, 72, 68, 82, 32 } },
    {      0x137,                { 64, 64, 68, 72, 80, 104, 84, 32 } },
    {      0x138,                   { 0, 0, 68, 72, 112, 72, 68, 0 } },
    {      0x139,                 { 32, 64, 64, 64, 64, 64, 126, 0 } },
    {      0x13a,                       { 4, 24, 8, 8, 8, 8, 28, 0 } },
    {      0x13b,                { 64, 64, 64, 64, 64, 64, 126, 16 } },
    {      0x13c,                      { 24, 8, 8, 8, 8, 8, 28, 16 } },
    {      0x13d,                 { 72, 80, 64, 64, 64, 64, 126, 0 } },
    {      0x13e,                      { 25, 10, 8, 8, 8, 8, 28, 0 } },
    {      0x13f,                 { 64, 64, 64, 72, 64, 64, 126, 0 } },
    {      0x140,                      { 24, 8, 8, 10, 8, 8, 28, 0 } },
    {      0x141,                { 64, 64, 96, 192, 64, 64, 126, 0 } },
    {      0x142,                     { 24, 8, 12, 24, 8, 8, 28, 0 } },
    {      0x143,                   { 4, 74, 98, 82, 74, 70, 66, 0 } },
    {      0x144,                    { 4, 8, 92, 98, 66, 66, 66, 0 } },
    {      0x145,                 { 66, 98, 82, 74, 70, 66, 74, 16 } },
    {      0x146,                   { 0, 0, 92, 98, 66, 66, 74, 16 } },
    {      0x147,                  { 20, 74, 98, 82, 74, 70, 66, 0 } },
    {      0x148,                   { 20, 8, 92, 98, 66, 66, 66, 0 } },
    {      0x149,                 { 64, 128, 44, 50, 34, 34, 34, 0 } },
    {      0x14a,                  { 66, 98, 82, 74, 70, 66, 66, 6 } },
    {      0x14b,                    { 0, 0, 92, 98, 66, 66, 66, 6 } },
    {      0x14c,                   { 60, 0, 24, 36, 66, 36, 24, 0 } },
    {      0x14d,                   { 60, 0, 60, 66, 66, 66, 60, 0 } },
    {      0x14e,                  { 36, 24, 24, 36, 66, 36, 24, 0 } },
    {      0x14f,                  { 36, 24, 60, 66, 66, 66, 60, 0 } },
    {      0x150,                  { 18, 36, 24, 36, 66, 36, 24, 0 } },
    {      0x151,                  { 18, 36, 60, 66, 66, 66, 60, 0 } },
    {      0x152,                  { 30, 40, 72, 78, 72, 40, 30, 0 } },
    {      0x153,                    { 0, 0, 52, 74, 78, 72, 54, 0 } },
    {      0x154,                 { 8, 124, 66, 124, 72, 68, 66, 0 } },
    {      0x155,                   { 8, 16, 92, 98, 64, 64, 64, 0 } },
    {      0x156,               { 124, 66, 66, 124, 72, 68, 74, 16 } },
    {      0x157,                   { 0, 0, 92, 98, 64, 64, 72, 16 } },
    {      0x158,                { 40, 124, 66, 124, 72, 68, 66, 0 } },
    {      0x159,                  { 40, 16, 92, 98, 64, 64, 64, 0 } },
    {      0x15a,                    { 8, 62, 64, 60, 2, 66, 60, 0 } },
    {      0x15b,                    { 4, 8, 62, 64, 60, 2, 124, 0 } },
    {      0x15c,                    { 8, 54, 64, 60, 2, 66, 60, 0 } },
    {      0x15d,                   { 8, 20, 62, 64, 60, 2, 124, 0 } },
    {      0x15e,                  { 60, 66, 64, 60, 2, 66, 60, 16 } },
    {      0x15f,                   { 0, 0, 62, 64, 60, 2, 124, 16 } },
    {      0x160,                   { 20, 62, 64, 60, 2, 66, 60, 0 } },
    {      0x161,                   { 20, 8, 62, 64, 60, 2, 124, 0 } },
    {      0x162,                       { 62, 8, 8, 8, 8, 8, 4, 24 } },
    {      0x163,                { 16, 16, 124, 16, 16, 18, 12, 24 } },
    {      0x164,                       { 20, 62, 8, 8, 8, 8, 8, 0 } },
    {      0x165,                  { 2, 18, 16, 124, 16, 18, 12, 0 } },
    {      0x166,                       { 62, 8, 8, 28, 8, 8, 8, 0 } },
    {      0x167,                 { 16, 124, 16, 56, 16, 18, 12, 0 } },
    {      0x168,                   { 50, 76, 0, 66, 66, 66, 60, 0 } },
    {      0x169,                   { 50, 76, 0, 66, 66, 70, 58, 0 } },
    {      0x16a,                   { 60, 0, 66, 66, 66, 66, 60, 0 } },
    {      0x16b,                   { 62, 0, 66, 66, 66, 70, 58, 0 } },
    {      0x16c,                  { 36, 24, 66, 66, 66, 66, 60, 0 } },
    {      0x16d,                  { 36, 24, 66, 66, 66, 70, 58, 0 } },
    {      0x16e,                  { 24, 66, 66, 66, 66, 66, 60, 0 } },
    {      0x16f,                   { 24, 0, 66, 66, 66, 70, 58, 0 } },
    {      0x170,                   { 18, 36, 0, 66, 66, 66, 60, 0 } },
    {      0x171,                   { 18, 36, 0, 66, 66, 70, 58, 0 } },
    {      0x172,                 { 66, 66, 66, 66, 66, 66, 60, 12 } },
    {      0x173,                   { 0, 0, 66, 66, 66, 70, 58, 12 } },
    {      0x174,                  { 8, 86, 66, 90, 90, 102, 66, 0 } },
    {      0x175,                   { 8, 20, 65, 73, 73, 73, 54, 0 } },
    {      0x176,                     { 8, 20, 34, 34, 28, 8, 8, 0 } },
    {      0x177,                    { 8, 20, 0, 66, 70, 58, 2, 60 } },
    {      0x178,                     { 20, 0, 34, 34, 28, 8, 8, 0 } },
    {      0x179,                  { 4, 126, 6, 24, 32, 64, 126, 0 } },
    {      0x17a,                   { 4, 8, 126, 4, 24, 32, 126, 0 } },
    {      0x17b,                  { 8, 126, 6, 24, 32, 64, 126, 0 } },
    {      0x17c,                   { 8, 0, 126, 4, 24, 32, 126, 0 } },
    {      0x17d,                 { 20, 126, 6, 24, 32, 64, 126, 0 } },
    {      0x17e,                  { 20, 8, 126, 4, 24, 32, 126, 0 } },
    {      0x17f,                 { 14, 16, 16, 112, 16, 16, 16, 0 } },
    // Latin Extended-B
    {      0x180,                 { 32, 120, 32, 44, 50, 50, 44, 0 } },
    {      0x181,                  { 60, 82, 18, 28, 18, 18, 28, 0 } },
    {      0x182,                 { 124, 64, 64, 92, 98, 66, 60, 0 } },
    {      0x183,                 { 124, 64, 92, 98, 66, 98, 92, 0 } },
    {      0x184,                 { 112, 16, 28, 18, 18, 18, 28, 0 } },
    {      0x185,                  { 0, 112, 16, 28, 18, 18, 28, 0 } },
    {      0x186,                     { 56, 68, 2, 2, 2, 68, 56, 0 } },
    {      0x187,                   { 3, 30, 32, 64, 64, 34, 28, 0 } },
    {      0x188,                    { 0, 6, 60, 64, 64, 66, 60, 0 } },
    {      0x189,                 { 56, 36, 34, 114, 34, 36, 56, 0 } },
    {      0x18a,                  { 60, 82, 17, 17, 17, 18, 28, 0 } },
    {      0x18b,                    { 62, 2, 2, 62, 66, 66, 62, 0 } },
    {      0x18c,                    { 30, 2, 2, 58, 70, 70, 58, 0 } },
    {      0x18d,                    { 0, 0, 60, 66, 60, 12, 2, 62 } },
    {      0x18e,                    { 126, 2, 2, 30, 2, 2, 126, 0 } },
    {      0x18f,                  { 60, 66, 2, 126, 66, 66, 60, 0 } },
    {      0x190,                  { 30, 32, 64, 56, 64, 64, 60, 0 } },
    {      0x191,                 { 63, 32, 32, 62, 32, 32, 32, 64 } },
    {      0x192,                { 14, 16, 16, 124, 16, 16, 16, 48 } },
    {      0x193,                   { 3, 28, 32, 64, 78, 34, 28, 0 } },
    {      0x194,                   { 34, 34, 34, 34, 28, 8, 20, 8 } },
    {      0x195,                 { 64, 64, 113, 73, 73, 73, 70, 0 } },
    {      0x196,                        { 8, 8, 8, 8, 8, 8, 12, 0 } },
    {      0x197,                      { 28, 8, 8, 28, 8, 8, 28, 0 } },
    {      0x198,                 { 70, 72, 80, 112, 72, 68, 66, 0 } },
    {      0x199,                 { 48, 64, 68, 72, 80, 104, 68, 0 } },
    {      0x19a,                      { 24, 8, 8, 28, 8, 8, 28, 0 } },
    {      0x19b,                 { 64, 120, 16, 24, 36, 36, 66, 0 } },
    {      0x19c,                  { 73, 73, 73, 73, 73, 77, 59, 0 } },
    {      0x19d,                 { 33, 49, 41, 37, 35, 33, 33, 64 } },
    {      0x19e,                    { 0, 0, 92, 98, 66, 66, 66, 2 } },
    {      0x19f,                 { 24, 36, 66, 126, 66, 36, 24, 0 } },
    {      0x1a0,                  { 25, 39, 66, 66, 66, 36, 24, 0 } },
    {      0x1a1,                    { 0, 1, 63, 66, 66, 66, 60, 0 } },
    {      0x1a2,                  { 59, 69, 69, 69, 69, 69, 57, 1 } },
    {      0x1a3,                    { 0, 0, 59, 69, 69, 69, 57, 1 } },
    {      0x1a4,                  { 62, 81, 17, 30, 16, 16, 16, 0 } },
    {      0x1a5,               { 56, 64, 124, 66, 66, 124, 64, 64 } },
    {      0x1a6,                { 64, 124, 66, 66, 124, 72, 68, 2 } },
    {      0x1a7,                   { 60, 66, 2, 60, 64, 66, 60, 0 } },
    {      0x1a8,                    { 0, 0, 124, 2, 60, 64, 62, 0 } },
    {      0x1a9,                 { 126, 32, 16, 8, 16, 32, 126, 0 } },
    {      0x1aa,                      { 48, 72, 56, 8, 8, 8, 8, 6 } },
    {      0x1ab,                 { 16, 16, 124, 16, 16, 12, 4, 24 } },
    {      0x1ac,                       { 62, 72, 8, 8, 8, 8, 8, 0 } },
    {      0x1ad,                 { 12, 16, 124, 16, 16, 18, 12, 0 } },
    {      0x1ae,                        { 62, 8, 8, 8, 8, 8, 8, 4 } },
    {      0x1af,                  { 69, 71, 68, 68, 68, 68, 56, 0 } },
    {      0x1b0,                    { 0, 1, 69, 70, 68, 76, 58, 0 } },
    {      0x1b1,                  { 99, 20, 99, 65, 65, 34, 28, 0 } },
    {      0x1b2,                  { 76, 66, 66, 36, 36, 24, 24, 0 } },
    {      0x1b3,                  { 67, 68, 68, 56, 16, 16, 16, 0 } },
    {      0x1b4,                    { 0, 3, 68, 68, 68, 60, 4, 56 } },
    {      0x1b5,                  { 126, 2, 4, 60, 32, 64, 126, 0 } },
    {      0x1b6,                   { 0, 0, 126, 4, 60, 32, 126, 0 } },
    {      0x1b7,                    { 126, 4, 8, 28, 2, 66, 60, 0 } },
    {      0x1b8,                 { 126, 32, 16, 56, 64, 66, 60, 0 } },
    {      0x1b9,                    { 0, 0, 62, 16, 8, 28, 32, 30 } },
    {      0x1ba,                   { 0, 126, 4, 28, 2, 60, 32, 24 } },
    {      0x1bb,                  { 60, 66, 4, 60, 32, 64, 126, 0 } },
    {      0x1bc,                   { 126, 32, 60, 2, 2, 66, 60, 0 } },
    {      0x1bd,                    { 0, 0, 126, 32, 60, 2, 60, 0 } },
    {      0x1be,                   { 0, 32, 120, 32, 28, 2, 60, 0 } },
    {      0x1bf,                  { 0, 0, 92, 98, 68, 72, 112, 64 } },
    {      0x1c0,                         { 8, 8, 8, 8, 8, 8, 8, 8 } },
    {      0x1c1,                 { 36, 36, 36, 36, 36, 36, 36, 36 } },
    {      0x1c2,                       { 8, 8, 62, 8, 62, 8, 8, 8 } },
    {      0x1c3,                         { 8, 8, 8, 8, 8, 0, 8, 0 } },
    {      0x1c4,                 { 5, 119, 73, 74, 76, 76, 119, 0 } },
    {      0x1c5,                 { 5, 114, 79, 73, 74, 76, 119, 0 } },
    {      0x1c6,                  { 21, 18, 55, 81, 82, 84, 55, 0 } },
    {      0x1c7,                { 70, 66, 66, 66, 66, 82, 114, 12 } },
    {      0x1c8,                { 66, 64, 70, 66, 66, 82, 114, 12 } },
    {      0x1c9,                { 98, 32, 38, 34, 34, 34, 114, 12 } },
    {      0x1ca,                { 75, 105, 105, 89, 89, 73, 73, 6 } },
    {      0x1cb,                { 73, 104, 107, 89, 89, 73, 73, 6 } },
    {      0x1cc,                   { 1, 0, 83, 105, 73, 73, 73, 6 } },
    {      0x1cd,                 { 20, 24, 36, 66, 126, 66, 66, 0 } },
    {      0x1ce,                    { 20, 8, 56, 4, 60, 68, 58, 0 } },
    {      0x1cf,                      { 20, 8, 28, 8, 8, 8, 28, 0 } },
    {      0x1d0,                      { 20, 8, 0, 28, 8, 8, 28, 0 } },
    {      0x1d1,                   { 20, 8, 24, 36, 66, 36, 24, 0 } },
    {      0x1d2,                   { 20, 8, 60, 66, 66, 66, 60, 0 } },
    {      0x1d3,                   { 20, 8, 66, 66, 66, 66, 60, 0 } },
    {      0x1d4,                   { 20, 8, 66, 66, 66, 70, 58, 0 } },
    {      0x1d5,                  { 62, 20, 66, 66, 66, 66, 60, 0 } },
    {      0x1d6,                  { 62, 20, 66, 66, 66, 70, 58, 0 } },
    {      0x1d7,                    { 8, 82, 0, 66, 66, 66, 60, 0 } },
    {      0x1d8,                    { 8, 82, 0, 66, 66, 70, 58, 0 } },
    {      0x1d9,                   { 20, 74, 0, 66, 66, 66, 60, 0 } },
    {      0x1da,                   { 20, 74, 0, 66, 66, 70, 58, 0 } },
    {      0x1db,                   { 16, 74, 0, 66, 66, 66, 60, 0 } },
    {      0x1dc,                   { 16, 74, 0, 66, 66, 70, 58, 0 } },
    {      0x1dd,                    { 0, 0, 60, 2, 126, 66, 60, 0 } },
    {      0x1de,                { 126, 36, 24, 36, 66, 126, 66, 0 } },
    {      0x1df,                  { 126, 36, 56, 4, 60, 68, 58, 0 } },
    {      0x1e0,                  { 62, 8, 24, 36, 66, 126, 66, 0 } },
    {      0x1e1,                    { 62, 8, 56, 4, 60, 68, 58, 0 } },
    {      0x1e2,                  { 62, 0, 30, 40, 126, 72, 78, 0 } },
    {      0x1e3,                   { 62, 0, 52, 10, 62, 72, 54, 0 } },
    {      0x1e4,                  { 28, 34, 64, 66, 79, 34, 28, 0 } },
    {      0x1e5,                    { 0, 58, 70, 58, 2, 15, 2, 60 } },
    {      0x1e6,                  { 20, 28, 34, 64, 78, 34, 28, 0 } },
    {      0x1e7,                   { 20, 8, 62, 66, 66, 62, 2, 60 } },
    {      0x1e8,                 { 40, 84, 72, 112, 72, 68, 66, 0 } },
    {      0x1e9,                 { 20, 72, 64, 72, 80, 104, 68, 0 } },
    {      0x1ea,                 { 24, 36, 66, 66, 66, 36, 24, 12 } },
    {      0x1eb,                   { 0, 0, 60, 66, 66, 66, 60, 12 } },
    {      0x1ec,                  { 60, 0, 24, 36, 66, 36, 24, 12 } },
    {      0x1ed,                  { 60, 0, 60, 66, 66, 66, 60, 12 } },
    {      0x1ee,                   { 20, 126, 4, 28, 2, 66, 60, 0 } },
    {      0x1ef,                     { 20, 8, 62, 4, 8, 28, 2, 28 } },
    {      0x1f0,                     { 20, 8, 0, 12, 4, 4, 68, 56 } },
    {      0x1f1,                { 119, 73, 73, 74, 76, 76, 119, 0 } },
    {      0x1f2,                { 112, 72, 79, 73, 74, 76, 119, 0 } },
    {      0x1f3,                  { 16, 16, 55, 81, 82, 84, 55, 0 } },
    {      0x1f4,                   { 8, 28, 34, 64, 78, 34, 28, 0 } },
    {      0x1f5,                    { 4, 8, 62, 66, 66, 62, 2, 60 } },
    {      0x1f6,                 { 72, 72, 73, 121, 73, 73, 70, 0 } },
    {      0x1f7,                 { 92, 98, 68, 72, 112, 64, 64, 0 } },
    {      0x1f8,                  { 16, 74, 98, 82, 74, 70, 66, 0 } },
    {      0x1f9,                   { 16, 8, 92, 98, 66, 66, 66, 0 } },
    {      0x1fa,                  { 4, 24, 24, 36, 66, 126, 66, 0 } },
    {      0x1fb,                    { 4, 24, 56, 4, 60, 68, 58, 0 } },
    {      0x1fc,                   { 4, 8, 30, 40, 126, 72, 78, 0 } },
    {      0x1fd,                    { 4, 8, 52, 10, 62, 72, 54, 0 } },
    {      0x1fe,                   { 8, 26, 36, 74, 82, 36, 88, 0 } },
    {      0x1ff,                    { 4, 8, 60, 70, 90, 98, 60, 0 } },
    {      0x200,                 { 72, 36, 24, 36, 66, 126, 66, 0 } },
    {      0x201,                   { 72, 36, 56, 4, 60, 68, 58, 0 } },
    {      0x202,                 { 24, 36, 24, 36, 66, 126, 66, 0 } },
    {      0x203,                   { 24, 36, 56, 4, 60, 68, 58, 0 } },
    {      0x204,               { 72, 36, 126, 64, 120, 64, 126, 0 } },
    {      0x205,                 { 72, 36, 60, 66, 126, 64, 60, 0 } },
    {      0x206,               { 24, 36, 126, 64, 120, 64, 126, 0 } },
    {      0x207,                 { 24, 36, 60, 66, 126, 64, 60, 0 } },
    {      0x208,                     { 72, 36, 28, 8, 8, 8, 28, 0 } },
    {      0x209,                     { 72, 36, 0, 24, 8, 8, 28, 0 } },
    {      0x20a,                     { 24, 36, 28, 8, 8, 8, 28, 0 } },
    {      0x20b,                     { 24, 36, 0, 24, 8, 8, 28, 0 } },
    {      0x20c,                  { 72, 36, 24, 36, 66, 36, 24, 0 } },
    {      0x20d,                   { 72, 36, 0, 60, 66, 66, 60, 0 } },
    {      0x20e,                  { 24, 36, 24, 36, 66, 36, 24, 0 } },
    {      0x20f,                   { 24, 36, 0, 60, 66, 66, 60, 0 } },
    {      0x210,                { 72, 36, 124, 66, 124, 68, 66, 0 } },
    {      0x211,                   { 72, 36, 0, 92, 98, 64, 64, 0 } },
    {      0x212,                { 24, 36, 124, 66, 124, 68, 66, 0 } },
    {      0x213,                   { 24, 36, 0, 92, 98, 64, 64, 0 } },
    {      0x214,                   { 72, 36, 0, 66, 66, 66, 60, 0 } },
    {      0x215,                   { 72, 36, 0, 66, 66, 70, 58, 0 } },
    {      0x216,                   { 24, 36, 0, 66, 66, 66, 60, 0 } },
    {      0x217,                   { 24, 36, 0, 66, 66, 70, 58, 0 } },
    {      0x218,                  { 60, 66, 64, 60, 2, 66, 60, 16 } },
    {      0x219,                   { 0, 0, 62, 64, 60, 2, 124, 16 } },
    {      0x21a,                        { 62, 8, 8, 8, 8, 8, 0, 8 } },
    {      0x21b,                { 16, 16, 124, 16, 16, 18, 12, 16 } },
    {      0x21c,                      { 126, 4, 28, 2, 2, 4, 8, 0 } },
    {      0x21d,                       { 0, 0, 62, 4, 28, 2, 2, 4 } },
    {      0x21e,                  { 20, 8, 66, 66, 126, 66, 66, 0 } },
    {      0x21f,                  { 20, 72, 64, 92, 98, 66, 66, 0 } },
    {      0x220,                  { 92, 98, 66, 66, 66, 66, 66, 2 } },
    {      0x221,                    { 8, 8, 56, 72, 75, 77, 54, 4 } },
    {      0x222,                  { 66, 66, 60, 36, 66, 36, 24, 0 } },
    {      0x223,                  { 66, 66, 60, 66, 66, 66, 60, 0 } },
    {      0x224,                  { 126, 2, 4, 24, 32, 64, 126, 2 } },
    {      0x225,                   { 0, 0, 126, 4, 24, 32, 126, 2 } },
    {      0x226,                   { 8, 0, 24, 36, 66, 126, 66, 0 } },
    {      0x227,                     { 8, 0, 56, 4, 60, 68, 58, 0 } },
    {      0x228,              { 126, 64, 64, 120, 64, 64, 126, 24 } },
    {      0x229,                  { 0, 0, 60, 66, 126, 64, 60, 24 } },
    {      0x22a,                 { 126, 36, 24, 36, 66, 36, 24, 0 } },
    {      0x22b,                  { 126, 36, 0, 60, 66, 66, 60, 0 } },
    {      0x22c,                  { 126, 52, 8, 60, 66, 66, 60, 0 } },
    {      0x22d,                  { 126, 52, 8, 60, 66, 66, 60, 0 } },
    {      0x22e,                    { 8, 0, 24, 36, 66, 36, 24, 0 } },
    {      0x22f,                    { 8, 0, 60, 66, 66, 66, 60, 0 } },
    {      0x230,                  { 126, 8, 24, 36, 66, 36, 24, 0 } },
    {      0x231,                   { 126, 8, 0, 60, 66, 66, 60, 0 } },
    {      0x232,                     { 62, 0, 34, 34, 28, 8, 8, 0 } },
    {      0x233,                   { 60, 0, 66, 66, 70, 58, 2, 60 } },
    {      0x234,                 { 96, 32, 32, 32, 38, 42, 60, 16 } },
    {      0x235,                   { 0, 0, 88, 100, 71, 69, 70, 2 } },
    {      0x236,                 { 16, 16, 124, 16, 22, 26, 12, 8 } },
    {      0x237,                      { 0, 0, 12, 4, 4, 4, 68, 56 } },
    {      0x238,                    { 8, 8, 62, 73, 73, 73, 54, 0 } },
    {      0x239,                     { 0, 0, 62, 73, 73, 62, 8, 8 } },
    {      0x23a,                 { 24, 37, 66, 126, 74, 82, 98, 0 } },
    {      0x23b,                  { 29, 34, 68, 72, 80, 34, 92, 0 } },
    {      0x23c,                   { 0, 1, 62, 70, 72, 82, 60, 64 } },
    {      0x23d,                 { 32, 32, 32, 120, 32, 32, 62, 0 } },
    {      0x23e,                    { 1, 62, 12, 8, 24, 40, 72, 0 } },
    {      0x23f,                  { 0, 62, 64, 60, 2, 124, 32, 28 } },
    {      0x240,                  { 0, 0, 126, 4, 24, 32, 112, 14 } },
    {      0x241,                      { 28, 34, 2, 2, 12, 8, 8, 0 } },
    {      0x242,                      { 0, 0, 28, 34, 2, 12, 8, 0 } },
    {      0x243,               { 124, 34, 60, 34, 122, 34, 124, 0 } },
    {      0x244,                 { 34, 34, 34, 127, 34, 34, 28, 0 } },
    {      0x245,                  { 24, 24, 36, 36, 66, 66, 66, 0 } },
    {      0x246,                { 1, 126, 68, 120, 80, 96, 126, 0 } },
    {      0x247,                  { 0, 1, 62, 70, 126, 80, 60, 64 } },
    {      0x248,                     { 14, 4, 4, 14, 4, 68, 56, 0 } },
    {      0x249,                     { 4, 0, 12, 4, 14, 4, 68, 56 } },
    {      0x24a,                   { 58, 70, 66, 66, 70, 58, 2, 3 } },
    {      0x24b,                     { 0, 0, 62, 66, 66, 62, 2, 3 } },
    {      0x24c,                 { 60, 34, 34, 124, 40, 36, 34, 0 } },
    {      0x24d,                   { 0, 0, 46, 48, 32, 112, 32, 0 } },
    {      0x24e,                    { 34, 127, 34, 28, 8, 8, 8, 0 } },
    {      0x24f,                   { 0, 0, 34, 127, 38, 26, 2, 28 } },
    // Spacing Mod.
    {      0x2d9,                         { 8, 0, 0, 0, 0, 0, 0, 0 } },
    {      0x2da,                        { 8, 20, 8, 0, 0, 0, 0, 0 } },
    {      0x2dc,                       { 50, 76, 0, 0, 0, 0, 0, 0 } },
    {      0x2dd,                       { 18, 36, 0, 0, 0, 0, 0, 0 } },
    {      0x2b9,                         { 4, 8, 0, 0, 0, 0, 0, 0 } },
    {      0x2ba,                       { 18, 36, 0, 0, 0, 0, 0, 0 } },
    {      0x2bb,                         { 4, 8, 8, 0, 0, 0, 0, 0 } },
    {      0x2bc,                        { 8, 16, 0, 0, 0, 0, 0, 0 } },
    {      0x2bd,                        { 16, 8, 0, 0, 0, 0, 0, 0 } },
    {      0x2c4,                       { 8, 20, 34, 0, 0, 0, 0, 0 } },
    {      0x2c5,                       { 34, 20, 8, 0, 0, 0, 0, 0 } },
    {      0x2c6,                        { 8, 20, 0, 0, 0, 0, 0, 0 } },
    {      0x2c7,                        { 20, 8, 0, 0, 0, 0, 0, 0 } },
    {      0x2c8,                         { 8, 8, 0, 0, 0, 0, 0, 0 } },
    {      0x2c9,                        { 60, 0, 0, 0, 0, 0, 0, 0 } },
    {      0x2ca,                         { 4, 8, 0, 0, 0, 0, 0, 0 } },
    {      0x2cb,                         { 8, 4, 0, 0, 0, 0, 0, 0 } },
    {      0x2cc,                         { 0, 0, 0, 0, 0, 0, 8, 8 } },
    {      0x2cd,                        { 0, 0, 0, 0, 0, 0, 0, 60 } },
    {      0x2db,                       { 0, 0, 0, 0, 0, 8, 16, 12 } },
    // Greek
    {      0x370,                  { 56, 16, 16, 30, 16, 16, 56, 0 } },
    {      0x371,                    { 0, 0, 56, 16, 30, 16, 56, 0 } },
    {      0x372,                     { 62, 42, 42, 8, 8, 8, 28, 0 } },
    {      0x373,                      { 30, 42, 42, 8, 8, 8, 8, 0 } },
    {      0x374,                        { 0, 8, 16, 0, 0, 0, 0, 0 } },
    {      0x375,                        { 0, 0, 0, 0, 0, 8, 16, 0 } },
    {      0x376,                 { 115, 38, 38, 42, 42, 50, 55, 0 } },
    {      0x377,                   { 0, 0, 115, 38, 42, 50, 55, 0 } },
    {      0x37a,                       { 0, 0, 0, 0, 0, 0, 16, 24 } },
    {      0x37b,                     { 0, 0, 56, 68, 2, 68, 56, 0 } },
    {      0x37c,                    { 0, 0, 28, 34, 72, 34, 28, 0 } },
    {      0x37d,                    { 0, 0, 56, 68, 18, 68, 56, 0 } },
    {      0x37e,                      { 0, 0, 24, 0, 0, 24, 8, 16 } },
    {      0x37f,                      { 14, 4, 4, 4, 4, 36, 24, 0 } },
    {      0x384,                         { 4, 8, 0, 0, 0, 0, 0, 0 } },
    {      0x385,                        { 4, 8, 34, 0, 0, 0, 0, 0 } },
    {      0x386,                  { 44, 82, 33, 63, 33, 33, 33, 0 } },
    {      0x387,                         { 0, 0, 0, 0, 8, 0, 0, 0 } },
    {      0x388,                  { 63, 80, 16, 30, 16, 16, 31, 0 } },
    {      0x389,                  { 49, 81, 17, 31, 17, 17, 17, 0 } },
    {      0x38a,                      { 46, 68, 4, 4, 4, 4, 14, 0 } },
    {      0x38c,                 { 94, 161, 33, 33, 33, 33, 30, 0 } },
    {      0x38e,                     { 49, 81, 17, 14, 4, 4, 4, 0 } },
    {      0x38f,                 { 92, 162, 34, 34, 54, 20, 99, 0 } },
    {      0x390,                       { 4, 8, 34, 8, 8, 8, 12, 0 } },
    {      0x391,                 { 24, 36, 66, 126, 66, 66, 66, 0 } },
    {      0x392,               { 124, 66, 66, 124, 66, 66, 124, 0 } },
    {      0x393,                  { 62, 32, 32, 32, 32, 32, 32, 0 } },
    {      0x394,                  { 8, 20, 34, 34, 65, 65, 127, 0 } },
    {      0x395,               { 126, 64, 64, 124, 64, 64, 126, 0 } },
    {      0x396,                   { 127, 2, 4, 8, 16, 32, 127, 0 } },
    {      0x397,                 { 66, 66, 66, 126, 66, 66, 66, 0 } },
    {      0x398,                 { 60, 66, 66, 126, 66, 66, 60, 0 } },
    {      0x399,                       { 28, 8, 8, 8, 8, 8, 28, 0 } },
    {      0x39a,                  { 68, 72, 80, 96, 80, 72, 70, 0 } },
    {      0x39b,                   { 8, 20, 34, 65, 65, 65, 65, 0 } },
    {      0x39c,                 { 66, 102, 90, 74, 66, 66, 66, 0 } },
    {      0x39d,                  { 66, 98, 82, 74, 70, 66, 66, 0 } },
    {      0x39e,                    { 127, 0, 0, 62, 0, 0, 127, 0 } },
    {      0x39f,                  { 60, 66, 66, 66, 66, 66, 60, 0 } },
    {      0x3a0,                 { 126, 66, 66, 66, 66, 66, 66, 0 } },
    {      0x3a1,                { 124, 66, 66, 124, 64, 64, 64, 0 } },
    {      0x3a2,                   { 65, 65, 65, 34, 34, 20, 8, 0 } },
    {      0x3a3,                 { 126, 32, 16, 8, 16, 32, 126, 0 } },
    {      0x3a4,                      { 127, 73, 8, 8, 8, 8, 8, 0 } },
    {      0x3a5,                     { 65, 65, 65, 62, 8, 8, 8, 0 } },
    {      0x3a6,                     { 8, 62, 73, 73, 62, 8, 8, 0 } },
    {      0x3a7,                   { 65, 34, 20, 8, 20, 34, 65, 0 } },
    {      0x3a8,                     { 73, 73, 73, 62, 8, 8, 8, 0 } },
    {      0x3a9,                  { 62, 65, 65, 65, 99, 20, 99, 0 } },
    {      0x3aa,                      { 34, 0, 28, 8, 8, 8, 28, 0 } },
    {      0x3ab,                     { 20, 65, 65, 62, 8, 8, 8, 0 } },
    {      0x3ac,                    { 4, 8, 58, 68, 68, 68, 59, 0 } },
    {      0x3ad,                    { 4, 8, 62, 64, 60, 64, 62, 0 } },
    {      0x3ae,                    { 4, 8, 96, 60, 36, 36, 38, 0 } },
    {      0x3af,                        { 4, 8, 0, 8, 8, 8, 12, 0 } },
    {      0x3b0,                    { 4, 42, 0, 68, 66, 66, 60, 0 } },
    {      0x3b1,                    { 0, 0, 58, 68, 68, 68, 59, 0 } },
    {      0x3b2,                { 60, 66, 124, 66, 66, 124, 64, 0 } },
    {      0x3b3,                     { 0, 0, 34, 34, 34, 28, 8, 8 } },
    {      0x3b4,                  { 28, 32, 16, 60, 66, 66, 60, 0 } },
    {      0x3b5,                    { 0, 0, 62, 64, 60, 64, 62, 0 } },
    {      0x3b6,                   { 126, 8, 32, 64, 56, 2, 12, 0 } },
    {      0x3b7,                    { 0, 0, 96, 60, 36, 36, 38, 0 } },
    {      0x3b8,                  { 28, 34, 34, 62, 34, 34, 28, 0 } },
    {      0x3b9,                        { 0, 0, 8, 8, 8, 8, 12, 0 } },
    {      0x3ba,                    { 0, 0, 36, 40, 48, 40, 38, 0 } },
    {      0x3bb,                  { 64, 32, 16, 24, 36, 36, 66, 0 } },
    {      0x3bc,                   { 0, 0, 34, 34, 34, 61, 32, 64 } },
    {      0x3bd,                    { 0, 0, 66, 66, 36, 36, 24, 0 } },
    {      0x3be,                  { 28, 32, 32, 24, 32, 64, 60, 6 } },
    {      0x3bf,                    { 0, 0, 60, 66, 66, 66, 60, 0 } },
    {    0x003C0, { 0x00, 0x00, 0x03, 0x3e, 0x76, 0x36, 0x36, 0x00 } }, // GREEK SMALL LETTER PI
    {      0x3c1,                   { 0, 0, 60, 66, 98, 92, 64, 64 } },
    {      0x3c2,                    { 0, 0, 62, 64, 64, 60, 2, 12 } },
    {      0x3c3,                    { 0, 0, 62, 72, 68, 68, 56, 0 } },
    {      0x3c4,                        { 0, 0, 62, 8, 8, 8, 8, 0 } },
    {      0x3c5,                    { 0, 0, 68, 66, 66, 66, 60, 0 } },
    {      0x3c6,                     { 0, 0, 54, 73, 73, 62, 8, 8 } },
    {      0x3c7,                    { 0, 0, 66, 36, 24, 36, 66, 0 } },
    {      0x3c8,                      { 0, 0, 73, 73, 62, 8, 8, 8 } },
    {      0x3c9,                    { 0, 0, 65, 73, 73, 73, 54, 0 } },
    {      0x3ca,                       { 34, 0, 8, 8, 8, 8, 12, 0 } },
    {      0x3cb,                   { 36, 0, 68, 66, 66, 66, 60, 0 } },
    {      0x3cc,                    { 4, 8, 60, 66, 66, 66, 60, 0 } },
    {      0x3cd,                   { 8, 16, 68, 66, 66, 66, 60, 0 } },
    {      0x3ce,                    { 4, 8, 65, 73, 73, 73, 54, 0 } },
    {      0x3cf,                 { 66, 68, 72, 112, 72, 68, 66, 4 } },
    {      0x3d0,                 { 48, 72, 112, 92, 98, 66, 60, 0 } },
    {      0x3d1,                   { 28, 34, 34, 31, 2, 98, 28, 0 } },
    {      0x3d2,                     { 65, 34, 34, 28, 8, 8, 8, 0 } },
    {      0x3d3,                    { 65, 162, 34, 28, 8, 8, 8, 0 } },
    {      0x3d4,                    { 20, 65, 34, 34, 28, 8, 8, 0 } },
    {      0x3d5,                    { 0, 8, 62, 73, 73, 73, 62, 8 } },
    {      0x3d6,                   { 0, 0, 127, 73, 73, 73, 54, 0 } },
    {      0x3d7,                 { 0, 102, 36, 44, 52, 100, 70, 2 } },
    {      0x3d8,                  { 62, 65, 65, 65, 65, 65, 62, 8 } },
    {      0x3d9,                    { 0, 0, 62, 65, 65, 65, 62, 8 } },
    {      0x3da,                   { 2, 60, 64, 64, 64, 60, 2, 12 } },
    {      0x3db,                    { 0, 2, 60, 64, 64, 60, 2, 12 } },
    {      0x3dc,                { 126, 64, 64, 120, 72, 64, 64, 0 } },
    {      0x3dd,                 { 0, 0, 126, 64, 64, 120, 64, 64 } },
    {      0x3de,                       { 0, 0, 8, 16, 62, 4, 8, 0 } },
    {      0x3df,                      { 4, 8, 16, 62, 4, 8, 16, 0 } },
    {      0x3f0,                 { 0, 102, 36, 44, 52, 100, 70, 2 } },
    {      0x3f1,                  { 0, 0, 60, 66, 66, 124, 64, 60 } },
    {      0x3f2,                    { 0, 0, 62, 64, 64, 64, 62, 0 } },
    {      0x3f3,                      { 4, 0, 12, 4, 4, 4, 36, 24 } },
    {      0x3f4,                 { 28, 34, 65, 127, 65, 34, 28, 0 } },
    {      0x3f5,                   { 0, 0, 62, 64, 124, 64, 62, 0 } },
    {      0x3f6,                    { 0, 0, 124, 2, 62, 2, 124, 0 } },
    {      0x3f7,                 { 0, 64, 124, 66, 66, 124, 64, 0 } },
    {      0x3f8,                 { 0, 64, 92, 98, 66, 124, 64, 64 } },
    {      0x3f9,                  { 30, 32, 64, 64, 64, 32, 30, 0 } },
    {      0x3fa,                 { 66, 102, 90, 90, 66, 66, 66, 0 } },
    {      0x3fb,                   { 0, 0, 66, 102, 90, 66, 66, 0 } },
    {      0x3fc,                 { 0, 14, 49, 33, 62, 32, 120, 32 } },
    {      0x3fd,                     { 56, 68, 2, 2, 2, 68, 56, 0 } },
    {      0x3fe,                  { 28, 34, 64, 72, 64, 34, 28, 0 } },
    {      0x3ff,                    { 56, 68, 2, 18, 2, 68, 56, 0 } },
    // Cyrillic
    {      0x400,               { 32, 16, 126, 64, 120, 64, 126, 0 } },
    {      0x401,                { 36, 0, 126, 64, 120, 64, 126, 0 } },
    {      0x402,                 { 124, 16, 22, 25, 17, 17, 22, 0 } },
    {      0x403,                    { 4, 8, 62, 32, 32, 32, 32, 0 } },
    {      0x404,                 { 30, 32, 64, 124, 64, 32, 30, 0 } },
    {      0x405,                   { 60, 66, 64, 60, 2, 66, 60, 0 } },
    {      0x406,                       { 28, 8, 8, 8, 8, 8, 28, 0 } },
    {      0x407,                      { 20, 0, 28, 8, 8, 8, 28, 0 } },
    {      0x408,                      { 14, 4, 4, 4, 4, 68, 56, 0 } },
    {      0x409,                  { 60, 20, 20, 22, 21, 85, 38, 0 } },
    {      0x40a,                 { 72, 72, 72, 124, 74, 74, 78, 0 } },
    {      0x40b,                 { 124, 16, 22, 25, 17, 17, 17, 0 } },
    {      0x40c,                  { 8, 84, 72, 112, 72, 68, 66, 0 } },
    {      0x40d,                   { 16, 8, 34, 38, 42, 50, 34, 0 } },
    {      0x40e,                  { 32, 16, 68, 68, 56, 4, 120, 0 } },
    {      0x40f,                { 66, 66, 66, 66, 66, 66, 126, 16 } },
    {      0x410,                 { 24, 36, 66, 126, 66, 66, 66, 0 } },
    {      0x411,                { 126, 32, 32, 60, 34, 34, 124, 0 } },
    {      0x412,                { 124, 34, 34, 60, 34, 34, 124, 0 } },
    {      0x413,                  { 62, 32, 32, 32, 32, 32, 32, 0 } },
    {      0x414,                { 28, 34, 34, 34, 34, 34, 127, 65 } },
    {      0x415,               { 126, 64, 64, 120, 64, 64, 126, 0 } },
    {      0x416,                  { 73, 42, 42, 28, 42, 42, 73, 0 } },
    {      0x417,                      { 60, 2, 2, 12, 2, 2, 60, 0 } },
    {      0x418,                  { 34, 38, 38, 42, 42, 50, 34, 0 } },
    {      0x419,                  { 18, 12, 34, 38, 42, 50, 34, 0 } },
    {      0x41a,                 { 66, 68, 72, 112, 72, 68, 66, 0 } },
    {      0x41b,                  { 14, 18, 34, 34, 34, 34, 34, 0 } },
    {      0x41c,                 { 66, 102, 90, 90, 66, 66, 66, 0 } },
    {      0x41d,                 { 66, 66, 66, 126, 66, 66, 66, 0 } },
    {      0x41e,                  { 24, 36, 66, 66, 66, 36, 24, 0 } },
    {      0x41f,                  { 62, 34, 34, 34, 34, 34, 34, 0 } },
    {      0x420,                { 124, 66, 66, 124, 64, 64, 64, 0 } },
    {      0x421,                  { 28, 34, 64, 64, 64, 34, 28, 0 } },
    {      0x422,                        { 62, 8, 8, 8, 8, 8, 8, 0 } },
    {      0x423,                    { 34, 34, 34, 28, 2, 2, 60, 0 } },
    {      0x424,                     { 8, 62, 42, 42, 62, 8, 8, 0 } },
    {      0x425,                  { 66, 66, 36, 24, 36, 66, 66, 0 } },
    {      0x426,                 { 68, 68, 68, 68, 68, 68, 126, 2 } },
    {      0x427,                     { 34, 34, 34, 62, 2, 2, 2, 0 } },
    {      0x428,                  { 42, 42, 42, 42, 42, 42, 62, 0 } },
    {      0x429,                 { 84, 84, 84, 84, 84, 84, 126, 2 } },
    {      0x42a,                 { 224, 32, 32, 60, 34, 34, 60, 0 } },
    {      0x42b,                { 66, 66, 66, 114, 74, 74, 114, 0 } },
    {      0x42c,                  { 32, 32, 32, 60, 34, 34, 60, 0 } },
    {      0x42d,                      { 60, 2, 2, 62, 2, 2, 60, 0 } },
    {      0x42e,                  { 38, 41, 41, 57, 41, 41, 38, 0 } },
    {      0x42f,                  { 30, 34, 34, 30, 10, 18, 34, 0 } },
    {      0x430,                     { 0, 0, 56, 4, 60, 68, 58, 0 } },
    {      0x431,                  { 28, 32, 44, 50, 50, 34, 28, 0 } },
    {      0x432,                  { 0, 0, 124, 34, 60, 34, 124, 0 } },
    {      0x433,                    { 0, 0, 62, 32, 32, 32, 32, 0 } },
    {      0x434,                  { 0, 0, 28, 34, 34, 34, 127, 65 } },
    {      0x435,                   { 0, 0, 60, 66, 126, 64, 60, 0 } },
    {      0x436,                    { 0, 0, 73, 42, 28, 42, 73, 0 } },
    {      0x437,                      { 0, 0, 60, 2, 12, 2, 60, 0 } },
    {      0x438,                    { 0, 0, 34, 38, 42, 50, 34, 0 } },
    {      0x439,                  { 18, 12, 34, 38, 42, 50, 34, 0 } },
    {      0x43a,                    { 0, 0, 34, 36, 56, 36, 34, 0 } },
    {      0x43b,                    { 0, 0, 14, 18, 34, 34, 34, 0 } },
    {      0x43c,                   { 0, 0, 66, 102, 90, 66, 66, 0 } },
    {      0x43d,                   { 0, 0, 66, 66, 126, 66, 66, 0 } },
    {      0x43e,                    { 0, 0, 60, 66, 66, 66, 60, 0 } },
    {      0x43f,                    { 0, 0, 62, 34, 34, 34, 34, 0 } },
    {      0x440,                  { 0, 0, 92, 98, 66, 124, 64, 64 } },
    {      0x441,                    { 0, 0, 60, 66, 64, 66, 60, 0 } },
    {      0x442,                        { 0, 0, 62, 8, 8, 8, 8, 0 } },
    {      0x443,                     { 0, 0, 34, 34, 28, 2, 60, 0 } },
    {      0x444,                    { 24, 8, 62, 42, 42, 62, 8, 8 } },
    {      0x445,                    { 0, 0, 66, 36, 24, 36, 66, 0 } },
    {      0x446,                   { 0, 0, 68, 68, 68, 68, 126, 2 } },
    {      0x447,                      { 0, 0, 34, 34, 62, 2, 2, 0 } },
    {      0x448,                    { 0, 0, 42, 42, 42, 42, 62, 0 } },
    {      0x449,                   { 0, 0, 84, 84, 84, 84, 126, 2 } },
    {      0x44a,                   { 0, 0, 112, 16, 30, 17, 30, 0 } },
    {      0x44b,                  { 0, 0, 66, 66, 114, 74, 114, 0 } },
    {      0x44c,                    { 0, 0, 32, 32, 60, 34, 60, 0 } },
    {      0x44d,                      { 0, 0, 60, 2, 62, 2, 60, 0 } },
    {      0x44e,                   { 0, 0, 76, 82, 114, 82, 76, 0 } },
    {      0x44f,                    { 0, 0, 30, 34, 30, 10, 18, 0 } },
    {      0x450,                  { 16, 8, 60, 66, 126, 64, 60, 0 } },
    {      0x451,                  { 36, 0, 60, 66, 126, 64, 60, 0 } },
    {      0x452,                 { 32, 120, 32, 44, 50, 34, 34, 6 } },
    {      0x453,                    { 4, 8, 62, 32, 32, 32, 32, 0 } },
    {      0x454,                   { 0, 0, 60, 64, 120, 64, 60, 0 } },
    {      0x455,                    { 0, 0, 62, 64, 60, 2, 124, 0 } },
    {      0x456,                       { 8, 0, 24, 8, 8, 8, 28, 0 } },
    {      0x457,                      { 20, 0, 24, 8, 8, 8, 28, 0 } },
    {      0x458,                      { 4, 0, 12, 4, 4, 4, 68, 56 } },
    {      0x459,                    { 0, 0, 60, 20, 22, 85, 38, 0 } },
    {      0x45a,                   { 0, 0, 72, 72, 124, 74, 78, 0 } },
    {      0x45b,                 { 32, 120, 32, 44, 50, 34, 34, 0 } },
    {      0x45c,                    { 4, 8, 34, 36, 56, 36, 34, 0 } },
    {      0x45d,                   { 16, 8, 34, 38, 42, 50, 34, 0 } },
    {      0x45e,                    { 16, 8, 34, 34, 28, 2, 60, 0 } },
    {      0x45f,                  { 0, 0, 66, 66, 66, 66, 126, 16 } },
    {      0x482,                    { 1, 10, 4, 42, 16, 40, 64, 0 } },
    {      0x490,                   { 2, 62, 32, 32, 32, 32, 32, 0 } },
    {      0x491,                    { 0, 2, 62, 32, 32, 32, 32, 0 } },
    {      0x492,                 { 62, 32, 32, 120, 32, 32, 32, 0 } },
    {      0x493,                   { 0, 0, 62, 32, 120, 32, 32, 0 } },
    {      0x4a0,                  { 97, 34, 36, 56, 36, 34, 33, 0 } },
    {      0x4a1,                    { 0, 0, 98, 36, 56, 36, 34, 0 } },
    {      0x4a2,                 { 68, 68, 68, 124, 68, 68, 70, 2 } },
    {      0x4a3,                   { 0, 0, 68, 68, 124, 68, 70, 2 } },
    {      0x4aa,                 { 28, 34, 64, 64, 64, 34, 28, 24 } },
    {      0x4ab,                   { 0, 0, 60, 66, 64, 66, 60, 24 } },
    {      0x4b0,                    { 34, 34, 34, 28, 8, 62, 8, 0 } },
    {      0x4b1,                     { 0, 0, 34, 34, 28, 8, 62, 8 } },
    {      0x4ba,                 { 64, 64, 64, 124, 66, 66, 66, 0 } },
    {      0x4bb,                   { 0, 0, 64, 64, 124, 66, 66, 0 } },
    {      0x4d8,                   { 60, 2, 2, 126, 66, 66, 60, 0 } },
    {      0x4d9,                    { 0, 0, 60, 2, 126, 66, 60, 0 } },
    {      0x4e8,                 { 24, 36, 66, 126, 66, 36, 24, 0 } },
    {      0x4e9,                   { 0, 0, 60, 66, 126, 66, 60, 0 } },
    // Punctations
    {     0x2010,                        { 0, 0, 0, 60, 0, 0, 0, 0 } },
    {     0x2011,                        { 0, 0, 0, 60, 0, 0, 0, 0 } },
    {     0x2012,                       { 0, 0, 0, 126, 0, 0, 0, 0 } },
    {     0x2013,                       { 0, 0, 0, 126, 0, 0, 0, 0 } },
    {     0x2014,                       { 0, 0, 0, 127, 0, 0, 0, 0 } },
    {     0x2015,                       { 0, 0, 0, 127, 0, 0, 0, 0 } },
    {     0x2016,                  { 36, 36, 36, 36, 36, 36, 36, 0 } },
    {     0x2017,                     { 0, 0, 0, 0, 0, 127, 0, 127 } },
    {     0x2018,                         { 4, 8, 8, 0, 0, 0, 0, 0 } },
    {     0x2019,                        { 8, 8, 16, 0, 0, 0, 0, 0 } },
    {     0x201a,                      { 0, 0, 0, 0, 0, 16, 16, 32 } },
    {     0x201b,                       { 16, 16, 8, 0, 0, 0, 0, 0 } },
    {     0x201c,                      { 18, 36, 36, 0, 0, 0, 0, 0 } },
    {     0x201d,                      { 36, 36, 72, 0, 0, 0, 0, 0 } },
    {     0x201e,                      { 0, 0, 0, 0, 0, 36, 36, 72 } },
    {     0x201f,                      { 36, 36, 18, 0, 0, 0, 0, 0 } },
    {     0x2020,                        { 8, 8, 8, 62, 8, 8, 8, 8 } },
    {     0x2021,                       { 8, 8, 62, 8, 8, 62, 8, 8 } },
    {     0x2022,                    { 0, 28, 62, 62, 62, 28, 0, 0 } },
    {     0x2023,                  { 16, 24, 28, 30, 28, 24, 16, 0 } },
    {     0x2024,                         { 0, 0, 0, 0, 0, 0, 8, 0 } },
    {     0x2025,                        { 0, 0, 0, 0, 0, 0, 36, 0 } },
    {     0x2026,                        { 0, 0, 0, 0, 0, 0, 42, 0 } },
    {     0x2027,                         { 0, 0, 0, 8, 0, 0, 0, 0 } },
    {     0x2030,                   { 0, 98, 100, 8, 16, 42, 74, 0 } },
    {     0x2031,                { 0, 100, 104, 16, 32, 85, 149, 0 } },
    {     0x2032,                        { 4, 8, 16, 0, 0, 0, 0, 0 } },
    {     0x2033,                      { 18, 36, 72, 0, 0, 0, 0, 0 } },
    {     0x2034,                      { 21, 42, 84, 0, 0, 0, 0, 0 } },
    {     0x2035,                        { 16, 8, 4, 0, 0, 0, 0, 0 } },
    {     0x2036,                      { 72, 36, 18, 0, 0, 0, 0, 0 } },
    {     0x2037,                      { 84, 42, 21, 0, 0, 0, 0, 0 } },
    {     0x2038,                       { 0, 0, 0, 0, 0, 8, 20, 34 } },
    {     0x2039,                      { 0, 8, 16, 32, 16, 8, 0, 0 } },
    {     0x203a,                       { 0, 16, 8, 4, 8, 16, 0, 0 } },
    {     0x203b,                    { 0, 0, 42, 20, 42, 20, 42, 0 } },
    {     0x2047,                  { 108, 18, 18, 36, 36, 0, 36, 0 } },
    {     0x2048,                   { 50, 74, 10, 18, 18, 0, 18, 0 } },
    {     0x2049,                   { 76, 82, 66, 68, 68, 0, 68, 0 } },
    {     0x204a,                        { 0, 0, 0, 62, 2, 2, 0, 0 } },
    {     0x2055,                   { 8, 42, 28, 127, 28, 42, 8, 0 } },
    {     0x205c,                     { 8, 42, 8, 127, 8, 42, 8, 0 } },
    // Currency
    {     0x20a0,                   { 28, 32, 79, 72, 62, 8, 15, 0 } },
    {     0x20a1,                  { 20, 30, 34, 64, 64, 62, 20, 0 } },
    {     0x20a2,                  { 30, 33, 64, 75, 76, 41, 30, 0 } },
    {     0x20a3,                { 126, 64, 64, 123, 76, 72, 72, 0 } },
    {     0x20a4,               { 24, 36, 112, 32, 112, 32, 126, 0 } },
    {     0x20a5,                   { 0, 1, 118, 77, 73, 89, 32, 0 } },
    {     0x20a6,                { 66, 98, 255, 74, 255, 66, 66, 0 } },
    {     0x20a7,                { 112, 76, 78, 116, 68, 68, 67, 0 } },
    {     0x20a8,                { 112, 72, 75, 114, 75, 73, 75, 0 } },
    {     0x20a9,                 { 34, 34, 127, 34, 42, 54, 34, 0 } },
    {     0x20aa,                 { 121, 69, 85, 85, 85, 81, 94, 0 } },
    {     0x20ab,                    { 4, 30, 4, 60, 68, 60, 0, 60 } },
    {     0x20ac,                { 15, 16, 126, 32, 124, 16, 15, 0 } },
    {     0x20ad,                 { 34, 36, 40, 127, 36, 34, 33, 0 } },
    {     0x20ae,                    { 127, 73, 8, 62, 8, 62, 8, 0 } },
    {     0x20af,                { 240, 72, 74, 77, 77, 78, 244, 4 } },
    {     0x20b0,                { 115, 76, 78, 116, 68, 68, 72, 0 } },
    {     0x20b1,                { 60, 34, 127, 34, 127, 32, 32, 0 } },
    {     0x20b2,                    { 8, 62, 72, 75, 73, 62, 8, 0 } },
    {     0x20b3,                 { 8, 20, 127, 34, 127, 34, 34, 0 } },
    {     0x20b4,                  { 60, 2, 127, 8, 127, 32, 30, 0 } },
    {     0x20b5,                    { 8, 62, 72, 72, 72, 62, 8, 0 } },
    {     0x20b6,                { 60, 34, 34, 124, 32, 120, 32, 0 } },
    {     0x20b9,                    { 126, 4, 126, 8, 48, 8, 6, 0 } },
    {     0x20ba,                  { 16, 20, 24, 52, 88, 50, 92, 0 } },
    // Letterlike
    {     0x2100,                   { 57, 74, 60, 8, 23, 36, 71, 0 } },
    {     0x2101,                  { 57, 74, 63, 12, 23, 33, 71, 0 } },
    {     0x2102,                  { 28, 50, 80, 80, 80, 50, 28, 0 } },
    {     0x2103,                      { 96, 102, 9, 8, 8, 9, 6, 0 } },
    {     0x2104,                  { 16, 60, 80, 80, 60, 16, 31, 0 } },
    {     0x2105,                   { 49, 66, 52, 8, 23, 37, 71, 0 } },
    {     0x2106,                   { 49, 66, 52, 8, 21, 37, 71, 0 } },
    {     0x2107,                   { 2, 62, 66, 48, 64, 66, 60, 0 } },
    {     0x2108,                    { 120, 4, 2, 30, 2, 4, 120, 0 } },
    {     0x2109,                     { 96, 111, 8, 14, 8, 8, 8, 0 } },
    {     0x210a,                  { 0, 58, 68, 56, 64, 60, 66, 60 } },
    {     0x210b,               { 38, 102, 36, 44, 52, 102, 100, 0 } },
    {     0x210c,                  { 60, 64, 46, 17, 17, 81, 34, 0 } },
    {     0x210d,                 { 0, 114, 82, 94, 82, 82, 114, 0 } },
    {     0x210e,                  { 32, 96, 44, 50, 34, 34, 35, 0 } },
    {     0x210f,                { 40, 112, 108, 50, 34, 34, 35, 0 } },
    {     0x2110,                    { 2, 14, 18, 4, 28, 42, 48, 0 } },
    {     0x2111,                     { 50, 76, 4, 4, 2, 34, 92, 0 } },
    {     0x2112,                 { 24, 36, 120, 32, 32, 34, 92, 0 } },
    {     0x2113,                    { 8, 20, 20, 56, 16, 20, 8, 0 } },
    {     0x2114,                 { 40, 126, 40, 46, 41, 41, 46, 0 } },
    {     0x2115,                  { 0, 98, 82, 106, 86, 74, 70, 0 } },
    {     0x2116,           { 136, 136, 200, 171, 155, 136, 139, 0 } },
    {     0x2117,                  { 28, 34, 93, 93, 81, 34, 28, 0 } },
    {     0x2118,                  { 38, 73, 81, 38, 80, 80, 32, 0 } },
    {     0x2119,                 { 0, 124, 82, 82, 92, 80, 112, 0 } },
    {     0x211a,                   { 0, 60, 82, 82, 86, 82, 61, 0 } },
    {     0x211b,                  { 62, 81, 18, 28, 82, 82, 49, 0 } },
    {     0x211c,                  { 44, 82, 17, 30, 18, 82, 33, 0 } },
    {     0x211d,                 { 0, 124, 82, 82, 92, 84, 114, 0 } },
    {     0x211e,                { 124, 66, 66, 124, 74, 68, 74, 0 } },
    {     0x211f,                { 24, 4, 124, 74, 124, 84, 98, 32 } },
    {     0x2120,                   { 106, 78, 42, 106, 0, 0, 0, 0 } },
    {     0x2121,                  { 0, 250, 82, 90, 82, 82, 91, 0 } },
    {     0x2122,                    { 122, 46, 42, 42, 0, 0, 0, 0 } },
    {     0x2123,                    { 0, 8, 69, 73, 50, 52, 72, 0 } },
    {     0x2124,                 { 0, 126, 10, 20, 40, 80, 126, 0 } },
    {     0x2125,                    { 60, 8, 16, 60, 8, 24, 4, 56 } },
    {     0x2126,                  { 0, 62, 65, 65, 65, 34, 119, 0 } },
    {     0x2127,                  { 0, 119, 34, 65, 65, 65, 62, 0 } },
    {     0x2128,                     { 0, 0, 60, 4, 24, 4, 36, 24 } },
    {     0x2129,                        { 0, 0, 48, 8, 8, 8, 8, 0 } },
    {     0x212a,                 { 66, 68, 72, 80, 104, 68, 66, 0 } },
    {     0x212b,                 { 24, 36, 24, 36, 66, 126, 66, 0 } },
    {     0x212c,                  { 60, 82, 18, 28, 82, 82, 60, 0 } },
    {     0x212d,                  { 62, 80, 72, 88, 64, 34, 28, 0 } },
    {     0x212e,                  { 0, 0, 60, 102, 126, 96, 60, 0 } },
    {     0x212f,                   { 0, 0, 28, 34, 124, 64, 60, 0 } },
    {     0x2130,                  { 28, 34, 64, 56, 64, 66, 60, 0 } },
    {     0x2131,                  { 62, 80, 16, 60, 16, 80, 32, 0 } },
    {     0x2132,                      { 2, 2, 2, 62, 2, 2, 126, 0 } },
    {     0x2133,                 { 34, 86, 90, 90, 66, 194, 67, 0 } },
    {     0x2134,                    { 0, 0, 28, 34, 66, 68, 56, 0 } },
    {     0x2135,                    { 0, 0, 66, 34, 60, 68, 66, 0 } },
    {     0x2136,                      { 0, 56, 4, 4, 4, 4, 126, 0 } },
    {     0x2137,                     { 0, 48, 8, 8, 20, 36, 66, 0 } },
    {     0x2138,                       { 0, 126, 4, 4, 4, 4, 4, 0 } },
    {     0x2139,                    { 0, 24, 0, 56, 24, 24, 60, 0 } },
    {     0x213a,                    { 0, 0, 61, 66, 69, 65, 62, 0 } },
    {     0x213b,               { 0, 0, 237, 149, 214, 157, 149, 0 } },
    {     0x213c,                   { 0, 0, 127, 42, 42, 42, 57, 0 } },
    {     0x213d,                  { 0, 0, 114, 42, 20, 40, 112, 0 } },
    {     0x213e,                { 126, 80, 80, 80, 80, 80, 112, 0 } },
    {     0x213f,                { 127, 85, 85, 85, 85, 85, 119, 0 } },
    {     0x2140,                { 126, 80, 40, 20, 40, 80, 126, 0 } },
    {     0x2141,                  { 56, 68, 66, 114, 2, 68, 56, 0 } },
    {     0x2142,                       { 126, 2, 2, 2, 2, 2, 2, 0 } },
    {     0x2143,                       { 2, 2, 2, 2, 2, 2, 126, 0 } },
    {     0x2144,                     { 8, 8, 8, 28, 34, 34, 34, 0 } },
    {     0x2145,                  { 0, 30, 21, 41, 41, 82, 124, 0 } },
    {     0x2146,                     { 0, 7, 5, 58, 74, 84, 56, 0 } },
    {     0x2147,                    { 0, 0, 28, 42, 94, 80, 60, 0 } },
    {     0x2148,                    { 0, 6, 0, 31, 10, 20, 126, 0 } },
    {     0x2149,                     { 0, 3, 0, 15, 5, 10, 84, 56 } },
    {     0x214a,                 { 16, 124, 82, 82, 92, 16, 30, 0 } },
    {     0x214b,                  { 92, 34, 82, 12, 18, 18, 12, 0 } },
    {     0x214d,                 { 33, 82, 119, 92, 23, 33, 71, 0 } },
    {     0x214e,                      { 0, 0, 2, 2, 30, 2, 126, 0 } },
    // Arrows
    {     0x2190,                   { 0, 0, 16, 32, 126, 32, 16, 0 } },
    {     0x2191,                       { 8, 28, 42, 8, 8, 8, 8, 0 } },
    {     0x2192,                       { 0, 0, 8, 4, 126, 4, 8, 0 } },
    {     0x2193,                       { 8, 8, 8, 8, 42, 28, 8, 0 } },
    {     0x2194,                   { 0, 0, 20, 34, 127, 34, 20, 0 } },
    {     0x2195,                     { 8, 28, 42, 8, 42, 28, 8, 0 } },
    {     0x2196,                    { 0, 120, 96, 80, 72, 4, 2, 0 } },
    {     0x2197,                    { 0, 30, 6, 10, 18, 32, 64, 0 } },
    {     0x2198,                    { 0, 64, 32, 18, 10, 6, 30, 0 } },
    {     0x2199,                    { 0, 2, 4, 72, 80, 96, 120, 0 } },
    {     0x21b5,                   { 0, 2, 18, 34, 126, 32, 16, 0 } },
    {     0x21d0,                    { 0, 0, 16, 62, 64, 62, 16, 0 } },
    {     0x21d1,                   { 8, 20, 54, 20, 20, 20, 20, 0 } },
    {     0x21d2,                     { 0, 0, 8, 124, 2, 124, 8, 0 } },
    {     0x21d3,                   { 20, 20, 20, 20, 54, 20, 8, 0 } },
    {     0x21d4,                    { 0, 0, 20, 62, 65, 62, 20, 0 } },
    {     0x21d5,                    { 8, 20, 54, 20, 54, 20, 8, 0 } },
    {     0x21e0,                   { 0, 0, 16, 32, 106, 32, 16, 0 } },
    {     0x21e1,                       { 8, 28, 42, 0, 8, 0, 8, 0 } },
    {     0x21e2,                        { 0, 0, 8, 4, 86, 4, 8, 0 } },
    {     0x21e3,                       { 8, 0, 8, 0, 42, 28, 8, 0 } },
    {     0x21e4,                   { 0, 0, 72, 80, 127, 80, 72, 0 } },
    {     0x21e5,                       { 0, 0, 9, 5, 127, 5, 9, 0 } },
    {     0x27f2,                   { 0, 60, 226, 65, 1, 66, 60, 0 } },
    {     0x27f3,                   { 0, 60, 71, 66, 64, 66, 60, 0 } },
    {     0x2912,                      { 62, 8, 28, 42, 8, 8, 8, 0 } },
    {     0x2913,                      { 8, 8, 8, 42, 28, 8, 62, 0 } },
    {     0x2b90,                   { 6, 2, 18, 34, 126, 32, 16, 0 } },
    // Keyboard
    {     0x21e7,                   { 8, 20, 34, 99, 34, 34, 62, 0 } },
    {     0x21ea,                    { 8, 20, 34, 99, 62, 0, 62, 0 } },
    {     0x2302,                   { 0, 8, 20, 34, 65, 65, 127, 0 } },
    {     0x2318,              { 119, 85, 127, 20, 127, 85, 119, 0 } },
    {     0x231a,                 { 62, 62, 69, 121, 65, 62, 62, 0 } },
    {     0x231b,               { 127, 65, 62, 28, 42, 127, 127, 0 } },
    {     0x2324,                     { 107, 20, 34, 0, 0, 0, 0, 0 } },
    {     0x2325,                      { 0, 0, 0, 118, 16, 8, 6, 0 } },
    {     0x2326,               { 0, 0, 252, 170, 145, 170, 252, 0 } },
    {     0x2327,                  { 0, 0, 127, 85, 73, 85, 127, 0 } },
    {     0x2328,                { 127, 85, 65, 85, 65, 93, 127, 0 } },
    {     0x232b,                   { 0, 0, 63, 85, 137, 85, 63, 0 } },
    // Mathematical
    {     0x2200,                 { 66, 66, 66, 126, 66, 36, 24, 0 } },
    {     0x2201,                   { 0, 28, 34, 32, 32, 34, 28, 0 } },
    {     0x2203,                   { 126, 2, 2, 126, 2, 2, 126, 0 } },
    {     0x2204,               { 1, 126, 6, 126, 18, 34, 126, 128 } },
    {     0x2205,                   { 1, 30, 38, 42, 50, 60, 64, 0 } },
    {     0x2208,                    { 0, 0, 28, 32, 56, 32, 28, 0 } },
    {     0x2209,                   { 0, 2, 28, 40, 56, 48, 60, 64 } },
    {     0x220b,                      { 0, 0, 56, 4, 28, 4, 56, 0 } },
    {     0x220c,                   { 0, 2, 60, 12, 28, 20, 56, 64 } },
    {     0x220e,               { 0, 0, 126, 126, 126, 126, 126, 0 } },
    {     0x2213,                       { 0, 0, 62, 0, 8, 62, 8, 0 } },
    {     0x2216,                       { 0, 0, 32, 16, 8, 4, 2, 0 } },
    {     0x2218,                      { 0, 0, 0, 28, 20, 28, 0, 0 } },
    {     0x221a,                   { 1, 2, 196, 72, 80, 96, 64, 0 } },
    {     0x2220,                     { 0, 0, 4, 8, 16, 32, 126, 0 } },
    {     0x2223,                         { 8, 8, 8, 8, 8, 8, 8, 0 } },
    {     0x2227,                       { 0, 0, 0, 34, 20, 8, 0, 0 } },
    {     0x2228,                       { 0, 0, 0, 8, 20, 34, 0, 0 } },
    {     0x2229,                    { 0, 0, 28, 34, 34, 34, 34, 0 } },
    {     0x222a,                    { 0, 0, 34, 34, 34, 34, 28, 0 } },
    {     0x2234,                   { 0, 0, 24, 24, 0, 102, 102, 0 } },
    {     0x223c,                       { 0, 0, 16, 42, 4, 0, 0, 0 } },
    {     0x2245,                   { 50, 76, 0, 126, 0, 126, 0, 0 } },
    {     0x2260,                     { 0, 4, 62, 8, 62, 16, 32, 0 } },
    {     0x2261,                   { 0, 126, 0, 126, 0, 126, 0, 0 } },
    {     0x2264,                       { 4, 8, 16, 8, 4, 0, 28, 0 } },
    {     0x2265,                      { 16, 8, 4, 8, 16, 0, 28, 0 } },
    {     0x2282,                     { 0, 0, 30, 32, 32, 30, 0, 0 } },
    {     0x2283,                       { 0, 0, 60, 2, 2, 60, 0, 0 } },
    {     0x2284,                    { 0, 64, 60, 80, 72, 60, 2, 0 } },
    {     0x2286,                    { 0, 30, 32, 32, 30, 0, 62, 0 } },
    {     0x2287,                      { 0, 60, 2, 2, 60, 0, 62, 0 } },
    {     0x2295,                   { 8, 28, 42, 127, 42, 28, 8, 0 } },
    {     0x2296,                   { 0, 28, 34, 127, 34, 28, 0, 0 } },
    {     0x2297,                    { 0, 28, 54, 42, 54, 28, 0, 0 } },
    {     0x22a0,                    { 0, 0, 62, 54, 42, 54, 62, 0 } },
    {     0x22a1,                    { 0, 0, 62, 34, 42, 34, 62, 0 } },
    {     0x22a2,                    { 0, 0, 32, 32, 62, 32, 32, 0 } },
    {     0x22a3,                        { 0, 0, 2, 2, 62, 2, 2, 0 } },
    {     0x22a4,                        { 0, 0, 62, 8, 8, 8, 8, 0 } },
    {     0x22a5,                        { 0, 0, 8, 8, 8, 8, 62, 0 } },
    {     0x22c2,                    { 0, 0, 24, 36, 36, 36, 36, 0 } },
    {     0x22c3,                    { 0, 0, 36, 36, 36, 36, 24, 0 } },
    {     0x22c4,                      { 0, 8, 20, 34, 20, 8, 0, 0 } },
    {     0x22c5,                      { 0, 0, 0, 28, 28, 28, 0, 0 } },
    {     0x22c6,                     { 0, 8, 42, 28, 20, 34, 0, 0 } },
    {     0x2202,                   { 28, 2, 30, 34, 34, 34, 28, 0 } },
    {     0x2206,                      { 0, 0, 0, 8, 20, 34, 62, 0 } },
    {     0x2207,                      { 0, 0, 62, 34, 20, 8, 0, 0 } },
    {     0x220f,                 { 127, 34, 34, 34, 34, 34, 34, 0 } },
    {     0x221d,                     { 0, 0, 54, 72, 72, 54, 0, 0 } },
    {     0x221e,                     { 0, 0, 36, 90, 90, 36, 0, 0 } },
    {     0x222b,                       { 6, 9, 8, 8, 8, 8, 72, 48 } },
    {     0x2248,                     { 0, 32, 84, 8, 32, 84, 8, 0 } },
    {     0x22c0,                       { 0, 8, 20, 34, 0, 0, 0, 0 } },
    {     0x22c1,                       { 0, 34, 20, 8, 0, 0, 0, 0 } },
    // Technical
    {     0x2309,                       { 0, 0, 0, 120, 8, 8, 8, 8 } },
    {     0x2308,                        { 0, 0, 0, 15, 8, 8, 8, 8 } },
    {     0x230a,                        { 8, 8, 8, 15, 0, 0, 0, 0 } },
    {     0x230b,                       { 8, 8, 8, 120, 0, 0, 0, 0 } },
    {     0x2329,                      { 4, 8, 16, 32, 16, 8, 4, 0 } },
    {     0x232a,                     { 32, 16, 8, 4, 8, 16, 32, 0 } },
    {     0x2336,                       { 0, 0, 62, 8, 8, 8, 62, 0 } },
    {     0x2337,                  { 62, 34, 34, 34, 34, 34, 62, 0 } },
    {     0x2339,                  { 62, 42, 34, 62, 34, 42, 62, 0 } },
    {     0x233d,                    { 8, 28, 42, 42, 42, 28, 8, 0 } },
    {     0x233f,                     { 0, 2, 4, 62, 16, 32, 64, 0 } },
    {     0x2340,                      { 0, 32, 16, 62, 4, 2, 1, 0 } },
    {     0x2349,                   { 64, 60, 50, 42, 38, 30, 1, 0 } },
    {     0x234a,                       { 0, 8, 8, 8, 62, 0, 62, 0 } },
    {     0x234b,                      { 8, 8, 20, 42, 62, 8, 8, 0 } },
    {     0x234e,                     { 0, 0, 28, 20, 28, 8, 62, 0 } },
    {     0x2351,                       { 62, 0, 62, 8, 8, 8, 8, 0 } },
    {     0x2352,                      { 8, 8, 62, 42, 20, 8, 8, 0 } },
    {     0x2355,                     { 0, 0, 62, 8, 28, 20, 28, 0 } },
    {     0x2358,                        { 8, 8, 8, 0, 0, 0, 28, 0 } },
    {     0x2359,                     { 0, 8, 20, 34, 62, 0, 62, 0 } },
    {     0x235a,                     { 8, 20, 34, 20, 8, 0, 62, 0 } },
    {     0x235b,                     { 0, 0, 28, 20, 28, 0, 62, 0 } },
    {     0x235c,                   { 28, 34, 34, 34, 28, 0, 62, 0 } },
    {     0x235d,                    { 0, 0, 24, 36, 36, 60, 36, 0 } },
    {     0x235e,                  { 62, 42, 42, 34, 34, 34, 62, 0 } },
    {     0x235f,                    { 0, 28, 42, 62, 42, 28, 0, 0 } },
    {     0x2360,                  { 62, 34, 42, 34, 42, 34, 62, 0 } },
    {     0x236b,                    { 16, 42, 4, 0, 62, 34, 20, 8 } },
    {     0x236e,                       { 8, 0, 8, 8, 16, 0, 60, 0 } },
    {     0x2370,                { 127, 89, 69, 73, 65, 73, 127, 0 } },
    {     0x2371,                     { 16, 42, 4, 0, 34, 20, 8, 0 } },
    {     0x2372,                     { 16, 42, 4, 0, 8, 20, 34, 0 } },
    {     0x2373,                       { 0, 0, 24, 8, 8, 8, 12, 0 } },
    {     0x2374,                   { 0, 28, 34, 50, 44, 32, 32, 0 } },
    {     0x2375,                    { 0, 0, 34, 34, 34, 42, 20, 0 } },
    {     0x2376,                    { 0, 26, 36, 36, 26, 0, 62, 0 } },
    {     0x2377,                   { 28, 32, 56, 32, 28, 0, 62, 0 } },
    {     0x2378,                      { 0, 24, 8, 8, 12, 0, 28, 0 } },
    {     0x2379,                    { 0, 34, 34, 42, 20, 0, 62, 0 } },
    {     0x237a,                    { 0, 0, 26, 36, 36, 36, 26, 0 } },
    {     0x237b,                   { 1, 10, 68, 74, 80, 96, 64, 0 } },
    {     0x237e,                    { 0, 0, 28, 34, 62, 20, 54, 0 } },
    {     0x2395,                  { 0, 0, 126, 66, 66, 66, 126, 0 } },
    {     0x2397,                 { 31, 17, 65, 249, 65, 17, 31, 0 } },
    {     0x2398,                   { 63, 33, 9, 125, 9, 33, 63, 0 } },
    {     0x2399,                   { 0, 28, 54, 93, 65, 65, 62, 0 } },
    {     0x239a,                    { 0, 0, 62, 65, 65, 65, 62, 0 } },
    {     0x23b7,                    { 1, 2, 68, 72, 80, 96, 64, 0 } }, // radical symbol bottom
    {     0x23cd,                 { 8, 107, 73, 65, 65, 65, 127, 0 } },
    {     0x2422,                  { 32, 56, 96, 44, 50, 50, 44, 0 } },
    {     0x2423,                      { 0, 0, 0, 0, 0, 66, 126, 0 } },
    {     0x2440,                   { 14, 10, 10, 8, 40, 40, 56, 0 } },
    {     0x2441,                     { 2, 2, 2, 62, 34, 34, 34, 0 } },
    {     0x2442,                     { 34, 34, 34, 62, 8, 8, 8, 0 } },
    {     0x2443,                     { 8, 8, 8, 62, 34, 34, 34, 0 } },
    {     0x2444,                   { 62, 42, 42, 8, 42, 42, 62, 0 } },
    {     0x2445,                    { 0, 34, 54, 42, 54, 34, 0, 0 } },
    {     0x2446,                   { 0, 6, 102, 96, 96, 102, 6, 0 } },
    {     0x2447,                   { 0, 3, 3, 11, 104, 104, 96, 0 } },
    {     0x2448,                     { 0, 6, 86, 80, 80, 80, 0, 0 } },
    {     0x2449,                     { 0, 0, 0, 109, 109, 0, 0, 0 } },
    {     0x244a,                   { 0, 160, 80, 40, 20, 10, 5, 0 } },
    {     0x2460,                  { 48, 48, 16, 16, 62, 62, 62, 0 } },
    {     0x2461,                    { 62, 2, 2, 62, 32, 32, 62, 0 } },
    {     0x2462,                   { 124, 4, 4, 126, 6, 6, 126, 0 } },
    {     0x2463,                  { 96, 96, 96, 102, 126, 6, 6, 0 } },
    {     0x2464,                    { 62, 32, 32, 62, 2, 2, 62, 0 } },
    {     0x2465,               { 124, 68, 64, 64, 127, 65, 127, 0 } },
    {     0x2466,                      { 62, 34, 34, 4, 8, 8, 8, 0 } },
    {     0x2467,                { 62, 34, 34, 127, 99, 99, 127, 0 } },
    {     0x2468,                   { 126, 66, 66, 126, 6, 6, 6, 0 } },
    {     0x24ea,                  { 62, 65, 65, 65, 65, 65, 62, 0 } },
    // Drawing & Gemetric
    {    0x02500, { 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00 } },
    {    0x02501, { 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00 } }, // $40 PETSCII box drawings light horizontal
    {    0x02502, { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 } }, // $5D PETSCII box drawings light vertical
    {     0x250C, { 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x18, 0x18, 0x18 } }, // $70 PETSCII box drawings light down and right
    {     0x2510, { 0x00, 0x00, 0x00, 0xf8, 0xf8, 0x18, 0x18, 0x18 } }, // $6E PETSCII box drawings light down and left
    {     0x2514, { 0x18, 0x18, 0x18, 0x1f, 0x1f, 0x00, 0x00, 0x00 } }, // $6D PETSCII box drawings light up and right
    {     0x2518, { 0x18, 0x18, 0x18, 0xf8, 0xf8, 0x00, 0x00, 0x00 } }, // $7D PETSCII box drawings light up and left
    {     0x251C, { 0x18, 0x18, 0x18, 0x1f, 0x1f, 0x18, 0x18, 0x18 } }, // $6B PETSCII box drawings light vertical and right
    {     0x2524, { 0x18, 0x18, 0x18, 0xf8, 0xf8, 0x18, 0x18, 0x18 } }, // $73 PETSCII box drawings light vertical and left
    {     0x252C, { 0x00, 0x00, 0x00, 0xff, 0xff, 0x18, 0x18, 0x18 } }, // $72 PETSCII box drawings light down and horizontal
    {     0x2534, { 0x18, 0x18, 0x18, 0xff, 0xff, 0x00, 0x00, 0x00 } }, // $71 PETSCII box drawings light up and horizontal
    {    0x0253C, { 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18 } }, // $5B PETSCII box drawings light vertical and horizontal
    {    0x0256D, { 0x00, 0x00, 0x00, 0x07, 0x0f, 0x1c, 0x18, 0x18 } }, // $55 PETSCII box drawings light arc down and right
    {    0x0256E, { 0x00, 0x00, 0x00, 0xe0, 0xf0, 0x38, 0x18, 0x18 } }, // $49 PETSCII box drawings light arc down and left
    {    0x0256F, { 0x18, 0x18, 0x38, 0xf0, 0xe0, 0x00, 0x00, 0x00 } }, // $4B PETSCII box drawings light arc up and left
    {    0x02570, { 0x18, 0x18, 0x1c, 0x0f, 0x07, 0x00, 0x00, 0x00 } }, // $4A PETSCII box drawings light arc up and right
    {    0x02571, { 0x03, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xc0 } }, // $4E PETSCII box drawings light diagonal upper right to lower left
    {    0x02572, { 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0x03 } }, // $4D PETSCII box drawings light diagonal upper left to lower right
    {    0x02573, { 0xc3, 0xe7, 0x7e, 0x3c, 0x3c, 0x7e, 0xe7, 0xc3 } }, // $56 PETSCII box drawings light diagonal cross
    {     0x25a0,             { 0, 126, 126, 126, 126, 126, 126, 0 } },
    {     0x25a1,                 { 0, 126, 66, 66, 66, 66, 126, 0 } },
    {     0x25ca,                      { 0, 0, 8, 20, 34, 20, 8, 0 } },
    {     0x25CB, { 0x00, 0x3c, 0x7e, 0x66, 0x66, 0x7e, 0x3c, 0x00 } }, // $57 PETSCII donut
    {     0x25e2,                    { 0, 1, 3, 7, 15, 31, 63, 127 } },
    {     0x25e3,           { 0, 128, 192, 224, 240, 248, 252, 254 } },
    {     0x25e4,         { 255, 254, 252, 248, 240, 224, 192, 128 } },
    {     0x25e5,                  { 255, 127, 63, 31, 15, 7, 3, 1 } },
    {     0x2550,                     { 0, 0, 0, 255, 0, 255, 0, 0 } },
    {     0x2551,                 { 20, 20, 20, 20, 20, 20, 20, 20 } },
    {     0x2554,                    { 0, 0, 0, 31, 16, 23, 20, 20 } },
    {     0x2557,                   { 0, 0, 0, 252, 4, 244, 20, 20 } },
    {     0x255a,                   { 20, 20, 20, 23, 16, 31, 0, 0 } },
    {     0x255d,                  { 20, 20, 20, 244, 4, 252, 0, 0 } },
    {     0x2560,                 { 20, 20, 20, 23, 16, 23, 20, 20 } },
    {     0x2563,                { 20, 20, 20, 244, 4, 244, 20, 20 } },
    {     0x2566,                   { 0, 0, 0, 255, 0, 247, 20, 20 } },
    {     0x2569,                  { 20, 20, 20, 247, 0, 255, 0, 0 } },
    {     0x256c,                { 20, 20, 20, 247, 0, 247, 20, 20 } },
    {     0x2574,                       { 0, 0, 0, 0, 240, 0, 0, 0 } },
    {     0x2575,                         { 8, 8, 8, 8, 0, 0, 0, 0 } },
    {     0x2576,                        { 0, 0, 0, 0, 15, 0, 0, 0 } },
    {     0x2577,                         { 0, 0, 0, 0, 8, 8, 8, 8 } },
#if 0  // these are hand drawn in emoji.inc
      // Block elements
        { 0x2580, { 255, 255, 255, 255, 0, 0, 0, 0 } },
        { 0x2581, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff } }, // $64 PETSCII lower one eighth block
        { 0x2582, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff } }, // $6F PETSCII lower one quarter block
        { 0x2583, { 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff } }, // $79 PETSCII lower three eights block
        { 0x2584, { 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff } }, // $62 PETSCII lower half block
        { 0x2585, { 0, 0, 0, 255, 255, 255, 255, 255 } },
        { 0x2586, { 0, 0, 255, 255, 255, 255, 255, 255 } },
        { 0x2587, { 0, 255, 255, 255, 255, 255, 255, 255 } },
        { 0x2588, { 255, 255, 255, 255, 255, 255, 255, 255 } },
        { 0x2589, { 254, 254, 254, 254, 254, 254, 254, 254 } },
        { 0x258a, { 252, 252, 252, 252, 252, 252, 252, 252 } },
        { 0x258b, { 248, 248, 248, 248, 248, 248, 248, 248 } },
        { 0x258C, { 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0 } }, // $61 PETSCII left half block
        { 0x258D, { 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0 } }, // $75 PETSCII left three eights block
        { 0x258E, { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0 } }, // $74 PETSCII left one quarter block
        { 0x258F, { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0 } }, // $65 PETSCII left one eighth block
        { 0x2590, { 15, 15, 15, 15, 15, 15, 15, 15 } },
        { 0x2591, { 170, 0, 170, 0, 170, 0, 170, 0 } },
        { 0x2592, { 0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0x33, 0x33 } }, // $66 PETSCII medium shade
        { 0x2593, { 255, 170, 255, 170, 255, 170, 255, 170 } },
        { 0x2594, { 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // $63 PETSCII upper one eighth block
        { 0x2595, { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 } }, // $67 PETSCII right one eighth block
        { 0x2596, { 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0, 0xf0 } }, // $7B PETSCII black small square lower left
        { 0x2597, { 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f } }, // $6C PETSCII black small square lower right
        { 0x2598, { 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00 } }, // $7E PETSCII black small square upper left
        { 0x2599, { 240, 240, 240, 240, 255, 255, 255, 255 } },
        { 0x259a, { 240, 240, 240, 240, 15, 15, 15, 15 } },
        { 0x259b, { 255, 255, 255, 255, 240, 240, 240, 240 } },
        { 0x259c, { 255, 255, 255, 255, 15, 15, 15, 15 } },
        { 0x259D, { 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00 } }, // $7C PETSCII black small square upper right
        { 0x259e, { 15, 15, 15, 15, 240, 240, 240, 240 } },
        { 0x259f, { 15, 15, 15, 15, 255, 255, 255, 255 } },

        { 0x025CF, { 0x00, 0x3c, 0x7e, 0x7e, 0x7e, 0x7e, 0x3c, 0x00 } }, // $51 PETSCII black circle
        { 0x25e6, { 0, 60, 66, 66, 66, 66, 60, 0 } }, // white bullet
        // Misc.
        { 0x02660, { 0x08, 0x1c, 0x3e, 0x7f, 0x7f, 0x1c, 0x3e, 0x00 } }, // $41 PETSCII black spade suit
        // Misc.
        { 0x260e, { 0, 60, 195, 24, 36, 102, 126, 0 } },
        { 0x2661, { 54, 73, 65, 65, 34, 20, 8, 0 } },
        { 0x2662, { 8, 20, 34, 65, 34, 20, 8, 0 } },
        { 0x02663, { 0x18, 0x18, 0x66, 0x66, 0x18, 0x18, 0x3c, 0x00 } }, // $58 PETSCII black club suit
        { 0x2664, { 8, 20, 34, 65, 107, 20, 62, 0 } },
        { 0x02665, { 0x36, 0x7f, 0x7f, 0x7f, 0x3e, 0x1c, 0x08, 0x00 } }, // $53 PETSCII black heart suit
        { 0x02666, { 0x08, 0x1c, 0x3e, 0x7f, 0x3e, 0x1c, 0x08, 0x00 } }, // $5A PETSCII black diamond suit
        { 0x2667, { 8, 20, 42, 85, 42, 8, 8, 0 } },
        { 0x2985, { 18, 36, 72, 72, 72, 36, 18, 0 } },
        { 0x2986, { 72, 36, 18, 18, 18, 36, 72, 0 } },
        { 0x2610, { 127, 65, 65, 65, 65, 65, 127, 0 } },
        { 0x2611, { 127, 65, 67, 69, 105, 81, 127, 0 } },
        { 0x2612, { 127, 99, 85, 73, 85, 99, 127, 0 } },
        { 0x2706, { 14, 30, 48, 32, 48, 30, 46, 0 } },
        { 0x2707, { 28, 42, 73, 73, 85, 34, 28, 0 } },
        { 0x270e, { 62, 42, 42, 42, 62, 20, 8, 0 } },
        { 0x270f, { 0, 124, 70, 125, 70, 124, 0, 0 } },
        { 0x2756, { 8, 28, 42, 119, 42, 28, 8, 0 } },
        { 0x2766, { 127, 136, 54, 126, 126, 60, 8, 0 } },
#endif

    // CJK symbols
    {     0x3001,                       { 0, 0, 0, 0, 32, 16, 8, 0 } },
    {     0x3002,                     { 0, 0, 0, 48, 72, 72, 48, 0 } },
    {     0x3003,                      { 0, 18, 36, 72, 0, 0, 0, 0 } },
    {     0x3004,               { 52, 70, 141, 153, 161, 82, 52, 0 } },
    {     0x3005,                      { 0, 8, 31, 33, 74, 4, 2, 0 } },
    {     0x3006,                    { 0, 2, 36, 88, 84, 96, 64, 0 } },
    {     0x3007,                  { 28, 34, 65, 65, 65, 34, 28, 0 } },
    {     0x300a,                  { 10, 20, 40, 80, 40, 20, 10, 0 } },
    {     0x300b,                  { 80, 40, 20, 10, 20, 40, 80, 0 } },
    {     0x300c,                     { 30, 16, 16, 16, 0, 0, 0, 0 } },
    {     0x300d,                       { 0, 0, 0, 8, 8, 8, 120, 0 } },
    {     0x300e,                    { 62, 34, 46, 40, 56, 0, 0, 0 } },
    {     0x300f,                  { 0, 0, 28, 20, 116, 68, 124, 0 } },
    {     0x3010,                  { 28, 24, 16, 16, 16, 24, 28, 0 } },
    {     0x3011,                     { 56, 24, 8, 8, 8, 24, 56, 0 } },
    {     0x3012,                       { 62, 0, 62, 8, 8, 8, 0, 0 } },
    {     0x3013,                 { 0, 126, 126, 0, 0, 126, 126, 0 } },
    {     0x3014,                    { 8, 16, 16, 16, 16, 16, 8, 0 } },
    {     0x3015,                       { 16, 8, 8, 8, 8, 8, 16, 0 } },
    {     0x3016,                  { 62, 36, 40, 40, 40, 36, 62, 0 } },
    {     0x3017,                { 124, 36, 20, 20, 20, 36, 124, 0 } },
    {     0x3018,                  { 28, 40, 40, 40, 40, 40, 28, 0 } },
    {     0x3019,                  { 56, 20, 20, 20, 20, 20, 56, 0 } },
    {     0x301f,                      { 0, 0, 0, 0, 0, 72, 36, 18 } },
    {     0x3020,                  { 127, 0, 127, 42, 8, 34, 28, 0 } },
    {     0x3021,                         { 8, 8, 8, 8, 8, 8, 8, 0 } },
    {     0x3022,                    { 4, 36, 36, 36, 36, 36, 4, 0 } },
    {     0x3023,                   { 65, 73, 73, 73, 73, 65, 1, 0 } },
    {     0x3024,                     { 2, 34, 20, 8, 20, 34, 2, 0 } },
    {     0x3025,                  { 34, 36, 28, 34, 34, 34, 28, 0 } },
    {     0x3026,                       { 8, 127, 0, 0, 0, 0, 0, 0 } },
    {     0x3027,                      { 8, 127, 0, 62, 0, 0, 0, 0 } },
    {     0x3028,                    { 8, 127, 0, 62, 0, 127, 0, 0 } },
    {     0x3029,                    { 16, 62, 84, 8, 20, 34, 2, 0 } },
    {     0x302a,                    { 0, 0, 0, 0, 112, 80, 112, 0 } },
    {     0x302b,                    { 0, 112, 80, 112, 0, 0, 0, 0 } },
    {     0x302c,                      { 0, 14, 10, 14, 0, 0, 0, 0 } },
    {     0x302d,                      { 0, 0, 0, 0, 14, 10, 14, 0 } },
    {     0x302e,                       { 0, 0, 0, 96, 96, 0, 0, 0 } },
    {     0x302f,                     { 0, 96, 96, 0, 96, 96, 0, 0 } },
    {     0x3030,                     { 0, 0, 0, 153, 102, 0, 0, 0 } },
    {     0x3031,                      { 8, 8, 16, 32, 16, 8, 4, 0 } },
    {     0x3032,                   { 16, 17, 37, 68, 32, 16, 8, 0 } },
    {     0x3033,                     { 2, 4, 8, 16, 32, 64, 64, 0 } },
    {     0x3034,                     { 2, 4, 8, 22, 32, 70, 64, 0 } },
    {     0x3035,                      { 64, 32, 16, 8, 4, 2, 2, 0 } },
    {     0x3037,                   { 0, 85, 85, 34, 51, 85, 85, 0 } },
    {     0x3038,                       { 8, 8, 8, 127, 8, 8, 8, 0 } },
    {     0x3039,                   { 4, 36, 36, 127, 36, 36, 4, 0 } },
    {     0x303a,                   { 2, 34, 42, 127, 42, 34, 2, 0 } },
    {     0x303b,                  { 32, 24, 12, 48, 64, 64, 62, 0 } },
    {     0x303c,                 { 0, 126, 70, 74, 82, 98, 126, 0 } },
    {     0x303d,                       { 40, 84, 4, 4, 4, 4, 2, 0 } },
    {     0x303e,                { 127, 91, 69, 73, 93, 73, 127, 0 } },
    {     0x303f,                { 127, 99, 85, 73, 85, 99, 127, 0 } },
    // Hiragana
    {     0x3041,                 { 0, 16, 126, 16, 62, 85, 121, 6 } },
    {     0x3042,                 { 16, 126, 16, 62, 85, 89, 50, 4 } },
    {     0x3043,                    { 0, 0, 64, 68, 66, 82, 32, 0 } },
    {     0x3044,                   { 0, 64, 72, 68, 66, 82, 32, 0 } },
    {     0x3045,                     { 0, 60, 0, 124, 2, 2, 28, 0 } },
    {     0x3046,                     { 60, 0, 124, 2, 2, 4, 24, 0 } },
    {     0x3047,                   { 0, 56, 0, 124, 24, 40, 70, 0 } },
    {     0x3048,                   { 56, 0, 124, 8, 24, 36, 71, 0 } },
    {     0x3049,                   { 0, 18, 57, 16, 62, 81, 51, 0 } },
    {     0x304a,                  { 18, 57, 16, 30, 49, 81, 50, 0 } },
    {     0x304b,                 { 32, 34, 121, 36, 36, 68, 76, 0 } },
    {     0x304d,                 { 32, 124, 16, 62, 4, 60, 64, 60 } },
    {     0x304f,                      { 4, 8, 16, 32, 16, 8, 4, 0 } },
    {     0x3051,                   { 4, 68, 94, 68, 100, 36, 8, 0 } },
    {     0x3053,                    { 0, 124, 8, 0, 64, 64, 62, 0 } },
    {     0x3055,                   { 16, 62, 4, 62, 64, 64, 60, 0 } },
    {     0x3057,                  { 32, 32, 32, 32, 32, 34, 28, 0 } },
    {     0x3059,                   { 8, 126, 8, 24, 40, 24, 48, 0 } },
    {     0x305b,                  { 4, 36, 126, 36, 44, 32, 28, 0 } },
    {     0x305d,                   { 60, 8, 126, 8, 16, 16, 12, 0 } },
    {     0x305f,                 { 32, 120, 32, 46, 32, 40, 71, 0 } },
    {     0x3061,                  { 32, 32, 120, 32, 60, 2, 2, 28 } },
    {     0x3063,                      { 0, 0, 124, 2, 2, 4, 56, 0 } },
    {     0x3064,                      { 0, 124, 2, 2, 2, 4, 56, 0 } },
    {     0x3066,                    { 0, 126, 4, 8, 16, 16, 12, 0 } },
    {     0x3068,                   { 0, 16, 16, 30, 96, 64, 62, 0 } },
    {     0x306a,                 { 32, 114, 37, 36, 76, 22, 24, 0 } },
    {     0x306b,                  { 64, 94, 64, 64, 80, 78, 64, 0 } },
    {     0x306c,                   { 8, 92, 42, 89, 83, 85, 34, 0 } },
    {     0x306d,                 { 32, 108, 50, 98, 38, 43, 36, 0 } },
    {     0x306e,                   { 0, 60, 74, 74, 82, 82, 36, 0 } },
    {     0x306f,                  { 68, 94, 68, 68, 76, 86, 72, 0 } },
    {     0x3072,                  { 96, 36, 70, 68, 68, 68, 56, 0 } },
    {     0x3075,                  { 48, 12, 16, 16, 74, 69, 24, 0 } },
    {     0x3078,                       { 0, 0, 0, 48, 72, 4, 2, 0 } },
    {     0x307b,                  { 94, 68, 94, 68, 76, 86, 72, 0 } },
    {     0x307e,                    { 8, 62, 8, 62, 40, 94, 48, 0 } },
    {     0x307f,                 { 112, 18, 18, 63, 82, 82, 36, 0 } },
    {     0x3080,                 { 32, 112, 36, 34, 80, 34, 60, 0 } },
    {     0x3081,                   { 4, 92, 42, 89, 73, 81, 38, 0 } },
    {     0x3082,                { 16, 124, 16, 124, 16, 18, 12, 0 } },
    {     0x3083,                   { 0, 72, 126, 33, 22, 16, 8, 0 } },
    {     0x3084,                   { 72, 126, 33, 22, 16, 8, 8, 0 } },
    {     0x3085,                   { 0, 8, 92, 106, 106, 92, 8, 0 } },
    {     0x3086,                   { 8, 92, 106, 73, 74, 92, 8, 0 } },
    {     0x3087,                     { 0, 8, 14, 8, 56, 78, 48, 0 } },
    {     0x3088,                     { 8, 14, 8, 8, 56, 78, 48, 0 } },
    {     0x3089,                  { 48, 76, 64, 124, 66, 2, 28, 0 } },
    {     0x308a,                   { 64, 76, 82, 98, 98, 66, 4, 0 } },
    {     0x308b,                  { 56, 16, 60, 66, 18, 42, 28, 0 } },
    {     0x308c,                 { 32, 96, 44, 50, 34, 98, 35, 32 } },
    {     0x308d,                    { 56, 16, 60, 66, 2, 2, 28, 0 } },
    {     0x308e,                   { 0, 32, 96, 60, 98, 34, 44, 0 } },
    {     0x308f,                 { 32, 112, 44, 50, 98, 34, 44, 0 } },
    {     0x3090,                   { 60, 8, 30, 49, 81, 87, 50, 0 } },
    {     0x3091,                  { 56, 16, 60, 70, 28, 52, 90, 0 } },
    {     0x3092,                  { 32, 112, 33, 62, 38, 40, 7, 0 } },
    {     0x3093,                 { 64, 64, 64, 64, 112, 73, 70, 0 } },
    {     0x3095,                  { 0, 32, 34, 121, 36, 68, 76, 0 } },
    {     0x3096,                   { 0, 68, 94, 68, 100, 36, 8, 0 } },
    {     0x3099,                      { 10, 10, 10, 0, 0, 0, 0, 0 } },
    {     0x309a,                      { 14, 10, 14, 0, 0, 0, 0, 0 } },
    {     0x309b,                      { 80, 80, 80, 0, 0, 0, 0, 0 } },
    {     0x309c,                    { 112, 80, 112, 0, 0, 0, 0, 0 } },
    {     0x309d,                      { 0, 32, 16, 8, 4, 24, 0, 0 } },
    {     0x309e,                      { 2, 37, 18, 8, 4, 24, 0, 0 } },
    {     0x309f,                 { 64, 124, 64, 92, 98, 66, 92, 0 } },
    // Katakana
    {     0x30a2,                  { 126, 2, 20, 24, 16, 16, 32, 0 } },
    {     0x30a4,                     { 2, 12, 24, 40, 72, 8, 8, 0 } },
    {     0x30a6,                     { 16, 126, 66, 2, 4, 8, 0, 0 } },
    {     0x30a8,                       { 0, 62, 8, 8, 8, 62, 0, 0 } },
    {     0x30aa,                    { 8, 126, 8, 24, 40, 72, 8, 0 } },
    {     0x30ab,                  { 16, 126, 18, 18, 34, 70, 0, 0 } },
    {     0x30ad,                       { 8, 62, 8, 62, 8, 8, 8, 0 } },
    {     0x30af,                    { 30, 34, 66, 4, 8, 16, 32, 0 } },
    {     0x30b1,                    { 32, 62, 72, 8, 8, 16, 32, 0 } },
    {     0x30b3,                     { 0, 126, 2, 2, 2, 126, 0, 0 } },
    {     0x30b5,                    { 36, 126, 36, 36, 4, 8, 0, 0 } },
    {     0x30b7,                     { 96, 0, 98, 2, 4, 120, 0, 0 } },
    {     0x30b9,                    { 126, 4, 8, 24, 36, 66, 0, 0 } },
    {     0x30bb,                  { 32, 126, 34, 36, 32, 30, 0, 0 } },
    {     0x30bd,                      { 66, 34, 2, 4, 8, 16, 0, 0 } },
    {     0x30bf,                   { 30, 34, 82, 12, 8, 16, 32, 0 } },
    {     0x30c1,                     { 2, 60, 8, 126, 8, 8, 16, 0 } },
    {     0x30c4,                      { 0, 82, 82, 2, 2, 4, 24, 0 } },
    {     0x30c6,                     { 60, 0, 126, 8, 8, 8, 16, 0 } },
    {     0x30c8,                  { 16, 16, 16, 24, 20, 16, 16, 0 } },
    {     0x30ca,                     { 8, 8, 126, 8, 8, 16, 32, 0 } },
    {     0x30cb,                      { 0, 60, 0, 0, 0, 126, 0, 0 } },
    {     0x30cc,                     { 62, 2, 20, 8, 20, 32, 0, 0 } },
    {     0x30cd,                      { 8, 62, 4, 8, 28, 42, 8, 0 } },
    {     0x30ce,                       { 4, 4, 4, 8, 16, 32, 0, 0 } },
    {     0x30cf,                    { 8, 36, 34, 34, 34, 66, 0, 0 } },
    {     0x30d2,                  { 64, 64, 124, 64, 64, 62, 0, 0 } },
    {     0x30d5,                       { 62, 2, 2, 4, 8, 16, 0, 0 } },
    {     0x30d8,                      { 0, 16, 40, 68, 2, 2, 0, 0 } },
    {     0x30db,                    { 8, 126, 8, 42, 42, 74, 8, 0 } },
    {     0x30de,                     { 126, 2, 36, 24, 8, 4, 0, 0 } },
    {     0x30df,                    { 24, 4, 18, 8, 36, 16, 12, 0 } },
    {     0x30e0,                    { 8, 16, 36, 68, 124, 2, 0, 0 } },
    {     0x30e1,                      { 2, 2, 20, 8, 20, 32, 0, 0 } },
    {     0x30e2,                  { 60, 16, 126, 16, 16, 14, 0, 0 } },
    {     0x30e4,                 { 16, 126, 18, 20, 16, 16, 16, 0 } },
    {     0x30e6,                      { 0, 60, 4, 4, 4, 126, 0, 0 } },
    {     0x30e8,                    { 0, 126, 2, 62, 2, 126, 0, 0 } },
    {     0x30e9,                      { 60, 0, 126, 2, 4, 8, 0, 0 } },
    {     0x30ea,                    { 34, 34, 34, 34, 34, 4, 8, 0 } },
    {     0x30eb,                   { 40, 40, 40, 42, 42, 76, 0, 0 } },
    {     0x30ec,                   { 32, 32, 32, 34, 36, 56, 0, 0 } },
    {     0x30ed,                  { 0, 126, 66, 66, 66, 126, 0, 0 } },
    {     0x30ef,                    { 126, 66, 66, 4, 8, 16, 0, 0 } },
    {     0x30f0,                    { 8, 126, 8, 40, 126, 8, 8, 0 } },
    {     0x30f1,                  { 126, 2, 20, 24, 16, 126, 0, 0 } },
    {     0x30f2,                     { 126, 2, 126, 2, 4, 8, 0, 0 } },
    {     0x30f3,                      { 96, 0, 2, 2, 4, 120, 0, 0 } },
    {     0x30fc,                       { 0, 0, 0, 126, 0, 0, 0, 0 } },
    {     0x30fd,                       { 0, 0, 32, 16, 8, 4, 0, 0 } },
    {     0x30ff,                       { 126, 2, 2, 2, 2, 2, 2, 0 } },
    // kanbun
    {     0x3190,                         { 8, 8, 8, 8, 8, 8, 8, 0 } },
    {     0x3191,                   { 0, 64, 64, 66, 68, 88, 96, 0 } },
    // CJK
    {     0x3190,                         { 0, 8, 8, 8, 8, 8, 8, 0 } },
    {     0x3191,                   { 0, 64, 64, 66, 68, 88, 96, 0 } },
    {     0x4e00,                       { 0, 0, 0, 126, 0, 0, 0, 0 } },
    {     0x4e01,                      { 0, 127, 8, 8, 8, 8, 24, 0 } },
    {     0x4e03,                  { 0, 32, 32, 126, 32, 32, 30, 0 } },
    {     0x4e07,                  { 0, 127, 16, 30, 18, 34, 70, 0 } },
    {     0x4e09,                    { 0, 126, 0, 60, 0, 126, 0, 0 } },
    {     0x4e0a,                      { 0, 8, 8, 14, 8, 8, 127, 0 } },
    {     0x4e0b,                     { 0, 127, 8, 12, 10, 8, 8, 0 } },
    {     0x4e19,                  { 0, 127, 8, 127, 85, 99, 65, 0 } },
    {     0x4e2d,                     { 8, 62, 42, 42, 62, 8, 8, 0 } },
    {     0x4e59,                   { 0, 124, 8, 16, 32, 66, 62, 0 } },
    {     0x4e5d,                  { 0, 32, 32, 124, 36, 36, 70, 0 } },
    {     0x4e8c,                      { 0, 0, 60, 0, 0, 126, 0, 0 } },
    {     0x4e94,                 { 0, 126, 16, 62, 18, 34, 127, 0 } },
    {     0x4eba,                      { 0, 8, 8, 8, 20, 34, 65, 0 } },
    {     0x516b,                    { 0, 16, 8, 36, 36, 36, 66, 0 } },
    {     0x516d,                    { 0, 8, 127, 0, 20, 34, 65, 0 } },
    {     0x5341,                       { 0, 8, 8, 127, 8, 8, 8, 0 } },
    {     0x5343,                      { 6, 56, 8, 127, 8, 8, 8, 0 } },
    {     0x56db,                { 0, 127, 85, 85, 103, 65, 127, 0 } },
    {     0x5730,                { 68, 84, 95, 85, 117, 144, 31, 0 } },
    {     0x5929,                     { 0, 62, 8, 62, 8, 20, 34, 0 } },
    {     0x5e74,                  { 64, 126, 8, 62, 40, 126, 8, 0 } },
    {     0x65e5,                  { 62, 34, 34, 62, 34, 34, 62, 0 } },
    {     0x6708,                  { 62, 34, 62, 34, 62, 34, 34, 0 } },
    {     0x70b9,                    { 8, 14, 8, 62, 34, 62, 85, 0 } },
    {     0x7532,                    { 62, 42, 62, 42, 62, 8, 8, 0 } },
    {     0x767e,                  { 127, 8, 62, 34, 62, 34, 62, 0 } },
    // private, PET screen codes not in Unicode
    {     0xe000,         { 128, 128, 128, 128, 128, 128, 128, 255 } }, //  CC one eighth block lower and left
    {     0xe001,                       { 1, 1, 1, 1, 1, 1, 1, 255 } }, //  BA one eighth block lower and right
    {     0xe002,         { 255, 128, 128, 128, 128, 128, 128, 128 } }, //  CF one eighth block upper and left
    {     0xe003,                       { 255, 1, 1, 1, 1, 1, 1, 1 } }, //  D0 one eighth block upper and right
    {     0xe004,                       { 0, 255, 0, 0, 0, 0, 0, 0 } }, //  C5 horizontal one eighth block-2
    {     0xe005,                       { 0, 0, 255, 0, 0, 0, 0, 0 } }, //  C4 horizontal one eighth block-3
    {     0xe006,                       { 0, 0, 0, 255, 0, 0, 0, 0 } }, //  C3 horizontal one eighth block-4
    {     0xe007,                       { 0, 0, 0, 0, 0, 255, 0, 0 } },
    {     0xe008,                       { 0, 0, 0, 0, 0, 0, 255, 0 } },
    {     0xe009,                 { 64, 64, 64, 64, 64, 64, 64, 64 } }, //-- vertical one eights bar-2
    {     0xe00a,                 { 32, 32, 32, 32, 32, 32, 32, 32 } }, //-- vertical one eights bar-3
    {     0xe00b,                 { 16, 16, 16, 16, 16, 16, 16, 16 } }, //-- vertical one eights bar-4
    {     0xe00c,                         { 4, 4, 4, 4, 4, 4, 4, 4 } }, //-- vertical one eights bar-5
    {     0xe00d,                         { 2, 2, 2, 2, 2, 2, 2, 2 } }, //-- vertical one eights bar-6
    {     0xe00e,               { 0, 60, 126, 126, 126, 126, 60, 0 } }, //  D1 BULLET/FILLED CIRCLE
    {     0xe00f,                   { 0, 60, 66, 66, 66, 66, 60, 0 } }, //  D7 white circle/donut
    {     0xe010,             { 170, 85, 170, 85, 170, 85, 170, 85 } }, //  A6 medium shade
    {     0xe011,             { 160, 80, 160, 80, 160, 80, 160, 80 } }, //  DC/7c left half medium shade
    {     0xe012,                   { 0, 0, 0, 0, 170, 85, 170, 85 } }, //  A8 lower half medium shade
    {     0xe013,             { 204, 204, 51, 51, 204, 204, 51, 51 } }, //-- medium shade, coarse
    {     0xe014,           { 204, 102, 51, 153, 204, 102, 51, 153 } }, //  DF upper left to lower right fill
    {     0xe015,           { 153, 51, 102, 204, 153, 51, 102, 204 } }, //  E9 upper right to lower left fill
    {     0xe016,                     { 255, 255, 0, 0, 0, 0, 0, 0 } }, //  B7 upper one quarter block
    {     0xe017,                   { 255, 255, 255, 0, 0, 0, 0, 0 } }, //  B8 upper three eights block
    {     0xe018,                         { 3, 3, 3, 3, 3, 3, 3, 3 } }, //  AA right one quarter block
    {     0xe019,                         { 7, 7, 7, 7, 7, 7, 7, 7 } }, //  B6 right three eighths block
    // original pet forms
    {     0xe035,                  { 126, 64, 120, 4, 2, 68, 56, 0 } }, //  5
    {     0xe037,                   { 126, 66, 4, 8, 16, 16, 16, 0 } }, //  7
    {     0xe070,                   { 0, 0, 92, 98, 98, 92, 64, 64 } }, //  p
    {     0xe071,                     { 0, 0, 58, 70, 70, 58, 2, 2 } }, //  q
    // MZ80-like
    {     0xe100,                 { 24, 36, 126, 255, 90, 36, 0, 0 } },
    {     0xe101,                    { 28, 28, 8, 62, 8, 20, 34, 0 } },
    {     0xe102,                   { 0, 9, 106, 124, 106, 9, 0, 0 } },
    {     0xe103,                    { 0, 72, 43, 31, 43, 72, 0, 0 } },
    {     0xe104,                    { 34, 20, 8, 62, 8, 28, 28, 0 } },
    {     0xe105,             { 60, 126, 219, 255, 231, 126, 60, 0 } },
    {     0xe106,               { 60, 66, 165, 129, 153, 66, 60, 0 } },
    {     0xe107,                 { 0, 192, 200, 84, 84, 85, 34, 0 } }, // snake
    {     0xe108,                { 65, 162, 60, 90, 126, 36, 54, 0 } }, // alien 1
    {     0xe109,             { 130, 70, 60, 90, 126, 255, 36, 108 } }, // alien 2
    // specials
    {     0xfffd,           { 127, 99, 93, 123, 119, 127, 119, 127 } },
    // arrows for legacy computing (as in "19025-terminals-prop.pdf")
    {    0x1f8b0,                    { 0, 120, 96, 80, 72, 8, 8, 0 } },
    {    0x1f8b1,                  { 0, 112, 8, 72, 80, 96, 120, 0 } },
    // graphics for legacy computing (as in "19025-terminals-prop.pdf")
    {    0x1fb00,                   { 240, 240, 240, 0, 0, 0, 0, 0 } },
    {    0x1fb01,                      { 15, 15, 15, 0, 0, 0, 0, 0 } },
    {    0x1fb02,                   { 255, 255, 255, 0, 0, 0, 0, 0 } },
    {    0x1fb03,                     { 0, 0, 0, 240, 240, 0, 0, 0 } },
    {    0x1fb04,               { 240, 240, 240, 240, 240, 0, 0, 0 } },
    {    0x1fb05,                  { 15, 15, 15, 240, 240, 0, 0, 0 } },
    {    0x1fb06,               { 255, 255, 255, 240, 240, 0, 0, 0 } },
    {    0x1fb07,                       { 0, 0, 0, 15, 15, 0, 0, 0 } },
    {    0x1fb08,                 { 240, 240, 240, 15, 15, 0, 0, 0 } },
    {    0x1fb09,                    { 15, 15, 15, 15, 15, 0, 0, 0 } },
    {    0x1fb0a,                 { 255, 255, 255, 15, 15, 0, 0, 0 } },
    {    0x1fb0b,                     { 0, 0, 0, 255, 255, 0, 0, 0 } },
    {    0x1fb0c,               { 240, 240, 240, 255, 255, 0, 0, 0 } },
    {    0x1fb0d,                  { 15, 15, 15, 255, 255, 0, 0, 0 } },
    {    0x1fb0e,               { 255, 255, 255, 255, 255, 0, 0, 0 } },
    {    0x1fb0f,                   { 0, 0, 0, 0, 0, 240, 240, 240 } },
    {    0x1fb10,             { 240, 240, 240, 0, 0, 240, 240, 240 } },
    {    0x1fb11,                { 15, 15, 15, 0, 0, 240, 240, 240 } },
    {    0x1fb12,             { 255, 255, 255, 0, 0, 240, 240, 240 } },
    {    0x1fb13,               { 0, 0, 0, 240, 240, 240, 240, 240 } },
    {    0x1fb14,            { 15, 15, 15, 240, 240, 240, 240, 240 } },
    {    0x1fb15,         { 255, 255, 255, 240, 240, 240, 240, 240 } },
    {    0x1fb16,                 { 0, 0, 0, 15, 15, 240, 240, 240 } },
    {    0x1fb17,           { 240, 240, 240, 15, 15, 240, 240, 240 } },
    {    0x1fb18,              { 15, 15, 15, 15, 15, 240, 240, 240 } },
    {    0x1fb19,           { 255, 255, 255, 15, 15, 240, 240, 240 } },
    {    0x1fb1a,               { 0, 0, 0, 255, 255, 240, 240, 240 } },
    {    0x1fb1b,         { 240, 240, 240, 255, 255, 240, 240, 240 } },
    {    0x1fb1c,            { 15, 15, 15, 255, 255, 240, 240, 240 } },
    {    0x1fb1d,         { 255, 255, 255, 255, 255, 240, 240, 240 } },
    {    0x1fb1e,                      { 0, 0, 0, 0, 0, 15, 15, 15 } },
    {    0x1fb1f,                { 240, 240, 240, 0, 0, 15, 15, 15 } },
    {    0x1fb20,                   { 15, 15, 15, 0, 0, 15, 15, 15 } },
    {    0x1fb21,                { 255, 255, 255, 0, 0, 15, 15, 15 } },
    {    0x1fb22,                  { 0, 0, 0, 240, 240, 15, 15, 15 } },
    {    0x1fb23,            { 240, 240, 240, 240, 240, 15, 15, 15 } },
    {    0x1fb24,               { 15, 15, 15, 240, 240, 15, 15, 15 } },
    {    0x1fb25,            { 255, 255, 255, 240, 240, 15, 15, 15 } },
    {    0x1fb26,                    { 0, 0, 0, 15, 15, 15, 15, 15 } },
    {    0x1fb27,              { 240, 240, 240, 15, 15, 15, 15, 15 } },
    {    0x1fb28,              { 255, 255, 255, 15, 15, 15, 15, 15 } },
    {    0x1fb29,                  { 0, 0, 0, 255, 255, 15, 15, 15 } },
    {    0x1fb2a,            { 240, 240, 240, 255, 255, 15, 15, 15 } },
    {    0x1fb2b,               { 15, 15, 15, 255, 255, 15, 15, 15 } },
    {    0x1fb2c,            { 255, 255, 255, 255, 255, 15, 15, 15 } },
    {    0x1fb2d,                   { 0, 0, 0, 0, 0, 255, 255, 255 } },
    {    0x1fb2e,             { 240, 240, 240, 0, 0, 255, 255, 255 } },
    {    0x1fb2f,                { 15, 15, 15, 0, 0, 255, 255, 255 } },
    {    0x1fb30,             { 255, 255, 255, 0, 0, 255, 255, 255 } },
    {    0x1fb31,               { 0, 0, 0, 240, 240, 255, 255, 255 } },
    {    0x1fb32,         { 240, 240, 240, 240, 240, 255, 255, 255 } },
    {    0x1fb33,            { 15, 15, 15, 240, 240, 255, 255, 255 } },
    {    0x1fb34,         { 255, 255, 255, 240, 240, 255, 255, 255 } },
    {    0x1fb35,                 { 0, 0, 0, 15, 15, 255, 255, 255 } },
    {    0x1fb36,           { 240, 240, 240, 15, 15, 255, 255, 255 } },
    {    0x1fb37,              { 15, 15, 15, 15, 15, 255, 255, 255 } },
    {    0x1fb38,           { 255, 255, 255, 15, 15, 255, 255, 255 } },
    {    0x1fb39,               { 0, 0, 0, 255, 255, 255, 255, 255 } },
    {    0x1fb3a,         { 240, 240, 240, 255, 255, 255, 255, 255 } },
    {    0x1fb3b,            { 15, 15, 15, 255, 255, 255, 255, 255 } },
    {    0x1fb3c,                   { 0, 0, 0, 0, 0, 128, 224, 240 } },
    {    0x1fb3d,                   { 0, 0, 0, 0, 0, 192, 248, 255 } },
    {    0x1fb3e,               { 0, 0, 0, 128, 192, 224, 224, 240 } },
    {    0x1fb3f,               { 0, 0, 0, 128, 224, 248, 254, 255 } },
    {    0x1fb40,         { 128, 128, 192, 192, 224, 224, 240, 240 } },
    {    0x1fb41,           { 15, 31, 127, 255, 255, 255, 255, 255 } },
    {    0x1fb42,              { 0, 7, 63, 255, 255, 255, 255, 255 } },
    {    0x1fb43,             { 15, 15, 31, 63, 127, 255, 255, 255 } },
    {    0x1fb44,                { 0, 1, 7, 31, 127, 255, 255, 255 } },
    {    0x1fb45,               { 15, 15, 31, 31, 63, 63, 127, 127 } },
    {    0x1fb46,                 { 0, 0, 3, 15, 63, 255, 255, 255 } },
    {    0x1fb47,                        { 0, 0, 0, 0, 0, 1, 7, 15 } },
    {    0x1fb48,                      { 0, 0, 0, 0, 0, 3, 31, 255 } },
    {    0x1fb49,                        { 0, 0, 0, 1, 3, 7, 7, 31 } },
    {    0x1fb4a,                    { 0, 0, 0, 1, 7, 31, 127, 255 } },
    {    0x1fb4b,                       { 1, 1, 3, 3, 7, 7, 15, 15 } },
    {    0x1fb4c,         { 240, 248, 254, 255, 255, 255, 255, 255 } },
    {    0x1fb4d,           { 0, 224, 252, 255, 255, 255, 255, 255 } },
    {    0x1fb4e,         { 240, 240, 248, 252, 254, 255, 255, 255 } },
    {    0x1fb4f,           { 0, 128, 224, 248, 254, 255, 255, 255 } },
    {    0x1fb50,         { 240, 240, 248, 248, 252, 252, 254, 254 } },
    {    0x1fb51,             { 0, 0, 192, 240, 252, 255, 255, 255 } },
    {    0x1fb52,           { 255, 255, 255, 255, 255, 127, 31, 15 } },
    {    0x1fb53,              { 255, 255, 255, 255, 255, 63, 7, 0 } },
    {    0x1fb54,             { 255, 255, 255, 127, 63, 31, 15, 15 } },
    {    0x1fb55,                { 255, 255, 255, 127, 31, 7, 1, 0 } },
    {    0x1fb56,               { 127, 127, 63, 63, 31, 31, 15, 15 } },
    {    0x1fb57,                   { 240, 224, 128, 0, 0, 0, 0, 0 } },
    {    0x1fb58,                   { 255, 248, 192, 0, 0, 0, 0, 0 } },
    {    0x1fb59,               { 240, 224, 224, 192, 128, 0, 0, 0 } },
    {    0x1fb5a,               { 255, 254, 248, 224, 128, 0, 0, 0 } },
    {    0x1fb5b,         { 240, 240, 224, 224, 192, 192, 128, 128 } },
    {    0x1fb5c,               { 255, 255, 252, 240, 192, 0, 0, 0 } },
    {    0x1fb5d,         { 255, 255, 255, 255, 255, 254, 248, 240 } },
    {    0x1fb5e,           { 255, 255, 255, 255, 255, 252, 224, 0 } },
    {    0x1fb5f,         { 255, 255, 255, 254, 252, 248, 240, 240 } },
    {    0x1fb60,           { 255, 255, 255, 254, 248, 224, 128, 0 } },
    {    0x1fb61,         { 254, 254, 252, 252, 248, 248, 240, 240 } },
    {    0x1fb62,                        { 15, 7, 1, 0, 0, 0, 0, 0 } },
    {    0x1fb63,                      { 255, 31, 3, 0, 0, 0, 0, 0 } },
    {    0x1fb64,                        { 31, 7, 7, 3, 1, 0, 0, 0 } },
    {    0x1fb65,                    { 255, 127, 31, 7, 1, 0, 0, 0 } },
    {    0x1fb66,                       { 15, 15, 7, 7, 3, 3, 1, 1 } },
    {    0x1fb67,                   { 255, 255, 63, 15, 3, 0, 0, 0 } },
    {    0x1fb68,               { 127, 63, 31, 15, 15, 31, 63, 127 } },
    {    0x1fb69,           { 0, 129, 195, 231, 255, 255, 255, 255 } },
    {    0x1fb6a,         { 254, 252, 248, 240, 240, 248, 252, 254 } },
    {    0x1fb6b,           { 255, 255, 255, 255, 231, 195, 129, 0 } },
    {    0x1fb6c,           { 128, 192, 224, 240, 224, 192, 128, 0 } },
    {    0x1fb6d,                     { 127, 62, 28, 8, 0, 0, 0, 0 } },
    {    0x1fb6e,                        { 1, 3, 7, 15, 7, 3, 1, 0 } },
    {    0x1fb6f,                     { 0, 0, 0, 0, 8, 28, 62, 127 } },

#if 0
        { 0x1FB70, { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60 } }, // $54 PETSCII box drawings light vertical two quarters left
        { 0x1FB72, { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 } }, // $42 PETSCII box drawings light vertical
        { 0x1fb73, { 8, 8, 8, 8, 8, 8, 8, 8 } },
        { 0x1FB75, { 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06 } }, // $59 PETSCII box drawings light vertical two quarters right
        { 0x1FB77, { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 } }, // $44 PETSCII box drawings light horizontal one quarter up
        { 0x1FB78, { 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00 } }, // $43 PETSCII box drawings light horizontal
        { 0x1fb79, { 0, 0, 0, 0, 255, 0, 0, 0 } },
        { 0x1FB7C, { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, 0xff } }, // $4C PETSCII bottom left corner
        { 0x1FB7D, { 0xff, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0 } }, // $4F PETSCII top left corner
        { 0x1FB7E, { 0xff, 0xff, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 } }, // $50 PETSCII top right corner
        { 0x1fb7f, { 1, 1, 1, 1, 1, 1, 1, 255 } },
        { 0x1fb80, { 255, 0, 0, 0, 0, 0, 0, 255 } },
#endif

    {    0x1fb81,                 { 255, 0, 255, 0, 255, 0, 0, 255 } },
    {    0x1FB82, { 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // $77 PETSCII upper one quarter block
    {    0x1FB83, { 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // $78 PETSCII upper three eights block
    {    0x1fb84,               { 255, 255, 255, 255, 255, 0, 0, 0 } },
    {    0x1fb85,             { 255, 255, 255, 255, 255, 255, 0, 0 } },
    {    0x1fb86,           { 255, 255, 255, 255, 255, 255, 255, 0 } },
    {    0x1FB87, { 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 } }, // $6A PETSCII right one quarter block
    {    0x1FB88, { 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07 } }, // $76 PETSCII right three eights block
    {    0x1fb89,                 { 31, 31, 31, 31, 31, 31, 31, 31 } },
    {    0x1fb8a,                 { 63, 63, 63, 63, 63, 63, 63, 63 } },
    {    0x1fb8b,         { 127, 127, 127, 127, 127, 127, 127, 127 } },
    {    0x1FB8C, { 0x50, 0xa0, 0x50, 0xa0, 0x50, 0xa0, 0x50, 0xa0 } }, // LEFT HALF MEDIUM SHADE
    {    0x1FB8D, { 0x05, 0x0a, 0x05, 0x0a, 0x05, 0x0a, 0x05, 0x0a } }, // RIGHT HALF MEDIUM SHADE
    {    0x1FB8E, { 0x55, 0xaa, 0x55, 0xaa, 0x00, 0x00, 0x00, 0x00 } }, // UPPER HALF MEDIUM SHADE
    {    0x1FB8F, { 0x00, 0x00, 0x00, 0x00, 0x55, 0xaa, 0x55, 0xaa } }, // LOWER HALF MEDIUM SHADE $68 PETSCII
    {    0x1FB90, { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 } }, // INVERSE MEDIUM SHADE
    {    0x1fb91,           { 255, 255, 255, 255, 85, 170, 85, 170 } },
    {    0x1fb92,           { 85, 170, 85, 170, 255, 255, 255, 255 } },
    {    0x1fb93,         { 245, 250, 245, 250, 245, 250, 245, 250 } },
    {    0x1fb94,             { 95, 175, 95, 175, 95, 175, 95, 175 } },
    {    0x1fb95,             { 204, 204, 51, 51, 204, 204, 51, 51 } },
    {    0x1fb96,             { 51, 51, 204, 204, 51, 51, 204, 204 } },
    {    0x1fb97,                 { 0, 0, 255, 255, 0, 0, 255, 255 } },
    {    0x1FB98, { 0x33, 0x99, 0xcc, 0x66, 0x33, 0x99, 0xcc, 0x66 } }, // UPPER LEFT TO LOWER RIGHT FILL '\\'
    {    0x1FB99, { 0xcc, 0x99, 0x33, 0x66, 0xcc, 0x99, 0x33, 0x66 } }, // UPPER RIGHT TO LOWER LEFT FILL '//'
    {    0x1fb9a,             { 255, 126, 60, 24, 24, 60, 126, 255 } },
    {    0x1fb9b,         { 129, 195, 231, 255, 255, 231, 195, 129 } },
    {    0x1fb9c,              { 170, 84, 168, 80, 160, 64, 128, 0 } },
    {    0x1fb9d,                   { 170, 85, 42, 21, 10, 5, 2, 1 } },
    {    0x1fb9e,                   { 1, 2, 5, 10, 21, 42, 85, 170 } },
    {    0x1fb9f,              { 0, 128, 64, 160, 80, 168, 84, 170 } },
    {    0x1fba0,                  { 24, 48, 96, 192, 128, 0, 0, 0 } },
    {    0x1fba1,                       { 24, 12, 6, 3, 1, 0, 0, 0 } },
    {    0x1fba2,                  { 0, 0, 0, 128, 192, 96, 48, 24 } },
    {    0x1fba3,                       { 0, 0, 0, 1, 3, 6, 12, 24 } },
    {    0x1fba4,               { 24, 48, 96, 192, 192, 96, 48, 24 } },
    {    0x1fba5,                     { 24, 12, 6, 3, 3, 6, 12, 24 } },
    {    0x1fba6,                 { 0, 0, 0, 129, 195, 102, 60, 24 } },
    {    0x1fba7,                 { 24, 60, 102, 195, 129, 0, 0, 0 } },
    {    0x1fba8,                { 24, 48, 96, 193, 131, 6, 12, 24 } },
    {    0x1fba9,                { 24, 12, 6, 131, 193, 96, 48, 24 } },
    {    0x1fbaa,               { 24, 12, 6, 131, 195, 102, 60, 24 } },
    {    0x1fbab,              { 24, 48, 96, 193, 195, 102, 60, 24 } },
    {    0x1fbac,               { 24, 60, 102, 195, 131, 6, 12, 24 } },
    {    0x1fbad,              { 24, 60, 102, 195, 193, 96, 48, 24 } },
    {    0x1fbae,             { 24, 60, 102, 195, 195, 102, 60, 24 } },
    {    0x1fbaf,                       { 8, 8, 8, 8, 255, 8, 8, 8 } },
    {    0x1fbb0,                { 0, 0, 64, 96, 112, 120, 108, 66 } },
    {    0x1fbb1,         { 255, 254, 253, 155, 215, 239, 239, 255 } },
    {    0x1fbb2,                   { 7, 3, 63, 70, 79, 6, 126, 32 } },
    {    0x1fbb3,                 { 0, 24, 224, 0, 224, 48, 16, 14 } },
    {    0x1fbb4,           { 255, 253, 221, 157, 1, 159, 223, 255 } },
    {    0x1fbb5,               { 200, 24, 56, 127, 56, 24, 8, 247 } },
    {    0x1fbb6,               { 19, 24, 28, 254, 28, 24, 16, 239 } },
    {    0x1fbb7,                     { 9, 9, 8, 127, 62, 28, 9, 1 } },
    {    0x1fbb8,                     { 1, 9, 28, 62, 127, 8, 9, 9 } },
    {    0x1fbb9,                  { 0, 62, 65, 64, 64, 64, 127, 0 } },
    {    0x1fbba,                     { 0, 0, 252, 2, 2, 2, 254, 0 } },
    {    0x1fbbb,                 { 20, 20, 119, 0, 119, 20, 20, 0 } },
    {    0x1fbbc,                   { 255, 1, 1, 25, 25, 1, 1, 255 } },
    {    0x1fbbd,             { 60, 153, 195, 231, 195, 153, 60, 0 } },
    {    0x1fbbe,           { 255, 255, 254, 252, 249, 243, 231, 0 } },
    {    0x1fbbf,            { 231, 195, 153, 60, 153, 195, 231, 0 } },
    {    0x1fbc0,           { 102, 153, 129, 66, 66, 129, 153, 102 } },
    {    0x1fbc1,               { 95, 96, 99, 98, 100, 120, 96, 95 } },
    {    0x1fbc2,                 { 255, 4, 3, 252, 2, 252, 4, 248 } },
    {    0x1fbc3,                     { 252, 2, 252, 0, 0, 0, 0, 0 } },
    {    0x1fbc4,           { 127, 99, 93, 115, 119, 127, 119, 127 } },
    {    0x1fbc5,                   { 28, 28, 8, 127, 8, 20, 34, 0 } },
    {    0x1fbc6,                   { 28, 28, 73, 62, 8, 20, 34, 0 } },
    {    0x1fbc7,                   { 28, 28, 9, 62, 72, 20, 34, 0 } },
    {    0x1fbc8,                   { 28, 28, 72, 62, 9, 20, 34, 0 } },
    {    0x1fbc9,                  { 28, 28, 8, 127, 20, 62, 20, 0 } },
    {    0x1fbca,                   { 8, 20, 34, 34, 42, 54, 34, 0 } },
    {    0x1fbf0,                   { 62, 99, 99, 0, 99, 99, 62, 0 } },
    {    0x1fbf1,                         { 3, 3, 3, 0, 3, 3, 3, 0 } },
    {    0x1fbf2,                    { 62, 3, 3, 62, 96, 96, 62, 0 } },
    {    0x1fbf3,                      { 62, 3, 3, 62, 3, 3, 62, 0 } },
    {    0x1fbf4,                     { 99, 99, 99, 62, 3, 3, 3, 0 } },
    {    0x1fbf5,                    { 62, 96, 96, 62, 3, 3, 62, 0 } },
    {    0x1fbf6,                  { 62, 96, 96, 62, 99, 99, 62, 0 } },
    {    0x1fbf7,                        { 62, 3, 3, 0, 3, 3, 3, 0 } },
    {    0x1fbf8,                  { 62, 99, 99, 62, 99, 99, 62, 0 } },
    {    0x1fbf9,                    { 62, 99, 99, 62, 3, 3, 62, 0 } },


#if defined(BA67_GRAPHICS_ENABLE_EMOJI)
    // emoji overwrites char8js
    #include "emoji.inc"

#endif


    // We use these for PETSCII - they break Unicode
    {    0x1FB70, { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60 } }, // $54 PETSCII box drawings light vertical two quarters left
    {    0x1FB71, { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 } }, // $47 PETSCII box drawings light vertical one quarter left
    {    0x1FB74, { 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c } }, // $48 PETSCII box drawings light vertical one quarter right
    {    0x1FB75, { 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06 } }, // $59 PETSCII box drawings light vertical two quarters right
    {    0x1FB76, { 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // $45 PETSCII box drawings light horizontal two quarters up (bad)
    {    0x1FB77, { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 } }, // $44 PETSCII box drawings light horizontal one quarter up
    {    0x1FB78, { 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00 } }, // $43 PETSCII box drawings light horizontal two quarter down
    {    0x1FB7A, { 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 } }, // $46 PETSCII box drawings light horizontal one quarter down
    {    0x1FB7B, { 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00 } }, // $52 PETSCII box drawings light horizontal two quarters down
    {    0x1FB8C, { 0x50, 0xa0, 0x50, 0xa0, 0x50, 0xa0, 0x50, 0xa0 } }, // LEFT HALF MEDIUM SHADE


    // NEVER TOUCH THIS LINE
    { 0xffffffff,                         { 0, 0, 0, 0, 0, 0, 0, 0 } }
};

FontDataBits::DataStruct* FontDataBits::getBits() {
    return &staticTable[0];
}
