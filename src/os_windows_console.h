#pragma once
#ifdef _WIN32
    #include "os.h"
class OsWindowsConsole : public Os {
public:
    OsWindowsConsole() = default;
    virtual ~OsWindowsConsole();

    uint64_t tick() const override;

    bool init(Basic* basic, SoundSystem* ss) override;
    void updateEvents();

    size_t getFreeMemoryInBytes() override;

    // --- SCREEN ---
    void presentScreen() override;

    // --- KEYBOARD ---
    const bool isKeyPressed(char32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const override;
    KeyPress getFromKeyboardBuffer() override;

    // --- FILE SYSTEM ---
    std::string getHomeDirectory() override;
    std::string getEnv(const std::string& name) override;
    void setEnv(const std::string& name, const std::string& value) override;

private:
    void setTextColor(int index);
    void setBackgroundColor(int index);
    std::string buffer;
};

#endif // _WIN32