#ifndef LUA_SYNCED_TABLE_H
#define LUA_SYNCED_TABLE_H
// LuaSyncedTable.h: interface for the LuaSyncedTable class.
//
//   Adds UNSYNCED table, snext(), spairs(), and sipairs()
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

struct lua_State;

class LuaSyncedTable {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_SYNCED_TABLE_H */
