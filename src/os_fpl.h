#pragma once

#include "os.h"
#include <mutex>
#include <condition_variable>

class OsFPL : public Os {
public:
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


    // thread buffered window update
    bool hasFocus; // window has focus/is active
    std::condition_variable cv;

    std::atomic<bool> stopThread = false;

    std::atomic<size_t> videoW = 100, videoH = 100; // size of rendering window in pixels

    struct Buffered {
        // accessing these requires the screenLock
        Buffered() {
        }

        // the front buffer is what the thread currently works on.
        // swapping happens in the thread.
        // when presenting a new screen, write to the back buffer.
        // use the screenLock to lock the back buffer.
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


        // size_t videoW = 100, videoH = 100;

        bool dirtyFlag    = false;
        bool imageCreated = false; // memBackBuffer is rendered
        bool crtEmulation = true;

        void lock() { } // { mutex.lock(); }
        void unlock() { } // { mutex.unlock(); }
        // std::mutex mutex;
    private:
    } buffer1 = {}, buffer2 = {};
    // std::mutex bufferLock; // prevent pointers from swapping

    Buffered *frontBuffer = &buffer1, *backBuffer = &buffer2;
};
