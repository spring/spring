#ifndef TEAM_BASE_H
#define TEAM_BASE_H

#include <string>
#include <map>

#include "float3.h"

class TeamBase
{
public:
	typedef std::map<std::string, std::string> customOpts;
	TeamBase();

	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const
	{
		return customValues;
	};

	int leader;
	unsigned char color[4];
	float handicap;
	std::string side;
	float3 startPos;
	int teamStartNum;
	int teamAllyteam;

	float startMetal;
	float startEnergy;
	bool readyToStart;

private:
	customOpts customValues;
};

#endif
