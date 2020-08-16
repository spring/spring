/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MATRIX_H
#define LUA_MATRIX_H

struct lua_State;

class LuaMatrix {
public:
	static int GetMatrix(lua_State* L);
	static bool PushEntries(lua_State* L);
};

#endif //LUA_MATRIX_H
