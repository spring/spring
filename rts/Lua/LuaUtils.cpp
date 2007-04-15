#include "StdAfx.h"
// LuaUtils.cpp: implementation of the CLuaUtils class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUtils.h"
#include <set>
#include <cctype>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}


static int depth;
static int maxDepth = 256;


/******************************************************************************/
/******************************************************************************/


static bool CopyPushData(lua_State* dst, lua_State* src, int index);
static bool CopyPushTable(lua_State* dst, lua_State* src, int index);


static bool CopyPushData(lua_State* dst, lua_State* src, int index)
{
	if (lua_isboolean(src, index)) {
		lua_pushboolean(dst, lua_toboolean(src, index));
	}
	else if (lua_isnumber(src, index)) {
		lua_pushnumber(dst, lua_tonumber(src, index));
	}
	else if (lua_isstring(src, index)) {
		lua_pushstring(dst, lua_tostring(src, index));
	}
	else if (lua_istable(src, index)) {
		CopyPushTable(dst, src, index);
	}
	else {
		lua_pushnil(dst); // unhandled type
		return false;
	}
	return true;
}


static bool CopyPushTable(lua_State* dst, lua_State* src, int index)
{
	if (depth > maxDepth) {
		return false;
	}

	depth++;
	lua_newtable(dst);
	const int table = (index >= 0) ? index : (lua_gettop(src) - index + 1);
	for (lua_pushnil(src); lua_next(src, table) != 0; lua_pop(src, 1)) {
		CopyPushData(dst, src, -2); // copy the key
		CopyPushData(dst, src, -1); // copy the value
		lua_rawset(dst, -3);
	}
	depth--;

	return true;
}


int LuaUtils::CopyData(lua_State* dst, lua_State* src, int count)
{
	const int srcTop = lua_gettop(src);
	if (srcTop < count) {
		return 0;
	}

	depth = 0;

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;
	for (int i = startIndex; i <= endIndex; i++) {
		CopyPushData(dst, src, i);
	}

	return count;
}


/******************************************************************************/
/******************************************************************************/
