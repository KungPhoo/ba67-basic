#pragma once
#include <array>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <variant>
#include <vector>

class Os;
class SoundSystem;

class Basic {
public:
    std::string version() { return "7.01"; }

    Os* os;
    Basic(Os* os, SoundSystem* ss = nullptr);

    struct Options {
        bool noSpaceSeparator = false; // 'true' allows FORI=1TO10 without spaces
        bool dotAsZero        = false; // 'true' allows a=. instead of a=0
    };
    static Options options;



    // Variable types
    class Operator {
    public:
        Operator()                = default;
        Operator(const Operator&) = default;
        Operator(const std::string& s) { value = s; }
        Operator& operator=(const Operator&) = default;
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
    using Value = std::variant<int64_t, double, std::string, Operator>;
#endif

    enum class ErrorId {
        INTERNAL = 1,
        SYNTAX,
        FILE_NOT_FOUND,
        ILLEGAL_DEVICE,
        UNDEFD_STATEMENT,
        TYPE_MISMATCH,
        ILLEGAL_QUANTITY,
        BAD_SUBSCRIPT // out of dim bounds
        ,
        UNIMPLEMENTED_COMMAND,
        OUT_OF_DATA,
        RETURN_WITHOUT_GOSUB,
        FORMULA_TOO_COMPLEX,

        // these are not from BASIC V7

        BREAK,
        UNDEFD_MODULE,
        ARGUMENT_COUNT = 101,
        VARIABLE_UNDEFINED
    };

    std::map<ErrorId, std::string> errorMessages = {
        {                ErrorId::SYNTAX,                "SYNTAX ERROR" },
        {        ErrorId::FILE_NOT_FOUND,        "FILE NOT FOUND ERROR" },
        {        ErrorId::ILLEGAL_DEVICE,        "ILLEGAL DEVICE ERROR" },
        {      ErrorId::UNDEFD_STATEMENT,     "UNDEF'D STATEMENT ERROR" },
        {         ErrorId::TYPE_MISMATCH,        "TYPE MISTMATCH ERROR" },
        {      ErrorId::ILLEGAL_QUANTITY,      "ILLEGAL QUANTITY ERROR" },
        {         ErrorId::BAD_SUBSCRIPT,         "BAD SUBSCRIPT ERROR" },
        { ErrorId::UNIMPLEMENTED_COMMAND, "UNIMPLEMENTED COMMAND ERROR" },
        {           ErrorId::OUT_OF_DATA,           "OUT OF DATA ERROR" },
        {  ErrorId::RETURN_WITHOUT_GOSUB,        "RETURN WITHOUT GOSUB" },
        {   ErrorId::FORMULA_TOO_COMPLEX,         "FORMULA TOO COMPLEX" },
        {                 ErrorId::BREAK,                       "BREAK" },
        {         ErrorId::UNDEFD_MODULE,         "UNDEFD MODULE ERROR" },
        {        ErrorId::ARGUMENT_COUNT,        "ARGUMENT COUNT ERROR" },
        {    ErrorId::VARIABLE_UNDEFINED,    "VARIABLE UNDEFINED ERROR" }
    };

    class Error {
    public:
        Error(ErrorId id) {
            if (id != ErrorId::BREAK) {
                int stop = 1;
            }
            ID = id;
        }
        Error(const Error&)            = default;
        Error& operator=(const Error&) = default;
        ErrorId ID;
    };

protected:
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
        TokenType type;
        std::string value;
    };

    std::set<std::string> keywords;

public:
    using cmdpointer = std::function<void(Basic*, const std::vector<Value>&)>; // PRINT
    using fktpointer = std::function<Value(Basic*, const std::vector<Value>&)>; // MID$()

    std::unordered_map<std::string, cmdpointer> commands;
    std::unordered_map<std::string, fktpointer> functions;
    std::array<uint8_t, 0x20000> memory; // for PEEK&POKE - no other use

    // Arrays
    struct ArrayIndex {
        ArrayIndex()                             = default;
        ArrayIndex(const ArrayIndex&)            = default;
        ArrayIndex& operator=(const ArrayIndex&) = default;
        ArrayIndex(size_t a, size_t b = 0, size_t c = 0, size_t d = 0)
            : index { a, b, c, d } {
        }
        std::array<size_t, 4> index = {};
        int dimensions() const {
            for (size_t i = 0; i < index.size(); ++i) {
                if (index[i] == 0) {
                    return int(i);
                }
            }
            return int(index.size());
        }
    };
    struct Array {
        std::vector<Value> data;
        ArrayIndex bounds = {}; // 5 = [0..4]

        // dim a(4) = (0..4)
        void dim(size_t i0, size_t i1 = 0, size_t i2 = 0, size_t i3 = 0);
        void dim(const ArrayIndex& ai);
        Value& at(const ArrayIndex& ix);
    };

    struct ProgramCounter {
        std::map<int, std::string>::iterator line;
        size_t position;
    };

    // Loop stack for nested FOR loops
    struct ForLoop {
        std::string varName;
        int64_t start;
        int64_t end;
        int64_t step;
        int line_number;
        size_t char_position;
    };

    // Def Fn
    struct FunctionDefinition {
        std::vector<Token> parameters;
        std::vector<Token> body;
    };

    // Files
    class FileHandle {
    public:
        FileHandle()
            : pfile(nullptr) {
        }
        virtual ~FileHandle() {
            if (pfile) {
                fclose(pfile);
                pfile = nullptr;
            }
        }
        FILE* pfile = nullptr;
    };
    std::array<FileHandle, 255> openFiles;
    size_t currentFileNo = 0;

    // Modules
    class Module {
    public:
        // listing[-2] = immediate mode argument
        // listing[-1] = "END"
        std::string filenameQSAVE;
        std::map<int, std::string> listing; // [basic number] = line
        std::unordered_map<std::string, Value> variables;
        std::unordered_map<std::string, Array> arrays;
        std::vector<ForLoop> forStack;
        std::vector<ProgramCounter> gosubStack;
        std::unordered_map<std::string, FunctionDefinition> functionTable;
        size_t autoNumbering          = 0; // set this value with AUTO
        int64_t lastEnteredLineNumber = 0;

        ProgramCounter programCounter = { listing.end(), 0 };

        ProgramCounter readDataPosition = { listing.begin(), 0 };
        int readDataIndex               = 0; // from the data at readDataPosition, read the readDataIndex's element next

        bool fastMode = true;
        bool traceOn  = false;

        void restoreDataPosition() {
            readDataPosition.line     = listing.begin();
            readDataPosition.position = 0;
            readDataIndex             = 0;
        }

        void setProgramCounterToEnd() {
            programCounter.line     = listing.end();
            programCounter.position = 0;
        }
    };

    std::map<std::string, Module> modules; // modules currently in memory
    std::vector<std::map<std::string, Module>::iterator> moduleVariableStack; // entered modules - this is for the variable space
    std::vector<std::map<std::string, Module>::iterator> moduleListingStack; // entered modules - this is for the listing and program counter

    int colorForModule(const std::string& str) const;

    // pointer to the current program counter
    // this one might differ from the currentModule().programCounter!
    // Because of this syntax:

    // code in main.bas   VARS  LINES
    //                    cm()  pc        cm=currentModule(), pc=programCounter
    // a=1                MAIN  MAIN
    // MODULE MODL        MODL  MAIN
    // LOAD "mod.bas"     MODL  MAIN
    // END                MODL  MAIN
    // m.a = a            MAIN  MAIN
    // MODULE NODL        MODL  MAIN
    // RUN                MODL  MODL
    // PRINT a            MAIN  MAIN
    // ProgramCounter* programCounter; // the position in the current listing

    Module& currentModule() { return moduleVariableStack.back()->second; } // the module (variable space) to work in
    std::map<int, std::string>& currentListing() { return moduleListingStack.back()->second.listing; }
    ProgramCounter& programCounter() { return moduleListingStack.back()->second.programCounter; }

public:
    // Represent value as string
    static std::string valueToString(const Value& v);
    static double valueToDouble(const Value& v);
    static int64_t valueToInt(const Value& v);
    static bool valueIsOperator(const Value& v);
    static bool valueIsString(const Value& v);

    static bool isEndOfWord(char c);
    static const char* skipWhite(const char*& str);
    static bool parseDouble(const char*& str, double* number = nullptr);
    static bool parseInt(const char*& str, int64_t* number = nullptr); // int - not a double! "1.23" returns false
    static bool parseFileHandle(const char*& str, std::string* number = nullptr);
    static int64_t strToInt(const std::string& str); // parses "255" and "$ff"
protected:
    bool parseKeyword(const char*& str, std::string* keyword = nullptr);
    bool parseCommand(const char*& str, std::string* command = nullptr);
    bool parseString(const char*& str, std::string* stringUnquoted);
    bool parseOperator(const char*& str, std::string* op);
    bool parseIdentifier(const char*& str, std::string* identifier);

    std::vector<Token> tokenize(ProgramCounter* pProgramCounter = nullptr);
    std::vector<std::vector<Token>> tokenize(const std::string& code);

    // Operator precedence
    int precedence(const std::string& op);

    Basic::Value evaluateDefFnCall(Basic::FunctionDefinition& fn, const std::vector<Basic::Value>& arguments);

    // Evaluate expression using Shunting-Yard algorithm
    std::vector<Value> evaluateExpression(const std::vector<Token>& tokens, size_t start, size_t* ptrEnd = nullptr);

    static ArrayIndex indexFromValues(const std::vector<Value>& vals);

    void EndAndPopModule();

    void doNEW();
    void doEND();
    void handleLET(const std::vector<Token>& tokens);
    void handleRUN(const std::vector<Token>& tokens);
    void handleMODULE(const std::vector<Token>& tokens);
    void doPrintValue(Value& v);
    void handlePRINT(std::vector<Token>& tokens);
    void handlePRINT_USING(const std::vector<Token>& tokens);
    void handleGET(const std::vector<Token>& tokens, bool waitForKeypress);
    void handleINPUT(const std::vector<Token>& tokens);
    void handleDIM(const std::vector<Token>& tokens);
    void handleLIST(const std::vector<Token>& tokens);

    void handleDELETE(const std::vector<Token>& tokens);

    void handleKEY(const std::vector<Token>& tokens);

    void handleRCHARDEF(const std::vector<Token>& tokens);

    char valuePostfix(const Token& t) const; // returns '#', '%', '$'

    // find assignable value from 'a' or arr(1+3). returns nullptr on error
    Value* findLeftValue(Module& module, const std::vector<Token>& tokens, size_t start, size_t* endPtr, bool allowDimArray = false);

    void doGOTO(int line, bool isGoSub);
    void handleGOTO(std::vector<Token>& tokens);
    void handleGOSUB(std::vector<Token>& tokens);
    void handleONGOTO(std::vector<Token>& tokens);
    void handleHELP(std::vector<Token>& tokens);
    void handleDEFFN(std::vector<Token>& tokens);
    void handleFOR(const std::vector<Token>& tokens);
    void handleNEXT(const std::vector<Token>& tokens);

    void handleIFTHEN(std::vector<Token>& tokens);

    Value readNextData();
    void handleREAD(std::vector<Token>& tokens);
    void handleRESTORE(std::vector<Token>& tokens);

    void updateConstantVariables();
    void executeTokens(std::vector<Token>& tokens);

    std::array<std::string, 12> keyShortcuts; // F1..F12 key shortcuts. Set with KEY command

public:
    bool isCursorActive = true;
    bool insertMode     = false; // insert/overwrite Alt+INS

    void uppercaseProgram(std::string& line);

    void printUtf8String(const char* utf8, bool applyCtrlCodes = false);
    inline void printUtf8String(const std::string& utf8, bool applyCtrlCodes = false) { printUtf8String(utf8.c_str(), applyCtrlCodes); }
    // inline void printUtf8String(const char8_t* utf8, bool applyCtrlCodes = false) { printUtf8String((const char*)utf8, applyCtrlCodes); }

    // List all variables and their values
    void dumpVariables();

    enum class ParseStatus {
        PS_ERROR = 0,
        PS_EXECUTED,
        PS_PROGRAMMED,
        PS_IDLE // just pressed enter
    };
    void restoreColorsAndCursor(bool resetFont);
    ParseStatus parseInput(const char* pline);

    std::string inputLine(bool allowVertical = true);

    void handleEscapeKey(bool allowPauseWithShift = false);
    void runInterpreter();

    void waitForKeypress();

    std::string findFirstFileNameWildcard(std::string filenameUtf8, bool isDirectory = false);
    FILE* fopenUtf8(const std::string& filenameUtf8, const char* mode);
    bool loadProgram(const char* filenameUtf8);
    bool loadProgram(std::string& inOutFilenameUtf8);
    bool saveProgram(std::string filenameUtf8);
    bool fileExists(const std::string& filenameUtf8, bool allowWildCard);
};
