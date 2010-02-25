/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SYNCED_CALL_H
#define LUA_SYNCED_CALL_H

struct lua_State;


class LuaSyncedCall {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_SYNCED_CALL_H */
