#pragma once

#include <string>
#include <vector>

#include "intermediate.hpp"

struct InternalRepresentation
{
	std::vector<StatementP> statements{};

	virtual void
	dumps(std::ostream& ostream) const;
};

auto
parse(std::string file) -> InternalRepresentation;
