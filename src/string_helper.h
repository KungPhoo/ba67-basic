#include <string>
#include <vector>

class StringHelper {
public:
    static std::vector<std::string> split(const std::string& in, const std::string& delim = " \t\r\n");

};