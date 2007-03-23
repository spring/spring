#include "StdAfx.h"
// LuaConstSpring.cpp: implementation of the LuaConstSpring class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaConstSpring.h"

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaUtils.h"
#include "LuaHandle.h"


/******************************************************************************/
/******************************************************************************/

bool LuaConstSpring::PushEntries(lua_State* L)
{
	LuaPushNamedNumber(L, "NO_ACCESS_TEAM",  CLuaHandle::NoAccessTeam);	
	LuaPushNamedNumber(L, "ALL_ACCESS_TEAM", CLuaHandle::AllAccessTeam);	
	
	return true;
}


/******************************************************************************/
/******************************************************************************/
