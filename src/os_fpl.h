#pragma once

#include "os.h"

class OsFPL: public Os {
public:
    bool init(Basic* basic, SoundSystem* sound) override;
    uint64_t tick() const override;
    void delay(int ms)const override;
    size_t getFreeMemoryInBytes() override;

    void presentScreen() override;

    void updateKeyboardBuffer() override;

    const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const override;

    void putToKeyboardBuffer(Os::KeyPress key) override;

    bool cursorVisible = true;
    uint64_t nextShowCursor = 0; // blink time

};
