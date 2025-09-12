#pragma once
#include "screen_buffer.h"
#include <bitset>
#include <string>
#include <vector>
#include "fileptr.h"

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

class NullSoundSystem : public SoundSystem {
public:
    bool SOUND(int voice, const std::string& parameters) override { return true; }
    bool PLAY(const std::string& music) override { return true; }
};

class BA68settings {
public:
    bool fullscreen = false;
    struct RenderMode {
        enum Enum {
            Software = 0,
            OpenGL   = 1
        };
    };
    RenderMode::Enum renderMode = RenderMode::Software;
    bool emulateCRT             = true;
    bool demoMode               = false; // slowly process the input buffer for creating videos
};

class Os {
public:
    virtual ~Os() { }

    // initialize your operating specific data. Call this base function first!
    virtual bool init(Basic*, SoundSystem*);

    // implement this to put new characters in the keyboard buffer,
    // when they are pressed. Keep you user interface alive.
    // Pump messages.
    // The framework will call this like every frame or so.
    virtual void updateEvents() = 0;


    // --- TIME ---
    // [ms] get a continuous timer value
    virtual uint64_t tick() const = 0;

    // delay for some time.
    virtual void delay(int ms);

public:
    // --- SCREEN ---
    // Screen buffer
    ScreenBuffer screen {};
    virtual void presentScreen() = 0; // copy off-screen buffer to visible window
    virtual void setBorderColor(int colorIndex) { };
    virtual size_t getFreeMemoryInBytes() { return 122365; }

public:
    // --- KEYBOARD ---
    enum class KeyConstant : uint32_t {
        ESCAPE    = 27,
        BACKSPACE = '\b', // 8
        RETURN    = '\r', // 13
        NUM_ENTER = '\n', // 10
        // 65..126 printable ASCII characters
        DEL = 127, // WinNt.h defines DELETE
        F1  = 128,
        F2  = 129,
        F3  = 130,
        F4  = 131,
        F5  = 132,
        F6  = 133,
        F7  = 134,
        F8  = 135,
        F9  = 136,
        F10 = 137,
        F11 = 138,
        F12 = 139,

        HOME       = 140,
        INSERT     = 141,
        END        = 142,
        PG_UP      = 143,
        PG_DOWN    = 144,
        CRSR_UP    = 145,
        CRSR_DOWN  = 146,
        CRSR_LEFT  = 147,
        CRSR_RIGHT = 148,

        SCROLL      = 149,
        PAUSE       = 150,
        SHIFT_LEFT  = 151,
        SHIFT_RIGHT = 152

        // 161 start of visible characters in Unicode
        // no more than 255
    };
    // query non-blocking keyboard status. The index is either the Unicode code point
    // one of KeyConstant.
    virtual const bool isKeyPressed(uint32_t index, bool withShift = false, bool withAlt = false, bool withCtrl = false) const = 0;
    virtual const bool isKeyPressed(KeyConstant k, bool withShift = false, bool withAlt = false, bool withCtrl = false) const { return isKeyPressed(uint32_t(k), withShift, withAlt, withCtrl); }

    // check, if there is a character in the keyboard buffer.
    // (recursively) calls updateEvents()
    virtual bool keyboardBufferHasData();

    class KeyPress {
    public:
        KeyPress()                           = default;
        KeyPress(const KeyPress&)            = default;
        KeyPress& operator=(const KeyPress&) = default;
        KeyPress(uint32_t character)
            : KeyPress() {
            code      = character;
            printable = true;
        }

        // utf32 representation of the input character, or one of KeyConstant.
        // Provides both, upper and lowercase.
        char32_t code  = 0;
        bool printable = false; // true: visible character, false: cursor keys etc.
        bool holdShift = false;
        bool holdAlt   = false;
        bool holdAltGr = false;
        bool holdCtrl  = false;

        void debug() const;
    };

    // returns a Unicode character or a KEY_CONSTANT for a pressed key
    // of the keyboard buffer. This function should block until a new key
    // is pressed.
    virtual KeyPress getFromKeyboardBuffer();

    // peek and return the last key on the stack. Do not wait.
    virtual KeyPress peekKeyboardBuffer() const;

    // pushes a key-press to the keyboard buffer. Drops overflow keys.
    virtual void putToKeyboardBuffer(Os::KeyPress key, bool applyBufferLimit = true);

    // get/set utf8 clipboard text data
    virtual std::string getClipboardData() { return {}; }
    virtual void setClipboardData(const std::string utf8) { (void)utf8; };
    // show emoji keyboard and return selected codepoint(s)
    virtual std::vector<char32_t> emojiPicker() {
        return {};
    }


public:
    // --- JOYPADS ---
    struct GamepadState {
        bool connected   = false;
        const char* name = nullptr;
        std::bitset<32> buttons;
        struct DPad {
            int8_t x = 0, y = 0; // x and y axis -1,/0/1
        } dpad = {};
        struct Analog {
            float x = 0.0f, y = 0.0f; // x and y axis [-1 .. 1.0]
        } analogLeft = {}, analogRight = {};
    };
    virtual void updateGamepadState() { }
    virtual const GamepadState& getGamepadState(int index); // index 0..8

public:
    // --- MOUSE (Light-PEN) ---
    struct MouseStatus {
        // x,y coordinates of the screen -{50,50}, if the mouse is clicked
        int x, y;
        uint8_t buttonBits;
    };
    virtual MouseStatus getMouseStatus() { return {}; }

public:
    // --- SOUND SYSTEM ---
    SoundSystem& soundSystem();

public:
    // --- FILE SYSTEM ---
    // utf8 strings. Directory separator is '/'
    struct FileInfo {
        std::string name;
        uint64_t filesize = 0;
        bool isDirectory  = false;
        bool isLocked     = false; // password protected cloud file
        bool operator<(const FileInfo& i) const {
            if (isDirectory != i.isDirectory) {
                return isDirectory;
            }
            return name < i.name;
        }
    };
    virtual std::string lockSymbol() const;
    virtual std::string getCurrentDirectory();
    virtual std::string getHomeDirectory() = 0; // ~/BASIC e.g.
    virtual bool setCurrentDirectory(const std::string& dir);
    virtual std::vector<FileInfo> listCurrentDirectory();
    virtual bool doesFileExist(const std::string& path);
    virtual bool isRelativePath(const std::string& path);
    virtual bool isDirectory(const std::string& path);
    virtual bool scratchFile(const std::string& fileName);
    virtual int systemCall(const std::string& commandLineUtf8, bool printOutput = true);

    std::string findFirstFileNameWildcard(std::string filenameUtf8, bool isDirectory = false);

    std::string cloudUrl  = "https://www.ba67.org/cloud/";
    std::string cloudUser = "examples@ba67.org";
    bool dirIsInCloud() const { return currentDirIsCloud; }

private:
    friend class FilePtr;
    bool currentDirIsCloud = false;
    std::string cloudUserHash() const;

public:
    // Command-line settings
    static BA68settings settings;

protected:
    Basic* basic = nullptr;
    int foregnd = 1, bkgnd = 0;
    std::vector<Os::KeyPress> keyboardBuffer;
    SoundSystem* sound = nullptr;

private:
    char32_t getc(); // for systemCall

    bool initialized = false;
};
