/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <set>

#include "PlayerBase.h"
#include "PlayerStatistics.h"
#include "FPSUnitController.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"

class CPlayer;
class CUnit;

class CPlayer : public PlayerBase
{
public:
	CR_DECLARE(CPlayer);
	CPlayer();
	~CPlayer() {}

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

	bool active;

	int playerNum;

	int ping;

	typedef PlayerStatistics Statistics;

	Statistics currentStats;
	FPSUnitController fpsController;

private:
	std::set<int> controlledTeams;
};

#endif /* PLAYER_H */
