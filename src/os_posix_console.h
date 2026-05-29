#pragma once

#ifndef _WIN32

#include "os.h"
#include <string>

class OsPosixConsole : public Os {
public:
    virtual ~OsPosixConsole();

    bool init(Basic* basic, SoundSystem* ss) override;

    uint64_t tick() const override;

    size_t getFreeMemoryInBytes() override;

    void setTextColor(int index);
    void setBackgroundColor(int index);
    void setBorderColor(int colorIndex);

    void presentScreen() override;

    void setCaretPos(int x, int y);
    void setCursorVisibility(bool visible);

    const bool isKeyPressed(
        char32_t index,
        bool withShift = false,
        bool withAlt = false,
        bool withCtrl = false) const override;

    KeyPress getFromKeyboardBuffer() override;

    std::string getHomeDirectory() override;
    std::string getEnv(const std::string& name) override;
    void setEnv(
        const std::string& name,
        const std::string& value) override;

    void updateEvents() override;

private:
    std::u32string chars;
    std::string colors;
};

#endif