/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ALLY_TEAM_H
#define ALLY_TEAM_H

#include <string>
#include <vector>

#include "System/creg/creg_cond.h"
#include "System/UnorderedMap.hpp"


class AllyTeam
{
	CR_DECLARE_STRUCT(AllyTeam)

public:
	AllyTeam() { customValues.reserve(8); }

	typedef spring::unordered_map<std::string, std::string> customOpts;

	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const { return customValues; }

	float startRectTop    = 0.0f;
	float startRectBottom = 1.0f;
	float startRectLeft   = 0.0f;
	float startRectRight  = 1.0f;

	std::vector<bool> allies;

private:
	customOpts customValues;
};

#endif
