#if defined(__EMSCRIPTEN__)

    #include "os_sdl2.h"
    #include <vector>
    #include <queue>
    #include <array>
    #include <bitset>
    #include <string>
    #include <chrono>
    #include <cstring>
    #include <iostream>
    #include <emscripten.h>
    #include "unicode.h"
    #include <filesystem>
    #include <emscripten.h>
    #include <emscripten/html5.h>

static void dummy_main_loop() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            // you can stop the loop if desired
    #ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
    #endif
        }
    }
    // This runs every browser frame, but doesn't block.
}

void start_os_sdl2() {
    #ifdef __EMSCRIPTEN__
    // Register a dummy loop so SDL/Emscripten is happy.
    emscripten_set_main_loop(dummy_main_loop, 0, 0);
    #else
        // On desktop, you don't need it.
    #endif
}


bool OsSDL2::init(Basic* basic, SoundSystem* sound) {
    start_os_sdl2();
    Os::init(basic, sound);


    #ifdef __EMSCRIPTEN__


    // Create a mount point
    EM_ASM(

        FS.mkdir('/home/web_user/BASIC');
        FS.mount(IDBFS, {}, '/home/web_user/BASIC');

        // Load previously saved files (async)
        FS.syncfs(true, function(err) { console.error("Error loading persistent data:", err); });

    );

    std::string home = "/home/web_user";
    if (doesFileExist(home) && isDirectory(home)) {
        setCurrentDirectory(home);
        home += "/BASIC";
        if (!doesFileExist(home)) {
            std::filesystem::create_directory("BASIC");
        }
        setCurrentDirectory(home);
    }
    #endif


    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_StartTextInput(); // sends SDL_TEXTINPUT messages

    window = SDL_CreateWindow(
        "BA68 - BASIC", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        int(screen.width) * ScreenInfo::charPixX, int(screen.height) * ScreenInfo::charPixY,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        return false;
    }

    txW     = 0;
    txH     = 0;
    texture = nullptr;

    return true;
}

// --- TIME ---
uint64_t OsSDL2::tick() const {
    return SDL_GetTicks64(); // ms
}

void OsSDL2::delay(int ms) {
    SDL_Delay(ms);
}

// --- SCREEN ---
void OsSDL2::presentScreen() {
    // TODO in WASM, this function takes about 20 ms on my PC
    uint64_t now             = tick();
    static uint64_t nextShow = 0;
    if (nextShow > now) {
        return;
    }
    #ifdef __EMSCRIPTEN__
    nextShow = now + 32;
    #else
    nextShow = now + 12;
    #endif

    if (texture == nullptr || txW != int(screen.width) * ScreenInfo::charPixX || txH != int(screen.height) * ScreenInfo::charPixY) {
        txW = int(screen.width) * ScreenInfo::charPixX;
        txH = int(screen.height) * ScreenInfo::charPixY;
        // std::cout << "resize screen to " << txW << " x " << txH << "\n";
        if (txW == 0 || txH == 0) {
            return;
        }

        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
        texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            txW,
            txH);
    }

    bool cursorVisible = screen.isCursorActive();
    if (cursorVisible && (tick() % 800) < 400) {
        cursorVisible = false;
    }
    screen.updateScreenPixelsPalette(cursorVisible);
    screen.updateScreenBitmap();
    SDL_UpdateTexture(texture, nullptr, screen.screenBitmap.pixelsRGB.data(), ScreenInfo::pixX * 4 /*bytes per row*/);

    #ifdef __EMSCRIPTEN__
    static int oldW = 0, oldH = 0;
    int w, h;
    emscripten_get_canvas_element_size("canvas", &w, &h);
    if (w != oldW || h != oldH) {
        oldW = w;
        oldH = h;
        SDL_SetWindowSize(window, w, h);
    }
    #endif

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"); // or "nearest", "linear", "best"
    SDL_RenderSetLogicalSize(renderer, txW, txH);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

    // --- KEYBOARD ---
    #if defined(__EMSCRIPTEN__)
extern "C" {
EMSCRIPTEN_KEEPALIVE
void ba67_push_sdl_textinput(const char* txt, bool withShift, bool withAlt, bool withCtrl) {
    static SDL_Event event;

    if (!withAlt && !withCtrl) {
        event.type = SDL_TEXTINPUT;
        strncpy(event.text.text, txt, SDL_TEXTINPUTEVENT_TEXT_SIZE - 1);
        event.text.text[SDL_TEXTINPUTEVENT_TEXT_SIZE - 1] = '\0';
    } else { // Alt or Ctrl
        event.type           = SDL_KEYDOWN;
        event.key.keysym.sym = OsSDL2::SDLKeyFromIndex(Unicode::parseNextUtf8(txt));
        event.key.keysym.mod = (withShift ? KMOD_SHIFT : 0)
                             | (withAlt ? KMOD_ALT : 0)
                             | (withCtrl ? KMOD_CTRL : 0);
    }
    SDL_PushEvent(&event);
}
}
    #endif

const bool OsSDL2::isKeyPressed(uint32_t index, bool withShift, bool withAlt, bool withCtrl) const {
    auto peek = Os::peekKeyboardBuffer();
    if (peek.code == index && peek.holdAlt == withAlt && peek.holdShift == withShift && peek.holdCtrl == withCtrl) {
        return true;
    }

    const Uint8* state = SDL_GetKeyboardState(nullptr);
    SDL_Keycode key    = SDLKeyFromIndex(index);
    SDL_Scancode sc    = SDL_GetScancodeFromKey(key);
    bool pressed       = state[sc];

    if (pressed && (withShift || withAlt || withCtrl)) {
        SDL_Keymod mods = SDL_GetModState();
        if (withShift && !(mods & KMOD_SHIFT))
            return false;
        if (withAlt && !(mods & KMOD_ALT))
            return false;
        if (withCtrl && !(mods & KMOD_CTRL))
            return false;
    }
    return pressed;
}

void OsSDL2::updateEvents() {
    // keep the cursor alive
    presentScreen();


    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        } else if (event.type == SDL_TEXTINPUT) {
            KeyPress k;
            k.printable     = true;
            const char* str = event.text.text;
            while (str != nullptr && *str != '\0') {
                k.code = Unicode::parseNextUtf8(str);
                putToKeyboardBuffer(k);
            }
        } else if (event.type == SDL_KEYDOWN) {
            KeyPress k;
            codepointFromSDLKey(event.key.keysym, k);
            auto mods   = event.key.keysym.mod;
            k.holdShift = (mods & KMOD_SHIFT) != 0;
            k.holdAlt   = (mods & KMOD_ALT) != 0;
            k.holdCtrl  = (mods & KMOD_CTRL) != 0;

            // only for special keys. Alt+1 / Crsr / Return
            if ((k.holdAlt || k.holdCtrl) || k.code != 0) {
                if (k.code == 0) {
                    k.code = uint32_t(event.key.keysym.sym);
                }
                putToKeyboardBuffer(k);
            }
        }
    }
}

SDL_Keycode OsSDL2::SDLKeyFromIndex(uint32_t idx) {
    // direct mapping for ASCII
    if (idx < 256) {
        return SDL_Keycode(idx);
    }
    switch (idx) {
    case uint32_t(KeyConstant::ESCAPE):     return SDLK_ESCAPE;
    case uint32_t(KeyConstant::CRSR_UP):    return SDLK_UP;
    case uint32_t(KeyConstant::CRSR_DOWN):  return SDLK_DOWN;
    case uint32_t(KeyConstant::CRSR_LEFT):  return SDLK_LEFT;
    case uint32_t(KeyConstant::CRSR_RIGHT): return SDLK_RIGHT;
    default:                                return SDLK_UNKNOWN;
    }
}

// --- CLIPBOARD ---
std::string OsSDL2::getClipboardData() {
    if (SDL_HasClipboardText()) {
        char* txt = SDL_GetClipboardText();
        std::string s(txt);
        SDL_free(txt);
        return s;
    }
    return {};
}

void OsSDL2::setClipboardData(const std::string utf8) {
    SDL_SetClipboardText(utf8.c_str());
}

// --- MOUSE ---
Os::MouseStatus OsSDL2::getMouseStatus() {
    MouseStatus m {};
    int x, y;
    uint32_t btn = SDL_GetMouseState(&x, &y);
    m.x          = x;
    m.y          = y;
    m.buttonBits = uint8_t(btn & 0xFF);
    return m;
}


// captures all keys, that are not sent via SDL_TEXTINPUT
void OsSDL2::codepointFromSDLKey(const SDL_Keysym& keysym, Os::KeyPress& k) {
    k.code      = 0; // all others are handled with SDL_TEXTINPUT
    k.printable = false;
    switch (keysym.sym) {
    case SDLK_ESCAPE: k.code = uint32_t(KeyConstant::ESCAPE); break;
    case SDLK_F1:     k.code = uint32_t(KeyConstant::F1); break;
    case SDLK_F2:     k.code = uint32_t(KeyConstant::F2); break;
    case SDLK_F3:     k.code = uint32_t(KeyConstant::F3); break;
    case SDLK_F4:     k.code = uint32_t(KeyConstant::F4); break;
    case SDLK_F5:     k.code = uint32_t(KeyConstant::F5); break;
    case SDLK_F6:     k.code = uint32_t(KeyConstant::F6); break;
    case SDLK_F7:     k.code = uint32_t(KeyConstant::F7); break;
    case SDLK_F8:     k.code = uint32_t(KeyConstant::F8); break;
    case SDLK_F9:     k.code = uint32_t(KeyConstant::F9); break;
    case SDLK_F10:    k.code = uint32_t(KeyConstant::F10); break;
    case SDLK_F11:    k.code = uint32_t(KeyConstant::F11); break;
    case SDLK_F12:    k.code = uint32_t(KeyConstant::F12); break;

    case SDLK_HOME:   k.code = uint32_t(KeyConstant::HOME); break;
    case SDLK_END:    k.code = uint32_t(KeyConstant::END); break;
    case SDLK_UP:     k.code = uint32_t(KeyConstant::CRSR_UP); break;
    case SDLK_DOWN:   k.code = uint32_t(KeyConstant::CRSR_DOWN); break;
    case SDLK_LEFT:   k.code = uint32_t(KeyConstant::CRSR_LEFT); break;
    case SDLK_RIGHT:  k.code = uint32_t(KeyConstant::CRSR_RIGHT); break;
    case SDLK_INSERT: k.code = uint32_t(KeyConstant::INSERT); break;
    case SDLK_DELETE: k.code = uint32_t(KeyConstant::DEL); break;
    case SDLK_KP_BACKSPACE:
    case SDLK_BACKSPACE:
        k.code = uint32_t(KeyConstant::BACKSPACE);
        break;

    case SDLK_RETURN:
    case SDLK_RETURN2:
        k.code      = uint32_t(KeyConstant::RETURN);
        k.printable = true;
        break;
    default:
        break;
    }
}

#endif // __EMSCRIPTEN__
