#pragma once

#include <cstring>
#include <memory>
#include <string>

auto
toLowerCase(std::string value) -> std::string;

auto
makeUUID4() -> std::string;

auto
ltrim(std::string& v, std::string chars = " ") -> std::string&;
auto
rtrim(std::string& v, std::string chars = " ") -> std::string&;
auto
trim(std::string& v, std::string chars = " ") -> std::string&;
auto
ltrim(std::string&& v, std::string chars = " ") -> std::string;
auto
rtrim(std::string&& v, std::string chars = " ") -> std::string;
auto
trim(std::string&& v, std::string chars = " ") -> std::string;

auto
exec(std::initializer_list<std::string> cmd) -> std::pair<int, std::string>;

template<typename T>
struct is_shared_ptr : std::false_type
{
};

template<typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
{
};

template<class Base, class T, std::enable_if_t<!is_shared_ptr<T>::value, bool> = true>
constexpr auto
instanceOf(T const& t) -> bool
{
	return dynamic_cast<Base>(&t) != nullptr;
}

template<class Base, class T>
constexpr auto
instanceOf(std::shared_ptr<T> const& t) -> bool
{
	return std::dynamic_pointer_cast<Base>(t) != nullptr;
}

// std::string == string literal is still missbehaving occasionally?

inline constexpr auto
StrEqual(std::string const& a, const char* b) -> bool
{
	return std::strcmp(a.c_str(), b) == 0;
}

inline constexpr auto
StrEqual(const char* a, const char* b) -> bool
{
	return std::strcmp(a, b) == 0;
}

inline constexpr auto
StrEqual(const char* a, std::string const& b) -> bool
{
	return std::strcmp(a, b.c_str()) == 0;
}

inline constexpr auto
StrEqual(std::string const& a, std::string const& b) -> bool
{
	return std::strcmp(a.c_str(), b.c_str()) == 0;
}
