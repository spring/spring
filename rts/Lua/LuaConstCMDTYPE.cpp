/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaConstCMDTYPE.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "Sim/Units/CommandAI/Command.h"


/******************************************************************************/
/******************************************************************************/


bool LuaConstCMDTYPE::PushEntries(lua_State* L)
{
#define PUSH_CMDTYPE(cmd) LuaInsertDualMapPair(L, #cmd, CMDTYPE_ ## cmd)

	PUSH_CMDTYPE(ICON);
	PUSH_CMDTYPE(ICON_MODE);
	PUSH_CMDTYPE(ICON_MAP);
	PUSH_CMDTYPE(ICON_AREA);
	PUSH_CMDTYPE(ICON_UNIT);
	PUSH_CMDTYPE(ICON_UNIT_OR_MAP);
	PUSH_CMDTYPE(ICON_FRONT);
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
