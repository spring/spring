/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_GAIA_H
#define LUA_GAIA_H

#include <string>

#include "LuaHandleSynced.h"


class CLuaGaia : public CSplitLuaHandle
{
public:
	static bool CanLoadHandler();
	static bool ReloadHandler() { return (FreeHandler(), LoadFreeHandler()); } // NOTE the ','
	static bool LoadFreeHandler(bool dryRun = false) { return (LoadHandler(dryRun) || FreeHandler()); }

	static bool LoadHandler(bool dryRun);
	static bool FreeHandler();

protected:
	bool AddSyncedCode(lua_State* L) override { return true; }
	bool AddUnsyncedCode(lua_State* L) override { return true; }

	std::string GetUnsyncedFileName() const;
	std::string GetSyncedFileName() const;
	std::string GetInitFileModes() const;
	int GetInitSelectTeam() const;

private:
	CLuaGaia(bool dryRun);
	virtual ~CLuaGaia();
};


extern CLuaGaia* luaGaia;


#endif /* LUA_GAIA_H */
