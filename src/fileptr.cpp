#include "fileptr.h"
#include "minifetch.h"
#include <filesystem>
#include <cstdarg>
#include "os.h"
#include "unicode.h"
#include "string_helper.h"

#if defined(__EMSCRIPTEN__)
    #include <emscripten.h>




#endif

// TODO persistentStorage https://stackoverflow.com/questions/54617194/how-to-save-files-from-c-to-browser-storage-with-emscripten

bool FilePtr::close() {
    lastStatus.clear();

    if (file != nullptr) {
        if (!fileIsStdIo) {
            fclose(file);
        }
        fileIsStdIo = false;
        file        = nullptr;
    }

    bool rv = true;
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
                {     "file", cloudFileName },
                { "password",      password }
            };
            auto resp  = ft.fetch();
            lastStatus = resp.toString();

            if (resp.status != MiniFetch::Status::OK) {
                if ((resp.status / 100) == 4) {
                    rv = false;
                }
            }
        }
    }
    if (!localTempPath.empty()) {
        std::error_code ec;
        std::filesystem::remove(localTempPath, ec);
    }
    dirty = false;
    localTempPath.clear();
    cloudFileName.clear();


#ifdef __EMSCRIPTEN__
    EM_ASM(

        FS.syncfs(function(err) {
            // Error
        });

        // // make a promise to await the sync
        // function syncfs(populate) {
        //     return new Promise((resolve, reject) = > {
        //         FS.syncfs(populate, (err) = > {
        // if (err) reject(err);
        // else resolve(); });
        //     });
        // }
        //
        // await syncfs(false);
        // end
    );
#endif

    // TODO throw, maybe
    return rv;
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


void FilePtr::setPassword(std::string pw) {
    password = pw; // we can use any character and emoji here.
}

bool FilePtr::open(std::string filenameUtf8, const char* mode) {
    lastStatus.clear();

    close();
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
            auto resp  = ft.fetch();
            lastStatus = resp.toString();
            if (resp.status == MiniFetch::Status::OK) {
                FilePtr ftmp(os);
                ftmp.fopenLocal(filenameUtf8, "wb");
                ftmp.printf("%s", resp.toString().c_str());
                ftmp.close();
            } else {
                return false;
            }
        }
    }

    this->fopenLocal(filenameUtf8, mode);
    return file != nullptr;
}

bool FilePtr::openStdOut() {
    close();
    lastStatus.clear();
    file        = stdout;
    fileIsStdIo = true;
    return file != nullptr;
}

bool FilePtr::openStdErr() {
    close();
    lastStatus.clear();
    file        = stdout;
    fileIsStdIo = true;
    return file != nullptr;
}

// Cross-platform fprintf-like method
int FilePtr::printf(const char* fmt, ...) {
    if (!file) {
        lastStatus = "BAD FILE";
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
    if (!file || fileIsStdIo) {
        lastStatus = "BAD FILE";
        return -1;
    }
    return std::fseek(file, long(offset), origin);
}

size_t FilePtr::tell() {
    if (!file || fileIsStdIo) {
        lastStatus = "BAD FILE";
        return -1;
    }
    return std::ftell(file);
}

size_t FilePtr::read(void* buffer, size_t bytes) {
    if (!file) {
        lastStatus = "BAD FILE";
        return 0;
    }
    return fread(buffer, bytes, 1, file);
}

std::string FilePtr::getline() {
    std::string str;
    char c = 0;
    while (read(&c, 1) != 0) {
        str += c;
        if (c == '\n') {
            break;
        }
    }
    return str;
}

size_t FilePtr::write(const void* buffer, size_t bytes) {
    if (!file) {
        lastStatus = "BAD FILE";
        return 0;
    }
    return fwrite(buffer, bytes, 1, file);
}

std::vector<uint8_t> FilePtr::readAll() {
    if (fileIsStdIo) {
        return {};
    }
    seek(0, SEEK_END);
    size_t len = tell();
    seek(0, SEEK_SET);
    std::vector<uint8_t> bytes(len);
    read(&bytes[0], len);
    return bytes;
}

void FilePtr::sanitizePath(std::string& path, char separator) {
    size_t pos = 0;
    for (auto& c : path) {
        if (c == '/' || c == '\\') {
            c = separator;
        }
        if (c == '?' || c == '*' || c == '\"' || c == '|' || c == '<' || c == '>') {
            c = '-';
        }
        if (c == ':' && pos != 1) {
            c = '.';
        }
        ++pos;
    }
}

char FilePtr::nativeDirectorySeparator() {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

void FilePtr::fopenLocal(std::string filenameUtf8, const char* mode) {
    sanitizePath(filenameUtf8, nativeDirectorySeparator());
    lastStatus.clear();
    file        = nullptr;
    fileIsStdIo = false;
    dirty       = false;
    if (mode[0] == 'w') {
        isWriting = true;

#ifdef _WIN32
        std::u16string u16;
        Unicode::toU16String(filenameUtf8.c_str(), u16);
        file = ::_wfsopen(reinterpret_cast<const wchar_t*>(u16.c_str()), L"wb", _SH_DENYNO); // allow shared reading - even if some editor has the file open
#else
        file = std::fopen(filenameUtf8.c_str(), "wb");
#endif
        if (!file) {
            lastStatus = "CAN'T OPEN FOR WRITING";
        }
    } else if (mode[0] == 'r') {
        isWriting = false;
#ifdef _WIN32
        std::u16string u16;
        Unicode::toU16String(filenameUtf8.c_str(), u16);
        file = ::_wfsopen(reinterpret_cast<const wchar_t*>(u16.c_str()), L"rb", _SH_DENYNO); // allow shared reading - even if some editor has the file open
#else
        file = std::fopen(filenameUtf8.c_str(), "rb");
#endif
        if (!file) {
            lastStatus = "CAN'T OPEN FOR READING";
        }
    } else {
        if (!file) {
            lastStatus = "UNSUPPORTED MODE";
        }
        throw std::runtime_error("FilePtr::open - only r and w supported");
    }
}
