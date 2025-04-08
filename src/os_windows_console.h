#pragma once
#ifdef _WIN32
    #include "os.h"
class OsWindowsConsole : public Os {
    uint64_t tick() const override;

    bool init(Basic* basic, SoundSystem* ss) override;
    size_t getFreeMemoryInBytes() override;

    // --- SCREEN ---
    void presentScreen() override;
    void setBorderColor(int colorIndex) override;

    void setCaretPos(int x, int y);
    // --- KEYBOARD ---
    const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const override;
    KeyPress getFromKeyboardBuffer() override;
    void setCursorVisibility(bool visible);

private:
    void setTextColor(int index);
    void setBackgroundColor(int index);
    void updateKeyboardBuffer();
    std::u32string chars;
    std::string colors;
};

#endif // _WIN32