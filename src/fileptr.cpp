#include "fileptr.h"
#include "minifetch.h"
#include <filesystem>
#include <cstdarg>
#include "os.h"
#include "unicode.h"

void FilePtr::close() {
    if (file != nullptr) {
        fclose(file);
        file = nullptr;
    }

    if (!cloudFileName.empty() && !localTempPath.empty()) {
        if (dirty) {
            // Upload a file
            // os->systemCall("curl -sS -X POST --data-binary @\"" + localTempPath + "\" \"" + os->cloudUrl + "?file=" + cloudFileName + "\" -H \"X-Auth: " + os->cloudUserHash() + "\"");
            MiniFetch ft;
            ft.request.method = "POST";
            ft.request.fillServerFromUrl(os->cloudUrl);
            ft.request.headers = {
                { "X-Auth", os->cloudUserHash() }
            };
            ft.request.postFileNames = {
                { "filedata1", localTempPath }
            };
            ft.request.getVariables = {
                { "file", cloudFileName }
            };
            auto resp = ft.fetch();

            if (resp.status != MiniFetch::Status::OK) {
            }
            auto str = resp.toString();
        }
        cloudFileName.clear();
    }
    if (!localTempPath.empty()) {
        std::error_code ec;
        std::filesystem::remove(localTempPath, ec);
        localTempPath.clear();
    }
    // TODO throw, maybe
    dirty = false;
}

std::string FilePtr::tempFileName() {
    namespace fs = std::filesystem;

    std::error_code ec;
    fs::path tempDir = fs::temp_directory_path(ec);
    if (!ec) {
        // Generate a unique file path
        for (int i = 0; i < 999; ++i) {
            auto tempFile = tempDir / fs::path("ba67-cloud-" + std::to_string(std::rand()) + ".bas");
            if (!fs::exists(tempFile)) {
                return (const char*)(tempFile.u8string().c_str());
            }
        }
    }
    return "bad-path.tmp";
}


bool FilePtr::open(std::string filenameUtf8, const char* mode) {
    if (os->currentDirIsCloud) {
        localTempPath = FilePtr::tempFileName();
        cloudFileName = filenameUtf8;
        filenameUtf8  = localTempPath;
        if (mode[0] == 'r') {
            // systemCall("curl -sS -X GET \"" + cloudUrl + "?file=" + f.cloudFileName + "\" -H \"X-Auth: " + cloudUserHash() + "\" -o \"" + f.localTempPath + "\"", false);

            MiniFetch ft;
            ft.request.method = "GET";
            ft.request.fillServerFromUrl(os->cloudUrl);
            ft.request.headers = {
                { "X-Auth", os->cloudUserHash() }
            };
            ft.request.getVariables = {
                { "file", cloudFileName }
            };
            auto resp = ft.fetch();
            if (resp.status == MiniFetch::Status::OK) {
                FilePtr ftmp(os);
                ftmp.fopenLocal(filenameUtf8, "wb");
                ftmp.printf("%s", resp.toString().c_str());
                ftmp.close();
            }
        }
    }

    this->fopenLocal(filenameUtf8, mode);
    return file != nullptr;
}

// Cross-platform fprintf-like method
int FilePtr::printf(const char* fmt, ...) {
    if (!file) {
        return -1;
    }
    dirty = true;
    va_list args;
    va_start(args, fmt);
    int result = std::vfprintf(file, fmt, args);
    va_end(args);
    return result;
}

void FilePtr::flush() {
    if (file) {
        std::fflush(file);
    }
}

int FilePtr::seek(int offset, int origin) {
    if (!file) {
        return -1;
    }
    return std::fseek(file, long(offset), origin);
}

size_t FilePtr::tell() {
    if (!file) {
        return -1;
    }
    return std::ftell(file);
}

size_t FilePtr::read(void* buffer, size_t bytes) {
    if (!file) {
        return 0;
    }
    return fread(buffer, bytes, 1, file);
}

size_t FilePtr::write(void* buffer, size_t bytes) {
    if (!file) {
        return 0;
    }
    return fwrite(buffer, bytes, 1, file);
}

std::vector<uint8_t> FilePtr::readAll() {
    seek(0, SEEK_END);
    size_t len = tell();
    seek(0, SEEK_SET);
    std::vector<uint8_t> bytes(len);
    read(&bytes[0], len);
    return bytes;
}

void FilePtr::fopenLocal(std::string filenameUtf8, const char* mode) {
    dirty = false;
    if (mode[0] == 'w') {
        isWriting = true;

#ifdef _WIN32
        std::u16string u16;
        Unicode::toU16String(filenameUtf8.c_str(), u16);
        file = ::_wfsopen(reinterpret_cast<const wchar_t*>(u16.c_str()), L"wb", _SH_DENYNO); // allow shared reading - even if some editor has the file open
#else
        file = std::fopen(filenameUtf8.c_str(), "wb");
#endif
    } else if (mode[0] == 'r') {
        isWriting = false;
#ifdef _WIN32
        std::u16string u16;
        Unicode::toU16String(filenameUtf8.c_str(), u16);
        file = ::_wfsopen(reinterpret_cast<const wchar_t*>(u16.c_str()), L"rb", _SH_DENYNO); // allow shared reading - even if some editor has the file open
#else
        file = std::fopen(filenameUtf8.c_str(), "rb");
#endif
    } else {
        throw std::runtime_error("FilePtr::open - only r and w supported");
    }
}
