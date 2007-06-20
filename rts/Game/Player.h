#ifndef PLAYER_H
#define PLAYER_H
// Player.h: interface for the CPlayer class.
//
//////////////////////////////////////////////////////////////////////

class CCraft;
#include <string>

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

class CPlayer  
{
public:
	CR_DECLARE(CPlayer);
	CR_DECLARE_SUB(Statistics);
	CPlayer();
	virtual ~CPlayer();

	bool active;
	std::string playerName;
	std::string countryCode;

	bool spectator;
	int team;
	bool readyToStart;

	float cpuUsage;
	int ping;

	struct Statistics{
		CR_DECLARE_STRUCT(Statistics);
		int mousePixels;			//how many pixels the mouse has traversed in total
		int mouseClicks;
		int keyPresses;

		int numCommands;
		int unitCommands;			//total amount of units affected by commands (divide by numCommands for average units/command)
	};

	Statistics* currentStats;

#ifdef DIRECT_CONTROL_ALLOWED
	DirectControlStruct myControl;

	CUnit* playerControlledUnit;
	
	void StopControllingUnit();
#endif

};

#endif /* PLAYER_H */
