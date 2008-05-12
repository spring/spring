/**
 * @file GlobalStuff.cpp
 * @brief Globally accessible stuff
 *
 * Contains implementation of synced and
 * unsynced global stuff
 */
#include "StdAfx.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Game/GameHelper.h"
#include "Sync/SyncTracer.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "Rendering/Textures/TAPalette.h"
#include "SDL_types.h"
#include "SDL_timer.h"
#include "mmgr.h"

/**
 * @brief global synced
 *
 * Global instance of CGlobalSyncedStuff
 */
CGlobalSyncedStuff* gs;

/**
 * @brief global unsynced
 *
 * Global instance of CGlobalUnsyncedStuff
 */
CGlobalUnsyncedStuff* gu;

/**
 * @brief randint max
 *
 * Defines the maximum random integer as 0x7fff
 */
const int RANDINT_MAX = 0x7fff;

CR_BIND(CGlobalSyncedStuff,);

CR_REG_METADATA(CGlobalSyncedStuff, (
				CR_MEMBER(randSeed),
				CR_MEMBER(frameNum),
				CR_MEMBER(speedFactor),
				CR_MEMBER(userSpeedFactor),
				CR_MEMBER(paused),
				CR_MEMBER(tempNum),
				CR_MEMBER(godMode),
				CR_MEMBER(cheatEnabled),
				CR_MEMBER(noHelperAIs),
				CR_MEMBER(editDefsEnabled),
				CR_MEMBER(useLuaRules),
				CR_MEMBER(useLuaGaia),
				CR_MEMBER(gaiaTeamID),
				CR_MEMBER(gaiaAllyTeamID),
				CR_MEMBER(gameMode),
				CR_MEMBER(players),
				CR_MEMBER(activeTeams),
				CR_MEMBER(activeAllyTeams),
				CR_MEMBER(activePlayers),
				CR_MEMBER(allies),
				CR_MEMBER(team2allyteam),
				CR_MEMBER(teams),
				CR_RESERVED(64)
				));

CR_BIND(CGlobalUnsyncedStuff,);

CR_REG_METADATA(CGlobalUnsyncedStuff, (
				CR_MEMBER(usRandSeed),
				CR_MEMBER(modGameTime),
				CR_MEMBER(gameTime),
				CR_MEMBER(lastFrameTime),
				CR_MEMBER(myPlayerNum),
				CR_MEMBER(myTeam),
				CR_MEMBER(myAllyTeam),
				CR_MEMBER(spectating),
				CR_MEMBER(spectatingFullView),
				CR_MEMBER(spectatingFullSelect),
				CR_MEMBER(viewRange),
				CR_MEMBER(timeOffset),
				CR_MEMBER(drawFog),
				CR_MEMBER(autoQuit),
				CR_MEMBER(quitTime),
				CR_RESERVED(64)
				));

/**
 * Initializes variables in CGlobalSyncedStuff
 */
CGlobalSyncedStuff::CGlobalSyncedStuff()
{
	hmapx=256;
	hmapy=256;
	randSeed=18655;//li.LowPart;
	frameNum=0;
	speedFactor=1;
	userSpeedFactor=1;
	paused=false;
	godMode=false;
	cheatEnabled=false;
	noHelperAIs=false;
	editDefsEnabled=false;
	tempNum=2;
	gameMode=0;
	useLuaGaia=true;
	gaiaTeamID=-1;
	gaiaAllyTeamID=-1;
	useLuaRules=true;
	
	for(int a=0; a < MAX_TEAMS; ++a){
		teams[a]=SAFE_NEW CTeam();
		teams[a]->teamNum=a;
		team2allyteam[a]=a;
	}
	for(int a=0; a < MAX_PLAYERS; ++a){
		players[a]=SAFE_NEW CPlayer();
		players[a]->team=0;
	}

	for(int a=0; a < MAX_TEAMS; ++a){
		for(int b=0;b<MAX_TEAMS;++b){
			allies[a][b]=false;
		}
		allies[a][a]=true;
	}

	activeTeams=2;
	activeAllyTeams=2;
	activePlayers=MAX_PLAYERS;
}

/**
 * Destroys data inside CGlobalSyncedStuff
 */
CGlobalSyncedStuff::~CGlobalSyncedStuff()
{
	for(int a=0;a<MAX_TEAMS;a++)
		delete teams[a];
	for(int a=0;a<gs->activePlayers;a++)
		delete players[a];
}

/**
 * @return synced random integer
 *
 * returns a synced random integer
 */
int CGlobalSyncedStuff::randInt()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return randSeed & RANDINT_MAX;
}

/**
 * @return synced random float
 *
 * returns a synced random float
 */
float CGlobalSyncedStuff::randFloat()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return float(randSeed & RANDINT_MAX)/RANDINT_MAX;
}

/**
 * @return synced random vector
 *
 * returns a synced random vector
 */
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

int CGlobalSyncedStuff::Player(const std::string& name)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		if (players[i] && players[i]->playerName == name)
			return i;
	}
	return -1;
}

/**
 * Initializes variables in CGlobalUnsyncedStuff
 */
CGlobalUnsyncedStuff::CGlobalUnsyncedStuff()
{
	Uint64 randnum;
	randnum = SDL_GetTicks();
	usRandSeed=randnum&0xffffffff;
	modGameTime=0;
	gameTime=0;
	lastFrameTime=0;
	viewSizeX=100;
	viewSizeY=100;
	pixelX=0.01f;
	pixelY=0.01f;
	aspectRatio=1.0f;
	myPlayerNum=0;
	myTeam=1;
	myAllyTeam=1;
	spectating           = false;
	spectatingFullView   = false;
	spectatingFullSelect = false;
	drawdebug=false;
	active=true;
	viewRange=MAX_VIEW_RANGE;
	timeOffset=0;
	drawFog=true;
	compressTextures=false;
	teamNanospray=false;
	autoQuit=false;
	quitTime=0;
#ifdef DIRECT_CONTROL_ALLOWED
	directControl=0;
#endif
}

/**
 * Destroys variables in CGlobalUnsyncedStuff
 */
CGlobalUnsyncedStuff::~CGlobalUnsyncedStuff()
{
}

/**
 * @return unsynced random integer
 *
 * Returns an unsynced random integer
 */
int CGlobalUnsyncedStuff::usRandInt()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return usRandSeed & RANDINT_MAX;
}

/**
 * @return unsynced random float
 *
 * returns an unsynced random float
 */
float CGlobalUnsyncedStuff::usRandFloat()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return float(usRandSeed & RANDINT_MAX)/RANDINT_MAX;
}

/**
 * @return unsynced random vector
 *
 * returns an unsynced random vector
 */
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




