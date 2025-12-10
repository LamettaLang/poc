// purpose:
// arg parsing and triggering various stages of parsing

#include <exception>
#include <iostream>
#include <print>
#include <string>
#include <vector>

#include "common/argsparse.hpp"
#include "common/traced_error.hpp"
#include "parser.hpp"
#include "tokenstream.hpp"

auto argOutFile    = ArgDef{{"-output", "o"}, ArgType::PATH};
auto argSourceFile = ArgDef{"source", ArgType::FILE};
auto argHelp       = ArgDef{{"-help", "?"}, ArgType::NONE};
auto argDepManager = ArgDef{{"-depmgr"}, ArgType::FILE};

auto
doParse(Arguments const& args) -> int
{
	// TokenStreamP stream = std::make_shared<TokenStream>(args.get(argSourceFile));
	// while (stream->peek()) {
	// 	auto token = stream->pop();
	// 	std::print("{}:{:04}: Token {:10} {}\n", token.at.filename, token.at.line, *token.type, token.token);
	// }
	// return 0;
	parse(args.get(argSourceFile)).dumps(std::cout);
	return 0;
}

int
main(int c_argc, char** c_argv)
{
	auto args = Arguments::parse(c_argc, c_argv, {argOutFile, argHelp, argDepManager, argSourceFile});
	if (!args.has_value()) {
		std::print("Argument error: {}\n", args.error());
		return 1;
	}

	if (args->has(argHelp) > 10) {
		std::print("Such arguments, very confusion...\n\n");
	} else if (args->has(argHelp) > 3) {
		std::print("Relax!\n\n");
	}
	if (args->has(argHelp)) {
		std::print("Usage: {} [--output|-o:PATH] [--depmgr:FILE] [--help|-?] source:FILE\n", args->cmdname);
		std::print("    --output  The location for the output binary\n");
		std::print("    -o\n");
		std::print("    --help    You are here!\n");
		std::print("    -?\n");
		std::print("    --depmgr  Binary to use for depdendency management\n");
		std::print("    source    You main source file\n");
		return 0;
	}

	try {
		return doParse(*args);
	} catch (file_error& fe) {
		std::print("{}: \e[91merror:\e[0m {}\n", fe.where().toGCCString(), fe.what());
		std::print("{}\n", fe.stacktrace());
	} catch (stacktraced_error& te) {
		std::print("\e[91merror:\e[0m {}\n", te.what());
		std::print("{}\n", te.stacktrace());
	} catch (std::exception& e) {
		std::print("\e[91merror:\e[0m {}\n", e.what());
	}

	return 1;
}
