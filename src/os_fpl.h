#pragma once

#include "os.h"
#include <mutex>

class OsFPL: public Os {
public:
    virtual ~OsFPL();
    bool init(Basic* basic, SoundSystem* sound) override;
    uint64_t tick() const override;
    void delay(int ms)const override;
    size_t getFreeMemoryInBytes() override;

    void presentScreen() override;

    void updateKeyboardBuffer() override;

    const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const override;

    void putToKeyboardBuffer(Os::KeyPress key) override;



    std::mutex screenLock;
    bool dirtyFlag = true;
    std::vector<uint32_t> pixelsVideo;
    struct Buffered {
        bool stopThread = false;
        std::vector<uint8_t> pixelsPal;
        size_t videoW = 0, videoH = 0;


        std::array<uint32_t, 16> palette;
        uint8_t crsrColor = 1;
        uint8_t borderColor = 0;
        ScreenBuffer::Cursor crsrPos;
        bool isCursorActive;
        bool imageCreated = false;
    }buffered = {};
};
