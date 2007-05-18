#include "StdAfx.h"
// LuaBoolOps.cpp: implementation of the LuaBoolOps class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaBoolOps.h"

#include "LuaInclude.h"

#include "LuaUtils.h"


const int mask = 0x00FFFFFF; // 2^24


/******************************************************************************/
/******************************************************************************/

bool LuaBoolOps::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "bool_or",   bool_or);
	LuaPushNamedCFunc(L, "bool_and",  bool_and);
	LuaPushNamedCFunc(L, "bool_xor",  bool_xor);
	LuaPushNamedCFunc(L, "bool_inv",  bool_inv);
	LuaPushNamedCFunc(L, "bool_bits", bool_bits);
	return true;
}


/******************************************************************************/
/******************************************************************************/

int LuaBoolOps::bool_or(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	unsigned int result = 0x00000000;
	for (int i = 1; i <= args; i++) {
		if (!lua_isnumber(L, i)) {
			break;
		}
		result = result | (unsigned int)lua_tonumber(L, i);
	}
	lua_pushnumber(L, result & mask);
	return 1;
}


int LuaBoolOps::bool_and(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to bool_and()");
	}
	unsigned int result = 0xFFFFFFFF;
	for (int i = 1; i <= args; i++) {
		if (!lua_isnumber(L, i)) {
			break;
		}
		result = result & (unsigned int)lua_tonumber(L, i);
	}
	lua_pushnumber(L, result & mask);
	return 1;
}


int LuaBoolOps::bool_xor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to bool_xor()");
	}
	unsigned int result = 0x00000000;
	for (int i = 1; i <= args; i++) {
		if (!lua_isnumber(L, i)) {
			break;
		}
		result = result ^ (unsigned int)lua_tonumber(L, i);
	}
	lua_pushnumber(L, result & mask);
	return 1;
}


int LuaBoolOps::bool_inv(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to bool_inv()");
	}
	const unsigned int b1 = (unsigned int)lua_tonumber(L, 1);
	lua_pushnumber(L, (~b1) & mask);
	return 1;
}


int LuaBoolOps::bool_bits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	unsigned int result = 0x00000000;
	for (int i = 1; i <= args; i++) {
		if (!lua_isnumber(L, i)) {
			break;
		}
		const int bit = (unsigned int)lua_tonumber(L, i);
		result = result | (1 << bit);
	}
	lua_pushnumber(L, result & mask);
	return 1;
}


/******************************************************************************/
/******************************************************************************/
