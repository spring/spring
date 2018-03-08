/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaGaia.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/FileSystem/FileHandler.h" // SPRING_VFS*
#include "System/FileSystem/VFSHandler.h"
#include "System/StringUtil.h"
#include "System/Threading/SpringThreading.h"


CLuaGaia* luaGaia = nullptr;

static const char* LuaGaiaSyncedFilename   = "LuaGaia/main.lua";
static const char* LuaGaiaUnsyncedFilename = "LuaGaia/draw.lua";


/******************************************************************************/
/******************************************************************************/

static spring::mutex m_singleton;

DECL_LOAD_HANDLER(CLuaGaia, luaGaia)
DECL_FREE_HANDLER(CLuaGaia, luaGaia)


/******************************************************************************/
/******************************************************************************/

CLuaGaia::CLuaGaia(): CLuaHandleSynced("LuaGaia", LUA_HANDLE_ORDER_GAIA)
{
	if (!IsValid())
		return;

	SetFullCtrl(true);
	SetFullRead(true);
	SetCtrlTeam(CEventClient::AllAccessTeam);
	SetReadTeam(CEventClient::AllAccessTeam);
	SetReadAllyTeam(CEventClient::AllAccessTeam);
	SetSelectTeam(teamHandler.GaiaTeamID());

	Init(LuaGaiaSyncedFilename, LuaGaiaUnsyncedFilename, SPRING_VFS_MAP_BASE);
}

CLuaGaia::~CLuaGaia()
{
	luaGaia = nullptr;
}


bool CLuaGaia::CanLoadHandler()
{
	return (gs->useLuaGaia && (vfsHandler->FileExists(LuaGaiaSyncedFilename, CVFSHandler::Map) || vfsHandler->FileExists(LuaGaiaUnsyncedFilename, CVFSHandler::Map)));
}

