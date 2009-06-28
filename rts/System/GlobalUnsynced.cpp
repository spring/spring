/**
 * @file GlobalStuff.cpp
 * @brief Globally accessible stuff
 *
 * Contains implementation of synced and
 * unsynced global stuff
 */
#include "StdAfx.h"

#include "GlobalUnsynced.h"

#include <cstring>
#include <assert.h>

#include "mmgr.h"
#include "Util.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Sync/SyncTracer.h"
#include "Sim/Misc/Team.h"
#include "Game/Player.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Rendering/Textures/TAPalette.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "SDL_timer.h"


/**
 * @brief global unsynced
 *
 * Global instance of CGlobalUnsyncedStuff
 */
CGlobalUnsyncedStuff* gu;


CR_BIND(CGlobalUnsyncedStuff,);

CR_REG_METADATA(CGlobalUnsyncedStuff, (
				CR_MEMBER(teamNanospray), // ??
				CR_MEMBER(modGameTime),
				CR_MEMBER(gameTime),
				CR_MEMBER(lastFrameTime),
				CR_MEMBER(lastFrameStart),
				CR_MEMBER(weightedSpeedFactor), // ??
				CR_MEMBER(drawFrame), // ??
				CR_MEMBER(myPlayerNum),
				CR_MEMBER(myTeam),
				CR_MEMBER(myAllyTeam),
				CR_MEMBER(spectating),
				CR_MEMBER(spectatingFullView),
				CR_MEMBER(spectatingFullSelect),
				CR_MEMBER(drawdebug), // ??
				CR_MEMBER(active),
				CR_MEMBER(viewRange),
				CR_MEMBER(timeOffset),
//				CR_MEMBER(compressTextures),
				CR_MEMBER(drawFog),
				CR_MEMBER(autoQuit),
				CR_MEMBER(quitTime),
				CR_MEMBER(usRandSeed),
				CR_RESERVED(64)
				));

/**
 * Initializes variables in CGlobalUnsyncedStuff
 */
CGlobalUnsyncedStuff::CGlobalUnsyncedStuff()
{
	boost::uint64_t randnum;
	randnum = SDL_GetTicks();
	usRandSeed = randnum&0xffffffff;
	modGameTime = 0;
	gameTime = 0;
	lastFrameTime = 0;
	drawFrame = 1;
	viewSizeX = 100;
	viewSizeY = 100;
	pixelX = 0.01f;
	pixelY = 0.01f;
	aspectRatio = 1.0f;
	myPlayerNum = 0;
	myTeam = 1;
	myAllyTeam = 1;
	spectating           = false;
	spectatingFullView   = false;
	spectatingFullSelect = false;
	drawdebug = false;
	active = true;
	viewRange = MAX_VIEW_RANGE;
	timeOffset = 0;
	drawFog = true;
	teamNanospray = false;
	autoQuit = false;
	quitTime = 0;
	directControl = 0;
	compressTextures = false;
	atiHacks = false;
	supportNPOTs = GLEW_ARB_texture_non_power_of_two;
	{
		std::string vendor = std::string((char*)glGetString(GL_VENDOR));
		StringToLowerInPlace(vendor);
		bool isATi = (vendor.find("ati ") != string::npos);
		if (isATi) {
			std::string renderer = std::string((char*)glGetString(GL_RENDERER));
			StringToLowerInPlace(renderer);
			supportNPOTs = (renderer.find(" x") == string::npos && renderer.find(" 9") == string::npos); //! x-series doesn't support NPOTs
		}
	}
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
	do {
		ret.x = usRandFloat() * 2 - 1;
		ret.y = usRandFloat() * 2 - 1;
		ret.z = usRandFloat() * 2 - 1;
	} while (ret.SqLength() > 1);

	return ret;
}

void CGlobalUnsyncedStuff::SetMyPlayer(const int mynumber)
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
