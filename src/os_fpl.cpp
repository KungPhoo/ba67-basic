#include "os_fpl.h"

// https://libfpl.org/docs/page_categories.html
#define FPL_IMPLEMENTATION
#define FPL_LOGGING
#define FPL_NO_AUDIO

#include "final_platform_layer.h"
#include "basic.h"
#include <thread>
#include <filesystem>
#include <cmath>
#include <cstring>
#include <array>
#include "unicode.h"

#if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)
    #include <GL/gl.h>
#endif

#ifdef _WIN32
    #include <Windows.h>
    #include <tchar.h>
    #include "../resources/resource.h"
#endif

void displayUpdateThread(OsFPL* fpl);

OsFPL::~OsFPL() {
    screenLock.lock();
    buffered.stopThread = true;
    screenLock.unlock();
    fplWindowShutdown();
}

bool OsFPL::init(Basic* basic, SoundSystem* sound) {
    Os::init(basic, sound);

    fplSettings settings;
    fplSetDefaultSettings(&settings);

    const int border = 64;
    settings.window.isResizable = true;
    settings.window.windowSize = {640 + 2 * border, 400 + 2 * border};
    strcpy(settings.window.title, "BA67 BASIC");
    settings.window.fullscreenRefreshRate = 60;
    settings.video.isAutoSize = false;  // we resize ourself

    settings.window.isFullscreen = false;
    settings.window.isScreenSaverPrevented = true;
    settings.window.isMonitorPowerPrevented = true;

    settings.input.controllerDetectionFrequency = 5000;

#if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)
    if (this->settings.renderMode == BA68settings::OpenGL) {
        // Use Legacy OpenGL (1.1)
        settings.video.backend = fplVideoBackendType_OpenGL;
        settings.video.graphics.opengl.compabilityFlags = fplOpenGLCompabilityFlags_Legacy;
        settings.video.isVSync = true;
    } else
#endif
    {
        settings.video.backend = fplVideoBackendType_Software;
    }
    // settings.window.icons[0] // TODO: icons
    // settings.window.icons[0].data = icon16Data;
    // settings.window.icons[0].width = iconW;
    // settings.window.icons[0].height = iconH;
    // settings.window.icons[0].type = fplImageType_RGBA;

    if (!fplPlatformInit(
            /*fplInitFlags_Console | fplInitFlags_Audio |*/ fplInitFlags_Window | fplInitFlags_Video | fplInitFlags_GameController,
            &settings)) {
        return false;
    }

    if (this->settings.fullscreen) {
        // Enable fullscreen on the nearest desktop
        fplSetWindowFullscreenSize(true, 0, 0, 0);
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
        free(user);
        user = nullptr;
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

void OsFPL::delay(int ms) {
    if (ms > 20) {
        auto endTick = tick() + ms;
        while (ms > 20) {
            presentScreen();
            ms -= 20;
            fplThreadSleep(18);
        }
        uint64_t now = tick();
        if (now + 5 < endTick) {
            fplThreadSleep(uint32_t(endTick - now) - 4);
        }
    } else {
        fplThreadSleep(ms);
    }
}

#if !defined(_WIN32)
    #include <cstdio>
    #include <cstring>
// Return the amount of free memory in kilobytes.
// Returns -1 if something went wrong.
// https://stackoverflow.com/a/17518259/2721136
int getFreeMemoryProcMeminfo() {
    printf("parsing /proc/meminfo\n");
    int returnValue;
    const int BUFFER_SIZE = 1000;
    char buffer[BUFFER_SIZE];
    FILE* fInput;
    int loop;
    int len;
    char ch;
    returnValue = 0;
    fInput = fopen("/proc/meminfo", "r");
    if (fInput != NULL) {
        while (!feof(fInput)) {
            fgets(buffer, BUFFER_SIZE - 1, fInput);
            if (feof(fInput)) {
                break;
            }
            buffer[BUFFER_SIZE - 1] = 0;
            // Look for serial number
            if (strncmp(buffer, "MemFree:", 8) == 0) {
                printf("found MemFree:\n");
                // Extract mem free from the line.
                for (loop = 0; loop < BUFFER_SIZE; loop++) {
                    ch = buffer[loop];
                    if (ch == 0) {
                        break;
                    } else if ((ch >= '0') && (ch <= '9')) {
                        returnValue = returnValue * 10 + (ch - '0');
                    }
                }
                break;
            }
        }
        fclose(fInput);
    } else {
        printf("failed to open /proc/meminfo\n");
    }
    return returnValue;
}
#endif

size_t OsFPL::getFreeMemoryInBytes() {
    fplMemoryInfos mem = {};
    if (fplMemoryGetInfos(&mem)) {
        return mem.freePhysicalSize;
    }
#if !defined(_WIN32)
    return 1024 * size_t(getFreeMemoryProcMeminfo());
#endif
    return 128 * 1024;
}

static uint32_t emphasizeRGB(uint32_t color, double facR, double facG, double facB, double facDark) {
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t b = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t r = (color) & 0xFF;

    // Convert RGB to perceived brightness (luminance)
    double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

    // const double brightness = 37.0;  // -200..200
    // const double contrast = 1.5;     // 0..1

    const double brightness = 60;  // -200..200
    const double contrast = 1.5;   // 0..1

    // https://ie.nitk.ac.in/blog/2020/01/19/algorithms-for-adjusting-brightness-and-contrast-of-an-image/
    auto truncate = [](double f) {if (f > 255.0) { return 255.0; } if (f < 0.0) { return 0.0; } return f; };
    double factor = (259.0f * (contrast + 255.0)) / (255.0 * (259.0 - contrast));
    double new_r = truncate((((r - 128.0) * contrast) + 128 + brightness) * facR * facDark);
    double new_g = truncate((((g - 128.0) * contrast) + 128 + brightness) * facG * facDark);
    double new_b = truncate((((b - 128.0) * contrast) + 128 + brightness) * facB * facDark);

    r = (uint8_t)new_r;
    g = (uint8_t)new_g;
    b = (uint8_t)new_b;

    return (a << 24) | (b << 16) | (g << 8) | r;
}

void displayUpdateThread(OsFPL* fpl) {
    bool dirty = true;
    std::array<std::array<uint32_t, 16>, 6> palettes;  // [r,g,b, dark r,g,b]
    OsFPL::Buffered state;
    // std::vector<uint8_t> pixelsPal; // what's actually drawn

    bool oldCursorVisible = false;
    uint64_t nextShowCursor = 0;                    // blink time
    constexpr size_t srcWidth = ScreenInfo::pixX;   // ScreenBitmap width (80x8)
    constexpr size_t srcHeight = ScreenInfo::pixY;  // ScreenBitmap height (25x16)

    uint64_t sleepAfter = 0;

    static int passes = 0;
    bool cursorVisible = true;
    for (;;) {
        // == UPDATE STATE ==
        uint64_t now = fplMillisecondsQuery();
        fpl->screenLock.lock();
        if (fpl->buffered.stopThread) {
            fpl->screenLock.unlock();
            return;
        }

        if (!dirty) { dirty = fpl->buffered.screen.dirtyFlag; }
        if (dirty) {
            state = fpl->buffered;  // copy to thread data - this way we don't need to lock the main thread for drawing
            fpl->buffered.screen.dirtyFlag = false;
            nextShowCursor = 0;
            sleepAfter = now + 500;
        }
        if (state.videoH * state.videoW != fpl->pixelsVideo.size()) {
            fpl->pixelsVideo.resize(state.videoH * state.videoW);
            dirty = true;
        }
        fpl->screenLock.unlock();

        // overwrite cursor box
        const uint64_t flashSpeed = 560;

        if (now > nextShowCursor) {
            nextShowCursor = now + flashSpeed;
            cursorVisible = !cursorVisible;
            dirty = true;
        }

        if (!dirty) {
            if (sleepAfter < now) {
                fplThreadSleep(50);
            }
            continue;
        }

        // render chars and sprites to palette based buffer
        state.screen.updateScreenPixelsPalette();
        if (state.screen.screenBitmap.pixelsPal.empty()) { continue; }
        if (state.videoH + state.videoW == 0) { continue; }

        // make a deep copy of the pixel buffer - we optionally overwrite the blinking cursor
        // pixelsPal = state.screen.screenBitmap.pixelsPal;

        if (state.isCursorActive) {
            auto crsr = state.screen.getCursorPos();
            if (cursorVisible && crsr.x < ScreenBuffer::width && crsr.y < ScreenBuffer::height) {
                auto sc = state.screen.getLineBuffer()[crsr.y]->cols[crsr.x];
                uint8_t bkcol = sc.col;
                uint8_t txcol = bkcol & 0x0f;
                bkcol >>= 4;
                if (sc.ch == U'\0') {
                    txcol = state.screen.getTextColor();
                    bkcol = state.screen.getBackgroundColor();
                }
                if (txcol == bkcol) {
                    txcol = 1;
                    bkcol = 0;
                }

                for (size_t y = 0; y < ScreenInfo::charPixY; ++y) {
                    uint8_t* pdest = &state.screen.screenBitmap.pixelsPal[(y + crsr.y * ScreenInfo::charPixY) * srcWidth + crsr.x * ScreenInfo::charPixX];
                    for (size_t x = 0; x < 8; ++x) {
                        if (*pdest == bkcol) {
                            *pdest = txcol;
                        } else {
                            *pdest = bkcol;
                        }
                        ++pdest;
                    }
                }
            }
        }

        // == DRAW ==

        // simulate a CRT TV with a 3x3 pixel matrix
#ifdef BA67_GRAPHICS_CRT_EMULATION_ON
        for (size_t p = 0; p < 6; ++p) {
            const double facDark = 0.7;                           // factor for darker scanlines
            const double facNeigbour = 0.6, facNeighbour2 = 0.6;  // factor for neighbored r,g,b channels. On true CRT screen, that would be 0.0
            double r = 1.0, g = 1.0, b = 1.0, darken = 1.0;       // factor for r,g,b channels
            if (p == 0 || p == 3) {
                r = 1.0;
                g = facNeigbour;
                b = facNeighbour2;
            }
            if (p == 1 || p == 4) {
                r = facNeighbour2;
                g = 1.0;
                b = facNeigbour;
            }
            if (p == 2 || p == 5) {
                r = facNeigbour;
                g = facNeighbour2;
                b = 1.0;
            }
            //        if (p == 3 || p == 7) { r = g = b = 0.95; }
            if (p > 2) { darken = facDark; }
            for (size_t i = 0; i < 16; ++i) {
                if (state.crtEmulation) {
                    palettes[p][i] = emphasizeRGB(state.screen.palette[i], r, g, b, darken);
                } else {
                    palettes[p][i] = state.screen.palette[i];
                }
            }
        }
#else
        for (size_t p = 0; p < 6; ++p) {
            for (size_t i = 0; i < 16; ++i) {
                palettes[p][i] = state.screen.palette[i];
            }
        }
#endif

        // we don't access the RGB buffer - we use the color indices
        // screen.updateScreenBitmap();

        // Compute scaling factors
        double scaleX = static_cast<double>(state.videoW) / srcWidth;
        double scaleY = static_cast<double>(state.videoH) / srcHeight;
        double scale = std::max(0.25, std::min(scaleX, scaleY));  // Keep aspect ratio
        if (scale > 1.0) {
            scale = floor(scale);  // scale to full pixels
        }

        // Compute offset for centered output
        size_t scaledWidth = static_cast<size_t>(srcWidth * scale);
        size_t scaledHeight = static_cast<size_t>(srcHeight * scale);
        size_t offsetX = (state.videoW - scaledWidth) / 2;
        size_t offsetY = (state.videoH - scaledHeight) / 2;

        // Nearest-neighbor scaling loop
        fpl->videoLock.lock();
        for (int y = 0; y < int(state.videoH); ++y) {
            uint32_t* pdest = &fpl->pixelsVideo[0] + y * state.videoW;

            size_t srcY = std::min(static_cast<size_t>((y - offsetY) / scale), srcHeight - 1);
            for (size_t x = 0; x < state.videoW; ++x) {
                // Map screen coordinates to original ScreenBitmap using nearest-neighbor
                size_t srcX = std::min(static_cast<size_t>((x - offsetX) / scale), srcWidth - 1);

                // Default background if outside scaled region
                uint8_t color = state.screen.getBorderColor();  // Transparent or black

                // Only sample from ScreenBitmap if inside scaled bounds
                if (x >= offsetX && x < offsetX + scaledWidth &&
                    y >= offsetY && y < offsetY + scaledHeight) {
                    color = state.screen.screenBitmap.pixelsPal[srcY * srcWidth + srcX];
                }

                // buffer->pixels[y * buffer->width + x] = color;
                size_t pal = (x % 3);
                if ((y % 3) == 2) { pal += 3; }  // dark row
                *pdest++ = palettes[pal][color];
            }
        }
        fpl->videoLock.unlock();

        fpl->screenLock.lock();
        dirty = false;
        fpl->buffered.imageCreated = true;
        fpl->screenLock.unlock();
    }
}

void OsFPL::presentScreen() {
    updateKeyboardBuffer();
    updateGamepadState();
    fplEvent ev;
    while (fplPollEvent(&ev)) {}      // pump messages
    if (!fplWindowUpdate()) {         // window might be minimized
        if (!fplIsWindowRunning()) {  // window was closed
            exit(0);
        }

        // TODO minimizing the window exits
        return;
    }

    static uint64_t nextPresend = 0;
    uint64_t now = tick();
    if (nextPresend > now) { return; }
    nextPresend = now + 50;

    screenLock.lock();
    buffered.crtEmulation = settings.emulateCRT;
    switch (settings.renderMode) {
        case BA68settings::OpenGL:
#if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)
            renderOpenGL();
            break;
#endif
        case BA68settings::Software:
            renderSoftware();
            break;
    }

    if (screen.dirtyFlag) {
        ScreenBuffer::copyWithLock(buffered.screen, screen);
        screen.dirtyFlag = false;
    }
    buffered.isCursorActive = basic->isCursorActive;
    screenLock.unlock();
}

void OsFPL::renderSoftware() {
    // window resized?
    fplWindowSize windowsz{0, 0};
    fplGetWindowSize(&windowsz);
    fplVideoBackBuffer* buffer = fplGetVideoBackBuffer();
    if (buffer != nullptr) {
        buffered.videoH = buffer->height;
        buffered.videoW = buffer->width;
        if (buffer->width != windowsz.width || buffer->height != windowsz.height) {
            screen.dirtyFlag = true;
            buffered.imageCreated = false;
            fplResizeVideoBackBuffer(windowsz.width, windowsz.height);
            buffer = fplGetVideoBackBuffer();
        }
    }

    if (buffered.imageCreated) {
        buffered.imageCreated = false;
        videoLock.lock();
        if (buffer != nullptr && buffer->pixels != nullptr && buffer->width * buffer->height == pixelsVideo.size()) {
            memcpy(buffer->pixels, &pixelsVideo[0], sizeof(uint32_t) * pixelsVideo.size());
        }
        videoLock.unlock();
        fplVideoFlip();
    }
}

void OsFPL::renderOpenGL() {
#if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)

    // Get window size
    fplWindowSize winSize = {};
    if (!fplGetWindowSize(&winSize)) {
        printf("can't get window size\n");
        return;
    }
    if (buffered.videoW != winSize.width || buffered.videoH != winSize.height) {
        buffered.videoW = winSize.width;
        buffered.videoH = winSize.height;
        screen.dirtyFlag = true;
        buffered.imageCreated = false;
        return;
    }

    if (winSize.width * winSize.height == pixelsVideo.size()) {
        glClear(GL_COLOR_BUFFER_BIT);

        // Flip the image vertically
        glViewport(0, 0, winSize.width, winSize.height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, winSize.width, 0, winSize.height, 0.1, 1);
        glPixelZoom(1, -1);
        glRasterPos3f(0, GLfloat(winSize.height - 1), -0.3f);

        // Draw pixels
        glDrawPixels(winSize.width, winSize.height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixelsVideo.data());

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
        return;  // window minimized
    }

    static KeyPress lastCharPress = {};

    fplEvent event;
    while (fplPollEvent(&event)) {
        if (event.type == fplEventType_Window) {
            if (event.window.type == fplWindowEventType_Closed) {
                exit(0);
            }

            if (event.window.type == fplWindowEventType_Restored || event.window.type == fplWindowEventType_Maximized) {
                fplResizeVideoBackBuffer(sz.width, sz.height);
            }
        }

        if (event.type == fplEventType_Keyboard) {
            KeyPress keyPress;
            keyPress.holdShift = event.keyboard.modifiers & fplKeyboardModifierFlags_LShift;
            keyPress.holdCtrl = event.keyboard.modifiers & fplKeyboardModifierFlags_LCtrl;
            keyPress.holdAlt = event.keyboard.modifiers & fplKeyboardModifierFlags_LAlt;
            keyPress.code = uint32_t(event.keyboard.keyCode);  // has umlaut characters

            if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Release) {
                lastCharPress.code = 0;
            }

            if (event.keyboard.type == fplKeyboardEventType_Input) {
                if (keyPress.code == 0x7f) {  // DEL (only sent on Linux)
                    lastCharPress.code = 0;
                    continue;
                }

                keyPress.printable = true;
                putToKeyboardBuffer(keyPress);
                lastCharPress = keyPress;
            } else if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Press) {
                // keyPress.code = 0;
                keyPress.printable = false;
                keyPress.code = uint32_t(event.keyboard.mappedKey);  // here, the keyCode on Linux is just wrong

                bool repeatable = false;
                // these provide character input
                // case fplKey_Backspace:keyPress.code = uint32_t(KeyConstant::BACKSPACE); break;
                // case fplKey_Return:   keyPress.code = uint32_t(KeyConstant::RETURN); break;
                // case fplKey_KPEnter:  yPress.code = uint32_t(KeyConstant::NUM_ENTER); break;
                // case fplKey_Escape:   keyPress.code = uint32_t(KeyConstant::ESCAPE); break;
                switch (event.keyboard.mappedKey) {
                    case fplKey_Delete:
                        keyPress.code = uint32_t(KeyConstant::DEL);  // Linux sends this AND input 0x7f. Windows just this.
                        repeatable = true;
                        break;
                    case fplKey_F1:
                        keyPress.code = uint32_t(KeyConstant::F1);
                        break;
                    case fplKey_F2:
                        keyPress.code = uint32_t(KeyConstant::F2);
                        break;
                    case fplKey_F3:
                        keyPress.code = uint32_t(KeyConstant::F3);
                        break;
                    case fplKey_F4:
                        keyPress.code = uint32_t(KeyConstant::F4);
                        break;
                    case fplKey_F5:
                        keyPress.code = uint32_t(KeyConstant::F5);
                        break;
                    case fplKey_F6:
                        keyPress.code = uint32_t(KeyConstant::F6);
                        break;
                    case fplKey_F7:
                        keyPress.code = uint32_t(KeyConstant::F7);
                        break;
                    case fplKey_F8:
                        keyPress.code = uint32_t(KeyConstant::F8);
                        break;
                    case fplKey_F9:
                        keyPress.code = uint32_t(KeyConstant::F9);
                        break;
                    case fplKey_F10:
                        keyPress.code = uint32_t(KeyConstant::F10);
                        break;
                    case fplKey_F11:
                        keyPress.code = uint32_t(KeyConstant::F11);
                        break;
                    case fplKey_F12:
                        keyPress.code = uint32_t(KeyConstant::F12);
                        break;
                    case fplKey_Home:
                        keyPress.code = uint32_t(KeyConstant::HOME);
                        break;
                    case fplKey_Insert:
                        keyPress.code = uint32_t(KeyConstant::INSERT);
                        repeatable = true;
                        break;
                    case fplKey_End:
                        keyPress.code = uint32_t(KeyConstant::END);
                        break;
                    case fplKey_PageUp:
                        keyPress.code = uint32_t(KeyConstant::PG_UP);
                        repeatable = true;
                        break;
                    case fplKey_PageDown:
                        keyPress.code = uint32_t(KeyConstant::PG_DOWN);
                        repeatable = true;
                        break;
                    case fplKey_Up:
                        keyPress.code = uint32_t(KeyConstant::CRSR_UP);
                        repeatable = true;
                        break;
                    case fplKey_Down:
                        keyPress.code = uint32_t(KeyConstant::CRSR_DOWN);
                        repeatable = true;
                        break;
                    case fplKey_Left:
                        keyPress.code = uint32_t(KeyConstant::CRSR_LEFT);
                        repeatable = true;
                        break;
                    case fplKey_Right:
                        keyPress.code = uint32_t(KeyConstant::CRSR_RIGHT);
                        repeatable = true;
                        break;
                    case fplKey_Scroll:
                        keyPress.code = uint32_t(KeyConstant::SCROLL);
                        break;
                    case fplKey_Pause:
                        keyPress.code = uint32_t(KeyConstant::PAUSE);
                        break;

                        // these do not create key-press buffer entries
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
                        if (!keyPress.holdAlt && !keyPress.holdCtrl) {
                            keyPress.code = 0;
                        }
                        break;
                }
                if (keyPress.code != 0) {
                    // printf("press mappedkey $%x keycode: $%x shift %c alt %c ctrl %c \n", int(event.keyboard.mappedKey), int(event.keyboard.keyCode), keyPress.holdShift ? 'X' : 'O', keyPress.holdAlt ? 'X' : 'O', keyPress.holdCtrl ? 'X' : 'O');
                    putToKeyboardBuffer(keyPress);
                    if (repeatable) {
                        lastCharPress = keyPress;
                    } else {
                        lastCharPress.code = 0;
                    }
                }
            } else if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Repeat) {
                if (lastCharPress.code != 0) {
                    putToKeyboardBuffer(lastCharPress);
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
        case uint32_t(KeyConstant::ESCAPE):
            index = fplKey_Escape;
            break;
        case uint32_t(KeyConstant::BACKSPACE):
            index = fplKey_Backspace;
            break;
        case uint32_t(KeyConstant::RETURN):
            index = fplKey_Return;
            break;
            // case uint32_t(KeyConstant::NUM_ENTER): index = fplKey_KPEnter ; break;
        case uint32_t(KeyConstant::DEL):
            index = fplKey_Delete;
            break;
        case uint32_t(KeyConstant::F1):
            index = fplKey_F1;
            break;
        case uint32_t(KeyConstant::F2):
            index = fplKey_F2;
            break;
        case uint32_t(KeyConstant::F3):
            index = fplKey_F3;
            break;
        case uint32_t(KeyConstant::F4):
            index = fplKey_F4;
            break;
        case uint32_t(KeyConstant::F5):
            index = fplKey_F5;
            break;
        case uint32_t(KeyConstant::F6):
            index = fplKey_F6;
            break;
        case uint32_t(KeyConstant::F7):
            index = fplKey_F7;
            break;
        case uint32_t(KeyConstant::F8):
            index = fplKey_F8;
            break;
        case uint32_t(KeyConstant::F9):
            index = fplKey_F9;
            break;
        case uint32_t(KeyConstant::F10):
            index = fplKey_F10;
            break;
        case uint32_t(KeyConstant::F11):
            index = fplKey_F11;
            break;
        case uint32_t(KeyConstant::F12):
            index = fplKey_F12;
            break;
        case uint32_t(KeyConstant::HOME):
            index = fplKey_Home;
            break;
        case uint32_t(KeyConstant::INSERT):
            index = fplKey_Insert;
            break;
        case uint32_t(KeyConstant::END):
            index = fplKey_End;
            break;
        case uint32_t(KeyConstant::PG_UP):
            index = fplKey_PageUp;
            break;
        case uint32_t(KeyConstant::PG_DOWN):
            index = fplKey_PageDown;
            break;
        case uint32_t(KeyConstant::CRSR_UP):
            index = fplKey_Up;
            break;
        case uint32_t(KeyConstant::CRSR_DOWN):
            index = fplKey_Down;
            break;
        case uint32_t(KeyConstant::CRSR_LEFT):
            index = fplKey_Left;
            break;
        case uint32_t(KeyConstant::CRSR_RIGHT):
            index = fplKey_Right;
            break;
        case uint32_t(KeyConstant::SCROLL):
            index = fplKey_Scroll;
            break;
        case uint32_t(KeyConstant::PAUSE):
            index = fplKey_Pause;
            break;
        case uint32_t(KeyConstant::SHIFT_LEFT):
            index = fplKey_LeftShift;
            break;
        case uint32_t(KeyConstant::SHIFT_RIGHT):
            index = fplKey_RightShift;
            break;
        default:
            break;
    }
    return (keyboardState.buttonStatesMapped[index] >= fplButtonState_Press);
}

void OsFPL::putToKeyboardBuffer(Os::KeyPress key, bool applyLimit) {
    Os::putToKeyboardBuffer(key, applyLimit);
}

std::string OsFPL::getClipboardData() {
    const size_t maxlen = 1024 * 1024;
    char* buffer = new char[maxlen];
    buffer[0] = '\0';
    std::string ret;
    if (fplGetClipboardText(buffer, maxlen + 1)) {
        ret = buffer;
    }
    delete[] buffer;
    return ret;
}

void OsFPL::setClipboardData(const std::string utf8) {
    fplSetClipboardText(utf8.c_str());
}

std::vector<char32_t> OsFPL::emojiPicker() {
    std::string emojiPanelPath = "emoji-panel";
#ifdef _WIN32
    WCHAR exepath[1024];
    if (GetModuleFileNameW(NULL, exepath, 1023) < 1023) {
        WCHAR* pEnd = wcsrchr(exepath, L'\\');
        if (pEnd != nullptr) {
            *pEnd = L'\0';
            WCHAR* pEnd = wcsrchr(exepath, L'\\');
            if (pEnd != nullptr) {
                *pEnd = L'\0';
                std::string binpath = Unicode::toUtf8String(reinterpret_cast<const char16_t*>(exepath));
                binpath += "\\3rd-party\\emoji-panel\\bin\\emoji-panel.exe";
                if (Os::doesFileExist(binpath)) {
                    emojiPanelPath = binpath;
                }
            }
        }
    }
#endif
    ScreenBuffer sb;
    ScreenBuffer::copyWithLock(sb, screen);
    std::string oldCb = this->getClipboardData();
    setClipboardData("");
    std::string cmd = std::string("\"") + emojiPanelPath + "\" --nomove --nocls --stdin";
    Os::systemCall(cmd.c_str(), true);
    std::string newCb = this->getClipboardData();
    std::vector<char32_t> chars;

    if (newCb != "") {
        const char* pu = newCb.c_str();
        for (;;) {
            char32_t c = Unicode::parseNextUtf8(pu);
            if (c == 0) { break; }
            chars.push_back(c);
        }
    }
    setClipboardData(oldCb);
    ScreenBuffer::copyWithLock(screen, sb);
    return chars;
}

#if 0  // def FPL_PLATFORM_LINUX
    #include <bluetooth/bluetooth.h>
    #include <bluetooth/hci.h>
    #include <bluetooth/hci_lib.h>
// Function to scan and connect to new gamepads
static void linuxScanAndConnectBTgamepad() {
    int device_id = hci_get_route(nullptr);
    int sock = hci_open_dev(device_id);

    if (device_id < 0 || sock < 0)
    {
        std::cerr << "Error: Unable to access Bluetooth device" << std::endl;
        return;
    }

    int max_rsp = 10;  // Max number of responses
    inquiry_info* devices = new inquiry_info[max_rsp];

    int num_responses = hci_inquiry(device_id, 8, max_rsp, nullptr, &devices, IREQ_CACHE_FLUSH);

    if (num_responses < 0)
    {
        std::cerr << "Error: Inquiry failed" << std::endl;
        delete[] devices;
        return;
    }

    for (int i = 0; i < num_responses; i++)
    {
        char addr[19] = {0};
        ba2str(&(devices[i].bdaddr), addr);
        std::cout << "Found device: " << addr << std::endl;

        // Get device name
        char name[248] = {0};
        if (hci_read_remote_name(sock, &(devices[i].bdaddr), sizeof(name), name, 0) < 0)
        {
            strcpy(name, "[unknown]");
        }
        std::cout << "Device Name: " << name << std::endl;

        // Attempt to pair, trust, and connect
        std::cout << "Pairing with " << addr << "..." << std::endl;
        system(("bluetoothctl pair " + std::string(addr)).c_str());
        system(("bluetoothctl trust " + std::string(addr)).c_str());
        system(("bluetoothctl connect " + std::string(addr)).c_str());
        std::cout << "Device " << addr << " connected!" << std::endl;
    }

    delete[] devices;
    close(sock);
}
#endif

static fplGamepadStates gamepadStates = {};
void OsFPL::updateGamepadState() {
#if 0  // def FPL_PLATFORM_LINUX // TODO: find a way to connect Bluetooth gamepads
    static auto lastTick = tick();
    auto tickNow = tick();
    auto deltaTick = tickNow - lastTick;
    if (deltaTick < 16)
    {
        delay(int(deltaTick));
        lastTick = tickNow;
        linuxScanAndConnectBTgamepad();
    }
#endif
    if (!fplPollGamepadStates(&gamepadStates)) { return; }
}

const Os::GamepadState& OsFPL::getGamepadState(int index) {
    static Os::GamepadState st = {};
    if (index < 0 || index >= FPL_MAX_GAMEPAD_STATE_COUNT) {
        st = {};
        st.connected = false;
        return st;
    }

    auto& g = gamepadStates.deviceStates[index];
    st.connected = g.isConnected;
    if (!st.connected) {
        st = {};
        st.connected = false;
        return st;
    }

    st.name = g.deviceName;
    st.analogLeft.x = g.leftStickX;
    st.analogLeft.y = g.leftStickY;
    st.analogRight.x = g.rightStickX;
    st.analogRight.y = g.rightStickY;
    size_t numButtons = sizeof(fplGamepadState::buttons) / sizeof(fplGamepadState::buttons[0]);
    if (numButtons > 32) { numButtons = 32; }
    for (size_t i = 0; i < numButtons; ++i) {
        st.buttons.set(i, g.buttons[i].isDown ? true : false);
    }
    st.dpad.x = g.dpadLeft.isDown ? -1 : (g.dpadRight.isDown ? 1 : 0);
    st.dpad.y = g.dpadUp.isDown ? -1 : (g.dpadDown.isDown ? 1 : 0);
    return st;
}
