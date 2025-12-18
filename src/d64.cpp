#include "d64.h"

// https://ist.uwaterloo.ca/~schepers/formats/D64.TXT

#include "petscii.h"
#include "unicode.h"
#include "fileptr.h"
#include <set>
#include <cstring>

static const int TRACKS      = 35; // standard image
static const int SECTOR_SIZE = 256;
static const int IMAGE_SIZE  = 174848; // 35-track d64

struct DIRENTRY {
    uint8_t next_t, next_s, file_type, file_track, file_sector;
    uint8_t file_name[16];
    uint8_t rel_t, rel_s, rel_len;
    uint8_t unused[6];
    uint8_t size_low, size_high;
};

static const int sectorsPerTrack[TRACKS] = {
    // tracks 1..35
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    19, 19, 19, 19, 19, 19, 19,
    18, 18, 18, 18, 18, 18,
    17, 17, 17, 17, 17
};

// Convert (track [1..35], sector [0..N-1]) -> byte offset in .d64 image
size_t ts_offset(int track, int sector) {
    if (track < 1 || track > TRACKS) {
        throw "track out of range";
    }
    int t      = track - 1;
    size_t off = 0;
    for (int i = 0; i < t; ++i) {
        off += sectorsPerTrack[i] * SECTOR_SIZE;
    }
    off += sector * SECTOR_SIZE;
    return off;
}

// We need to manage BAM (Block Availability Map), allocate sectors, write linked sectors & directory

struct Bam {
    // For each track (1-based index), store bitmask of sectors (1=free). We'll use vector<uint8_t> as bytes in same layout
    // Also store free-sector counts per track
    std::vector<uint8_t> freeCount; // [1..TRACKS]
    // raw bitmaps as they appear in BAM sector: for track i, 3 bytes little-endian (up to 24 bits) but sector count <= 21 -> 3 bytes enough
    std::vector<std::vector<uint8_t>> map;
    Bam() {
        freeCount.assign(TRACKS + 1, 0);
        map.assign(TRACKS + 1, { 0, 0, 0 });
    }
};


// Helper to clamp or create an empty image
static std::vector<uint8_t> make_blank_image() {
    std::vector<uint8_t> img(IMAGE_SIZE, 0);
    // default BAM values will be written by write_bam()
    return img;
}

// --- PETSCII helpers (very small): trim 0xA0 padding and convert common PETSCII->ASCII
static std::string petscii_to_ascii_name(const uint8_t* raw) {
    // raw is typically 16 bytes (0xA0 used for padding). We'll convert bytes >= 0x20
    std::string s;
    for (int i = 0; i < 16; ++i) {
        uint8_t petscii = raw[i];
        if (petscii == 0xA0) {
            break; // indicates end of filename
        }
        if (petscii == 0) { // it's an inverse 'at' symbol
            s.push_back('@');
            continue;
        }
        char32_t unicode = PETSCII::toUnicode(petscii);
        Unicode::appendAsUtf8(s, unicode);
    }
    return s;
}

static std::vector<uint8_t> ascii_name_to_petscii16(const std::string& name) {
    std::vector<uint8_t> out(16, 0xA0);

    size_t pos       = 0;
    const char* utf8 = name.c_str();
    for (;;) {
        char32_t unicode = Unicode::parseNextUtf8(utf8);
        if (unicode == 0 || pos >= 16) {
            break;
        }
        uint8_t petscii = PETSCII::fromUnicode(unicode, '-');
        out[pos++]      = petscii;
    }
    while (pos < 16) {
        out[pos++] = 0xa0;
    } // end of filename padding
    return out;
}


// --- Writing D64 structure -> image
// static Bam read_bam_from_image(const std::vector<uint8_t>& img) {
//     Bam bam;
//     size_t bam_off = ts_offset(18, 0);
//     // bytes 4.. correspond to track 1 map?  (BAM layout: offset 4 + (track-1)*4 gives [free count][map0][map1][map2])
//     for (int t = 1; t <= TRACKS; ++t) {
//         size_t base = bam_off + 4 + (t - 1) * 4;
//         if (base + 3 >= img.size()) {
//             break;
//         }
//         bam.freeCount[t] = img[base + 0];
//         bam.map[t][0]    = img[base + 1];
//         bam.map[t][1]    = img[base + 2];
//         bam.map[t][2]    = img[base + 3];
//     }
//     return bam;
// }

static void write_bam_to_image(std::vector<uint8_t>& img, const Bam& bam, const std::string& diskName) {
    size_t bam_off = ts_offset(18, 0);
    // write header: disk id / dos type left as zeros typically. We'll write disk name at offset 0x90
    std::vector<uint8_t> name_petscii = ascii_name_to_petscii16(diskName);
    for (size_t i = 0; i < name_petscii.size(); ++i) {
        img[bam_off + 0x90 + i] = name_petscii[i];
    }

    // Write BAM entries starting offset +4
    for (int t = 1; t <= TRACKS; ++t) {
        size_t base   = bam_off + 4 + (t - 1) * 4;
        img[base + 0] = bam.freeCount[t];
        img[base + 1] = bam.map[t][0];
        img[base + 2] = bam.map[t][1];
        img[base + 3] = bam.map[t][2];
    }
}

// Initialize BAM all-free according to maximum sectors per track
static Bam make_fresh_bam() {
    Bam bam;
    for (int t = 1; t <= TRACKS; ++t) {
        int spt          = sectorsPerTrack[t - 1];
        bam.freeCount[t] = spt;
        // set bits for sectors 0..spt-1 to 1 in bitmap (LSB is sector 0)
        uint32_t mask = 0;
        for (int s = 0; s < spt; ++s) {
            mask |= (1u << s);
        }
        bam.map[t][0] = mask & 0xFF;
        bam.map[t][1] = (mask >> 8) & 0xFF;
        bam.map[t][2] = (mask >> 16) & 0xFF;
    }
    // mark directory track/sector and BAM sector as used: track 18 sector 0..2 (BAM at 18,0 and directory chain starts at 18,1)
    // We'll mark sector 0 and 1 as used
    auto mark_used = [&](int t, int s) {
        int bit = s;
        bam.map[t][bit / 8] &= ~(1u << (bit % 8));
        bam.freeCount[t] -= 1;
    };
    // 18,0 is BAM sector
    mark_used(18, 0);
    // 18,1 will be used for directory, but in some disks directory spans more sectors; we reserve sector 1 initially
    mark_used(18, 1);
    return bam;
}

// Find next free sector scanning tracks ascending, optionally skipping track 18 sector 0/1
static bool find_free_sector(const Bam& bam, int& out_t, int& out_s) {
    for (int t = 1; t <= TRACKS; ++t) {
        int spt = sectorsPerTrack[t - 1];
        for (int s = 0; s < spt; ++s) {
            // check map bit
            int byteIdx = s / 8;
            int bitIdx  = s % 8;
            if ((bam.map[t][byteIdx] >> bitIdx) & 1u) {
                out_t = t;
                out_s = s;
                return true;
            }
        }
    }
    return false;
}

// Mark a sector used in bam (modify bam in place)
static void bam_mark_used(Bam& bam, int t, int s) {
    int byteIdx = s / 8;
    int bitIdx  = s % 8;
    if (((bam.map[t][byteIdx] >> bitIdx) & 1u) == 0) {
        // already marked used
        return;
    }
    bam.map[t][byteIdx] &= ~(1u << bitIdx);
    if (bam.freeCount[t] > 0) {
        bam.freeCount[t] -= 1;
    }
}

// Write file data into image using free sectors from bam, return starting T/S
static std::pair<int, int> write_file_to_image(std::vector<uint8_t>& img, Bam& bam, const std::vector<uint8_t>& data) {
    // split data into (SECTOR_SIZE-2) byte chunks
    size_t perSector = SECTOR_SIZE - 2;
    size_t remaining = data.size();
    size_t written   = 0;
    int first_t = 0, first_s = 0;
    int prev_t = 0, prev_s = 0;
    while (remaining > 0) {
        // find a free sector
        int t, s;
        if (!find_free_sector(bam, t, s)) {
            throw "disk full while writing file";
        }
        // mark used
        bam_mark_used(bam, t, s);
        // find sector offset
        size_t off = ts_offset(t, s);
        // compute how many bytes to write in this sector chunk
        size_t towrite = std::min(perSector, remaining);
        // copy chunk into sector payload area (bytes 2..)
        // We'll write next_t/next_s later, set temporarily to 0
        // But we must write chain pointer at bytes 0 & 1: next track and next sector.
        // For now set next to 0, we'll overwrite when we know next.
        // Fill payload
        for (size_t i = 0; i < towrite; ++i) {
            img[off + 2 + i] = data[written + i];
        }
        // If this sector is not fully filled (last sector), zero rest (not strictly needed)
        for (size_t i = towrite; i < perSector; ++i) {
            img[off + 2 + i] = 0;
        }

        // link chain
        if (first_t == 0) {
            first_t = t;
            first_s = s;
        }
        if (prev_t != 0) {
            // set previous sector's next pointers to this new t,s
            size_t poff   = ts_offset(prev_t, prev_s);
            img[poff + 0] = (uint8_t)t;
            img[poff + 1] = (uint8_t)s;
        }
        prev_t = t;
        prev_s = s;

        written += towrite;
        remaining -= towrite;
    }
    // finish last sector: set next track = 0, next sector = <file-length-in-last-sector> ???
    // On 1541, the last sector's next track=0 and next sector = number of used bytes in this final sector's data area (0..254)
    // But many tools write next sector byte as count of used bytes (0..254). We'll set it accordingly.
    if (prev_t != 0) {
        size_t lastOff   = ts_offset(prev_t, prev_s);
        img[lastOff + 0] = 0; // next track = 0
        // number of used bytes in payload
        size_t lastUsed  = (data.size() == 0) ? 0 : ((data.size() - 1) % (SECTOR_SIZE - 2) + 1);
        img[lastOff + 1] = (uint8_t)lastUsed;
    }
    return { first_t, first_s };
}

// Write directory entries for d64.files
// We will write directory starting at track 18 sector 1 and allocate new directory sectors as needed.
static void write_directory_to_image(std::vector<uint8_t>& img, Bam& bam, const D64& d64, const std::vector<std::pair<int, int>>& starts) {
    // directory entries are 32 bytes, 8 per sector, chain starts at 18,1
    int dir_t = 18, dir_s = 1;
    size_t dir_off = ts_offset(dir_t, dir_s);
    // We'll manage writing directory entries sequentially; if need more sectors, allocate them and link
    int entryIndex = 0;
    int fileIndex  = 0;
    int filesCount = (int)d64.files.size();

    // We'll maintain a vector of allocated directory sectors (t,s) to compute chaining later
    std::vector<std::pair<int, int>> dirSectors;
    dirSectors.push_back({ dir_t, dir_s });

    while (fileIndex < filesCount) {
        // find next empty slot
        int sectorIdx = entryIndex / 8; // index within dirSectors
        while (sectorIdx >= (int)dirSectors.size()) {
            // allocate a new directory sector
            int nt, ns;
            if (!find_free_sector(bam, nt, ns)) {
                throw "no free sector to extend directory";
            }
            bam_mark_used(bam, nt, ns);
            dirSectors.push_back({ nt, ns });
        }
        // compute offset for the directory entry
        auto [ct, cs]   = dirSectors[sectorIdx];
        size_t coff     = ts_offset(ct, cs);
        int slot        = entryIndex % 8;
        size_t entryOff = coff + slot * 32;

        DIRENTRY* entry = (DIRENTRY*)(&img[0] + entryOff);

        memset(entry, 0, sizeof(DIRENTRY));

        // fill entry for fileIndex
        const auto& f = d64.files[fileIndex];
        // file type: write high bits including locked & closed flags - we set basic low 3 bits to indicate type
        uint8_t typeByte = 0;

        const int fl_closed = 0x80;
        const int fl_locked = 0x40;

        switch (f.type) {
        case D64::PRG: typeByte = 0x02 | fl_closed; break; // PRG closed
        case D64::SEQ: typeByte = 0x01 | fl_closed; break;
        case D64::USR: typeByte = 0x03 | fl_closed; break;
        case D64::REL: typeByte = 0x04 | fl_closed; break;
        default:       typeByte = 0x02 | fl_closed; break;
        }
        // img[entryOff + 2] = typeByte;

        entry->file_type = typeByte;

        // filename 16 bytes at +3..+18
        auto namep = ascii_name_to_petscii16(f.name);
        for (int i = 0; i < 16; ++i) {
            // img[entryOff + 3 + i] = namep[i];
            entry->file_name[i] = namep[i];
        }
        entry->file_track  = (uint8_t)starts[fileIndex].first;
        entry->file_sector = (uint8_t)starts[fileIndex].second;

        // starting track & sector go at +19 and +20 historically
        // img[entryOff + 19] = (uint8_t)starts[fileIndex].first;
        // img[entryOff + 20] = (uint8_t)starts[fileIndex].second;
        // write file size in blocks at +30..+31 little endian? Many tools store block count at +30/31
        // We'll compute number of 254-byte blocks used
        size_t perSector = SECTOR_SIZE - 2;
        size_t blocks    = (f.data.size() + perSector - 1) / perSector;
        // img[entryOff + 30] = (uint8_t)(blocks & 0xFF);
        // img[entryOff + 31] = (uint8_t)((blocks >> 8) & 0xFF);

        entry->size_low  = (uint8_t)(blocks & 0xFF);
        entry->size_high = (uint8_t)((blocks >> 8) & 0xFF);

        ++entryIndex;
        ++fileIndex;
    }

    // Now set up directory sector chaining bytes for each used directory sector
    for (size_t i = 0; i < dirSectors.size(); ++i) {
        size_t off = ts_offset(dirSectors[i].first, dirSectors[i].second);
        if (i + 1 < dirSectors.size()) {
            img[off + 0] = (uint8_t)dirSectors[i + 1].first;
            img[off + 1] = (uint8_t)dirSectors[i + 1].second;
        } else {
            img[off + 0] = 0;
            img[off + 1] = 255; // standard end-of-chain markers (track 0)
        }
    }
}

bool D64::load(std::string path) {
    FilePtr f(os);
    if (!f.fopenLocal(path, "rb")) {
        return false;
    }
    auto img = f.readAll();
    f.close();
    return load_d64_from_image(img);
}

bool D64::save(std::string path) const {
    auto img = save_d64_to_image();

    FilePtr f(os);
    if (!f.fopenLocal(path, "wb")) {
        return false;
    }
    f.write(&img[0], img.size());
    f.close();
    return true;
}

bool D64::removeFile(std::string filename) {
    for (size_t i = 0; i < files.size(); ++i) {
        if (files[i].name == filename) {
            files.erase(files.begin() + i);
            return true;
        }
    }
    return false;
}

// --- Parsing D64 image -> populate D64 structure
bool D64::load_d64_from_image(const std::vector<uint8_t>& img) {
    if (img.size() < IMAGE_SIZE) {
        return false;
        // throw "image too small or truncated";
    }

    // Read disk name from BAM track: Track 18, Sector 0, offset 0x90..0x9F (16 bytes)
    size_t bam_off = ts_offset(18, 0);
    // convert
    this->diskName = petscii_to_ascii_name(&img[0] + bam_off + 0x90);

    // Directory chain starts at Track 18, Sector 1
    int dir_t = 18, dir_s = 1;
    while (dir_t != 0) {
        size_t off = ts_offset(dir_t, dir_s);
        if (off + SECTOR_SIZE > img.size()) {
            throw "directory sector out of range";
            return false;
        }
        int next_t = img[off + 0]; // next directory entry: track & sector
        int next_s = img[off + 1];

        // 8 directory entries per sector, each 32 bytes, starting at offset + 2
        for (int ientry = 0; ientry < 8; ++ientry) {
            size_t eoff = off + ientry * 32;

            const DIRENTRY* entry = (const DIRENTRY*)(&img[0] + eoff);

            FILE f = {};
            f.name = petscii_to_ascii_name(entry->file_name);

            // file type low 3 bits determine type bits per VICE/CBM: use stored byte
            switch (entry->file_type & 0x07) {
            case 0x02: f.type = D64::PRG; break; // PRG
            case 0x01: f.type = D64::SEQ; break; // SEQ
            case 0x03: f.type = D64::USR; break;
            case 0x04: f.type = D64::REL; break;
            default:   f.type = D64::PRG; break;
            }

            // follow sector chain, using start track/sector from entry->file_track/sector
            int ft = entry->file_track;
            int fs = entry->file_sector;
            if (ft == 0) {
                // start track==0 can happen for scratched but not empty entries; skip reading data
                // continue; // or push empty file record if you want
            } else {
                std::set<std::pair<int, int>> seen_file_sectors;
                while (ft != 0) {
                    if (seen_file_sectors.count({ ft, fs })) {
                        break; // loop protection
                    }
                    seen_file_sectors.insert({ ft, fs });

                    size_t fsec     = ts_offset(ft, fs);
                    uint8_t next_ft = img[fsec + 0];
                    uint8_t next_fs = img[fsec + 1];

                    if (next_ft == 0) {
                        // final sector — next_fs is count of used bytes in payload
                        size_t used = (size_t)next_fs;
                        if (used > 0) {
                            // bounds check (used <= 254)
                            if (used > (SECTOR_SIZE - 2)) {
                                used = SECTOR_SIZE - 2;
                            }
                            f.data.insert(f.data.end(), img.begin() + fsec + 2, img.begin() + fsec + 2 + used);
                        }
                    } else {
                        // normal sector — append full 254 bytes
                        f.data.insert(f.data.end(), img.begin() + fsec + 2, img.begin() + fsec + SECTOR_SIZE);
                    }

                    // advance
                    ft = next_ft;
                    fs = next_fs;
                }
            }

            // Trim possible filler bytes appended by reading last sector fully: trailing 0x00 or 0x00.. in PRG not reliable.
            // We leave full raw bytes; user can trim to known file length if available.
            if (!f.data.empty()) {
                this->files.push_back(std::move(f));
            }
        }

        dir_t = next_t;
        dir_s = next_s;
    }

    return true;
}

std::vector<uint8_t> D64::save_d64_to_image() const {
    auto img = make_blank_image();
    // initialize BAM
    Bam bam = make_fresh_bam();

    // write BAM header and initial map later
    write_bam_to_image(img, bam, diskName);

    // For each file, write data and remember start track/sector
    std::vector<std::pair<int, int>> starts;
    for (const auto& f : files) {
        auto st = write_file_to_image(img, bam, f.data);
        starts.push_back(st);
    }

    // write directory entries
    write_directory_to_image(img, bam, *this, starts);

    // finally write BAM with updated map and disk name
    write_bam_to_image(img, bam, diskName);

    return img;
}
