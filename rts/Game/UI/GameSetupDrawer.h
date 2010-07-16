/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_SETUP_DRAWER
#define GAME_SETUP_DRAWER

#include "InputReceiver.h"

class GameSetupDrawer : public CInputReceiver
{
public:
	static void Enable();
	static void Disable();
	
	static void StartCountdown(unsigned time);

private:
	GameSetupDrawer();
	~GameSetupDrawer();

	virtual void Draw();

	static GameSetupDrawer* instance;
	bool lctrl_pressed;
	int readyCountdown;
	unsigned lastTick;
};


#endif // GAME_SETUP_DRAWER
