/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameSetupDrawer.h"

#include "KeyBindings.h"
#include "StartPosSelecter.h"
#include "Game/GameServer.h"
#include "Game/CameraHandler.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Rendering/glFont.h"
#include "System/NetProtocol.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"

#include <SDL_timer.h>
#include <SDL_keysym.h>

#include <cassert>
#include <string>
#include <map>


GameSetupDrawer* GameSetupDrawer::instance = NULL;


void GameSetupDrawer::Enable()
{
	assert(instance == NULL);
	assert(gameSetup);

	instance = new GameSetupDrawer();
}


void GameSetupDrawer::Disable()
{
	delete instance;
	instance = NULL;
}

void GameSetupDrawer::StartCountdown(unsigned time)
{
	if (instance) {
		instance->lastTick = SDL_GetTicks();
		instance->readyCountdown = (int)time;
		const std::string modeName = configHandler->GetString("CamModeName");
		if (!modeName.empty()) {
			camHandler->SetCameraMode(modeName);
		} else {
			const int modeIndex = configHandler->GetInt("CamMode");
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

		if (readyCountdown <= 0) {
			GameSetupDrawer::Disable();
			return; // *this is deleted!
		}
	}

	std::string state = "Unknown state.";
	if (readyCountdown > 0) {
		char buf[64];
		sprintf(buf, "Starting in %i", readyCountdown / 1000);
		state = buf;
	} else if (!playerHandler->Player(gu->myPlayerNum)->spectator && !playerHandler->Player(gu->myPlayerNum)->readyToStart) {
		state = "Choose start pos";
	} else if (gameServer) {
		CKeyBindings::HotkeyList list = keyBindings->GetHotkeys("forcestart");
		const std::string primary = list.empty() ? "<none>" : list.front();
		state = std::string("Waiting for players, press ")+primary + " to force start";
	} else {
		state = "Waiting for players";
	}

	int numPlayers = (int)playerHandler->ActivePlayers();
	//! not the most efficent way to do this, but who cares?
	std::map<int, std::string> playerStates;
	for (int a = 0; a < numPlayers; a++) {
		const CPlayer* player = playerHandler->Player(a);

		if (!player->active) {
			// player does not become active until we receive NETMSG_PLAYERNAME
			playerStates[a] = "missing";
		} else if (!player->spectator && !player->readyToStart) {
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

	for (int a = 0; a <= numPlayers; a++) {
		const float4* color;
		static const float4      red(1.0f, 0.2f, 0.2f, 1.0f);
		static const float4    green(0.2f, 1.0f, 0.2f, 1.0f);
		static const float4   yellow(0.8f, 0.8f, 0.2f, 1.0f);
		static const float4    white(1.0f, 1.0f, 1.0f, 1.0f);
		static const float4     cyan(0.0f, 0.9f, 0.9f, 1.0f);
		static const float4 lightred(1.0f, 0.5f, 0.5f, 1.0f);
		if (a == numPlayers) {
			color = &white;
		} else if (playerHandler->Player(a)->spectator) {
			if (!playerHandler->Player(a)->active) {
				color = &lightred;
			} else {
				color = &cyan;
			}
		} else if (!playerHandler->Player(a)->active) {
			color = &red;
		} else if (!playerHandler->Player(a)->readyToStart) {
			color = &yellow;
		} else {
			color = &green;
		}

		std::string name = "Players:";
		if (a != numPlayers) {
			name = playerHandler->Player(a)->name;
		}
		const float fontScale = 1.0f;
		const float fontSize  = fontScale * font->GetSize();
		const float yScale = fontSize * font->GetLineHeight() * globalRendering->pixelY;
		const float yPos = 0.5f - (0.5f * yScale * numPlayers) + (yScale * (float)a);
		const float xPos = 10.0f * globalRendering->pixelX;
		font->SetColors(color, NULL);
		font->glPrint(xPos, yPos, fontSize, FONT_OUTLINE | FONT_NORM, name);
	}
	font->End();
}
