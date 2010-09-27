/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <set>

#include "PlayerBase.h"
#include "PlayerStatistics.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"

class CPlayer;
class CUnit;

struct DirectControlStruct {

	DirectControlStruct();

	bool forward;
	bool back;
	bool left;
	bool right;
	bool mouse1;
	bool mouse2;

	float3 viewDir;
	float3 targetPos;
	float targetDist;
	CUnit* target;
	CPlayer* myController;
};

struct DirectControlClientState {
	DirectControlClientState() {
		oldPitch   = 0;
		oldHeading = 0;
		oldState   = 255;
		oldDCpos   = ZeroVector;

		playerControlledUnit = NULL;
	}

	void SendStateUpdate(bool*);

	CUnit* playerControlledUnit; //! synced
	short oldHeading, oldPitch;  //! unsynced
	unsigned char oldState;      //! unsynced
	float3 oldDCpos;             //! unsynced

	// TODO: relocate the CUnit* from GlobalUnsynced
	// to here as well so everything is in one place
};


class CPlayer : public PlayerBase
{
public:
	CR_DECLARE(CPlayer);
	CPlayer();
	~CPlayer();

	bool CanControlTeam(int teamID) const {
		return (controlledTeams.find(teamID) != controlledTeams.end());
	}
	void SetControlledTeams();
	/// SetControlledTeams() for all players
	static void UpdateControlledTeams();

	void StartSpectating();
	void GameFrame(int frameNum);

	CPlayer& operator=(const PlayerBase& base) { PlayerBase::operator=(base); return *this; }

	void StopControllingUnit();

	bool active;

	int playerNum;

	int ping;

	typedef PlayerStatistics Statistics;

	Statistics currentStats;

	DirectControlStruct myControl;
	DirectControlClientState dccs;

private:
	std::set<int> controlledTeams;
};

#endif /* PLAYER_H */
