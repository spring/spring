/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GlobalSynced.h"

#include <assert.h>
#include <cstring>

#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Util.h"
#include "System/Log/FramePrefixer.h"


/**
 * @brief global synced
 *
 * Global instance of CGlobalSynced
 */
CGlobalSynced* gs = NULL;
CGlobalSynced::SyncedRNG CGlobalSynced::rng;


CR_BIND(CGlobalSynced, )

CR_REG_METADATA(CGlobalSynced, (
	CR_MEMBER(frameNum),
	CR_MEMBER(tempNum),

	CR_MEMBER(speedFactor),
	CR_MEMBER(wantedSpeedFactor),

	CR_MEMBER(paused),
	CR_MEMBER(godMode),
	CR_MEMBER(cheatEnabled),
	CR_MEMBER(noHelperAIs),
	CR_MEMBER(editDefsEnabled),
	CR_MEMBER(useLuaGaia),

	CR_MEMBER(randSeed),
	CR_MEMBER(initRandSeed)
))


/**
 * Initializes variables in CGlobalSynced
 */
CGlobalSynced::CGlobalSynced()
{
	randSeed     = 18655;
	initRandSeed = randSeed;

	assert(teamHandler == NULL);
	ResetState();
}


CGlobalSynced::~CGlobalSynced()
{
	SafeDelete(teamHandler);
	assert(teamHandler == NULL);

	log_framePrefixer_setFrameNumReference(NULL);
}


void CGlobalSynced::ResetState() {
	frameNum = -1; // first real frame is 0
	tempNum  =  1;

	speedFactor       = 1.0f;
	wantedSpeedFactor = 1.0f;

	paused  = false;
	godMode = false;

	cheatEnabled    = false;
	noHelperAIs     = false;
	editDefsEnabled = false;
	useLuaGaia      = true;

	log_framePrefixer_setFrameNumReference(&frameNum);

	if (teamHandler == NULL) {
		// needs to be available as early as PreGame
		teamHandler = new CTeamHandler();
	} else {
		// less cavemanly than delete + new
		teamHandler->ResetState();
		skirmishAIHandler.ResetState();
	}
}

void CGlobalSynced::LoadFromSetup(const CGameSetup* setup)
{
	noHelperAIs     = setup->noHelperAIs;
	useLuaGaia      = setup->useLuaGaia;

	teamHandler->LoadFromSetup(setup);
	skirmishAIHandler.LoadFromSetup(*setup);
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
		ret.x = randFloat() * 2.0f - 1.0f;
		ret.y = randFloat() * 2.0f - 1.0f;
		ret.z = randFloat() * 2.0f - 1.0f;
	} while (ret.SqLength() > 1.0f);

	return ret;
}
