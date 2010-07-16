/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

//  FIXME: it'd probably be faster overall to have a Script.NewSyncTable()
//         available to the synced side, that checks data assignments in a
//         __newindex call. This could be used in conjunction with the
//         current setup to avoid table creation in WrapTable().

#include "mmgr.h"

#include "LuaSyncedTable.h"

#include "LuaInclude.h"

#include "LuaHashString.h"
#include "LuaUtils.h"


// replace a normal table with a read-only proxy
static bool WrapTable(lua_State* L);

// iteration routines
static int Next(lua_State* L);
static int Pairs(lua_State* L);
static int IPairs(lua_State* L);

// meta-table calls
static int SyncTableIndex(lua_State* L);
static int SyncTableNewIndex(lua_State* L);
static int SyncTableMetatable(lua_State* L);


/******************************************************************************/
/******************************************************************************/


bool LuaSyncedTable::PushEntries(lua_State* L)
{
	HSTR_PUSH(L, "SYNCED");
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	WrapTable(L); // replace the GLOBAL table with a proxy
	lua_rawset(L, -3);

	LuaPushNamedCFunc(L, "snext", Next);
	LuaPushNamedCFunc(L, "spairs", Pairs);
	LuaPushNamedCFunc(L, "sipairs", IPairs);
	
	return true;
}


static bool WrapTable(lua_State* L)
{
	const int realTable = lua_gettop(L);

	lua_newtable(L); { // the proxy table

		lua_newtable(L); { // the metatable

			HSTR_PUSH(L, "__index");
			lua_pushvalue(L, realTable);
			lua_pushcclosure(L, SyncTableIndex, 1);
			lua_rawset(L, -3); // closure 

			HSTR_PUSH(L, "__newindex");
			lua_pushcfunction(L, SyncTableNewIndex);
			lua_rawset(L, -3);

			HSTR_PUSH(L, "__metatable");
			lua_pushcfunction(L, SyncTableMetatable);
			lua_rawset(L, -3);

			HSTR_PUSH(L, "realTable");
			lua_pushvalue(L, realTable);
			lua_rawset(L, -3);
		}

		lua_setmetatable(L, -2);
	}

	lua_remove(L, realTable);

	return true;
}


/******************************************************************************/

inline static bool SafeType(lua_State* L, const int& index)
{
	const int t = lua_type(L, index);

	return (
		(t == LUA_TSTRING)
	     || (t == LUA_TNUMBER)
	     || (t == LUA_TBOOLEAN)
	     || (t == LUA_TTABLE)
	);
}

/******************************************************************************/

static int SyncTableIndex(lua_State* L)
{
	lua_gettable(L, lua_upvalueindex(1));

	if (lua_isstring(L, -1) ||
	    lua_isnumber(L, -1) ||
	    lua_isboolean(L, -1)) {
		return 1;
	}
	
	if (lua_istable(L, -1)) {
		if (WrapTable(L)) {
			return 1;
		} else {
			return 0;
		}
	}
	
	return 0; // functions, userdata, etc...
}


static int SyncTableNewIndex(lua_State* L)
{
	luaL_error(L, "Attempt to write to SYNCED table");
	return 0;
}


static int SyncTableMetatable(lua_State* L)
{
	luaL_error(L, "Attempt to access SYNCED metatable");
	return 0;
}


/******************************************************************************/

static inline void PushRealTable(lua_State* L, int index, const char* name)
{
	if (lua_getmetatable(L, 1) == 0) {
		luaL_error(L, "Error: using %s() with an invalid table", name);
	}
	HSTR_PUSH(L, "realTable");
	lua_rawget(L, -2);
	if (!lua_istable(L, -1)) {
		luaL_error(L, "Error: using %s() with an invalid table", name);
	}
	return;
}


static int Next(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 2); // create a 2nd argument if there isn't one

	PushRealTable(L, 1, "snext");
	const int realTable = lua_gettop(L);

	lua_pushvalue(L, 2); // push the key to the top
	lua_pushnil(L); // gets removed by lua_pop beneath!
	do {
		lua_pop(L, 1); // remove the value
		if (!lua_next(L, realTable)) {
			lua_pushnil(L);
			return 1;
		}
	} while (lua_istable(L, -2) || !SafeType(L, -1));

	if (lua_istable(L, -1)) {
		WrapTable(L);
	}
	
	return 2;
}


static int Pairs(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushcfunction(L, Next); // iterator
	lua_pushvalue(L, 1);        // state (table)
	lua_pushnil(L);             // initial value
	return 3;
}


static int IPairs(lua_State* L)
{
	lua_Number i = lua_tonumber(L, 2);
	luaL_checktype(L, 1, LUA_TTABLE);

	// initializing
	if ((i == 0) && lua_isnone(L, 2)) {
		PushRealTable(L, 1, "sipairs");
		lua_pushcclosure(L, IPairs, 1); // iterator
		lua_pushvalue(L, 1);            // state (table)
		lua_pushnumber(L, 0);           // initial value
		return 3;
	}

	lua_pushnil(L);
	lua_pushnil(L); // gets removed by lua_pop beneath!
	do {
		lua_pop(L, 2); // remove old i,v pair

		// incrementing
		i++;
		lua_pushnumber(L, i);                        // key (i)
		lua_rawgeti(L, lua_upvalueindex(1), (int)i); // value (v)

		// no more elements left in the table
		if (lua_isnil(L, -1)) {
			return 1;
		}
	} while (!SafeType(L, -1));

	// wrap tables
	if (lua_istable(L, -1)) {
		WrapTable(L);
	}

	return 2;
}


/******************************************************************************/
/******************************************************************************/
