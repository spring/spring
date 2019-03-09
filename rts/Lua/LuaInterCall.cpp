/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaInterCall.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaGaia.h"
#include "LuaRules.h"
#include "LuaUI.h"


enum {
	LUA_UI,
	LUA_RULES,
	LUA_GAIA
};


static CLuaHandle* GetLuaHandle(lua_State* L, int index)
{
	const int* addr = (const int*) lua_touserdata(L, index);

	if (addr == nullptr) {
		luaL_error(L, "[LuaInterCall::%s] nil XCall target", __func__);
		return nullptr;
	}

	switch (*addr) {
		case LUA_UI:
			return luaUI;
		case LUA_RULES:
			// handle is not currently active
			if (luaRules == nullptr)
				return nullptr;

			return (CLuaHandle::GetHandleSynced(L)) ? static_cast<CLuaHandle*>(&luaRules->syncedLuaHandle) : static_cast<CLuaHandle*>(&luaRules->unsyncedLuaHandle);
		case LUA_GAIA:
			// handle is not currently active
			if (luaGaia == nullptr)
				return nullptr;

			return (CLuaHandle::GetHandleSynced(L)) ? static_cast<CLuaHandle*>(&luaGaia->syncedLuaHandle) : static_cast<CLuaHandle*>(&luaGaia->unsyncedLuaHandle);

		default:
			luaL_error(L, "[LuaInterCall::%s] bad XCall target %d", __func__, *addr);
	};

	return nullptr;
}


/******************************************************************************/
/******************************************************************************/


static int HandleXCall(lua_State* L)
{
	const int addrIndex = lua_upvalueindex(1);
	const int nameIndex = lua_upvalueindex(2);

	CLuaHandle* lh = GetLuaHandle(L, addrIndex);
	const char* funcName = luaL_checkstring(L, nameIndex);

	if (lh == nullptr)
		return 0;

	return lh->XCall(L, funcName);
}


static int IndexHook(lua_State* L)
{
	if (!lua_israwstring(L, -1))
		return luaL_error(L, "Script.XYZ: only strings allowed got ", lua_typename(L, -1));

	lua_pushvalue(L, lua_upvalueindex(1));
	lua_pushvalue(L, -2);
	lua_pushcclosure(L, HandleXCall, 2);
	return 1;
}


static int CallHook(lua_State* L)
{
	const int addrIndex = lua_upvalueindex(1);
	const CLuaHandle* lh = GetLuaHandle(L, addrIndex);

	if (lh == nullptr)
		return 0;

	if (lua_gettop(L) <= 1) {
		// see if the handle currently active (arg 1 is the table)
		lua_pushboolean(L, (lh != nullptr));
	} else {
		// see if the specified function exists
		lua_pushboolean(L, lh->HasXCall(luaL_checksstring(L, 2)));
	}

	return 1;
}


static int PushCallHandler(lua_State* L, int luaInstance, const string& name)
{
	lua_pushsstring(L, name);
	int* ptr = (int*) lua_newuserdata(L, sizeof(int));
	*ptr = luaInstance;
	{ // create metatable of the userdata
		lua_newtable(L); {
			lua_pushliteral(L, "__index");
			lua_pushvalue(L, -3); //userdata
			lua_pushcclosure(L,  IndexHook, 1);
			lua_rawset(L, -3);
			lua_pushliteral(L, "__call");
			lua_pushvalue(L, -3); //userdata
			lua_pushcclosure(L, CallHook, 1);
			lua_rawset(L, -3);
			lua_pushliteral(L, "__metatable");
			lua_pushliteral(L, "can't touch this");
			lua_rawset(L, -3);
		}
		lua_setmetatable(L, -2); // set the userdata's metatable
	}
	lua_rawset(L, -3);
	return 0;
}


bool LuaInterCall::PushEntriesSynced(lua_State* L)
{
	PushCallHandler(L, LUA_GAIA,  "LuaGaia");
	PushCallHandler(L, LUA_RULES, "LuaRules");
	return true;
}


bool LuaInterCall::PushEntriesUnsynced(lua_State* L)
{
	PushCallHandler(L, LUA_GAIA,  "LuaGaia");
	PushCallHandler(L, LUA_RULES, "LuaRules");
	PushCallHandler(L, LUA_UI,    "LuaUI");
	return true;
}


/******************************************************************************/
/******************************************************************************/
