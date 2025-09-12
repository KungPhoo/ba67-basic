#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <bit>

class Os;

// a very simple class for serializing data to a "text" file.
// All data is written in ascii, but values are written as hex
// bytes. For long blocks a simple RLE encoding is implemented.
// Numbers are serialized in (easier to read) big endian form.
// You 'can' write shorter integers to the file, but make sure you
// always provide a hex pair for each byte to be read.
// key names must not contain the colon ':' character. Better not
// use unicode and umlaut characters.
class RawConfig {
public:
    RawConfig(Os* pOs)
        : os(pOs) { }
    bool load(const char* filename);

    bool save(const char* filename) const;


    // == SET ==
    void set(const std::string& key, bool value) {
        data[key] = { static_cast<uint8_t>(value ? 1 : 0) };
    }
    void set(const std::string& key, const std::string& value) {
        data[key] = std::vector<uint8_t>(value.begin(), value.end());
    }

    void set(const std::string& key, int32_t value) {
        std::vector<uint8_t> bytes(4);
        toBigEndian(&value, bytes.data(), 4);
        data[key] = std::move(bytes);
    }
    void set(const std::string& key, uint32_t value) {
        std::vector<uint8_t> bytes(4);
        toBigEndian(&value, bytes.data(), 4);
        data[key] = std::move(bytes);
    }
    void set(const std::string& key, int64_t value) { setI64(key, value); }
    void set(const std::string& key, uint64_t value) { setI64(key, int64_t(value)); }

private:
    void setI64(const std::string& key, int64_t value) {
        std::vector<uint8_t> bytes(8);
        toBigEndian(&value, bytes.data(), 8);
        data[key] = std::move(bytes);
    }

public:
    void set(const std::string& key, const void* ptr, size_t len) {
        const uint8_t* p = static_cast<const uint8_t*>(ptr);
        data[key]        = std::vector<uint8_t>(p, p + len);
    }

    // == GET ==
    bool get(const std::string& key, bool& value) const {
        auto it = data.find(key);
        if (it == data.end() || it->second.empty()) {
            return false;
        }
        value = (it->second[0] != 0);
        return true;
    }
    bool get(const std::string& key, std::string& value) const {
        auto it = data.find(key);
        if (it == data.end()) {
            return false;
        }
        value = std::string(it->second.begin(), it->second.end());
        return true;
    }

    bool get(const std::string& key, int32_t& value) const {
        auto it = data.find(key);
        if (it == data.end()) { //|| it->second.size() != 4) {
            return false;
        }
        int32_t v;
        fromBigEndian(it->second, &v, 4);
        value = v;
        return true;
    }
    bool get(const std::string& key, uint32_t& value) const {
        auto it = data.find(key);
        if (it == data.end()) { //|| it->second.size() != 4) {
            return false;
        }
        int32_t v;
        fromBigEndian(it->second, &v, 4);
        value = v;
        return true;
    }
    bool get(const std::string& key, int64_t& value) const {
        int64_t i64 = value;
        if (getI64(key, i64)) {
            value = i64;
            return true;
        }
        return false;
    }
    bool get(const std::string& key, uint64_t& value) const {
        int64_t i64 = value;
        if (getI64(key, i64)) {
            value = i64;
            return true;
        }
        return false;
    }


    size_t get(const std::string& key, void* out, size_t maxlen) const;

private:
    int64_t getI64(const std::string& key, int64_t def = 0) const {
        auto it = data.find(key);
        if (it == data.end() || it->second.size() < 8) {
            return def;
        }
        int64_t v;
        fromBigEndian(it->second, &v, 8);
        return v;
    }

private:
    template <typename T>
    static void toBigEndian(const T* src, void* dst, size_t size) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(src);
        uint8_t* d       = reinterpret_cast<uint8_t*>(dst);
        for (size_t i = 0; i < size; ++i) {
            d[i] = p[i]; // works on BE
        }

        if (std::endian::native == std::endian::little) {
            // reverse to big-endian
            for (size_t i = 0; i < size / 2; ++i) {
                std::swap(d[i], d[size - 1 - i]);
            }
        }
    }

    template <typename T>
    static void fromBigEndian(std::vector<uint8_t> vsrc, T* dst, size_t size) {
        while (size > vsrc.size()) {
            vsrc.insert(vsrc.begin(), 0);
        }
        const void* src = vsrc.data();

        const uint8_t* s = reinterpret_cast<const uint8_t*>(src);
        uint8_t* d       = reinterpret_cast<uint8_t*>(dst);
        for (size_t i = 0; i < size; ++i) {
            d[i] = s[i];
        }
        if (std::endian::native == std::endian::little) {
            for (size_t i = 0; i < size / 2; ++i) {
                std::swap(d[i], d[size - 1 - i]);
            }
        }
    }

    static bool encodeRLE(const std::vector<uint8_t>& in, std::vector<uint8_t>& out);


    static std::vector<uint8_t> decodeRLE(const std::vector<uint8_t>& in);

    Os* os = nullptr;

    std::map<std::string, std::vector<uint8_t>> data;
};
