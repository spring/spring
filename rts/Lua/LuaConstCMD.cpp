/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMD.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/******************************************************************************/
/******************************************************************************/


bool LuaConstCMD::PushEntries(lua_State* L)
{
	LuaPushNamedNumber(L, "OPT_ALT",      ALT_KEY);
	LuaPushNamedNumber(L, "OPT_CTRL",     CONTROL_KEY);
	LuaPushNamedNumber(L, "OPT_SHIFT",    SHIFT_KEY);
	LuaPushNamedNumber(L, "OPT_RIGHT",    RIGHT_MOUSE_KEY);
	LuaPushNamedNumber(L, "OPT_INTERNAL", INTERNAL_ORDER);
	LuaPushNamedNumber(L, "OPT_META",     META_KEY);

	LuaPushNamedNumber(L, "MOVESTATE_NONE"    , MOVESTATE_NONE    );
	LuaPushNamedNumber(L, "MOVESTATE_HOLDPOS" , MOVESTATE_HOLDPOS );
	LuaPushNamedNumber(L, "MOVESTATE_MANEUVER", MOVESTATE_MANEUVER);
	LuaPushNamedNumber(L, "MOVESTATE_ROAM"    , MOVESTATE_ROAM    );

	LuaPushNamedNumber(L, "FIRESTATE_NONE"         , FIRESTATE_NONE         );
	LuaPushNamedNumber(L, "FIRESTATE_HOLDFIRE"     , FIRESTATE_HOLDFIRE     );
	LuaPushNamedNumber(L, "FIRESTATE_RETURNFIRE"   , FIRESTATE_RETURNFIRE   );
	LuaPushNamedNumber(L, "FIRESTATE_FIREATWILL"   , FIRESTATE_FIREATWILL   );
	LuaPushNamedNumber(L, "FIRESTATE_FIREATNEUTRAL", FIRESTATE_FIREATNEUTRAL);

	LuaPushNamedNumber(L, "WAITCODE_TIME",   CMD_WAITCODE_TIMEWAIT);
	LuaPushNamedNumber(L, "WAITCODE_DEATH",  CMD_WAITCODE_DEATHWAIT);
	LuaPushNamedNumber(L, "WAITCODE_SQUAD",  CMD_WAITCODE_SQUADWAIT);
	LuaPushNamedNumber(L, "WAITCODE_GATHER", CMD_WAITCODE_GATHERWAIT);

#define PUSH_CMD(cmd) LuaInsertDualMapPair(L, #cmd, CMD_ ## cmd);

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
	PUSH_CMD(LOAD_UNITS);
	PUSH_CMD(LOAD_ONTO);
	PUSH_CMD(UNLOAD_UNITS);
	PUSH_CMD(UNLOAD_UNIT);
	PUSH_CMD(ONOFF);
	PUSH_CMD(RECLAIM);
	PUSH_CMD(CLOAK);
	PUSH_CMD(STOCKPILE);
	PUSH_CMD(MANUALFIRE);
	LuaInsertDualMapPair(L, "DGUN", CMD_MANUALFIRE); // backward compability (TODO: find a way to print a warning when used!)
	PUSH_CMD(RESTORE);
	PUSH_CMD(REPEAT);
	PUSH_CMD(TRAJECTORY);
	PUSH_CMD(RESURRECT);
	PUSH_CMD(CAPTURE);
	PUSH_CMD(AUTOREPAIRLEVEL);
	LuaInsertDualMapPair(L, "LOOPBACKATTACK", CMD_ATTACK); // backward compability (TODO: find a way to print a warning when used!)
	PUSH_CMD(IDLEMODE);

	return true;
}


/******************************************************************************/
/******************************************************************************/
