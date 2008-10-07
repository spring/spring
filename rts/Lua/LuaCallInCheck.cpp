#include "StdAfx.h"
// LuaCallInCheck.cpp: implementation of the LuaCallInCheck class.
//
//////////////////////////////////////////////////////////////////////
#include "mmgr.h"

#include "LuaCallInCheck.h"
#include "LuaInclude.h"
#include "System/LogOutput.h"


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

