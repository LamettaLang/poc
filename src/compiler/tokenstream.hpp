#pragma once

#include <deque>
#include <fstream>
#include <functional>
#include <memory>
#include <stacktrace>
#include <string>
#include <utility>

struct SourceLocation
{
	std::string filename{};
	size_t line{0};
	size_t column{0};

	void
	advance(char c);
	[[nodiscard]]
	auto
	relative(int columns, int lines = 0) const -> SourceLocation;
	[[nodiscard]]
	auto
	toGCCString() const -> std::string;
};

class file_error : public std::runtime_error
{
	SourceLocation mWhere;
	std::stacktrace mTrace;

 public:
	file_error(const char* what, SourceLocation where, std::stacktrace trace = std::stacktrace::current())
	: runtime_error(what), mWhere(std::move(where)), mTrace(std::move(trace))
	{}

	file_error(std::string const& what, SourceLocation where, std::stacktrace trace = std::stacktrace::current())
	: runtime_error(what), mWhere(std::move(where)), mTrace(std::move(trace))
	{}

	[[nodiscard]]
	auto
	where() const -> SourceLocation const&;
	[[nodiscard]]
	auto
	stacktrace() const -> std::stacktrace const&;
};

enum struct TokenType
{
	INVALID,
	TKeyword,
	TIdentifier,
	TLiteralInteger,
	TLiteralDecimal,
	TLiteralString,
	TAttribute,  ///< @word
	TOperator,   ///< /+*%^=<>!~*&|?!:,.#-@
	TParen,      ///< ()[]{}<>
	TTerminal,   ///< ;\n
};

auto
operator*(TokenType const& t) noexcept -> std::string_view;

enum struct Keywords
{
	IF,
	ELSE,
	FOR,
	WHILE,
	DO,
	UNTIL,
	BREAK,
	CONTINUE,
	IN,
	VOID,
	BOOL,
	U8,
	U16,
	U32,
	U64,
	U128,
	UINT,
	S8,
	S16,
	S32,
	S64,
	S128,
	SINT,
	F32,
	F64,
	F128,
	FLOAT,
	C32,
	C64,
	C128,
	COMPLEX,
	ADDRESS,
	STR,
	AUTO,
	ANY,
	HANDLE,
	VARIANT,
	ENUM,
	OPEN,
	NONE,
	COPY,
	MOVE,
	REF,
	CONST,
	VIEWAS,
	CAST,
	BIND,
	DEF,
	AS,
	IS,
	SET,
	VEC,
	LIST,
	DICT,
	TUPLE,
	STRUCT,
	FUN,
	METHODMAP,
	EXPORT,
	PUBLIC,
	PROTECTED,
	PRIVATE,
	NATIVE,
	INCLUDE,
	WITH,
	MATCH,
	PROJECT,
	TYPEOF,
	KEYS,
	_COUNT,
};

auto
operator*(Keywords const& t) noexcept -> std::string_view;

struct Token
{
	TokenType type{TokenType::INVALID};
	std::string token{};
	SourceLocation at{};
};

class TokenStream
{
	Token prev{};
	std::deque<Token> peekQueue{};
	std::ifstream stream;
	SourceLocation pos;

 public:
	// since ifstream is move only, make this explicitly move only
	TokenStream(TokenStream const&)     = delete;
	TokenStream(TokenStream&&) noexcept = default;
	~TokenStream()                      = default;
	auto
	operator=(TokenStream const&) -> TokenStream& = delete;
	auto
	operator=(TokenStream&&) noexcept -> TokenStream& = default;

	explicit TokenStream(std::string file);
	auto
	peek(size_t distance = 0) -> std::optional<Token>;
	auto
	splice(size_t offset, size_t drop = 0, std::initializer_list<Token> insert = {}) -> void;
	auto
	pop() -> Token;
	void
	drop(size_t amount);
	auto
	eof() const -> bool;

	inline auto
	getPosition() -> SourceLocation
	{
		return pos;
	}

 private:
	auto
	pushToken() -> bool;
	/** return new peekQueue size */
	auto
	fillQueue(size_t size) -> size_t;
	auto
	skipSpace() -> bool;
	void
	skipComment(bool block);
	auto
	readWhile(std::function<bool(char)> const& pred) -> std::string;
	auto
	readUntil(std::function<bool(char)> const& pred) -> std::string;
	auto
	previous(size_t ofOffset) -> Token;
	auto
	readOperator() -> std::string;
	auto
	readNumericLiteral() -> std::tuple<std::string, TokenType>;
	auto
	readStringLiteral() -> std::string;
	auto
	readAttribute() -> std::string;
};

using TokenStreamP = std::shared_ptr<TokenStream>;
