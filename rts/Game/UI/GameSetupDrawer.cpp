/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameSetupDrawer.h"

#include "KeyBindings.h"
#include "StartPosSelecter.h"
#include "Game/CameraHandler.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Net/GameServer.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Rendering/Fonts/glFont.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"

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
		instance->lastTick = spring_gettime(); //FIXME
		instance->readyCountdown = spring_msecs(time);
	}
}


GameSetupDrawer::GameSetupDrawer():
	readyCountdown(spring_notime),
	lastTick(spring_notime)
{
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
	if (readyCountdown > spring_nulltime) {
		readyCountdown -= (spring_gettime() - lastTick);
		lastTick = spring_gettime();

		if (readyCountdown <= spring_nulltime) {
			GameSetupDrawer::Disable();
			return; // *this is deleted!
		}
	}

	std::map<int, std::string> playerStates;
	std::string startState = "Unknown state.";

	if (readyCountdown > spring_nulltime) {
		startState = "Starting in " + IntToString(readyCountdown.toSecsi(), "%i");
	} else if (!playerHandler->Player(gu->myPlayerNum)->spectator && !playerHandler->Player(gu->myPlayerNum)->IsReadyToStart()) {
		startState = "Choose start pos";
	} else if (gameServer) {
		// we are the host and can get the show on the road by force
		const CKeyBindings::HotkeyList& fsKeys = keyBindings->GetHotkeys("forcestart");
		const std::string fsKey = fsKeys.empty() ? "<none>" : *fsKeys.begin();
		startState = std::string("Waiting for players, press ") + fsKey + " to force start";
	} else {
		startState = "Waiting for players";
	}

	const unsigned int numPlayers = playerHandler->ActivePlayers();

	// not the most efficent way to do this, but who cares?
	for (unsigned int a = 0; a < numPlayers; a++) {
		const CPlayer* player = playerHandler->Player(a);

		if (!player->active) {
			// player does not become active until we receive NETMSG_PLAYERNAME
			playerStates[a] = "missing";
		} else if (!player->spectator && !player->IsReadyToStart()) {
			playerStates[a] = "notready";
		} else {
			playerStates[a] = "ready";
		}
	}

	// if choosing in-game, selector remains non-NULL
	// so long as a position has not been picked yet
	// and deletes itself afterwards
	bool playerHasReadied = (CStartPosSelecter::GetSelector() == NULL);

	if (eventHandler.GameSetup(startState, playerHasReadied, playerStates)) {
		if (CStartPosSelecter::GetSelector() != NULL) {
			CStartPosSelecter::GetSelector()->ShowReadyBox(false);

			if (playerHasReadied) {
				// either we were ready before or a client
				// listening to the GameSetup event forced
				// us to be
				CStartPosSelecter::GetSelector()->Ready(true);
			}
		}

		// LuaUI says it will do the rendering
		return;
	}

	// LuaUI doesn't want to draw, keep showing the box
	if (CStartPosSelecter::GetSelector() != NULL) {
		CStartPosSelecter::GetSelector()->ShowReadyBox(true);
	}

	font->Begin();
	font->SetColors(); // default
	font->glPrint(0.3f, 0.7f, 1.0f, FONT_OUTLINE | FONT_SCALE | FONT_NORM, startState);

	for (unsigned int a = 0; a <= numPlayers; a++) {
		static const float4      red(1.0f, 0.2f, 0.2f, 1.0f);
		static const float4    green(0.2f, 1.0f, 0.2f, 1.0f);
		static const float4   yellow(0.8f, 0.8f, 0.2f, 1.0f);
		static const float4    white(1.0f, 1.0f, 1.0f, 1.0f);
		static const float4     cyan(0.0f, 0.9f, 0.9f, 1.0f);
		static const float4 lightred(1.0f, 0.5f, 0.5f, 1.0f);

		const float fontScale = 1.0f;
		const float fontSize  = fontScale * font->GetSize();
		const float yScale = fontSize * font->GetLineHeight() * globalRendering->pixelY;
		// note: list is drawn in reverse order, last player first
		const float yPos = 0.5f - (0.5f * yScale * numPlayers) + (yScale * a);
		const float xPos = 10.0f * globalRendering->pixelX;

		const CPlayer* player = NULL;
		const float4* color = NULL;

		std::string name;

		if (a == numPlayers) {
			color = &white;
			name = "Players:";
		} else {
			player = playerHandler->Player(a);
			name = player->name;

			if (player->spectator) {
				if (!player->active) {
					color = &lightred;
				} else {
					color = &cyan;
				}
			} else if (!player->active) {
				color = &red;
			} else if (!player->IsReadyToStart()) {
				color = &yellow;
			} else {
				color = &green;
			}
		}

		font->SetColors(color, NULL);
		font->glPrint(xPos, yPos, fontSize, FONT_OUTLINE | FONT_NORM, name);
	}
	font->End();
}
