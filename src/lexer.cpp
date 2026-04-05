#include "debug.h"
#include "error.h"
#include "operator.h"
#include "lexer.h"

#include <string>
#include <cstddef>
#include <cctype>
#include <vector>
#include <string_view>
#include <variant>
#include <unordered_map>

static const std::unordered_map<std::string_view, Token_ID> keywordMap = {
	// --- Primitive Data Types ---
	{"void",      Token_ID::KEYWORD},
	{"bool",      Token_ID::KEYWORD},
	{"char",      Token_ID::KEYWORD},
	{"int",       Token_ID::KEYWORD},
	{"float",     Token_ID::KEYWORD},
	{"double",    Token_ID::KEYWORD},
	{"long",      Token_ID::KEYWORD},
	{"signed",    Token_ID::KEYWORD},
	{"unsigned",  Token_ID::KEYWORD},

	// --- Control Flow ---
	{"if",        Token_ID::KEYWORD},
	{"else",      Token_ID::KEYWORD},
	{"switch",    Token_ID::KEYWORD},
	{"case",      Token_ID::KEYWORD},
	{"default",   Token_ID::KEYWORD},
	{"do",        Token_ID::KEYWORD},
	{"while",     Token_ID::KEYWORD},
	{"for",       Token_ID::KEYWORD},
	{"break",     Token_ID::KEYWORD},
	{"continue",  Token_ID::KEYWORD},
	{"return",    Token_ID::KEYWORD},

	// --- Storage & Scope Modifiers ---
	{"const",     Token_ID::KEYWORD},
	{"static",    Token_ID::KEYWORD},
	{"extern",    Token_ID::KEYWORD},
	{"typedef",   Token_ID::KEYWORD},
	{"using",     Token_ID::KEYWORD},

	// --- User-Defined Types ---
	{"struct",    Token_ID::KEYWORD},
	{"class",     Token_ID::KEYWORD},
	{"enum",      Token_ID::KEYWORD},

	// --- Literals & Utilities ---
	{"true",      Token_ID::KEYWORD},
	{"false",     Token_ID::KEYWORD},
	{"nullptr",   Token_ID::KEYWORD},
	{"sizeof",    Token_ID::KEYWORD}
};

// Raw strings dont count escape characters as one byte but c++ does so we check both

bool Lexer::isAtEscapeNullChar() const {
	return (currentChar == '\0') ? true : (currentChar == '\\' && peek(1) == '0');
}

bool Lexer::isAtEscapeNewlineChar() const {
	return (currentChar == '\n') ? true : (currentChar == '\\' && peek(1) == 'n');
}

void Lexer::consumeEscapeChar() {
	if (currentChar == '\\') {
		// Counted as two chars
		advance();
		// Safety so no null terminator is accidently consumed.
		if (currentChar != '\0') advance();
	}
	else {
		// Counted as one
		advance();
	}
}

void Lexer::advance() {
	if (index < src.length()) {
		index++;
		// If we just hit the end, currentChar becomes 0
		currentChar = (index < src.length()) ? src[index] : '\0';
	}
	else {
		currentChar = '\0';
	}
}

char Lexer::peek(size_t len) const {
	if (index >= src.length()) return '\0';
	const size_t i = index + len;
	return (i < src.length()) ? src[i] : '\0';
}

void Lexer::skipWhitespace() {
	while (currentChar == ' ' || currentChar == '\n' || currentChar == '\t')
		advance();
}

void Lexer::skipComments() {
	while (currentChar != '\0' && currentChar != '\n') {
		advance();
	}
	if (currentChar == '\n') advance(); // Consume the newline
}

Token Lexer::lexCharLiterals() {
	const size_t start = Lexer::index;
	advance(); // Consume opening '

	if (currentChar == '\0') return Token(Token_ID::UNKNOWN);

	if (currentChar == '\\') {
		advance(); // Consume '\'

		if (currentChar == '\0') return Token(Token_ID::UNKNOWN); // Past end of src

		if (currentChar == 'x' || currentChar == 'X') {
			advance();
			int nd = 0;
			while (isxdigit(static_cast<unsigned char>(currentChar))) {
				advance();
				nd++;
			}
			if (nd == 0) {
				while (currentChar != '\'' && currentChar != '\n' && currentChar != '\0')
					advance();
				if (currentChar == '\'') advance();
				return Token(Token_ID::UNKNOWN);
			}
		}
		else {
			if (currentChar != '\0') advance(); // Consume the escape character, but guard it
		}
	}
	else if (currentChar != '\'') {
		advance();
	}

	if (currentChar == '\'') {
		advance();
		if (index - start == 2) return Token(Token_ID::UNKNOWN);
		return Token(Token_ID::CHAR_LITERAL, src.substr(start, index - start));
	}

	while (currentChar != '\'' && currentChar != '\n' && currentChar != '\0')
		advance();

	if (currentChar == '\'') advance();
	return Token(Token_ID::UNKNOWN);
}

Token Lexer::lexNumberLiterals() {
	size_t start = Lexer::index;
	bool isFloat = false;

	// Hexadecimal logic
	if (currentChar == '0') {
		char next = peek(1);
		if (next == 'x' || next == 'X') {
			advance(); // Consume '0'
			advance(); // Consume 'x'

			int nd = 0;
			while (isxdigit(static_cast<unsigned char>(currentChar))) {
				advance();
				nd++;
			}

			if (nd == 0) {
				// '0x' with no valid digits following — skip any junk and return UNKNOWN
				while (currentChar != ';' && currentChar != '\n' && currentChar != '\0')
					advance();
				return Token(Token_ID::UNKNOWN);
			}

			std::string_view hexStr = Lexer::src.substr(start, Lexer::index - start);
			try {
				return Token(Token_ID::INTEGER_LITERAL, std::stoll((std::string)hexStr, nullptr, 16));
			}
			catch (...) {
				// Value too large for stoll (e.g. 0xDEADBEEFDEADBEEFFF)
				return Token(Token_ID::UNKNOWN);
			}
		}
	}

	if (currentChar == '.') {
		isFloat = true;
		advance();
		while (isdigit((unsigned char)currentChar)) advance();
	}
	else {
		while (isdigit((unsigned char)currentChar)) advance();

		if (currentChar == '.') {
			isFloat = true;
			advance();
			while (isdigit((unsigned char)currentChar)) advance();
		}
	}

	// Consume any data type suffixes
	while (currentChar == 'f' || currentChar == 'F' ||
		currentChar == 'l' || currentChar == 'L' ||
		currentChar == 'u' || currentChar == 'U')
		advance();

	return Token((isFloat ? Token_ID::FLOAT_LITERAL : Token_ID::INTEGER_LITERAL), Lexer::src.substr(start, Lexer::index - start));
}

Token Lexer::lexOperators() {
	const auto& ops = getOpList();
	std::string_view remaining = src.substr(index);

	for (const auto& op : ops) {
		if (remaining.starts_with(op.symbol)) {
			size_t len = std::strlen(op.symbol);

			for (size_t i = 0; i < len; ++i) advance();

			// Intercept ambiguous cases
			switch (op.opID) {
				case Operator_ID::opFunctionalCall:  return Token(Token_ID::OPEN_PAREN, "(");
				case Operator_ID::opSubscript:       return Token(Token_ID::OPEN_SUBSCRIPT, "[");
				case Operator_ID::opScopeResolution: return Token(Token_ID::SCOPE_RESOLUTION, "::");
				case Operator_ID::opVaArg:			 return Token(Token_ID::VA_ARG, "...");
				default:							 return Token(Token_ID::OPERATOR, op.opID);
			}
		}
	}

	// Token_ID::UNKNOWN means not an operator here, not invalid token
	return Token(Token_ID::UNKNOWN);
}

Token Lexer::lexStringLiterals() {
	size_t start = Lexer::index;
	advance(); // Consume opening "

	while (currentChar != '"' && currentChar != '\n' && currentChar != '\0') {
		if (currentChar == '\\') {
			advance(); // Skip '\'
			if (currentChar != '\0') advance(); // Skip the escaped char
			continue;
		}
		advance();
	}

	if (currentChar != '"')
		return Token(Token_ID::UNKNOWN); // Unterminated or hit newline/null

	advance(); // Consume closing "
	return Token(Token_ID::STRING_LITERAL, Lexer::src.substr(start, Lexer::index - start));
}

Token Lexer::lexKeywordsAndIdentifiers() {
	std::string_view remaining = src.substr(Lexer::index);
	int len = 0;

	while (len < (int)remaining.size() && (isalnum((unsigned char)remaining[len]) || remaining[len] == '_'))
		len++;

	std::string_view value = src.substr(Lexer::index, len);

	auto it = keywordMap.find(value);
	Token_ID id = (it != keywordMap.end()) ? it->second : Token_ID::IDENTIFIER;

	for (int i = 0; i < len; ++i) advance();

	return Token(id, value);
}

Token Lexer::lexSpecialCharacters() {
    switch (currentChar) {
        case ')': advance(); return Token(Token_ID::CLOSE_PAREN,     ")");
        case ']': advance(); return Token(Token_ID::CLOSE_SUBSCRIPT, "]");
        case '{': advance(); return Token(Token_ID::OPEN_BRACE,      "{");
        case '}': advance(); return Token(Token_ID::CLOSE_BRACE,     "}");
        case ';': advance(); return Token(Token_ID::ENDOFCOMMAND,    ";");
        case ':': advance(); return Token(Token_ID::SEPERATOR,       ":");
    }

	size_t start = index;
	advance();
	// UTF-8
	while ((currentChar & 0xC0) == 0x80) advance();
	return Token(Token_ID::UNKNOWN, src.substr(start, index - start));
}

Token Lexer::getNextToken() {
start_of_lexer:
	skipWhitespace();

	if (isAtEscapeNullChar()) return Token(Token_ID::EOS);

	if ((currentChar == '/' && peek(1) == '/') || currentChar == '//') {
		skipComments();
		goto start_of_lexer;
	}

	if (currentChar == '\'') {
		return lexCharLiterals();
	}

	if (isdigit((unsigned char)currentChar) || (currentChar == '.' && isdigit((unsigned char)peek(1)))) {
		return lexNumberLiterals();
	}

	Token op = lexOperators();
	if (op.tkID != Token_ID::UNKNOWN) {
		return op;
	}

	if (currentChar == '"') {
		return lexStringLiterals();
	}

	if (isalpha((unsigned char)currentChar) || currentChar == '_') {
		return lexKeywordsAndIdentifiers();
	}

	return lexSpecialCharacters();

	Token tk = Token(Token_ID::UNKNOWN, src.substr(index, 1));
	advance();
	return tk;
}

std::vector<Token> Lexer::tokenize() {
	std::vector<Token> _tokens;
	Token t = getNextToken();

	while (t.tkID != Token_ID::EOS) {
		_tokens.push_back(t);
		t = getNextToken();
	}

	_tokens.push_back(t); // Make sure to include EOS token for parser
	return _tokens;
}

#if DEBUG_MODE
namespace Debug::Lexer {

	constexpr const char* tokenIDToString(Token_ID _id) {
		switch (_id) {
				// --- Literals (Data) ---
			case Token_ID::INTEGER_LITERAL:   return "INTEGER_LITERAL";
			case Token_ID::FLOAT_LITERAL:     return "FLOAT_LITERAL";
			case Token_ID::STRING_LITERAL:    return "STRING_LITERAL";
			case Token_ID::CHAR_LITERAL:      return "CHAR_LITERAL";

				// --- Language Basics ---
			case Token_ID::KEYWORD:           return "KEYWORD";
			case Token_ID::OPERATOR:          return "OPERATOR";
			case Token_ID::IDENTIFIER:        return "IDENTIFIER";

				// --- Grouping & Scoping ---
			case Token_ID::OPEN_PAREN:        return "OPEN_PAREN";
			case Token_ID::CLOSE_PAREN:       return "CLOSE_PAREN";
			case Token_ID::OPEN_BRACE:        return "OPEN_BRACE";
			case Token_ID::CLOSE_BRACE:       return "CLOSE_BRACE";
			case Token_ID::OPEN_SUBSCRIPT:    return "OPEN_SUBSCRIPT";
			case Token_ID::CLOSE_SUBSCRIPT:   return "CLOSE_SUBSCRIPT";

				// --- Special Delimiters ---
			case Token_ID::END_ARG:           return "END_ARG";
			case Token_ID::VA_ARG:            return "VA_ARG";
			case Token_ID::ENDOFCOMMAND:      return "ENDOFCOMMAND";
			case Token_ID::SEPERATOR:		  return "SEPERATOR";

				// --- Advanced Syntax ---
			case Token_ID::TYPE_CAST:         return "TYPE_CAST";
			case Token_ID::SCOPE_RESOLUTION:  return "SCOPE_RESOLUTION";

				// --- System ---
			case Token_ID::EOS:               return "EOS";
			case Token_ID::UNKNOWN:           return "UNKNOWN";
			default:						  return "INVALID_TOKEN_ID";
		}
	}

	void dumpTokenData(Token _token) {

		COMPILER_ASSERT(std::string_view(tokenIDToString(_token.tkID)) != "INVALID_TOKEN_ID", 
			"Lexer passed a token with an invalid ID!");

		std::string tkIDStr = tokenIDToString(_token.tkID);
		std::string info = "[" + tkIDStr + "]";
		for (int i = 0; i < padding - tkIDStr.length(); i++)
			info += " ";
		info += "| [Data] ";

		// Variant data type to string conversion
		// Checks if a pointer toward the data type exists in variant
		if (auto* val = std::get_if<long long>(&_token.data)) {
			info += std::to_string(*val);
		}
		else if (auto* val = std::get_if<double>(&_token.data)) {
			info += std::to_string(*val);
		}
		else if (auto* val = std::get_if<Operator_ID>(&_token.data)) {
			info += Debug::Operator::opIDToString(*val);
		}
		else if (auto* val = std::get_if<std::string_view>(&_token.data)) {
			info += std::string(*val);
		}
		else {
			info += "N/A";
		}

		Log::out(Log::Level::Info, "Token ID found: " + info);
	};
	void dumpAllTokens(std::vector<Token> _tokens) {
		Log::out(Log::Level::Trace, "--- FULL TOKEN DUMP ---");

		for (const auto& _token : _tokens) {
			dumpTokenData(_token);
		}

		Log::out(Log::Level::Trace, "--- END DUMP ---");
	};
}
#endif