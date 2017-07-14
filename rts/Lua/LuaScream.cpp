/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaScream.h"

#include "LuaInclude.h"
#include "LuaUtils.h"
#include "System/Log/ILog.h"

bool LuaScream::PushEntries(lua_State* L)
{
	CreateMetatable(L);

	REGISTER_LUA_CFUNC(CreateScream);
	return true;
}


/******************************************************************************/
/******************************************************************************/

bool LuaScream::CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, "Scream");
	HSTR_PUSH_CFUNC(L, "__gc",        meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     meta_index);
	HSTR_PUSH_CFUNC(L, "__newindex",  meta_newindex);
	lua_pop(L, 1);
	return true;
}


int LuaScream::meta_gc(lua_State* L)
{
	int* refPtr = (int*)luaL_checkudata(L, 1, "Scream");
	lua_rawgeti(L, LUA_REGISTRYINDEX, *refPtr);
	if (lua_isfunction(L, -1)) {
		const int error = lua_pcall(L, 0, 0, 0);
		if (error != 0) {
			LOG_L(L_ERROR, "Scream: error(%i) = %s",
					error, lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}
	else if (lua_isstring(L, -1)) {
		LOG("SCREAM: %s", lua_tostring(L, -1));
	}
	luaL_unref(L, LUA_REGISTRYINDEX, *refPtr);
	return 0;
}


int LuaScream::meta_index(lua_State* L)
{
	int* refPtr = (int*)luaL_checkudata(L, 1, "Scream");
	const string key = luaL_checkstring(L, 2);
	if (key == "func") {
		lua_rawgeti(L, LUA_REGISTRYINDEX, *refPtr);
		return 1;
	}
	return 0;
}


int LuaScream::meta_newindex(lua_State* L)
{
	int* refPtr = (int*)luaL_checkudata(L, 1, "Scream");
	const string key = luaL_checkstring(L, 2);
	if (key == "func") {
		lua_pushvalue(L, 3);
		lua_rawseti(L, LUA_REGISTRYINDEX, *refPtr);
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaScream::CreateScream(lua_State* L)
{
	int* refPtr = (int*)lua_newuserdata(L, sizeof(int));
	luaL_getmetatable(L, "Scream");
	lua_setmetatable(L, -2);

	lua_pushvalue(L, 1);
	*refPtr = luaL_ref(L, LUA_REGISTRYINDEX);

	return 1;
}


/******************************************************************************/
/******************************************************************************/
