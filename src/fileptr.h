#pragma once

#include <string>
#include <cstdio>
#include <vector>
#include <cstdint>
#include "ipc.h"

class Os;

class FilePtr {
    friend class Os;

public:
    FilePtr(Os* o)
        : os(o) { }
    FilePtr()                          = delete;
    FilePtr(const FilePtr&)            = delete;
    FilePtr& operator=(const FilePtr&) = delete;

    inline FilePtr(FilePtr&& other) noexcept
        : os(other.os)
        , dirty(other.dirty)
        , isWriting(other.isWriting)
        , cloudFileName(std::move(other.cloudFileName))
        , d64FileName(std::move(other.d64FileName))
        , localTempPath(std::move(other.localTempPath))
        , file(other.file)
        , ipc(other.ipc) {
        other.os   = nullptr;
        other.file = nullptr;
        other.ipc  = nullptr;
    }
    inline FilePtr& operator=(FilePtr&& other) noexcept {
        if (this != &other) {
            close();

            os            = other.os;
            dirty         = other.dirty;
            isWriting     = other.isWriting;
            cloudFileName = std::move(other.cloudFileName);
            d64FileName   = std::move(other.d64FileName);
            localTempPath = std::move(other.localTempPath);
            file          = other.file;
            ipc           = other.ipc;

            other.os   = nullptr;
            other.file = nullptr;
            other.ipc  = nullptr;
        }
        return *this;
    }

    ~FilePtr() { close(); }
    // operator FILE*() { return file; }
    operator bool() const {
        if (ipc != nullptr) {
            return ipc->isRunning();
        }
        return file != nullptr;
    }
    bool close();

    void setPassword(std::string pw);

    // open
    bool open(std::string filenameUtf8, const char* mode);
    bool openStdOut();
    bool openStdErr();
    bool openStdIn();
    bool openIPC(const IPC::Options& options);

    // io
    int printf(const char* fmt, ...);
    void flush();
    int seek(int offset, int origin);
    size_t tell();
    size_t read(void* buffer, size_t bytes);
    std::string getline();
    size_t write(const void* buffer, size_t bytes);
    std::vector<uint8_t> readAll();
    std::string status() const { return lastStatus; }

    // static
    static std::string tempFileName();
    static void sanitizePath(std::string& path, char separator = '/');
    static char nativeDirectorySeparator();

protected:
    friend class D64;

    Os* os         = nullptr;
    bool dirty     = false;
    bool isWriting = false;
    std::string password; // SAVE "xx,P" -> locking files in the cloud to read-only
    std::string cloudFileName; // filename for cloud
    std::string d64FileName; // filename for D64 file
    std::string localTempPath; // in case this is a cloud file
    bool fileIsStdIo = false;
    FILE* file       = nullptr;
    IPC* ipc         = nullptr;
    std::string lastStatus;

    bool fopenLocal(std::string filenameUtf8, const char* mode);
    void fopenCloud(std::string filenameUtf8, const char* mode);
};
