#pragma once

#include "debug.h"
#include "operator.h"

#include <variant>
#include <string>
#include <cstddef>
#include <vector>
#include <string_view>

using Token_Data = std::variant<
    std::monostate,  // e.g. ;, {}, ()
    long long,       // e.g. 42, 0xFF, 1000
    double,          // e.g. 3.14, 0.0001, 2.0
    Operator_ID,     // e.g. +, *, ::, >>=
    std::string_view // e.g. myVar, "Hello", return
>;

enum class Token_ID {
    // --- Literals (Data) ---
    INTEGER_LITERAL,       // e.g., '5'
    FLOAT_LITERAL,         // e.g., '5.32'
    STRING_LITERAL,        // e.g., "hello world"
    CHAR_LITERAL,          // e.g., 'a', '/n'

    // --- Language Basics ---
    KEYWORD,               // e.g., 'return', 'if', 'else'
    OPERATOR,              // e.g., '+', 
    IDENTIFIER,            // e.g., 'myVariable', 'main',

    // --- Grouping & Scoping ---
    OPEN_PAREN,            // '(' - Starts function arguments or math precedence
    CLOSE_PAREN,           // ')' - Ends function arguments or math precedence
    OPEN_BRACE,            // '{' - Starts a new code block or scope
    CLOSE_BRACE,           // '}' - Ends a code block or scope
    OPEN_SUBSCRIPT,        // '[' - Starts array indexing or pointer offset
    CLOSE_SUBSCRIPT,       // ']' - Ends array indexing or pointer offset
    SEPERATOR,             // ':' - Seperates cases

    // --- Special Delimiters ---
    END_ARG,               // ',' - Separates arguments in a function call or list
    VA_ARG,                // '...' - Represents "Variadic Arguments" (variable number of params)
    ENDOFCOMMAND,          // ';' - Terminates a statement (The "period" of a code sentence)

    // --- Advanced Syntax ---
    TYPE_CAST,             // e.g., '(int)', 'static_cast<float>' (Forcing a type change)
    SCOPE_RESOLUTION,      // '::' - Accessing members of a namespace or class (e.g., std::cout)

    EOS,
    UNKNOWN
};

struct Token {
    Token_ID tkID;
    Token_Data data;

    Token(Token_ID _id) : tkID(_id), data(std::monostate()) {}

    Token(Token_ID _id, long long num) : tkID(_id), data(num) {}

    Token(Token_ID _id, double flt) : tkID(_id), data(flt) {}

    Token(Token_ID _id, Operator_ID _opID) : tkID(_id), data(_opID) {}

    Token(Token_ID _id, std::string_view str) : tkID(_id), data(str) {}

};

class Lexer {
private:
    const std::string_view src;
    size_t index;
    char currentChar;

    bool isAtEscapeNullChar() const;
    bool isAtEscapeNewlineChar() const;
    void consumeEscapeChar();
    void advance();
    char peek(size_t len) const;

    void skipWhitespace();
    void skipComments();
    Token lexCharLiterals();
    Token lexNumberLiterals();
    Token lexOperators();
    Token lexStringLiterals();
    Token lexKeywordsAndIdentifiers();
    Token lexSpecialCharacters();

public:
    Lexer(const std::string_view input) : src(input), index(0), currentChar(src.empty() ? '\0' : src[0]) {}
    Token getNextToken();
    std::vector<Token> tokenize();
};

#if DEBUG_MODE
namespace Debug::Lexer {
    const int padding = 16; // Ewww hardcoded constants
    constexpr const char* tokenIDToString(Token_ID _id);
    void dumpTokenData(Token _token);
    void dumpAllTokens(std::vector<Token> _tokens);
}
#endif