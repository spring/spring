/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_GAIA_H
#define LUA_GAIA_H

#include <string>

#include "LuaHandleSynced.h"


class CLuaGaia : public CSplitLuaHandle
{
public:
	static bool CanLoadHandler();
	static bool ReloadHandler(bool onlySynced = false) { return (FreeHandler(), LoadFreeHandler(onlySynced)); } // NOTE the ','
	static bool LoadFreeHandler(bool onlySynced = false) { return (LoadHandler(onlySynced) || FreeHandler()); }

	static bool LoadHandler(bool onlySynced);
	static bool FreeHandler();

protected:
	bool AddSyncedCode(lua_State* L) override { return true; }
	bool AddUnsyncedCode(lua_State* L) override { return true; }

	std::string GetUnsyncedFileName() const;
	std::string GetSyncedFileName() const;
	std::string GetInitFileModes() const;
	int GetInitSelectTeam() const;

private:
	CLuaGaia(bool onlySynced);
	virtual ~CLuaGaia();
};


extern CLuaGaia* luaGaia;


#endif /* LUA_GAIA_H */
