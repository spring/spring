/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief Globally accessible stuff
 * Contains implementation of synced and unsynced global stuff.
 */

#include "StdAfx.h"

#include "GlobalUnsynced.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/Unit.h"
#include "System/mmgr.h"
#include "System/ConfigHandler.h"
#include "System/creg/creg_cond.h"

#include <string>
#include <assert.h>
#include <SDL_timer.h>


/**
 * @brief global unsynced
 *
 * Global instance of CGlobalUnsynced
 */
CGlobalUnsynced* gu;


CR_BIND(CGlobalUnsynced, );

CR_REG_METADATA(CGlobalUnsynced, (
				CR_MEMBER(modGameTime),
				CR_MEMBER(gameTime),
				CR_MEMBER(myPlayerNum),
				CR_MEMBER(myTeam),
				CR_MEMBER(myAllyTeam),
				CR_MEMBER(spectating),
				CR_MEMBER(spectatingFullView),
				CR_MEMBER(spectatingFullSelect),
				CR_MEMBER(moveWarnings),
				CR_MEMBER(buildWarnings),
				CR_MEMBER(directControl),
				CR_MEMBER(usRandSeed),
				CR_RESERVED(64)
				));

CGlobalUnsynced::CGlobalUnsynced()
{
	boost::uint64_t randnum;
	randnum = SDL_GetTicks();
	usRandSeed = randnum & 0xffffffff;

	modGameTime = 0;
	gameTime = 0;

	myPlayerNum = 0;
	myTeam = 1;
	myAllyTeam = 1;

	spectating           = false;
	spectatingFullView   = false;
	spectatingFullSelect = false;

	moveWarnings  = !!configHandler->Get("MoveWarnings", 0);
	buildWarnings = !!configHandler->Get("BuildWarnings", 0);

	directControl = NULL;
}



void CGlobalUnsynced::PostInit() {
}




/**
 * @return unsynced random integer
 *
 * Returns an unsynced random integer
 */
int CGlobalUnsynced::usRandInt()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return usRandSeed & RANDINT_MAX;
}

/**
 * @return unsynced random float
 *
 * returns an unsynced random float
 */
float CGlobalUnsynced::usRandFloat()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return float(usRandSeed & RANDINT_MAX) / RANDINT_MAX;
}

/**
 * @return unsynced random vector
 *
 * returns an unsynced random vector
 */
float3 CGlobalUnsynced::usRandVector()
{
	float3 ret;
	do {
		ret.x = usRandFloat() * 2 - 1;
		ret.y = usRandFloat() * 2 - 1;
		ret.z = usRandFloat() * 2 - 1;
	} while (ret.SqLength() > 1);

	return ret;
}

void CGlobalUnsynced::SetMyPlayer(const int mynumber)
{
	myPlayerNum = mynumber;
	if (gameSetup && gameSetup->playerStartingData.size() > mynumber)
	{
		myTeam = gameSetup->playerStartingData[myPlayerNum].team;
		myAllyTeam = gameSetup->teamStartingData[myTeam].teamAllyteam;

		spectating = gameSetup->playerStartingData[myPlayerNum].spectator;
		spectatingFullView   = gameSetup->playerStartingData[myPlayerNum].spectator;
		spectatingFullSelect = gameSetup->playerStartingData[myPlayerNum].spectator;

		assert(myPlayerNum >= 0
				&& gameSetup->playerStartingData.size() >= static_cast<size_t>(myPlayerNum)
				&& myTeam >= 0
				&& gameSetup->teamStartingData.size() >= myTeam);
	}
}
