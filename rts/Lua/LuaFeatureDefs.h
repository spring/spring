/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_FEATUREDEFS_H
#define LUA_FEATUREDEFS_H

#include <string>

struct lua_State;

class LuaFeatureDefs {
public:
	static bool PushEntries(lua_State* L);
};

#endif /* LUA_FEATUREDEFS_H */
