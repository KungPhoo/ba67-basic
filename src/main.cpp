#include "os_fpl.h"
#include "soundsystem_soloud.h"
#ifdef _WIN32
    #include <Windows.h>
#endif

#include <vector>
#include "basic.h"
#include <iostream>
#include "unicode.h"

#if _DEBUG
    #include "markdown_parser.h"
#endif

#if 0
void cmdWHATEVER(Basic* basic, const std::vector<Basic::Value>& values) {
    // attention! values contains operators (COMMA)
    for (auto& v : values)
    {
        basic->printUtf8String(
            Basic::valueToString(v).c_str());
    }
}
Basic::Value fktFOO$(Basic* basic, const std::vector<Basic::Value>& values) {
    // attention! values contains operators (COMMA)
    return {"foo"};
}
int main(){
    ...
    // here's how to add your own commands
    basic.commands["WHATEVER"] = cmdWHATEVER;
    // and functions
    basic.functions["FOO$"] = fktFOO$;
    return basic.parseInput("WHATEVER foo$(1,2,3)");
}
#endif

void printfHelp();

// ---------------------------------------
// MAIN
// ---------------------------------------
int main(int argc, char** argv) {
#if _DEBUG
    MarkdownParser::ParseAndApplyManual();
#endif

    // ARGV to UTF-8
    std::vector<std::string> args;  // utf-8
#ifdef _WIN32
    (void)argc;
    (void)argv;
    LPWSTR* szArglist;
    int nArgs;
    szArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (NULL == szArglist) {
        return printf("CommandLineToArgvW failed\n");
    }
    for (int i = 0; i < nArgs; i++) { args.push_back(Unicode::toUtf8String(reinterpret_cast<const char16_t*>(szArglist[i]))); }
    LocalFree(szArglist);
    szArglist = nullptr;
#else
    for (int i = 0; i < argc; ++i) { args.emplace_back(argv[i]); }
#endif

#ifdef _DEBUG
    args = {
        "--video", "opengl",  // "--fullscreen"
        // "--crtemulation", "false",
        ""};
#endif

    auto& sets = Os::settings;
    args.push_back("");  // ensure [i] and [i+1]
    for (size_t i = 1; i + 1 < args.size(); ++i) {
        if (args[i] == "--help") {
            printfHelp();
        }
        if (args[i] == "--fullscreen") { sets.fullscreen = true; }
        if (args[i] == "--video") {
            if (args[i + 1] == "opengl") {
                sets.renderMode = BA68settings::OpenGL;
                printf("Render mode: OpenGL\n");
            } else {
                printf("Render mode: Software\n");
            }
        }
        if (args[i] == "--crtemulation") {
            if (args[i + 1] == "true") {
                sets.emulateCRT = true;
            } else {
                sets.emulateCRT = false;
            }
        }
    }

    // OsWindowsConsole os;
    OsFPL os;

    SoundSystemSoLoud soloud;
    Basic basic(os, &soloud);

    // load boot.bas
    try {
        if (basic.loadProgram("boot.bas")) {
            basic.parseInput("RUN");
            basic.parseInput("NEW");
        }
    } catch (...) {}

    // run bas program from command line
    for (size_t i = 1; i < args.size(); ++i) {
        if (
            (args[i].ends_with(".bas") ||
             args[i].ends_with(".ba67")) &&
            basic.loadProgram(args[i])) {
            basic.parseInput("RUN");
            break;
        }
    }

    basic.runInterpreter();
    return 0;
}

void printfHelp() {
    printf("BA68 BASIC interpreter. www.ba67.org/\n");
    printf("\n");
    printf("--help                      show this help\n");
    printf("--video        software     (default) use X11 or Windows GDI\n");
    printf("               opengl       Use OpenGL 1.1 glDrawPixels\n");
    printf("--crtemulation true         (default) enable the CRT RGB emulation\n");
    printf("               false        disable the CRT RGB emulation\n");
}