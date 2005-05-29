#ifndef GLOBALSTUFF_H
#define GLOBALSTUFF_H
// GlobalStuff1.h: interface for the CGlobalStuff class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLOBALSTUFF1_H__2B3603E2_4EBE_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_GLOBALSTUFF1_H__2B3603E2_4EBE_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000


#define PI 3.141592654f
#define MAX_WORLD_SIZE 1000000;
#define SQUARE_SIZE 8
#define GAME_SPEED 30
#define RANDINT_MAX 0x7fff
#define MAX_VIEW_RANGE 8000
#define MAX_TEAMS 10
#define MAX_PLAYERS 16

void SendChat(char* c);

#include <list>
#include "float3.h"

struct int2 {
	int x;
	int y;
};
struct float2 {
	float x;
	float y;
};

const float3 UpVector(0,1,0);
const float3 ZeroVector(0,0,0);

class CTeam;
class CUnit;
class CPlayer;

class CGlobalSyncedStuff  
{
public:
	CGlobalSyncedStuff();
	virtual ~CGlobalSyncedStuff();

	int randInt();				//synced random stuff (dont use for unsynced stuff)
	float randFloat();
	float3 randVector();
	int randSeed;
	
	int frameNum;
	float speedFactor;				//real gamespeed used by the game
	float userSpeedFactor;		//real gamespeed can be up to this but is lower if a computer cant keep up
	bool paused;
	int mapx;							//number of squares (note that the number verteces is one more)
	int mapy;
	int mapSquares;			//Total number of squares on map.
	int hmapx;						//half the number of squares
	int hmapy;
	int tempNum;					//used to get a temporary unique number (increase after use)
	bool drawdebug;
	bool cheatEnabled;
	float viewRange;

	float gravity;				//note that this is a negative number and in units/frame^2 not a positive number in units/second^2 as in the mapfile

	CTeam* teams[MAX_TEAMS];			//the teams (note that neutrals are team zero and not counted as active)
	CPlayer* players[MAX_PLAYERS];
	int activeTeams;							//number of active teams (dont change during play)
	int activeAllyTeams;					

	float3 sunVector;											//direction of the sun
	float sunVector4[4];										//for stuff that requires 4 components

	bool allies[MAX_TEAMS][MAX_TEAMS];		//allies[a][b]=true means allyteam a is allied with allyteam b not b is allied with b
	int team2allyteam[MAX_TEAMS];			//what ally team a specific team is part of
};

class CGlobalUnsyncedStuff  
{
public:
	CGlobalUnsyncedStuff();
	virtual ~CGlobalUnsyncedStuff();

	int usRandInt();				//unsynced random stuff (dont use for synced stuff)
	float usRandFloat();
	float3 usRandVector();
	int usRandSeed;

	double modGameTime;		//how long the game has been going on modified with speed factor
	double gameTime;			//how long has passed since start of game in real time
	float lastFrameTime;	//how long the last draw cycle took in real time
	int screenx;
	int screeny;
	int myPlayerNum;
	int myTeam;
	int myAllyTeam;
	bool spectating;			//see everything but cant give orders
	bool drawdebug;
	float viewRange;

	float timeOffset;			//time (in number of frames) since last update (for interpolation)

#ifdef DIRECT_CONTROL_ALLOWED
	CUnit* directControl;
#endif

};

extern CGlobalSyncedStuff* gs;
extern CGlobalUnsyncedStuff* gu;

#endif // !defined(AFX_GLOBALSTUFF1_H__2B3603E2_4EBE_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* GLOBALSTUFF_H */
