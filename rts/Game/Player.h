/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLAYER_H
#define PLAYER_H

#include "PlayerBase.h"
#include "PlayerStatistics.h"
#include "FPSUnitController.h"
#include "System/creg/creg_cond.h"

#include <string>
#include <set>

class CPlayer;
class CUnit;

/// @see CPlayer::ping
#define PATHING_FLAG 0xFFFFFFFF


class CPlayer : public PlayerBase
{
public:
	CR_DECLARE(CPlayer);

	CPlayer();

	bool CanControlTeam(int teamID) const {
		return (controlledTeams.find(teamID) != controlledTeams.end());
	}
	void SetControlledTeams();
	/// SetControlledTeams() for all players
	static void UpdateControlledTeams();

	void StartSpectating();
	void JoinTeam(int newTeam);
	void GameFrame(int frameNum);

	CPlayer& operator=(const PlayerBase& base) { PlayerBase::operator=(base); return *this; }

	void StartControllingUnit();
	void StopControllingUnit();

public:
	bool active;

	int playerNum;

	/**
	 * Contains either the current ping of the player to the game host,
	 * or the value of the pathign flag.
	 * @see PATHING_FLAG
	 */
	int ping;

	PlayerStatistics currentStats;
	FPSUnitController fpsController;

private:
	std::set<int> controlledTeams;
};

#endif /* PLAYER_H */
