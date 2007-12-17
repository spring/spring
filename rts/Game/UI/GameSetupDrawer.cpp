#include "GameSetupDrawer.h"

#include <assert.h>
#include <string>
#include <map>
#include <SDL_timer.h>
#include <SDL_keysym.h>

#include "../Player.h"
#include "../GameSetup.h"
#include "StartPosSelecter.h"
#include "LuaUI.h"
#include "Rendering/glFont.h"

GameSetupDrawer* GameSetupDrawer::instance = 0;

extern Uint8 *keys;

void GameSetupDrawer::Enable()
{
	assert(instance == 0);
	assert(gameSetup);

	instance = new GameSetupDrawer();
}

void GameSetupDrawer::Disable()
{
	if (instance)
	{
		delete instance;
		instance = 0;
	}
}

GameSetupDrawer::GameSetupDrawer()
{
	if (gameSetup->startPosType == CGameSetup::StartPos_ChooseInGame) {
		new CStartPosSelecter();
	}
	lctrl_pressed = false;
}

GameSetupDrawer::~GameSetupDrawer()
{
}

void GameSetupDrawer::Draw()
{
	float xshift = 0.0f;
	std::string state = "Unknown state.";
	if (gameSetup->readyTime > 0) {
		char buf[64];
		sprintf(buf, "Starting in %i", 3 - (SDL_GetTicks() - gameSetup->readyTime) / 1000);
		state = buf;
	} else if (!gameSetup->readyTeams[gu->myTeam]) {
		state = "Choose start pos";
	} else if (gu->myPlayerNum==0) {
		state = "Waiting for players, Ctrl+Return to force start";
	} else {
		state = "Waiting for players";
	}

	// not the most efficent way to do this, but who cares?
	std::map<int, std::string> playerStates;
	for (int a = 0; a < gameSetup->numPlayers; a++) {
		if (!gs->players[a]->readyToStart) {
			playerStates[a] = "missing";
		} else if (!gameSetup->readyTeams[gs->players[a]->team]) {
			playerStates[a] = "notready";
		} else {
			playerStates[a] = "ready";
		}
	}

	CStartPosSelecter* selector = CStartPosSelecter::selector;
	bool ready = (selector == NULL);
	if (luaUI && luaUI->GameSetup(state, ready, playerStates)) {
		if (selector) {
			selector->ShowReady(false);
			if (ready) {
				selector->Ready();
			}
		}
		return; // LuaUI says it will do the rendering
	}
	if (selector) {
		selector->ShowReady(true);
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslatef(0.3f, 0.7f, 0.0f);
	glScalef(0.03f, 0.04f, 0.1f);
	glTranslatef(xshift, 0.0f, 0.0f);
	font->glPrint(state.c_str());
	glPopMatrix();

	for (int a = 0; a <= gameSetup->numPlayers; a++) {
		const float* color;
		const float red[4]    = { 1.0f ,0.2f, 0.2f, 1.0f };
		const float green[4]  = { 0.2f, 1.0f, 0.2f, 1.0f };
		const float yellow[4] = { 0.8f, 0.8f, 0.2f, 1.0f };
		const float dark[4]   = { 0.2f, 0.2f, 0.2f, 0.8f };
		const float white[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };
		if (a == gameSetup->numPlayers) {
			color = white; //blue;
		} else if (!gs->players[a]->readyToStart) {
			color = red;
		} else if (!gameSetup->readyTeams[gs->players[a]->team]) {
			color = yellow;
		} else {
			color = green;
		}

		const char* name;
		if (a == gameSetup->numPlayers) {
			name = "Players:";
		} else {
			name  = gs->players[a]->playerName.c_str();
		}
		const float yScale = 0.028f;
		const float xScale = yScale / gu->aspectRatio;
		const float xPixel  = 1.0f / (xScale * (float)gu->viewSizeX);
		const float yPixel  = 1.0f / (yScale * (float)gu->viewSizeY);
		const float yPos = 0.5f - (0.5f * yScale * gameSetup->numPlayers) + (yScale * (float)a);
		const float xPos = xScale;
		glPushMatrix();
		glTranslatef(xPos, yPos, 0.0f);
		glScalef(xScale, yScale, 1.0f);
		font->glPrintOutlined(name, xPixel, yPixel, color, dark);
		glPopMatrix();
	}
}

bool GameSetupDrawer::KeyPressed(unsigned short key, bool isRepeat)
{
	if (keys[SDLK_LCTRL] && key == SDLK_RETURN)
		gameSetup->forceReady = true;
	return false;
}


