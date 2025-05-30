﻿#include "string_helper.h"

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
