#include <string>
#include <vector>

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

    static std::string int2hex(int64_t n);
};