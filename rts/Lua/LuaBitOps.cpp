/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaBitOps.h"

#include "LuaInclude.h"

#include "LuaUtils.h"


// spring's lua uses FLOATS as its number type which can only represent
// integer values up to 1<<24 exactly
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

static inline unsigned int luaL_checkuint(lua_State* L, int index)
{
	return (unsigned int)luaL_checkint(L, index);
}


int LuaBitOps::bit_or(lua_State* L)
{
	unsigned int result = 0x00000000;
	for (int i = 1; !lua_isnone(L, i); i++) {
		result = result | luaL_checkuint(L, i);
	}
	lua_pushnumber(L, result & mask);
	return 1;
}


int LuaBitOps::bit_and(lua_State* L)
{
	unsigned int result = 0xFFFFFFFF;
	for (int i = 1; !lua_isnone(L, i); i++) {
		result = result & luaL_checkuint(L, i);
	}
	lua_pushnumber(L, result & mask);
	return 1;
}


int LuaBitOps::bit_xor(lua_State* L)
{
	unsigned int result = 0x00000000;
	for (int i = 1; !lua_isnone(L, i); i++) {
		result = result ^ luaL_checkuint(L, i);
	}
	lua_pushnumber(L, result & mask);
	return 1;
}


int LuaBitOps::bit_inv(lua_State* L)
{
	const unsigned int result = ~luaL_checkuint(L, 1);
	lua_pushnumber(L, result & mask);
	return 1;
}


int LuaBitOps::bit_bits(lua_State* L)
{
	unsigned int result = 0x00000000;
	for (int i = 1; !lua_isnone(L, i); i++) {
		const int bit = (unsigned int)luaL_checkint(L, i);
		result = result | (1 << bit);
	}
	lua_pushnumber(L, result & mask);
	return 1;
}


/******************************************************************************/
/******************************************************************************/
