#pragma once

#include <string>

struct TypeInfo
{
	std::string name;
	std::string type_id;
};

struct ValueTypeInfo : public TypeInfo
{
	bool dynamic{};
};

struct ReferenceTypeInfo : public TypeInfo
{
};
