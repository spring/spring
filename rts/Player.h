#ifndef PLAYER_H
#define PLAYER_H
// Player.h: interface for the CPlayer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PLAYER_H__BB0F0501_9236_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_PLAYER_H__BB0F0501_9236_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

class CCraft;
#include <string>

#ifdef DIRECT_CONTROL_ALLOWED
class CPlayer;
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
	CPlayer();
	virtual ~CPlayer();

	bool active;
	std::string playerName;

	bool spectator;
	int team;
	bool readyToStart;

	float cpuUsage;
	int ping;

	struct Statistics{
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
#endif

};

#endif // !defined(AFX_PLAYER_H__BB0F0501_9236_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* PLAYER_H */
