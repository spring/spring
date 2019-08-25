/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GlobalSynced.h"

#include <assert.h>
#include <cstring>

#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/SafeUtil.h"
#include "System/Log/FramePrefixer.h"

#ifdef SYNCCHECK
	#include "System/Sync/SyncChecker.h"
#endif


/**
 * @brief global synced
 *
 * Global instance of CGlobalSynced
 */

CGlobalSynced gsOBJ;
CGlobalSyncedRNG gsRNG;

CGlobalSynced* gs = &gsOBJ;


CR_BIND(CGlobalSynced, )

CR_REG_METADATA(CGlobalSynced, (
	CR_MEMBER(frameNum),
	CR_MEMBER(tempNum),
	CR_MEMBER(godMode),

	CR_MEMBER(speedFactor),
	CR_MEMBER(wantedSpeedFactor),

	CR_MEMBER(paused),
	CR_MEMBER(cheatEnabled),
	CR_MEMBER(noHelperAIs),
	CR_MEMBER(editDefsEnabled),
	CR_MEMBER(useLuaGaia)
))


void CGlobalSynced::Kill()
{
	log_framePrefixer_setFrameNumReference(nullptr);
}


void CGlobalSynced::ResetState() {
	frameNum = -1; // first real frame is 0
	tempNum  =  1;
	godMode  =  0;

#ifdef SYNCCHECK
	// reset checksum
	CSyncChecker::NewFrame();
#endif

	speedFactor       = 1.0f;
	wantedSpeedFactor = 1.0f;

	paused          = false;
	cheatEnabled    = false;
	noHelperAIs     = false;
	editDefsEnabled = false;
	useLuaGaia      = true;

	gsRNG.SetSeed(18655, true);
	log_framePrefixer_setFrameNumReference(&frameNum);
}

void CGlobalSynced::LoadFromSetup(const CGameSetup* setup)
{
	noHelperAIs = setup->noHelperAIs;
	useLuaGaia  = setup->useLuaGaia;

	teamHandler.ResetState();
	teamHandler.LoadFromSetup(setup);
	skirmishAIHandler.ResetState();
	skirmishAIHandler.LoadFromSetup(*setup);
}

