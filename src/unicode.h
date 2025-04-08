#pragma once
#include <string>

class Unicode {
public:
    // parses next unicode code point from utf8 string. Advances 'utf8'. Returns 0 on error.
    static char32_t parseNextUtf8(const char*& utf8);

    // append unicode code point to utf8 string
    static void appendAsUtf8(std::string& out, char32_t unicodeCodepoint);

    // convert uint32_t* to utf8 string
    static std::string toUtf8String(const char32_t* str);
    static std::string toUtf8String(const char16_t* str);

    // convert utf8 string to u32string
    static bool toU32String(const char* utf8String, std::u32string& result);
    static bool toU16String(const char* utf8String, std::u16string& result);

    // code point length of utf8 string
    static size_t utf8StrLen(const char* utf8);

    // uppercase a unicode code point (A-Z)
    static char32_t toUpperAscii(char32_t c);
    static char32_t toLowerAscii(char32_t c);
    // uppercase a utf8 string (A-Z)
    static std::string toUpperAscii(const char* utf8);
    static std::string toLowerAscii(const char* utf8);

    // true Unicode conversion - might be slow!
    static char32_t toUpper(char32_t c);
    static char32_t toLower(char32_t c);
    static std::string toUpper(const char* utf8);
    static std::string toLower(const char* utf8);

    static std::string substr(const std::string& utf8, size_t startCodePoint, size_t length = ~size_t(0));

    static std::string::size_type strstr(const std::string& utf8, const std::string& utf8Find, size_t startCodePoint);
    static bool wildcardMatch(const char32_t* str, const char32_t* pattern);
};
