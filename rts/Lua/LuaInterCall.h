/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_INTER_CALL_H
#define LUA_INTER_CALL_H

struct lua_State;


class LuaInterCall {
	public:
		static bool PushEntriesSynced(lua_State* L);
		static bool PushEntriesUnsynced(lua_State* L);
};

#endif /* LUA_INTER_CALL_H */
