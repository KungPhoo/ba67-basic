#include "os_fpl.h"

// https://libfpl.org/docs/page_categories.html
#define FPL_IMPLEMENTATION
#include "final_platform_layer.h"
#include "basic.h"
#include <thread>
#include <filesystem>

#ifdef _WIN32
#include "../resources/resource.h"
#endif


static uint32_t emphasizeRGB(uint32_t color, float facR, float facG, float facB, float facDark) {
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t b = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t r = (color) & 0xFF;

    // Convert RGB to perceived brightness (luminance)
    float luminance = 0.2126f * r + 0.7152f * g + 0.0722f * b;

    const float brightness = 37.0f; // -200..200
    const float contrast = 1.5f;  // 0..1


    // https://ie.nitk.ac.in/blog/2020/01/19/algorithms-for-adjusting-brightness-and-contrast-of-an-image/
    auto truncate = [](float f) {if (f > 255.0f) { return 255.0f; } if (f < 0.0f) { return 0.0f; } return f; };
    float factor = (259.0f * (contrast + 255.0f)) / (255.0f * (259.0f - contrast));
    float new_r = truncate((((r - 128.0f) * contrast) + 128 + brightness) * facR * facDark);
    float new_g = truncate((((g - 128.0f) * contrast) + 128 + brightness) * facG * facDark);
    float new_b = truncate((((b - 128.0f) * contrast) + 128 + brightness) * facB * facDark);

    r = (uint8_t)new_r;
    g = (uint8_t)new_g;
    b = (uint8_t)new_b;

    return (a << 24) | (b << 16) | (g << 8) | r;
}



void displayUpdateThread(OsFPL* fpl) {
    bool dirty = true;
    std::array<std::array<uint32_t, 16>, 6> palettes; // [r,g,b, dark r,g,b]
    OsFPL::Buffered state;
    std::vector<uint8_t> pixelsPal; // what's actually drawn

    bool oldCursorVisible = false;
    uint64_t nextShowCursor = 0; // blink time
    constexpr size_t srcWidth = ScreenInfo::pixX;  // ScreenBitmap width (80x8)
    constexpr size_t srcHeight = ScreenInfo::pixY; // ScreenBitmap height (25x16)

    static int passes = 0;
    bool cursorVisible = true;
    for (;;) {
        // == UPDATE STATE ==
        fpl->screenLock.lock();
        if (fpl->buffered.stopThread) { fpl->screenLock.unlock(); return; }
        if (!dirty) { dirty = fpl->dirtyFlag; }
        if (dirty) {
            std::swap(state, fpl->buffered);
            fpl->dirtyFlag = false;
        }
        if (state.videoH * state.videoW != fpl->pixelsVideo.size()) {
            fpl->pixelsVideo.resize(state.videoH * state.videoW);
            dirty = true;
        }
        fpl->screenLock.unlock();


        // overwrite cursor box
        const uint64_t flashSpeed = 1250;
        uint64_t now = fplMillisecondsQuery();

        cursorVisible = (now % flashSpeed) > (flashSpeed / 2);
        if (oldCursorVisible != cursorVisible) {
            oldCursorVisible = cursorVisible;
            dirty = true;
        }

        // if (now > nextShowCursor) {
        //     nextShowCursor = now + flashSpeed;
        //     cursorVisible = !cursorVisible;
        //     dirty = true;
        // }

        if (!dirty) {
            fplThreadSleep(30);
            continue;
        }


        // make a deep copy of the pixel buffer - we optionally overwrite the blinking cursor
        pixelsPal = state.pixelsPal;

        if (pixelsPal.size() != state.pixelsPal.size() || state.isCursorActive) {

            auto crsr = state.crsrPos;
            if (cursorVisible
                && crsr.x < ScreenBuffer::width
                && crsr.y < ScreenBuffer::height) {
                bool draw = false;
                for (size_t y = 0; y < ScreenInfo::charPixY; ++y) {
                    auto* pdest = &pixelsPal[(y + crsr.y * ScreenInfo::charPixY) * srcWidth + crsr.x * ScreenInfo::charPixX];
                    for (size_t x = 0; x < 8; ++x) {
                        draw = !draw;
                        if (draw || y + 1 == ScreenInfo::charPixY) {
                            *pdest = state.crsrColor;
                        }
                        ++pdest;
                    }
                    draw = !draw;
                }
            }
        }

        if (state.videoH + state.videoW == 0) { continue; }
        if (pixelsPal.empty()) { continue; }


        // == DRAW ==

        // simulate a CRT TV with a 3x3 pixel matrix
        for (size_t p = 0; p < 6; ++p) {
            const float facDark = 0.7f;
            const float facNeigbour = 0.6f, facNeighbour2 = 0.6f;
            float r = 1.0f, g = 1.0f, b = 1.0f, darken = 1.0f;
            if (p == 0 || p == 3) { r = 1.0f;   g = facNeigbour; b = facNeighbour2; }
            if (p == 1 || p == 4) { r = facNeighbour2;   g = 1.0f;  b = facNeigbour; }
            if (p == 2 || p == 5) { r = facNeigbour; g = facNeighbour2; b = 1.0f; }
            //        if (p == 3 || p == 7) { r = g = b = 0.95; }
            if (p > 2) { darken = facDark; }
            for (size_t i = 0; i < 16; ++i) {
                palettes[p][i] = emphasizeRGB(state.palette[i], r, g, b, darken);
            }
        }

        // we don't access the RGB buffer - we use the colour indices
        // screen.updateScreenBitmap();

        // Compute scaling factors
        float scaleX = static_cast<float>(state.videoW) / srcWidth;
        float scaleY = static_cast<float>(state.videoH) / srcHeight;
        float scale = std::max(0.25f, std::min(scaleX, scaleY)); // Keep aspect ratio
        if (scale > 1.0f) {
            scale = floorf(scale); // scale to full pixels
        }

        // Compute offset for centered output
        size_t scaledWidth = static_cast<size_t>(srcWidth * scale);
        size_t scaledHeight = static_cast<size_t>(srcHeight * scale);
        size_t offsetX = (state.videoW - scaledWidth) / 2;
        size_t offsetY = (state.videoH - scaledHeight) / 2;

        // Nearest-neighbor scaling loop
        for (int y = 0; y < int(state.videoH); ++y) {

            fpl->screenLock.lock();
            uint32_t* pdest = &fpl->pixelsVideo[0] + y * state.videoW;
            fpl->screenLock.unlock();

            size_t srcY = std::min(static_cast<size_t>((y - offsetY) / scale), srcHeight - 1);
            for (size_t x = 0; x < state.videoW; ++x) {
                // Map screen coordinates to original ScreenBitmap using nearest-neighbor
                size_t srcX = std::min(static_cast<size_t>((x - offsetX) / scale), srcWidth - 1);

                // Default background if outside scaled region
                uint8_t color = state.borderColor; // Transparent or black

                // Only sample from ScreenBitmap if inside scaled bounds
                if (x >= offsetX && x < offsetX + scaledWidth &&
                    y >= offsetY && y < offsetY + scaledHeight) {
                    color = pixelsPal[srcY * srcWidth + srcX];
                }

                // buffer->pixels[y * buffer->width + x] = color;
                size_t pal = (x % 3);
                if ((y % 3) == 2) { pal += 3; } // dark row
                *pdest++ = palettes[pal][color];
            }
        }

        dirty = false;
        fpl->screenLock.lock();
        fpl->buffered.imageCreated = true;
        fpl->screenLock.unlock();
    }
}



OsFPL::~OsFPL() {
    screenLock.lock();
    buffered.stopThread = true;
    screenLock.unlock();
}

bool OsFPL::init(Basic* basic, SoundSystem* sound) {
    Os::init(basic, sound);

    fplSettings settings;
    fplSetDefaultSettings(&settings);

    const int border = 64;
    settings.window.isResizable = true;
    settings.window.windowSize = {640 + 2 * border, 400 + 2 * border};
    strcpy(settings.window.title, "BA67 BASIC");
    settings.window.fullscreenRefreshRate = 30;

    settings.video.isAutoSize = false; // we resize ourself

    settings.video.backend = fplVideoBackendType_Software;
    // settings.window.icons[0] // TODO icons

    if (!fplPlatformInit(
        /*fplInitFlags_Console | fplInitFlags_Audio |*/ fplInitFlags_Window | fplInitFlags_Video | fplInitFlags_GameController,
        &settings)) {
        return false;
    }



#ifdef _WIN32
    fpl__PlatformAppState* appState = fpl__global__AppState;
    fpl__Win32AppState* win32AppState = &appState->win32;
    fpl__Win32WindowState* windowState = &appState->window.win32;
    HWND hWnd = windowState->windowHandle;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
    SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    wchar_t* user = nullptr;
    size_t len = 0;
    if (0 == _wdupenv_s(&user, &len, L"USERPROFILE") && user != nullptr) {
        std::wstring home = user;
        free(user); user = nullptr;
        home += L"\\Documents";
        ::CreateDirectoryW(home.c_str(), NULL);
        home += L"\\BASIC";
        ::CreateDirectoryW(home.c_str(), NULL);
        ::SetCurrentDirectoryW(home.c_str());
    }
#else
    char homepath[1024] = {};
    fplGetHomePath(homepath, 1024);
    std::string home = homepath;
    for (auto& c : home) {
        if (c == '\\') { c = '/'; }
    }

    if (doesFileExist(home) && isDirectory(home)) {
        setCurrentDirectory(home);
        home += "/BASIC";
        if (!doesFileExist(home)) {
            std::filesystem::create_directory("BASIC");
        }
        setCurrentDirectory(home);
    }
#endif

    std::thread upd(displayUpdateThread, this);
    upd.detach();
    return true;
}

uint64_t OsFPL::tick() const {
    static auto start = fplMillisecondsQuery();
    return fplMillisecondsQuery() - start;
}

void OsFPL::delay(int ms) const {
    fplThreadSleep(ms);
}

size_t OsFPL::getFreeMemoryInBytes() {
    fplMemoryInfos mem = {};
    fplMemoryGetInfos(&mem);
    return mem.freePhysicalSize;
}


void OsFPL::presentScreen() {
    updateKeyboardBuffer(); // pump win32 messages
    static uint64_t nextPresend = 0;
    uint64_t now = tick();
    if (nextPresend > now) { return; }
    nextPresend = now + 50;

    screenLock.lock();

    // window resized?
    fplWindowSize windowsz{0, 0};
    fplGetWindowSize(&windowsz);
    fplVideoBackBuffer* buffer = fplGetVideoBackBuffer();
    if (buffer != nullptr) {
        buffered.videoH = buffer->height;
        buffered.videoW = buffer->width;
        if (buffer->width != windowsz.width || buffer->height != windowsz.height) {
            dirtyFlag = true;
            buffered.imageCreated = false;
            fplResizeVideoBackBuffer(windowsz.width, windowsz.height);
            buffer = fplGetVideoBackBuffer();
        }
    }

    if (buffered.imageCreated) {
        buffered.imageCreated = false;
        if (buffer != nullptr && buffer->pixels != nullptr && buffer->width * buffer->height == pixelsVideo.size()) {
            memcpy(buffer->pixels, &pixelsVideo[0], sizeof(uint32_t) * pixelsVideo.size());
        }
        fplVideoFlip();
    }

    screen.updateScreenPixelsPalette();
    if (buffered.pixelsPal.size() == screen.screenBitmap.pixelsPal.size()) {
        auto* src = &screen.screenBitmap.pixelsPal[0];
        auto* dst = &buffered.pixelsPal[0];
        for (size_t i = screen.screenBitmap.pixelsPal.size(); i > 0; --i) {
            if (*dst != *src) {
                dirtyFlag = true;
                *dst = *src;
            }
            ++src;
            ++dst;
        }
    } else {
        dirtyFlag = true;
        buffered.pixelsPal = screen.screenBitmap.pixelsPal;
    }
    buffered.isCursorActive = basic->isCursorActive;
    buffered.crsrColor = screen.getTextColor();
    buffered.borderColor = screen.getBorderColor();
    auto crsr = screen.getCursorPos();
    if (crsr != buffered.crsrPos) {
        dirtyFlag = true;
        buffered.crsrPos = crsr;
    }
    buffered.palette = screen.palette;

    screenLock.unlock();

#if 0
    return;


    fplWindowSize windowsz{0, 0};
    fplGetWindowSize(&windowsz);
    fplVideoBackBuffer* buffer = fplGetVideoBackBuffer();
    if (buffer != nullptr) {
        if (buffer->width != windowsz.width || buffer->height != windowsz.height) {
            fplResizeVideoBackBuffer(windowsz.width, windowsz.height);
            buffer = fplGetVideoBackBuffer();
        }
    }


    // simulate a CRT TV
    std::array<std::array<uint32_t, 16>, 6> palettes; // [r,g,b, dark r,g,b]
    for (size_t p = 0; p < 6; ++p) {
        const float facDark = 0.7f;
        const float facNeigbour = 0.6f, facNeighbour2 = 0.6f;
        float r = 1.0f, g = 1.0f, b = 1.0f, darken = 1.0f;
        if (p == 0 || p == 3) { r = 1.0f;   g = facNeigbour; b = facNeighbour2; }
        if (p == 1 || p == 4) { r = facNeighbour2;   g = 1.0f;  b = facNeigbour; }
        if (p == 2 || p == 5) { r = facNeigbour; g = facNeighbour2; b = 1.0f; }
        //        if (p == 3 || p == 7) { r = g = b = 0.95; }
        if (p > 2) { darken = facDark; }
        for (size_t i = 0; i < 16; ++i) {
            palettes[p][i] = emphasizeRGB(screen.palette[i], r, g, b, darken);
        }
    }


    if (buffer) {
        screen.updateScreenPixelsPalette();
        // memcpy(buffer->pixels, screen.pixelData, buffer->width * buffer->height * 4);
        constexpr size_t srcWidth = ScreenInfo::pixX;  // ScreenBitmap width (80x8)
        constexpr size_t srcHeight = ScreenInfo::pixY; // ScreenBitmap height (25x16)

        // overwrite cursor box
        uint8_t crsrColor = uint8_t(screen.getTextColor());
        const uint64_t flashSpeed = 240;
        auto now = tick();
        if (nextShowCursor == 0) { nextShowCursor = now; }
        if (cursorVisible && now >= nextShowCursor) {
            if (now > nextShowCursor + flashSpeed) {
                nextShowCursor = now + 2 * flashSpeed;
            }

            auto crsr = screen.getCursorPos();
            if (this->cursorVisible
                && this->basic->isCursorActive
                && crsr.x < screen.getWidth()
                && crsr.y < screen.getHeight()) {
                bool draw = false;
                for (size_t y = 0; y < ScreenInfo::charPixY; ++y) {
                    auto* pdest = &screen.screenBitmap.pixelsPal[(y + crsr.y * ScreenInfo::charPixY) * srcWidth + crsr.x * ScreenInfo::charPixX];
                    for (size_t x = 0; x < 8; ++x) {
                        draw = !draw;
                        if (draw || y + 1 == ScreenInfo::charPixY) {
                            *pdest = crsrColor;
                        }
                        ++pdest;
                    }
                    draw = !draw;
                }
            }
        }
        // we don't access the RGB buffer - we use the colour indices
        // screen.updateScreenBitmap();

        // Compute scaling factors
        float scaleX = static_cast<float>(buffer->width) / srcWidth;
        float scaleY = static_cast<float>(buffer->height) / srcHeight;
        float scale = std::max(0.25f, std::min(scaleX, scaleY)); // Keep aspect ratio
        if (scale > 1.0f) {
            scale = floorf(scale); // scale to full pixels
        }

        // Compute offset for centered output
        size_t scaledWidth = static_cast<size_t>(srcWidth * scale);
        size_t scaledHeight = static_cast<size_t>(srcHeight * scale);
        size_t offsetX = (buffer->width - scaledWidth) / 2;
        size_t offsetY = (buffer->height - scaledHeight) / 2;

        uint8_t borderColor = screen.getBorderColor();
        // Nearest-neighbor scaling loop
    #pragma omp for
        for (int y = 0; y < int(buffer->height); ++y) {
            uint32_t* pdest = &buffer->pixels[0] + y * buffer->width;
            for (size_t x = 0; x < buffer->width; ++x) {
                // Map screen coordinates to original ScreenBitmap using nearest-neighbor
                size_t srcX = std::min(static_cast<size_t>((x - offsetX) / scale), srcWidth - 1);
                size_t srcY = std::min(static_cast<size_t>((y - offsetY) / scale), srcHeight - 1);

                // Default background if outside scaled region
                uint8_t color = borderColor; // Transparent or black

                // Only sample from ScreenBitmap if inside scaled bounds
                if (x >= offsetX && x < offsetX + scaledWidth &&
                    y >= offsetY && y < offsetY + scaledHeight) {
                    color = screen.screenBitmap.pixelsPal[srcY * srcWidth + srcX];
                }

                // buffer->pixels[y * buffer->width + x] = color;
                size_t pal = (x % 3);
                if ((y % 3) == 2) { pal += 3; } // dark row
                *pdest++ = palettes[pal][color];
            }
        }

        fplVideoFlip();
    }
#endif
}

void OsFPL::updateKeyboardBuffer() {
    static auto lastTick = tick();
    auto tickNow = tick();
    auto deltaTick = tickNow - lastTick;
    if (deltaTick < 16) {
        delay(int(deltaTick));
        lastTick = tickNow;
    }

    fplWindowSize sz{0, 0};
    fplGetWindowSize(&sz);

    // it seems fplWindowEventType_Closed is never fired
    if (sz.width == 0) {
        exit(0);
    }

    static KeyPress lastCharPress = {};

    fplEvent event;
    while (fplPollEvent(&event)) {
        if (event.type == fplEventType_Window) {
            if (event.window.type == fplWindowEventType_Closed) {
                exit(0);
            }

            if (event.window.type == fplWindowEventType_Restored
                || event.window.type == fplWindowEventType_Maximized) {
                fplResizeVideoBackBuffer(sz.width, sz.height);
            }
        }

        if (event.type == fplEventType_Keyboard) {
            KeyPress keyPress;
            keyPress.holdShift = event.keyboard.modifiers & fplKeyboardModifierFlags_LShift;
            keyPress.holdCtrl = event.keyboard.modifiers & fplKeyboardModifierFlags_LCtrl;
            keyPress.holdAlt = event.keyboard.modifiers & fplKeyboardModifierFlags_LAlt;
            keyPress.code = uint32_t(event.keyboard.keyCode);

            if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Release) {
                lastCharPress.code = 0;
            }

            if (event.keyboard.type == fplKeyboardEventType_Input) {
                keyPress.printable = true;
                putToKeyboardBuffer(keyPress);
                lastCharPress = keyPress;
            } else if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Repeat) {
                if (lastCharPress.code != 0) {
                    putToKeyboardBuffer(lastCharPress);
                }
            } else if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Press) {
                keyPress.code = 0;
                keyPress.printable = false;

                bool repeatable = false;
                // these provide character input
                // case fplKey_Backspace:keyPress.code = uint32_t(KeyConstant::BACKSPACE); break;
                // case fplKey_Return:   keyPress.code = uint32_t(KeyConstant::RETURN); break;
                // case fplKey_KPEnter:  yPress.code = uint32_t(KeyConstant::NUM_ENTER); break;
                // case fplKey_Escape:   keyPress.code = uint32_t(KeyConstant::ESCAPE); break;
                switch (event.keyboard.keyCode) {
                case fplKey_Delete:   keyPress.code = uint32_t(KeyConstant::DEL); repeatable = true; break;
                case fplKey_F1:       keyPress.code = uint32_t(KeyConstant::F1); break;
                case fplKey_F2:       keyPress.code = uint32_t(KeyConstant::F2); break;
                case fplKey_F3:       keyPress.code = uint32_t(KeyConstant::F3); break;
                case fplKey_F4:       keyPress.code = uint32_t(KeyConstant::F4); break;
                case fplKey_F5:       keyPress.code = uint32_t(KeyConstant::F5); break;
                case fplKey_F6:       keyPress.code = uint32_t(KeyConstant::F6); break;
                case fplKey_F7:       keyPress.code = uint32_t(KeyConstant::F7); break;
                case fplKey_F8:       keyPress.code = uint32_t(KeyConstant::F8); break;
                case fplKey_F9:       keyPress.code = uint32_t(KeyConstant::F9); break;
                case fplKey_F10:      keyPress.code = uint32_t(KeyConstant::F10); break;
                case fplKey_F11:      keyPress.code = uint32_t(KeyConstant::F11); break;
                case fplKey_F12:      keyPress.code = uint32_t(KeyConstant::F12); break;
                case fplKey_Home:     keyPress.code = uint32_t(KeyConstant::HOME); break;
                case fplKey_Insert:   keyPress.code = uint32_t(KeyConstant::INSERT); repeatable = true; break;
                case fplKey_End:      keyPress.code = uint32_t(KeyConstant::END); break;
                case fplKey_PageUp:   keyPress.code = uint32_t(KeyConstant::PG_UP);  repeatable = true; break;
                case fplKey_PageDown: keyPress.code = uint32_t(KeyConstant::PG_DOWN);  repeatable = true; break;
                case fplKey_Up:       keyPress.code = uint32_t(KeyConstant::CRSR_UP);  repeatable = true; break;
                case fplKey_Down:     keyPress.code = uint32_t(KeyConstant::CRSR_DOWN);  repeatable = true; break;
                case fplKey_Left:     keyPress.code = uint32_t(KeyConstant::CRSR_LEFT);  repeatable = true; break;
                case fplKey_Right:    keyPress.code = uint32_t(KeyConstant::CRSR_RIGHT);  repeatable = true; break;
                case fplKey_Scroll:   keyPress.code = uint32_t(KeyConstant::SCROLL); break;
                case fplKey_Pause:    keyPress.code = uint32_t(KeyConstant::PAUSE); break;

                    // these do not create keypress buffer entries
                case fplKey_Alt:
                case fplKey_Control:
                case fplKey_Shift:
                case fplKey_LeftAlt:
                case fplKey_LeftControl:
                case fplKey_LeftShift:
                case fplKey_LeftSuper:
                case fplKey_RightAlt:
                case fplKey_RightControl:
                case fplKey_RightShift:
                case fplKey_RightSuper:
                    repeatable = false;
                    continue;
                default:
                    break;
                }
                if (keyPress.code != 0) {
                    putToKeyboardBuffer(keyPress);
                    if (repeatable) {
                        lastCharPress = keyPress;
                    } else {
                        lastCharPress.code = 0;
                    }
                }
            }
        }
    }
}

const bool OsFPL::isKeyPressed(uint32_t index, bool withShift, bool withAlt, bool withCtrl) const {
    fplKeyboardState keyboardState;
    if (!fplPollKeyboardState(&keyboardState)) {
        return false;
    }

    switch (index) {
    case uint32_t(KeyConstant::ESCAPE):    index = fplKey_Escape ; break;
    case uint32_t(KeyConstant::BACKSPACE): index = fplKey_Backspace ; break;
    case uint32_t(KeyConstant::RETURN):    index = fplKey_Return ; break;
        // case uint32_t(KeyConstant::NUM_ENTER): index = fplKey_KPEnter ; break;
    case uint32_t(KeyConstant::DEL):       index = fplKey_Delete ; break;
    case uint32_t(KeyConstant::F1):        index = fplKey_F1 ; break;
    case uint32_t(KeyConstant::F2):        index = fplKey_F2 ; break;
    case uint32_t(KeyConstant::F3):        index = fplKey_F3 ; break;
    case uint32_t(KeyConstant::F4):        index = fplKey_F4 ; break;
    case uint32_t(KeyConstant::F5):        index = fplKey_F5 ; break;
    case uint32_t(KeyConstant::F6):        index = fplKey_F6 ; break;
    case uint32_t(KeyConstant::F7):        index = fplKey_F7 ; break;
    case uint32_t(KeyConstant::F8):        index = fplKey_F8 ; break;
    case uint32_t(KeyConstant::F9):        index = fplKey_F9 ; break;
    case uint32_t(KeyConstant::F10):       index = fplKey_F10; break;
    case uint32_t(KeyConstant::F11):       index = fplKey_F11; break;
    case uint32_t(KeyConstant::F12):       index = fplKey_F12; break;
    case uint32_t(KeyConstant::HOME):      index = fplKey_Home ; break;
    case uint32_t(KeyConstant::INSERT):    index = fplKey_Insert ; break;
    case uint32_t(KeyConstant::END):       index = fplKey_End ; break;
    case uint32_t(KeyConstant::PG_UP):     index = fplKey_PageUp ; break;
    case uint32_t(KeyConstant::PG_DOWN):   index = fplKey_PageDown ; break;
    case uint32_t(KeyConstant::CRSR_UP):   index = fplKey_Up ; break;
    case uint32_t(KeyConstant::CRSR_DOWN): index = fplKey_Down ; break;
    case uint32_t(KeyConstant::CRSR_LEFT): index = fplKey_Left ; break;
    case uint32_t(KeyConstant::CRSR_RIGHT):index = fplKey_Right ; break;
    case uint32_t(KeyConstant::SCROLL):    index = fplKey_Scroll; break;
    case uint32_t(KeyConstant::PAUSE):     index = fplKey_Pause ; break;
    default: break;
    }
    return (keyboardState.buttonStatesMapped[index] >= fplButtonState_Press);
}

void OsFPL::putToKeyboardBuffer(Os::KeyPress key) {
    Os::putToKeyboardBuffer(key);
}
