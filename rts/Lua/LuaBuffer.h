/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_BUFFER_H
#define LUA_BUFFER_H

struct lua_State;

class LuaBuffer {
public:
	static int GetBuffer(lua_State* L);
	static bool PushEntries(lua_State* L);
private:
	static bool CheckAndReportSupported(lua_State* L, const unsigned int target);
};


#endif //LUA_BUFFER_H