#include "parse_string.h"
#include "string_helper.h"
#include "error.h"

namespace BA67 {

bool isEndOfWord(char c) {
    return (
        (c >= 0 && c < '0') // operators, space, newline, tab, quotes, braces
        || (c >= ':' && c <= '@') // colons, comparators
        || (c >= '{' && c <= 0x7f) // curly braces, or-operator
        // || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ':' || c == ';' || c == '\"' || c == '\0'
    );
}
bool isNumeric(char c) {
    return c >= '0' && c <= '9';
}

const char* skipWhite(const char*& str) {
    while (isWhiteSpace(*str)) {
        ++str;
    }
    return str;
}

// positive double value
bool parseDouble(const char*& str, double* number) {
    skipWhite(str);

    char* end;
    double d = strtod(str, &end);
    if (number) {
        *number = d;
    }
    if (end > str) {
        str = end;
        return true;
    }

    if (/* options.dotAsZero && */ *str == '.') {
        ++str;
        if (number) {
            *number = 0.0;
        }
        return true;
    }

    return false;
}

// positive int - not a double! "1.23" returns false
 bool parseInt(const char*& str, int64_t* number) {
    skipWhite(str);
    char* endInt;

    int base = 10;
    if (*str == '$') {
        ++str;
        base = 16;
    }

    int64_t i = strtoll(str, &endInt, base);
    if (number) {
        *number = i;
    }

    if (endInt == str) {
        return false;
    }

    // might be a double?
    if (base == 10) {
        const char* endDbl = str;
        if (!parseDouble(endDbl)) {
            throw Error(ErrorId::SYNTAX);
        }
        if (endDbl > endInt) {
            return false;
        }
    }
    str = endInt;
    return true;
}

bool parseGotoInt(const char*& str, int64_t* number) {
    skipWhite(str);
    char* endInt;

    int64_t i = strtoll(str, &endInt, 10);
    if (number) {
        *number = i;
    }
    if (endInt == str) {
        return false;
    }
    while (!isEndOfWord(*str)) {
        ++str;
    }

    str = endInt;
    return true;
}

bool parseFileHandle(const char*& str, std::string_view* number) {
    skipWhite(str);
    char* endInt;
    if (*str == '#') {
        int64_t i = strtoll(str + 1, &endInt, 10);
        if (number) {
            std::string_view s((const char*)(str) + 1, (const char*)(endInt));
            *number = s;
        }
        if (endInt == str + 1) {
            return false;
        }
        str = endInt;
        return true;
    }
    return false;
}

int64_t strToInt(const std::string& str) {
    return strToInt(std::string_view(str));
}
int64_t strToInt(const std::string_view& str) {
    if (str[0] == '$' && str[1] != '\0') { // str.starts_with("$") && str.length() > 1
        return StringHelper::strtoi64_hex(str.substr(1));
    } else if (str[0] != '\0') { // str.length() > 0
        return StringHelper::strtoi64(str);
    }
    return 0;
}


};