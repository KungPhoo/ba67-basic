#pragma once

#include "os.h"

class OsFPL : public Os {
public:
    OsFPL() = default;
    virtual ~OsFPL();
    bool init(Basic* basic, SoundSystem* sound) override;
    void updateEvents() override;

    uint64_t tick() const override;
    void delay(int ms) override;
    size_t getFreeMemoryInBytes() override;

    void presentScreen() override;

private:
    void renderSoftware();
    void renderOpenGL();

public:
    const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const override;


    std::string getClipboardData() override;
    void setClipboardData(const std::string utf8) override;
    std::vector<char32_t> emojiPicker() override;

    void updateGamepadState() override;
    const GamepadState& getGamepadState(int index) override;

    MouseStatus getMouseStatus() override;

    std::string getHomeDirectory() override;



    // thread buffered window update
    bool hasFocus; // window has focus/is active

    size_t videoW = 100, videoH = 100; // size of rendering window in pixels

    struct Buffered {
        // accessing these requires the screenLock
        Buffered() {
        }
        std::vector<uint32_t> memBackBuffer; // the thread draws to this back buffer. It's then copied to the real one
        std::vector<uint8_t> pixelsPal; // final screen, pixel index in color palette
        std::vector<uint32_t> palette; // AABBGGRR little endian format
        size_t pixelsW = 0, pixelsH = 0; // width/height number of pixels in pixelsPal
        uint8_t borderColorIndex = 0;

        // pixel size of borders - filled in thread
        struct WindowPixels {
            int borderx = 0, bordery = 0; // pixels of borders
            int pixelscalex = 1, pixelscaley = 1; // number of screen pixels for one BA67 pixel
        } windowPixels = {};

        bool dirtyFlag    = false;
        bool crtEmulation = true;

    private:
    } buffer = {};

    Buffered* backBuffer = &buffer;
};
