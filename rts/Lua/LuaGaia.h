/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_GAIA_H
#define LUA_GAIA_H

#include <string>
using std::string;

#include "LuaHandleSynced.h"


class CLuaGaia : public CLuaHandleSynced
{
	public:
		static bool ReloadHandler() { return (FreeHandler(), LoadFreeHandler()); } // NOTE the ','
		static bool LoadFreeHandler() { return (LoadHandler() || FreeHandler()); }

		static bool LoadHandler();
		static bool FreeHandler();

	protected:
		bool AddSyncedCode(lua_State* L);
		bool AddUnsyncedCode(lua_State* L);

	private:
		CLuaGaia();
		virtual ~CLuaGaia();
};


extern CLuaGaia* luaGaia;


#endif /* LUA_GAIA_H */
