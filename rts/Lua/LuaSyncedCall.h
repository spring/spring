#ifndef LUA_SYNCED_CALL_H
#define LUA_SYNCED_CALL_H
// LuaSyncedCall.h: interface for the LuaSyncedCall class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


struct lua_State;


class LuaSyncedCall {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_SYNCED_CALL_H */
