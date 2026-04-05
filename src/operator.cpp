#include "debug.h"
#include "error.h"
#include "operator.h"

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>

static const std::vector<Operator> opList = [] {
    std::vector<Operator> tmp = {
        // --- Postfix & Primary (Precedence 150-160) ---
        { Operator_ID::opScopeResolution,   _Associativity::assocLeft,  _Arity::Infix,   160, "::" },
        { Operator_ID::opMemberAccess,      _Associativity::assocLeft,  _Arity::Infix,   150, "."  },
        { Operator_ID::opDerefMemberAccess, _Associativity::assocLeft,  _Arity::Infix,   150, "->" },
        { Operator_ID::opSubscript,         _Associativity::assocLeft,  _Arity::Infix,   150, "["  },
        { Operator_ID::opFunctionalCall,    _Associativity::assocLeft,  _Arity::Infix,   150, "("  },

        // --- Ambiguous Tokens (Lexer Output) ---
        // These replace the specific Unary/Binary/Prefix/Postfix versions in the Lexer search.
        { Operator_ID::opGenericIncrement,  _Associativity::assocNone,  _Arity::arNULL,  0, "++" },
        { Operator_ID::opGenericDecrement,  _Associativity::assocNone,  _Arity::arNULL,  0, "--" },
        { Operator_ID::opGenericPlus,       _Associativity::assocNone,  _Arity::arNULL,  0, "+"  },
        { Operator_ID::opGenericMinus,      _Associativity::assocNone,  _Arity::arNULL,  0, "-"  },
        { Operator_ID::opGenericStar,       _Associativity::assocNone,  _Arity::arNULL,  0, "*"  },
        { Operator_ID::opGenericAmpersand,  _Associativity::assocNone,  _Arity::arNULL,  0, "&"  },

        // --- Prefix & Unary (Unique Symbols) ---
        { Operator_ID::opLogicalNot,        _Associativity::assocRight, _Arity::Prefix,  140, "!"  },
        { Operator_ID::opBitwiseNot,        _Associativity::assocRight, _Arity::Prefix,  140, "~"  },

        // --- Binary: Arithmetic (Unique Symbols) ---
        { Operator_ID::opExp,               _Associativity::assocRight, _Arity::Infix,   135, "^^" },
        { Operator_ID::opDiv,               _Associativity::assocLeft,  _Arity::Infix,   130, "/"  },
        { Operator_ID::opMod,               _Associativity::assocLeft,  _Arity::Infix,   130, "%"  },

        // --- Binary: Bitwise & Shifts (Precedence 80-110) ---
        { Operator_ID::opLeftShift,         _Associativity::assocLeft,  _Arity::Infix,   110, "<<" },
        { Operator_ID::opRightShift,        _Associativity::assocLeft,  _Arity::Infix,   110, ">>" },
        { Operator_ID::opBitwiseXor,        _Associativity::assocLeft,  _Arity::Infix,   90,  "^"  },
        { Operator_ID::opBitwiseOr,         _Associativity::assocLeft,  _Arity::Infix,   80,  "|"  },

        // --- Binary: Comparison (Precedence 60-70) ---
        { Operator_ID::opLessThanEqual,     _Associativity::assocLeft,  _Arity::Infix,   70,  "<=" },
        { Operator_ID::opGreaterThanEqual,  _Associativity::assocLeft,  _Arity::Infix,   70,  ">=" },
        { Operator_ID::opLessThan,          _Associativity::assocLeft,  _Arity::Infix,   70,  "<"  },
        { Operator_ID::opGreaterThan,       _Associativity::assocLeft,  _Arity::Infix,   70,  ">"  },
        { Operator_ID::opEqual,             _Associativity::assocLeft,  _Arity::Infix,   60,  "==" },
        { Operator_ID::opNotEqual,          _Associativity::assocLeft,  _Arity::Infix,   60,  "!=" },

        // --- Binary: Logical (Precedence 40-50) ---
        { Operator_ID::opLogicalAnd,        _Associativity::assocLeft,  _Arity::Infix,   50,  "&&" },
        { Operator_ID::opLogicalOr,         _Associativity::assocLeft,  _Arity::Infix,   40,  "||" },

        // --- Assignment (Precedence 20) ---
        { Operator_ID::opAssign,            _Associativity::assocRight, _Arity::Infix,   20,  "="   },
        { Operator_ID::opAddAssign,         _Associativity::assocRight, _Arity::Infix,   20,  "+="  },
        { Operator_ID::opSubAssign,         _Associativity::assocRight, _Arity::Infix,   20,  "-="  },
        { Operator_ID::opMulAssign,         _Associativity::assocRight, _Arity::Infix,   20,  "*="  },
        { Operator_ID::opDivAssign,         _Associativity::assocRight, _Arity::Infix,   20,  "/="  },
        { Operator_ID::opModAssign,         _Associativity::assocRight, _Arity::Infix,   20,  "%="  },
        { Operator_ID::opLeftShiftAssign,   _Associativity::assocRight, _Arity::Infix,   20,  "<<=" },
        { Operator_ID::opRightShiftAssign,  _Associativity::assocRight, _Arity::Infix,   20,  ">>=" },
        { Operator_ID::opAndAssign,         _Associativity::assocRight, _Arity::Infix,   20,  "&="  },
        { Operator_ID::opXorAssign,         _Associativity::assocRight, _Arity::Infix,   20,  "^="  },
        { Operator_ID::opOrAssign,          _Associativity::assocRight, _Arity::Infix,   20,  "|="  },

        // --- Misc (Precedence 0-10) ---
        { Operator_ID::opComma,             _Associativity::assocLeft,  _Arity::Infix,   10,  ","   },
        { Operator_ID::opVaArg, _Associativity::assocNone, _Arity::arNULL, 0, "..." },
    };

    // Sorting by length ensures "++" is matched before "+"
    std::sort(tmp.begin(), tmp.end(), [](const Operator& a, const Operator& b) {
        return std::strlen(a.symbol) > std::strlen(b.symbol);
        });

    return tmp;
} ();// Lambda function automatically executes, and lets const values be computed at runtime.

// For O(1) lookup time in the parser
const std::unordered_map<Operator_ID, Operator> opTable = [] {
	std::unordered_map<Operator_ID, Operator> tmp;

	// The map will automatically create any entry that is searched but not found.
	// At key op.opID, set the data to op.
    for (const auto& op : opList) {
        tmp[op.opID] = op;

        // Replace generic ids with specifics for the parser.
        switch (op.opID) {
        case Operator_ID::opGenericPlus:
            tmp[Operator_ID::opUnaryPlus] = { Operator_ID::opUnaryPlus, _Associativity::assocRight, _Arity::Prefix, 140, "+" };
            tmp[Operator_ID::opAdd] = { Operator_ID::opAdd,       _Associativity::assocLeft,  _Arity::Infix,  120, "+" };
            break;

        case Operator_ID::opGenericMinus:
            tmp[Operator_ID::opUnaryMinus] = { Operator_ID::opUnaryMinus, _Associativity::assocRight, _Arity::Prefix, 140, "-" };
            tmp[Operator_ID::opSub] = { Operator_ID::opSub,        _Associativity::assocLeft,  _Arity::Infix,  120, "-" };
            break;

        case Operator_ID::opGenericStar:
            tmp[Operator_ID::opDereference] = { Operator_ID::opDereference, _Associativity::assocRight, _Arity::Prefix, 140, "*" };
            tmp[Operator_ID::opMul] = { Operator_ID::opMul,         _Associativity::assocLeft,  _Arity::Infix,  130, "*" };
            break;

        case Operator_ID::opGenericAmpersand:
            tmp[Operator_ID::opReference] = { Operator_ID::opReference,  _Associativity::assocRight, _Arity::Prefix, 140, "&" };
            tmp[Operator_ID::opBitwiseAnd] = { Operator_ID::opBitwiseAnd, _Associativity::assocLeft,  _Arity::Infix,  100, "&" };
            break;

        case Operator_ID::opGenericIncrement:
            tmp[Operator_ID::opPrefixIncrement] = { Operator_ID::opPrefixIncrement,  _Associativity::assocRight, _Arity::Prefix,  140, "++" };
            tmp[Operator_ID::opPostfixIncrement] = { Operator_ID::opPostfixIncrement, _Associativity::assocLeft,  _Arity::Postfix, 150, "++" };
            break;

        case Operator_ID::opGenericDecrement:
            tmp[Operator_ID::opPrefixDecrement] = { Operator_ID::opPrefixDecrement,  _Associativity::assocRight, _Arity::Prefix,  140, "--" };
            tmp[Operator_ID::opPostfixDecrement] = { Operator_ID::opPostfixDecrement, _Associativity::assocLeft,  _Arity::Postfix, 150, "--" };
            break;

        default:
            break;
        }
    }

	return tmp;
} ();

const std::vector<Operator>& getOpList() {
	return opList;
}

#if DEBUG_MODE
namespace Debug::Operator {

    constexpr const char* opIDToString(Operator_ID _id) {
        switch (_id) {
            // Postfix & Primary
        case Operator_ID::opScopeResolution:   return "SCOPE_RESOLUTION";
        case Operator_ID::opMemberAccess:      return "MEMBER_ACCESS";
        case Operator_ID::opDerefMemberAccess: return "DEREF_MEMBER_ACCESS";
        case Operator_ID::opSubscript:         return "SUBSCRIPT";
        case Operator_ID::opFunctionalCall:    return "FUNCTION_CALL";
        case Operator_ID::opFunctionalCast:    return "FUNCTION_CAST";
        case Operator_ID::opPostfixIncrement:  return "POSTFIX_INCREMENT";
        case Operator_ID::opPostfixDecrement:  return "POSTFIX_DECREMENT";

            // Prefix & Unary
        case Operator_ID::opPrefixIncrement:   return "PREFIX_INCREMENT";
        case Operator_ID::opPrefixDecrement:   return "PREFIX_DECREMENT";
        case Operator_ID::opUnaryPlus:         return "UNARY_PLUS";
        case Operator_ID::opUnaryMinus:        return "UNARY_MINUS";
        case Operator_ID::opLogicalNot:        return "LOGICAL_NOT";
        case Operator_ID::opBitwiseNot:        return "BITWISE_NOT";
        case Operator_ID::opDereference:       return "DEREFERENCE";
        case Operator_ID::opReference:         return "REFERENCE";

            // Binary: Arithmetic
        case Operator_ID::opAdd:               return "ADD";
        case Operator_ID::opSub:               return "SUB";
        case Operator_ID::opMul:               return "MUL";
        case Operator_ID::opDiv:               return "DIV";
        case Operator_ID::opExp:               return "EXP";
        case Operator_ID::opMod:               return "MOD";

            // Binary: Bitwise & Shifts
        case Operator_ID::opLeftShift:         return "LEFT_SHIFT";
        case Operator_ID::opRightShift:        return "RIGHT_SHIFT";
        case Operator_ID::opBitwiseAnd:        return "BITWISE_AND";
        case Operator_ID::opBitwiseXor:        return "BITWISE_XOR";
        case Operator_ID::opBitwiseOr:         return "BITWISE_OR";

            // Binary: Comparison
        case Operator_ID::opLessThan:          return "LESS_THAN";
        case Operator_ID::opGreaterThan:       return "GREATER_THAN";
        case Operator_ID::opLessThanEqual:     return "LESS_THAN_EQUAL";
        case Operator_ID::opGreaterThanEqual:  return "GREATER_THAN_EQUAL";
        case Operator_ID::opEqual:             return "EQUAL";
        case Operator_ID::opNotEqual:          return "NOT_EQUAL";

            // Binary: Logical
        case Operator_ID::opLogicalAnd:        return "LOGICAL_AND";
        case Operator_ID::opLogicalOr:         return "LOGICAL_OR";

            // Assignment
        case Operator_ID::opAssign:            return "ASSIGN";
        case Operator_ID::opAddAssign:         return "ADD_ASSIGN";
        case Operator_ID::opSubAssign:         return "SUB_ASSIGN";
        case Operator_ID::opMulAssign:         return "MUL_ASSIGN";
        case Operator_ID::opDivAssign:         return "DIV_ASSIGN";
        case Operator_ID::opModAssign:         return "MOD_ASSIGN";
        case Operator_ID::opLeftShiftAssign:   return "LEFT_SHIFT_ASSIGN";
        case Operator_ID::opRightShiftAssign:  return "RIGHT_SHIFT_ASSIGN";
        case Operator_ID::opAndAssign:         return "AND_ASSIGN";
        case Operator_ID::opXorAssign:         return "XOR_ASSIGN";
        case Operator_ID::opOrAssign:          return "OR_ASSIGN";

            // --- Ambiguous Tokens (Lexer Output) ---
        case Operator_ID::opGenericPlus:       return "GENERIC_PLUS";
        case Operator_ID::opGenericMinus:      return "GENERIC_MINUS";
        case Operator_ID::opGenericStar:       return "GENERIC_STAR";
        case Operator_ID::opGenericAmpersand:  return "GENERIC_AMPERSAND";
        case Operator_ID::opGenericIncrement:  return "GENERIC_INCREMENT";
        case Operator_ID::opGenericDecrement:  return "GENERIC_DECREMENT";

            // Misc
        case Operator_ID::opComma:             return "COMMA";
        case Operator_ID::opNULL:              return "NULL_OP";
        default:                               return "UNKNOWN_OP";
        }
    }

    constexpr const char* assocToString(_Associativity _assoc) {
        switch (_assoc) {
        case _Associativity::assocLeft:  return "LEFT";
        case _Associativity::assocRight: return "RIGHT";
        case _Associativity::assocNone:  return "NONE";
        }
        return "UNKNOWN_ASSOC";
    }

    constexpr const char* arityToString(_Arity _arity) {
        switch (_arity) {
        case _Arity::Prefix:  return "PREFIX";
        case _Arity::Infix:   return "INFIX";
        case _Arity::Postfix: return "POSTFIX";
        case _Arity::arNULL:  return "NULL_ARITY";
        }
        return "UNKNOWN_ARITY";
    }

    std::string padString(int _padding, std::string _str) {
        for (int i = 0; i < _padding; i++)
            _str += " ";
        return _str;
    }

    void dumpOpData(Operator_ID _id) {
        auto it = opTable.find(_id);

        // If id is not found, it points to a marker at the end of the map
        COMPILER_ASSERT(it != opTable.end(), "Lexer passed an ID that doesn't exist in opTable!");

        const auto& data = it->second;

        std::string info = "[" + std::string(data.symbol) + "]";
        info = padString(padding - ((std::string)data.symbol).length(), info);
        info += " | [" + (std::string)opIDToString(data.opID) + "]";
        // Hardcoded bc lazy, opIDs are longer than everything else
        info = padString(padding + 15 - ((std::string)opIDToString(data.opID)).length(), info);
        info += " | [PREC]: " + std::to_string(data.Precedence);
        info = padString(padding - std::to_string(data.Precedence).length(), info);
        info += " | [ASSOC]: " + (std::string)assocToString(data.Associativity);
        info = padString(padding - ((std::string)assocToString(data.Associativity)).length(), info);
        info += " | [ARITY]: " + (std::string)arityToString(data.Arity);

        Log::out(Log::Level::Info, "opID found: " + info);
    }

    void dumpAllOpData() {
        Log::out(Log::Level::Trace, "--- FULL OPERATOR TABLE DUMP ---");

        for (const auto& op : getOpList()) {
            dumpOpData(op.opID);
        }

        Log::out(Log::Level::Trace, "--- END DUMP ---");
    }
}
#endif