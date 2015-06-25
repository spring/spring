/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_WEAPONDEFS_H
#define LUA_WEAPONDEFS_H

struct lua_State;

class LuaWeaponDefs {
public:
	static bool PushEntries(lua_State* L);
};

#endif /* LUA_WEAPONDEFS_H */
