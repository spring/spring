/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VAO_H
#define LUA_VAO_H

struct lua_State;

class LuaVAO {
public:
	static int GetVAO(lua_State* L);
	static bool PushEntries(lua_State* L);
};


#endif //LUA_VAO_H