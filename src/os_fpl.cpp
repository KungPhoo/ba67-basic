#ifndef __EMSCRIPTEN__
    #include "os_fpl.h"

    // https://libfpl.org/docs/page_categories.html
    #define FPL_IMPLEMENTATION
    #define FPL_NO_AUDIO
    #if defined(_DEBUG)
        #define FPL_LOGGING
    #endif

    #include "basic.h"
    #include "final_platform_layer.h"
    #include "unicode.h"
    #include <array>
    #include <cmath>
    // #include <cstring>
    #include <filesystem>
    #include <chrono>
    #include "string_helper.h"

    #if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)
        #include "opengl_painter.h"
    #endif

    #ifdef _WIN32
        #include "../resources/resource.h"
        #include <Windows.h>
        #include <tchar.h>
    #endif


OsFPL::~OsFPL() {
    fplWindowShutdown();
}

bool OsFPL::init(Basic* basic, SoundSystem* sound) {
    Os::init(basic, sound);

    fplSettings settings;
    fplSetDefaultSettings(&settings);

    const int border            = 64;
    settings.window.isResizable = true;
    settings.window.windowSize  = { 640 + 2 * border, 400 + 2 * border };
    StringHelper::strcpy(settings.window.title, "BA67 BASIC");
    settings.window.fullscreenRefreshRate = 60;
    settings.video.isAutoSize             = false; // we resize ourself

    settings.window.isFullscreen            = false;
    settings.window.isScreenSaverPrevented  = true;
    settings.window.isMonitorPowerPrevented = true;

    settings.input.controllerDetectionFrequency = 5000;

    #if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)
    if (this->settings.renderMode == BA68settings::RenderMode::OpenGL) {
        // Use Legacy OpenGL (1.1)
        settings.video.backend = fplVideoBackendType_OpenGL;
        // settings.video.graphics.opengl.compabilityFlags = fplOpenGLCompabilityFlags_Legacy;
        settings.video.graphics.opengl.compabilityFlags = fplOpenGLCompabilityFlags_Core;
        settings.video.graphics.opengl.majorVersion     = 3;
        settings.video.graphics.opengl.minorVersion     = 3;
        settings.video.isVSync                          = true;
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
    #ifdef _DEBUG
            fplInitFlags_Console |
    #endif
                // fplInitFlags_Audio |
                fplInitFlags_Window | fplInitFlags_Video | fplInitFlags_GameController,
            &settings)) {
        return false;
    }

    if (this->settings.fullscreen) {
        // Enable fullscreen on the nearest desktop
        fplSetWindowFullscreenSize(true, 0, 0, 0);
    }


    #ifdef _WIN32
    fpl__PlatformAppState* appState    = fpl__global__AppState;
    fpl__Win32AppState* win32AppState  = &appState->win32;
    fpl__Win32WindowState* windowState = &appState->window.win32;
    HWND hWnd                          = windowState->windowHandle;
    HINSTANCE hInstance                = GetModuleHandle(NULL);
    HICON hIcon                        = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
    SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    #endif

    return true;
}

std::string OsFPL::getHomeDirectory() {
    std::string homeDir = ".";
    #ifdef _WIN32
    wchar_t* user = nullptr;
    size_t len    = 0;
    if (0 == _wdupenv_s(&user, &len, L"USERPROFILE") && user != nullptr) {
        std::wstring home = user;
        free(user);
        user = nullptr;
        home += L"\\Documents";
        ::CreateDirectoryW(home.c_str(), NULL);
        home += L"\\BASIC";
        ::CreateDirectoryW(home.c_str(), NULL);
        homeDir = Unicode::toUtf8String((const char16_t*)home.c_str());
        ;
        // ::SetCurrentDirectoryW(home.c_str());
    }
    #else
    char homepath[1024] = "~";
    fplGetHomePath(homepath, 1024);
    std::string home = homepath;
    for (auto& c : home) {
        if (c == '\\') {
            c = '/';
        }
    }

    if (doesFileExist(home) && isDirectory(home)) {
        setCurrentDirectory(home);
        home += "/BASIC";
        if (!doesFileExist(home)) {
            std::filesystem::create_directory("BASIC");
        }
        homeDir = home;
    }
    #endif

    return homeDir;
}

uint64_t OsFPL::tick() const {
    static auto start = fplMillisecondsQuery();
    return fplMillisecondsQuery() - start;
}

void OsFPL::delay(int ms) {
    if (ms > 100) {
        auto endTick = tick() + ms;
        while (ms > 100) {
            presentScreen();
            ms -= 100;
            fplThreadSleep(98);
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
    fInput      = fopen("/proc/meminfo", "r");
    if (fInput != NULL) {
        while (!feof(fInput)) {
            fgets(buffer, BUFFER_SIZE - 1, fInput);
            if (feof(fInput)) {
                break;
            }
            buffer[BUFFER_SIZE - 1] = 0;
            // Look for serial number
            if (StringHelper::strncmp(buffer, "MemFree:", 8) == 0) {
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

    const double brightness = 60; // -200..200
    const double contrast   = 1.5; // 0..1

    // https://ie.nitk.ac.in/blog/2020/01/19/algorithms-for-adjusting-brightness-and-contrast-of-an-image/
    auto truncate = [](double f) {if (f > 255.0) { return 255.0; } if (f < 0.0) { return 0.0; } return f; };
    double factor = (259.0f * (contrast + 255.0)) / (255.0 * (259.0 - contrast));
    double new_r  = truncate((((r - 128.0) * contrast) + 128 + brightness) * facR * facDark);
    double new_g  = truncate((((g - 128.0) * contrast) + 128 + brightness) * facG * facDark);
    double new_b  = truncate((((b - 128.0) * contrast) + 128 + brightness) * facB * facDark);

    r = (uint8_t)new_r;
    g = (uint8_t)new_g;
    b = (uint8_t)new_b;

    return (a << 24) | (b << 16) | (g << 8) | r;
}


// == DRAW ==
static void renderToFrontBuffer(OsFPL* fpl, OsFPL::Buffered* buffer) {
    std::array<std::array<uint32_t, 16>, 6> palettes; // [r,g,b, dark r,g,b]


    size_t videoW   = fpl->videoW;
    size_t videoH   = fpl->videoH;
    size_t videoWxH = videoH * videoW;
    if (videoWxH != buffer->memBackBuffer.size()) {
        buffer->memBackBuffer.resize(videoWxH);
    }
    if (buffer->memBackBuffer.empty()) {
        fpl->delay(1000);
        return;
    }


    // simulate a CRT TV with a 3x3 pixel matrix
    #ifdef BA67_GRAPHICS_CRT_EMULATION_ON
    for (size_t p = 0; p < 6; ++p) {
        const double facDark     = 0.7; // factor for darker scanlines
        const double facNeigbour = 0.6, facNeighbour2 = 0.6; // factor for neighboured r,g,b channels. On true CRT screen, that would be 0.0
        double r = 1.0, g = 1.0, b = 1.0, darken = 1.0; // factor for r,g,b channels
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
        if (p > 2) {
            darken = facDark;
        }
        for (size_t i = 0; i < 16; ++i) {
            if (buffer->crtEmulation) {
                palettes[p][i] = emphasizeRGB(buffer->palette[i], r, g, b, darken);
            } else {
                palettes[p][i] = buffer->palette[i];
            }
        }
    }
    #else
    for (size_t p = 0; p < 6; ++p) {
        for (size_t i = 0; i < 16; ++i) {
            palettes[p][i] = buffer->palette[i];
        }
    }
    #endif

    // Compute scaling factors
    double scaleX = static_cast<double>(videoW) / buffer->pixelsW;
    double scaleY = static_cast<double>(videoH) / buffer->pixelsH;

    if (buffer->pixelsW / buffer->pixelsH == 1 /*320x200*/) {
        double scale = std::max(0.25, std::min(scaleX, scaleY)); // Keep aspect ratio
        scaleX = scaleY = scale;
    } else { // 640x200
        double scale = std::max(0.25, std::min(scaleX, scaleY / 2.0)); // Keep aspect ratio
        scaleX       = scale;
        scaleY       = scale * 2;
    }

    // scale to full pixels
    if (scaleX > 1.0) {
        scaleX = floor(scaleX);
    }
    if (scaleY > 1.0) {
        scaleY = floor(scaleY);
    }

    // Compute offset for centered output
    size_t scaledWidth  = static_cast<size_t>(buffer->pixelsW * scaleX);
    size_t scaledHeight = static_cast<size_t>(buffer->pixelsH * scaleY);
    size_t offsetX      = (videoW - scaledWidth) / 2;
    size_t offsetY      = (videoH - scaledHeight) / 2;


    auto& wipx       = buffer->windowPixels;
    wipx.borderx     = int(offsetX);
    wipx.bordery     = int(offsetY);
    wipx.pixelscalex = int(scaleX);
    wipx.pixelscaley = int(scaleY);


    // Nearest-neighbor scaling loop
    #pragma omp parallel for
    for (int y = 0; y < int(videoH); ++y) {
        uint32_t* pdest = &buffer->memBackBuffer[0] + y * videoW;

        size_t srcY = std::min(static_cast<size_t>((y - offsetY) / scaleY), buffer->pixelsH - 1);
        for (size_t x = 0; x < videoW; ++x) {
            // Map screen coordinates to original ScreenBitmap using nearest-neighbor
            size_t srcX = std::min(static_cast<size_t>((x - offsetX) / scaleX), buffer->pixelsW - 1);

            // Default background if outside scaled region
            uint8_t color = buffer->borderColorIndex; // Transparent or black

            // Only sample from ScreenBitmap if inside scaled bounds
            if (x >= offsetX && x < offsetX + scaledWidth && y >= offsetY && y < offsetY + scaledHeight) {
                color = buffer->pixelsPal[srcY * buffer->pixelsW + srcX];
            }

            // buffer->pixels[y * buffer->width + x] = color;
            size_t pal = (x % 3);
            if ((y % 3) == 2) {
                pal += 3;
            } // darker row
            *pdest++ = palettes[pal][color];
        }
    }
}

void OsFPL::presentScreen() {

    uint64_t now             = tick();
    static uint64_t nextShow = 0;
    if (nextShow > now) {
        return;
    }
    nextShow = now + 5;

    static bool lastCursorBlink = false;
    bool cursorVisible          = true;
    if ((tick() % 800) < 400) {
        cursorVisible = false;
    }
    if (lastCursorBlink != cursorVisible) {
        screen.dirtyFlag = true;
        lastCursorBlink  = cursorVisible;
    }

    if (screen.dirtyFlag) {
        screen.dirtyFlag = false;

        // Get window size
        fplWindowSize winSize = {};
        if (!fplGetWindowSize(&winSize)) {
            printf("can't get window size\n");
            return;
        }
        // it seems fplWindowEventType_Closed is never fired
        if (winSize.width == 0) {
            return; // window minimized
        }
        videoW = winSize.width;
        videoH = winSize.height;

        backBuffer->palette.resize(16);
        for (size_t i = 0; i < 16; ++i) {
            backBuffer->palette[i] = screen.palette[i];
        }
        screen.updateScreenPixelsPalette(cursorVisible, backBuffer->pixelsPal);

        switch (settings.renderMode) {
        case BA68settings::RenderMode::OpenGL:
    #if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)
            renderOpenGL();
            break;
    #endif
        case BA68settings::RenderMode::Software:
            renderSoftware();
            break;
        }
    }
}


void OsFPL::renderSoftware() {
    backBuffer->pixelsH          = screen.height * ScreenInfo::charPixX;
    backBuffer->pixelsW          = screen.width * ScreenInfo::charPixY;
    backBuffer->borderColorIndex = screen.getBorderColor();
    backBuffer->dirtyFlag        = true;
    backBuffer->crtEmulation     = settings.emulateCRT;

    renderToFrontBuffer(this, backBuffer);

    fplVideoBackBuffer* vidBackBuffer = fplGetVideoBackBuffer();
    if (vidBackBuffer != nullptr) {
        if (vidBackBuffer->width != videoW || vidBackBuffer->height != videoH) {
            screen.dirtyFlag = true;
            fplResizeVideoBackBuffer(uint32_t(videoW), uint32_t(videoH));
            vidBackBuffer = fplGetVideoBackBuffer();
        }
    }

    if (vidBackBuffer != nullptr && vidBackBuffer->pixels != nullptr && vidBackBuffer->width * vidBackBuffer->height == backBuffer->memBackBuffer.size()) {
        StringHelper::memcpy(vidBackBuffer->pixels, &backBuffer->memBackBuffer[0], sizeof(uint32_t) * backBuffer->memBackBuffer.size());
    }
    fplVideoFlip();
}

void OsFPL::renderOpenGL() {
    #if defined(BA67_GRAPHICS_ENABLE_OPENGL_ON)

    static OpenGLPainter ogl {};
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        ogl.initGL();
    }
    backBuffer->memBackBuffer.resize(backBuffer->pixelsPal.size());
    screen.updateScreenBitmap(backBuffer->pixelsPal, backBuffer->memBackBuffer);

    ogl.draw(videoW, videoH,
             screen.width * ScreenInfo::charPixX,
             screen.height * ScreenInfo::charPixY,
             &backBuffer->memBackBuffer[0],
             screen.palette[screen.getBorderColor()]);

    fplVideoFlip();
    #else
    renderSoftware();
    #endif
}

void OsFPL::updateEvents() {
    // static auto lastTick = tick();
    // auto tickNow         = tick();
    // auto deltaTick       = tickNow - lastTick;
    // if (deltaTick < 16) {
    //     delay(int(deltaTick));
    //     lastTick = tickNow;
    // }

    // we don't get a close event (bug)
    if (!fplIsWindowRunning()) {
        exit(0);
    }


    static KeyPress lastCharPress = {};
    static char32_t lastAltKey    = -1; // HACK I get a button event for Alt+A, and then an input event A.

    bool forceWindowUpdate = false;
    fplEvent event;
    while (fplPollEvent(&event)) {
        if (event.type == fplEventType_Window) {

            fplWindowSize sz { 0, 0 };
            if (event.window.type == fplWindowEventType_Closed) {
                exit(0);
            } else if (event.window.type == fplWindowEventType_GotFocus) {
                hasFocus          = true;
                forceWindowUpdate = true;
            } else if (event.window.type == fplWindowEventType_LostFocus) {
                hasFocus = false;
            } else if (event.window.type == fplWindowEventType_Restored || event.window.type == fplWindowEventType_Maximized) {
                if (sz.width > 0) {
                    fplResizeVideoBackBuffer(sz.width, sz.height);
                    forceWindowUpdate = true;
                }
            } else {
                int pause = 0;
            }
        } else if (event.type == fplEventType_Keyboard) {
            KeyPress keyPress;
            keyPress.holdShift = event.keyboard.modifiers & (fplKeyboardModifierFlags_LShift | fplKeyboardModifierFlags_RShift);
            keyPress.holdCtrl  = event.keyboard.modifiers & (fplKeyboardModifierFlags_LCtrl | fplKeyboardModifierFlags_RCtrl);
            keyPress.holdAlt   = event.keyboard.modifiers & fplKeyboardModifierFlags_LAlt;
            keyPress.holdAltGr = event.keyboard.modifiers & fplKeyboardModifierFlags_RAlt;
            keyPress.code      = uint32_t(event.keyboard.keyCode); // has umlaut characters

            //
            if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Release) {
                lastCharPress.code = 0;
            }

            if (event.keyboard.type == fplKeyboardEventType_Input) {
                if (keyPress.code == 0x7f) { // DEL (only sent on Linux)
                    lastCharPress.code = 0;
                    continue;
                }

                keyPress.printable = true;
                if (keyPress.code != lastAltKey && keyPress.code - 'a' + 'A' != lastAltKey) {
                    putToKeyboardBuffer(keyPress);
                }
                lastAltKey    = -1;
                lastCharPress = keyPress;


            } else if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Press) {
                // keyPress.code = 0;
                keyPress.printable = false;
                keyPress.code      = uint32_t(event.keyboard.mappedKey); // here, the keyCode on Linux is just wrong

                bool repeatable = false;
                // these provide character input
                // case fplKey_Backspace:keyPress.code = uint32_t(KeyConstant::BACKSPACE); break;
                // case fplKey_Return:   keyPress.code = uint32_t(KeyConstant::RETURN); break;
                // case fplKey_KPEnter:  yPress.code = uint32_t(KeyConstant::NUM_ENTER); break;
                // case fplKey_Escape:   keyPress.code = uint32_t(KeyConstant::ESCAPE); break;
                switch (event.keyboard.mappedKey) {
                case fplKey_Delete:
                    keyPress.code = uint32_t(KeyConstant::DEL); // Linux sends this AND input 0x7f. Windows just this.
                    repeatable    = true;
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
                    repeatable    = true;
                    break;
                case fplKey_End:
                    keyPress.code = uint32_t(KeyConstant::END);
                    break;
                case fplKey_PageUp:
                    keyPress.code = uint32_t(KeyConstant::PG_UP);
                    repeatable    = true;
                    break;
                case fplKey_PageDown:
                    keyPress.code = uint32_t(KeyConstant::PG_DOWN);
                    repeatable    = true;
                    break;
                case fplKey_Up:
                    keyPress.code = uint32_t(KeyConstant::CRSR_UP);
                    repeatable    = true;
                    break;
                case fplKey_Down:
                    keyPress.code = uint32_t(KeyConstant::CRSR_DOWN);
                    repeatable    = true;
                    break;
                case fplKey_Left:
                    keyPress.code = uint32_t(KeyConstant::CRSR_LEFT);
                    repeatable    = true;
                    break;
                case fplKey_Right:
                    keyPress.code = uint32_t(KeyConstant::CRSR_RIGHT);
                    repeatable    = true;
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
                    // printf("press mapped key $%x key code: $%x shift %c alt %c ctrl %c \n", int(event.keyboard.mappedKey), int(event.keyboard.keyCode), keyPress.holdShift ? 'X' : 'O', keyPress.holdAlt ? 'X' : 'O', keyPress.holdCtrl ? 'X' : 'O');
                    putToKeyboardBuffer(keyPress);

                    if (keyPress.holdAlt) {
                        lastAltKey = keyPress.code;
                    }

                    if (repeatable) {
                        lastCharPress = keyPress;
                    } else {
                        lastCharPress.code = 0;
                    }
                }
            } else if (event.keyboard.type == fplKeyboardEventType_Button && event.keyboard.buttonState == fplButtonState_Repeat) {
                if (lastCharPress.code != 0
                    && event.keyboard.mappedKey != fplKey_Shift
                    && event.keyboard.mappedKey != fplKey_Alt) {
                    putToKeyboardBuffer(lastCharPress);
                }
            }
        } else {
            // no event when the window was closed in Windows. (FPL bug)
        }
    }

    if (forceWindowUpdate) {
        screen.dirtyFlag = true;
        presentScreen();
    }

    updateGamepadState();
}

const bool OsFPL::isKeyPressed(uint32_t index, bool withShift, bool withAlt, bool withCtrl) const {
    static fplKeyboardState keyboardState = {};

    if (!hasFocus) {
        return false;
    }

    static auto lastTick = tick();
    auto tickNow         = tick();
    if (tickNow - lastTick >= 16) {
        lastTick = tickNow;
        if (!fplPollKeyboardState(&keyboardState)) {
            return false;
        }
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

std::string OsFPL::getClipboardData() {
    const size_t maxlen = 1024 * 1024;
    char* buffer        = new char[maxlen];
    buffer[0]           = '\0';
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
            *pEnd       = L'\0';
            WCHAR* pEnd = wcsrchr(exepath, L'\\');
            if (pEnd != nullptr) {
                *pEnd               = L'\0';
                std::string binpath = Unicode::toUtf8String(reinterpret_cast<const char16_t*>(exepath));
                binpath += "\\3rd-party\\emoji-panel\\bin\\emoji-panel.exe";
                if (Os::doesFileExist(binpath)) {
                    emojiPanelPath = binpath;
                }
            }
        }
    }
    #endif

    auto oldState     = screen.saveState();
    std::string oldCb = this->getClipboardData();
    setClipboardData("");
    std::string cmd = std::string("\"") + emojiPanelPath + "\" --nomove --nocls --ascii";
    Os::systemCall(cmd.c_str(), true);
    std::string newCb = this->getClipboardData();
    std::vector<char32_t> chars;

    if (newCb != "") {
        const char* pu = newCb.c_str();
        for (;;) {
            char32_t c = Unicode::parseNextUtf8(pu);
            if (c == 0) {
                break;
            }
            chars.push_back(c);
        }
    }
    setClipboardData(oldCb);
    screen.restoreState(oldState);
    return chars;
}

    #if 0 // def FPL_PLATFORM_LINUX
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
    static auto lastTick = tick();
    auto tickNow         = tick();
    if (tickNow - lastTick < 16) {
        return;
    }
    lastTick = tickNow;


    #if 0 // def FPL_PLATFORM_LINUX // TODO: find a way to connect Bluetooth gamepads
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
    if (!fplPollGamepadStates(&gamepadStates)) {
        return;
    }
}

const Os::GamepadState& OsFPL::getGamepadState(int index) {
    static Os::GamepadState st = {};
    if (index < 0 || index >= FPL_MAX_GAMEPAD_STATE_COUNT) {
        st           = {};
        st.connected = false;
        return st;
    }

    auto& g      = gamepadStates.deviceStates[index];
    st.connected = g.isConnected;
    if (!st.connected) {
        st           = {};
        st.connected = false;
        return st;
    }

    st.name           = g.deviceName;
    st.analogLeft.x   = g.leftStickX;
    st.analogLeft.y   = g.leftStickY;
    st.analogRight.x  = g.rightStickX;
    st.analogRight.y  = g.rightStickY;
    size_t numButtons = sizeof(fplGamepadState::buttons) / sizeof(fplGamepadState::buttons[0]);
    if (numButtons > 32) {
        numButtons = 32;
    }
    for (size_t i = 0; i < numButtons; ++i) {
        st.buttons.set(i, g.buttons[i].isDown ? true : false);
    }
    st.dpad.x = g.dpadLeft.isDown ? -1 : (g.dpadRight.isDown ? 1 : 0);
    st.dpad.y = g.dpadUp.isDown ? -1 : (g.dpadDown.isDown ? 1 : 0);
    return st;
}

Os::MouseStatus OsFPL::getMouseStatus() {
    static Os::MouseStatus st = {};

    fplMouseState fst;
    if (fplPollMouseState(&fst)) {
        auto wpx = backBuffer->windowPixels;
        if (wpx.pixelscalex == 0) {
            wpx.pixelscalex = 1;
        }
        if (wpx.pixelscaley == 0) {
            wpx.pixelscaley = 1;
        }
        st.x          = (fst.x - wpx.borderx) / wpx.pixelscalex + 25;
        st.y          = (fst.y - wpx.bordery) / wpx.pixelscaley + 50;
        st.buttonBits = (fst.buttonStates[0] == fplButtonState_Release ? 0 : 1)
                      + (fst.buttonStates[1] == fplButtonState_Release ? 0 : 2)
                      + (fst.buttonStates[2] == fplButtonState_Release ? 0 : 4);
    }
    return st;
}
#endif // !__EMSCRIPTEN__