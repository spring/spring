/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaRules.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "LuaObjectRendering.h"
#include "LuaCallInCheck.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Scripts/CobInstance.h" // for UNPACK{X,Z}
#include "System/Log/ILog.h"
#include "System/FileSystem/VFSModes.h" // for SPRING_VFS_*
#include "System/Threading/SpringThreading.h"

#include <cassert>

CLuaRules* luaRules = nullptr;

static const char* LuaRulesSyncedFilename   = "LuaRules/main.lua";
static const char* LuaRulesUnsyncedFilename = "LuaRules/draw.lua";

const int* CLuaRules::currentCobArgs = nullptr;


/******************************************************************************/
/******************************************************************************/

static spring::mutex m_singleton;

DECL_LOAD_SPLIT_HANDLER(CLuaRules, luaRules)
DECL_FREE_HANDLER(CLuaRules, luaRules)


/******************************************************************************/
/******************************************************************************/

CLuaRules::CLuaRules(bool onlySynced): CSplitLuaHandle("LuaRules", LUA_HANDLE_ORDER_RULES)
{
	currentCobArgs = nullptr;

	if (!IsValid())
		return;

	Init(onlySynced);
}

CLuaRules::~CLuaRules()
{
	luaRules = nullptr;
	currentCobArgs = nullptr;
}


std::string CLuaRules::GetUnsyncedFileName() const
{
	return LuaRulesUnsyncedFilename;
}

std::string CLuaRules::GetSyncedFileName() const
{
	return LuaRulesSyncedFilename;
}

std::string CLuaRules::GetInitFileModes() const
{
	return SPRING_VFS_MOD_BASE;
}

int CLuaRules::GetInitSelectTeam() const
{
	return CEventClient::AllAccessTeam;
}


bool CLuaRules::AddSyncedCode(lua_State* L)
{
	lua_getglobal(L, "Script");
	LuaPushNamedCFunc(L, "PermitHelperAIs", PermitHelperAIs);
	lua_pop(L, 1);

	return true;
}


bool CLuaRules::AddUnsyncedCode(lua_State* L)
{
	lua_getglobal(L, "Spring");

	lua_pushliteral(L, "UnitRendering");
	lua_newtable(L);
	LuaObjectRendering<LUAOBJ_UNIT>::PushEntries(L);
	lua_rawset(L, -3);

	lua_pushliteral(L, "FeatureRendering");
	lua_newtable(L);
	LuaObjectRendering<LUAOBJ_FEATURE>::PushEntries(L);
	lua_rawset(L, -3);

	lua_pop(L, 1); // Spring

	return true;
}


/******************************************************************************/
/******************************************************************************/

int CLuaRules::UnpackCobArg(lua_State* L)
{
	if (currentCobArgs == nullptr) {
		luaL_error(L, "Error in UnpackCobArg(), no current args");
	}
	const int arg = luaL_checkint(L, 1) - 1;
	if ((arg < 0) || (arg >= MAX_LUA_COB_ARGS)) {
		luaL_error(L, "Error in UnpackCobArg(), bad index");
	}
	const int value = currentCobArgs[arg];
	lua_pushnumber(L, UNPACKX(value));
	lua_pushnumber(L, UNPACKZ(value));
	return 2;
}


void CLuaRules::Cob2Lua(const LuaHashString& name, const CUnit* unit,
                        int& argsCount, int args[MAX_LUA_COB_ARGS])
{
	static int callDepth = 0;
	if (callDepth >= 16) {
		LOG_L(L_WARNING, "[LuaRules::%s] call overflow: %s", __func__, name.GetString());
		args[0] = 0; // failure
		return;
	}

	auto L = syncedLuaHandle.L;

	LUA_CALL_IN_CHECK(L);

	const int top = lua_gettop(L);

	if (!lua_checkstack(L, 1 + 3 + argsCount)) {
		LOG_L(L_WARNING, "[LuaRules::%s] lua_checkstack() error: %s", __func__, name.GetString());
		args[0] = 0; // failure
		lua_settop(L, top);
		return;
	}

	if (!name.GetGlobalFunc(L)) {
		LOG_L(L_WARNING, "[LuaRules::%s] missing function: %s", __func__, name.GetString());
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
	const int* oldArgs = currentCobArgs;
	currentCobArgs = args;

	const bool error = !syncedLuaHandle.RunCallIn(L, name, 3 + argsCount, LUA_MULTRET);

	currentCobArgs = oldArgs;
	callDepth--;

	// bail on error
	if (error) {
		args[0] = 0; // failure
		lua_settop(L, top);
		return;
	}

	// get the results
	const int retArgs = std::min(lua_gettop(L) - top, (MAX_LUA_COB_ARGS - 1));
	for (int a = 1; a <= retArgs; a++) {
		const int index = (a + top);
		if (lua_isnumber(L, index)) {
			args[a] = lua_toint(L, index);
		}
		else if (lua_isboolean(L, index)) {
			args[a] = lua_toboolean(L, index) ? 1 : 0;
		}
		else if (lua_istable(L, index)) {
			lua_rawgeti(L, index, 1);
			lua_rawgeti(L, index, 2);
			if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
				const int x = lua_toint(L, -2);
				const int z = lua_toint(L, -1);
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
}


/******************************************************************************/
/******************************************************************************/
//
// LuaRules Call-Outs
//

int CLuaRules::PermitHelperAIs(lua_State* L)
{
	if (!lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect argument to PermitHelperAIs()");
	}
	gs->noHelperAIs = !lua_toboolean(L, 1);
	LOG("LuaRules has %s helper AIs",
			(gs->noHelperAIs ? "disabled" : "enabled"));
	return 0;
}

/******************************************************************************/
/******************************************************************************/
