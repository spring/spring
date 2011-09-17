/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_METAL_MAP_H
#define LUA_METAL_MAP_H

struct lua_State;


class LuaMetalMap {
	public:
		static bool PushReadEntries(lua_State* L);
		static bool PushCtrlEntries(lua_State* L);

		static int GetMetalMapSize(lua_State* L);
		static int GetMetalAmount(lua_State* L);
		static int SetMetalAmount(lua_State* L);
		static int GetMetalExtraction(lua_State* L);
};


#endif /* LUA_METAL_MAP_H */
