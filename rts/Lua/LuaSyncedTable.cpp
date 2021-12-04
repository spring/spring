/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaSyncedTable.h"

#include "LuaInclude.h"

#include "LuaHandleSynced.h"
#include "LuaHashString.h"
#include "LuaUtils.h"


static int SyncTableIndex(lua_State* L);
static int SyncTableNewIndex(lua_State* L);
static int SyncTableMetatable(lua_State* L);


/******************************************************************************/
/******************************************************************************/

static bool PushSyncedTable(lua_State* L)
{
	HSTR_PUSH(L, "SYNCED");
	lua_newtable(L); { // the proxy table

		lua_newtable(L); { // the metatable
			LuaPushNamedCFunc(L, "__index",     SyncTableIndex);
			LuaPushNamedCFunc(L, "__newindex",  SyncTableNewIndex);
			LuaPushNamedCFunc(L, "__metatable", SyncTableMetatable);
		}

		lua_setmetatable(L, -2);
	}
	lua_rawset(L, -3);

	return true;
}


/******************************************************************************/

static int SyncTableIndex(lua_State* dstL)
{
	if (lua_isnoneornil(dstL, -1))
		return 0;

	auto slh = CSplitLuaHandle::GetSyncedHandle(dstL);
	if (!slh->IsValid())
		return 0;
	auto srcL = slh->GetLuaState();

	const int srcTop = lua_gettop(srcL);
	const int dstTop = lua_gettop(dstL);

	// copy the index & get value
	lua_pushvalue(srcL, LUA_GLOBALSINDEX);
	const int keyCopied = LuaUtils::CopyData(srcL, dstL, 1);
	assert(keyCopied > 0);
	lua_rawget(srcL, -2);

	// copy to destination
	const int valueCopied = LuaUtils::CopyData(dstL, srcL, 1);
	if (lua_istable(dstL, -1)) {
		// disallow writing in SYNCED[...]
		lua_newtable(dstL); { // the metatable
			LuaPushNamedCFunc(dstL, "__newindex",  SyncTableNewIndex);
			LuaPushNamedCFunc(dstL, "__metatable", SyncTableMetatable);
		}
		lua_setmetatable(dstL, -2);
	}

	assert(valueCopied == 1);
	assert(dstTop + 1 == lua_gettop(dstL));

	lua_settop(srcL, srcTop);
	return valueCopied;
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
/******************************************************************************/

bool LuaSyncedTable::PushEntries(lua_State* L)
{
	PushSyncedTable(L);

	// backward compability
	lua_pushliteral(L, "snext");
		lua_getglobal(L, "next");
		lua_rawset(L, -3);
	lua_pushliteral(L, "spairs");
		lua_getglobal(L, "pairs");
		lua_rawset(L, -3);
	lua_pushliteral(L, "sipairs");
		lua_getglobal(L, "ipairs");
		lua_rawset(L, -3);

	return true;
}


/******************************************************************************/
/******************************************************************************/
