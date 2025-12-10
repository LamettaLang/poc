#pragma once

// purpose:
// argsparse is just a custom thin CLAP layer to make arg validation a tad more
// convenient

#include <exception>
#include <expected>
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

enum struct ArgType
{
	NONE,       // store true, no value
	STRING,
	NUMBER,     // any number
	INTEGER,    // any integer
	FILE,       // existing file
	DIRECTORY,  // existing directory
	PATH,       // parsable file system path
	CUSTOM,     // supply a function to process
};
enum struct ArgKind
{
	FLAG,
	POSITIONAL,
	OPTIONAL,  // positional, has to be after positionals
};

struct ArgDef
{
	std::vector<std::string> aliases{};
	ArgType type{ArgType::NONE};
	ArgKind kind{ArgKind::POSITIONAL};
	std::function<bool(std::string const&)> customValidator{};

	ArgDef() = default;
	/** short flags are registered with a single letter, long flags with a single dash, positionals with long names.
	 * only short flags are case sensitive. only flags can have multiple aliases. */
	ArgDef(std::string alias, ArgType, bool optional = true, std::function<bool(std::string const&)> customValidator = {});
	/** short flags are registered with a single letter, long flags with a single dash, positionals with long names.
	 * only short flags are case sensitive. only flags can have multiple aliases. */
	ArgDef(std::vector<std::string> aliases, ArgType, bool optional = true, std::function<bool(std::string const&)> customValidator = {});
	/** short flags are registered with a single letter, long flags with a single dash, positionals with long names.
	 * only short flags are case sensitive. only flags can have multiple aliases. */
	ArgDef(std::initializer_list<const char*> aliases, ArgType, bool optional = true, std::function<bool(std::string const&)> customValidator = {});

	auto
	operator==(ArgDef const&) const noexcept -> bool;
};

template<>
struct std::hash<ArgDef>
{
	auto
	operator()(ArgDef const& s) const noexcept -> size_t;
};

struct Arguments
{
	std::string cmdname{};
	std::unordered_map<ArgDef, size_t> options{};
	std::unordered_map<ArgDef, std::string> values{};
	std::vector<ArgDef> allDefs{};

	Arguments(int argc, char** argv, std::initializer_list<ArgDef> defs);
	static auto
	parse(int argc, char** argv, std::initializer_list<ArgDef> defs) -> std::expected<Arguments, std::string>;

	auto
	has(ArgDef const& def) const -> size_t;
	auto
	get(ArgDef const& def) const -> std::string;
	auto
	missing() const -> std::optional<ArgDef>;

 private:
	void
	pushArg(ArgDef const& def, std::string const& value);
};
