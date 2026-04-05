#include "lexer.h"

#include <iostream>
#include <vector>
#include <string>

/*
In release, any functions wrapped in DEBUG are ignored at runtime

NOTES:
- In future a filesorter will add \n characters, if you do not add \n after a comment while testing
  the lexer will think everything is a comment and ignore it.
- Any unicode will not break the lexer, however it cannot be correctly represented with the current
  console output system. A U+XXXX codepoint format is stored internally in the token, but display
  depends on terminal UTF-8 support. A proper error reporting system should handle this later.
- Raw string literals (R"(...)") treat escape sequences as two characters, matching file reader behaviour.

TODO:
- Lexer.cpp/Operator.cpp => Look at optimising string usage, look into iomanip lib
- Error reporting system with line/column tracking
- Unicode identifier display in error messages (U+XXXX format already stored in tokens)
*/

int main() {
    const std::string input = R"(

    // Comment at start
    int main() {
        int a = 10;
        float b = .5;
        float c = 5.;
        float d = 1.2f;
        double e = 10.0L;

        int hex1 = 0xFF;
        int hex2 = 0x;
        int hex3 = 0xDEADBEEF;
        int hexOverflow = 0xFFFFFFFFFFFFFFFFFFFF;

        char c1 = 'a';
        char c2 = '\n';
        char c3 = '\x41';
        char c4 = '\x';
        char c5 = '';

        string s1 = "hello world";
        string s2 = "escaped \" quote";
        string s3 = "unterminated string

        int identifier_test_123 = 5;
        int _underscoreStart = 6;

        a++;
        b--;
        c += 10;
        d <<= 2;
        e >>= 3;
        f == g;
        h != i;
        j && k;
        l || m;
        n -> o;
        p :: q;
        r...;

        // UTF-8
        int π = 3;
        int 漢字 = 10;

        // Weird spacing
        int     spaced     =      5;

        // End of file identifier test
        lastIdentifier

    }
    )";

	Lexer lexer(input);
	std::vector<Token> tkns = lexer.tokenize();
	DEBUG(Debug::Lexer::dumpAllTokens(tkns));
}