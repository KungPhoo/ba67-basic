#pragma once

#include "os.h"
#include <mutex>
#include <condition_variable>

class OsFPL : public Os {
public:
    virtual ~OsFPL();
    bool init(Basic* basic, SoundSystem* sound) override;
    uint64_t tick() const override;
    void delay(int ms) override;
    size_t getFreeMemoryInBytes() override;

    void presentScreen() override;

private:
    void renderSoftware();
    void renderOpenGL();

public:
    void updateKeyboardBuffer() override;

    const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const override;

    void putToKeyboardBuffer(Os::KeyPress key, bool applyLimit = true) override;

    std::string getClipboardData() override;
    void setClipboardData(const std::string utf8) override;
    std::vector<char32_t> emojiPicker() override;

    void updateGamepadState() override;
    const GamepadState& getGamepadState(int index) override;

    MouseStatus getMouseStatus() override;


    // thread buffered window update
    bool hasFocus; // window has focus/is active
    std::mutex screenLock, videoLock;
    std::condition_variable cv;
    std::vector<uint32_t> memBackBuffer; // the thread draws to this back buffer. It's then copied to the real one
    struct Buffered {
        ScreenBuffer screen; // this one is only accessed in the drawing thread

        bool stopThread = false;
        size_t videoW = 0, videoH = 0; // size of rendering window in pixels

        bool isCursorActive;
        bool insertMode   = false;
        bool imageCreated = false;
        bool crtEmulation = true;

        Buffered& operator=(const Buffered& b) {
            ScreenBuffer::copyWithLock(screen, b.screen);
            crtEmulation   = b.crtEmulation;
            videoW         = b.videoW;
            videoH         = b.videoH;
            stopThread     = b.stopThread;
            isCursorActive = b.isCursorActive;
            insertMode     = b.insertMode;
            imageCreated   = b.imageCreated;
            return *this;
        }

    } buffered = {};
};
