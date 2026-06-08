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

    #define ESC "\x1b"


// ------------------------------------------------------------
// ANSI terminal helpers
// ------------------------------------------------------------

static inline void ansi(const std::string& s) {
    std::cout << s;
}

static inline void flushConsole() {
    std::cout.flush();
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
    // raw.c_oflag &= ~(OPOST); // won't print newlines, otherwise

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
    ansi(ESC "[0m");
    ansi(ESC "[?25h");

    ansi(ESC "[?1049l"); // leave alternate screen

    flushConsole();
}

// ------------------------------------------------------------
// Timing
// ------------------------------------------------------------

uint64_t OsPosixConsole::tick() const {
    static auto start = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<
               std::chrono::milliseconds>(now - start)
        .count();
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


    int col = 80;
    int row = 25;
    struct winsize w {};
    ioctl(0, TIOCGWINSZ, &w);
    if (w.ws_row * w.ws_col > 64) {
        if (w.ws_col < col) {
            col = w.ws_col;
        }
        if (w.ws_row <= row) {
            row = w.ws_row-1;
        }
    }

    // GRAPHIC 5
    screen.setSize(col, row);
    
    printf(ESC "[2J"); // clear screen
    printf(ESC "[H"); // home cursor

    printf(ESC "[?1049h"); // enter alternate screen
    printf(ESC "[1 q"); // cursor blinking block


    return true;
}

// ------------------------------------------------------------
// Memory
// ------------------------------------------------------------

size_t OsPosixConsole::getFreeMemoryInBytes() {
    long pages     = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return size_t(pages * page_size);
}



// ------------------------------------------------------------
// Rendering
// ------------------------------------------------------------

void OsPosixConsole::presentScreen() {
    printf("%s", screen.updateScreenTerminal().c_str());
    fflush(stdout);
}

// ------------------------------------------------------------
// Keyboard polling
// ------------------------------------------------------------

const bool OsPosixConsole::isKeyPressed(
    char32_t index,
    bool withShift,
    bool withAlt,
    bool withCtrl) const {

    auto peek = Os::peekKeyboardBuffer();
    if (peek.code == index && peek.holdAlt == withAlt && peek.holdShift == withShift && peek.holdCtrl == withCtrl) {
        return true;
    }

    // find cached (even shifted) escape key in keyboard buffer
    if (index == char32_t(KeyConstant::ESCAPE)) {
        if (escPressed) { 
            escPressed = false;
            return true;
        }
        for (auto& k : keyboardBuffer) {
            if (k.code == index) {
                return true;
            }
        }
    }

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
    pfd.fd     = STDIN_FILENO;
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
        key.code      = ch;


        // printf("%.2X,", int(ch));

        // ANSI escape sequences
        if (ch == 0x1b) {

            // Arrow keys
            if (i + 2 < n && buffer[i + 1] == '[') {

                switch (buffer[i + 2]) {

                case 'A':
                    key.code      = uint32_t(Os::KeyConstant::CRSR_UP);
                    key.printable = false;
                    break;

                case 'B':
                    key.code      = uint32_t(Os::KeyConstant::CRSR_DOWN);
                    key.printable = false;
                    break;

                case 'C':
                    key.code      = uint32_t(Os::KeyConstant::CRSR_RIGHT);
                    key.printable = false;
                    break;

                case 'D':
                    key.code      = uint32_t(Os::KeyConstant::CRSR_LEFT);
                    key.printable = false;
                    break;
                }

                i += 2;
            } else {
                key.code      = uint32_t(Os::KeyConstant::ESCAPE);
                key.printable = false;
                escPressed=true;
            }
        }

        if (ch == 127) {
            key.code      = uint32_t(Os::KeyConstant::BACKSPACE);
            key.printable = false;
        }

        if (ch == '\r' || ch == '\n') {
            key.code      = uint32_t(Os::KeyConstant::RETURN);
            key.printable = false;
        }

        putToKeyboardBuffer(key);
    }
}

#endif // !_WIN32