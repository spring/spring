#ifndef GLOBALSTUFF_H
#define GLOBALSTUFF_H
// GlobalStuff1.h: interface for the CGlobalStuff class.
//
//////////////////////////////////////////////////////////////////////

#define PI 3.141592654f
#define MAX_WORLD_SIZE 1000000;
#define SQUARE_SIZE 8
#define GAME_SPEED 30
#define RANDINT_MAX 0x7fff
#define MAX_VIEW_RANGE 8000
#define MAX_TEAMS 17     // team index 16 is the GAIA team
#define MAX_PLAYERS 32

#define NEAR_PLANE 2.8f

void SendChat(char* c);

#include <list>
#include "float3.h"

struct int2 {
	int2(){};
	int2(int x,int y):x(x),y(y){};
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
	int pwr2mapx;					//sizes rounded up to next pwr2
	int pwr2mapy;
	int tempNum;					//used to get a temporary unique number (increase after use)
	bool drawdebug;
	bool cheatEnabled;
	float viewRange;

	int gameMode;					//0=cmd cont,1=cmd ends
	float gravity;				//note that this is a negative number and in units/frame^2 not a positive number in units/second^2 as in the mapfile

	CPlayer* players[MAX_PLAYERS];
	int activeTeams;							//number of active teams (dont change during play)
	int activeAllyTeams;					
	int activePlayers;

	float3 sunVector;											//direction of the sun
	float sunVector4[4];										//for stuff that requires 4 components

	CTeam *Team(int i) { return teams[i]; }
	bool Ally(int a,int b) { return allies[a][b]; }
	int AllyTeam(int team) { return team2allyteam[team]; }
	bool AlliedTeams(int a,int b) { return allies[team2allyteam[a]][team2allyteam[b]]; }
	
	void SetAllyTeam(int team, int allyteam) { team2allyteam[team]=allyteam; }
	void SetAlly(int teamA,int teamB, bool allied) { allies[teamA][teamB]=allied; }

protected:
	bool allies[MAX_TEAMS][MAX_TEAMS];		//allies[a][b]=true means allyteam a is allied with allyteam b not b is allied with a
	int team2allyteam[MAX_TEAMS];			//what ally team a specific team is part of
	CTeam* teams[MAX_TEAMS];
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

#endif /* GLOBALSTUFF_H */
