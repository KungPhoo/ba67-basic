#include "os.h"
#include <filesystem>
#include <algorithm>
#include "unicode.h"

// static instance
BA68settings Os::settings = {};

Os::KeyPress Os::getFromKeyboardBuffer() {
    while (!keyboardBufferHasData()) {
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
    if (keyboardBuffer.size() > 128 * 1024) {
        keyboardBuffer.pop_back();
    }
}

std::string Os::getCurrentDirectory() {
    return (const char*)(std::filesystem::current_path().u8string().c_str());
}

bool Os::setCurrentDirectory(const std::string& dir) {
    // TODU umlaut does not work, yet.
    std::error_code ec;
    std::filesystem::current_path(dir, ec);
    return !ec;  // Returns true if no error occurred
}

std::vector<Os::FileInfo> Os::listCurrentDirectory() {
    std::vector<Os::FileInfo> files, dirs;
    Os::FileInfo info = {};
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        info.isDirectory = entry.is_directory();
        info.filesize = info.isDirectory ? 0 : entry.file_size();
        info.name = (const char*)(entry.path().filename().u8string().c_str());
        files.push_back(info);
    }
    std::sort(files.begin(), files.end());
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
    if (sound == nullptr) {
        static NullSoundSystem nss;
        return nss;
    }
    return *sound;
}

const Os::GamepadState& Os::getGamepadState(int index) {
    static GamepadState gs;
    return gs;
}

char32_t Os::getc() {
    this->updateKeyboardBuffer();
    if (this->keyboardBufferHasData()) {
        return getFromKeyboardBuffer().code;
    }
    return U'\0';
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

    if (keyboardBuffer.empty()) {
        uint64_t now = tick();
        if (nextPoll < now) {
            nextPoll = now + 100;
            updateKeyboardBuffer();
        }
    }
    return !keyboardBuffer.empty();
}

#if defined(_WIN32)
    #include <Windows.h>
int Os::systemCall(const std::string& commandLineUtf8, bool printOutput) {
    std::string utf8;
    auto flushUtf8 = [&]() {
        if (printOutput) {
            const char* pc = utf8.c_str();
            if (*pc == '\0') { return; }
            for (char32_t c32 = Unicode::parseNextUtf8(pc); c32 != 0; c32 = Unicode::parseNextUtf8(pc)) {
                this->screen.putC(c32);
            }
            // this->screen.putC('\n');
        }
        utf8.clear();
    };

    std::u16string cmd_w;
    if (!Unicode::toU16String(commandLineUtf8.c_str(), cmd_w)) {
        return -1;
    }

    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hReadOutput, hWriteOutput;
    HANDLE hReadInput, hWriteInput;

    if (!CreatePipe(&hReadOutput, &hWriteOutput, &sa, 0) || !CreatePipe(&hReadInput, &hWriteInput, &sa, 0)) {
        return -1;
    }
    if (!SetHandleInformation(hReadOutput, HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(hWriteInput, HANDLE_FLAG_INHERIT, 0)) {
        return -1;
    }

    PROCESS_INFORMATION pi = {0};
    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWriteOutput;
    si.hStdError = hWriteOutput;
    si.hStdInput = hReadInput;

    if (!CreateProcessW(NULL, LPWSTR(&cmd_w[0]), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadOutput);
        CloseHandle(hWriteOutput);
        CloseHandle(hReadInput);
        CloseHandle(hWriteInput);
        return -1;
    }

    CloseHandle(hWriteOutput);
    CloseHandle(hReadInput);

    char buffer[512];
    DWORD bytesRead;
    bool running = true;

    while (running) {
        // **Check if there's output available before reading**
        DWORD bytesAvailable = 0;
        if (PeekNamedPipe(hReadOutput, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
            if (ReadFile(hReadOutput, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                utf8 += buffer;
                flushUtf8();
            }
        }

        // **Check for user input**
        while (true) {
            char32_t c32s[2] = {0, 0};
            c32s[0] = this->getc();
            if (c32s[0] == '\r') { c32s[0] = '\n'; }
            if (c32s[0] == 0) break;  // No input available

            this->updateKeyboardBuffer();
            this->screen.putC(c32s[0]);

            std::string inputUtf8 = Unicode::toUtf8String(&c32s[0]);
            DWORD bytesWritten;
            WriteFile(hWriteInput, inputUtf8.c_str(), DWORD(inputUtf8.size()), &bytesWritten, NULL);
        }

        // **Check if process is still running**
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        if (exitCode != STILL_ACTIVE) {
            running = false;
        }

        this->presentScreen();
        Sleep(10);  // Prevent 100% CPU usage by adding a small delay
    }

    CloseHandle(hReadOutput);
    CloseHandle(hWriteInput);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode;
}
#endif

#if !defined(_WIN32)
    #include <iostream>
    #include <string>
    #include <vector>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <fcntl.h>
    #include <poll.h>
    #include <errno.h>

int Os::systemCall(const std::string& commandLineUtf8, bool printOutput) {
    int stdoutPipe[2], stdinPipe[2];
    if (pipe(stdoutPipe) == -1 || pipe(stdinPipe) == -1) {
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {  // fork failed

        return -1;
    }

    if (pid == 0) {  // Child process

        close(stdoutPipe[0]);  // Close read end of stdout pipe
        close(stdinPipe[1]);   // Close write end of stdin pipe

        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stdoutPipe[1], STDERR_FILENO);
        dup2(stdinPipe[0], STDIN_FILENO);

        close(stdoutPipe[1]);
        close(stdinPipe[0]);

        std::vector<std::string> args;
        std::string temp;
        for (char c : commandLineUtf8) {
            if (c == ' ') {
                if (!temp.empty()) {
                    args.push_back(temp);
                    temp.clear();
                }
            } else {
                temp += c;
            }
        }
        if (!temp.empty()) args.push_back(temp);

        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(&arg[0]);
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        exit(EXIT_FAILURE);  // exit child
    }

    // Main program thread
    close(stdoutPipe[1]);  // Close write end of stdout pipe
    close(stdinPipe[0]);   // Close read end of stdin pipe

    char buffer[512];
    std::string utf8;
    struct pollfd fds[] = {
        {stdoutPipe[0], POLLIN, 0},
        {STDIN_FILENO, POLLIN, 0}};

    bool running = true;
    while (running) {
        if (poll(fds, 2, 10) > 0) {
            if (fds[0].revents & POLLIN) {  // Check process output
                ssize_t bytesRead = read(stdoutPipe[0], buffer, sizeof(buffer) - 1);
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    utf8 += buffer;
                    if (printOutput) {
                        const char* pc = utf8.c_str();
                        while (*pc) {
                            char32_t c32 = Unicode::parseNextUtf8(pc);
                            this->screen.putC(c32);
                        }
                        utf8.clear();
                    }
                }
            }

            if (fds[1].revents & POLLIN) {  // Check for user input
                char32_t c32 = this->getc();
                if (c32 == '\r') c32 = '\n';
                if (c32 != 0) {
                    this->updateKeyboardBuffer();
                    this->screen.putC(c32);
                    std::string inputUtf8 = Unicode::toUtf8String(&c32);
                    write(stdinPipe[1], inputUtf8.c_str(), inputUtf8.size());
                }
            }
        }

        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            running = false;
        }

        this->presentScreen();
    }

    close(stdoutPipe[0]);
    close(stdinPipe[1]);

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
#endif
