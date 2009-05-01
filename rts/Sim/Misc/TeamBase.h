#ifndef TEAM_BASE_H
#define TEAM_BASE_H

#include <string>
#include <map>

#include "float3.h"

class TeamBase
{
public:
	TeamBase();

	void SetValue(const std::string& key, const std::string& value);

	int leader;
	unsigned char color[4];
	float handicap;
	std::string side;
	float3 startPos;
	int teamStartNum;
	int teamAllyteam;

private:
	std::map<std::string, std::string> customValues;
};

#endif
