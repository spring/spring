#ifndef SELECT_MENU
#define SELECT_MENU

#include "GameController.h"

class SelectMenu : public CGameController
{
public:
	SelectMenu(bool server);
	
	bool Draw();
	int KeyPressed(unsigned short k, bool isRepeat);
	bool Update();
	
private:
	bool isServer;
};

#endif