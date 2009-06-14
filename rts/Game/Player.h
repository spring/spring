#ifndef PLAYER_H
#define PLAYER_H
// Player.h: interface for the CPlayer class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <set>

#include "creg/creg_cond.h"

#include "PlayerBase.h"

#include "float3.h"
class CPlayer;
class CUnit;
struct DirectControlStruct{
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
	static void UpdateControlledTeams(); // SetControlledTeams() for all players

	void StartSpectating();
	void GameFrame(int frameNum);

	void operator=(const PlayerBase& base) { PlayerBase::operator=(base); };

	bool active;

	int playerNum;
	bool readyToStart;

	float cpuUsage;
	int ping;

	typedef PlayerStatistics Statistics;

	Statistics currentStats;

	DirectControlStruct myControl;

	CUnit* playerControlledUnit;

	void StopControllingUnit();

private:
	std::set<int> controlledTeams;
};

#endif /* PLAYER_H */
