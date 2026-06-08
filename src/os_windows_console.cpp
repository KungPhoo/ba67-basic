#ifdef _WIN32
    #include "os_windows_console.h"
    #include "os.h"
    #include "unicode.h"
    #include <Windows.h>
    #include <conio.h>
    #include <iostream>
    #include <stdlib.h>
    #include <string>

#define ESC "\x1b"
#define CSI "\x1b["



// DOS colors
// 0=black,1=dblue, 2=dgreen, 3=dcyan, 4=dred, 5=dpurple, 6=dyellow,7=ltgray
// 8=gray  9= blue,10= green,11= cyan,12= red,13= purple,14= yellow,15=white
void ConsoleColor(int c = 15, int bk = 0) {
    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, c + bk * 16);
}

void SetC64ConsoleColours(const uint32_t* aarrggbb) {
    CONSOLE_SCREEN_BUFFER_INFOEX sbInfoEx;
    sbInfoEx.cbSize   = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    HANDLE consoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfoEx(consoleOut, &sbInfoEx);

    // C64 color palette
    sbInfoEx.ColorTable[0]  = aarrggbb[0]  & 0x00ffffff; // RGB(0x00, 0x00, 0x00); // Black
    sbInfoEx.ColorTable[1]  = aarrggbb[1]  & 0x00ffffff; // RGB(0xFF, 0xFF, 0xFF); // White
    sbInfoEx.ColorTable[2]  = aarrggbb[2]  & 0x00ffffff; // RGB(0x96, 0x28, 0x2e); // Red
    sbInfoEx.ColorTable[3]  = aarrggbb[3]  & 0x00ffffff; // RGB(0x5b, 0xd6, 0xce); // Cyan
    sbInfoEx.ColorTable[4]  = aarrggbb[4]  & 0x00ffffff; // RGB(0x9f, 0x2d, 0xad); // Purple
    sbInfoEx.ColorTable[5]  = aarrggbb[5]  & 0x00ffffff; // RGB(0x41, 0xb9, 0x36); // Green
    sbInfoEx.ColorTable[6]  = aarrggbb[6]  & 0x00ffffff; // RGB(0x27, 0x24, 0xc4); // Blue
    sbInfoEx.ColorTable[7]  = aarrggbb[7]  & 0x00ffffff; // RGB(0xef, 0xf3, 0x47); // Yellow
    sbInfoEx.ColorTable[8]  = aarrggbb[8]  & 0x00ffffff; // RGB(0x9f, 0x48, 0x15); // Orange
    sbInfoEx.ColorTable[9]  = aarrggbb[9]  & 0x00ffffff; // RGB(0x5e, 0x35, 0x00); // Brown
    sbInfoEx.ColorTable[10] = aarrggbb[10] & 0x00ffffff; // RGB(0xda, 0x5f, 0x66); // Light Red
    sbInfoEx.ColorTable[11] = aarrggbb[11] & 0x00ffffff; // RGB(0x47, 0x47, 0x47); // Dark Gray
    sbInfoEx.ColorTable[12] = aarrggbb[12] & 0x00ffffff; // RGB(0x78, 0x78, 0x78); // Medium Gray
    sbInfoEx.ColorTable[13] = aarrggbb[13] & 0x00ffffff; // RGB(0x91, 0xff, 0x84); // Light Green
    sbInfoEx.ColorTable[14] = aarrggbb[14] & 0x00ffffff; // RGB(0x68, 0x64, 0xff); // Light Blue
    sbInfoEx.ColorTable[15] = aarrggbb[15] & 0x00ffffff; // RGB(0xae, 0xae, 0xae); // Light Gray

    SetConsoleScreenBufferInfoEx(consoleOut, &sbInfoEx);
}

void SetConsoleFont(const wchar_t* fontName, SHORT fontSize, bool bold) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize       = sizeof(CONSOLE_FONT_INFOEX);
    cfi.nFont        = 0;
    cfi.dwFontSize.X = 0; // Width (0 = auto)
    cfi.dwFontSize.Y = fontSize; // Font size (height)
    cfi.FontFamily   = FF_DONTCARE;
    cfi.FontWeight   = bold ? FW_BOLD : FW_NORMAL;
    wcscpy_s(cfi.FaceName, fontName); // Set the font name

    if (!SetCurrentConsoleFontEx(hConsole, FALSE, &cfi)) {
        std::cerr << "Failed to set console font!" << std::endl;
    }
}

static BOOL SetConsoleSize(size_t cols, size_t rows) {
    ++rows;
    HWND hWnd;
    HANDLE hConOut;
    CONSOLE_FONT_INFO fi;
    CONSOLE_SCREEN_BUFFER_INFO bi;
    int w, h, bw, bh;
    RECT rect   = { 0, 0, 0, 0 };
    COORD coord = { 0, 0 };
    hWnd        = GetConsoleWindow();
    hConOut     = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hWnd != NULL && hConOut && hConOut != (HANDLE)-1) {
        if (GetCurrentConsoleFont(hConOut, FALSE, &fi) && GetClientRect(hWnd, &rect)) {
            w = rect.right - rect.left;
            h = rect.bottom - rect.top;
            if (GetWindowRect(hWnd, &rect)) {
                bw = rect.right - rect.left - w;
                bh = rect.bottom - rect.top - h;
                if (GetConsoleScreenBufferInfo(hConOut, &bi)) {
                    coord.X = bi.dwSize.X;
                    coord.Y = bi.dwSize.Y;
                    if (coord.X < cols || coord.Y < rows) {
                        if (coord.X < short(cols)) {
                            coord.X = short(cols);
                        }
                        if (coord.Y < short(rows)) {
                            coord.Y = short(rows);
                        }
                        if (!SetConsoleScreenBufferSize(hConOut, coord)) {
                            return FALSE;
                        }
                    }
                    return SetWindowPos(hWnd, NULL, rect.left, rect.top, int(cols) * fi.dwFontSize.X + bw, int(rows) * fi.dwFontSize.Y + bh, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
                }
            }
        }
    }
    return FALSE;
}

OsWindowsConsole::~OsWindowsConsole() {
    printf(ESC "[?1049l"); // leave alternate screen

}

uint64_t OsWindowsConsole::tick() const {
    static uint64_t tick0 = GetTickCount64();
    return GetTickCount64() - tick0;
}


bool EnableVTMode() {
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        return false;
    }
    return true;
}

bool OsWindowsConsole::init(Basic* basic, SoundSystem* ss) {
    Os::init(basic, ss);

    // system("chcp 65001");
    SetConsoleOutputCP(CP_UTF8); // 65001 - give programs a chance to output utf-8, if they care
    EnableVTMode();

    wchar_t* user = nullptr;
    size_t len    = 0;
    if (0 == _wdupenv_s(&user, &len, L"USERPROFILE")) {
        std::wstring home = (user == nullptr) ? L"" : user;
        free(user);
        user = nullptr;
        home += L"\\Documents";
        ::CreateDirectoryW(home.c_str(), NULL);
        home += L"\\BASIC";
        ::CreateDirectoryW(home.c_str(), NULL);
        ::SetCurrentDirectoryW(home.c_str());
    }

    SetConsoleSize(screen.width + 1, screen.height + 2);
    // SetConsoleFont(L"Cascadia Mono", 24, true);
    SetConsoleFont(L"Consolas", 24, true);
    SetConsoleFont(L"BA67 square", 24, true);
    // SetConsoleFont(L"Cascadia Code PL", 24, true);
    // SetC64ConsoleColours(&screen.palette[0]); // std::array<uint32_t, 16> palette; // AABBGGRR little endian format


    printf(ESC "[?1049h"); // enter alternate screen
    printf(ESC "[1 q"); // cursor blinking block

    return true;
}

size_t OsWindowsConsole::getFreeMemoryInBytes() {
    MEMORYSTATUSEX memoryInfo;
    memoryInfo.dwLength = sizeof(memoryInfo);

    if (GlobalMemoryStatusEx(&memoryInfo)) {
        return size_t(memoryInfo.ullAvailPhys);
        // std::cout << "Total Physical Memory: " << memoryInfo.ullTotalPhys / 1024 / 1024 << " MB" << std::endl;
        // std::cout << "Available Physical Memory: " << memoryInfo.ullAvailPhys / 1024 / 1024 << " MB" << std::endl;
    }
    return Os::getFreeMemoryInBytes();
}


void OsWindowsConsole::presentScreen() {
    printf("%s", screen.updateScreenTerminal().c_str());
    fflush(stdout);
}


const bool OsWindowsConsole::isKeyPressed(char32_t index, bool withShift, bool withAlt, bool withCtrl) const {
    size_t bitmask = 0x8000;
    switch (index) {
    case char32_t(Os::KeyConstant::ESCAPE):
        index = VK_ESCAPE;
        break;
    case char32_t(Os::KeyConstant::RETURN):
        index = VK_RETURN;
        break;
    case char32_t(Os::KeyConstant::NUM_ENTER):
        index = VK_RETURN;
        break;
    case char32_t(Os::KeyConstant::F1):
        index = VK_F1;
        break;
    case char32_t(Os::KeyConstant::F2):
        index = VK_F2;
        break;
    case char32_t(Os::KeyConstant::F3):
        index = VK_F3;
        break;
    case char32_t(Os::KeyConstant::F4):
        index = VK_F4;
        break;
    case char32_t(Os::KeyConstant::F5):
        index = VK_F5;
        break;
    case char32_t(Os::KeyConstant::F6):
        index = VK_F6;
        break;
    case char32_t(Os::KeyConstant::F7):
        index = VK_F7;
        break;
    case char32_t(Os::KeyConstant::F8):
        index = VK_F8;
        break;
    case char32_t(Os::KeyConstant::F9):
        index = VK_F9;
        break;
    case char32_t(Os::KeyConstant::F10):
        index = VK_F10;
        break;
    case char32_t(Os::KeyConstant::F11):
        index = VK_F11;
        break;
    case char32_t(Os::KeyConstant::F12):
        index = VK_F12;
        break;
    case char32_t(Os::KeyConstant::HOME):
        index = VK_HOME;
        break;
    case char32_t(Os::KeyConstant::END):
        index = VK_END;
        break;
    case char32_t(Os::KeyConstant::PG_UP):
        index = VK_PRIOR;
        break; // Page Up
    case char32_t(Os::KeyConstant::PG_DOWN):
        index = VK_NEXT;
        break; // Page Down
    case char32_t(Os::KeyConstant::INSERT):
        index = VK_INSERT;
        break;
    case char32_t(Os::KeyConstant::DEL):
        index = VK_DELETE;
        break;
    case char32_t(Os::KeyConstant::CRSR_UP):
        index = VK_UP;
        break;
    case char32_t(Os::KeyConstant::CRSR_DOWN):
        index = VK_DOWN;
        break;
    case char32_t(Os::KeyConstant::CRSR_LEFT):
        index = VK_LEFT;
        break;
    case char32_t(Os::KeyConstant::CRSR_RIGHT):
        index = VK_RIGHT;
        break;
    case char32_t(Os::KeyConstant::SCROLL):
        index   = VK_SCROLL;
        bitmask = 0x0001;
        break;
    case char32_t(Os::KeyConstant::PAUSE):
        index = VK_PAUSE;
        break;
    case char32_t(Os::KeyConstant::SHIFT_LEFT):
        index = VK_LSHIFT;
        break;
    case char32_t(Os::KeyConstant::SHIFT_RIGHT):
        index = VK_RSHIFT;
        break;
        // case char32_t(Os::KeyConstant::CTRL_LEFT):
        //     index = VK_LCONTROL;
        //     break;
        // case char32_t(Os::KeyConstant::CTRL_RIGHT):
        //     index = VK_RCONTROL;
        //     break;
    }

    bool pressed = ((GetKeyState(int(index)) & bitmask) != 0);
    if (withShift) {
        pressed &= ((GetAsyncKeyState(int(VK_LSHIFT)) & 0x8000) != 0 || (GetAsyncKeyState(int(VK_RSHIFT)) & 0x8000) != 0);
    }
    if (withAlt) {
        pressed &= ((GetAsyncKeyState(int(VK_LMENU)) & 0x8000) != 0);
    }
    if (withCtrl) {
        pressed &= ((GetAsyncKeyState(int(VK_LCONTROL)) & 0x8000) != 0 || (GetAsyncKeyState(int(VK_RCONTROL)) & 0x8000) != 0);
    }
    return pressed;
}

Os::KeyPress OsWindowsConsole::getFromKeyboardBuffer() {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE) {
        throw std::exception("Failed to get console handle");
    }
    // Wait for an event on the console handle.
    DWORD dwWaitResult;
    while (!this->keyboardBufferHasData() && getMouseStatus().buttonBits == 0) {
        // also returns when mouse button was clicked
        dwWaitResult = WaitForSingleObject(hStdin, 50 /*ms*/);
        updateEvents();
    }
    return Os::getFromKeyboardBuffer();
}


// --- FILE SYSTEM ---
std::string OsWindowsConsole::getHomeDirectory() {
    std::string homeDir = ".";
    wchar_t* user       = nullptr;
    size_t len          = 0;
    if (0 == _wdupenv_s(&user, &len, L"USERPROFILE") && user != nullptr) {
        std::wstring home = user;
        free(user);
        user = nullptr;
        home += L"\\Documents";
        ::CreateDirectoryW(home.c_str(), NULL);
        home += L"\\BASIC";
        ::CreateDirectoryW(home.c_str(), NULL);
        homeDir = Unicode::toUtf8String((const char16_t*)home.c_str());
        ;
        // ::SetCurrentDirectoryW(home.c_str());
    }
    return homeDir;
}

std::string OsWindowsConsole::getEnv(const std::string& name) {
    std::u16string n16;
    Unicode::toU16String(name.c_str(), n16);
    std::wstring rv = _wgetenv((const wchar_t*)n16.c_str());
    return Unicode::toUtf8String((const char16_t*)rv.c_str());
    return std::string();
}

void OsWindowsConsole::setEnv(const std::string& name, const std::string& value) {
    std::string all = name + "=" + value;
    std::u16string n16;
    Unicode::toU16String(all.c_str(), n16);
    std::u16string v16;
    Unicode::toU16String(value.c_str(), v16);
    ::SetEnvironmentVariableW((const wchar_t*)n16.c_str(), (const wchar_t*)v16.c_str());
}

void OsWindowsConsole::updateEvents() {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get console handle.\n";
        return;
    }

    INPUT_RECORD inputRecord;
    DWORD eventsRead;

    static wchar_t highSurrogate = 0; // Store high surrogate if needed

    while (PeekConsoleInputW(hStdin, &inputRecord, 1, &eventsRead) && eventsRead > 0) {
        // https://learn.microsoft.com/en-us/windows/console/key-event-record-str
        ReadConsoleInputW(hStdin, &inputRecord, 1, &eventsRead);

        if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown) {
            WCHAR ch = inputRecord.Event.KeyEvent.uChar.UnicodeChar;
            // if (ch == 0) continue;  // Ignore non-character keys

            Os::KeyPress key;
            key.printable = false;
            key.holdShift = (inputRecord.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) != 0;
            key.holdAlt   = (inputRecord.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) != 0;
            key.holdCtrl  = (inputRecord.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) != 0;

            switch (inputRecord.Event.KeyEvent.wVirtualKeyCode) {
            case VK_LEFT:
                key.code = uint32_t(Os::KeyConstant::CRSR_LEFT);
                break;
            case VK_RIGHT:
                key.code = uint32_t(Os::KeyConstant::CRSR_RIGHT);
                break;
            case VK_UP:
                key.code = uint32_t(Os::KeyConstant::CRSR_UP);
                break;
            case VK_DOWN:
                key.code = uint32_t(Os::KeyConstant::CRSR_DOWN);
                break;

            case VK_SCROLL:
                key.code = uint32_t(Os::KeyConstant::SCROLL);
                break;
            case VK_PAUSE:
                key.code = uint32_t(Os::KeyConstant::PAUSE);
                break;
            case VK_LSHIFT:
                key.code = uint32_t(Os::KeyConstant::SHIFT_LEFT);
                break;
            case VK_RSHIFT:
                key.code = uint32_t(Os::KeyConstant::SHIFT_RIGHT);
                break;

            case VK_BACK:
                key.code = uint32_t(Os::KeyConstant::BACKSPACE);
                break;
            case VK_DELETE:
                key.code = uint32_t(Os::KeyConstant::DEL);
                break;
            case VK_INSERT:
                key.code = uint32_t(Os::KeyConstant::INSERT);
                break;
            case VK_HOME:
                key.code = uint32_t(Os::KeyConstant::HOME);
                break;
            case VK_END:
                key.code = uint32_t(Os::KeyConstant::END);
                break;
            case VK_PRIOR:
                key.code = uint32_t(Os::KeyConstant::PG_UP);
                break; // PgUp
            case VK_NEXT:
                key.code = uint32_t(Os::KeyConstant::PG_DOWN);
                break; // PgDn
            case VK_ESCAPE:
                key.code = uint32_t(Os::KeyConstant::ESCAPE);
                break;

            case VK_F1:
                key.code = uint32_t(Os::KeyConstant::F1);
                break;
            case VK_F2:
                key.code = uint32_t(Os::KeyConstant::F2);
                break;
            case VK_F3:
                key.code = uint32_t(Os::KeyConstant::F3);
                break;
            case VK_F4:
                key.code = uint32_t(Os::KeyConstant::F4);
                break;
            case VK_F5:
                key.code = uint32_t(Os::KeyConstant::F5);
                break;
            case VK_F6:
                key.code = uint32_t(Os::KeyConstant::F6);
                break;
            case VK_F7:
                key.code = uint32_t(Os::KeyConstant::F7);
                break;
            case VK_F8:
                key.code = uint32_t(Os::KeyConstant::F8);
                break;
            case VK_F9:
                key.code = uint32_t(Os::KeyConstant::F9);
                break;
            case VK_F10:
                key.code = uint32_t(Os::KeyConstant::F10);
                break;
            case VK_F11:
                key.code = uint32_t(Os::KeyConstant::F11);
                break;
            case VK_F12:
                key.code = uint32_t(Os::KeyConstant::F12);
                break;
            case VK_RETURN:
                if ((inputRecord.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) != 0) {
                    key.code = uint32_t(Os::KeyConstant::NUM_ENTER);
                } else {
                    key.code = uint32_t(Os::KeyConstant::RETURN);
                }
                break;

            default:

                // Detect surrogate pairs
                if (ch >= 0xD800 && ch <= 0xDBFF) {
                    // It's a high surrogate, store it
                    highSurrogate = ch;
                    continue;
                } else if (ch >= 0xDC00 && ch <= 0xDFFF && highSurrogate != 0) {
                    // It's a low surrogate, combine with high surrogate
                    key.printable          = (ch != 0);
                    uint32_t fullCodePoint = ((highSurrogate - 0xD800) << 10) + (ch - 0xDC00) + 0x10000;
                    key.code               = fullCodePoint;
                    highSurrogate          = 0; // Reset high surrogate storage
                } else {
                    // Normal BMP character (or isolated surrogate, which shouldn't happen)
                    key.printable = (ch != 0);
                    key.code      = ch;
                    highSurrogate = 0; // Reset in case of an unexpected sequence
                }
                break;
            }

            this->putToKeyboardBuffer(key);
        }
    }
}
#endif // _WIN32