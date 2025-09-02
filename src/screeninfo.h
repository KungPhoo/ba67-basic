#pragma once
// by default we have a screen 80x25 chars with 8x8 characters

// however, in 80x25 mode, when drawing the graphics to the window,
// the vertical scanlines are doubled to look like 8x16 characters.
// If you want true 8x16 characters, define it here, but then DEFCHAR etc.
// will behave differently.
struct ScreenInfo {
    static const size_t charsX = 80, charsY = 25;
    static const size_t charPixX = 8 /*don't change!*/, charPixY = 8 /*8 or 16*/;
    // static const size_t pixX = charsX * charPixX; // back buffer size
    // static const size_t pixY = charsY * charPixY;
};
