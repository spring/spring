/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <set>
#include <cctype>

#include "mmgr.h"

#include "LuaGaia.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "LuaSyncedCtrl.h"
#include "LuaSyncedRead.h"
#include "LuaUnitDefs.h"
#include "LuaWeaponDefs.h"
#include "LuaOpenGL.h"

#include "Rendering/UnitDrawer.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/Command.h"
#include "LogOutput.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"
#include "Util.h"


CLuaGaia* luaGaia = NULL;

static const char* LuaGaiaSyncedFilename   = "LuaGaia/main.lua";
static const char* LuaGaiaUnsyncedFilename = "LuaGaia/draw.lua";


/******************************************************************************/
/******************************************************************************/

void CLuaGaia::LoadHandler()
{
	if (luaGaia) {
		return;
	}

	new CLuaGaia();

	if (!luaGaia->IsValid()) {
		delete luaGaia;
	}
}


void CLuaGaia::FreeHandler()
{
	delete luaGaia;
}


/******************************************************************************/
/******************************************************************************/

CLuaGaia::CLuaGaia()
: CLuaHandleSynced("LuaGaia", LUA_HANDLE_ORDER_GAIA)
{
	luaGaia = this;

	if (!IsValid()) {
		return;
	}

	teamsLocked = true;

	SetFullCtrl(true, true);
	SetFullRead(true, true);
	SetCtrlTeam(AllAccessTeam, true); //teamHandler->GaiaTeamID();
	SetReadTeam(AllAccessTeam, true);
	SetReadAllyTeam(AllAccessTeam, true);
	SetSelectTeam(teamHandler->GaiaTeamID(), true);

	Init(LuaGaiaSyncedFilename, LuaGaiaUnsyncedFilename, SPRING_VFS_MAP);
}


CLuaGaia::~CLuaGaia()
{
	if (L_Sim != NULL || L_Draw != NULL) {
		Shutdown();
		KillLua();
	}
	luaGaia = NULL;
}


bool CLuaGaia::AddSyncedCode(lua_State *L)
{
	return true;
}


bool CLuaGaia::AddUnsyncedCode(lua_State *L)
{
	/*lua_pushstring(L, "UNSYNCED");
	lua_gettable(L, LUA_REGISTRYINDEX);*/

	return true;
}

/******************************************************************************/
/******************************************************************************/
