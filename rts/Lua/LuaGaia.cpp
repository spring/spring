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

DECL_LOAD_SPLIT_HANDLER(CLuaGaia, luaGaia)
DECL_FREE_HANDLER(CLuaGaia, luaGaia)


/******************************************************************************/
/******************************************************************************/

CLuaGaia::CLuaGaia(bool onlySynced): CSplitLuaHandle("LuaGaia", LUA_HANDLE_ORDER_GAIA)
{
	if (!IsValid())
		return;

	Init(onlySynced);
}

CLuaGaia::~CLuaGaia()
{
	luaGaia = nullptr;
}


std::string CLuaGaia::GetUnsyncedFileName() const
{
	return LuaGaiaUnsyncedFilename;
}

std::string CLuaGaia::GetSyncedFileName() const
{
	return LuaGaiaSyncedFilename;
}

std::string CLuaGaia::GetInitFileModes() const
{
	return SPRING_VFS_MAP_BASE;
}

int CLuaGaia::GetInitSelectTeam() const
{
	return teamHandler.GaiaTeamID();
}


bool CLuaGaia::CanLoadHandler()
{
	return (gs->useLuaGaia && (vfsHandler->FileExists(LuaGaiaSyncedFilename, CVFSHandler::Map) == 1 || vfsHandler->FileExists(LuaGaiaUnsyncedFilename, CVFSHandler::Map) == 1));
}

