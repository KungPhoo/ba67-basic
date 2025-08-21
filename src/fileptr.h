#pragma once

#include <string>
#include <cstdio>
#include <vector>
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
        , localTempPath(std::move(other.localTempPath))
        , file(other.file) {
        other.os   = nullptr;
        other.file = nullptr;
    }
    inline FilePtr& operator=(FilePtr&& other) noexcept {
        if (this != &other) {
            close();

            os            = other.os;
            dirty         = other.dirty;
            isWriting     = other.isWriting;
            cloudFileName = std::move(other.cloudFileName);
            localTempPath = std::move(other.localTempPath);
            file          = other.file;

            other.os   = nullptr;
            other.file = nullptr;
        }
        return *this;
    }

    ~FilePtr() { close(); }
    // operator FILE*() { return file; }
    operator bool() const { return file != nullptr; }
    void close();
    static std::string tempFileName();

    bool open(std::string filenameUtf8, const char* mode);
    bool openStdOut();
    bool openStdErr();

    int printf(const char* fmt, ...);
    void flush();
    int seek(int offset, int origin);
    size_t tell();
    size_t read(void* buffer, size_t bytes);
    size_t write(void* buffer, size_t bytes);
    std::vector<uint8_t> readAll();


private:
    Os* os         = nullptr;
    bool dirty     = false;
    bool isWriting = false;
    std::string cloudFileName; // filename for cloud
    std::string localTempPath; // in case this is a cloud file
    bool fileIsStdIo = false;
    FILE* file       = nullptr;

    void fopenLocal(std::string filenameUtf8, const char* mode);
};
