/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MENU_H
#define LUA_MENU_H

#include "LuaHandle.h"


class CLuaMenu : public CLuaHandle
{
public:
	static bool ReloadHandler() { return (FreeHandler(), LoadFreeHandler()); } // NOTE the ','
	static bool LoadFreeHandler() { return (LoadHandler() || FreeHandler()); }

	static bool LoadHandler();
	static bool FreeHandler();

protected:
	CLuaMenu();
	virtual ~CLuaMenu();

	string LoadFile(const string& name) const;
	static bool LoadUnsyncedCtrlFunctions(lua_State* L);
	static bool LoadUnsyncedReadFunctions(lua_State* L);
	static bool RemoveSomeOpenGLFunctions(lua_State* L);
	static bool PushGameVersion(lua_State* L);
	void InitLuaSocket(lua_State* L);
};


extern CLuaMenu* luaMenu;


#endif /* LUA_MENU_H */
