#pragma once

#include "debug.h"

#include <cstddef>
#include <vector>
#include <unordered_map>

#if DEBUG_MODE
#include <string>
#endif

enum class Operator_ID {

	// Postfix & Primary
	opScopeResolution,   // a::b
	opMemberAccess,      // a.b
	opDerefMemberAccess, // a->b
	opSubscript,         // a[b] => Access array index
	opFunctionalCall,    // a(b)
	opFunctionalCast,    // T(a) => Type cast
	opPostfixIncrement,  // a++
	opPostfixDecrement,  // a--


	// Prefix & Unary
	opPrefixIncrement,   // ++a
	opPrefixDecrement,    // --a
	opUnaryPlus,         // +a
	opUnaryMinus,        // -a
	opLogicalNot,        // !a
	opBitwiseNot,        // ~a
	opDereference,       // *a
	opReference,         // &a

	// Binary: Arithmetic
	opAdd,               // a + b
	opSub,               // a - b
	opMul,               // a * b
	opDiv,               // a / b
	opExp,               // a ^^ b
	opMod,               // a % b

	// Binary: Bitwise & Shifts
	opLeftShift,         // a << b
	opRightShift,        // a >> b
	opBitwiseAnd,        // a & b
	opBitwiseXor,        // a ^ b
	opBitwiseOr,         // a | b

	// Binary: Comparison
	opLessThan,          // a < b
	opGreaterThan,       // a > b
	opLessThanEqual,     // a <= b
	opGreaterThanEqual,  // a >= b
	opEqual,             // a == b
	opNotEqual,          // a != b

	// Binary: Logical
	opLogicalAnd,        // a && b
	opLogicalOr,         // a || b

	// Assignment
	opAssign,            // a = b
	opAddAssign,         // a += b
	opSubAssign,         // a -= b
	opMulAssign,         // a *= b
	opDivAssign,         // a /= b
	opModAssign,         // a %= b
	opLeftShiftAssign,   // a <<= b
	opRightShiftAssign,  // a >>= b
	opAndAssign,         // a &= b
	opXorAssign,         // a ^= b
	opOrAssign,          // a |= b

	// --- Ambiguous Tokens (Lexer Output) ---
	opGenericPlus,      // +
	opGenericMinus,     // -
	opGenericStar,      // *
	opGenericAmpersand, // &
	opGenericIncrement, // ++
	opGenericDecrement, // --

	// Misc
	opComma,             // ,
	opVaArg,             // ...
	opNULL,              // N/A
};

enum class _Associativity {
	assocLeft,
	assocRight,
	assocNone
};

enum class _Arity {
	Prefix,              // ++a, --a, !a
	Infix,               // a == b, a != b
	Postfix,             // a++, a--
	arNULL,              // Default
};

struct Operator {
	Operator_ID opID;
	_Associativity Associativity;
	_Arity Arity;
	size_t Precedence;
	const char* symbol;

	Operator() : // Default NULL constructor
		opID(Operator_ID::opNULL),
		Associativity(_Associativity::assocNone),
		Arity(_Arity::arNULL),
		Precedence(-1), // "infinite" precedence 0xFFFFFFFFFFFFFFFF (64 bit) since size_t is unsigned
		symbol("NULL") {}

	Operator(Operator_ID _id, _Associativity _assoc, _Arity _arity, size_t _prec, const char* _sym)
		: opID(_id), Associativity(_assoc), Arity(_arity), Precedence(_prec), symbol(_sym) {}
};

// extern prevents multiple copies in memory for public map
extern const std::unordered_map<Operator_ID, Operator> opTable;

// Getter function for static (private) vector
const std::vector<Operator>& getOpList();

#if DEBUG_MODE
namespace Debug::Operator {
	const int padding = 5;
	constexpr const char* opIDToString(Operator_ID _id);
	constexpr const char* assocToString(_Associativity _assoc);
	constexpr const char* arityToString(_Arity _Arity);
	std::string padString(int _padding, std::string _str);
	void dumpOpData(Operator_ID id);
	void dumpAllOpData();
}
#endif