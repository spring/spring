#ifndef LUA_UNSYNCED_CALL_H
#define LUA_UNSYNCED_CALL_H
// LuaUnsyncedCall.h: interface for the LuaUnsyncedCall class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


struct lua_State;


class LuaUnsyncedCall {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_UNSYNCED_CALL_H */
