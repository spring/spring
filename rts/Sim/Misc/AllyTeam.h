/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ALLY_TEAM_H
#define ALLY_TEAM_H

#include <string>
#include <map>
#include <vector>

class AllyTeam
{
public:
	AllyTeam();

	typedef std::map<std::string, std::string> customOpts;
	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const
	{
		return customValues;
	};

	float startRectTop;
	float startRectBottom;
	float startRectLeft;
	float startRectRight;
	std::vector<bool> allies;

private:
	customOpts customValues;
};

#endif
