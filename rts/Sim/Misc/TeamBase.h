/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAM_BASE_H
#define TEAM_BASE_H

#include <algorithm>
#include <string>

#include "System/float3.h"
#include "System/UnorderedMap.hpp"
#include "System/creg/creg_cond.h"


class TeamBase
{
	CR_DECLARE(TeamBase)

public:
	typedef spring::unordered_map<std::string, std::string> customOpts;

	TeamBase();
	virtual ~TeamBase() {}
	virtual void UpdateControllerName() {}

	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const { return customValues; }

	const char* GetSideName() const { return sideName; }
	const char* GetControllerName(bool update = true) const {
		if (update)
			const_cast<TeamBase*>(this)->UpdateControllerName();
		return controllerName;
	}

	void SetStartPos(const float3& pos) { startPos = pos; }
	const float3& GetStartPos() const { return startPos; }

	bool HasValidStartPos() const {
		// if a player never chose (net-sent) a position
		if (startPos == ZeroVector)
			return false;
		// start-positions that are sent across the net
		// will always be clamped to a team's start-box
		// (and hence the map) when clients receive them
		// so this should be redundant
		if (!startPos.IsInBounds())
			return false;

		return true;
	}

	bool HasLeader() const { return (leader != -1); }
	void SetLeader(int leadPlayer) { leader = leadPlayer; }
	int GetLeader() const { return leader; }


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
	void SetAdvantage(float advantage) { SetIncomeMultiplier(std::max(0.0f, advantage) + 1.0f); }

	void SetIncomeMultiplier(float incomeMult) { incomeMultiplier = std::max(0.0f, incomeMult); }
	float GetIncomeMultiplier() const { return incomeMultiplier; }

	void SetDefaultColor(int teamNum) {
		for (size_t colorIdx = 0; colorIdx < 3; ++colorIdx) {
			color[colorIdx] = teamDefaultColor[teamNum % NUM_DEFAULT_TEAM_COLORS][colorIdx];
		}

		color[3] = 255;
	}

public:
	/**
	 * Player ID of the player in charge of this team.
	 * The player either controls this team directly,
	 * or an AI running on his computer does so.
	 */
	int leader = -1;

	int teamStartNum = -1;
	int teamAllyteam = -1;

	/**
	 * The team-color in RGB, with values in [0, 255].
	 * The fourth channel (alpha) has to be 255, always.
	 */
	uint8_t color[4];
	uint8_t origColor[4];

	static constexpr int NUM_DEFAULT_TEAM_COLORS = 10;
	static uint8_t teamDefaultColor[NUM_DEFAULT_TEAM_COLORS][4];

protected:
	/**
	 * All the teams resource income is multiplied by this factor.
	 * The default value is 1.0f, the valid range is [0.0, FLOAT_MAX].
	 *
	 * @see #SetAdvantage()
	 */
	float incomeMultiplier = 1.0f;

	/**
	 * Side/Factions name, eg. "ARM" or "CORE".
	 */
	char sideName[32];
	char controllerName[256];

	float3 startPos;

private:
	customOpts customValues;
};

#endif // TEAM_BASE_H
