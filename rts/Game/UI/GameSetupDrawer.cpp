#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <assert.h>
#include <string>
#include <map>
#include <SDL_timer.h>
#include <SDL_keysym.h>

#include "mmgr.h"

#include "GameSetupDrawer.h"

#include "NetProtocol.h"
#include "ConfigHandler.h"
#include "Game/CameraHandler.h"
#include "Game/PlayerHandler.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "StartPosSelecter.h"
#include "Rendering/glFont.h"
#include "EventHandler.h"

extern Uint8 *keys;


GameSetupDrawer* GameSetupDrawer::instance = NULL;


void GameSetupDrawer::Enable()
{
	assert(instance == NULL);
	assert(gameSetup);

	instance = new GameSetupDrawer();
}


void GameSetupDrawer::Disable()
{
	if (instance) {
		delete instance;
		instance = NULL;
	}
}


void GameSetupDrawer::StartCountdown(unsigned time)
{
	if (instance) {
		instance->lastTick = SDL_GetTicks();
		instance->readyCountdown = (int)time;
		const std::string modeName = configHandler.GetString("CamModeName", "");
		if (!modeName.empty()) {
			camHandler->SetCameraMode(modeName);
		} else {
			const int modeIndex = configHandler.Get("CamMode", 1);
			camHandler->SetCameraMode(modeIndex);
		}
	}
}


GameSetupDrawer::GameSetupDrawer()
{
	readyCountdown = 0;
	lastTick = 0;
	if (gameSetup->startPosType == CGameSetup::StartPos_ChooseInGame && !gameSetup->hostDemo) {
		new CStartPosSelecter();
	}
	lctrl_pressed = false;
}

GameSetupDrawer::~GameSetupDrawer()
{
}


void GameSetupDrawer::Draw()
{
	if (readyCountdown > 0) {
		readyCountdown -= (SDL_GetTicks() - lastTick);
		lastTick = SDL_GetTicks();
	}
	else if (readyCountdown < 0) {
		GameSetupDrawer::Disable();
	}

	std::string state = "Unknown state.";
	if (readyCountdown > 0) {
		char buf[64];
		sprintf(buf, "Starting in %i", readyCountdown / 1000);
		state = buf;
	} else if (!teamHandler->Team(gu->myTeam)->IsReadyToStart()) {
		state = "Choose start pos";
	} else if (gu->myPlayerNum==0) {
		state = "Waiting for players, Ctrl+Return to force start";
	} else {
		state = "Waiting for players";
	}

	//! not the most efficent way to do this, but who cares?
	std::map<int, std::string> playerStates;
	for (int a = 0; a < gameSetup->numPlayers; a++) {
		if (!playerHandler->Player(a)->readyToStart) {
			playerStates[a] = "missing";
		} else if (!teamHandler->Team(playerHandler->Player(a)->team)->IsReadyToStart()) {
			playerStates[a] = "notready";
		} else {
			playerStates[a] = "ready";
		}
	}

	CStartPosSelecter* selector = CStartPosSelecter::selector;
	bool ready = (selector == NULL);
	if (eventHandler.GameSetup(state, ready, playerStates)) {
		if (selector) {
			selector->ShowReady(false);
			if (ready) {
				selector->Ready();
			}
		}
		return; //! LuaUI says it will do the rendering
	}
	if (selector) {
		selector->ShowReady(true);
	}

	font->Begin();
	font->SetColors(); //! default
	font->glPrint(0.3f, 0.7f, 1.0f, FONT_OUTLINE | FONT_SCALE | FONT_NORM, state);

	for (int a = 0; a <= gameSetup->numPlayers; a++) {
		const float4* color;
		static const float4      red(1.0f, 0.2f, 0.2f, 1.0f);
		static const float4    green(0.2f, 1.0f, 0.2f, 1.0f);
		static const float4   yellow(0.8f, 0.8f, 0.2f, 1.0f);
		static const float4    white(1.0f, 1.0f, 1.0f, 1.0f);
		static const float4     cyan(0.0f, 0.9f, 0.9f, 1.0f);
		static const float4 lightred(1.0f, 0.5f, 0.5f, 1.0f);
		if (a == gameSetup->numPlayers) {
			color = &white;
		} else if (playerHandler->Player(a)->spectator) {
			if (!playerHandler->Player(a)->readyToStart) {
				color = &lightred;
			} else {
				color = &cyan;
			}
		} else if (!playerHandler->Player(a)->readyToStart) {
			color = &red;
		} else if (!teamHandler->Team(playerHandler->Player(a)->team)->IsReadyToStart()) {
			color = &yellow;
		} else {
			color = &green;
		}

		std::string name = "Players:";
		if (a != gameSetup->numPlayers) {
			name = playerHandler->Player(a)->name;
		}
		const float fontScale = 1.0f;
		const float fontSize  = fontScale * font->GetSize();
		const float yScale = fontSize * font->GetLineHeight() * gu->pixelY;
		const float yPos = 0.5f - (0.5f * yScale * gameSetup->numPlayers) + (yScale * (float)a);
		const float xPos = 10.0f * gu->pixelX;
		font->SetColors(color, NULL);
		font->glPrint(xPos, yPos, fontSize, FONT_OUTLINE | FONT_NORM, name);
	}
	font->End();
}
