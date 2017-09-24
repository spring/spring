/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONST_GAME_H
#define LUA_CONST_GAME_H

struct lua_State;

class LuaConstGame {
	public:
		static bool PushEntries(lua_State* L);
};

#endif

