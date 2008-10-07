#include "StdAfx.h"
// LuaConstCMDTYPE.cpp: implementation of the LuaConstCMDTYPE class.
//
//////////////////////////////////////////////////////////////////////
#include "mmgr.h"

#include "LuaConstCMDTYPE.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/******************************************************************************/
/******************************************************************************/

static void InsertDualMapPair(lua_State* L, const string& name, int number)
{
	lua_pushstring(L, name.c_str());
	lua_pushnumber(L, number);
	lua_rawset(L, -3);
	lua_pushnumber(L, number);
	lua_pushstring(L, name.c_str());
	lua_rawset(L, -3);
}


bool LuaConstCMDTYPE::PushEntries(lua_State* L)
{
#define PUSH_CMDTYPE(cmd) InsertDualMapPair(L, #cmd, CMDTYPE_ ## cmd)

	PUSH_CMDTYPE(ICON);
	PUSH_CMDTYPE(ICON_MODE);
	PUSH_CMDTYPE(ICON_MAP);
	PUSH_CMDTYPE(ICON_AREA);
	PUSH_CMDTYPE(ICON_UNIT);
	PUSH_CMDTYPE(ICON_UNIT_OR_MAP);
	PUSH_CMDTYPE(ICON_FRONT);
	PUSH_CMDTYPE(COMBO_BOX);
	PUSH_CMDTYPE(ICON_UNIT_OR_AREA);
	PUSH_CMDTYPE(NEXT);
	PUSH_CMDTYPE(PREV);
	PUSH_CMDTYPE(ICON_UNIT_FEATURE_OR_AREA);
	PUSH_CMDTYPE(ICON_BUILDING);
	PUSH_CMDTYPE(CUSTOM);
	PUSH_CMDTYPE(ICON_UNIT_OR_RECTANGLE);
	PUSH_CMDTYPE(NUMBER);
	
	return true;
}


/******************************************************************************/
/******************************************************************************/
