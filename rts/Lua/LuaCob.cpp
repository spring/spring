#include "StdAfx.h"
// LuaCob.cpp: implementation of the CLuaCob class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaCob.h"
#include <set>
#include <cctype>

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
#include "Sim/Units/COB/CobInstance.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/FileSystem.h"


CLuaCob* luaCob = NULL;

static const char* LuaCobSyncedFilename   = "LuaCob/main.lua";
static const char* LuaCobUnsyncedFilename = "LuaCob/draw.lua";

const int* CLuaCob::currentArgs = NULL;


/******************************************************************************/
/******************************************************************************/

void CLuaCob::LoadHandler()
{
	if (luaCob) {
		return;
	}

	SAFE_NEW CLuaCob();

	if (luaCob->L == NULL) {
		delete luaCob;
	}
}


void CLuaCob::FreeHandler()
{
	delete luaCob;
}


/******************************************************************************/
/******************************************************************************/

CLuaCob::CLuaCob()
: CLuaHandleSynced("LuaCob", LUA_HANDLE_ORDER_COB, CobCallback, ".luacob ")
{
	luaCob = this;

	if (L == NULL) {
		return;
	}

	fullCtrl = true;
	fullRead = true;
	ctrlTeam = AllAccessTeam;
	readTeam = AllAccessTeam;
	readAllyTeam = AllAccessTeam;
	selectTeam = AllAccessTeam;

	Init(LuaCobSyncedFilename, LuaCobUnsyncedFilename, SPRING_VFS_MOD);
}


CLuaCob::~CLuaCob()
{
	if (L != NULL) {
		Shutdown();
		KillLua();
	}
	luaCob = NULL;
}


bool CLuaCob::AddSyncedCode()
{
	lua_getglobal(L, "Spring");
	LuaPushNamedCFunc(L, "UnpackCobArg", UnpackCobArg);
	lua_pop(L, 1);
	return true;
}


/******************************************************************************/

int CLuaCob::UnpackCobArg(lua_State* L)
{
	if (currentArgs == NULL) {
		luaL_error(L, "Error in UnpackCobArg(), no current args");
	}
	const int arg = (int)luaL_checknumber(L, 1) - 1;
	if ((arg < 0) || (arg >= MAX_LUA_COB_ARGS)) {
		luaL_error(L, "Error in UnpackCobArg(), bad index");
	}
	const int value = currentArgs[arg];
	lua_pushnumber(L, UNPACKX(value));
	lua_pushnumber(L, UNPACKZ(value));
	return 2;
}


void CLuaCob::CobCallback(int retCode, void* p1, void* p2)
{
	if (luaCob) {
		CobCallbackData cbd(retCode, *((int*)&p1), *((float*)&p2));
		luaCob->cobCallbackEntries.push_back(cbd);
	}
}


/******************************************************************************/
/******************************************************************************/
//
// LuaCob Call-Ins
//

void CLuaCob::CallFunction(const LuaHashString& name, const CUnit* unit,
                           int& argsCount, int args[MAX_LUA_COB_ARGS])
{
	static int callDepth = 0;
	if (callDepth >= 16) {
		logOutput.Print("CLuaCob::CallFunction() call overflow: %s\n",
		                name.GetString().c_str());
		args[0] = 0; // failure
		return;
	}

	if (!lua_checkstack(L, argsCount + 4)) {
		logOutput.Print("CLuaCob::CallFunction() lua_checkstack() error: %s\n",
		                name.GetString().c_str());
		args[0] = 0; // failure
		return;
	}

	const int top = lua_gettop(L);

	if (!name.GetGlobalFunc(L)) {
		logOutput.Print("CLuaCob::CallFunction() missing function: %s\n",
		                name.GetString().c_str());
		args[0] = 0; // failure
		lua_settop(L, top);
		return;
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	for (int a = 0; a < argsCount; a++) {
		lua_pushnumber(L, args[a]);
	}

	// call the routine
	callDepth++;
	const int* oldArgs = currentArgs;
	currentArgs = args;

	const bool error = !RunCallIn(name, 3 + argsCount, LUA_MULTRET);

	currentArgs = oldArgs;
	callDepth--;

	// bail on error
	if (error) {
		args[0] = 0; // failure
		lua_settop(L, top);
		return;
	}

	// get the results
	const int retArgs = min(lua_gettop(L), (MAX_LUA_COB_ARGS - 1));
	for (int a = 1; a <= retArgs; a++) {
		if (lua_isnumber(L, a)) {
			args[a] = (int)lua_tonumber(L, a);
		}
		else if (lua_isboolean(L, a)) {
			args[a] = lua_toboolean(L, a) ? 1 : 0;
		}
		else if (lua_istable(L, a)) {
			lua_rawgeti(L, a, 1);
			lua_rawgeti(L, a, 2);
			if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
				const int x = (int)lua_tonumber(L, -2);
				const int z = (int)lua_tonumber(L, -1);
				args[a] = PACKXZ(x, z);
			} else {
				args[a] = 0;
			}
			lua_pop(L, 2);
		}
		else {
			args[a] = 0;
		}
	}

	args[0] = 1; // success
	lua_settop(L, top);
	return;
}


/******************************************************************************/
/******************************************************************************/
