#include "StdAfx.h"
// LuaUtils.cpp: implementation of the CLuaUtils class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUtils.h"
#include <set>
#include <cctype>

#include "LuaInclude.h"


static       int depth = 0;
static const int maxDepth = 256;


/******************************************************************************/
/******************************************************************************/


static bool CopyPushData(lua_State* dst, lua_State* src, int index);
static bool CopyPushTable(lua_State* dst, lua_State* src, int index);


static inline int PosLuaIndex(lua_State* src, int index)
{
	if (index > 0) {
		return index;
	} else {
		return (lua_gettop(src) + index + 1);
	}
}


static bool CopyPushData(lua_State* dst, lua_State* src, int index)
{
	if (lua_isboolean(src, index)) {
		lua_pushboolean(dst, lua_toboolean(src, index));
	}
	else if (lua_israwnumber(src, index)) {
		lua_pushnumber(dst, lua_tonumber(src, index));
	}
	else if (lua_israwstring(src, index)) {
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
		lua_pushnil(dst); // push something
		return false;
	}

	depth++;
	lua_newtable(dst);
	const int table = PosLuaIndex(src, index);
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
	const int dstTop = lua_gettop(dst);
	if (srcTop < count) {
		return 0;
	}
	lua_checkstack(dst, dstTop + count); // FIXME: not enough for table chains

	depth = 0;

	const int startIndex = (srcTop - count + 1);
	const int endIndex   = srcTop;
	for (int i = startIndex; i <= endIndex; i++) {
		CopyPushData(dst, src, i);
	}
	lua_settop(dst, dstTop + count);

	return count;
}


/******************************************************************************/
/******************************************************************************/

static void PushCurrentFunc(lua_State* L, const char* caller)
{
	// get the current function
	lua_Debug ar;
	if (lua_getstack(L, 1, &ar) == 0) {
		luaL_error(L, "%s() lua_getstack() error", caller);
	}
	if (lua_getinfo(L, "f", &ar) == 0) {
		luaL_error(L, "%s() lua_getinfo() error", caller);
	}
	if (!lua_isfunction(L, -1)) {
		luaL_error(L, "%s() invalid current function", caller);
	}
}


static void PushFunctionEnv(lua_State* L, const char* caller, int funcIndex)
{
	lua_getfenv(L, funcIndex);
	lua_pushliteral(L, "__fenv");
	lua_rawget(L, -2);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1); // there is no fenv proxy
	} else {
		lua_remove(L, -2); // remove the orig table, leave the proxy
	}

	if (!lua_istable(L, -1)) {
		luaL_error(L, "%s() invalid fenv", caller);
	}
}


void LuaUtils::PushCurrentFuncEnv(lua_State* L, const char* caller)
{
	PushCurrentFunc(L, caller);
	PushFunctionEnv(L, caller, -1);
	lua_remove(L, -2); // remove the function
}


/******************************************************************************/
/******************************************************************************/

// copied from lua/src/lauxlib.cpp:luaL_checkudata()
void* LuaUtils::GetUserData(lua_State* L, int index, const string& type)
{
	const char* tname = type.c_str();
	void *p = lua_touserdata(L, index);
	if (p != NULL) {                               // value is a userdata?
		if (lua_getmetatable(L, index)) {            // does it have a metatable?
			lua_getfield(L, LUA_REGISTRYINDEX, tname); // get correct metatable
			if (lua_rawequal(L, -1, -2)) {             // the correct mt?
				lua_pop(L, 2);                           // remove both metatables
				return p;
			}
		}
	}
	return NULL;
}


/******************************************************************************/
/******************************************************************************/

void LuaUtils::PrintStack(lua_State* L)
{
	const int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) {
		printf("  %i: type = %s (%p)", i, luaL_typename(L, i), lua_topointer(L, i));
		const int type = lua_type(L, i);
		if (type == LUA_TSTRING) {
			printf("\t\t%s\n", lua_tostring(L, i));
		} else if (type == LUA_TNUMBER) {
			printf("\t\t%f\n", lua_tonumber(L, i));
		} else if (type == LUA_TBOOLEAN) {
			printf("\t\t%s\n", lua_toboolean(L, i) ? "true" : "false");
		} else {
			printf("\n");
		}
	}
}


/******************************************************************************/
/******************************************************************************/
