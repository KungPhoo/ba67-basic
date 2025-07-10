#include "string_helper.h"

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

        if (c1 != c2)
            return c1 - c2;
        if (c1 == '\0')
            return 0; // both must be '\0' since c1 == c2
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
    if (!*needle)
        return haystack;

    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n)
            return haystack;
    }
    return nullptr;
}

char* StringHelper::strtok_r(char* str, const char* delim, char** saveptr) {
    auto is_delim = [&](char c) {
        for (const char* d = delim; *d; ++d)
            if (c == *d)
                return true;
        return false;
    };

    if (!str)
        str = *saveptr;

    while (*str && is_delim(*str))
        ++str;
    if (!*str)
        return *saveptr = str, nullptr;

    char* token_start = str;
    while (*str && !is_delim(*str))
        ++str;

    if (*str)
        *str++ = '\0';
    *saveptr = str;
    return token_start;
}

void StringHelper::memcpy(void* dst, const void* src, size_t len) {
    uintptr_t dst_addr = reinterpret_cast<uintptr_t>(dst);
    uintptr_t src_addr = reinterpret_cast<uintptr_t>(src);

    // Fast path: 8-byte alignment and size
    if (((dst_addr | src_addr | len) & 0x7) == 0) {
        int64_t* idst       = reinterpret_cast<int64_t*>(dst);
        const int64_t* isrc = reinterpret_cast<const int64_t*>(src);
        int64_t* end        = idst + (len >> 3);

        while (idst != end) {
            *idst++ = *isrc++;
        }
    } else {
        // Fallback: byte-by-byte copy
        char* cdst       = reinterpret_cast<char*>(dst);
        const char* csrc = reinterpret_cast<const char*>(src);
        char* end        = cdst + len;

        while (cdst != end) {
            *cdst++ = *csrc++;
        }
    }
}

void StringHelper::strcpy(char* dest, const char* src) {
    for (;;) {
        *dest = *src;
        if (*src == '\0')
            break;
        ++dest;
        ++src;
    }
}
