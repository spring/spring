/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "LuaMetalMap.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"

/******************************************************************************/
/******************************************************************************/

bool LuaMetalMap::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(GetMetalMapSize);
	REGISTER_LUA_CFUNC(GetMetalMapAmount);
	REGISTER_LUA_CFUNC(GetMetalMapExtraction);

	return true;
}


int LuaMetalMap::GetMetalMapSize(lua_State* L)
{
	lua_pushnumber(L, readmap->metalMap->GetSizeX());
	lua_pushnumber(L, readmap->metalMap->GetSizeZ());
	return 2;
}

int LuaMetalMap::GetMetalMapAmount(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		luaL_error(L, "Incorrect arguments to GetMetalMapAmount(int, int)");

	const int x = lua_toint(L, 1);
	const int z = lua_toint(L, 2);
	// GetMetalAmount automatically clamps the value
	lua_pushnumber(L, readmap->metalMap->GetMetalAmount(x, z));
	return 1;
}

int LuaMetalMap::GetMetalMapExtraction(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		luaL_error(L, "Incorrect arguments to GetMetalMapExtraction(int, int)");

	const int x = lua_toint(L, 1);
	const int z = lua_toint(L, 2);
	// GetExtraction automatically clamps the value
	lua_pushnumber(L, readmap->metalMap->GetExtraction(x, z));
	return 1;
}




/******************************************************************************/
/******************************************************************************/
