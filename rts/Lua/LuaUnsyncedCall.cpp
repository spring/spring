#include "StdAfx.h"
// LuaUnsyncedCall.cpp: implementation of the LuaUnsyncedCall class.
//
//////////////////////////////////////////////////////////////////////

#include "mmgr.h"

#include <set>
#include <list>
#include <cctype>
using namespace std;

#include "LuaUnsyncedCall.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "Game/UI/LuaUI.h"
#include "LuaGaia.h"
#include "LuaRules.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "System/LogOutput.h"


/******************************************************************************/
/******************************************************************************/

static int HandleXCall(lua_State* L)
{
	const int addrIndex = lua_upvalueindex(1);
	const int nameIndex = lua_upvalueindex(2);

	if (!lua_isuserdata(L, addrIndex) || !lua_israwstring(L, nameIndex)) {
		luaL_error(L, "Bad function name type");
	}

	CLuaHandle** addr = (CLuaHandle**) lua_touserdata(L, addrIndex);
	if (*addr == NULL) {
		return 0; // handle is not currently active
	}
	CLuaHandle* lh = *addr;

	const char* funcName = lua_tostring(L, nameIndex);

	return lh->UnsyncedXCall(L, funcName);
}


static int IndexHook(lua_State* L)
{
	CLuaHandle** addr = (CLuaHandle**) lua_touserdata(L, lua_upvalueindex(1));
	if (!lua_israwstring(L, 2)) {
		return 0; // missing string name for function
	}
	lua_pushlightuserdata(L, addr);
	lua_pushstring(L, lua_tostring(L, 2));
	lua_pushcclosure(L, HandleXCall, 2);	
	return 1;
}


static int CallHook(lua_State* L)
{
	CLuaHandle** addr = (CLuaHandle**) lua_touserdata(L, lua_upvalueindex(1));
	const int args = lua_gettop(L); // arg 1 is the table
	if (args <= 1) {
		// is the handle currently active?
		lua_pushboolean(L, (*addr != NULL));
		return 1;
	}
	else if ((args >= 2) && lua_israwstring(L, 2)) {
		// see if the specified function exists
		const string funcName = lua_tostring(L, 2);
		CLuaHandle* lh = *addr;
		if (lh == NULL) {
			return 0; // not running
		}
		lua_pushboolean(L, lh->HasUnsyncedXCall(funcName));
		return 1;
	}
	return 0;
}


static int PushCallHandler(lua_State* L, CLuaHandle** addr, const string& name)
{
	lua_pushstring(L, name.c_str());
	CLuaHandle*** ptr = (CLuaHandle***) lua_newuserdata(L, sizeof(CLuaHandle**));
	*ptr = addr;
	lua_newtable(L); {
		lua_pushstring(L, "__index");
		lua_pushlightuserdata(L, addr);
		lua_pushcclosure(L, IndexHook, 1);
		lua_rawset(L, -3);
		lua_pushstring(L, "__call");
		lua_pushlightuserdata(L, addr);
		lua_pushcclosure(L, CallHook, 1);
		lua_rawset(L, -3);
		lua_pushstring(L, "__metatable");
		lua_pushstring(L, "can't touch this");
		lua_rawset(L, -3);
	}
	lua_setmetatable(L, -2); // set the userdata's metatable
	lua_rawset(L, -3);
	return 0;
}


bool LuaUnsyncedCall::PushEntries(lua_State* L)
{
	PushCallHandler(L, (CLuaHandle**) &luaGaia,  "LuaGaia");
	PushCallHandler(L, (CLuaHandle**) &luaRules, "LuaRules");
	PushCallHandler(L, (CLuaHandle**) &luaUI,    "LuaUI");
	return true;
}


/******************************************************************************/
/******************************************************************************/
