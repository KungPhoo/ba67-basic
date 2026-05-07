#include "value.h"
#include "error.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "parse_string.h"

using namespace BA67;
namespace BA67{

// Represent value as string
std::string ValueToString(const Value& v) {
    if (auto s = std::get_if<std::string>(&v)) {
        return *s;
    }
    if (auto i = std::get_if<int64_t>(&v)) {
        return std::to_string(*i);
    }
    if (auto d = std::get_if<double>(&v)) {
#if defined(__cplusplus) && __cplusplus > 202002L
        return return std::format("{:.9g}", *d);
#else
        std::ostringstream oss;
        oss << std::setprecision(9) << std::noshowpoint << *d;
        return oss.str();
#endif
        return std::to_string(*d);
    }
    throw Error(ErrorId::TYPE_MISMATCH);
}

double ValueToDouble(const Value& v) {
    if (auto i = std::get_if<int64_t>(&v)) {
        return (double)(*i);
    }
    if (auto d = std::get_if<double>(&v)) {
        return (*d);
    }
    if (auto s = std::get_if<std::string>(&v)) {
        const char* str = s->c_str();
        double d        = 0;
        if (parseDouble(str, &d) && *str == '\0') {
            return d;
        }
    }
    throw Error(ErrorId::TYPE_MISMATCH);
}
double ValueToDoubleOrZero(const Value& v) {
    if (auto i = std::get_if<int64_t>(&v)) {
        return (double)(*i);
    }
    if (auto d = std::get_if<double>(&v)) {
        return (*d);
    }
    if (auto s = std::get_if<std::string>(&v)) {
        const char* str = s->c_str();
        double d        = 0;
        if (parseDouble(str, &d) && *str == '\0') {
            return d;
        }
    }
    return 0.0; // VAL("X") instead of throw Error(ErrorId::TYPE_MISMATCH);
}

 int64_t ValueToInt(const Value& v) {
    if (auto* i = std::get_if<int64_t>(&v)) {
        return (*i);
    }
    if (auto* d = std::get_if<double>(&v)) {
        return (int)(*d);
    }
    if (auto* s = std::get_if<std::string>(&v)) {
        const char* str = s->c_str();
        int64_t i       = 0;
        if (parseInt(str, &i) && *str == '\0') {
            return i;
        }
    }
    throw Error(ErrorId::TYPE_MISMATCH);
}

bool ValueIsOperator(const Value& v) {
    if (auto i = std::get_if<BA67::Operator>(&v)) {
        return true;
    }
    return false;
}

bool ValueIsString(const Value& v) {
    if (auto* s = std::get_if<std::string>(&v)) {
        return true;
    }
    return false;
}
bool ValueIsInt(const Value& v) {
    if (auto* s = std::get_if<int64_t>(&v)) {
        return true;
    }
    return false;
}
bool ValueIsDouble(const Value& v) {
    if (auto* s = std::get_if<double>(&v)) {
        return true;
    }
    return false;
}

}; // namespace