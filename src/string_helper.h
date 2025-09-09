#include <string>
#include <vector>
#include <cstdint>

class StringHelper {
public:
    static std::vector<std::string> split(const std::string& in, const std::string& delim = " \t\r\n");

    static void trimLeft(std::string& str, const char* delims);

    static void trimRight(std::string& str, const char* delims);

    static bool replace(std::string& s, const std::string& fnd, const std::string& repl);

    static size_t strlen(const char* text);
    static int strncmp(const char* text, const char* find, size_t count);
    static int strcmp(const char* text, const char* find);
    static const char* strchr(const char* text, char find);
    static const char* strstr(const char* haystack, const char* needle);

    static char* strtok_r(char* str, const char* delim, char** saveptr);
    static void memcpy(void* dst, const void* src, size_t len);
    static void strcpy(char* dest, const char* src);

    static std::string int2hex(int64_t n, bool uppercase = true);




    static int64_t strtoi64(const std::string_view str) {
        const char* p   = str.data();
        const char* end = p + str.size();
        while (p < end && std::isspace(static_cast<unsigned char>(*p))) {
            ++p;
        }
        bool negative = false;
        if (p < end && (*p == '-' || *p == '+')) {
            negative = (*p == '-');
            ++p;
        }
        int64_t result = 0;
        while (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
            result = result * 10 + (*p - '0');
            ++p;
        }
        return negative ? -result : result;
    }

    static uint64_t strtou64(const std::string_view str) {
        const char* p   = str.data();
        const char* end = p + str.size();
        while (p < end && std::isspace(static_cast<unsigned char>(*p))) {
            ++p;
        }
        uint64_t result = 0;
        while (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
            result = result * 10 + (*p - '0');
            ++p;
        }
        return result;
    }

    static int64_t strtoi64_hex(const std::string_view str) {
        const char* p   = str.data();
        const char* end = p + str.size();
        while (p < end && std::isspace(static_cast<unsigned char>(*p))) {
            ++p;
        }
        // Handle optional $ prefix
        if ((end - p) >= 1 && p[0] == '$') {
            ++p;
        }
        int64_t result = 0;
        while (p < end && std::isxdigit(static_cast<unsigned char>(*p))) {
            char c    = *p;
            int digit = (c >= '0' && c <= '9') ? (c - '0')
                      : (c >= 'a' && c <= 'f') ? (c - 'a' + 10)
                                               : (c - 'A' + 10);
            result    = (result << 4) | digit;
            ++p;
        }
        return result;
    }
};