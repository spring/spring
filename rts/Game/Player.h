#ifndef PLAYER_H
#define PLAYER_H
// Player.h: interface for the CPlayer class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <set>
#include "Platform/byteorder.h"
#include "float3.h"

#include "PlayerBase.h"

#ifdef DIRECT_CONTROL_ALLOWED
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
#endif


class CPlayer : public PlayerBase
{
public:
	CR_DECLARE(CPlayer);
	CR_DECLARE_SUB(Statistics);
	CPlayer();
	~CPlayer();

	std::set<int> controlledTeams;
	bool CanControlTeam(int teamID) const {
		return (controlledTeams.find(teamID) != controlledTeams.end());
	}
	void SetControlledTeams();
	static void UpdateControlledTeams(); // SetControlledTeams() for all players

	void StartSpectating();

	void operator=(const PlayerBase& base) { PlayerBase::operator=(base); };

	bool active;

	int playerNum;
	bool readyToStart;

	float cpuUsage;
	int ping;

	struct Statistics {
		CR_DECLARE_STRUCT(Statistics);
		/// how many pixels the mouse has traversed in total
		int mousePixels;
		int mouseClicks;
		int keyPresses;

		int numCommands;
		/// total amount of units affected by commands
		/// (divide by numCommands for average units/command)
		int unitCommands;

		/// Change structure from host endian to little endian or vice versa.
		void swab() {
			mousePixels = swabdword(mousePixels);
			mouseClicks = swabdword(mouseClicks);
			keyPresses = swabdword(keyPresses);
			numCommands = swabdword(numCommands);
			unitCommands = swabdword(unitCommands);
		}
	};

	Statistics* currentStats;

#ifdef DIRECT_CONTROL_ALLOWED
	DirectControlStruct myControl;

	CUnit* playerControlledUnit;

	void StopControllingUnit();
#endif

};

#endif /* PLAYER_H */
