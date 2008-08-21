#include "StdAfx.h"
// LuaGaia.cpp: implementation of the CLuaGaia class.
//
//////////////////////////////////////////////////////////////////////

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

#include "Sim/Units/CommandAI/Command.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/FileSystem.h"


CLuaGaia* luaGaia = NULL;

string CLuaGaia::configString;

static const char* LuaGaiaSyncedFilename   = "LuaGaia/main.lua";
static const char* LuaGaiaUnsyncedFilename = "LuaGaia/draw.lua";


/******************************************************************************/
/******************************************************************************/

void CLuaGaia::LoadHandler()
{
	if (luaGaia) {
		return;
	}

	SAFE_NEW CLuaGaia();

	if (luaGaia->L == NULL) {
		delete luaGaia;
	}
}


void CLuaGaia::FreeHandler()
{
	delete luaGaia;
}


bool CLuaGaia::SetConfigString(const string& cfg)
{
	configString = cfg;
	if ((cfg == "0") || (cfg == "disabled")) {
		return false;
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/

CLuaGaia::CLuaGaia()
: CLuaHandleSynced("LuaGaia", LUA_HANDLE_ORDER_GAIA, CobCallback, ".luagaia ")
{
	luaGaia = this;

	if (L == NULL) {
		return;
	}

	teamsLocked = true;

	fullCtrl = false;
	fullRead = false;
	ctrlTeam = gs->gaiaTeamID;
	readTeam = gs->gaiaTeamID;
	readAllyTeam = gs->gaiaAllyTeamID;
	selectTeam = gs->gaiaTeamID;

	Init(LuaGaiaSyncedFilename, LuaGaiaUnsyncedFilename, SPRING_VFS_MAP);
}


CLuaGaia::~CLuaGaia()
{
	if (L != NULL) {
		Shutdown();
		KillLua();
	}
	luaGaia = NULL;
}


bool CLuaGaia::AddSyncedCode()
{
	lua_getglobal(L, "Script");
	LuaPushNamedCFunc(L, "GetConfigString", GetConfigString);
	lua_pop(L, 1);

	return true;
}


bool CLuaGaia::AddUnsyncedCode()
{
	lua_pushstring(L, "UNSYNCED");
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushstring(L, "Script");
	lua_rawget(L, -2);
	LuaPushNamedCFunc(L, "GetConfigString", GetConfigString);
	lua_pop(L, 1);

	return true;
}


/******************************************************************************/

void CLuaGaia::CobCallback(int retCode, void* p1, void* p2)
{
	if (luaGaia) {
		CobCallbackData cbd(retCode, *((int*)&p1), *((float*)&p2));
		luaGaia->cobCallbackEntries.push_back(cbd);
	}
}


/******************************************************************************/

int CLuaGaia::GetConfigString(lua_State* L)
{
	lua_pushlstring(L, configString.c_str(), configString.size());
	return 1;
}


/******************************************************************************/
/******************************************************************************/
