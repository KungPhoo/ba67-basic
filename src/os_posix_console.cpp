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
    #include <cstring>


    #include <fcntl.h>
#ifdef __linux__
    #include <linux/kd.h>
#endif

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

    printf(ESC "[?7h"); // enable scrolling
    ansi(ESC "[?1049l"); // leave alternate screen

    flushConsole();
}



/* may be called with buf==NULL if we only want info */
/* must not exit - we may have cleanup to do */
int getfont(int fd, char* buf, int* count, int* width, int* height) {
    struct console_font_op cfo;
    int i;
    /* First attempt: KDFONTOP */
    fprintf(stderr, "Method 1\n");
    cfo.op    = KD_FONT_OP_GET;
    cfo.flags = 0;
    cfo.width = cfo.height = 32;
    cfo.charcount          = *count;
    cfo.data               = (unsigned char*)buf;
    i                      = ioctl(fd, KDFONTOP, &cfo);
    if (i == 0) {
        *count = cfo.charcount;
        if (height) {
            *height = cfo.height;
        }
        if (width) {
            *width = cfo.width;
        }
        return 0;
    }
    // if (errno != ENOSYS && errno != EINVAL) {
    //     fprintf(stderr, "getfont: KDFONTOP err %d\n", errno);
    //     return -1;
    // }
    /* Second attempt: GIO_FONTX */
    struct consolefontdesc cfd;
    fprintf(stderr, "Method 2\n");
    cfd.charcount  = *count;
    cfd.charheight = 0;
    cfd.chardata   = buf;
    i              = ioctl(fd, GIO_FONTX, &cfd);
    if (i == 0) {
        *count = cfd.charcount;
        if (height) {
            *height = cfd.charheight;
        }
        if (width) {
            *width = 8;
        }
        return 0;
    }
    return -1;
}


// ------------------------------------------------------------
// Init
// ------------------------------------------------------------
bool OsPosixConsole::init(Basic* basic, SoundSystem* ss) {
    Os::init(basic, ss);

    enableRawMode();

    // UTF-8 terminal
    this->setEnv("LANG", "en_US.UTF-8");

    // Create ~/Documents/BASIC
    std::string home = getHomeDirectory();

    if (!home.empty()) {
        fs::create_directories(home);
        fs::current_path(home);
    }

    // GRAPHIC 5
    int col = 80;
    int row = 25;
    struct winsize w {};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (w.ws_row * w.ws_col > 64) {
        if (w.ws_col < col) {
            col = w.ws_col;
        }
        if (w.ws_row < row) {
            row = w.ws_row;
        }
    }
    printf("terminal size is %dx%d\n", col, row);
    screen.setSize(col, row);

    // See, if we can redefine glyphs
    canUpdateFont=false;


    // int count=256, width=0, height=0;
    // int rv = getfont(STDOUT_FILENO, nullptr, &count, &width, &height);
    // printf("rv%d. c%d w%d h%d\n", rv, count, width, height);

    fontSlotCount = 256;

    std::vector<unsigned char> kdFontBuf(256 * 32);
    console_font_op op {};
    op.op        = KD_FONT_OP_GET;
    op.flags=0;
    op.width=op.height=32;
    op.data      = nullptr;
    op.charcount = 256;
    // get font information _and_ font glyphs - no way to just get the count.
    if (ioctl(STDOUT_FILENO, KDFONTOP, &op) == 0) {
        // 8x16x256
        printf("Linux console. %ux%u, %u glyphs\n",
               op.width,
               op.height,
               op.charcount);
        fontSlotCount = op.charcount;
        canUpdateFont=true;
    } else {
        fprintf(stderr, "KDFONTOP failed: errno=%d (%s)\n", errno, strerror(errno));
        char* tty = ttyname(STDOUT_FILENO);
        fprintf(stderr, "ttyname(stdout) = %s\n", tty ? tty : "(null)");

        printf("not a Linux virtual console\n");
    }


    slotToCodepoint.resize(fontSlotCount);
    for (size_t i = 0; i < fontSlotCount; ++i) { 
        slotToCodepoint[i] = char32_t(i);
    }
    nextSlot=128;
    mustReloadFont = true;

    // printf(ESC "[2J"); // clear screen
    // printf(ESC "[H"); // home cursor

    printf(ESC "[?1049h"); // enter alternate screen
    printf(ESC "[1 q"); // cursor blinking block
    printf(ESC "[?7l"); // disable scrolling and automatic line wrapping

    return true;
}

void OsPosixConsole::reloadFont() {
    if (!mustReloadFont) {
        return;
    }
    mustReloadFont = false;

    if (!canUpdateFont) {
        return;
    }

    static std::vector<uint8_t> fontData;
    fontData.resize(8*fontSlotCount);
    for (size_t i = 0; i < fontSlotCount; ++i) { 
        auto& cd = this->screen.getCharDefinition(slotToCodepoint[i]); // pixels for mapped character
        for (size_t y = 0; y < 8; ++y) { 
            fontData[i*8+y] = cd.bits[y];
        }
    }

    // map 16 bit Unicode to font slot for Linux console
    // our map is Unicode 0..255 = 0..255
    // The character bitmaps 128..255 are manipulated to represent the Unicode
    // characters in unimap[].
    // struct unipair {
    //     uint16_t unicode;
    //     uint16_t fontpos;
    // };
    // struct unimapdesc {
    //     uint16_t entry_ct;
    //     struct unipair* entries;
    // };
    static std::vector<unipair> pairs;
    pairs.resize(fontSlotCount);
    for (size_t i = 0; i < fontSlotCount; ++i) {
        pairs[i].unicode = uint16_t(i);
        pairs[i].fontpos = uint16_t(i);
    }

    unimapdesc umapd {};
    umapd.entry_ct = uint16_t(pairs.size());
    umapd.entries  = &pairs[0];

#ifdef __linux__
    static const char* devname = ttyname(STDOUT_FILENO);
    if (devname == NULL) { 
        devname = "/dev/tty"; // /dev/tty means "the controlling terminal of this process"
    }
    int fd = open(devname, O_RDWR); 
    if (fd < 0) {
        return;
    }

    // glyphs
    console_font_op op {};
    op.op        = KD_FONT_OP_SET;
    op.width     = 8;
    op.height    = 8;
    op.charcount = fontSlotCount;
    op.data      = const_cast<unsigned char*>(&fontData[0]);

    if (ioctl(fd, KDFONTOP, &op) < 0) {
        fprintf(stderr, "KDFONTOP");
    }

    // mapping
    struct unimapinit advise {};
    ioctl(fd, PIO_UNIMAPCLR, &advise);
    ioctl(fd, PIO_UNIMAP, &umapd);

    close(fd);
#endif
}


char32_t OsPosixConsole::mapUnicodeToFontpos(char32_t c) { 
    if (!canUpdateFont) { return c;}

    // we use the font slots [128 .. 255] for variable Unicode font characters
    for (size_t i = 127; i < fontSlotCount; ++i) { 
        if (slotToCodepoint[i] == c) { 
            return char32_t(i);
        }
    }
    // not found, add
    mustReloadFont=true;
    char32_t ret = char32_t(nextSlot);
    slotToCodepoint[ret] = c;
    if (++nextSlot >= fontSlotCount) {
        nextSlot = 128;
    }
    return ret;
}


// ------------------------------------------------------------
// Timing
// ------------------------------------------------------------

uint64_t OsPosixConsole::tick() const {
    static auto start = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
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
    printf("%s", screen.updateScreenTerminal(
        [this](char32_t c)
    {
        return mapUnicodeToFontpos(c);
    }
    ).c_str());
    fflush(stdout);
    reloadFont();
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

void OsPosixConsole::setEnv(const std::string& name, const std::string& value) {
    setenv(name.c_str(), value.c_str(), 1);
}

// ------------------------------------------------------------
// Event handling
// ------------------------------------------------------------
// TODO: insert, home, end, del, UTF-8 sequence (Umlaut)
//         e5[  7e~
// home:   5b 48
// end:    5b 46
// del:    5b 33 7e
// insert: 5b 32 7e
// äÄ, öÖ, üÜ, ß: c3 a4, c3 84, c3 b6, c3 96, c3 bc, c3 9c, c3 9f
//
// ansi(ESC "[?1049l"); // leave alternate screen
// for (ssize_t i = 0; i < n; ++i) {
//     printf("%X,\n", int(uint8_t(buffer[i])));
// }
// exit(0);

void OsPosixConsole::updateEvents() {
    pollfd pfd {};
    pfd.fd     = STDIN_FILENO;
    pfd.events = POLLIN;

    int rv = poll(&pfd, 1, 0);

    if (rv <= 0) {
        return;
    }

    char tmp[128];
    ssize_t n = read(STDIN_FILENO, tmp, sizeof(tmp));

    if (n <= 0) {
        return;
    }

    inputBuffer.append(tmp, static_cast<size_t>(n));

    parseInputBuffer();
}

void OsPosixConsole::parseInputBuffer() {
    size_t pos = 0;

    while (pos < inputBuffer.size()) {
        Os::KeyPress key {};
        key.printable = true;

        uint8_t ch    = static_cast<uint8_t>(inputBuffer[pos]);

        // ANSI sequence
        if (ch == 0x1B) {
            size_t consumed = 0;
            if (parseEscapeSequence(pos, consumed, key) && consumed > 0) {
                pos += consumed;
                putToKeyboardBuffer(key);
                continue;
            }else{
                // incomplete sequence
                if (pos+1 == inputBuffer.length()) { 
                    break; // wait for more bytes
                }
                ++pos;
                key.holdAlt=true;
                key.printable=false;
                ch = static_cast<uint8_t>(inputBuffer[pos]);

                if (ch >= 'A' && ch <= 'Z') { 
                    key.holdShift=true;
                }
            }
        }

        // control keys
        if (ch == 127) {
            ++pos;
            key.code      = uint32_t(Os::KeyConstant::BACKSPACE);
            key.printable = false;
            putToKeyboardBuffer(key);
            continue;
        }

        // UTF-8 character
        const char* buf    = inputBuffer.c_str() + pos;
        char32_t codepoint = Unicode::parseNextUtf8(buf);
        if (codepoint == 0) {
            // incomplete UTF-8 sequence
            pos = inputBuffer.length();
            break;
        }

        inputBuffer = std::string(buf);
        pos=0;
        key.code      = codepoint;

        putToKeyboardBuffer(key);
    }

    inputBuffer.erase(0, pos);
}


bool OsPosixConsole::parseEscapeSequence(
    size_t pos,
    size_t& consumed,
    Os::KeyPress& key) {
    const std::string& s = inputBuffer;

    if (s[pos] != 0x1B) {
        return false;
    }

    // standalone ESC
    if (pos + 1 >= s.size()) {
        consumed      = 1;
        key.code      = uint32_t(Os::KeyConstant::ESCAPE);
        key.printable = false;
        escPressed    = true;
        return true;
    }

    if (s.length() > pos + 2 && s[pos + 1] == 'O') {
        switch (s[pos + 2]) {
        case 'P':
            key.code      = uint32_t(Os::KeyConstant::F1);
            key.printable = false;
            consumed      = 3;
            return true;
        case 'Q':
            key.code      = uint32_t(Os::KeyConstant::F2);
            key.printable = false;
            consumed      = 3;
            return true;
        case 'R':
            key.code      = uint32_t(Os::KeyConstant::F3);
            key.printable = false;
            consumed      = 3;
            return true;
        case 'S':
            key.code      = uint32_t(Os::KeyConstant::F4);
            key.printable = false;
            consumed      = 3;
            return true;
        }
    }

    if (s[pos + 1] != '[') {
        // terminal sends ESC O for Shift+Alt+O. We would need a timeout to check for more data (F1)
        return false;
    }

    size_t p = pos + 2;

    while (p < s.size()) {
        unsigned char ch = static_cast<unsigned char>(s[p]);
        if (ch >= '@' && ch <= '~') {
            // is:  @ A..Z a..z [\]^_{|}~
            // not: !"#$%&'()*+,-./0..9:;<=>?
            break;
        }
        ++p;
    }

    // incomplete sequence
    if (p >= s.size()) {
        return false;
    }

    std::string seq = s.substr(pos + 2, p - (pos + 2));
    char finalChar  = s[p];
    consumed        = p - pos + 1;
    key.printable   = false;

    if (seq.length() > 3 && seq[1] == ';') { 
        int mod = std::atoi(seq.c_str());
        seq.erase(seq.begin(), seq.begin()+2);
        switch (mod) { 
        case 2: key.holdShift = true; break;
        case 3: key.holdAlt = true; break;
        case 4: key.holdShift = true; key.holdAlt = true;break;
        case 5: key.holdCtrl = true; break;
        case 6: key.holdCtrl = true; key.holdShift = true; break;
        case 7: key.holdCtrl = true; key.holdAlt = true; break;
        case 8: key.holdShift = true;key.holdCtrl = true; key.holdAlt = true; break;
        }
    }

    // ESC [ A/B/C/D
    if (seq.empty()) {
        switch (finalChar) {
        case 'A':
            key.code = uint32_t(Os::KeyConstant::CRSR_UP);
            return true;
        case 'B':
            key.code = uint32_t(Os::KeyConstant::CRSR_DOWN);
            return true;
        case 'C':
            key.code = uint32_t(Os::KeyConstant::CRSR_RIGHT);
            return true;
        case 'D':
            key.code = uint32_t(Os::KeyConstant::CRSR_LEFT);
            return true;
        case 'F':
            key.code = uint32_t(Os::KeyConstant::END);
            return true;
        case 'H':
            key.code = uint32_t(Os::KeyConstant::HOME);
            return true;
        }
    }

    // ESC [ n ~
    if (finalChar == '~') {
        int value = std::atoi(seq.c_str());
        switch (value) {
        case 1: key.code = uint32_t(Os::KeyConstant::HOME); return true;
        case 2: key.code = uint32_t(Os::KeyConstant::INSERT); return true;
        case 3: key.code = uint32_t(Os::KeyConstant::DEL); return true;
        case 4: key.code = uint32_t(Os::KeyConstant::END); return true;
        case 5: key.code = uint32_t(Os::KeyConstant::PG_UP); return true;
        case 6: key.code = uint32_t(Os::KeyConstant::PG_DOWN); return true;

        case 15: key.code = uint32_t(Os::KeyConstant::F5); return true;
        case 16: key.code = uint32_t(Os::KeyConstant::F6); return true;
        case 17: key.code = uint32_t(Os::KeyConstant::F7); return true;
        case 18: key.code = uint32_t(Os::KeyConstant::F8); return true;
        case 19: key.code = uint32_t(Os::KeyConstant::F9); return true;
        case 20: key.code = uint32_t(Os::KeyConstant::F10); return true;
        case 21: key.code = uint32_t(Os::KeyConstant::F11); return true;
        case 22: key.code = uint32_t(Os::KeyConstant::F12); return true;
        }
    }

    // Unknown CSI sequence.
    key.code = uint32_t(Os::KeyConstant::ESCAPE);
    return true;
}


#endif // !_WIN32