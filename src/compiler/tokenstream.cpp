#include "tokenstream.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <codecvt>
#include <exception>
#include <iostream>
#include <locale>
#include <print>

#include "../common/utils.hpp"

// c++ enums are great /s

auto
operator*(TokenType const& t) noexcept -> std::string_view
{
	switch (t) {
		case TokenType::TKeyword:
			return "keyword";
		case TokenType::TIdentifier:
			return "identifier";
		case TokenType::TLiteralDecimal:
			return "decimal";
		case TokenType::TLiteralInteger:
			return "integer";
		case TokenType::TLiteralString:
			return "string";
		case TokenType::TAttribute:
			return "attribute";
		case TokenType::TOperator:
			return "operator";
		case TokenType::TParen:
			return "paren";
		case TokenType::TTerminal:
			return "terminal";
		default:
			return "<TOKEN TYPE>";
	};
}

auto
operator*(Keywords const& t) noexcept -> std::string_view
{
	switch (t) {
		case Keywords::IF:
			return "if";
		case Keywords::ELSE:
			return "else";
		case Keywords::FOR:
			return "for";
		case Keywords::WHILE:
			return "while";
		case Keywords::DO:
			return "do";
		case Keywords::UNTIL:
			return "until";
		case Keywords::BREAK:
			return "break";
		case Keywords::CONTINUE:
			return "continue";
		case Keywords::IN:
			return "in";
		case Keywords::VOID:
			return "void";
		case Keywords::BOOL:
			return "bool";
		case Keywords::U8:
			return "u8";
		case Keywords::U16:
			return "u16";
		case Keywords::U32:
			return "u32";
		case Keywords::U64:
			return "u64";
		case Keywords::U128:
			return "u128";
		case Keywords::UINT:
			return "uint";
		case Keywords::S8:
			return "i8";
		case Keywords::S16:
			return "i16";
		case Keywords::S32:
			return "i32";
		case Keywords::S64:
			return "i64";
		case Keywords::S128:
			return "i128";
		case Keywords::SINT:
			return "int";
		case Keywords::F32:
			return "f32";
		case Keywords::F64:
			return "f64";
		case Keywords::F128:
			return "f128";
		case Keywords::FLOAT:
			return "float";
		case Keywords::C32:
			return "c32";
		case Keywords::C64:
			return "c64";
		case Keywords::C128:
			return "c128";
		case Keywords::COMPLEX:
			return "complex";
		case Keywords::ADDRESS:
			return "address";
		case Keywords::STR:
			return "str";
		case Keywords::AUTO:
			return "auto";
		case Keywords::ANY:
			return "any";
		case Keywords::HANDLE:
			return "handle";
		case Keywords::VARIANT:
			return "variant";
		case Keywords::ENUM:
			return "enum";
		case Keywords::OPEN:
			return "open";
		case Keywords::NONE:
			return "none";
		case Keywords::COPY:
			return "copy";
		case Keywords::MOVE:
			return "move";
		case Keywords::REF:
			return "ref";
		case Keywords::CONST:
			return "const";
		case Keywords::VIEWAS:
			return "view_as";
		case Keywords::CAST:
			return "cast";
		case Keywords::BIND:
			return "bind";
		case Keywords::DEF:
			return "def";
		case Keywords::AS:
			return "as";
		case Keywords::IS:
			return "is";
		case Keywords::SET:
			return "set";
		case Keywords::VEC:
			return "vec";
		case Keywords::LIST:
			return "list";
		case Keywords::DICT:
			return "dict";
		case Keywords::TUPLE:
			return "tuple";
		case Keywords::STRUCT:
			return "struct";
		case Keywords::FUN:
			return "fun";
		case Keywords::METHODMAP:
			return "methodmap";
		case Keywords::EXPORT:
			return "export";
		case Keywords::PUBLIC:
			return "public";
		case Keywords::PROTECTED:
			return "protected";
		case Keywords::PRIVATE:
			return "private";
		case Keywords::NATIVE:
			return "native";
		case Keywords::INCLUDE:
			return "include";
		case Keywords::WITH:
			return "with";
		case Keywords::MATCH:
			return "match";
		case Keywords::PROJECT:
			return "project";
		case Keywords::TYPEOF:
			return "type_of";
		case Keywords::KEYS:
			return "keys";
		default:
			std::cerr << "Fix your enum! (" << (unsigned)t << ")" << std::endl;
			std::terminate();
	};
}

std::array<std::string_view, (size_t)Keywords::_COUNT> allKeywords = {
		*Keywords::IF,        *Keywords::ELSE,    *Keywords::FOR,     *Keywords::WHILE,     *Keywords::DO,      *Keywords::UNTIL,   *Keywords::BREAK,
		*Keywords::CONTINUE,  *Keywords::IN,      *Keywords::VOID,    *Keywords::BOOL,      *Keywords::U8,      *Keywords::U16,     *Keywords::U32,
		*Keywords::U64,       *Keywords::U128,    *Keywords::UINT,    *Keywords::S8,        *Keywords::S16,     *Keywords::S32,     *Keywords::S64,
		*Keywords::S128,      *Keywords::SINT,    *Keywords::F32,     *Keywords::F64,       *Keywords::F128,    *Keywords::FLOAT,   *Keywords::C32,
		*Keywords::C64,       *Keywords::C128,    *Keywords::COMPLEX, *Keywords::ADDRESS,   *Keywords::STR,     *Keywords::AUTO,    *Keywords::ANY,
		*Keywords::HANDLE,    *Keywords::VARIANT, *Keywords::ENUM,    *Keywords::OPEN,      *Keywords::NONE,    *Keywords::COPY,    *Keywords::MOVE,
		*Keywords::REF,       *Keywords::CONST,   *Keywords::VIEWAS,  *Keywords::CAST,      *Keywords::BIND,    *Keywords::DEF,     *Keywords::AS,
		*Keywords::IS,        *Keywords::SET,     *Keywords::VEC,     *Keywords::LIST,      *Keywords::DICT,    *Keywords::STRUCT,  *Keywords::FUN,
		*Keywords::METHODMAP, *Keywords::EXPORT,  *Keywords::PUBLIC,  *Keywords::PROTECTED, *Keywords::PRIVATE, *Keywords::INCLUDE, *Keywords::WITH,
		*Keywords::MATCH,     *Keywords::PROJECT, *Keywords::TYPEOF,  *Keywords::KEYS,
};
static_assert(allKeywords.size() == (unsigned)Keywords::_COUNT);

// end boilerplate

auto
file_error::where() const -> SourceLocation const&
{
	return mWhere;
}

auto
file_error::stacktrace() const -> std::stacktrace const&
{
	return mTrace;
}

void
SourceLocation::advance(char c)
{
	if (c == '\n') {
		line   += 1;
		column  = 1;
	} else {
		column += 1;
	}
}

auto
SourceLocation::relative(int columns, int lines) const -> SourceLocation
{
	return {filename, line + lines, column + columns};
}

auto
SourceLocation::toGCCString() const -> std::string
{
	return std::format("{}:{}:{}", filename, line, column);
}

TokenStream::TokenStream(std::string file)
: stream{file}, pos{std::move(file), 1, 1}
{}

auto
TokenStream::peek(size_t distance) -> std::optional<Token>
{
	fillQueue(distance + 1);
	if (distance >= peekQueue.size()) {
		std::println("[DBG] Peek {}: EOF", distance);
		return {};
	} else {
		std::println("[DBG] Peek {}: {} '{}'", distance, *(peekQueue.at(distance).type), peekQueue.at(distance).token);
		return peekQueue.at(distance);
	}
}

auto
TokenStream::pop() -> Token
{
	fillQueue(2);  // provoke eof
	auto it = peekQueue.front();
	peekQueue.pop_front();
	std::println("[DBG] Pop {} '{}'", *it.type, it.token);
	return prev = it;
}

void
TokenStream::drop(size_t amount)
{
	fillQueue(amount);
	std::println("[DBG] Drop {}", amount);
	if (peekQueue.empty() || amount < 1) {
		return;
	} else if (peekQueue.size() <= amount) {
		peekQueue.clear();
	} else {
		peekQueue.erase(peekQueue.begin(), peekQueue.begin() + amount);
	}
}

auto
TokenStream::eof() const -> bool
{
	std::println("[DBG] Eof: {} and {}", stream.eof(), peekQueue.empty());
	return stream.eof() && peekQueue.empty();
}

auto
TokenStream::fillQueue(size_t size) -> size_t
{
	while (peekQueue.size() < size && !eof()) {
		if (!pushToken()) {
			break;
		}
		if (peekQueue.size() >= 2) {
			// skip condition for new lines as statement terminators is an operator before or after the line break.
			// in either case, remove the new line, we don't need it anymore.
			// note: removing the new line here, under strict rules makes later parsing way easier
			if (peekQueue.crbegin()->type == TokenType::TOperator && (peekQueue.crbegin() + 1)->token == "\n") {
				peekQueue.erase(peekQueue.cend() - 2);
			} else if ((peekQueue.crbegin() + 1)->type == TokenType::TOperator && peekQueue.crbegin()->token == "\n") {
				peekQueue.erase(peekQueue.cend() - 1);
			}
		}
	}
	return peekQueue.size();
}

auto
TokenStream::previous(size_t ofOffset) -> Token
{
	if (ofOffset) {
		return peek(ofOffset - 1).value();
	} else {
		return prev;
	}
}

auto
TokenStream::skipSpace() -> bool
{
	bool hadNewLine{false};
	while (stream.good()) {
		int c = stream.peek();
		if (c == std::ifstream::traits_type::eof() || !std::isspace(c)) {
			break;
		}
		hadNewLine |= c == '\n';
		pos.advance(stream.get());
	}
	return hadNewLine;
}

void
TokenStream::skipComment(bool blockComment)
{
	// expects to start inside a comment
	if (blockComment) {
		char p = '\0';
		readUntil([&](char c) {
			bool blockEnd = p == '*' && c == '/';
			p             = c;
			return blockEnd;
		});
	} else {
		readUntil([](char c) { return c == '\r' || c == '\n'; });
		skipSpace();
	}
}

auto
TokenStream::readWhile(std::function<bool(char)> const& pred) -> std::string
{
	std::vector<char> buf{};
	buf.reserve(32);
	while (stream.good()) {
		int c = stream.peek();
		if (c == std::ifstream::traits_type::eof() || !pred(c)) {
			break;
		}
		buf.emplace_back(c);
		pos.advance(stream.get());
	}
	return std::string{buf.data(), buf.size()};
}

auto
TokenStream::readUntil(std::function<bool(char)> const& pred) -> std::string
{
	std::vector<char> buf{};
	buf.reserve(32);
	while (stream.good()) {
		int c = stream.peek();
		if (c == std::ifstream::traits_type::eof() || pred(c)) {
			break;
		}
		buf.emplace_back(c);
		pos.advance(stream.get());
	}
	return std::string{buf.data(), buf.size()};
}

constexpr auto
charIn(std::string_view choice, int value) noexcept -> bool
{
	if (value == std::istream::traits_type::eof()) {
		return false;
	}
	return choice.contains(value);
}

constexpr auto
identifierChar(int value) noexcept -> bool
{
	if (value == std::istream::traits_type::eof()) {
		return false;
	}
	return std::isalnum(value) || charIn("_", value);
}

auto
TokenStream::readOperator() -> std::string
{
	// grabs all op-ish
	// + - * / %
	// ~ ^ & | << >> >>>
	// ! && || == != < > <= >=
	// = . :: ::? ::[ ::_ |> @
	// , ->
	// : ...
	// ? ! #
	// "grouping": [ ] { } ( )
	// (// /* to detect fast forwards)

	auto peek = stream.peek();
	if (charIn("+*/%~^,@?!#", peek)) {
		pos.advance(stream.get());
		return std::string{(char)peek};
	} else if (peek == '-') {
		// if a minus follows an operator, assume a negative number is about to start
		if (previous(0).type == TokenType::TOperator) {
			return "";
		}
		pos.advance(stream.get());
		if (stream.peek() == '>') {
			pos.advance(stream.get());
			return "->";
		} else {
			return "-";
		}
	} else if (peek == '&') {
		pos.advance(stream.get());
		if (stream.peek() == '&') {
			pos.advance(stream.get());
			return "&&";
		} else {
			return "&";
		}
	} else if (peek == '|') {
		pos.advance(stream.get());
		if (stream.peek() == '|') {
			pos.advance(stream.get());
			return "||";
		} else {
			return "|";
		}
	} else if (peek == '<') {
		pos.advance(stream.get());
		if (stream.peek() == '<') {
			pos.advance(stream.get());
			return "<<";
		} else if (stream.peek() == '=') {
			pos.advance(stream.get());
			return "<=";
		} else {
			return "<";
		}
	} else if (peek == '>') {
		pos.advance(stream.get());
		if (stream.peek() == '>') {
			pos.advance(stream.get());
			if (stream.peek() == '>') {
				pos.advance(stream.get());
				return ">>>";
			} else {
				return ">>";
			}
		} else if (stream.peek() == '=') {
			pos.advance(stream.get());
			return "<=";
		} else {
			return ">";
		}
	} else if (peek == '!') {
		pos.advance(stream.get());
		if (stream.peek() == '=') {
			pos.advance(stream.get());
			return "!=";
		} else {
			return "!";
		}
	} else if (peek == '=') {
		pos.advance(stream.get());
		if (stream.peek() == '=') {
			pos.advance(stream.get());
			return "==";
		} else {
			return "=";
		}
	} else if (peek == '.') {
		pos.advance(stream.get());
		if (stream.peek() == '.') {
			pos.advance(stream.get());
			if (stream.peek() == '.') {
				pos.advance(stream.get());
				return "...";
			} else {
				throw file_error("Unknown operator ..", pos);
			}
		} else {
			return ".";
		}
	} else if (peek == ':') {
		pos.advance(stream.get());
		if (stream.peek() == ':') {
			pos.advance(stream.get());
			if (stream.peek() == '[') {
				pos.advance(stream.get());
				return "::[";
			} else if (stream.peek() == '_') {
				pos.advance(stream.get());
				return "::_";
			} else if (stream.peek() == '?') {
				pos.advance(stream.get());
				return "::?";
			} else {
				return "::";
			}
		} else {
			return ":";
		}
	}
	throw file_error("Unexpected opish around", pos);
}

auto
TokenStream::readNumericLiteral() -> std::tuple<std::string, TokenType>
{
	char first = stream.peek();
	std::string number{};
	std::string post{};
	if (first == '-') {
		number = "-";
		pos.advance(stream.get());
		first = stream.peek();
		if (std::isdigit(first) == 0) {
			return {number, TokenType::TOperator};  // this is now an operator 🤷
		}
	}
	bool supportDecimal{true}, isDecimal{false};
	if (first == '0') {
		pos.advance(stream.get());
		char second = stream.peek();
		if (second == 'x') {
			supportDecimal = false;
			pos.advance(stream.get());
			post = readWhile([](char c) { return std::isxdigit(c) != 0 || c == '_'; });
			std::erase(post, '_');
			if (post.length() == 0) {
				throw file_error("Numeric hex literal has no value after base prefix", pos);
			}
			number += "0x" + post;
			if (stream.peek() == 'p') {
				number += "p";
				pos.advance(stream.get());
				if (stream.peek() == '-') {
					isDecimal  = true;  // negative exponent
					number    += (char)stream.peek();
					pos.advance(stream.get());
				}
				post = readWhile([](char c) { return std::isdigit(c) != 0 || c == '_'; });
				std::erase(post, '_');
				if (post.length() == 0) {
					throw file_error("Numeric hex literal exponent has no value after exponent separator", pos);
				}
				number += post;
			}
		} else if (second == 'o') {
			supportDecimal = false;
			pos.advance(stream.get());
			post = readWhile([](char c) { return ('0' <= c && c <= '7') || c == '_'; });
			std::erase(post, '_');
			if (post.length() == 0) {
				throw file_error("Numeric octal literal has no value after base prefix", pos);
			}
			number += "0o" + post;
		} else if (second == 'b') {
			supportDecimal = false;
			pos.advance(stream.get());
			post = readWhile([](char c) { return c == '0' || c == '1' || c == '_'; });
			std::erase(post, '_');
			if (post.length() == 0) {
				throw file_error("Numeric binary literal has no value after base prefix", pos);
			}
			number += "0b" + post;
		} else {
			readWhile([](char c) { return c == '0' || c == '_'; });  // pop leading zeros
			number += "0";
			if (stream.eof()) {
				return {number, TokenType::TLiteralInteger};
			}
			first = stream.peek();
		}
	}

	if (supportDecimal) {
		number += readWhile([](char c) { return std::isdigit(c) != 0 || c == '_'; });
		if (stream.peek() == '.') {
			pos.advance(stream.get());
			number    += ".";
			isDecimal  = true;
			post       = readWhile([](char c) { return std::isdigit(c) != 0 || c == '_'; });
			std::erase(post, '_');
			if (post.length() == 0) {
				throw file_error("Numeric literal has no value after decimal separator", pos);
			}
			number += post;
			if (stream.peek() == 'e' || stream.peek() == 'p') {
				number += "e";
				pos.advance(stream.get());
				if (stream.peek() == '-') {
					number += (char)stream.peek();
					pos.advance(stream.get());
				}
				post = readWhile([](char c) { return std::isdigit(c) != 0 || c == '_'; });
				std::erase(post, '_');
				if (post.length() == 0) {
					throw file_error("Numeric literal has no value after exponent separator", pos);
				}
				number += post;
			}
		}
	}

	std::erase(number, '_');
	return {number, isDecimal ? TokenType::TLiteralDecimal : TokenType::TLiteralInteger};
}

auto
TokenStream::readStringLiteral() -> std::string
{
	auto readHexits = [&](unsigned length) -> uint32_t {
		uint32_t codepoint{0};
		char conv[2] = {0, 0};
		for (int n{0}; n < length; n++) {
			int c = stream.peek();
			pos.advance(stream.get());
			if (!std::isxdigit(c)) {
				throw file_error("Expected hexit", pos);
			}
			conv[0]   = (char)c;
			codepoint = codepoint * 16 + std::stoi(conv, 0, 16);
		}
		return codepoint;
	};

	std::vector<char> buffer{};
	buffer.reserve(64);
	auto tokenPos = pos;
	pos.advance(stream.get());
	uint16_t highSurrogate{0};
	while (true) {
		if (stream.eof()) {
			throw file_error("Unextected EOF in string literal", tokenPos);
		}
		auto next = stream.get();
		pos.advance(next);
		if (next == '\\') {
			auto escaped = stream.get();
			pos.advance(escaped);
			if (escaped == 'a') {
				buffer.emplace_back('\x07');
			} else if (escaped == 'b') {
				buffer.emplace_back('\x08');
			} else if (escaped == 'e') {
				buffer.emplace_back('\x1B');
			} else if (escaped == 'f') {
				buffer.emplace_back('\x0C');
			} else if (escaped == 'n') {
				buffer.emplace_back('\x0A');
			} else if (escaped == 'r') {
				buffer.emplace_back('\x0D');
			} else if (escaped == 't') {
				buffer.emplace_back('\x09');
			} else if (escaped == 'v') {
				buffer.emplace_back('\x0B');
			} else if (escaped == '\\') {
				buffer.emplace_back('\x5C');
			} else if (escaped == '"') {
				buffer.emplace_back('"');
			} else if (escaped == 'x') {
				char8_t byte = readHexits(2) & 0xFF;
				buffer.emplace_back(byte & 0xFF);
			} else if (escaped == 'u') {
				char16_t surrogate = (uint16_t)(readHexits(4) & 0xFFFF);
				if ((surrogate & 0xFC00) == 0xD800) {
					// high surrogate
					if (highSurrogate) {
						throw file_error("Encoding error, expected utf16 low surrogate after high surrogate", pos);
					}
					highSurrogate = surrogate;
				} else if ((surrogate & 0xFC00) == 0xDC00) {
					// low surrogate
					if (highSurrogate == 0) {
						throw file_error("Encoding error, expected utf16 low surrogate to be preceeded by high surrogate", pos);
					}
					uint32_t codepoint = (((highSurrogate & 0x03FF) << 10) | (surrogate & 0x03FF)) + 0x10000;
					highSurrogate      = 0;
					try {
						std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
						auto str = convert.to_bytes(codepoint);
						buffer.insert(buffer.end(), str.begin(), str.end());
					} catch (...) {
						throw file_error("Encoding error, broken lower surrogate", pos);
					}
				} else {
					// BMP
					try {
						std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> convert;
						auto str = convert.to_bytes(surrogate);
						buffer.insert(buffer.end(), str.begin(), str.end());
					} catch (...) {
						throw file_error("Encoding error, broken BMP charater", pos);
					}
				}
			} else if (escaped == 'U') {
				char32_t codepoint = readHexits(8) & 0xFFFFFFFF;
				try {
					std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
					auto str = convert.to_bytes(codepoint);
					buffer.insert(buffer.end(), str.begin(), str.end());
				} catch (...) {
					throw file_error("Encoding error, invalid utf32 codepoint", pos);
				}
			} else {
				throw file_error(std::format("Unsupported string escape \"\\{}\"", (char)escaped), pos);
			}
		} else {
			if (highSurrogate) {
				throw file_error("Encoding error, expected utf16 low surrogate after high surrogate", pos);
			}
			if (next == '"') {
				break;
			}
			buffer.emplace_back(next);
		}
	}
	return std::string(buffer.data(), buffer.data() + buffer.size());
}

auto
TokenStream::readAttribute() -> std::string
{
	auto tokenPos    = pos;
	auto tokenString = "@" + readWhile(identifierChar);
	readWhile([](const char c) { return c == ' ' || c == '\t'; });
	if (stream.peek() == '(') {
		tokenString += "(";
		pos.advance(stream.get());
		int parens{1};
		bool inQuotes{false};
		while (inQuotes || parens > 0) {
			if (stream.eof()) {
				throw file_error(std::format("Unexpected EOF in attribute value {}", tokenString), tokenPos);
			}
			char next    = stream.get();
			tokenString += next;
			pos.advance(next);
			if (inQuotes && next == '\\') {
				continue;
			} else if (next == '"') {
				inQuotes = !inQuotes;
			} else if (!inQuotes && next == '(') {
				parens++;
			} else if (!inQuotes && next == ')') {
				parens--;
			}
		}
	}
	return tokenString;
}

auto
TokenStream::pushToken() -> bool
{
	if (stream.peek() == std::ifstream::traits_type::eof()) {
		// dont use stream.eof() here because that is not set unless you try to read past eof
		return false;
	}

	auto tokenPos = pos;
	if (skipSpace()) {
		peekQueue.push_back(Token{TokenType::TTerminal, "\n", tokenPos});
	}

	tokenPos  = pos;
	char peek = stream.peek();
	if (stream.eof()) {
		return false;
	}

	if (peek == ';') {
		pos.advance(stream.get());
		peekQueue.push_back(Token{TokenType::TTerminal, ";", tokenPos});
	} else if (peek == '/') {
		pos.advance(stream.get());
		if (stream.peek() == '/') {
			pos.advance(stream.get());
			skipComment(false);
		} else if (stream.peek() == '*') {
			pos.advance(stream.get());
			skipComment(true);
		} else {
			peekQueue.push_back(Token{TokenType::TOperator, "/", tokenPos});
		}
	} else if (charIn("+*%^=<>!~*&|?!:,.#", peek) || (previous(0).type != TokenType::TOperator) && peek == '-') {
		peekQueue.push_back(Token{TokenType::TOperator, readOperator(), tokenPos});
	} else if (peek == '@') {
		pos.advance(stream.get());
		peek = stream.peek();
		if (std::isalpha(peek)) {
			auto tokenString = readAttribute();
			peekQueue.push_back(Token{TokenType::TAttribute, tokenString, tokenPos});
		} else {
			peekQueue.push_back(Token{TokenType::TOperator, "@", tokenPos});
		}
	} else if (charIn("()[]{}", peek) || (previous(0).type == TokenType::TParen && charIn("<>", peek))) {
		pos.advance(stream.get());
		peekQueue.push_back(Token{TokenType::TParen, std::string{peek}, tokenPos});
	} else if (std::isalpha(peek) || charIn("_", peek)) {
		auto tokenString = readWhile(identifierChar);
		auto it          = std::find(allKeywords.cbegin(), allKeywords.cend(), toLowerCase(tokenString));
		if (it != allKeywords.cend()) {
			peekQueue.push_back(Token{TokenType::TKeyword, std::string{*it}, tokenPos});
		} else {
			peekQueue.push_back(Token{TokenType::TIdentifier, tokenString, tokenPos});
		}
	} else if (std::isdigit(peek) || peek == '-') {
		auto [tokenString, tokenType] = readNumericLiteral();
		peekQueue.push_back(Token{tokenType, tokenString, tokenPos});
	} else if (peek == '"') {
		auto tokenString = readStringLiteral();
		peekQueue.push_back(Token{TokenType::TLiteralString, tokenString, tokenPos});
	} else {
		throw file_error(std::format("Unknown token {}", readWhile([](char c) { return c != ' '; })), tokenPos);
	}
	// provoke eof bit
	return stream.peek() != std::ifstream::traits_type::eof();
}

auto
TokenStream::splice(size_t offset, size_t drop, std::initializer_list<Token> insert) -> void
{
	auto max = fillQueue(offset + drop);
	if (max < offset) {
		return;
	}
	auto end = offset + drop;
	if (max < end) {
		end = max;
	}

	auto removeFrom = std::next(peekQueue.begin(), offset);
	auto removeTo   = std::next(peekQueue.begin(), end);
	if (removeTo > peekQueue.end()) {
		removeTo = peekQueue.end();
	}
	peekQueue.erase(removeFrom, removeTo);
	peekQueue.insert(peekQueue.cbegin() + offset, insert.begin(), insert.end());
}
