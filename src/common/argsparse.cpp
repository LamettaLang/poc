#include "argsparse.hpp"

#include <deque>
#include <filesystem>
#include <format>
#include <functional>
#include <initializer_list>
#include <map>
#include <print>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../common/utils.hpp"

ArgDef::ArgDef(std::string alias, ArgType type, bool optional, std::function<bool(std::string const&)> customValidator)
: ArgDef(std::vector<std::string>{alias}, type, optional, customValidator)
{}

ArgDef::ArgDef(std::initializer_list<const char*> aliases, ArgType type, bool optional, std::function<bool(std::string const&)> customValidator)
: ArgDef(std::vector<std::string>{aliases.begin(), aliases.end()}, type, optional, customValidator)
{}

ArgDef::ArgDef(std::vector<std::string> aliases, ArgType type, bool optional, std::function<bool(std::string const&)> customValidator)
{
	auto allFlags  = std::all_of(aliases.cbegin(), aliases.cend(), [](auto const& it) { return it.length() == 1 || it.at(0) == '-'; });
	auto noneFlags = std::none_of(aliases.cbegin(), aliases.cend(), [](auto const& it) { return it.length() == 1 || it.at(0) == '-'; });
	if (allFlags == noneFlags) {
		throw std::logic_error("Mixed spec for positionals and flags");
	} else if (allFlags) {
		this->aliases = aliases;
		this->kind    = ArgKind::FLAG;
	} else if (noneFlags) {
		this->aliases = aliases;
		this->kind    = optional ? ArgKind::OPTIONAL : ArgKind::POSITIONAL;
	}
	this->type = type;
	if (type == ArgType::CUSTOM) {
		this->customValidator = customValidator;
	}
}

auto
ArgDef::operator==(ArgDef const& other) const noexcept -> bool
{
	return this->aliases == other.aliases && this->kind == other.kind && this->type == other.type;
}

auto
std::hash<ArgDef>::operator()(ArgDef const& s) const noexcept -> size_t
{
	size_t hash{0};
	hash ^= hash * 31 + (int)s.kind;
	hash ^= hash * 31 + (int)s.type;
	for (auto const& alias : s.aliases) {
		hash ^= std::hash<std::string>{}(alias);
	}
	return hash;
}

/**
 * tries to read a flag value.
 * short flags should use offset to ensure no other short flags are skipped.
 * long flags will search a key value separator.
 * separators are ':' or '=', if no separator is found, the next arg is used.
 * automatically advances the iterator, so ++iterator always points at the next argument.
 * @param iterator - point at the current flag
 * @param boundary - arugments.cend()
 * @param offset - should be seperator offset withing *iterator for short flags
 * @param separatorOut - write back for index of separator, if found
 * @return the extracted string value for whatever flag we are; and the separator index into *iterator, if split with colon or equals, npos for space
 * @throws std::runtime_error if (*iterator)[offset{>0}]!=0 or iterator can not be advanced without hitting the boundary
 */
auto
popFlagValue(std::vector<std::string>::const_iterator& iterator, std::vector<std::string>::const_iterator const& boundary, size_t offset = 0)
		-> std::pair<std::string, size_t>
{
	std::string const& arg = *iterator;
	size_t separator       = std::string::npos;

	if (offset == 0) {
		auto colon  = arg.find(':');
		auto equals = arg.find('=');
		if (colon == std::string::npos) {
			separator = equals;
		} else if (equals == std::string::npos) {
			separator = colon;
		} else if (colon < equals) {
			separator = colon;
		} else {
			separator = equals;
		}
	} else if (arg[offset] == ':' || arg[offset] == '=') {
		separator = offset;
	} else if (arg[offset]) {
		throw std::runtime_error("Value flags need to be followed by a separator (colon, equals or space)");
	}
	if (separator != std::string::npos) {
		return {arg.substr(separator), separator};
	} else {
		++iterator;
		if (iterator == boundary) {
			throw std::runtime_error("Value flag at the end of the arugment list");
		}
		return {*iterator, std::string::npos};
	}
}

/** validate type of value using def, then push onto internal values */
auto
Arguments::pushArg(ArgDef const& def, std::string const& value) -> void
{
	if (def.kind == ArgKind::FLAG && def.type == ArgType::NONE && !value.empty()) {
		throw std::runtime_error(std::format("Argument {} has no value", def.aliases.at(0)));
	}
	if (this->values.contains(def)) {
		throw std::runtime_error(std::format("Arguemnt {} is already set", def.aliases.at(0)));
	}

	std::function<bool(std::string const&)> validator;
	switch (def.type) {
		case ArgType::NONE:
		case ArgType::STRING:
			validator = [](std::string const& _) { return true; };
			break;
		case ArgType::NUMBER:
			validator = [](std::string const& value) {
				std::istringstream istream(value);
				double dummy;
				istream >> dummy;
				return istream.eof() && !istream.fail();
			};
			break;
		case ArgType::INTEGER:
			validator = [](std::string const& value) {
				std::istringstream istream(value);
				int64_t dummy;
				istream >> dummy;
				return istream.eof() && !istream.fail();
			};
			break;
		case ArgType::FILE:
			validator = [](std::string const& value) { return std::filesystem::is_regular_file(value); };
			break;
		case ArgType::DIRECTORY:
			validator = [](std::string const& value) { return std::filesystem::is_directory(value); };
			break;
		case ArgType::PATH:
			validator = [](std::string const& value) {
				try {
					return !value.empty() && !std::filesystem::weakly_canonical(std::filesystem::path(value)).make_preferred().empty();
				} catch (...) {
					return false;
				}
			};
			break;
		case ArgType::CUSTOM:
			validator = def.customValidator;
			if (!validator) {
				throw std::logic_error("Validator for custom arg type not set up");
			}
			break;
	}
	if (!validator(value)) {
		throw std::runtime_error(std::format("Argument value for {} has incorrect value type", def.aliases.at(0)));
	}
	this->values.emplace(def, value);
}

Arguments::Arguments(int _argc, char** _argv, std::initializer_list<ArgDef> defs)
: allDefs(defs)
{
	std::vector<std::string> args{};
	args.reserve(_argc);
	args.assign(_argv, _argv + _argc);

	// lookup alias -> def
	std::map<std::string, ArgDef> meta{};
	for (const auto& it : defs) {
		for (const auto& alias : it.aliases) {
			if (alias.length() > 1) {
				meta[toLowerCase(alias)] = it;
			} else {
				meta[alias] = it;
			}
		}
	}

	// build stack for ordered arguments
	std::deque<ArgDef> ordered{};
	bool hadOptional{false};
	for (auto const& d : defs) {
		if (d.kind == ArgKind::FLAG) {
			continue;
		} else if (d.kind == ArgKind::OPTIONAL) {
			hadOptional = true;
		} else if (hadOptional) {
			throw std::logic_error("Optional positionals have to follow required ones");
		}
		ordered.push_back(d);
	}

	// GO
	auto it = args.cbegin();
	bool readFlags{true};
	this->cmdname = *it;
	it++;
	for (; it != args.cend(); it++) {
		const auto arg = *it;
		if (readFlags && arg[0] == '-') {
			if (arg == "--") {
				readFlags = false;
				continue;
			}
			auto flag = arg.substr(1);
			auto def  = meta.cend();
			if (flag[0] != '-') {
				char shortflag[3] = " ";
				for (uint i = 0; flag[i]; i++) {
					shortflag[0] = flag[i];
					def          = meta.find(shortflag);
					if (def != meta.cend()) {
						if (def->second.kind != ArgKind::FLAG) {
							throw std::runtime_error(std::format("\"{}\" is not registered as flag!", flag));
						} else if (def->second.type == ArgType::NONE) {
							this->options.try_emplace(def->second, 0).first->second++;
						} else {
							auto [value, _] = popFlagValue(it, args.cend(), i + 2);
							pushArg(def->second, value);
							break;
						}
					} else {
						throw std::runtime_error(std::format("Unknown short flag \"{}\"", flag));
					}
				}
			} else {
				auto [value, sep] = popFlagValue(it, args.cend());
				auto def          = meta.find(toLowerCase(flag.substr(0, sep)));
				if (def != meta.cend()) {
					if (def->second.kind != ArgKind::FLAG) {
						throw std::runtime_error(std::format("\"{}\" is not registered as flag!", flag));
					} else if (def->second.type == ArgType::NONE) {
						this->options.try_emplace(def->second, 0).first->second++;
					} else {
						pushArg(def->second, value);
						break;
					}
				} else {
					throw std::runtime_error(std::format("Unknown short flag \"{}\"", flag));
				}
			}
		} else {
			if (ordered.empty()) {
				throw std::runtime_error("Too many arguments");
			}
			auto next = ordered.at(0);
			ordered.pop_front();
			pushArg(next, arg);
		}
	}
	if (!ordered.empty() && ordered.front().kind != ArgKind::OPTIONAL) {
		throw std::runtime_error(std::format("Missing required argument {}", ordered.front().aliases.at(0)));
	}
}

auto
Arguments::parse(int argc, char** argv, std::initializer_list<ArgDef> defs) -> std::expected<Arguments, std::string>
{
	try {
		return Arguments(argc, argv, defs);
	} catch (std::exception& e) {
		return std::unexpected(e.what());
	}
}

auto
Arguments::has(ArgDef const& def) const -> size_t
{
	if (def.kind == ArgKind::FLAG) {
		auto at = options.find(def);
		return at == options.end() ? 0 : at->second;
	} else {
		auto at = values.find(def);
		return at == values.end() ? 0 : 1;
	}
}

auto
Arguments::get(ArgDef const& def) const -> std::string
{
	if (def.kind == ArgKind::FLAG) {
		auto at = options.find(def);
		return at == options.end() ? std::string() : def.aliases.at(0);
	} else {
		auto at = values.find(def);
		return at == values.end() ? std::string() : values.at(def);
	}
}

auto
Arguments::missing() const -> std::optional<ArgDef>
{
	auto missingFilter = [&](ArgDef const& def) { return def.kind != ArgKind::POSITIONAL || values.contains(def); };
	auto at            = std::find_if(allDefs.cbegin(), allDefs.cend(), missingFilter);
	if (at != allDefs.cend()) {
		return *at;
	} else {
		return {};
	}
}
