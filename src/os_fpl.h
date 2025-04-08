#pragma once

#include "os.h"
#include <mutex>

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

    // thread buffered window update
    std::mutex screenLock, videoLock;
    std::vector<uint32_t> pixelsVideo;
    struct Buffered {
        ScreenBuffer screen; // this one is only accessed in the drawing thread

        bool stopThread = false;
        //        std::vector<uint8_t> pixelsPal;
        size_t videoW = 0, videoH = 0;

        // std::array<uint32_t, 16> palette;
        // uint8_t crsrColor = 1;
        // uint8_t borderColor = 0;
        // ScreenBuffer::Cursor crsrPos;
        bool isCursorActive;
        bool imageCreated = false;
        bool crtEmulation = true;

        Buffered& operator=(const Buffered& b) {
            ScreenBuffer::copyWithLock(screen, b.screen);
            crtEmulation   = b.crtEmulation;
            videoW         = b.videoW;
            videoH         = b.videoH;
            stopThread     = b.stopThread;
            isCursorActive = b.isCursorActive;
            imageCreated   = b.imageCreated;
            return *this;
        }

    } buffered = {};
};
