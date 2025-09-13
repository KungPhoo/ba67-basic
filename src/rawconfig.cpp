#include "rawconfig.h"
#include "fileptr.h"

#include <cstdio>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

#include "string_helper.h"

bool RawConfig::load(const char* filename) {
    FilePtr f(os);
    f.open(filename, "r");
    if (!f) {
        return false;
    }
    data.clear();
    size_t len = 0;


    for (;;) {
        std::string s(f.getline());
        if (s.empty()) {
            break;
        }
        if (s.back() == '\n') {
            s.pop_back();
        }
        auto colon = s.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = s.substr(0, colon);

        bool RLE = false;
        if (colon + 1 < s.length() && s.at(colon + 1) == 'R') {
            ++colon;
            RLE = true;
        }

        std::vector<uint8_t> bytes;
        for (const char* hex = s.c_str() + (colon + 1); hex[0] != '\0' && hex[1] != '\0'; hex += 2) {
            unsigned int b;
            b = (StringHelper::hexchartoi(hex[0]) << 4) | StringHelper::hexchartoi(hex[1]);
            bytes.push_back(static_cast<uint8_t>(b));
        }
        if (RLE) {
            data[key] = decodeRLE(bytes);
        } else {
            data[key] = bytes;
        }
    }
    f.close();
    return true;
}

bool RawConfig::save(const char* filename) const {
    FilePtr f(os);
    f.open(filename, "w");
    if (!f) {
        return false;
    }
    for (auto& [key, bytes] : data) {
        f.printf("%s:", key.c_str());

        std::vector<uint8_t> encoded;
        if (encodeRLE(bytes, encoded)) {
            f.printf("R");
            for (uint8_t b : encoded) {
                f.printf("%02X", b);
            }
        } else {
            for (uint8_t b : encoded) {
                f.printf("%02X", b);
            }
        }
        f.printf("\n");
    }
    f.close();
    return true;
}

size_t RawConfig::get(const std::string& key, void* out, size_t count, size_t bytesPerItem) const {
    auto it = data.find(key);
    if (it == data.end()) {
        return 0;
    }

    size_t n = (it->second.size() / bytesPerItem < count) ? it->second.size() / bytesPerItem : count;

    const char* src = (const char*)it->second.data();
    char* dst       = (char*)out;
    for (size_t i = 0; i < n; ++i) {
        fromBigEndian(src, dst, bytesPerItem);
        src += bytesPerItem;
        dst += bytesPerItem;
    }
    return n;
}

bool RawConfig::encodeRLE(const std::vector<uint8_t>& in, std::vector<uint8_t>& out) {
    if (in.size() <= 8) {
        out = in;
        return false;
    }

    out.clear();
    if (in.empty()) {
        return false;
    }
    size_t i = 0;
    while (i < in.size()) {
        uint8_t value = in[i];
        size_t run    = 1;
        while (i + run < in.size() && in[i + run] == value && run < 255) {
            run++;
        }
        out.push_back(uint8_t(run));
        out.push_back(value);
        i += run;
    }
    if (out.size() < in.size()) {
        return true;
    }
    out = in;
    return false;
}

std::vector<uint8_t> RawConfig::decodeRLE(const std::vector<uint8_t>& in) {
    std::vector<uint8_t> out;
    for (size_t i = 0; i + 1 < in.size(); i += 2) {
        uint8_t run   = in[i];
        uint8_t value = in[i + 1];
        out.insert(out.end(), run, value);
    }
    return out;
}
