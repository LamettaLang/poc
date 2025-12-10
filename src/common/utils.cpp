#include "utils.hpp"

#include <cstdio>
#include <memory>
#include <random>
#include <sstream>

auto
toLowerCase(std::string value) -> std::string
{
	for (char& c : value) {
		c = ('A' <= c && c <= 'Z') ? tolower(c) : c;
	}
	return value;
}

auto
makeUUID4() -> std::string
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<unsigned> dis32(0, 0xFFFFFFFF);
	static std::uniform_int_distribution<unsigned> dis16(0, 0xFFFF);
	std::stringstream buffer;
	buffer << std::hex << dis32(gen) << "-" << dis16(gen);
	buffer << "-" << ((dis16(gen) & 0x0FFF) | 0x4000);
	buffer << "-" << ((dis16(gen) & 0x3FFF) | 0x8000);
	buffer << "-" << dis16(gen) << dis32(gen);
	return buffer.str();
}

auto
ltrim(std::string& v, std::string chars) -> std::string&
{
	v.erase(0, v.find_first_not_of(chars));
	return v;
}

auto
rtrim(std::string& v, std::string chars) -> std::string&
{
	v.erase(v.find_last_not_of(chars) + 1);
	return v;
}

auto
trim(std::string& v, std::string chars) -> std::string&
{
	return ltrim(rtrim(v, chars), chars);
}

auto
ltrim(std::string&& v, std::string chars) -> std::string
{
	v.erase(0, v.find_first_not_of(chars));
	return v;
}

auto
rtrim(std::string&& v, std::string chars) -> std::string
{
	v.erase(v.find_last_not_of(chars) + 1);
	return v;
}

auto
trim(std::string&& v, std::string chars) -> std::string
{
	return ltrim(rtrim(v, chars), chars);
}

auto
exec(std::initializer_list<std::string> command) -> std::pair<int, std::string>
{
	auto args = std::vector<std::string>{command};
	std::vector<char> escaped{};
	escaped.reserve(128);
	for (int i = 0; i < args.size(); i++) {
		auto arg = args[i];
		escaped.emplace_back(' ');
		for (char c : arg) {
			// on windows ^ should be the escape character, idk idc
			if (!std::isalnum(c)) {
				escaped.emplace_back('\\');
			}
			escaped.emplace_back(c);
		}
	}
	escaped.emplace_back('\0');

	char buffer[1024];
	std::string result = "";
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(escaped.data(), "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
		result += buffer;
	}
	return {0, result};
}
