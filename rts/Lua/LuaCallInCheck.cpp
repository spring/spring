/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "LuaCallInCheck.h"
#include "LuaInclude.h"
#include "LogOutput.h"


/******************************************************************************/
/******************************************************************************/

LuaCallInCheck::LuaCallInCheck(lua_State* _L, const char* name)
{
	L = _L;
	startTop = lua_gettop(L);
	funcName = name;
}


LuaCallInCheck::~LuaCallInCheck()
{
	const int endTop = lua_gettop(L);
	if (startTop != endTop) {
		logOutput.Print("LuaCallInCheck mismatch for %s():  start = %i,  end = %i",
		                funcName, startTop, endTop);
	}
}


/******************************************************************************/
/******************************************************************************/

