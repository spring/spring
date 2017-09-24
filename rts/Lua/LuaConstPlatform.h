/* This file is part of the Spring System (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONST_PLATFORM_H
#define LUA_CONST_PLATFORM_H

struct lua_State;

class LuaConstPlatform {
	public:
		static bool PushEntries(lua_State* L);
};

#endif
