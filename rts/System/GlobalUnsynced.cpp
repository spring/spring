/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief Globally accessible stuff
 * Contains implementation of synced and unsynced global stuff.
 */

#include "StdAfx.h"

#include <string>
#include <assert.h>
#include <SDL_timer.h>

#include "GlobalUnsynced.h"
#include "Game/GameSetup.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/mmgr.h"
#include "System/Util.h"
#include "System/ConfigHandler.h"
#include "System/LogOutput.h"


/**
 * @brief global unsynced
 *
 * Global instance of CGlobalUnsynced
 */
CGlobalUnsynced* gu;


CR_BIND(CGlobalUnsynced, );

CR_REG_METADATA(CGlobalUnsynced, (
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

	drawSky      = true;
	drawWater    = true;
	drawGround   = true;
	drawFog      = true;
	drawMapMarks = true;
	drawdebug    = false;

	active = true;
	viewRange = MAX_VIEW_RANGE;
	timeOffset = 0;

	teamNanospray = false;
	moveWarnings  = !!configHandler->Get("MoveWarnings", 0);
	buildWarnings = !!configHandler->Get("BuildWarnings", 0);

	directControl = NULL;

	maxTextureSize = 1024;
	compressTextures = false;
	atiHacks = false;

	initialNetworkTimeout =  std::max(configHandler->Get("InitialNetworkTimeout", 30), 0);
	networkTimeout = std::max(configHandler->Get("NetworkTimeout", 120), 0);
	reconnectTimeout = configHandler->Get("ReconnectTimeout", 15);
}



void CGlobalUnsynced::PostInit() {
	supportNPOTs = GLEW_ARB_texture_non_power_of_two;
	haveARB = GLEW_ARB_vertex_program && GLEW_ARB_fragment_program;
	haveGLSL = !!GLEW_VERSION_2_0;

	{
		std::string vendor = StringToLower(std::string((char*) glGetString(GL_VENDOR)));
		haveATI = (vendor.find("ati ") != std::string::npos);

		if (haveATI) {
			std::string renderer = StringToLower(std::string((char*) glGetString(GL_RENDERER)));
			//! x-series doesn't support NPOTs (but hd-series does)
			supportNPOTs = (renderer.find(" x") == std::string::npos && renderer.find(" 9") == std::string::npos);
		}
	}

	// Runtime compress textures?
	if (GLEW_ARB_texture_compression) {
		// we don't even need to check it, 'cos groundtextures must have that extension
		// default to off because it reduces quality (smallest mipmap level is bigger)
		compressTextures = !!configHandler->Get("CompressTextures", 0);
	}

	// maximum 2D texture size
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	}

	// use some ATI bugfixes?
	if ((atiHacks = !!configHandler->Get("AtiHacks", haveATI? 1: 0))) {
		logOutput.Print("ATI hacks enabled\n");
	}
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
