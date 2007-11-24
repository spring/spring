#ifndef GAME_SETUP_DRAWER
#define GAME_SETUP_DRAWER

#include "InputReceiver.h"

class GameSetupDrawer : public CInputReceiver
{
public:
	static void Enable();
	static void Disable();

private:
	GameSetupDrawer();
	~GameSetupDrawer();

	virtual void Draw();
	virtual bool KeyPressed(unsigned short key, bool isRepeat);

	static GameSetupDrawer* instance;
	bool lctrl_pressed;
};


#endif // GAME_SETUP_DRAWER
