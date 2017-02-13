/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MENU_CONTROLLER
#define LUA_MENU_CONTROLLER

#include "Game/GameController.h"
#include "System/Misc/SpringTime.h"

class CLuaMenuController : public CGameController
{
public:
	CLuaMenuController(const std::string& menuName);
	~CLuaMenuController();

	// Reloads the archives but not LuaMenu
	void Reset();
	bool Activate();

	int KeyReleased(int k) override;
	int KeyPressed(int k, bool isRepeat) override;
	int TextInput(const std::string& utf8Text) override;

	bool Draw() override;
	void ResizeEvent() override;



	bool Valid() const { return !menuArchive.empty(); }
	const std::string& GetMenuName() const { return menuArchive; }

private:
	std::string menuArchive;
	spring_time lastDrawFrameTime;
};

extern CLuaMenuController* luaMenuController;

#endif //LUA_MENU_CONTROLLER
