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

	/**
	 * Player ID of the player in charge of this team.
	 * The player either controlls this team directly,
	 * or an AI running on his computer does so.
	 */
	int leader;
	unsigned char color[4];
	float handicap;
	/**
	 * Side/Factions name, eg. "ARM" or "CORE".
	 */
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

#endif // TEAM_BASE_H
