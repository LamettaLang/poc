#include "tokenmatching.hpp"

#include "common/traced_error.hpp"

auto
canSkipToken(TokenStreamP stream, size_t offset, TokenType nextType) -> bool
{
	auto token = stream->peek(offset);
	return token && token->type == TokenType::TTerminal && nextType != TokenType::TTerminal;
}

auto
TokenTest::type(TokenType type) -> TokenPredicate
{
	return [=](TokenStreamP stream, size_t offset) {
		auto token = stream->peek(offset);
		return (token && token->type == type) ? 1 : TEST_NO_HIT;
	};
}

auto
cicmp(std::string_view a, std::string_view b) -> int
{
	auto cmp = a.length() - b.length();
	if (cmp != 0) {
		return cmp;
	}
	auto ita = a.cbegin();
	auto itb = b.cbegin();
	for (; ita != a.cend(); ita++, itb++) {
		cmp = tolower(*ita) - tolower(*itb);
		if (cmp != 0) {
			return cmp;
		}
	}
	return 0;
}

auto
TokenTest::token(TokenType type, std::string value) -> TokenPredicate
{
	return [=](TokenStreamP stream, size_t offset) {
		auto token = stream->peek(offset);
		return (token && token->type == type && cicmp(token->token, value) == 0) ? 1 : TEST_NO_HIT;
	};
}

auto
TokenTest::keyword(std::string value) -> TokenPredicate
{
	return token(TokenType::TKeyword, value);
}

auto
TokenTest::operand(std::string value) -> TokenPredicate
{
	return token(TokenType::TOperator, value);
}

auto
TokenTest::optional(TokenTestElement element) -> TokenMatcherP
{
	return std::make_shared<Optional>(std::move(element));
}

auto
TokenTest::optional(std::string name, TokenTestElement element) -> TokenMatcherP
{
	auto test = optional(std::move(element));
	return (NamedMatchers[name] = test);
}

auto
TokenTest::sequence(std::initializer_list<TokenTestElement> elements) -> TokenMatcherP
{
	return std::make_shared<Sequence>(std::move(elements));
}

auto
TokenTest::sequence(std::string name, std::initializer_list<TokenTestElement> elements) -> TokenMatcherP
{
	auto test = sequence(std::move(elements));
	return (NamedMatchers[name] = test);
}

auto
TokenTest::choice(std::initializer_list<TokenTestElement> elements) -> TokenMatcherP
{
	return std::make_shared<Choice>(std::move(elements));
}

auto
TokenTest::choice(std::string name, std::initializer_list<TokenTestElement> elements) -> TokenMatcherP
{
	auto test = choice(std::move(elements));
	return (NamedMatchers[name] = test);
}

auto
TokenTest::repeat(size_t reps, TokenTestElement element) -> TokenMatcherP
{
	return std::make_shared<Repetition>(reps, std::move(element));
}

auto
TokenTest::repeat(std::string name, size_t reps, TokenTestElement element) -> TokenMatcherP
{
	auto test = repeat(reps, std::move(element));
	return (NamedMatchers[name] = test);
}

std::map<std::string, TokenMatcherP> NamedMatchers{};

inline auto
testAt(TokenStreamP stream, size_t offset, TokenTestElement element) -> ssize_t
{
	if (std::holds_alternative<std::string>(element)) {
		auto name = std::get<std::string>(element);
		try {
			auto matcher = NamedMatchers.at(name);
			return matcher->test(stream, offset);
		} catch (std::exception const& e) {
			throw stacktraced_error(e);
		}
	} else if (std::holds_alternative<TokenMatcherP>(element)) {
		auto matcher = std::get<TokenMatcherP>(element);
		return matcher->test(stream, offset);
	} else if (std::holds_alternative<TokenPredicate>(element)) {
		auto predicate = std::get<TokenPredicate>(element);
		return predicate(stream, offset);
	} else if (std::holds_alternative<TokenType>(element)) {
		auto tokenType = std::get<TokenType>(element);
		auto token     = stream->peek(offset);
		return token && token->type == tokenType ? 1 : TEST_NO_HIT;
	} else if (std::holds_alternative<Keywords>(element)) {
		auto keyword = *std::get<Keywords>(element);
		auto token   = stream->peek(offset);
		return token && token->type == TokenType::TKeyword && token->token == keyword ? 1 : TEST_NO_HIT;
	} else {
		throw stacktraced_error(std::runtime_error("testAt holds unmatched variant"));
	}
}

/** thin wrapper, not to be manually constructed, thus only in cpp */
class TokenPredicateMatcher : public TokenMatcher
{
	TokenPredicate predicate;

 public:
	TokenPredicateMatcher(TokenPredicate predicate)
	: predicate(predicate)
	{}

	inline auto
	test(TokenStreamP stream, size_t offset) const -> ssize_t override
	{
		return predicate(stream, offset);
	}
};

auto
TokenTest::named(std::string name, TokenPredicate predicate) -> TokenMatcherP
{
	class TokenPredicateMatcher : public TokenMatcher
	{
	 private:
		TokenPredicate pred;

	 public:
		TokenPredicateMatcher(TokenPredicate predicate)
		: pred(predicate)
		{}

		auto
		test(TokenStreamP stream, size_t offset) const -> ssize_t override
		{
			return pred(stream, offset);
		}
	};

	auto matcher        = std::make_shared<TokenPredicateMatcher>(predicate);
	NamedMatchers[name] = matcher;
	return matcher;
}

auto
TokenTest::byName(std::string name) -> TokenMatcherP
{
	if (!NamedMatchers.contains(name)) {
		throw std::runtime_error("Dereferenced unregistered TokenMatcher " + name);
	}
	return NamedMatchers[name];
}

Sequence::Sequence(std::initializer_list<TokenTestElement> elements)
: elements{std::move(elements)}
{}

auto
Sequence::test(TokenStreamP stream, size_t offset) const -> ssize_t
{
	ssize_t count{0};
	for (auto const& it : elements) {
		auto v = testAt(stream, offset + count, it);
		if (v == TEST_NO_HIT) {
			return TEST_NO_HIT;
		}
		count += v;
	}
	return count;
}

Optional::Optional(TokenTestElement element)
: wrapped{std::move(element)}
{}

auto
Optional::test(TokenStreamP stream, size_t offset) const -> ssize_t
{
	auto v = testAt(stream, offset, wrapped);
	return v == TEST_NO_HIT ? 0 : v;
}

Choice::Choice(std::initializer_list<TokenTestElement> elements)
: elements{elements}
{}

auto
Choice::test(TokenStreamP stream, size_t offset) const -> ssize_t
{
	for (auto const& it : elements) {
		auto v = testAt(stream, offset, it);
		if (v > TEST_NO_HIT) {
			return v;
		}
	}
	return TEST_NO_HIT;
}

Repetition::Repetition(size_t reps, TokenTestElement element)
: reps{reps}, wrapped{element}
{}

auto
Repetition::test(TokenStreamP stream, size_t offset) const -> ssize_t
{
	size_t hits{0};
	size_t length{0};
	while (true) {
		auto v = testAt(stream, offset + length, wrapped);
		if (v == TEST_NO_HIT) {
			break;
		}
		length += v;
		hits   += 1;
	}
	return hits < reps ? TEST_NO_HIT : length;
}
