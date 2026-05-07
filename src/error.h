#pragma once
#include <exception>
#include <unordered_map>
#include <string>

namespace BA67 {
    
    // https://sta.c64.org/cbm64baserr.html
enum ErrorId {
    INTERNAL         = 1,
    SYNTAX           = 11,
    FILE_OPEN        = 2,
    FILE_NOT_OPEN    = 3,
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
        static std::unordered_map<ErrorId, std::string> errorMessages = {
            {              ErrorId::INTERNAL,              "INTERNAL ERROR" },
            {                ErrorId::SYNTAX,                "SYNTAX ERROR" },
            {             ErrorId::FILE_OPEN,             "FILE OPEN ERROR" },
            {         ErrorId::FILE_NOT_OPEN,         "FILE NOT OPEN ERROR" },
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

};