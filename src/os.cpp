#include "os.h"
#include "unicode.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
// #include <cstring>
#include "string_helper.h"
#include "minifetch.h"

// static instance
BA68settings Os::settings = {};

Os::KeyPress Os::getFromKeyboardBuffer() {
    while (!keyboardBufferHasData()) {
        delay(150); // this cools the CPU when we wait for keyboard input
        updateEvents();
        presentScreen();
    }
    auto k = keyboardBuffer.back();
    keyboardBuffer.pop_back();
    if (settings.demoMode) {
        delay(200);
    }

    return k;
}


Os::KeyPress Os::peekKeyboardBuffer() const {
    if (keyboardBuffer.empty()) {
        return {};
    }
    return keyboardBuffer.back();
}


void Os::putToKeyboardBuffer(Os::KeyPress key, bool applyBufferLimit) {
    // key.debug();
    keyboardBuffer.insert(keyboardBuffer.begin(), key);

    // Keep buffer size limited
    if (applyBufferLimit && keyboardBuffer.size() > 128 * 1024) {
        keyboardBuffer.pop_back();
    }
}

std::string Os::getCurrentDirectory() {
    if (currentDirIsCloud) {
        return "CLOUD";
    }
    return (const char*)(std::filesystem::current_path().u8string().c_str());
}

bool Os::setCurrentDirectory(const std::string& dir) {
    if (dir == "CLOUD") {
        currentDirIsCloud = true;
        return true;
    }
    currentDirIsCloud = false;

    const int cpp = __cplusplus;
    std::error_code ec;
    std::filesystem::current_path(dir, ec);
    return !ec; // Returns true if no error occurred
}

std::vector<Os::FileInfo> Os::listCurrentDirectory() {
    std::vector<Os::FileInfo> files, dirs;

    if (currentDirIsCloud) {
        // List files

        std::string tmp = FilePtr::tempFileName();

        // systemCall("curl -sS -X LIST \"" + cloudUrl + "\" -H \"X-Auth: " + cloudUserHash() + "\" -o \"" + tmp + "\"", false);
        MiniFetch ft;
        ft.request.fillServerFromUrl(cloudUrl);
        ft.request.headers = {
            { "X-Auth", cloudUserHash() }
        };
        ft.request.method = "LIST";
        auto resp         = ft.fetch();
        if (resp.status != MiniFetch::Status::OK) {
            auto str = resp.toString();
            std::cerr << "MiniFetch error: " << int(resp.status) << " " << str << "\n";
            return {};
        }

        currentDirIsCloud = false; // now we're writing to local disk
        // auto f            = fopen(tmp, "rb");
        // if (f == nullptr) {
        //     return {};
        // }

        char* next_line = nullptr;
        char* buffer    = nullptr;
        if (!resp.bytes.empty()) {
            buffer = StringHelper::strtok_r((char*)(&resp.bytes[0]), "\r\n", &next_line);
        }
        while (buffer != nullptr) {
            for (size_t i = 0; i < 512; ++i) {
                if (buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '\0') {
                    buffer[i] = '\0';
                    break;
                }
            }
            Os::FileInfo fi;
            fi.filesize   = atoi(buffer);
            const char* c = buffer;
            while (*c != '\0' && *c != ' ') {
                ++c;
            }
            if (*c == ' ') {
                ++c;
            }
            fi.name = c;
            if (!fi.name.empty()) {
                files.emplace_back(fi);
            }

            buffer = StringHelper::strtok_r(nullptr, "\r\n", &next_line);
        }
        // char buffer[512] = { 0 };
        // while (!feof(f)) {
        //     memset(buffer, 0, sizeof(buffer));
        //     fgets(buffer, 512, f);
        //     for (size_t i = 0; i < 512; ++i) {
        //         if (buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '\0') {
        //             buffer[i] = '\0';
        //             break;
        //         }
        //     }
        //     Os::FileInfo fi;
        //     fi.filesize   = atoi(buffer);
        //     const char* c = buffer;
        //     while (*c != '\0' && *c != ' ') {
        //         ++c;
        //     }
        //     if (*c == ' ') {
        //         ++c;
        //     }
        //     fi.name = c;
        //     if (!fi.name.empty()) {
        //         files.emplace_back(fi);
        //     }
        // }
        // f.close();
        // scratchFile(tmp);

        currentDirIsCloud = true; // restore cloud state
    } else {
        Os::FileInfo info = {};
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
            info.isDirectory = entry.is_directory();
            info.filesize    = info.isDirectory ? 0 : entry.file_size();
            info.name        = (const char*)(entry.path().filename().u8string().c_str());
            files.push_back(info);
        }
        std::sort(files.begin(), files.end());
    }

    return files;
}

bool Os::doesFileExist(const std::string& path) {
    if (currentDirIsCloud && isRelativePath(path)) {
        auto files = listCurrentDirectory();
        for (auto& f : files) {
            if (f.name == path) {
                return true;
            }
        }
        return false;
    } else {
        return std::filesystem::exists(path);
    }
}

bool Os::isRelativePath(const std::string& path) {
#if defined(_WIN32)
    if (path.length() > 1 && (path[1] == ':' || path.starts_with("\\\\"))) {
        return false;
    }
#endif

    if (path.starts_with("/")) {
        return false;
    }
    return true;
}

bool Os::isDirectory(const std::string& path) {
    if (path == "CLOUD") {
        return true;
    }
    return std::filesystem::is_directory(path);
}

bool Os::scratchFile(const std::string& fileName) {
    if (currentDirIsCloud && isRelativePath(fileName)) {

        // systemCall("curl -sS -X DELETE \"" + cloudUrl + "\" -H \"X-Auth: " + cloudUserHash() + "\"", false);

        MiniFetch ft;
        ft.request.fillServerFromUrl(cloudUrl);
        ft.request.headers = {
            { "X-Auth", cloudUserHash() }
        };
        ft.request.getVariables = {
            { "file", fileName }
        };
        ft.request.method = "DELETE";
        auto resp         = ft.fetch();
        if (resp.status != MiniFetch::Status::OK) {
            auto str = resp.toString();
            return false;
        }

        return true;
    } else {
        if (doesFileExist(fileName)) {
            return std::filesystem::remove(fileName.c_str());
        }
    }
    return false;
}





std::string Os::cloudUserHash() const {
    uint64_t hash = 5381, hash2 = 7109; // Start with a large prime (DJB2 base)
    std::string username = (cloudUser + "asdfjka98324nkjn342i0nv8w08234x").substr(32);
    for (char c : cloudUser) {
        hash  = ((hash << 5) + hash) + static_cast<unsigned char>(c); // hash * 33 + c
        hash2 = ((hash2 << 5) + hash2) + static_cast<unsigned char>(0xff - c); // hash * 33 + c
    }

    std::string hex;
    auto addHex = [&hex](uint64_t i) {
        std::string s = StringHelper::int2hex(i & 0x00000000000000ff, false);
        hex += s.substr(s.length() - 2, 2);
    };

    // Convert to hex string
    for (int i = 56; i >= 0; i -= 8) {
        addHex(hash >> i);
    }
    for (int i = 56; i >= 0; i -= 8) {
        addHex(hash2 >> i);
    }
    return hex;
}


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
    this->updateEvents();
    if (this->keyboardBufferHasData()) {
        return getFromKeyboardBuffer().code;
    }
    return U'\0';
}

// delay for some time
void Os::delay(int ms) {
    uint64_t t = tick() + ms;
    while (tick() < t) {
        if (ms > 100) {
            presentScreen();
        }
    }
}

// initialize your operating specific data
bool Os::init(Basic* basic, SoundSystem* ss) {
    sound       = ss;
    sound->os   = this;
    this->basic = basic;
    return true;
}

bool Os::keyboardBufferHasData() {
    static uint64_t nextPoll = 0;

    if (keyboardBuffer.empty()) {
        // uint64_t now = tick();
        // if (nextPoll < now) {
        //     nextPoll = now + 10;
        //     updateEvents();
        // }
    }
    return !keyboardBuffer.empty();
}


#if defined __EMSCRIPTEN__

int Os::systemCall(const std::string& commandLineUtf8, bool printOutput) {
    const char* msg = "systemCall not supported\n";
    while (*msg != '\0') {
        this->screen.putC(*msg);
        ++msg;
    }
    return -1;
}

#elif defined(_WIN32)
    #include <Windows.h>
int Os::systemCall(const std::string& commandLineUtf8, bool printOutput) {

    bool lastWasCR = false;
    std::string utf8;
    auto flushUtf8 = [&]() {
        if (printOutput) {
            const char* pc = utf8.c_str();
            if (*pc == '\0') {
                return;
            }
            for (char32_t c32 = Unicode::parseNextUtf8(pc); c32 != 0; c32 = Unicode::parseNextUtf8(pc)) {
                // even if you print '\n', Windows will convert it to "\r\n".
                if (c32 == U'\n') {
                    if (lastWasCR) {
                        continue;
                    }
                }
                lastWasCR = (c32 == U'\r');
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


    #if defined(_DEBUG)
    utf8 = commandLineUtf8 + "\n";
    flushUtf8();
    #endif

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hReadOutput, hWriteOutput;
    HANDLE hReadInput, hWriteInput;

    if (!CreatePipe(&hReadOutput, &hWriteOutput, &sa, 0) || !CreatePipe(&hReadInput, &hWriteInput, &sa, 0)) {
        return -1;
    }
    if (!SetHandleInformation(hReadOutput, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(hWriteInput, HANDLE_FLAG_INHERIT, 0)) {
        return -1;
    }




    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFOW si        = { sizeof(STARTUPINFOW) };
    si.dwFlags             = STARTF_USESTDHANDLES;
    si.hStdOutput          = hWriteOutput;
    si.hStdError           = hWriteOutput;
    si.hStdInput           = hReadInput;

    if (!CreateProcessW(NULL, LPWSTR(&cmd_w[0]), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadOutput);
        CloseHandle(hWriteOutput);
        CloseHandle(hReadInput);
        CloseHandle(hWriteInput);
        return -1;
    }

    // JobObject to terminate the child process, when the parent crashes or gets closed.
    HANDLE hJob                               = CreateJobObject(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
    jeli.BasicLimitInformation.LimitFlags     = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE; // <-- does exactly this
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
    // Assign the process to the job
    AssignProcessToJobObject(hJob, pi.hProcess);


    CloseHandle(hWriteOutput);
    CloseHandle(hReadInput);

    char buffer[512] = {};
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
            char32_t c32s[2] = { 0, 0 };
            c32s[0]          = this->getc();
            if (c32s[0] == '\r') {
                c32s[0] = '\n';
            }
            if (c32s[0] == 0) {
                break; // No input available
            }
            this->updateEvents();
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
        Sleep(10); // Prevent 100% CPU usage by adding a small delay
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


#else // Linux builds
    #include <errno.h>
    #include <fcntl.h>
    #include <iostream>
    #include <poll.h>
    #include <string>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
    #include <vector>

    #include <linux/prctl.h> /* Definition of PR_* constants */
    #include <sys/prctl.h>

int Os::systemCall(const std::string& commandLineUtf8, bool printOutput) {
    int stdoutPipe[2], stdinPipe[2];
    if (pipe(stdoutPipe) == -1 || pipe(stdinPipe) == -1) {
        return -1;
    }


    pid_t pidParent = getppid();

    pid_t pid = fork();
    if (pid == -1) { // fork failed
        return -1;
    }

    if (pid == 0) { // Child process

        // When parent exists, send SIGKILL to all children
        prctl(PR_SET_PDEATHSIG, SIGKILL);

        close(stdoutPipe[0]); // Close read end of stdout pipe
        close(stdinPipe[1]); // Close write end of stdin pipe

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
        if (!temp.empty())
            args.push_back(temp);

        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(&arg[0]);
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());

        if (getppid() != pidParent) { }



        exit(EXIT_FAILURE); // exit child
    }

    // Main program thread
    close(stdoutPipe[1]); // Close write end of stdout pipe
    close(stdinPipe[0]); // Close read end of stdin pipe

    char buffer[512];
    std::string utf8;
    struct pollfd fds[] = {
        { stdoutPipe[0], POLLIN, 0 },
        {  STDIN_FILENO, POLLIN, 0 }
    };

    bool running = true;
    while (running) {
        if (poll(fds, 2, 10) > 0) {
            if (fds[0].revents & POLLIN) { // Check process output
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

            if (fds[1].revents & POLLIN) { // Check for user input
                char32_t c32 = this->getc();
                if (c32 == '\r')
                    c32 = '\n';
                if (c32 != 0) {
                    this->updateEvents();
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


std::string Os::findFirstFileNameWildcard(std::string filenameUtf8, bool isDirectory) {
    // no - the slow code below also fixes the case insensitivity
    // if (filenameUtf8.find('*') == std::string::npos && filenameUtf8.find('?') == std::string::npos) {
    //     return filenameUtf8;
    // }
    std::string cd = getCurrentDirectory();

    std::string fixedDirs;
    if (!currentDirIsCloud) {
        for (;;) {
            size_t endOfDir = filenameUtf8.find('/');
            if (endOfDir == std::string::npos) {
                break;
            }

            std::string folder = findFirstFileNameWildcard(filenameUtf8.substr(0, endOfDir), true);
            fixedDirs += folder;
            fixedDirs += '/';
            filenameUtf8 = filenameUtf8.substr(endOfDir + 1);

            if (!setCurrentDirectory(folder)) {
                break;
            }
        }
    }

    std::string outpath = fixedDirs + filenameUtf8;

    // list and search
    std::u32string fileu32;
    if (Unicode::toU32String(filenameUtf8.c_str(), fileu32)) {
        auto files = listCurrentDirectory();
        for (auto& f : files) {
            std::u32string fu32;
            if (f.isDirectory == isDirectory && Unicode::toU32String(f.name.c_str(), fu32)) {
                if (Unicode::wildcardMatchNoCase(fu32.c_str(), fileu32.c_str())) {
                    outpath = fixedDirs + f.name;
                    break;
                }
            }
        }
    }

    setCurrentDirectory(cd);
    return outpath;
}

void Os::KeyPress::debug() const {
    printf("KeyPress: code $%x, printable %c Shift %c Alt %c Ctrl %c\n",
           int(code), printable ? 'X' : 'O', holdShift ? 'X' : 'O', holdAlt ? 'X' : 'O', holdCtrl ? 'X' : 'O');
}
