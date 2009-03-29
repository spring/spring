#include "StdAfx.h"
// LuaConstCMD.cpp: implementation of the LuaConstCMD class.
//
//////////////////////////////////////////////////////////////////////
#include "mmgr.h"

#include "LuaConstCMD.h"

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


bool LuaConstCMD::PushEntries(lua_State* L)
{
	LuaPushNamedNumber(L, "OPT_ALT",      ALT_KEY);
	LuaPushNamedNumber(L, "OPT_CTRL",     CONTROL_KEY);
	LuaPushNamedNumber(L, "OPT_SHIFT",    SHIFT_KEY);
	LuaPushNamedNumber(L, "OPT_RIGHT",    RIGHT_MOUSE_KEY);
	LuaPushNamedNumber(L, "OPT_INTERNAL", INTERNAL_ORDER);
	LuaPushNamedNumber(L, "OPT_SPACE",    SPACE_KEY);

	LuaPushNamedNumber(L, "WAITCODE_TIME",   CMD_WAITCODE_TIMEWAIT);
	LuaPushNamedNumber(L, "WAITCODE_DEATH",  CMD_WAITCODE_DEATHWAIT);
	LuaPushNamedNumber(L, "WAITCODE_SQUAD",  CMD_WAITCODE_SQUADWAIT);
	LuaPushNamedNumber(L, "WAITCODE_GATHER", CMD_WAITCODE_GATHERWAIT);

#define PUSH_CMD(cmd) InsertDualMapPair(L, #cmd, CMD_ ## cmd);

	PUSH_CMD(STOP);
	PUSH_CMD(INSERT);
	PUSH_CMD(REMOVE);
	PUSH_CMD(WAIT);
	PUSH_CMD(TIMEWAIT);
	PUSH_CMD(DEATHWAIT);
	PUSH_CMD(SQUADWAIT);
	PUSH_CMD(GATHERWAIT);
	PUSH_CMD(MOVE);
	PUSH_CMD(PATROL);
	PUSH_CMD(FIGHT);
	PUSH_CMD(ATTACK);
	PUSH_CMD(AREA_ATTACK);
	PUSH_CMD(GUARD);
	PUSH_CMD(AISELECT);
	PUSH_CMD(GROUPSELECT);
	PUSH_CMD(GROUPADD);
	PUSH_CMD(GROUPCLEAR);
	PUSH_CMD(REPAIR);
	PUSH_CMD(FIRE_STATE);
	PUSH_CMD(MOVE_STATE);
	PUSH_CMD(SETBASE);
	PUSH_CMD(INTERNAL);
	PUSH_CMD(SELFD);
	PUSH_CMD(SET_WANTED_MAX_SPEED);
	PUSH_CMD(LOAD_UNITS);
	PUSH_CMD(LOAD_ONTO);
	PUSH_CMD(UNLOAD_UNITS);
	PUSH_CMD(UNLOAD_UNIT);
	PUSH_CMD(ONOFF);
	PUSH_CMD(RECLAIM);
	PUSH_CMD(CLOAK);
	PUSH_CMD(STOCKPILE);
	PUSH_CMD(DGUN);
	PUSH_CMD(RESTORE);
	PUSH_CMD(REPEAT);
	PUSH_CMD(TRAJECTORY);
	PUSH_CMD(RESURRECT);
	PUSH_CMD(CAPTURE);
	PUSH_CMD(AUTOREPAIRLEVEL);
	PUSH_CMD(LOOPBACKATTACK);
	PUSH_CMD(IDLEMODE);
	
	return true;
}


/******************************************************************************/
/******************************************************************************/
