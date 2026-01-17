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
#include <list>
#include "cpu-6502.h"

#include "os.h"

class SoundSystem;

class Basic {
public:
    static const std::string version() { return "7.02"; }
    static const std::string buildVersion();

    Os* os;
    Basic(Os* os, SoundSystem* ss = nullptr);

    struct Options {
        bool spacingRequired  = true; // 'false' allows FORI=1TO10 without spaces
        bool dotAsZero        = true; // 'true' allows a=. instead of a=0
        bool uppercaseInput   = false; // 'true' returns only upper-case characters in GET/INPUT
        bool colorzizeListing = true;
    };
    static Options options;

    static double constexpr eps = 2.32830644e-10; // found by 10 E=1 20 IF1+E=1 THEN END 30 E=E/2: GOTO 20


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
    using Value = std::variant<int64_t, double, std::string, Operator>;
#endif

    // https://sta.c64.org/cbm64baserr.html
    enum ErrorId {
        INTERNAL         = 1,
        SYNTAX           = 11,
        FILE_NOT_FOUND   = 4,
        ILLEGAL_DEVICE   = 9, // illegal device NUMBER
        UNDEFD_STATEMENT = 17,
        TYPE_MISMATCH    = 22,
        ILLEGAL_QUANTITY = 14,
        BAD_SUBSCRIPT    = 18 // out of dim bounds
        ,
        OUT_OF_DATA          = 13,
        OUT_OF_MEMORY        = 16,
        NEXT_WITHOUT_FOR     = 10,
        RETURN_WITHOUT_GOSUB = 12,
        FORMULA_TOO_COMPLEX  = 25,

        // these are not from BASIC V7

        BREAK                 = 30,
        UNDEFD_MODULE         = 254,
        ARGUMENT_COUNT        = 101,
        VARIABLE_UNDEFINED    = 201,
        DEF_WITHOUT_FNEND     = 202,
        READY_COMMAND         = 203, // easter egg: press enter on "READY."
        UNIMPLEMENTED_COMMAND = 255
    };


    class Error : std::exception {
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

        const char* what() const noexcept override {
            static std::map<ErrorId, std::string> errorMessages = {
                {              ErrorId::INTERNAL,              "INTERNAL ERROR" },
                {                ErrorId::SYNTAX,                "SYNTAX ERROR" },
                {        ErrorId::FILE_NOT_FOUND,        "FILE NOT FOUND ERROR" },
                {        ErrorId::ILLEGAL_DEVICE,        "ILLEGAL DEVICE ERROR" },
                {      ErrorId::UNDEFD_STATEMENT,     "UNDEF'D STATEMENT ERROR" },
                {         ErrorId::TYPE_MISMATCH,        "TYPE MISTMATCH ERROR" },
                {      ErrorId::ILLEGAL_QUANTITY,      "ILLEGAL QUANTITY ERROR" },
                {         ErrorId::BAD_SUBSCRIPT,         "BAD SUBSCRIPT ERROR" },
                {           ErrorId::OUT_OF_DATA,           "OUT OF DATA ERROR" },
                {         ErrorId::OUT_OF_MEMORY,         "OUT OF MEMORY ERROR" },
                {  ErrorId::RETURN_WITHOUT_GOSUB,        "RETURN WITHOUT GOSUB" },
                {      ErrorId::NEXT_WITHOUT_FOR,            "NEXT WITHOUT FOR" },
                {   ErrorId::FORMULA_TOO_COMPLEX,         "FORMULA TOO COMPLEX" },
                {                 ErrorId::BREAK,                       "BREAK" },
                {         ErrorId::UNDEFD_MODULE,         "UNDEFD MODULE ERROR" },
                {        ErrorId::ARGUMENT_COUNT,        "ARGUMENT COUNT ERROR" },
                {    ErrorId::VARIABLE_UNDEFINED,    "VARIABLE UNDEFINED ERROR" },
                {     ErrorId::DEF_WITHOUT_FNEND,           "DEF WITHOUT FNEND" },
                {         ErrorId::READY_COMMAND,                "YES, I AM..." },
                { ErrorId::UNIMPLEMENTED_COMMAND, "UNIMPLEMENTED COMMAND ERROR" }
            };

            return errorMessages[ID].c_str();
        }
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
        std::string_view value;

        // returns '#', '%', '$'
        char valuePostfix() const {
            if (value.ends_with('$')) {
                return '$';
            }
            if (value.ends_with('%')) {
                return '%';
            }
            if (type == TokenType::INTEGER) {
                return '%';
            }
            if (type == TokenType::NUMBER) {
                return '#';
            }
            if (type == TokenType::STRING) {
                return '$';
            }
            return '#';
        }
    };


    std::set<std::string> keywords;

public:
    using cmdpointer = std::function<void(Basic*, const std::vector<Value>&)>; // PRINT
    using fktpointer = std::function<Value(Basic*, const std::vector<Value>&)>; // MID$()


    // we need this hash for unordered containers, so we can quickly find(const char*)
    struct string_hash {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(const char* txt) const {
            return std::hash<std::string_view> {}(txt);
        }
        [[nodiscard]] size_t operator()(const std::string_view& txt) const {
            return std::hash<std::string_view> {}(txt);
        }
        [[nodiscard]] size_t operator()(const std::string& txt) const {
            return std::hash<std::string> {}(txt);
        }
    };


    std::unordered_map<std::string, cmdpointer, string_hash, std::equal_to<>> commands;
    std::unordered_map<std::string, fktpointer, string_hash, std::equal_to<>> functions;
    std::vector<MEMCELL> memory; // for PEEK&POKE. charram, colram, lineLinkTable,...

    CPU6502 cpu;

    Value TIvariable;
    Value TI$variable;
    Value STvariable; // set with memory[krnl.STATUS] = Basic::FS_DEVICE_ERROR;

    uint64_t time0; // time to subtract from tick() to get TI.

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
        std::list<std::pair<Value, Value>> dict;
        bool isDictionary = false;
        ArrayIndex bounds = {}; // 5 = [0..4]

        // dim a(4) = (0..4)
        void dim(Value init, size_t i0, size_t i1 = 0, size_t i2 = 0, size_t i3 = 0);
        void dim(Value init, const ArrayIndex& ai);
        void setIsDictionary(bool isDict) {
            isDictionary = isDict;
            if (isDict) {
                data.clear();
            } else {
                dict.clear();
            }
        }
        Value& at(const ArrayIndex& ix);
        Value& atKey(const Value& key) {
            if (!isDictionary) {
                throw Basic::Error(Basic::ErrorId::TYPE_MISMATCH);
            }
            for (auto& p : dict) {
                if (p.first == key) {
                    return p.second;
                }
            }
            dict.emplace_back(key, Value {});
            return dict.back().second;
        }
    };

    struct ProgramLine {
        ProgramLine() = default;
        ProgramLine(const ProgramLine& l)
            : ProgramLine() { *this = l; }
        ProgramLine& operator=(const ProgramLine& l) {
            code = l.code;
            return *this;
        }
        std::string code;
        std::vector<std::vector<Token>> tokens; // tokens for each command. Has string_views to code
        operator std::string&() { return code; }
    };


    struct ProgramCounter {
        std::map<int, ProgramLine>::iterator line;
        size_t cmdpos; // current command is at nth vector in ProgramLine.tokens[]
        // size_t position; // character index in program code
    };

    // Loop stack for nested FOR loops and GOSUB stack
    struct LoopItem {
        enum LoopType {
            GOSUB   = 1,
            FORNEXT = 2
        } type
            = GOSUB;

        std::string varName; // FOR variable name or "GOSUB"
        int64_t start = 0;
        int64_t end   = 0;
        int64_t step  = 1;

        // int line_number;
        // size_t char_position;

        ProgramCounter jump; // where to jump on RETURN/NEXT
    };


    // Def Fn
    struct FunctionDefinition {
        std::string fnName; // FNA e.g.
        std::string lineCopy; // because Token has string_views
        std::vector<Token> parameters;

        std::vector<Token> body; // if this has values, the function was defined with =
        int32_t gotoLine = 0; // if body is empty, goto the end of this line, return when hitting FNEND
        void clear() {
            fnName.clear();
            lineCopy.clear();
            parameters.clear();
            body.clear();
            gotoLine = 0;
        }
    };

    // Files
    std::vector<FilePtr> fileHandles;
    size_t currentFileNo = 0;
    // memory[krnl.STATUS]
    enum FileStatus {
        FS_OK           = 0,
        FS_ERROR_WRITE  = 1, // TODO OK?
        FS_ERROR_READ   = 2,
        FS_EOF          = 64,
        FS_DEVICE_ERROR = 128,
    };


    // Modules
    class Module {
    public:
        // listing[-2] = immediate mode argument
        // listing[-1] = "END"
        std::string filenameQSAVE;
        std::map<int32_t, ProgramLine> listing; // [basic number] = line

        using VariableMap      = std::unordered_map<std::string, Value, string_hash, std::equal_to<>>;
        using ArrayVariableMap = std::unordered_map<std::string, Array, string_hash, std::equal_to<>>;

        VariableMap variables;
        ArrayVariableMap arrays;

        VariableMap::iterator findOrCreateVariable(const std::string_view& variableName);

        std::vector<LoopItem> loopStack;
        // std::vector<ProgramCounter> gosubStack;

        std::unordered_map<std::string, FunctionDefinition, string_hash, std::equal_to<>> functionTable;
        int32_t autoNumbering         = 0; // set this value with AUTO
        int32_t lastEnteredLineNumber = 0;

        ProgramCounter programCounter = { listing.end(), 0 };
        int32_t lineNumberForCONT     = 0; // where to continue with CONT command
        size_t tokenIndexForCONT      = 0; // where to continue with CONT

        ProgramCounter readDataPosition = { listing.begin(), 0 };
        int readDataIndex               = 0; // from the data at readDataPosition, read the readDataIndex's element next

        bool fastMode = true;
        bool traceOn  = false;

        void restoreDataPosition() {
            readDataPosition.line   = listing.begin();
            readDataPosition.cmdpos = 0;
            readDataIndex           = 0;
        }

        void setProgramCounterToEnd() {
            programCounter.line   = listing.end();
            programCounter.cmdpos = 0;
        }

        bool isInDirectMode() const {
            return (programCounter.line == listing.end() || programCounter.line->first <= 0);
        }

        void forceTokenizing() {
            for (auto& ln : listing) {
                ln.second.tokens.clear();
            }
        }
    };

    using moduleT = std::unordered_map<std::string, Module, string_hash, std::equal_to<>>;
    moduleT modules; // modules currently in memory
    std::vector<moduleT::iterator> moduleVariableStack; // entered modules - this is for the variable space
    std::vector<moduleT::iterator> moduleListingStack; // entered modules - this is for the listing and program counter

    uint8_t colorForModule(const std::string& str) const;

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
    std::map<int, ProgramLine>& currentListing() { return moduleListingStack.back()->second.listing; }
    ProgramCounter& programCounter() { return moduleListingStack.back()->second.programCounter; }
    void storeProgramCounterForCont();

public:
    // Represent value as string
    static std::string valueToString(const Value& v);
    static double valueToDouble(const Value& v);
    static double valueToDoubleOrZero(const Value& v);
    static int64_t valueToInt(const Value& v);
    static bool valueIsOperator(const Value& v);
    static bool valueIsString(const Value& v);
    static bool valueIsInt(const Value& v);
    static bool valueIsDouble(const Value& v);

    static bool isEndOfWord(char c);
    inline static bool isWhiteSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
    static const char* skipWhite(const char*& str);
    static bool parseDouble(const char*& str, double* number = nullptr);
    static bool parseInt(const char*& str, int64_t* number = nullptr); // int - not a double! "1.23" returns false
    static bool parseFileHandle(const char*& str, std::string_view* number = nullptr);
    static int64_t strToInt(const std::string& str); // parses "255" and "$ff"
    static int64_t strToInt(const std::string_view& str); // parses "255" and "$ff"
protected:
    bool parseKeyword(const char*& str, std::string_view* keyword = nullptr);
    bool parseCommand(const char*& str, std::string_view* command = nullptr);
    bool parseFunction(const char*& str, std::string_view* command = nullptr);
    bool parseString(const char*& str, std::string_view* stringUnquoted);
    bool parseOperator(const char*& str, std::string_view* op);
    bool parseIdentifier(const char*& str, std::string_view* identifier);

    // void tokenizeNextCommand(ProgramCounter* pProgramCounter);
    const char* tokenizeNextCommand(const char* pc, std::vector<Token>& tokens);
    void tokenizeLine(ProgramLine& line);
    // std::vector<std::vector<Token>> tokenizeNextCommand(const std::string& code);

    // Operator precedence
    int precedence(const std::string_view& op);

    Basic::Value evaluateDefFnCall(Basic::FunctionDefinition& fn, const std::vector<Basic::Value>& arguments);

    // Evaluate expression using Shunting-Yard algorithm
    std::vector<Value> evaluateExpression(const std::vector<Token>& tokens, size_t start, size_t* ptrEnd = nullptr, bool breakEarly = false);

    static ArrayIndex indexFromValues(const std::vector<Value>& vals);

    void EndAndPopModule();

    void doNEW();
    void doEND();
    void handleCLR();
    void handleLET(const std::vector<Token>& tokens);
    void handleRUN(const std::vector<Token>& tokens);
    void handleMODULE(const std::vector<Token>& tokens);
    void doPrintValue(Value& v);
    void handlePRINT(const std::vector<Token>& tokens);
    void handlePRINT_USING(const std::vector<Token>& tokens);
    void handleGET(const std::vector<Token>& tokens, bool waitForKeypress);

private:
    void handleINPUTFile(const std::vector<Token>& tokens);

protected:
    void handleINPUT(const std::vector<Token>& tokens);
    void handleNETGET(const std::vector<Token>& tokens);
    void handleDIM(const std::vector<Token>& tokens);
    void handleLIST(const std::vector<Token>& tokens);
    void handleDELETE(const std::vector<Token>& tokens);
    void handleDUMP(const std::vector<Token>& tokens);
    void handleKEY(const std::vector<Token>& tokens);
    void handleRCHARDEF(const std::vector<Token>& tokens);

    // find assignable value from 'a' or arr(1+3). returns nullptr on error
    Value* findLeftValue(Module& module, const std::vector<Token>& tokens, size_t start, size_t* endPtr, bool allowDimArray = false);

    void doGOTO(int line, bool isGoSub);
    void handleGOTO(const std::vector<Token>& tokens);
    void handleGOSUB(const std::vector<Token>& tokens);
    void handleONGOTO(const std::vector<Token>& tokens);
    void handleHELP(const std::vector<Token>& tokens);
    void handleDEFFN(const std::vector<Token>& tokens);
    void handleFOR(const std::vector<Token>& tokens);
    void handleNEXT(const std::vector<Token>& tokens);

    void handleIFTHEN(const std::vector<Token>& tokens);

    void readNextData(Value* pval, char valuePostfix);
    void handleREAD(const std::vector<Token>& tokens);
    void handleRESTORE(const std::vector<Token>& tokens);

    void ensureTokenized(Module& module, ProgramCounter& pc);
    void executeParsedTokens(const std::vector<Token>& tokens);
    // void execute(ProgramLine& line, bool mustTokenize = true);

    std::array<std::string, 12> keyShortcuts; // F1..F12 key shortcuts. Set with KEY command

public:
    // bool isCursorActive = true; // screen.cursor.active
    bool insertMode = false; // insert/overwrite Alt+INS

    void uppercaseProgram(std::string& line);

    // print to current device
    void printUtf8String(const char* utf8, const char* pend = nullptr, bool applyCtrlCodes = false, bool ctrlInQuotes = false);
    inline void printUtf8String(const std::string& utf8, bool applyCtrlCodes = false, bool ctrlInQuotes = false) {
        printUtf8String(utf8.c_str(), utf8.c_str() + utf8.length(), applyCtrlCodes, ctrlInQuotes);
    }
    void printUtf8String(std::string_view utf8, bool applyCtrlCodes = false, bool ctrlInQuotes = false) {
        printUtf8String(utf8.data(), utf8.data() + utf8.length(), applyCtrlCodes, ctrlInQuotes);
    }
    // inline void printUtf8String(const char8_t* utf8, bool applyCtrlCodes = false) { printUtf8String((const char*)utf8, applyCtrlCodes); }

    enum class ParseStatus {
        PS_ERROR = 0,
        PS_EXECUTED,
        PS_PROGRAMMED,
        PS_IDLE // just pressed enter
    };
    void restoreColorsAndCursor(bool resetFont);
    ParseStatus parseInput(const char* pline); // program or run BASIC code - will reset program counter
    void executeCommands(const char* pline); // will not reset program counter

    std::string inputLine(bool allowVertical = true);

    void handleEscapeKey(bool allowPauseWithShift = false);
    void runInterpreter();
    void runToEnd();

    void waitForKeypress();

    bool loadProgram(const char* filenameUtf8);
    bool loadProgram(std::string& inOutFilenameUtf8);
    bool saveProgram(std::string filenameUtf8);
    bool fileExists(const std::string& filenameUtf8, bool allowWildCard);

    bool saveState(std::string& filenameUtf8);
    bool loadState(std::string& filenameUtf8);

    bool AreYouSureQuestion();
    void monitor();
};
