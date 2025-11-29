#pragma once
#ifdef _WIN32
    #include "os.h"
class OsWindowsConsole : public Os {
public:
    OsWindowsConsole() = default;

    uint64_t tick() const override;

    bool init(Basic* basic, SoundSystem* ss) override;
    void updateEvents();

    size_t getFreeMemoryInBytes() override;

    // --- SCREEN ---
    void presentScreen() override;
    void setBorderColor(int colorIndex) override;

    void setCaretPos(int x, int y);
    // --- KEYBOARD ---
    const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const override;
    KeyPress getFromKeyboardBuffer() override;
    void setCursorVisibility(bool visible);

    // --- FILE SYSTEM ---
    std::string getHomeDirectory() override;

private:
    void setTextColor(int index);
    void setBackgroundColor(int index);
    std::u32string chars;
    std::string colors;
};

#endif // _WIN32