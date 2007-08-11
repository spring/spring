#ifndef LUA_SYNCED_CALL_H
#define LUA_SYNCED_CALL_H
// LuaSyncedCall.h: interface for the LuaSyncedCall class.
//
//////////////////////////////////////////////////////////////////////

struct lua_State;


class LuaSyncedCall {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_SYNCED_CALL_H */
