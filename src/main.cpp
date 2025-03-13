
#include <Windows.h>

#include "os_windows_console.h"
#include "os_fpl.h"
#include "soundsystem_soloud.h"

#include <vector>
#include "basic.h"
#include <iostream>
#include "unicode.h"



void cmdWHATEVER(Basic* basic, const std::vector<Basic::Value>& values) {
    for (auto& v : values) {
        basic->printUtf8String(
            Basic::valueToString(v).c_str()
        );
    }
}
Basic::Value fktFOO(Basic* basic, const std::vector<Basic::Value>& values) {
    return {"foo"};
}


int main(int argc, char** argv) {
    std::vector<std::string> args; // utf-8
#ifdef _WIN32
    (void)argc; (void)argv;
    LPWSTR* szArglist;
    int nArgs;
    szArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (NULL == szArglist) {
        return printf("CommandLineToArgvW failed\n");
    }
    for (int i = 0; i < nArgs; i++) { args.push_back(Unicode::toUtf8String(reinterpret_cast<const char16_t*>(szArglist[i]))); }
    LocalFree(szArglist); szArglist = nullptr;
#else
    for (int i = 0; i < argc; ++i) { args.emplace_back(argv[i]); }
#endif


#if 0
    OsWindowsConsole os;
#else
    OsFPL os;
#endif
    SoundSystemSoLoud soloud;
    Basic basic(os, &soloud);

    // here's how to add your own commands
    basic.commands["WHATEVER"] = cmdWHATEVER;
    // and functions
    basic.functions["FOO"] = fktFOO;

    basic.parseInput("IF -3+4*(-2)-(-(-5))/(3+-1)<>-13.5 THEN PRINT \"ERROR: COMPLEX MATH 1\"");

    // basic.parseInput("FOR I = 1 TO 3: FOR J = 1 TO 2: PRINT I, J: NEXT: NEXT");
    // basic.parseInput("X=-3+4*(-2)"); // should be -11
    // basic.parseInput("X=(-(-5))/(3+-1)"); // should be 2.5
    // basic.parseInput("X=-3+4*(-2)-(-(-5))/(3+-1)"); // should be -13.5

    try {
        if (basic.loadProgram("boot.bas")) {
            basic.parseInput("RUN");
            basic.parseInput("NEW");
        }
    } catch (...) {}

#if 1
    if (args.size() > 1) {
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].ends_with(".bas") && basic.loadProgram(args[i])) {
                basic.parseInput("RUN");
            }
        }
    }
#endif


    basic.runInterpreter();
    return 0;
}
