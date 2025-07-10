#pragma once
#include <string>
#include <cstdint>
#include <vector>
class PrgTool {
private:
    static uint16_t getword(const uint8_t*& p);
    static const char* gettoken(const uint8_t*& prg);

public:
    static std::string PRGtoBASIC(const uint8_t* prgBytes);
    static std::vector<uint8_t> BASICtoPRG(const char* basicUtf8);
};