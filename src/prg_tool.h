#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include "kernal.h"
class PrgTool {
private:
    static uint16_t getword(const uint8_t*& p);
    const char* gettoken(const uint8_t*& prg);

public:
    int startAddressOfPRG = int(krnl.BASICCODE);
    bool compress         = false;
    int basicVersion      = 7; // 1,2,3,7
    std::vector<std::pair<int, std::string>> errorDetails;

    std::string PRGtoBASIC(const uint8_t* prgBytes);
    std::vector<uint8_t> BASICtoPRG(const char* basicUtf8);
};