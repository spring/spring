/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "GlobalSynced.h"

#include <assert.h>
#include <cstring>

#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/GameSetup.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Util.h"


/**
 * @brief global synced
 *
 * Global instance of CGlobalSynced
 */
CGlobalSynced* gs = NULL;



CR_BIND(CGlobalSynced, );

CR_REG_METADATA(CGlobalSynced, (
	CR_MEMBER(frameNum),
	CR_MEMBER(speedFactor),
	CR_MEMBER(userSpeedFactor),
	CR_MEMBER(paused),
	CR_MEMBER(mapx),
	CR_MEMBER(mapy),
	CR_MEMBER(mapSquares),
	CR_MEMBER(hmapx),
	CR_MEMBER(hmapy),
	CR_MEMBER(pwr2mapx),
	CR_MEMBER(pwr2mapy),
	CR_MEMBER(tempNum),
	CR_MEMBER(godMode),
	CR_MEMBER(globalLOS),
	CR_MEMBER(cheatEnabled),
	CR_MEMBER(noHelperAIs),
	CR_MEMBER(editDefsEnabled),
	CR_MEMBER(useLuaGaia),
	CR_MEMBER(randSeed),
	CR_MEMBER(initRandSeed),
	CR_RESERVED(64)
));


/**
 * Initializes variables in CGlobalSynced
 */
CGlobalSynced::CGlobalSynced()
{
	mapx  = 512;
	mapy  = 512;
	mapSquares = mapx * mapy;
	hmapx = mapx>>1;
	hmapy = mapy>>1;
	pwr2mapx = mapx; //next_power_of_2(mapx);
	pwr2mapy = mapy; //next_power_of_2(mapy);
	randSeed = 18655;
	initRandSeed = randSeed;
	frameNum = 0;
	speedFactor = 1;
	userSpeedFactor = 1;
	paused = false;
	godMode = false;
	globalLOS = false;
	cheatEnabled = false;
	noHelperAIs = false;
	editDefsEnabled = false;
	tempNum = 2;
	useLuaGaia = true;

	teamHandler = new CTeamHandler();
}


CGlobalSynced::~CGlobalSynced()
{
	SafeDelete(teamHandler);
}


void CGlobalSynced::LoadFromSetup(const CGameSetup* setup)
{
	noHelperAIs = !!setup->noHelperAIs;
	useLuaGaia  = setup->useLuaGaia;

	skirmishAIHandler.LoadFromSetup(*setup);
	teamHandler->LoadFromSetup(setup);
}

/**
 * @return synced random integer
 *
 * returns a synced random integer
 */
int CGlobalSynced::randInt()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return (randSeed >> 16) & RANDINT_MAX;
}

/**
 * @return synced random float
 *
 * returns a synced random float
 */
float CGlobalSynced::randFloat()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return float((randSeed >> 16) & RANDINT_MAX)/RANDINT_MAX;
}

/**
 * @return synced random vector
 *
 * returns a synced random vector
 */
float3 CGlobalSynced::randVector()
{
	float3 ret;
	do {
		ret.x = randFloat()*2-1;
		ret.y = randFloat()*2-1;
		ret.z = randFloat()*2-1;
	} while(ret.SqLength()>1);

	return ret;
}
