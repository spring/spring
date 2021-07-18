/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VBO_H
#define LUA_VBO_H

struct lua_State;

class LuaVBO {
public:
	static int GetVBO(lua_State* L);
	static bool PushEntries(lua_State* L);
private:
	static bool CheckAndReportSupported(lua_State* L, const unsigned int target);
};


#endif //LUA_VBO_H