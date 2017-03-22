/* This file is part of the Spring System (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONST_SYSTEM_H
#define LUA_CONST_SYSTEM_H

struct lua_State;

class LuaConstSystem {
	public:
		static bool PushEntries(lua_State* L);
};

#endif
