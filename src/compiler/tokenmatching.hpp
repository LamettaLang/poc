#pragma once

#include <functional>
#include <map>
#include <memory>
#include <variant>
#include <vector>

#include "tokenstream.hpp"

typedef std::function<ssize_t(TokenStreamP stream, size_t offset)> TokenPredicate;

constexpr ssize_t TEST_NO_HIT = -1;

struct TokenMatcher
{
	/// check if this matcher matches the stream at an offset
	///@returns the amount of matched tokens or -1 if no match
	virtual auto
	test(TokenStreamP, size_t) const -> ssize_t = 0;

	inline auto
	matches(TokenStreamP stream) const -> bool
	{
		return test(stream, 0) != TEST_NO_HIT;
	}
};

typedef std::shared_ptr<TokenMatcher> TokenMatcherP;
typedef std::variant<TokenMatcherP, TokenPredicate, std::string, Keywords, TokenType> TokenTestElement;

extern std::map<std::string, TokenMatcherP> NamedMatchers;

namespace TokenTest
{
auto type(TokenType) -> TokenPredicate;
auto token(TokenType, std::string) -> TokenPredicate;
auto keyword(std::string) -> TokenPredicate;
auto operand(std::string) -> TokenPredicate;
auto named(std::string, TokenPredicate) -> TokenMatcherP;
auto byName(std::string) -> TokenMatcherP;
auto sequence(std::initializer_list<TokenTestElement>) -> TokenMatcherP;
auto sequence(std::string, std::initializer_list<TokenTestElement>) -> TokenMatcherP;
auto choice(std::initializer_list<TokenTestElement>) -> TokenMatcherP;
auto choice(std::string, std::initializer_list<TokenTestElement>) -> TokenMatcherP;
auto optional(TokenTestElement) -> TokenMatcherP;
auto optional(std::string, TokenTestElement) -> TokenMatcherP;
auto
repeat(size_t, TokenTestElement) -> TokenMatcherP;
auto
repeat(std::string, size_t, TokenTestElement) -> TokenMatcherP;
};  // namespace TokenTest

class Sequence : public TokenMatcher
{
	std::vector<TokenTestElement> elements;

 public:
	Sequence(std::initializer_list<TokenTestElement>);
	auto
	test(TokenStreamP, size_t) const -> ssize_t override;
};

class Optional : public TokenMatcher
{
	TokenTestElement wrapped{};

 public:
	Optional(TokenTestElement);
	auto
	test(TokenStreamP, size_t) const -> ssize_t override;
};

class Choice : public TokenMatcher
{
	std::vector<TokenTestElement> elements;

 public:
	Choice(std::initializer_list<TokenTestElement>);
	auto
	test(TokenStreamP, size_t) const -> ssize_t override;
};

class Repetition : public TokenMatcher
{
	size_t reps;
	TokenTestElement wrapped;

 public:
	Repetition(size_t reps, TokenTestElement);
	auto
	test(TokenStreamP, size_t) const -> ssize_t override;
};
