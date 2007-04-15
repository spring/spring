#include "StdAfx.h"
// LuaUnsyncedCall.cpp: implementation of the LuaUnsyncedCall class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUnsyncedCall.h"
#include <set>
#include <list>
#include <cctype>
using namespace std;

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaHandle.h"
#include "Game/UI/LuaUI.h"
#include "LuaCob.h"
#include "LuaGaia.h"
#include "LuaRules.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "System/LogOutput.h"


/******************************************************************************/
/******************************************************************************/

static int HandleCall(lua_State* L)
{
	const int addrIndex = lua_upvalueindex(1);
	const int nameIndex = lua_upvalueindex(2);

	if (!lua_isuserdata(L, addrIndex) || !lua_isstring(L, nameIndex)) {
		luaL_error(L, "Bad function name type");
	}

	CLuaHandle** addr = (CLuaHandle**) lua_touserdata(L, addrIndex);
	if (*addr == NULL) {
		return 0; // handle is not currently active
	}
	CLuaHandle* lh = *addr;

	const char* funcName = lua_tostring(L, nameIndex);

	printf("HandleCall(%s) %s\n", lh->GetName().c_str(), funcName);

	return lh->UnsyncedXCall(L, funcName);
}


static int CallIndex(lua_State* L)
{
	CLuaHandle** addr = (CLuaHandle**) lua_touserdata(L, lua_upvalueindex(1));
	if (*addr == NULL) {
		return 0; // handle is not currently active
	}
	if (!lua_isstring(L, 2)) {
		return 0; // missing string name for function
	}
	lua_pushlightuserdata(L, addr);
	lua_pushstring(L, lua_tostring(L, 2));
	lua_pushcclosure(L, HandleCall, 2);	
	return 1;
}


static int PushLuaHandle(lua_State* L, CLuaHandle** addr, const string& name)
{
	lua_pushstring(L, name.c_str());
	CLuaHandle*** ptr = (CLuaHandle***) lua_newuserdata(L, sizeof(CLuaHandle**));
	*ptr = addr;
	lua_newtable(L); {
		lua_pushstring(L, "__index");
		lua_pushlightuserdata(L, addr);
		lua_pushcclosure(L, CallIndex, 1);
		lua_rawset(L, -3);
	}
	lua_setmetatable(L, -2);
	lua_rawset(L, -3);
	return 0;
}


bool LuaUnsyncedCall::PushEntries(lua_State* L)
{
	PushLuaHandle(L, (CLuaHandle**) &luaCob,   "LuaCob");
	PushLuaHandle(L, (CLuaHandle**) &luaGaia,  "LuaGaia");
	PushLuaHandle(L, (CLuaHandle**) &luaRules, "LuaRules");
	PushLuaHandle(L, (CLuaHandle**) &luaUI,    "LuaUI");
	return true;
}


/******************************************************************************/
/******************************************************************************/
