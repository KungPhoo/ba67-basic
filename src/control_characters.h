#pragma once
struct ControlCharacters {
    static const uint8_t cursorDown              = 0x11;
    static const uint8_t cursorRight             = 0x1d;
    static const uint8_t cursorUp                = 0x91;
    static const uint8_t cursorLeft              = 0x9d;
    static const uint8_t cursorHome              = 0x13;
    static const uint8_t backspaceChar           = 0x14;
    static const uint8_t clearScreen             = 0x93;
    static const uint8_t reverseModeOn           = 0x12;
    static const uint8_t reverseModeeOff         = 0x92;
    static const uint8_t textColor0_Black        = 0x90;
    static const uint8_t textColor1_White        = 0x05;
    static const uint8_t textColor2_Red          = 0x1c;
    static const uint8_t textColor3_Cyan         = 0x9f;
    static const uint8_t textColor4_Purple       = 0x9c;
    static const uint8_t textColor5_Green        = 0x1e;
    static const uint8_t textColor6_Blue         = 0x1f;
    static const uint8_t textColor7_Yellow       = 0x9e;
    static const uint8_t textColor8_Orange       = 0x81;
    static const uint8_t textColor9_Brown        = 0x95;
    static const uint8_t textColor10_Light_Red   = 0x96;
    static const uint8_t textColor11_Dark_Gray   = 0x97;
    static const uint8_t textColor12_Medium_Gray = 0x98;
    static const uint8_t textColor13_Light_Green = 0x99;
    static const uint8_t textColor14_Light_Blue  = 0x9a;
    static const uint8_t textColor15_Light_Gray  = 0x9b;

    static uint8_t charForColor(uint8_t color) {
        color = color & 0x0f;
        switch (color) {
        case 1:  return textColor1_White;
        case 2:  return textColor2_Red;
        case 3:  return textColor3_Cyan;
        case 4:  return textColor4_Purple;
        case 5:  return textColor5_Green;
        case 6:  return textColor6_Blue;
        case 7:  return textColor7_Yellow;
        case 8:  return textColor8_Orange;
        case 9:  return textColor9_Brown;
        case 10: return textColor10_Light_Red;
        case 11: return textColor11_Dark_Gray;
        case 12: return textColor12_Medium_Gray;
        case 13: return textColor13_Light_Green;
        case 14: return textColor14_Light_Blue;
        case 15: return textColor15_Light_Gray;
        }
        return textColor0_Black;
    }
};
