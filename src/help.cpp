#include "help.h"

#include <map>
std::string Help::getUsage(const std::string& command) {
    static std::map<std::string, std::string> allHelp = {
#include "help.inc"
    };

    if (allHelp.find(command) == allHelp.end()) {
        return "NO HELP AVAILABLE";
    }
    return "*** " + command + " ***\n" + allHelp[command];
}
