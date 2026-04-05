#pragma once

#include <string>

// Debug mode for Visual studio
#ifdef _DEBUG
	#define DEBUG_MODE 1
#else
	#define DEBUG_MODE 0
#endif

#if DEBUG_MODE
	enum class Operator_ID;
	enum class _Associativity;
	enum class _Arity;
	namespace Debug::Operator {
		constexpr const char* opIDToString(Operator_ID _id);
		constexpr const char* assocToString(_Associativity _assoc);
		constexpr const char* arityToString(_Arity _arity);
		std::string padString(int _padding, std::string _str);
		void dumpOpData(Operator_ID _id);
		void dumpAllOpData();
	}

	struct Token;
	enum class Token_ID;
	namespace Debug::Lexer {
		constexpr const char* tokenIDToString(Token_ID _id);
		void dumpTokenData();
		void dumpAllTokens();
	}

	#define DEBUG(x) x
#else
	// If not in debug mode, this macro removes all debug functions from the compiler
	#define DEBUG(x)
#endif