/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_GAIA_H
#define LUA_GAIA_H

#include <string>
using std::string;

#include "LuaHandleSynced.h"


class CLuaGaia : public CLuaHandleSynced
{
	public:
		static void LoadHandler();
		static void FreeHandler();

	protected:
		bool AddSyncedCode(lua_State *L);
		bool AddUnsyncedCode(lua_State *L);

	private:
		CLuaGaia();
		~CLuaGaia();
};


extern CLuaGaia* luaGaia;


#endif /* LUA_GAIA_H */
