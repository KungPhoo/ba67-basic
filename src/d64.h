#pragma once
#include <string>
#include <vector>
// Simple, single-file C++17 implementation to parse and create .d64 images
// Supports standard 35-track, 174848-byte .d64 images.
// Features:
//  - Load .d64 into D64 structure (disk name + files with data and type)
//  - Write D64 structure back to a .d64 image
//  - Command-line helpers: "extract" to dump files from .d64 to a folder,
//    and "pack" to create a .d64 from a folder of files (extension .prg => PRG)
// Notes / limitations:
//  - PETSCII handling is basic: assumes filenames in directory are ASCII-compatible
//    and trims padding (0xA0). Full PETSCII transliteration is not implemented.
//  - REL-type files and subtypes are not handled specially.
//  - Directory entry and BAM handling follows Commodore 1541 conventions.
class Os;
class D64 {
public:
    D64() = default;
    void init(Os* pOs) { os = pOs; }
    std::string diskName;
    enum FileType : uint8_t { PRG = 0x82,
                              SEQ = 0x81,
                              USR = 0x83,
                              REL = 0x84,
                              DEL = 0x00 };
    struct FILE {
        std::string name; // Utf-8 file name
        std::vector<uint8_t> data; // raw bytes
        FileType type = PRG;
    };
    std::vector<FILE> files;

    bool load(std::string path);
    bool save(std::string path) const;
    bool removeFile(std::string filename);

private:
    Os* os = nullptr;
    bool load_d64_from_image(const std::vector<uint8_t>& img);
    std::vector<uint8_t> save_d64_to_image() const;
};
