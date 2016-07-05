/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_AI_H
#define LUA_AI_H

struct lua_State;

class LuaAI {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int GetAvailableAIs(lua_State* L);
};


#endif /* LUA_AI_H */
