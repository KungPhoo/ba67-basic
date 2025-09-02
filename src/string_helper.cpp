#include "string_helper.h"
#include <cstring>

std::vector<std::string> StringHelper::split(const std::string& in, const std::string& delim) {
    std::string::size_type firstPos  = in.find_first_not_of(delim);
    std::string::size_type secondPos = in.find_first_of(delim, firstPos);
    std::vector<std::string> vout;
    vout.clear();
    if (firstPos != std::string::npos) {
        vout.push_back(in.substr(firstPos, secondPos - firstPos));
    }
    while (secondPos != std::string::npos) {
        firstPos = in.find_first_not_of(delim, secondPos);
        if (firstPos == std::string::npos) {
            break;
        }
        secondPos = in.find_first_of(delim, firstPos);
        vout.push_back(in.substr(firstPos, secondPos - firstPos));
    }
    return vout;
}

void StringHelper::trimLeft(std::string& str, const char* delims = " \r\n\t") {
    str.erase(0, str.find_first_not_of(delims));
}

void StringHelper::trimRight(std::string& str, const char* delims = " \r\n\t") {
    str.erase(str.find_last_not_of(delims) + 1);
}

bool StringHelper::replace(std::string& s, const std::string& fnd, const std::string& repl) {
    if (fnd.empty()) {
        return false;
    }

    bool brepl   = false;
    size_t b     = 0;
    size_t fndsz = fnd.size();
    size_t repsz = repl.size();
    for (;;) {
        b = s.find(fnd, b);
        if (b == s.npos) {
            break;
        }
        s.replace(b, fndsz, repl);
        b += repsz;
        brepl = true;
    }
    return brepl;
}

int StringHelper::strncmp(const char* text, const char* find, size_t count) {
    for (size_t i = 0; i < count; i++) {
        unsigned char c1 = (unsigned char)text[i];
        unsigned char c2 = (unsigned char)find[i];

        if (c1 != c2) {
            return c1 - c2;
        }
        if (c1 == '\0') {
            return 0; // both must be '\0' since c1 == c2
        }
    }
    return 0;
}

size_t StringHelper::strlen(const char* text) {
    const char* p = text;
    while (*p != '\0') {
        ++p;
    }
    return p - text;
}

int StringHelper::strcmp(const char* text, const char* find) {
    size_t i = 0;
    while (text[i] != '\0' && find[i] != '\0') {
        if (text[i] != find[i]) {
            return (unsigned char)text[i] - (unsigned char)find[i];
        }
        i++;
    }
    return (unsigned char)text[i] - (unsigned char)find[i];
}

const char* StringHelper::strchr(const char* text, char find) {
    while (*text) {
        if (*text == find) {
            return text;
        }
        text++;
    }
    return (find == '\0') ? text : nullptr;
}

const char* StringHelper::strstr(const char* haystack, const char* needle) {
    if (!*needle) {
        return haystack;
    }

    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) {
            return haystack;
        }
    }
    return nullptr;
}

char* StringHelper::strtok_r(char* str, const char* delim, char** saveptr) {
    auto is_delim = [&](char c) {
        for (const char* d = delim; *d; ++d) {
            if (c == *d) {
                return true;
            }
        }
        return false;
    };

    if (!str) {
        str = *saveptr;
    }

    while (*str && is_delim(*str)) {
        ++str;
    }
    if (!*str) {
        if (saveptr) {
            *saveptr = str;
        }
        return nullptr;
    }

    char* token_start = str;
    while (*str && !is_delim(*str)) {
        ++str;
    }

    if (*str) {
        *str++ = '\0';
    }
    if (saveptr) {
        *saveptr = str;
    }
    return token_start;
}

void StringHelper::memcpy(void* dst, const void* src, size_t len) {
#if 1 // def __EMSCRIPTEN__
    std::memcpy(dst, src, len);
#else // most definitely slower than built in
    auto* d = static_cast<unsigned char*>(dst);
    auto* s = static_cast<const unsigned char*>(src);

    // Small copy optimization
    if (len < 16) {
        while (len--) {
            *d++ = *s++;
        }
        return dst;
    }

    // Align destination to 8 bytes
    while ((reinterpret_cast<uintptr_t>(d) & 7) && len > 0) {
        *d++ = *s++;
        --len;
    }

    // Bulk copy 64 bits at a time
    auto* dw = reinterpret_cast<uint64_t*>(d);
    auto* sw = reinterpret_cast<const uint64_t*>(s);

    size_t n = len / 8;
    for (size_t i = 0; i < n; ++i) {
        dw[i] = sw[i];
    }

    // Advance pointers
    d += n * 8;
    s += n * 8;
    len -= n * 8;

    // Tail
    while (len--) {
        *d++ = *s++;
    }

    return dst;
#endif
}

void StringHelper::strcpy(char* dest, const char* src) {
    for (;;) {
        *dest = *src;
        if (*src == '\0') {
            break;
        }
        ++dest;
        ++src;
    }
}

// returns a 16 characters long string with '0' padding.
std::string StringHelper::int2hex(int64_t n, bool uppercase) {
    static const char* digitsU = "0123456789ABCDEF";
    static const char* digitsL = "0123456789abcdef";
    const char* digits         = uppercase ? digitsU : digitsL;
    size_t hex_len             = sizeof(n) << 1;
    std::string rc(hex_len, '0');
    for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
        rc[i] = digits[(n >> j) & 0x0f];
    }
    return rc;
}
