#include "os.h"
#include <filesystem>
#include "unicode.h"

// static instance
BA68settings Os::settings = {};

Os::KeyPress Os::getFromKeyboardBuffer() {
    while (!keyboardBufferHasData())
    {
        presentScreen();
        delay(50);
    }
    auto k = keyboardBuffer.back();
    keyboardBuffer.pop_back();
    return k;
}

void Os::putToKeyboardBuffer(Os::KeyPress key) {
    keyboardBuffer.insert(keyboardBuffer.begin(), key);

    // Keep buffer size limited
    if (keyboardBuffer.size() > 128 * 1024)
    {
        keyboardBuffer.pop_back();
    }
}

std::string Os::getCurrentDirectory() {
    return std::filesystem::current_path().string();
}

bool Os::setCurrentDirectory(const std::string& dir) {
    std::error_code ec;
    std::filesystem::current_path(dir, ec);
    return !ec;  // Returns true if no error occurred
}

std::vector<Os::FileInfo> Os::listCurrentDirectory() {
    std::vector<Os::FileInfo> files;
    Os::FileInfo info = {};
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path()))
    {
        info.filesize = entry.file_size();
        info.isDirectory = entry.is_directory();
        info.name = entry.path().filename().string();
        files.push_back(info);
    }
    return files;
}

bool Os::doesFileExist(const std::string& path) {
    return std::filesystem::exists(path);
}

bool Os::isDirectory(const std::string& path) {
    return std::filesystem::is_directory(path);
}

class NullSoundSystem : public SoundSystem {
   public:
    // play a SFXR sound string in the backgroud
    bool SOUND(int voice, const std::string& parameters) override { return true; }
    // play an ABC music notation string in the background
    bool PLAY(const std::string& music) override { return true; }
};

SoundSystem& Os::soundSystem() {
    if (sound == nullptr)
    {
        static NullSoundSystem nss;
        return nss;
    }
    return *sound;
}

// delay for some time
void Os::delay(int ms) const {
    uint64_t t = tick() + ms;
    while (tick() < t) {}
}

// init your operating sepecific data
bool Os::init(Basic* basic, SoundSystem* ss) {
    sound = ss;
    sound->os = this;
    this->basic = basic;
    return true;
}

bool Os::keyboardBufferHasData() {
    static uint64_t nextPoll = 0;

    if (keyboardBuffer.empty())
    {
        uint64_t now = tick();
        if (nextPoll < now)
        {
            nextPoll = now + 100;
            updateKeyboardBuffer();
        }
    }
    return !keyboardBuffer.empty();
}

#ifdef _WIN32
    #include <Windows.h>
#endif

int Os::systemCall(const std::string& commandLineUtf8, bool printOutput) {
    std::string utf8;
    auto flushUtf8 = [&]() {
        if (printOutput)
        {
            const char* pc = utf8.c_str();
            if (*pc == '\0') { return; }
            for (char32_t c32 = Unicode::parseNextUtf8(pc); c32 != 0; c32 = Unicode::parseNextUtf8(pc))
            {
                this->screen.putC(c32);
            }
            this->screen.putC('\n');
            this->presentScreen();
        }
        utf8.clear();
    };

#ifdef _WIN32
    std::u16string cmd_w;
    if (!Unicode::toU16String(commandLineUtf8.c_str(), cmd_w))
    {
        return -1;
    }

    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) { return -1; }
    if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0)) { return -1; }

    PROCESS_INFORMATION pi = {0};
    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    if (!CreateProcessW(NULL, LPWSTR(&cmd_w[0]), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return -1;
    }

    CloseHandle(hWrite);

    char buffer[512];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        utf8 += buffer;
        // utf8 = Unicode::toUtf8String((const char16_t*)(buffer));
        flushUtf8();
    }
    flushUtf8();

    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode;

#else
    FILE* pipe = popen(commandLineUtf8.c_str(), "r");
    if (!pipe)
    {
        return -1;
    }

    int c = 0;
    while ((c = fgetc(pipe)) != EOF)
    {
        if (c == '\n' || c == '\r')
        {
            flushUtf8();
        }
        else
        {
            utf8 += static_cast<char>(c);
        }
    }
    flushUtf8();
    int status = pclose(pipe);
    if (WIFEXITED(status))
    {
        status = WEXITSTATUS(status);  // Extract exit code on Unix-like systems
    }
    else
    {
        status = -1;  // Command didn't exit normally
    }
    return status;
#endif
}
