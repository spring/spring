/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UNITDEFS_H
#define LUA_UNITDEFS_H

#include <string>

struct lua_State;

class LuaUnitDefs {
public:
	static bool PushEntries(lua_State* L);
};

#endif /* LUA_UNITDEFS_H */
