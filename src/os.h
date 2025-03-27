#pragma once
#include <string>
#include <vector>
#include <bitset>
#include "screen_buffer.h"

class Basic;
class Os;
class SoundSystem {
    public:
    Os* os = nullptr;
    // play a SFXR sound string in the backgroud
    virtual bool SOUND(int voice, const std::string& parameters) = 0;
    // play an ABC music notation string in the background
    virtual bool PLAY(const std::string& music) = 0;
};

class BA68settings {
    public:
    bool fullscreen = false;
    enum RenderMode {
        Software = 0,
        OpenGL = 1
    };
    RenderMode renderMode = Software;
    bool emulateCRT = true;
};

class Os {
    public:
    virtual ~Os() {}

    static BA68settings settings;

    // get a timer in ms
    virtual uint64_t tick() const = 0;

    // delay for some time
    virtual void delay(int ms) const;

    enum class KeyConstant : uint32_t {
        ESCAPE = 27,
        BACKSPACE = '\b',  // 8
        RETURN = '\r',     // 13
        NUM_ENTER = '\n',  // 10
        // 65..126 printable ASCII characters
        DEL = 127,  // WinNt.h defines DELETE
        F1 = 128,
        F2 = 129,
        F3 = 130,
        F4 = 131,
        F5 = 132,
        F6 = 133,
        F7 = 134,
        F8 = 135,
        F9 = 136,
        F10 = 137,
        F11 = 138,
        F12 = 139,

        HOME = 140,
        INSERT = 141,
        END = 142,
        PG_UP = 143,
        PG_DOWN = 144,
        CRSR_UP = 145,
        CRSR_DOWN = 146,
        CRSR_LEFT = 147,
        CRSR_RIGHT = 148,

        SCROLL = 149,
        PAUSE = 150,
        SHIFT_LEFT = 151,
        SHIFT_RIGHT = 152

        // 161 start of visible characters in unicode
        // no more than 255
    };

    // init your operating sepecific data
    virtual bool init(Basic*, SoundSystem*);

    // --- SCREEN ---
    // Screen buffer
    ScreenBuffer screen{};
    virtual void presentScreen() = 0;  // copy offscreen buffer to visible window
    virtual void setBorderColor(int colorIndex) {};
    virtual size_t getFreeMemoryInBytes() { return 122365; }

    // --- KEYBOARD ---
    // non-blocking keyboard status. The index is either the ASCII
    // key name or one of KeyConstant.
    virtual const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const = 0;
    virtual const bool isKeyPressed(KeyConstant k, bool withShift = false, bool withAlt = false, bool withCtrl = false) const { return isKeyPressed(uint32_t(k), withShift, withAlt, withCtrl); }

    // implement this to put new characters in the keyboard buffer,
    // when they are pressed. The framework will call this like every frame or so.
    virtual void updateKeyboardBuffer() = 0;

    // check, if there is a character in the keyboard buffer.
    // (recursively) calls updateKeyboardBuffer()
    virtual bool keyboardBufferHasData();

    class KeyPress {
        public:
        KeyPress() = default;
        KeyPress(const KeyPress&) = default;
        KeyPress& operator=(const KeyPress&) = default;
        KeyPress(uint32_t character)
            : KeyPress() {
            code = character;
            printable = true;
        }

        // utf32 representation of the input character, or one of KeyConstant.
        // Provides both, upper and lowercase.
        uint32_t code = 0;
        bool printable = false;  // true: visible character, false: cursor keys etc.
        bool holdShift = false;
        bool holdAlt = false;
        bool holdCtrl = false;
    };

    // returns a unicode character or a KEY_CONSTANT for a pressed key
    // of the keyboard buffer. This function should block until a new key
    // is pressed.
    virtual KeyPress getFromKeyboardBuffer();

    // pushes a keypress to the keyboard buffer. Drops overflow keys.
    virtual void putToKeyboardBuffer(Os::KeyPress key);

    // get/set utf8 clipbard text data
    virtual std::string getClipboardData() { return {}; }
    virtual void setClipboardData(const std::string utf8) { (void)utf8; };
    // show emoji keyboard and return selected codepoint(s)
    virtual std::vector<char32_t> emojiPicker() {
        return {};
    }

    // --- FILE SYSTEM ---
    // utf-8 strings. Directory separator is '/'
    struct FileInfo {
        std::string name;
        uint64_t filesize = 0;
        bool isDirectory = false;
        bool operator<(const FileInfo& i) const {
            if (isDirectory != i.isDirectory) { return isDirectory; }
            return name < i.name;
        }
    };
    virtual std::string getCurrentDirectory();
    virtual bool setCurrentDirectory(const std::string& dir);
    virtual std::vector<FileInfo> listCurrentDirectory();
    virtual bool doesFileExist(const std::string& path);
    virtual bool isDirectory(const std::string& path);
    virtual int systemCall(const std::string& commandLineUtf8, bool printOutput = true);

    // --- SOUND SYSTEM ---
    SoundSystem& soundSystem();

    // --- JOYPADS ---
    struct GamepadState {
        bool connected = false;
        const char* name = nullptr;
        std::bitset<32> buttons;
        struct DPad {
            int8_t x = 0, y = 0;  // x and y axis -1,/0/1
        } dpad = {};
        struct Analog {
            double x = 0.0, y = 0.0;  // x and y axis [-1 .. 1.0]
        } analogLeft = {}, analogRight = {};
    };
    virtual void updateGamepadState() {}
    virtual const GamepadState& getGamepadState(int index);  // index 0..8

    protected:
    Basic* basic = nullptr;
    int foregnd = 1, bkgnd = 0;
    std::vector<Os::KeyPress> keyboardBuffer;
    SoundSystem* sound = nullptr;

    private:
    char32_t getc();  // for systemCall

    bool initialized = false;
};
