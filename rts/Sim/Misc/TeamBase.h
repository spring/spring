/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
	 * The player either controls this team directly,
	 * or an AI running on his computer does so.
	 */
	int leader;
	/**
	 * The team-color in RGB, with values in [0, 255].
	 * The fourth channel (alpha) has to be 255, always.
	 */
	unsigned char color[4];
	float handicap;
	/**
	 * Side/Factions name, eg. "ARM" or "CORE".
	 */
	std::string side;
	float3 startPos;
	int teamStartNum;
	int teamAllyteam;

	static unsigned char teamDefaultColor[10][4];

private:
	customOpts customValues;
};

#endif // TEAM_BASE_H
