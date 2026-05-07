#pragma once
#include <string>

#include "value.h"
#include "error.h"

namespace BA67 {


// Token types
enum class TokenType {
    NUMBER,
    INTEGER,
    STRING,
    IDENTIFIER /*variable name*/,
    COMMA,
    OPERATOR,
    UNARY_OPERATOR,
    KEYWORD,
    COMMAND,
    PARENTHESIS,
    MODULE,
    FILEHANDLE,

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

}; // namespace