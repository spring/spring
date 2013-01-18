/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaMetalMap.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"

/******************************************************************************/
/******************************************************************************/

bool LuaMetalMap::PushReadEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(GetMetalMapSize);
	REGISTER_LUA_CFUNC(GetMetalAmount);
	REGISTER_LUA_CFUNC(GetMetalExtraction);

	return true;
}

bool LuaMetalMap::PushCtrlEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(SetMetalAmount);

	return true;
}

int LuaMetalMap::GetMetalMapSize(lua_State* L)
{
	lua_pushnumber(L, readmap->metalMap->GetSizeX());
	lua_pushnumber(L, readmap->metalMap->GetSizeZ());
	return 2;
}

int LuaMetalMap::GetMetalAmount(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		luaL_error(L, "Incorrect arguments to GetMetalAmount(int, int)");

	const int x = lua_toint(L, 1);
	const int z = lua_toint(L, 2);
	// GetMetalAmount automatically clamps the value
	lua_pushnumber(L, readmap->metalMap->GetMetalAmount(x, z));
	return 1;
}

int LuaMetalMap::SetMetalAmount(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 2))
		luaL_error(L, "Incorrect arguments to SetMetalAmount(int, int, num)");

	const int x = lua_toint(L, 1);
	const int z = lua_toint(L, 2);
	const float m = lua_tonumber(L, 3);
	// SetMetalAmount automatically clamps the value
	readmap->metalMap->SetMetalAmount(x, z, m);
	return 0;
}

int LuaMetalMap::GetMetalExtraction(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		luaL_error(L, "Incorrect arguments to GetMetalExtraction(int, int)");

	const int x = lua_toint(L, 1);
	const int z = lua_toint(L, 2);
	// GetMetalExtraction automatically clamps the value
	lua_pushnumber(L, readmap->metalMap->GetMetalExtraction(x, z));
	return 1;
}




/******************************************************************************/
/******************************************************************************/
