#include "StdAfx.h"
// LuaBitOps.cpp: implementation of the LuaBitOps class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaBitOps.h"

#include "LuaInclude.h"

#include "LuaUtils.h"


const int mask = 0x00FFFFFF; // 2^24


/******************************************************************************/
/******************************************************************************/

bool LuaBitOps::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "bit_or",   bit_or);
	LuaPushNamedCFunc(L, "bit_and",  bit_and);
	LuaPushNamedCFunc(L, "bit_xor",  bit_xor);
	LuaPushNamedCFunc(L, "bit_inv",  bit_inv);
	LuaPushNamedCFunc(L, "bit_bits", bit_bits);
	return true;
}


/******************************************************************************/
/******************************************************************************/

int LuaBitOps::bit_or(lua_State* L)
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


int LuaBitOps::bit_and(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to bit_and()");
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


int LuaBitOps::bit_xor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to bit_xor()");
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


int LuaBitOps::bit_inv(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to bit_inv()");
	}
	const unsigned int b1 = (unsigned int)lua_tonumber(L, 1);
	lua_pushnumber(L, (~b1) & mask);
	return 1;
}


int LuaBitOps::bit_bits(lua_State* L)
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
