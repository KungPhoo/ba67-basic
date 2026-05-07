#pragma once
#include <string>
#include <variant>


namespace BA67 {

// Variable types
class Operator {
public:
    Operator()                = default;
    Operator(const Operator&) = default;
    Operator(const std::string& s) { value = s; }
    Operator& operator=(const Operator&) = default;
    bool operator==(const Operator& o) const { return o.value == value; }
    std::string value;
};

#if 0 // _DEBUG

    class Value: public std::variant<int64_t, double, std::string, Operator> {
    public:
        Value(): std::variant<int64_t, double, std::string, Operator>(std::string()) {}
        Value(const std::string& p): std::variant<int64_t, double, std::string, Operator>(p) {}
        Value(int64_t p): std::variant<int64_t, double, std::string, Operator>(p) {}
        Value(double p): std::variant<int64_t, double, std::string, Operator>(p) {}
        Value(const Operator& p): std::variant<int64_t, double, std::string, Operator>(p) {}
        Value(const Value&) = default;

        Value& operator=(int64_t i) {
            if (i < 0) {
                int pause = 0;
            }
            std::variant<int64_t, double, std::string, Operator>::operator=(i);
            return *this;
        }
        Value& operator=(double i) {
            if (i < 0) {
                int pause = 0;
            }
            std::variant<int64_t, double, std::string, Operator>::operator=(i);
            return *this;
        }
        Value& operator=(const std::string& i) {
            std::variant<int64_t, double, std::string, Operator>::operator=(i);
            return *this;
        }
        Value& operator=(const Operator& i) {
            std::variant<int64_t, double, std::string, Operator>::operator=(i);
            return *this;
        }
        Value& operator=(const Value& i) {
            std::variant<int64_t, double, std::string, Operator>::operator=(i);
            return *this;
        }
    };

#else
// string is stored in UTF-8 encoding
using Value = std::variant<int64_t, double, std::string, Operator>;
#endif


    // Represent value as string
std::string ValueToString(const Value& v);
double      ValueToDouble(const Value& v);
double      ValueToDoubleOrZero(const Value& v);
int64_t     ValueToInt(const Value& v);
bool        ValueIsOperator(const Value& v);
bool        ValueIsString(const Value& v);
bool        ValueIsInt(const Value& v);
bool        ValueIsDouble(const Value& v);


}; // namespace