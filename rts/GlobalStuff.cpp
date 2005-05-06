#include "StdAfx.h"
// GlobalStuff.cpp: implementation of the CGlobalStuff class.
//
//////////////////////////////////////////////////////////////////////


#include "ProjectileHandler.h"
#include "GameHelper.h"
#include "SyncTracer.h"
#include "Team.h"
#include <windows.h>
#include "Player.h"
#include "TAPalette.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGlobalSyncedStuff* gs;
CGlobalUnsyncedStuff* gu;

CGlobalSyncedStuff::CGlobalSyncedStuff()
{
	hmapx=256;
	hmapy=256;
	randSeed=246;
	frameNum=0;
	speedFactor=1;
	userSpeedFactor=1;
	paused=false;
	cheatEnabled=false;
	tempNum=2;
	
	sunVector=float3(0,0,1);
	sunVector4[0]=0;
	sunVector4[1]=0;
	sunVector4[2]=1;
	sunVector4[3]=0;

	for(int a=0;a<MAX_TEAMS;a++){
		teams[a]=new CTeam();
		teams[a]->teamNum=a;
		team2allyteam[a]=a;
		teams[a]->colorNum=a;
	}
	for(int a=0;a<MAX_PLAYERS;a++){
		players[a]=new CPlayer();
		players[a]->team=(a%2);
	}

	for(int a=0;a<MAX_TEAMS;++a){
		for(int b=0;b<MAX_TEAMS;++b){
			allies[a][b]=false;
		}
		allies[a][a]=true;
	}

	activeTeams=2;
	activeAllyTeams=2;
	teams[1]->active=true;
	teams[2]->active=true;

	sunVector=float3(0,1,2).Normalize();

	gravity = -0.1;
}

CGlobalSyncedStuff::~CGlobalSyncedStuff()
{
	for(int a=0;a<MAX_TEAMS;a++)
		delete teams[a];
	for(int a=0;a<MAX_PLAYERS;a++)
		delete players[a];
}

CGlobalUnsyncedStuff::CGlobalUnsyncedStuff()
{
	LARGE_INTEGER randnum;
	QueryPerformanceCounter(&randnum);
	usRandSeed=randnum.LowPart;
	modGameTime=0;
	gameTime=0;
	lastFrameTime=0;
	screenx=100;
	screeny=100;
	myPlayerNum=0;
	myTeam=1;
	myAllyTeam=1;
	drawdebug=false;
	viewRange=MAX_VIEW_RANGE;
	spectating=false;
	
	timeOffset=0;
#ifdef DIRECT_CONTROL_ALLOWED
	directControl=0;
#endif
}

CGlobalUnsyncedStuff::~CGlobalUnsyncedStuff()
{
}

int CGlobalSyncedStuff::randInt()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return randSeed & 0x7FFF;
}

float CGlobalSyncedStuff::randFloat()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return float(randSeed & 0x7FFF)/RANDINT_MAX;
}

float3 CGlobalSyncedStuff::randVector()
{
	float3 ret;
	do{
		ret.x=randFloat()*2-1;
		ret.y=randFloat()*2-1;
		ret.z=randFloat()*2-1;
	} while(ret.SqLength()>1);

	return ret;
}

int CGlobalUnsyncedStuff::usRandInt()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return usRandSeed & 0x7FFF;
}

float CGlobalUnsyncedStuff::usRandFloat()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return float(usRandSeed & 0x7FFF)/RANDINT_MAX;
}

float3 CGlobalUnsyncedStuff::usRandVector()
{
	float3 ret;
	do{
		ret.x=usRandFloat()*2-1;
		ret.y=usRandFloat()*2-1;
		ret.z=usRandFloat()*2-1;
	} while(ret.SqLength()>1);

	return ret;
}

