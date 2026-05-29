#pragma once
#include <string>
#include <map>

#include "value.h"
#include "error.h"
#include <vector>

namespace BA67 {


// Token types
enum class TokenType {
    NUMBER, // floating point
    INTEGER,
    STRING,
    IDENTIFIER, // variable
    COMMA,    // ','
    OPERATOR, // AND NOT OR + - * / ^ ;
    UNARY_OPERATOR, // -
    KEYWORD, // IF FOR GOTO ...
    COMMAND, // PRINT INPUT ...
    PARENTHESIS, // ( )
    MODULE,
    FILEHANDLE, // #1

    END
};

// Token structure
struct Token {
    TokenType type = TokenType::END;
    std::string_view valueForDebugging;
    Value tv;


private:
    const std::string* strOrNull() const {
        if (auto* s = std::get_if<std::string>(&tv)) {
            return s;
        }
        if (auto* op = std::get_if<Operator>(&tv)) {
            return &op->value;
        }
        return nullptr;
    }

public:
    const std::string& str() const {
        auto* s = strOrNull();
        if (s != nullptr) {
            return *s;
        }
        throw ErrorId::INTERNAL;
    }


    bool is(char c) const {
        auto* s = strOrNull();
        if (s == nullptr) {
            return false;
        }
        return s->length() == 1 && (*s)[0] == c;
    }
    bool is(const std::string_view& str) const {
        auto* s = strOrNull();
        if (s == nullptr) {
            return false;
        }
        return (*s) == str;
    }

    // returns '#', '%', '$'
    char valuePostfix() const {
        if (type == TokenType::INTEGER) {
            return '%';
        }
        if (type == TokenType::NUMBER) {
            return '#';
        }
        if (type == TokenType::STRING) {
            return '$';
        }
        if (auto* s = std::get_if<std::string>(&tv)) {
            if (s->ends_with('$')) {
                return '$';
            }
            if (s->ends_with('%')) {
                return '%';
            }
        }
        return '#';
    }
};


struct ProgramLine {
    ProgramLine() = default;
    ProgramLine(const ProgramLine& l)
        : ProgramLine() { *this = l; }
    ProgramLine& operator=(const ProgramLine& l) {
        code = l.code;
        return *this;
    }
    std::string code;
    std::vector<std::vector<Token>> tokens; // tokens for each command. Has string_views to code
    operator std::string&() { return code; }
};


}; // namespace