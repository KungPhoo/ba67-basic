#ifndef _WIN32

#include "os_posix_console.h"
#include "unicode.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <poll.h>

namespace fs = std::filesystem;

// ------------------------------------------------------------
// ANSI terminal helpers
// ------------------------------------------------------------

static inline void ansi(const std::string& s) {
    std::cout << s;
}

static inline void flushConsole() {
    std::cout.flush();
}


static constexpr uint8_t C64_TO_ANSI16[16] = {
    0, // black
    7, // white
    1, // red
    6, // cyan
    5, // purple
    2, // green
    4, // blue
    3, // yellow
    3, // orange
    1, // brown
    9, // light red
    8, // dark gray
    7, // medium gray
    10, // light green
    12, // light blue
    15 // light gray
};


// DOS/C64 palette -> ANSI 16-color mapping
static inline int ansiFg(int c) {
    return C64_TO_ANSI16[c];
}

static inline int ansiBg(int c) {
    return C64_TO_ANSI16[c];
}

// ------------------------------------------------------------
// Terminal raw mode
// ------------------------------------------------------------

static termios g_oldTermios {};
static bool g_termiosInitialized = false;

static void enableRawMode() {
    if (g_termiosInitialized) {
        return;
    }

    tcgetattr(STDIN_FILENO, &g_oldTermios);

    termios raw = g_oldTermios;

    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);

    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    g_termiosInitialized = true;
}

static void disableRawMode() {
    if (!g_termiosInitialized) {
        return;
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_oldTermios);
    g_termiosInitialized = false;
}

// ------------------------------------------------------------
// Constructor / destructor
// ------------------------------------------------------------

OsPosixConsole::~OsPosixConsole() {
    disableRawMode();

    // Reset terminal state
    ansi("\033[0m");
    ansi("\033[?25h");
    flushConsole();
}

// ------------------------------------------------------------
// Timing
// ------------------------------------------------------------

uint64_t OsPosixConsole::tick() const {
    static auto start = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<
        std::chrono::milliseconds>(now - start).count();
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------

bool OsPosixConsole::init(Basic* basic, SoundSystem* ss) {
    Os::init(basic, ss);

    enableRawMode();

    // UTF-8 terminal
    setenv("LANG", "en_US.UTF-8", 1);

    // Create ~/Documents/BASIC
    std::string home = getHomeDirectory();

    if (!home.empty()) {
        fs::create_directories(home);
        fs::current_path(home);
    }

    // Clear screen
    ansi("\033[2J");
    ansi("\033[H");

    setCursorVisibility(false);

    return true;
}

// ------------------------------------------------------------
// Memory
// ------------------------------------------------------------

size_t OsPosixConsole::getFreeMemoryInBytes() {
    return Os::getFreeMemoryInBytes();
}

// ------------------------------------------------------------
// Colors
// ------------------------------------------------------------

void OsPosixConsole::setTextColor(int index) {
    foregnd = index & 0x0f;

    std::cout << "\033[" << ansiFg(foregnd) << "m";
}

void OsPosixConsole::setBackgroundColor(int index) {
    bkgnd = index & 0x0f;

    std::cout << "\033[" << ansiBg(bkgnd) << "m";
}

void OsPosixConsole::setBorderColor(int colorIndex) {
    // No-op on POSIX terminals
    (void)colorIndex;
}

// ------------------------------------------------------------
// Cursor
// ------------------------------------------------------------

void OsPosixConsole::setCaretPos(int x, int y) {
    std::cout << "\033[" << (y + 1)
              << ";" << (x + 1) << "H";
}

void OsPosixConsole::setCursorVisibility(bool visible) {
    ansi(visible ? "\033[?25h" : "\033[?25l");
}

// ------------------------------------------------------------
// Rendering
// ------------------------------------------------------------

void OsPosixConsole::presentScreen() {

    uint64_t now = tick();

    static uint64_t nextShow = 0;

    if (nextShow > now) {
        return;
    }

    nextShow = now + 5;

    chars.clear();
    colors.clear();

    const auto* memCh = screen.getMemory() + krnl.CHARRAM;
    const auto* memCl = screen.getMemory() + krnl.COLRAM;

    for (size_t y = 0; y < screen.height; ++y) {

        const auto* lnCh = memCh + y * screen.width;
        const auto* lnCo = memCl + y * screen.width;

        for (size_t x = 0; x < screen.width; ++x) {

            char32_t ch = lnCh[x];

            if (ch == U'\0') {
                break;
            }

            chars.push_back(ch);
            colors.push_back(char(lnCo[x]));
        }

        chars.push_back(U'\n');
        colors.push_back(0);
    }

    if (chars.empty()) {
        return;
    }

    setCursorVisibility(false);
    setCaretPos(0, 0);

    int ctext = -1;
    int cback = -1;

    for (size_t i = 0; i < chars.size(); ++i) {

        char32_t cp = chars[i];

        if (cp == U'\n') {
            std::cout << '\n';
            continue;
        }

        int craw = colors[i];

        int fg = craw & 0x0f;
        int bg = (craw >> 4) & 0x0f;

        if (fg != ctext) {
            ctext = fg;
            setTextColor(fg);
        }

        if (bg != cback) {
            cback = bg;
            setBackgroundColor(bg);
        }

        // Convert UTF-32 -> UTF-8
        std::u32string u32;
        u32.push_back(cp);

        std::string utf8 =
            Unicode::toUtf8String(
                reinterpret_cast<const char32_t*>(u32.c_str()));

        std::cout << utf8;
    }

    flushConsole();

    auto crsr = screen.getCursorPos();

    setCaretPos(int(crsr.x), int(crsr.y));
    setCursorVisibility(true);
}

// ------------------------------------------------------------
// Keyboard polling
// ------------------------------------------------------------

const bool OsPosixConsole::isKeyPressed(
    char32_t index,
    bool withShift,
    bool withAlt,
    bool withCtrl) const {

    (void)index;
    (void)withShift;
    (void)withAlt;
    (void)withCtrl;

    // Immediate-mode key state is not portable in terminals.
    // Event-driven input only.
    return false;
}

Os::KeyPress OsPosixConsole::getFromKeyboardBuffer() {

    while (!keyboardBufferHasData()) {
        updateEvents();
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1));
    }

    return Os::getFromKeyboardBuffer();
}

// ------------------------------------------------------------
// Filesystem
// ------------------------------------------------------------

std::string OsPosixConsole::getHomeDirectory() {

    const char* home = getenv("HOME");

    if (!home) {
        return ".";
    }

    fs::path p(home);
    p /= "Documents";
    p /= "BASIC";

    return p.string();
}

std::string OsPosixConsole::getEnv(
    const std::string& name) {

    const char* v = getenv(name.c_str());

    if (!v) {
        return {};
    }

    return v;
}

void OsPosixConsole::setEnv(
    const std::string& name,
    const std::string& value) {

    setenv(name.c_str(), value.c_str(), 1);
}

// ------------------------------------------------------------
// Event handling
// ------------------------------------------------------------

void OsPosixConsole::updateEvents() {

    pollfd pfd {};
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    int rv = poll(&pfd, 1, 0);

    if (rv <= 0) {
        return;
    }

    char buffer[32] {};
    ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));

    if (n <= 0) {
        return;
    }

    for (ssize_t i = 0; i < n; ++i) {

        unsigned char ch = buffer[i];

        Os::KeyPress key {};
        key.printable = true;
        key.code = ch;

        // ANSI escape sequences
        if (ch == 0x1b) {

            // Arrow keys
            if (i + 2 < n && buffer[i + 1] == '[') {

                switch (buffer[i + 2]) {

                case 'A':
                    key.code =
                        uint32_t(Os::KeyConstant::CRSR_UP);
                    key.printable = false;
                    break;

                case 'B':
                    key.code =
                        uint32_t(Os::KeyConstant::CRSR_DOWN);
                    key.printable = false;
                    break;

                case 'C':
                    key.code =
                        uint32_t(Os::KeyConstant::CRSR_RIGHT);
                    key.printable = false;
                    break;

                case 'D':
                    key.code =
                        uint32_t(Os::KeyConstant::CRSR_LEFT);
                    key.printable = false;
                    break;
                }

                i += 2;
            } else {

                key.code =
                    uint32_t(Os::KeyConstant::ESCAPE);

                key.printable = false;
            }
        }

        if (ch == 127) {
            key.code =
                uint32_t(Os::KeyConstant::BACKSPACE);
            key.printable = false;
        }

        if (ch == '\r' || ch == '\n') {
            key.code =
                uint32_t(Os::KeyConstant::RETURN);
            key.printable = false;
        }

        putToKeyboardBuffer(key);
    }
}

#endif // !_WIN32