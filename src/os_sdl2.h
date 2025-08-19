#pragma once
#if defined(__EMSCRIPTEN__)


    #include <SDL.h>
    #include "os.h"

class OsSDL2 : public Os {
public:
    OsSDL2() = default;
    ~OsSDL2() override {
        SDL_Quit();
    }

    bool init(Basic* basic, SoundSystem* sound) override;
    void updateEvents() override;

    // --- TIME ---
    uint64_t tick() const override;

    void delay(int ms) override;

    // --- SCREEN ---
    void presentScreen() override;

    // --- KEYBOARD ---
    const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const override;
    static SDL_Keycode SDLKeyFromIndex(uint32_t idx);

    // --- CLIPBOARD ---
    std::string getClipboardData() override;

    void setClipboardData(const std::string utf8) override;

    // --- MOUSE ---
    Os::MouseStatus getMouseStatus() override;

private:
    SDL_Window* window     = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture   = nullptr;
    bool running           = true;
    int txW, txH; // current texture size

    static void codepointFromSDLKey(const SDL_Keysym& keysym, Os::KeyPress& k);
};
#endif // __EMSCRIPTEN__
