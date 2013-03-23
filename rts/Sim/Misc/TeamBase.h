/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAM_BASE_H
#define TEAM_BASE_H

#include <string>
#include <map>

#include "System/float3.h"
#include "System/creg/creg_cond.h"


class TeamBase
{
	CR_DECLARE(TeamBase);

public:
	typedef std::map<std::string, std::string> customOpts;

	TeamBase();
	virtual ~TeamBase();

	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const {
		return customValues;
	}

	/**
	 * Sets the (dis-)advantage.
	 * The default is 0.0 -> no advantage, no disadvantage.
	 * Common values are [-1.0, 1.0].
	 * Valid values are [-1.0, FLOAT_MAX].
	 * Advantage is a meta value. It can be used
	 * to set multiple (dis-)advantage values simultaneously.
	 * As of now, the incomeMultiplier is the only means of giving an advantage.
	 * Possible extensions: buildTimeMultiplier, losMultiplier, ...
	 *
	 * Note: former handicap/bonus
	 * In lobbies, you will often be able to define this through
	 * a value called handicap or bonus in %.
	 *
	 * @see incomeMultiplier
	 */
	void SetAdvantage(float advantage);

	/// @see incomeMultiplier
	void SetIncomeMultiplier(float incomeMultiplier);
	/// @see incomeMultiplier
	float GetIncomeMultiplier() const { return incomeMultiplier; }


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
protected:
	/**
	 * All the teams resource income is multiplied by this factor.
	 * The default value is 1.0f, the valid range is [0.0, FLOAT_MAX].
	 *
	 * @see #SetAdvantage()
	 */
	float incomeMultiplier;
public:
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
