/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_LOADSCREEN_H
#define LUA_LOADSCREEN_H

#include "LuaHandle.h"


struct lua_State;


class CLuaIntro : public CLuaHandle
{
public:
	static bool CanLoadHandler() { return true; }
	static bool ReloadHandler() { return (FreeHandler(), LoadFreeHandler()); } // NOTE the ','
	static bool LoadFreeHandler() { return (LoadHandler() || FreeHandler()); }

	static bool LoadHandler();
	static bool FreeHandler();

public: // call-ins
	void DrawLoadScreen() override;
	void LoadProgress(const std::string& msg, const bool replace_lastline) override;

	void GamePreload() override;

protected:
	CLuaIntro();
	virtual ~CLuaIntro();

	std::string LoadFile(const std::string& filename) const;

private:
	static bool LoadUnsyncedCtrlFunctions(lua_State* L);
	static bool LoadUnsyncedReadFunctions(lua_State* L);
	static bool LoadSyncedReadFunctions(lua_State* L);
	static bool RemoveSomeOpenGLFunctions(lua_State* L);
};


extern CLuaIntro* luaIntro;


#endif /* LUA_LOADSCREEN_H */
