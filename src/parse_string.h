#pragma once
#include <string>

namespace BA67 {

bool isEndOfWord(char c);
bool isNumeric(char c);
inline bool isWhiteSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
const char* skipWhite(const char*& str);
bool parseDouble(const char*& str, double* number = nullptr);
bool parseInt(const char*& str, int64_t* number = nullptr); // int - not a double! "1.23" returns false
bool parseGotoInt(const char*& str, int64_t* number = nullptr);
bool parseFileHandle(const char*& str, std::string_view* number = nullptr);
int64_t strToInt(const std::string& str); // parses "255" and "$ff"
int64_t strToInt(const std::string_view& str); // parses "255" and "$ff"

};