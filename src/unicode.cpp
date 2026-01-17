#include "unicode.h"
#include <codecvt>
#include <cstring>
#include <locale>
#include "string_helper.h"

// parse the next utf8 character sequence from a string pointer.
// advance the referenced string pointer.
// return the Unicode character or '?' when it fails.
// this stops at a nul character
char32_t Unicode::parseNextUtf8(const char*& utf8) {
    char32_t codepoint = 0;
    if (static_cast<unsigned char>(static_cast<unsigned char>(*utf8)) < 0x80) {
        // 1-byte sequence (ASCII)
        codepoint = static_cast<unsigned char>(*utf8);
        ++utf8;
    } else if ((static_cast<unsigned char>(*utf8) & 0xE0) == 0xC0) {
        // 2-byte sequence
        codepoint = static_cast<unsigned char>(*utf8) & 0x1F;
        ++utf8;
        if ((static_cast<unsigned char>(*utf8) & 0xC0) != 0x80) {
            return 0;
        } // Invalid UTF-8 sequence
        codepoint = (codepoint << 6) | (static_cast<unsigned char>(*utf8) & 0x3F);
        ++utf8;
    } else if ((static_cast<unsigned char>(*utf8) & 0xF0) == 0xE0) {
        // 3-byte sequence
        codepoint = static_cast<unsigned char>(*utf8) & 0x0F;
        ++utf8;
        for (int i = 0; i < 2; ++i) {
            if ((static_cast<unsigned char>(*utf8) & 0xC0) != 0x80) {
                return 0;
            } // Invalid UTF-8 sequence
            codepoint = (codepoint << 6) | (static_cast<unsigned char>(*utf8) & 0x3F);
            ++utf8;
        }
    } else if ((static_cast<unsigned char>(*utf8) & 0xF8) == 0xF0) {
        // 4-byte sequence
        codepoint = static_cast<unsigned char>(*utf8) & 0x07;
        ++utf8;
        for (int i = 0; i < 3; ++i) {
            if ((static_cast<unsigned char>(*utf8) & 0xC0) != 0x80) {
                return 0;
            } // Invalid UTF-8 sequence
            codepoint = (codepoint << 6) | (static_cast<unsigned char>(*utf8) & 0x3F);
            ++utf8;
        }
    } else {
        if (*utf8 != '\0') {
            ++utf8;
        }
        return U'?'; // Invalid UTF-8 sequence
    }

    return codepoint;
}

// append a Unicode code point to an utf8 string
void Unicode::appendAsUtf8(std::string& out, char32_t unicodeCodepoint) {
    if (unicodeCodepoint <= 0x7f) {
        out.append(1, static_cast<char>(unicodeCodepoint));
    } else if (unicodeCodepoint <= 0x7ff) {
        out.append(1, static_cast<char>(0xc0 | ((unicodeCodepoint >> 6) & 0x1f)));
        out.append(1, static_cast<char>(0x80 | (unicodeCodepoint & 0x3f)));
    } else if (unicodeCodepoint <= 0xffff) {
        out.append(1, static_cast<char>(0xe0 | ((unicodeCodepoint >> 12) & 0x0f)));
        out.append(1, static_cast<char>(0x80 | ((unicodeCodepoint >> 6) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | (unicodeCodepoint & 0x3f)));
    } else {
        out.append(1, static_cast<char>(0xf0 | ((unicodeCodepoint >> 18) & 0x07)));
        out.append(1, static_cast<char>(0x80 | ((unicodeCodepoint >> 12) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | ((unicodeCodepoint >> 6) & 0x3f)));
        out.append(1, static_cast<char>(0x80 | (unicodeCodepoint & 0x3f)));
    }
}

// convert UCS-32 string to utf8 string
std::string Unicode::toUtf8String(const char32_t* str) {
    std::string utf8;
    while (*str != 0) {
        appendAsUtf8(utf8, *str++);
    }
    return utf8;
}

// convert UCS-16 string to utf8 string
std::string Unicode::toUtf8String(const char16_t* str) {
    if (!str) {
        return std::string();
    }

    std::string utf8;
    while (*str) {
        char32_t codepoint;

        // Decode UTF-16 to codepoint
        if (*str >= 0xD800 && *str <= 0xDBFF) { // High surrogate
            char16_t high = *str++;
            if (*str >= 0xDC00 && *str <= 0xDFFF) { // Low surrogate
                char16_t low = *str++;
                codepoint    = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
            } else {
                return ""; // Invalid UTF-16 sequence
            }
        } else {
            codepoint = char32_t(*str++);
        }
        // Encode codepoint to UTF-8
        if (codepoint <= 0x7F) {
            utf8.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            utf8.push_back(0xC0 | ((codepoint >> 6) & 0x1F));
            utf8.push_back(0x80 | (codepoint & 0x3F));
        } else if (codepoint <= 0xFFFF) {
            utf8.push_back(0xE0 | ((codepoint >> 12) & 0x0F));
            utf8.push_back(0x80 | ((codepoint >> 6) & 0x3F));
            utf8.push_back(0x80 | (codepoint & 0x3F));
        } else if (codepoint <= 0x10FFFF) {
            utf8.push_back(0xF0 | ((codepoint >> 18) & 0x07));
            utf8.push_back(0x80 | ((codepoint >> 12) & 0x3F));
            utf8.push_back(0x80 | ((codepoint >> 6) & 0x3F));
            utf8.push_back(0x80 | (codepoint & 0x3F));
        }
    }
    return utf8;
}

// convert utf8 string to UCS-32 string
bool Unicode::toU32String(const char* utf8String, std::u32string& result) {
    result.clear();
    size_t len = StringHelper::strlen(utf8String);
    if (result.capacity() < len) {
        result.reserve(len);
    }
    while (*utf8String != 0) {
        char32_t codepoint = parseNextUtf8(utf8String);
        if (codepoint == 0) {
            return false;
        }
        result.push_back(codepoint);
    }
    return true;
}

// convert utf8 string to UCS-16 string
bool Unicode::toU16String(const char* utf8String, std::u16string& result) {
    result.clear();
    size_t len = StringHelper::strlen(utf8String);
    if (result.capacity() < len) {
        result.reserve(len);
    }

    while (*utf8String != 0) {
        char32_t codepoint = parseNextUtf8(utf8String);
        if (codepoint == 0) {
            return false;
        }

        if (codepoint <= 0xFFFF) {
            // Directly store BMP characters
            result.push_back(static_cast<char16_t>(codepoint));
        } else if (codepoint <= 0x10FFFF) {
            // Convert to surrogate pair
            codepoint -= 0x10000;
            result.push_back(static_cast<char16_t>((codepoint >> 10) + 0xD800)); // High surrogate
            result.push_back(static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00)); // Low surrogate
        } else {
            return false; // Invalid UTF-8 sequence
        }
    }

    return true;
}

// Get length of utf8 string in Unicode code points.
// This version stops at null characters!
size_t Unicode::utf8StrLen(const char* utf8) {
    size_t len = 0;
    while (*utf8 != '\0') {
        if (parseNextUtf8(utf8) == 0) {
            break;
        }
        ++len;
    }
    return len;
}

// Get length of utf8 string in Unicode code points.
// This version even works with null characters.
size_t Unicode::utf8StrLen(const std::string& utf8) {
    const char* ptr = utf8.c_str();
    const char* end = ptr + utf8.length();

    size_t len = 0;
    while (ptr < end) {
        if (*ptr == '\0') {
            ++ptr;
        } else if (parseNextUtf8(ptr) == 0) {
            break;
        }
        ++len;
    }
    return len;
}

// uppercase a Unicode character, but only A-Z characters
char32_t Unicode::toUpperAscii(char32_t c) {
    if (c >= 'a' && c <= 'z') {
        c += 'A' - 'a';
    }
    return c;
}

// lowercase a Unicode character, but only A-Z characters
char32_t Unicode::toLowerAscii(char32_t c) {
    if (c >= 'A' && c <= 'Z') {
        c += 'a' - 'A';
    }
    return c;
}

// uppercase a string, but only A-Z characters
std::string Unicode::toUpperAscii(const char* utf8) {
    std::string str;
    while (*utf8 != '\0') {
        char32_t cp = parseNextUtf8(utf8);
        if (cp == 0) {
            break;
        }
        appendAsUtf8(str, Unicode::toUpperAscii(cp));
    }
    return str;
}

// lowercase a string, but only A-Z characters
std::string Unicode::toLowerAscii(const char* utf8) {
    std::string str;
    while (*utf8 != '\0') {
        char32_t cp = parseNextUtf8(utf8);
        if (cp == 0) {
            break;
        }
        appendAsUtf8(str, Unicode::toLowerAscii(cp));
    }
    return str;
}

// extern from unicode_case.cpp
extern char32_t ucase32(char32_t p);
extern char32_t lcase32(char32_t p);

// uppercase a Unicode character
char32_t Unicode::toUpper(char32_t c) {
    return ucase32(c);
}
// lowercase a Unicode character
char32_t Unicode::toLower(char32_t c) {
    return lcase32(c);
}

// uppercase an utf8 string
std::string Unicode::toUpper(const char* utf8) {
    std::string str;
    while (*utf8 != '\0') {
        char32_t cp = parseNextUtf8(utf8);
        if (cp == 0) {
            break;
        }
        appendAsUtf8(str, Unicode::toUpper(cp));
    }
    return str;
}

// lowercase an utf8 string
std::string Unicode::toLower(const char* utf8) {
    std::string str;
    while (*utf8 != '\0') {
        char32_t cp = parseNextUtf8(utf8);
        if (cp == 0) {
            break;
        }
        appendAsUtf8(str, Unicode::toLower(cp));
    }
    return str;
}

// uppercase a Unicode string
void Unicode::toUpper(std::string& utf8) {
    std::string tmp;
    char* str = utf8.data();
    while (*str != '\0') {
        char* start = str;
        char32_t cp = parseNextUtf8((const char*&)str);
        if (cp == 0) {
            break;
        }
        tmp.clear();
        appendAsUtf8(tmp, Unicode::toUpper(cp));
        if (str - start == tmp.length()) {
            memcpy(start, tmp.c_str(), tmp.length());
        } else {
            throw "case changes utf8 length!?";
        }
    }
}

// lowercase a Unicode string
void Unicode::toLower(std::string& utf8) {
    std::string tmp;
    char* str = utf8.data();
    while (*str != '\0') {
        char* start = str;
        char32_t cp = parseNextUtf8((const char*&)str);
        if (cp == 0) {
            break;
        }
        tmp.clear();
        appendAsUtf8(tmp, Unicode::toLower(cp));
        if (str - start == tmp.length()) {
            memcpy(start, tmp.c_str(), tmp.length());
        } else {
            throw "case changes utf8 length!?";
        }
    }
}


// Finds utf8Find in utf8 starting at a specific Unicode code point index.
std::string::size_type Unicode::strstr(const std::string& utf8, const std::string& utf8Find, size_t startCodePoint) {
    if (utf8Find.empty()) {
        return 0; // Empty substring matches at start
    }

    const char* search = utf8.c_str();
    const char* target = utf8Find.c_str();

    // Advance search to startCodePoint
    size_t currentCodePoint = 0;
    while (*search && currentCodePoint < startCodePoint) {
        if (!parseNextUtf8(search)) {
            return std::string::npos;
        }
        currentCodePoint++;
    }

    while (*search) {
        const char* searchPos = search;
        const char* findPos   = target;

        while (*findPos) {
            const char* tempSearch = searchPos;
            char32_t searchCp      = parseNextUtf8(tempSearch);
            char32_t findCp        = parseNextUtf8(findPos);

            if (searchCp == 0 || searchCp != findCp) {
                break;
            }
            searchPos = tempSearch;
        }

        if (*findPos == '\0') { // Found match
            return currentCodePoint; // search - utf8.c_str();
        }

        // Move to next code point in search string
        if (!parseNextUtf8(search)) {
            break;
        }
        ++currentCodePoint;
    }

    return std::string::npos;
}

// return substring. Can deal with null characters.
std::string Unicode::substr(const std::string& utf8, size_t startCodePoint, size_t length) {
    std::string rv;
    const char* str = utf8.c_str();
    const char* end = str + utf8.length();
    for (size_t i = 0; str != end; ++i) {
        char32_t c = 0;
        if (*str == '\0') {
            ++str;
        } else {
            c = parseNextUtf8(str);
            if (c == 0) {
                break;
            }
        }
        if (i >= startCodePoint && i - startCodePoint < length) {
            appendAsUtf8(rv, c);
        }
    }
    return rv;
}

// check if a wildcard pattern matches a string
bool Unicode::wildcardMatch(const char32_t* str, const char32_t* pattern) {
    const char32_t *s = nullptr, *p = nullptr;

    while (*str) {
        if (*pattern == U'*') {
            p = pattern++;
            s = str;
        } else if (*pattern == U'?' || *pattern == *str) {
            pattern++;
            str++;
        } else if (p != nullptr) {
            pattern = p + 1;
            str     = ++s;
        } else {
            return false;
        }
    }

    while (*pattern == U'*') {
        pattern++;
    }
    return *pattern == U'\0';
}

// check if a wildcard pattern matches a string, ignoring case sensitivity
bool Unicode::wildcardMatchNoCase(const char32_t* str, const char32_t* pattern) {
    const char32_t *s = nullptr, *p = nullptr;

    while (*str) {
        if (*pattern == U'*') {
            p = pattern++;
            s = str;
        } else if (*pattern == U'?' || toLower(*pattern) == toLower(*str)) {
            pattern++;
            str++;
        } else if (p != nullptr) {
            pattern = p + 1;
            str     = ++s;
        } else {
            return false;
        }
    }

    while (*pattern == U'*') {
        pattern++;
    }
    return *pattern == U'\0';
}
