/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief Globally accessible stuff
 * Contains implementation of synced and unsynced global stuff.
 */

#include "StdAfx.h"

#include "GlobalUnsynced.h"
#include "Game/PlayerHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalConstants.h" // for RANDINT_MAX
#include "Sim/Units/Unit.h" // required by CREG
#include "System/mmgr.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/creg/creg_cond.h"

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
	CR_MEMBER(startTime),
	CR_MEMBER(myPlayerNum),
	CR_MEMBER(myTeam),
	CR_MEMBER(myAllyTeam),
	CR_MEMBER(spectating),
	CR_MEMBER(spectatingFullView),
	CR_MEMBER(spectatingFullSelect),
	CR_MEMBER(directControl),
	CR_MEMBER(usRandSeed),
	CR_RESERVED(64)
));

CGlobalUnsynced::CGlobalUnsynced()
{
	usRandSeed = time(NULL) % ((SDL_GetTicks() + 1) * 9007);

	modGameTime = 0;
	gameTime = 0;
	startTime = 0;

	myPlayerNum = 0;
	myTeam = 1;
	myAllyTeam = 1;
	myPlayingTeam = -1;
	myPlayingAllyTeam = -1;

	spectating           = false;
	spectatingFullView   = false;
	spectatingFullSelect = false;

	directControl = NULL;
	playerHandler = new CPlayerHandler();

	globalQuit = false;
}

CGlobalUnsynced::~CGlobalUnsynced()
{
	SafeDelete(playerHandler);
}



void CGlobalUnsynced::LoadFromSetup(const CGameSetup* setup)
{
	playerHandler->LoadFromSetup(setup);
}



/**
 * @return unsynced random integer
 *
 * Returns an unsynced random integer
 */
int CGlobalUnsynced::usRandInt()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return (usRandSeed >> 16) & RANDINT_MAX;
}

/**
 * @return unsynced random float
 *
 * returns an unsynced random float
 */
float CGlobalUnsynced::usRandFloat()
{
	usRandSeed = (usRandSeed * 214013L + 2531011L);
	return float((usRandSeed >> 16) & RANDINT_MAX) / RANDINT_MAX;
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

void CGlobalUnsynced::SetMyPlayer(const int myNumber)
{
	myPlayerNum = myNumber;

	const CPlayer* myPlayer = playerHandler->Player(myPlayerNum);

	myTeam = myPlayer->team;
	if (!teamHandler->IsValidTeam(myTeam)) {
		throw content_error("Invalid MyTeam in player setup");
	}

	myAllyTeam = teamHandler->AllyTeam(myTeam);
	if (!teamHandler->IsValidAllyTeam(myAllyTeam)) {
		throw content_error("Invalid MyAllyTeam in player setup");
	}

	spectating           = myPlayer->spectator;
	spectatingFullView   = myPlayer->spectator;
	spectatingFullSelect = myPlayer->spectator;

	if (!spectating) {
		myPlayingTeam = myTeam;
		myPlayingAllyTeam = myAllyTeam;
	}
}
