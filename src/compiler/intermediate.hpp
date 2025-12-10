#pragma once

#include <fstream>
#include <iostream>
#include <print>
#include <variant>

#include "tokenstream.hpp"
#include "typeinfo.hpp"

struct Statement : public std::enable_shared_from_this<Statement>
{
	SourceLocation pos{};

	virtual inline auto
	isValid() -> bool
	{
		return true;
	};

	virtual inline void
	emitCpp(std::ofstream& outstream)
	{}

	virtual inline void
	dumps(std::ostream& ostream) const {};
};

using StatementP = std::shared_ptr<Statement>;

struct Expression : public Statement
{
	std::weak_ptr<Expression> parent{};

	inline Expression(SourceLocation const& where)
	: parent{}
	{
		pos = where;
	}
};

using ExpressionP = std::shared_ptr<Expression>;

struct Operator : public Expression
{
	Token op;
	unsigned prec;

	inline Operator(Token const& token)
	: Expression(token.at), op{token}, prec{precedence(token)}
	{}

 private:
	static constexpr auto
	precedence(Token const& op) -> unsigned;
};

constexpr auto
Operator::precedence(Token const& op) -> unsigned
{
	if (op.token == "(" || op.token == "[" || op.token == ".") {
		// call, subscript, member
		return 1;
	} else if (op.token == "~" || op.token == "!" || op.token == "#") {
		// negations (unary)
		return 2;
	} else if (op.token == "*" || op.token == "/" || op.token == "%") {
		// math
		return 3;
	} else if (op.token == "+" || op.token == "-") {
		// math
		return 4;
	} else if (op.token == "<<" || op.token == ">>" || op.token == ">>>") {
		// bit shift
		return 5;
	} else if (op.token == "<" || op.token == ">" || op.token == "<=" || op.token == ">=") {
		// inequality
		return 6;
	} else if (op.token == "==" || op.token == "!=") {
		// equality
		return 7;
	} else if (op.token == "&") {
		// bit and
		return 8;
	} else if (op.token == "^") {
		// bit xor
		return 9;
	} else if (op.token == "|") {
		// bit or
		return 10;
	} else if (op.token == "&&") {
		// logic and
		return 11;
	} else if (op.token == "||") {
		// logic or
		return 12;
	} else if (op.token == "=") {
		return 13;
	} else if (op.token == ",") {
		return 14;
	}
	throw file_error(std::format("Invalid operator {}", op.token), op.at);
}

struct BinaryOp : public Operator
{
	ExpressionP left{};
	ExpressionP right{};

	inline BinaryOp(Token const& token)
	: Operator(token)
	{}

	inline void
	dumps(std::ostream& ostream) const override
	{
		ostream << "(";
		if (left) {
			left->dumps(ostream);
		} else {
			ostream << "<?>";
		}
		ostream << op.token;
		if (right) {
			right->dumps(ostream);
		} else {
			ostream << "<?>";
		}
		ostream << ")";
	}
};

struct UnaryOp : public Operator
{
	ExpressionP child{};

	inline UnaryOp(Token const& token)
	: Operator(token)
	{}

	inline void
	dumps(std::ostream& ostream) const override
	{
		ostream << "(" << op.token;
		if (child) {
			child->dumps(ostream);
		} else {
			ostream << "<?>";
		}
		ostream << ")";
	}
};

struct RangeExpr : public Expression
{
	ExpressionP start, step, end;
	bool endInclusive;

	inline RangeExpr(Token const& token)
	: Expression(token.at)
	{}

	inline void
	dumps(std::ostream& ostream) const override
	{
		ostream << "[";
		start->dumps(ostream);
		ostream << ":";
		if (step) {
			step->dumps(ostream);
			ostream << ":";
		}
		end->dumps(ostream);
		ostream << (endInclusive ? "]" : "[");
	}
};

struct VariableDeclaration : public Expression
{
	TypeInfo type;
	std::string name;
};

enum struct ValueKind
{
	INT8,
	UINT8,
	INT16,
	UINT16,
	INT32,
	UINT32,
	INT64,
	UINT64,
	INT128,
	UINT128,
	FLOAT16,
	FLOAT32,
	FLOAT64,
	FLOAT128,
	STRING,
	LABEL,
};

struct Value : public Expression
{
	ValueKind kind;
	std::string rep;

	Value(Token const& token);

	inline void
	dumps(std::ostream& ostream) const override
	{
		ostream << rep;
	}
};

struct FunctionCall : public Value
{
	std::vector<ExpressionP> arguments{};

	FunctionCall(Token const& token);
	FunctionCall(Value const& label);

	inline void
	dumps(std::ostream& ostream) const override
	{
		ostream << rep << "(";
		bool first = true;
		for (auto const& elem : arguments) {
			if (!first) {
				ostream << ", ";
			}
			std::cout << elem;
			first = false;
		}
		ostream << ")";
	}
};

/// this is

struct DiscreteType;
struct LatentType;

typedef std::variant<DiscreteType, LatentType> LType;

struct DiscreteType
{
	SourceLocation declpos;
	std::vector<std::string> labels{};  ///< filled by defs
	std::string tid{};                  ///< type id : mangled string
	ssize_t size{-1};                   ///< -1 for dynamic size

	std::vector<LType> elements{};      ///< for structs and tuples, or element type
	ssize_t length{-1};                 ///< for arrays, -1 = pod
	Keywords kind = Keywords::NONE;     ///< kind of contaier, if any

	DiscreteType(Token const& token);
};

struct LatentType
{
	SourceLocation refpos;
	std::string label{};

	LatentType(Token const& token)
	: refpos{token.at}, label{token.token}
	{}
};
