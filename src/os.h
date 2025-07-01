#pragma once
#include "screen_buffer.h"
#include <bitset>
#include <string>
#include <vector>

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
        OpenGL   = 1
    };
    RenderMode renderMode = Software;
    bool emulateCRT       = true;
    bool demoMode         = false; // slowly process the input buffer for creating videos
};

class Os {
public:
    virtual ~Os() { }

    static BA68settings settings;

    // get a timer in ms
    virtual uint64_t tick() const = 0;

    // delay for some time
    virtual void delay(int ms);

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

    // initialize your operating specific data
    virtual bool init(Basic*, SoundSystem*);

    // --- SCREEN ---
    // Screen buffer
    ScreenBuffer screen {};
    virtual void presentScreen() = 0; // copy off-screen buffer to visible window
    virtual void setBorderColor(int colorIndex) { };
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
        bool holdCtrl  = false;

        void debug() const {
            printf("KeyPress: code $%x, printable %c Shift %c Alt %c Ctrl %c\n",
                   int(code), printable ? 'X' : 'O', holdShift ? 'X' : 'O', holdAlt ? 'X' : 'O', holdCtrl ? 'X' : 'O');
        }
    };

    // returns a Unicode character or a KEY_CONSTANT for a pressed key
    // of the keyboard buffer. This function should block until a new key
    // is pressed.
    virtual KeyPress getFromKeyboardBuffer();

    // pushes a key-press to the keyboard buffer. Drops overflow keys.
    virtual void putToKeyboardBuffer(Os::KeyPress key, bool applyBufferLimit = true);

    // get/set utf8 clipboard text data
    virtual std::string getClipboardData() { return {}; }
    virtual void setClipboardData(const std::string utf8) { (void)utf8; };
    // show emoji keyboard and return selected codepoint(s)
    virtual std::vector<char32_t> emojiPicker() {
        return {};
    }

    // --- FILE SYSTEM ---
    // utf8 strings. Directory separator is '/'
    struct FileInfo {
        std::string name;
        uint64_t filesize = 0;
        bool isDirectory  = false;
        bool operator<(const FileInfo& i) const {
            if (isDirectory != i.isDirectory) {
                return isDirectory;
            }
            return name < i.name;
        }
    };
    class FilePtr {
        friend class Os;

    public:
        FilePtr(Os* o)
            : os(o) { }
        ~FilePtr() { close(); }
        operator FILE*() { return file; }
        operator bool() const { return file != nullptr; }
        void close();
        static std::string tempFileName();

        int fprintf(const char* fmt, ...);

    private:
        Os* os         = nullptr;
        bool isWriting = false;
        std::string cloudFileName; // filename for cloud
        std::string localTempPath; // in case this is a cloud file
        FILE* file = nullptr;
    };
    FilePtr fopen(std::string filenameUtf8, const char* mode);
    virtual std::string getCurrentDirectory();
    virtual bool setCurrentDirectory(const std::string& dir);
    virtual std::vector<FileInfo> listCurrentDirectory();
    virtual bool doesFileExist(const std::string& path);
    virtual bool isDirectory(const std::string& path);
    virtual bool scratchFile(const std::string& fileName);
    virtual int systemCall(const std::string& commandLineUtf8, bool printOutput = true);

    std::string findFirstFileNameWildcard(std::string filenameUtf8, bool isDirectory = false);

    std::string cloudUrl  = "http://www.ba67.org/cloud.php";
    std::string cloudUser = "examples@ba67.org";

private:
    bool currentDirIsCloud = false;
    std::string cloudUserHash() const;

public:
    // --- SOUND SYSTEM ---
    SoundSystem& soundSystem();

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

    // MOUSE (Light-PEN)
    // return x,y coordinates of the screen -{50,50}, if the mouse is clicked
    struct MouseStatus {
        int x, y;
        uint8_t buttonBits;
    };
    virtual MouseStatus getMouseStatus() { return {}; }


protected:
    Basic* basic = nullptr;
    int foregnd = 1, bkgnd = 0;
    std::vector<Os::KeyPress> keyboardBuffer;
    SoundSystem* sound = nullptr;

private:
    char32_t getc(); // for systemCall

    bool initialized = false;
};
