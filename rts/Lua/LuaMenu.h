/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MENU_H
#define LUA_MENU_H

#include "LuaHandle.h"


class CLuaMenu : public CLuaHandle
{
public:
	enum QueuedAction {
		ACTION_RELOAD = 0,
		ACTION_DISABLE = 1,
		ACTION_NOVALUE = -1,
	};
public:
	void QueueAction(const QueuedAction action) { queuedAction = action; }
	void CheckAction() {
		switch (queuedAction) {
		case ACTION_RELOAD : { ReloadHandler(); } break;
		case ACTION_DISABLE: {   FreeHandler(); } break;
		default            : {                  } break;
		}
	}

	static bool CanLoadHandler() { return true; }
	static bool ReloadHandler() { return (FreeHandler(), LoadFreeHandler()); } // NOTE the ','
	static bool LoadFreeHandler() { return (LoadHandler() || FreeHandler()); }

	static bool Enable(bool enableCommand);
	static bool Disable();

	static bool LoadHandler();
	static bool FreeHandler();

	// callin called when LuaMenu is active with no game
	void ActivateMenu(const std::string& msg);

	// callin called when LuaMenu is active with a game
	void ActivateGame();

	// callin called before each render, can block rendering
	bool AllowDraw();

	bool PersistOnReload() const override { return true; }

	// Don't call GamePreload since it may be called concurrent
	// with other callins during loading.
	void GamePreload() override {}

	static int SendLuaUIMsg(lua_State* L);

protected:
	CLuaMenu();
	virtual ~CLuaMenu();

	string LoadFile(const string& name) const;
	static bool LoadUnsyncedCtrlFunctions(lua_State* L);
	static bool LoadLuaMenuFunctions(lua_State* L);
	static bool LoadUnsyncedReadFunctions(lua_State* L);
	static bool RemoveSomeOpenGLFunctions(lua_State* L);
	static bool PushGameVersion(lua_State* L);
	void InitLuaSocket(lua_State* L);
protected:
	QueuedAction queuedAction;
};


extern CLuaMenu* luaMenu;


#endif /* LUA_MENU_H */
