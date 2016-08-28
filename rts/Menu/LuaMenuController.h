/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MENU_CONTROLLER
#define LUA_MENU_CONTROLLER

#include "Game/GameController.h"

class CLuaMenuController : public CGameController
{
public:
	CLuaMenuController(const std::string& menuName);
	~CLuaMenuController();

	// Reloads the archives but not LuaMenu
	void Reset();

	void Activate();

	int KeyReleased(int k);
	int KeyPressed(int k, bool isRepeat);
	int TextInput(const std::string& utf8Text);

	bool Draw();

	bool Valid() { return !menuArchive.empty(); }

private:
	std::string menuArchive;
};

extern CLuaMenuController* luaMenuController;

#endif //LUA_MENU_CONTROLLER
