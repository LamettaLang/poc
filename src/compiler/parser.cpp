#include "parser.hpp"

#include <deque>
#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/utils.hpp"
#include "intermediate.hpp"
#include "tokenmatching.hpp"
#include "tokenstream.hpp"

void
InternalRepresentation::dumps(std::ostream& ostream) const
{
	for (auto const& statement : statements) {
		statement->dumps(ostream);
		std::println();
	}
}

std::set<std::string> scannedFiles{};

void
processAttribute(Token const& token, std::map<std::string, std::string>& attribs)
{
	if (token.type != TokenType::TAttribute) {
		return;
	}

	auto string = token.token;
	auto sep    = string.find('(');
	if (sep == std::string::npos) {
		auto key     = trim(string.substr(1));
		attribs[key] = "";
	} else {
		auto key     = trim(string.substr(1, sep - 1));
		auto value   = trim(string.substr(sep + 1, string.length() - sep - 2));
		attribs[key] = value;
	}
}

auto
processInclude(TokenStreamP currentStream, Token sourceToken, std::map<std::string, std::string> attribts) -> InternalRepresentation
{
	// let's see if the dependency is a fileish
	auto dependency     = sourceToken.token;
	auto looksLikeAFile = sourceToken.type == TokenType::TLiteralString && !dependency.contains(':') && std::filesystem::is_regular_file(dependency);

	if (!looksLikeAFile) {
		// TODO call resolver;
	}

	// include a file
	std::error_code error{};
	auto path = std::filesystem::canonical(dependency, error);
	if (error) {
		auto where = currentStream->getPosition();
		throw file_error(std::format("Could not include \"{}\": {}", dependency, error.message()), where);
	}

	if (scannedFiles.contains(path)) {
		// already included, return no new information
		return {};
	}

	scannedFiles.emplace(path);
	return parse(path);
}

/// ===== grammar in terms of token matching =====
// these do not require exhaustive search; often an indicator is enough!

auto pNOP        = TokenTest::token(TokenType::TTerminal, ";");
auto pAttributes = TokenTest::repeat(
		"attributes", 0, TokenTest::sequence({TokenTest::type(TokenType::TAttribute), TokenTest::optional(TokenTest::type(TokenType::TTerminal))})
);
auto pInclude = TokenTest::sequence(
		{Keywords::INCLUDE,
     TokenTest::choice({TokenType::TLiteralString, TokenType::TIdentifier}),
     TokenTest::optional({TokenTest::sequence({Keywords::AS, TokenType::TIdentifier})}),
     TokenTest::repeat(0, TokenTest::type(TokenType::TAttribute)),
     TokenType::TTerminal}
);
auto pProject                = TokenTest::sequence({"attributes", pNOP});
auto pExpression             = TokenTest::named("expression", [](TokenStreamP stream, size_t offset) -> ssize_t { return TEST_NO_HIT; });
auto pStatementBodySeparator = TokenTest::choice(
		{TokenTest::operand(":"),
     // require a block opening if colon is not present, but dont consume it, as it's part of a block statement
     [](TokenStreamP stream, size_t offset) -> ssize_t { return stream->peek(offset)->token == "{" ? 0 : TEST_NO_HIT; }}
);
auto pBuiltinType       = TokenTest::choice({
    Keywords::BOOL,    Keywords::U8,   Keywords::U16,   Keywords::U32,  Keywords::U64,    Keywords::U128,    Keywords::UINT,
    Keywords::S8,      Keywords::S16,  Keywords::S32,   Keywords::S64,  Keywords::S128,   Keywords::SINT,    Keywords::F32,
    Keywords::F64,     Keywords::F128, Keywords::FLOAT, Keywords::C32,  Keywords::C64,    Keywords::C128,    Keywords::COMPLEX,
    Keywords::ADDRESS, Keywords::STR,  Keywords::AUTO,  Keywords::ANY,  Keywords::HANDLE, Keywords::VARIANT, Keywords::ENUM,
    Keywords::SET,     Keywords::VEC,  Keywords::LIST,  Keywords::DICT, Keywords::TUPLE,  Keywords::STRUCT,  Keywords::METHODMAP,
});
TokenPredicate pNewLine = [](TokenStreamP stream, size_t offset) {
	auto next = stream->peek(offset);
	return next.has_value() && next->type == TokenType::TTerminal && next->token == "\n" ? 1 : TEST_NO_HIT;
};

// ===== process files and statements =====

auto
parseType(TokenStreamP stream) -> LType
{
	if (TokenTest::type(TokenType::TIdentifier)(stream, 0) == 1) {
		return LatentType{stream->pop()};
	} else if (TokenTest::choice({Keywords::BOOL,    Keywords::U8,   Keywords::U16,   Keywords::U32, Keywords::U64,   Keywords::U128, Keywords::UINT,
	                              Keywords::S8,      Keywords::S16,  Keywords::S32,   Keywords::S64, Keywords::S128,  Keywords::SINT, Keywords::F32,
	                              Keywords::F64,     Keywords::F128, Keywords::FLOAT, Keywords::C32, Keywords::C64,   Keywords::C128, Keywords::COMPLEX,
	                              Keywords::ADDRESS, Keywords::STR,  Keywords::AUTO,  Keywords::ANY, Keywords::HANDLE})
	               ->test(stream, 0) == 1) {
		return DiscreteType{stream->pop()};
	} else {
		// TODO complex types
	}
}

auto
parseDef(TokenStreamP stream) -> StatementP
{}

auto
parseFun(TokenStreamP stream) -> StatementP
{}

auto
parseDecl(TokenStreamP stream) -> StatementP
{
	LType type = parseType(stream);
}

/**
 * @return true if root changed
 */
auto
insertExprTree(ExpressionP& root, ExpressionP& head, ExpressionP insert) -> void
{
	if (!head) {
		head = insert;
		root = insert;
		return;
	}
	std::cout << "== insert ==========\n[DBG] root: ";
	root->dumps(std::cout);
	std::cout << "\n[DBG] head: ";
	head->dumps(std::cout);
	std::cout << "\n[DBG] insert: ";
	insert->dumps(std::cout);
	std::cout << "\n";

	auto insertNodeSetChild = [](ExpressionP& target, ExpressionP& insert, ExpressionP& child) {
		ExpressionP prevChild = child;   // make a copy so we can edit after
		insert->parent        = target;
		child                 = insert;  // we change it here
		if (!prevChild) {
			return;
		}
		prevChild->parent = insert;
		if (auto insertUnary = std::dynamic_pointer_cast<UnaryOp>(insert)) {
			if (!insertUnary->child) {
				insertUnary->child = std::move(prevChild);
			} else {
				throw file_error("Internal State Error: Can not insert node into tree, inserted unary already filled", insert->pos);
			}
		} else if (auto insertBinary = std::dynamic_pointer_cast<BinaryOp>(insert)) {
			if (!insertBinary->left) {
				insertBinary->left = std::move(prevChild);
			} else if (!insertBinary->right) {
				insertBinary->right = std::move(prevChild);
			} else {
				throw file_error("Internal State Error: Can not insert node into tree, inserted binary already filled", insert->pos);
			}
		}
	};
	auto isNodeFull = [](const auto& node) {
		if (!instanceOf<Operator>(node)) {
			return true;
		} else if (auto unary = std::dynamic_pointer_cast<UnaryOp>(node)) {
			return (bool)unary->child;
		} else if (auto binary = std::dynamic_pointer_cast<BinaryOp>(node)) {
			return (bool)binary->left && (bool)binary->right;
		} else {
			throw std::runtime_error("Unknown subclass of Operator (::insertExprTree::isNodeFull)");
		}
	};

	// values attach l2r on head or head parent if head is value. if that is full or not op, throw
	// op walks head up while insert precedence >= current precedence. if no more parent, new root; attach l2r; if full swap into right

	// anything not an operator, we treat as value
	auto isInsertValue = !instanceOf<Operator>(insert);
	// first we make head the insert point
	if (isInsertValue) {
		// values are just appended to the first node we find
		while (isNodeFull(head)) {
			head = head->parent.lock();
		}
	} else if (auto op = std::dynamic_pointer_cast<Operator>(insert)) {
		// for operators, we search up the tree, until we find a precedence where we can insert
		while (instanceOf<Value>(head) || op->prec >= std::dynamic_pointer_cast<Operator>(head)->prec) {
			auto parent = head->parent.lock();
			if (!parent) {
				// now op becomes new root
				if (auto unary = std::dynamic_pointer_cast<UnaryOp>(insert)) {
					if (unary->child) {
						throw file_error("Internal State Error: Can not prepend node onto tree, child already filled", insert->pos);
					}
					unary->child = head;
					head->parent = insert;
				} else if (auto binary = std::dynamic_pointer_cast<BinaryOp>(insert)) {
					if (!binary->left) {
						binary->left = head;
					} else if (!binary->right) {
						binary->right = head;
					} else {
						throw file_error("Internal State Error: Can not prepend node onto tree, children already filled", insert->pos);
					}
					head->parent = insert;
				} else {
					throw file_error("Internal State Error: Can not prepend node onto tree, value can't be root", insert->pos);
				}
				root = insert;
				head = insert;
				std::cout << "-- new root --------\n[DBG] root: ";
				root->dumps(std::cout);
				std::cout << "\n[DBG] head: ";
				head->dumps(std::cout);
				std::cout << "\n";
				return;
			}
			head = parent;
		}
	} else {
		throw file_error("Internal State Error: Element is not op or value", insert->pos);
	}

	if (auto unary = std::dynamic_pointer_cast<UnaryOp>(head)) {
		if (unary->child && isInsertValue) {
			throw file_error("Internal State Error: Can not swap value into middle of tree", insert->pos);
		}
		insertNodeSetChild(head, insert, unary->child);
	} else if (auto binary = std::dynamic_pointer_cast<BinaryOp>(head)) {
		if (!binary->left) {
			insertNodeSetChild(head, insert, binary->left);
		} else if (!binary->right) {
			insertNodeSetChild(head, insert, binary->right);
		} else {
			if (binary->right && isInsertValue) {
				throw file_error("Internal State Error: Can not swap value into middle of tree (right)", insert->pos);
			}
			insertNodeSetChild(head, insert, binary->right);
		}
	}
	head = insert;
	std::cout << "-- new node --------\n[DBG] root: ";
	root->dumps(std::cout);
	std::cout << "\n[DBG] head: ";
	head->dumps(std::cout);
	std::cout << "\n";
}

void
replaceTreeHead(ExpressionP& root, ExpressionP& head, ExpressionP newHead)
{
	if (!head) {
		throw file_error("Tried to replace empty head", newHead->pos);
		return;
	}
	if (!(head->parent).lock()) {
		root = head = newHead;
		return;
	}
	head->parent.reset();
	if (auto unary = std::dynamic_pointer_cast<UnaryOp>(head)) {
		unary->child    = newHead;
		newHead->parent = unary;
	} else if (auto binary = std::dynamic_pointer_cast<BinaryOp>(head)) {
		if (binary->left == head) {
			binary->left = newHead;
		} else if (binary->right == head) {
			binary->right = newHead;
		} else {
			throw file_error("Internal State Error: Can not swap value into tree (replace head)", newHead->pos);
		}
		newHead->parent = binary;
	}
}

/**
 * @return true on end of statement
 */
bool
skipSpace(TokenStreamP stream, bool expectValue, TokenPredicate terminal, Token const& previousToken)
{
	if (stream->eof() || terminal(stream, 0) != TEST_NO_HIT) {
		// we have a terminal -> try skipping
		if (expectValue) {
			throw file_error("Unextected end of input while parsing expression, value expected", stream->getPosition());
		}
		return true;  // we've seen a terminal
	}
	while ((previousToken.type == TokenType::TOperator && pNewLine(stream, 0) != TEST_NO_HIT) ||
	       (!expectValue && TokenTest::sequence({pNewLine, TokenType::TOperator})->matches(stream))) {
		// drop new line if:
		// - preceeded by operator, expression has to coninue on next line
		// - new line followed by operator
		stream->drop(1);
		if (stream->eof()) {
			throw file_error("Unextected EOF while parsing expression", stream->getPosition());
		} else if (terminal(stream, 0) != TEST_NO_HIT) {
			throw file_error("Unexpected Terminal after operator", stream->getPosition());
		}
	}
	return false;
}

auto
parseExpr(TokenStreamP stream, TokenPredicate terminal = TokenTest::type(TokenType::TTerminal)) -> ExpressionP
{
	// parsing: valueish, operator, valueish, ...
	// valueish: literal | label | call | group
	// call: label, '(', expr, ',', ... ')' -> still parsing, iif parens and comma are ops
	// group: '(' expr ')' -> parens after operator, not label
	ExpressionP root{}, head{};

	bool expectValue{true};
	Token token{};

	while (true) {
		if (skipSpace(stream, expectValue, terminal, token)) {
			break;
		}

		token = stream->pop();
		if (expectValue) {
			if (token.type == TokenType::TParen && token.token == "(") {
				// push group
				auto pCloseParen = [](TokenStreamP stream, size_t offset) {
					auto token = stream->peek(offset);
					return token && token->token == ")";
				};
				insertExprTree(root, head, parseExpr(stream, pCloseParen));
			}
			if (token.type == TokenType::TLiteralDecimal || token.type == TokenType::TLiteralInteger || token.type == TokenType::TLiteralString ||
			    token.type == TokenType::TIdentifier) {
				// push literal value
				insertExprTree(root, head, std::make_shared<Value>(token));
			} else if (token.type == TokenType::TOperator && (token.token == "!" || token.token == "~" || token.token == "#")) {
				// push unary expr
				expectValue = false;  // fix expectation, because we read a unary in place of a value
				insertExprTree(root, head, std::make_shared<UnaryOp>(token));
			} else {
				throw file_error(std::format("Value or unary expected, got {} '{}'", *token.type, token.token), token.at);
			}
		} else {
			if (token.type == TokenType::TParen) {
				if (token.token == "(") {
					// parse function call
					// transform head
					if (!head) {
						std::print("No head");
					} else {
						head->dumps(std::cout);
						std::cout << std::endl;
					}
					auto headValue = std::dynamic_pointer_cast<Value>(head);
					if (headValue == nullptr || headValue->kind != ValueKind::LABEL) {
						throw file_error(std::format("Expected function call, but preceeding token was no label. Missing operator or invalid syntax."), token.at);
					}
					auto functionCall = std::make_shared<FunctionCall>(*headValue);
					replaceTreeHead(root, head, functionCall);
					// parse args
					TokenPredicate pArgumentSeparator = [](TokenStreamP stream, size_t offset) {
						return TokenTest::choice({TokenTest::operand(","), TokenTest::token(TokenType::TParen, ")")})->test(stream, offset);
					};
					auto pokey = stream->peek();
					if (pokey->type == TokenType::TParen && pokey->token == ")") {
						break;
					}
					for (;;) {
						if (stream->eof()) {
							throw file_error("Unexpected end of input while parsing arguments", stream->getPosition());
						}
						functionCall->arguments.emplace_back(parseExpr(stream, pArgumentSeparator));
						token = stream->pop();
						if (token.type == TokenType::TParen && token.token == ")") {
							break;
						}
					}
					expectValue = true;  // adjust expectation, because we just cosumed args as operator, but the restult is a value
				} else if (token.token == "[") {
					// subscript and range syntax
					auto op      = std::make_shared<BinaryOp>(token);
					// subscript
					auto range   = std::make_shared<RangeExpr>(token);
					range->start = parseExpr(stream, [](TokenStreamP stream, size_t offset) {
						auto peek = stream->peek();
						return peek && ((peek->type == TokenType::TOperator && peek->token == ":") || (peek->type == TokenType::TParen && peek->token == "]"))
						           ? 1
						           : TEST_NO_HIT;
					});
					auto delim   = stream->pop();
					if (delim.token == "]") {
						// index
						op->right = range->start;
						goto skipOverRange;
					}
					range->end = parseExpr(stream, [](TokenStreamP stream, size_t offset) {
						auto peek = stream->peek();
						return peek && ((peek->type == TokenType::TOperator && peek->token == ":") ||
						                (peek->type == TokenType::TParen && (peek->token == "]" || peek->token == "[")))
						           ? 1
						           : TEST_NO_HIT;
					});
					delim      = stream->pop();
					if (delim.token == ":") {
						range->step = parseExpr(stream, [](TokenStreamP stream, size_t offset) {
							auto peek = stream->peek();
							return peek && peek->type == TokenType::TParen && (peek->token == "]" || peek->token == "[") ? 1 : TEST_NO_HIT;
						});
						range->step.swap(range->end);  // step is the 2nd element ind the [x:y:z] syntax
						delim = stream->pop();
					}
					range->endInclusive = delim.token == "]";
					op->right           = range;
					range->parent       = op;
				skipOverRange:
					expectValue = true;  // adjust expectation, because we just consumend the paren expression as value
					insertExprTree(root, head, op);
				} else {
					throw file_error(std::format("Invalid operator '{}'", token.token), token.at);
				}
			} else if (token.type == TokenType::TOperator) {
				if (token.token == "!" || token.token == "~" || token.token == "#") {
					throw file_error(std::format("Binary Operator expected, got unary '{}'", token.token), token.at);
				} else {
					// push binary
					insertExprTree(root, head, std::make_shared<BinaryOp>(token));
				}
			} else {
				throw file_error(std::format("Operator expected, got {} '{}'", *token.type, token.token), token.at);
			}
		}
		expectValue = !expectValue;
	}
	return root;
}

auto
parseToplevel(TokenStreamP stream) -> StatementP
{
	while (TokenTest::type(TokenType::TTerminal)(stream, 0) != TEST_NO_HIT) {
		stream->drop(1);
	}
	if (stream->eof()) {
		return {};
	}

	if (TokenTest::keyword("def")(stream, 0) != TEST_NO_HIT) {
		return parseDef(stream);
	} else if (TokenTest::keyword("fun")(stream, 0) != TEST_NO_HIT) {
		return parseFun(stream);
		// } else if (TokenTest::choice({pBuiltinType, TokenType::TIdentifier})->matches(stream)) {
		// 	return parseDecl(stream);  // globals; identifier has to resolve to a typeish
	} else {
		return parseExpr(stream);
	}
}

auto
parse(std::string file) -> InternalRepresentation
{
	TokenStreamP stream = std::make_shared<TokenStream>(file);

	// collect project attributes at start of file
	std::map<std::string, std::string> projectAttributes{};
	if (pProject->matches(stream)) {
		// collect attributes
		while (stream->peek() && stream->peek()->type == TokenType::TAttribute) {
			processAttribute(stream->pop(), projectAttributes);
		}

		// drop semicolon if present
		if (pNOP(stream, 0)) {
			stream->drop(1);
		}
	}

	// collect includes
	while (pInclude->matches(stream)) {
		std::map<std::string, std::string> attribs{};
		stream->drop(1);
		auto name = stream->pop();
		std::string includeNS{};
		if (TokenTest::keyword("as")(stream, 0) != TEST_NO_HIT) {
			stream->drop(1);
			includeNS = stream->pop().token;
		}
		while (stream->peek() && stream->peek()->type == TokenType::TAttribute) {
			processAttribute(stream->pop(), attribs);
		}
		auto included = processInclude(std::move(stream), name, attribs);
	}

	InternalRepresentation ir{};
	while (!stream->eof()) {
		auto statement = parseToplevel(stream);
		if (statement) {
			ir.statements.emplace_back(statement);
		}
	}

	return std::move(ir);
}
