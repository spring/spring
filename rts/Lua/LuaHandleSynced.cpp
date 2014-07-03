/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaHandleSynced.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "LuaCallInCheck.h"
#include "LuaConstGL.h"
#include "LuaConstCMD.h"
#include "LuaConstCMDTYPE.h"
#include "LuaConstCOB.h"
#include "LuaConstGame.h"
#include "LuaInterCall.h"
#include "LuaSyncedCtrl.h"
#include "LuaSyncedRead.h"
#include "LuaSyncedTable.h"
#include "LuaUnsyncedCtrl.h"
#include "LuaUnsyncedRead.h"
#include "LuaFeatureDefs.h"
#include "LuaUnitDefs.h"
#include "LuaWeaponDefs.h"
#include "LuaScream.h"
#include "LuaOpenGL.h"
#include "LuaVFS.h"
#include "LuaZip.h"

#include "Game/Game.h"
#include "Game/WordCompletion.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/Scripts/LuaUnitScript.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"



LuaRulesParams::Params  CLuaHandleSynced::gameParams;
LuaRulesParams::HashMap CLuaHandleSynced::gameParamsMap;



/******************************************************************************/
/******************************************************************************/
// ##     ## ##    ##  ######  ##    ## ##    ##  ######  ######## ########
// ##     ## ###   ## ##    ##  ##  ##  ###   ## ##    ## ##       ##     ##
// ##     ## ####  ## ##         ####   ####  ## ##       ##       ##     ##
// ##     ## ## ## ##  ######     ##    ## ## ## ##       ######   ##     ##
// ##     ## ##  ####       ##    ##    ##  #### ##       ##       ##     ##
// ##     ## ##   ### ##    ##    ##    ##   ### ##    ## ##       ##     ##
//  #######  ##    ##  ######     ##    ##    ##  ######  ######## ########

CUnsyncedLuaHandle::CUnsyncedLuaHandle(CLuaHandleSynced* _base, const string& _name, int _order)
	: CLuaHandle(_name, _order, false)
	, base(*_base)
{
	D.synced = false;
	D.allowChanges = false;
}


CUnsyncedLuaHandle::~CUnsyncedLuaHandle()
{
}


bool CUnsyncedLuaHandle::Init(const string& code, const string& file)
{
	if (!IsValid()) {
		return false;
	}

	// load the standard libraries
	LUA_OPEN_LIB(L, luaopen_base);
	LUA_OPEN_LIB(L, luaopen_math);
	LUA_OPEN_LIB(L, luaopen_table);
	LUA_OPEN_LIB(L, luaopen_string);
	//LUA_OPEN_LIB(L, luaopen_io);
	//LUA_OPEN_LIB(L, luaopen_os);
	//LUA_OPEN_LIB(L, luaopen_package);
	//LUA_OPEN_LIB(L, luaopen_debug);

	// delete some dangerous functions
	lua_pushnil(L); lua_setglobal(L, "dofile");
	lua_pushnil(L); lua_setglobal(L, "loadfile");
	lua_pushnil(L); lua_setglobal(L, "loadlib");
	lua_pushnil(L); lua_setglobal(L, "loadstring"); // replaced
	lua_pushnil(L); lua_setglobal(L, "require");

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	AddBasicCalls(L);

	// remove Script.Kill()
	lua_getglobal(L, "Script");
		LuaPushNamedNil(L, "Kill");
	lua_pop(L, 1);

	LuaPushNamedCFunc(L, "loadstring", CLuaHandleSynced::LoadStringData);
	LuaPushNamedCFunc(L, "CallAsTeam", CLuaHandleSynced::CallAsTeam);
	LuaPushNamedNumber(L, "COBSCALE",  COBSCALE);

	// load our libraries
	if (!LuaSyncedTable::PushEntries(L)                                    ||
	    !AddEntriesToTable(L, "VFS",         LuaVFS::PushUnsynced)         ||
	    !AddEntriesToTable(L, "VFS",         LuaZipFileReader::PushUnsynced) ||
	    !AddEntriesToTable(L, "VFS",         LuaZipFileWriter::PushUnsynced) ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
	    !AddEntriesToTable(L, "Script",      LuaInterCall::PushEntriesUnsynced) ||
	    !AddEntriesToTable(L, "Script",      LuaScream::PushEntries)       ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedRead::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedCtrl::PushEntries) ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedRead::PushEntries) ||
	    !AddEntriesToTable(L, "gl",          LuaOpenGL::PushEntries)       ||
	    !AddEntriesToTable(L, "GL",          LuaConstGL::PushEntries)      ||
	    !AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)    ||
	    !AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)     ||
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries) ||
	    !AddEntriesToTable(L, "LOG",         LuaUtils::PushLogEntries)
	) {
		KillLua();
		return false;
	}

	lua_settop(L, 0);

	// add code from the sub-class
	if (!base.AddUnsyncedCode(L)) {
		KillLua();
		return false;
	}

	lua_settop(L, 0);
	if (!LoadCode(L, code, file)) {
		KillLua();
		return false;
	}

	lua_settop(L, 0);
	eventHandler.AddClient(this);
	return true;
}


//
// Call-Ins
//

void CUnsyncedLuaHandle::RecvFromSynced(lua_State* srcState, int args)
{
	if (!IsValid())
		return;


	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2 + args, __FUNCTION__);

	static const LuaHashString cmdStr("RecvFromSynced");
	if (!cmdStr.GetGlobalFunc(L))
		return; // the call is not defined

	LuaUtils::CopyData(L, srcState, args);

	// call the routine
	RunCallIn(L, cmdStr, args, 0);
}


bool CUnsyncedLuaHandle::DrawUnit(const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	const bool oldDrawState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, game->GetDrawMode());

	const bool success = RunCallIn(L, cmdStr, 2, 1);
	LuaOpenGL::SetDrawingEnabled(L, oldDrawState);

	if (!success)
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CUnsyncedLuaHandle::DrawFeature(const CFeature* feature)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	const bool oldDrawState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, game->GetDrawMode());

	const bool success = RunCallIn(L, cmdStr, 2, 1);
	LuaOpenGL::SetDrawingEnabled(L, oldDrawState);

	if (!success)
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CUnsyncedLuaHandle::DrawShield(const CUnit* unit, const CWeapon* weapon)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	const bool oldDrawState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, weapon->weaponNum);
	lua_pushnumber(L, game->GetDrawMode());

	const bool success = RunCallIn(L, cmdStr, 3, 1);
	LuaOpenGL::SetDrawingEnabled(L, oldDrawState);

	if (!success)
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CUnsyncedLuaHandle::DrawProjectile(const CProjectile* projectile)
{
	if (!(projectile->weapon || projectile->piece))
		return false;

	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	const bool oldDrawState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	lua_pushnumber(L, projectile->id);
	lua_pushnumber(L, game->GetDrawMode());

	const bool success = RunCallIn(L, cmdStr, 2, 1);
	LuaOpenGL::SetDrawingEnabled(L, oldDrawState);

	if (!success)
		return false;

	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


//
// Call-Outs
//


/******************************************************************************/
/******************************************************************************/
//  ######  ##    ## ##    ##  ######  ######## ########
// ##    ##  ##  ##  ###   ## ##    ## ##       ##     ##
// ##         ####   ####  ## ##       ##       ##     ##
//  ######     ##    ## ## ## ##       ######   ##     ##
//       ##    ##    ##  #### ##       ##       ##     ##
// ##    ##    ##    ##   ### ##    ## ##       ##     ##
//  ######     ##    ##    ##  ######  ######## ########

CSyncedLuaHandle::CSyncedLuaHandle(CLuaHandleSynced* _base, const string& _name, int _order)
	: CLuaHandle(_name, _order, false)
	, base(*_base)
	, origNextRef(-1)
{
	D.synced = true;
	D.allowChanges = true;
}


CSyncedLuaHandle::~CSyncedLuaHandle()
{
	// kill all unitscripts running in this handle
	CLuaUnitScript::HandleFreed(this);
}


bool CSyncedLuaHandle::Init(const string& code, const string& file)
{
	if (!IsValid())
		return false;

	watchUnitDefs.resize(unitDefHandler->unitDefs.size() + 1, false);
	watchFeatureDefs.resize(featureHandler->GetFeatureDefs().size(), false);
	watchWeaponDefs.resize(weaponDefHandler->weaponDefs.size(), false);

	// load the standard libraries
	LUA_OPEN_LIB(L, luaopen_base);
	LUA_OPEN_LIB(L, luaopen_math);
	LUA_OPEN_LIB(L, luaopen_table);
	LUA_OPEN_LIB(L, luaopen_string);
	//LUA_OPEN_LIB(L, luaopen_io);
	//LUA_OPEN_LIB(L, luaopen_os);
	//LUA_OPEN_LIB(L, luaopen_package);
	//LUA_OPEN_LIB(L, luaopen_debug);

	lua_getglobal(L, "next");
	origNextRef = luaL_ref(L, LUA_REGISTRYINDEX);

	// delete/replace some dangerous functions
	lua_pushnil(L); lua_setglobal(L, "dofile");
	lua_pushnil(L); lua_setglobal(L, "loadfile");
	lua_pushnil(L); lua_setglobal(L, "loadlib");
	lua_pushnil(L); lua_setglobal(L, "require");
	lua_pushnil(L); lua_setglobal(L, "rawequal"); //FIXME not unsafe anymore since split?
	lua_pushnil(L); lua_setglobal(L, "rawget"); //FIXME not unsafe anymore since split?
	lua_pushnil(L); lua_setglobal(L, "rawset"); //FIXME not unsafe anymore since split?
//	lua_pushnil(L); lua_setglobal(L, "getfenv");
//	lua_pushnil(L); lua_setglobal(L, "setfenv");
	lua_pushnil(L); lua_setglobal(L, "newproxy"); // sync unsafe cause of __gc
	lua_pushnil(L); lua_setglobal(L, "gcinfo");
	lua_pushnil(L); lua_setglobal(L, "collectgarbage");

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	LuaPushNamedCFunc(L, "loadstring", CLuaHandleSynced::LoadStringData);
	LuaPushNamedCFunc(L, "pairs", SyncedPairs);
	LuaPushNamedCFunc(L, "next",  SyncedNext);
	lua_pop(L, 1);

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	AddBasicCalls(L); // into Global

	// adjust the math.random() and math.randomseed() calls
	lua_getglobal(L, "math");
		LuaPushNamedCFunc(L, "random", SyncedRandom);
		LuaPushNamedCFunc(L, "randomseed", SyncedRandomSeed);
	lua_pop(L, 1); // pop the global math table

	lua_getglobal(L, "Script");
		LuaPushNamedCFunc(L, "AddActionFallback",    AddSyncedActionFallback);
		LuaPushNamedCFunc(L, "RemoveActionFallback", RemoveSyncedActionFallback);
		LuaPushNamedCFunc(L, "GetWatchUnit",         GetWatchUnitDef);
		LuaPushNamedCFunc(L, "SetWatchUnit",         SetWatchUnitDef);
		LuaPushNamedCFunc(L, "GetWatchFeature",      GetWatchFeatureDef);
		LuaPushNamedCFunc(L, "SetWatchFeature",      SetWatchFeatureDef);
		LuaPushNamedCFunc(L, "GetWatchWeapon",       GetWatchWeaponDef);
		LuaPushNamedCFunc(L, "SetWatchWeapon",       SetWatchWeaponDef);
	lua_pop(L, 1);

	// add the custom file loader
	LuaPushNamedCFunc(L, "SendToUnsynced", SendToUnsynced);
	LuaPushNamedCFunc(L, "CallAsTeam",     CLuaHandleSynced::CallAsTeam);
	LuaPushNamedNumber(L, "COBSCALE",      COBSCALE);

	// load our libraries  (LuaSyncedCtrl overrides some LuaUnsyncedCtrl entries)
	if (
		!AddEntriesToTable(L, "VFS",         LuaVFS::PushSynced)           ||
		!AddEntriesToTable(L, "VFS",         LuaZipFileReader::PushSynced) ||
		!AddEntriesToTable(L, "VFS",         LuaZipFileWriter::PushSynced) ||
		!AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
		!AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
		!AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
		!AddEntriesToTable(L, "Script",      LuaInterCall::PushEntriesSynced)   ||
		!AddEntriesToTable(L, "Spring",      LuaUnsyncedCtrl::PushEntries) ||
		!AddEntriesToTable(L, "Spring",      LuaSyncedCtrl::PushEntries)   ||
		!AddEntriesToTable(L, "Spring",      LuaSyncedRead::PushEntries)   ||
		!AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)    ||
		!AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)     ||
		!AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries) ||
		!AddEntriesToTable(L, "COB",         LuaConstCOB::PushEntries)     ||
		!AddEntriesToTable(L, "SFX",         LuaConstSFX::PushEntries)     ||
		!AddEntriesToTable(L, "LOG",         LuaUtils::PushLogEntries)
	) {
		KillLua();
		return false;
	}

	// add code from the sub-class
	if (!base.AddSyncedCode(L)) {
		KillLua();
		return false;
	}

	lua_settop(L, 0);
	if (!LoadCode(L, code, file)) {
		KillLua();
		return false;
	}

	lua_settop(L, 0);
	eventHandler.AddClient(this);
	return true;
}


//
// Call-Ins
//

bool CSyncedLuaHandle::SyncedActionFallback(const string& msg, int playerID)
{
	string cmd = msg;
	const string::size_type pos = cmd.find_first_of(" \t");
	if (pos != string::npos) {
		cmd.resize(pos);
	}
	if (textCommands.find(cmd) == textCommands.end()) {
		return false;
	}
	string msg_ = msg.substr(1); // strip the '/'
	return GotChatMsg(msg_, playerID);
}


/// pushes 7 items on the stack
static void PushUnitAndCommand(lua_State* L, const CUnit* unit, const Command& cmd)
{
	// push the unit info
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// push the command id
	lua_pushnumber(L, cmd.GetID());

	// push the params list
	lua_newtable(L);
	for (int p = 0; p < (int)cmd.params.size(); p++) {
		lua_pushnumber(L, cmd.params[p]);
		lua_rawseti(L, -2, p + 1);
	}

	// push the options table
	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "coded", cmd.options);
	HSTR_PUSH_BOOL(L, "alt",   !!(cmd.options & ALT_KEY));
	HSTR_PUSH_BOOL(L, "ctrl",  !!(cmd.options & CONTROL_KEY));
	HSTR_PUSH_BOOL(L, "shift", !!(cmd.options & SHIFT_KEY));
	HSTR_PUSH_BOOL(L, "right", !!(cmd.options & RIGHT_MOUSE_KEY));
	HSTR_PUSH_BOOL(L, "meta",  !!(cmd.options & META_KEY));

	// push the command tag
	lua_pushnumber(L, cmd.tag);
}

bool CSyncedLuaHandle::CommandFallback(const CUnit* unit, const Command& cmd)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 9, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	PushUnitAndCommand(L, unit, cmd);

	// call the function
	if (!RunCallIn(L, cmdStr, 7, 1))
		return true;

	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval; // return 'true' to remove the command
}


bool CSyncedLuaHandle::AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 10, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	PushUnitAndCommand(L, unit, cmd);

	lua_pushboolean(L, fromSynced);

	// call the function
	if (!RunCallIn(L, cmdStr, 8, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowUnitCreation(const UnitDef* unitDef,
                                  const CUnit* builder, const BuildInfo* buildInfo)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 9, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, unitDef->id);
	lua_pushnumber(L, builder->id);
	lua_pushnumber(L, builder->team);

	if (buildInfo != NULL) {
		lua_pushnumber(L, buildInfo->pos.x);
		lua_pushnumber(L, buildInfo->pos.y);
		lua_pushnumber(L, buildInfo->pos.z);
		lua_pushnumber(L, buildInfo->buildFacing);
	}

	// call the function
	if (!RunCallIn(L, cmdStr, (buildInfo != NULL)? 7 : 3, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}



bool CSyncedLuaHandle::AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, newTeam);
	lua_pushboolean(L, capture);

	// call the function
	if (!RunCallIn(L, cmdStr, 5, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowUnitBuildStep(const CUnit* builder,
                                   const CUnit* unit, float part)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, builder->id);
	lua_pushnumber(L, builder->team);
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, part);

	// call the function
	if (!RunCallIn(L, cmdStr, 5, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowFeatureCreation(const FeatureDef* featureDef,
                                     int teamID, const float3& pos)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, featureDef->id);
	lua_pushnumber(L, teamID);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);

	// call the function
	if (!RunCallIn(L, cmdStr, 5, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowFeatureBuildStep(const CUnit* builder,
                                      const CFeature* feature, float part)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, builder->id);
	lua_pushnumber(L, builder->team);
	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->def->id);
	lua_pushnumber(L, part);

	// call the function
	if (!RunCallIn(L, cmdStr, 5, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowResourceLevel(int teamID, const string& type, float level)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 5, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, teamID);
	lua_pushsstring(L, type);
	lua_pushnumber(L, level);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowResourceTransfer(int oldTeam, int newTeam,
                                      const string& type, float amount)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 6, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, oldTeam);
	lua_pushnumber(L, newTeam);
	lua_pushsstring(L, type);
	lua_pushnumber(L, amount);

	// call the function
	if (!RunCallIn(L, cmdStr, 4, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowDirectUnitControl(int playerID, const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 6, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, playerID);

	// call the function
	if (!RunCallIn(L, cmdStr, 4, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowBuilderHoldFire(const CUnit* unit, int action)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 3 + 1, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, action);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::AllowStartPosition(int playerID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 13, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	// push the start position and playerID
	lua_pushnumber(L, clampedPos.x);
	lua_pushnumber(L, clampedPos.y);
	lua_pushnumber(L, clampedPos.z);
	lua_pushnumber(L, playerID);
	lua_pushnumber(L, readyState);
	lua_pushnumber(L, rawPickPos.x);
	lua_pushnumber(L, rawPickPos.y);
	lua_pushnumber(L, rawPickPos.z);

	// call the function
	if (!RunCallIn(L, cmdStr, 8, 1))
		return true;

	// get the results
	const bool retval = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::MoveCtrlNotify(const CUnit* unit, int data)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 6, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return false; // the call is not defined

	// push the unit info
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, data);

	// call the function
	if (!RunCallIn(L, cmdStr, 4, 1))
		return false;

	// get the results
	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}


bool CSyncedLuaHandle::TerraformComplete(const CUnit* unit, const CUnit* build)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 8, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);



	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return false; // the call is not defined

	// push the unit info
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// push the construction info
	lua_pushnumber(L, build->id);
	lua_pushnumber(L, build->unitDef->id);
	lua_pushnumber(L, build->team);

	// call the function
	if (!RunCallInTraceback(L, cmdStr, 6, 1, traceBack.GetErrFuncIdx(), false))
		return false;

	// get the results
	const bool retval = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return retval;
}



/**
 * called after every damage modification (even HitByWeaponId)
 * but before the damage is applied
 *
 * expects two numbers returned by lua code:
 * 1st is stored under *newDamage if newDamage != NULL
 * 2nd is stored under *impulseMult if impulseMult != NULL
 */
bool CSyncedLuaHandle::UnitPreDamaged(
	const CUnit* unit,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer,
	float* newDamage,
	float* impulseMult)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 2 + 2 + 10, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	int inArgCount = 5;
	int outArgCount = 2;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, damage);
	lua_pushboolean(L, paralyzer);
	//FIXME pass impulse too?

	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, weaponDefID); inArgCount += 1;
		lua_pushnumber(L, projectileID); inArgCount += 1;

		if (attacker != NULL) {
			lua_pushnumber(L, attacker->id);
			lua_pushnumber(L, attacker->unitDef->id);
			lua_pushnumber(L, attacker->team);
			inArgCount += 3;
		}
	}

	// call the routine
	// NOTE:
	//   RunCallInTraceback removes the error-handler by default
	//   this has to be disabled when using ScopedDebugTraceBack
	//   or it would mess up the stack
	if (!RunCallInTraceback(L, cmdStr, inArgCount, outArgCount, traceBack.GetErrFuncIdx(), false))
		return false;

	if (newDamage && lua_isnumber(L, -2)) {
		*newDamage = lua_tonumber(L, -2);
	} else if (!lua_isnumber(L, -2) || lua_isnil(L, -2)) {
		// first value is obligatory, so may not be nil
		LOG_L(L_WARNING, "%s(): 1st return-value should be a number (newDamage)", (cmdStr.GetString()).c_str());
	}

	if (impulseMult && lua_isnumber(L, -1)) {
		*impulseMult = lua_tonumber(L, -1);
	} else if (!lua_isnumber(L, -1) && !lua_isnil(L, -1)) {
		// second value is optional, so nils are OK
		LOG_L(L_WARNING, "%s(): 2nd return-value should be a number (impulseMult)", (cmdStr.GetString()).c_str());
	}

	lua_pop(L, outArgCount);
	return (*newDamage == 0.f && *impulseMult == 0.f); // returns true to disable engine dmg handling
}

bool CSyncedLuaHandle::FeaturePreDamaged(
	const CFeature* feature,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	float* newDamage,
	float* impulseMult)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 2 + 9 + 2, __FUNCTION__);

	static const LuaHashString cmdStr(__FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	int inArgCount = 4;
	int outArgCount = 2;

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->def->id);
	lua_pushnumber(L, feature->team);
	lua_pushnumber(L, damage);

	if (GetHandleFullRead(L)) {
		lua_pushnumber(L, weaponDefID); inArgCount += 1;
		lua_pushnumber(L, projectileID); inArgCount += 1;

		if (attacker != NULL) {
			lua_pushnumber(L, attacker->id);
			lua_pushnumber(L, attacker->unitDef->id);
			lua_pushnumber(L, attacker->team);
			inArgCount += 3;
		}
	}

	// call the routine
	if (!RunCallInTraceback(L, cmdStr, inArgCount, outArgCount, traceBack.GetErrFuncIdx(), false))
		return false;

	if (newDamage && lua_isnumber(L, -2)) {
		*newDamage = lua_tonumber(L, -2);
	} else if (!lua_isnumber(L, -2) || lua_isnil(L, -2)) {
		// first value is obligatory, so may not be nil
		LOG_L(L_WARNING, "%s(): 1st value returned should be a number (newDamage)", (cmdStr.GetString()).c_str());
	}

	if (impulseMult && lua_isnumber(L, -1)) {
		*impulseMult = lua_tonumber(L, -1);
	} else if (!lua_isnumber(L, -1) && !lua_isnil(L, -1)) {
		// second value is optional, so nils are OK
		LOG_L(L_WARNING, "%s(): 2nd value returned should be a number (impulseMult)", (cmdStr.GetString()).c_str());
	}

	lua_pop(L, outArgCount);
	return (*newDamage == 0.f && *impulseMult == 0.f); // returns true to disable engine dmg handling
}

bool CSyncedLuaHandle::ShieldPreDamaged(
	const CProjectile* projectile,
	const CWeapon* shieldEmitter,
	const CUnit* shieldCarrier,
	bool bounceProjectile
) {
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 2 + 5 + 1, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	// push the call-in arguments
	lua_pushnumber(L, projectile->id);
	lua_pushnumber(L, projectile->GetOwnerID());
	lua_pushnumber(L, shieldEmitter->weaponNum);
	lua_pushnumber(L, shieldCarrier->id);
	lua_pushboolean(L, bounceProjectile);

	// call the routine
	if (!RunCallInTraceback(L, cmdStr, 5, 1, traceBack.GetErrFuncIdx(), false))
		return false;

	// pop the return-value; must be true or false
	const bool ret = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return ret;
}


int CSyncedLuaHandle::AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID)
{
	int ret = -1;

	if (!watchWeaponDefs[attackerWeaponDefID])
		return ret;

	LUA_CALL_IN_CHECK(L, -1);
	luaL_checkstack(L, 2 + 3 + 1, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return ret;

	lua_pushnumber(L, attackerID);
	lua_pushnumber(L, attackerWeaponNum);
	lua_pushnumber(L, attackerWeaponDefID);

	if (!RunCallInTraceback(L, cmdStr, 3, 1, traceBack.GetErrFuncIdx(), false))
		return ret;

	ret = int(luaL_optboolean(L, -1, false)); //FIXME int????
	lua_pop(L, 1);
	return ret;
}


bool CSyncedLuaHandle::AllowWeaponTarget(
	unsigned int attackerID,
	unsigned int targetID,
	unsigned int attackerWeaponNum,
	unsigned int attackerWeaponDefID,
	float* targetPriority)
{
	assert(targetPriority != NULL);

	bool ret = true;

	if (!watchWeaponDefs[attackerWeaponDefID])
		return ret;

	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 5 + 2, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return ret;

	lua_pushnumber(L, attackerID);
	lua_pushnumber(L, targetID);
	lua_pushnumber(L, attackerWeaponNum);
	lua_pushnumber(L, attackerWeaponDefID);
	lua_pushnumber(L, *targetPriority);

	if (!RunCallInTraceback(L, cmdStr, 5, 2, traceBack.GetErrFuncIdx(), false))
		return ret;

	ret = luaL_optboolean(L, -2, false);

	if (lua_isnumber(L, -1)) {
		*targetPriority = lua_tonumber(L, -1);
	}

	lua_pop(L, 2);

	return ret;
}


bool CSyncedLuaHandle::AllowWeaponInterceptTarget(
	const CUnit* interceptorUnit,
	const CWeapon* interceptorWeapon,
	const CProjectile* interceptorTarget
) {
	bool ret = true;

	if (!watchWeaponDefs[interceptorWeapon->weaponDef->id])
		return ret;

	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 3 + 1, __FUNCTION__);
	const LuaUtils::ScopedDebugTraceBack traceBack(L);

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L))
		return ret;

	lua_pushnumber(L, interceptorUnit->id);
	lua_pushnumber(L, interceptorWeapon->weaponNum);
	lua_pushnumber(L, interceptorTarget->id);

	if (!RunCallInTraceback(L, cmdStr, 3, 1, traceBack.GetErrFuncIdx(), false))
		return ret;

	ret = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return ret;
}


//
// Call-Outs
//

int CSyncedLuaHandle::SyncedRandom(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args == 0) {
		lua_pushnumber(L, gs->randFloat());
	}
	else if ((args == 1) && lua_isnumber(L, 1)) {
		const int maxn = lua_toint(L, 1);
		if (maxn < 1) {
			luaL_error(L, "error: too small upper limit (%d) given to math.random(), should be >= 1 {synced}", maxn);
		}
		lua_pushnumber(L, 1 + (gs->randInt() % maxn));
	}
	else if ((args == 2) && lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
		const int lower = lua_toint(L, 1);
		const int upper = lua_toint(L, 2);
		if (lower > upper) {
			luaL_error(L, "Empty interval in math.random() {synced}");
		}
		const float diff  = (upper - lower);
		const float r = gs->randFloat(); // [0,1], not [0,1) ?
		int value = lower + (int)(r * (diff + 1));
		value = std::max(lower, std::min(upper, value));
		lua_pushnumber(L, value);
	}
	else {
		luaL_error(L, "Incorrect arguments to math.random() {synced}");
	}
	return 1;
}


int CSyncedLuaHandle::SyncedRandomSeed(lua_State* L)
{
	const int newseed = luaL_checkint(L, -1);
	gs->SetRandSeed(newseed);
	return 0;
}


int CSyncedLuaHandle::SyncedNext(lua_State* L)
{
	auto* slh = GetSyncedHandle(L);
	assert(slh->origNextRef > 0);

	const std::set<int> whiteList = {
		LUA_TSTRING,
		LUA_TNUMBER,
		LUA_TBOOLEAN,
		LUA_TNIL,
		LUA_TTHREAD //FIXME LUA_TTHREAD is normally _not_ synced safe but LUS handler needs it atm (and uses it in a safe way)
	};

	const int oldTop = lua_gettop(L);

	lua_rawgeti(L, LUA_REGISTRYINDEX, slh->origNextRef);
	lua_pushvalue(L, 1);
	if (oldTop >= 2) { lua_pushvalue(L, 2); } else { lua_pushnil(L); }
	lua_call(L, 2, LUA_MULTRET);
	const int retCount = lua_gettop(L) - oldTop;
	assert(retCount == 1 || retCount == 2);

	if (retCount >= 2) {
		const int keyType = lua_type(L, -2);
		if (whiteList.find(keyType) == whiteList.end()) {
			if (LuaUtils::PushDebugTraceback(L) > 0) {
				lua_pushfstring(L, "Iterating a table with keys of type \"%s\" in synced context!", lua_typename(L, keyType));
				lua_call(L, 1, 1);

				const auto* errMsg = lua_tostring(L, -1);
				LOG_L(L_WARNING, "%s", errMsg);
			}
			lua_pop(L, 1); // either nil or the errMsg
		}
	}

	return retCount;
}


int CSyncedLuaHandle::SyncedPairs(lua_State* L)
{
	/* copied from lbaselib.cpp */
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushcfunction(L, SyncedNext);  /* return generator, */
	lua_pushvalue(L, 1);  /* state, */
	lua_pushnil(L);  /* and initial value */
	return 3;
}


int CSyncedLuaHandle::SendToUnsynced(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args <= 0) {
		luaL_error(L, "Incorrect arguments to SendToUnsynced()");
	}

	static const int supportedTypes =
		  (1 << LUA_TNIL)
		| (1 << LUA_TBOOLEAN)
		| (1 << LUA_TNUMBER)
		| (1 << LUA_TSTRING)
	;

	for (int i = 1; i <= args; i++) {
		const int t = (1 << lua_type(L, i));
		if (!(t & supportedTypes)) {
			luaL_error(L, "Incorrect data type for SendToUnsynced(), arg %d", i);
		}
	}

	CUnsyncedLuaHandle* ulh = CLuaHandleSynced::GetUnsyncedHandle(L);
	ulh->RecvFromSynced(L, args);
	return 0;
}


int CSyncedLuaHandle::AddSyncedActionFallback(lua_State* L)
{
	string cmdRaw = luaL_checkstring(L, 1);
	cmdRaw = "/" + cmdRaw;

	string cmd = cmdRaw;
	const string::size_type pos = cmdRaw.find_first_of(" \t");
	if (pos != string::npos) {
		cmd.resize(pos);
	}

	if (cmd.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	auto lhs = GetSyncedHandle(L);
	lhs->textCommands[cmd] = luaL_checkstring(L, 2);
	wordCompletion->AddWord(cmdRaw, true, false, false);
	lua_pushboolean(L, true);
	return 1;
}


int CSyncedLuaHandle::RemoveSyncedActionFallback(lua_State* L)
{
	//TODO move to LuaHandle
	string cmdRaw  = luaL_checkstring(L, 1);
	cmdRaw = "/" + cmdRaw;

	string cmd = cmdRaw;
	const string::size_type pos = cmdRaw.find_first_of(" \t");
	if (pos != string::npos) {
		cmd.resize(pos);
	}

	if (cmd.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	auto lhs = GetSyncedHandle(L);
	map<string, string>::iterator it = lhs->textCommands.find(cmd);
	if (it != lhs->textCommands.end()) {
		lhs->textCommands.erase(it);
		wordCompletion->RemoveWord(cmdRaw);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}


#define GetWatchDef(DefType)                                          \
	int CSyncedLuaHandle::GetWatch ## DefType ## Def(lua_State* L) {  \
		CSyncedLuaHandle* lhs = GetSyncedHandle(L);                   \
		const unsigned int defID = luaL_checkint(L, 1);               \
		if (defID >= lhs->watch ## DefType ## Defs.size()) {          \
			return 0;                                                 \
		}                                                             \
		lua_pushboolean(L, lhs->watch ## DefType ## Defs[defID]);     \
		return 1;                                                     \
	}

#define SetWatchDef(DefType)                                          \
	int CSyncedLuaHandle::SetWatch ## DefType ## Def(lua_State* L) {  \
		CSyncedLuaHandle* lhs = GetSyncedHandle(L);                   \
		const unsigned int defID = luaL_checkint(L, 1);               \
		if (defID >= lhs->watch ## DefType ## Defs.size()) {          \
			return 0;                                                 \
		}                                                             \
		lhs->watch ## DefType ## Defs[defID] = luaL_checkboolean(L, 2);   \
		return 0;                                                     \
	}

GetWatchDef(Unit)
GetWatchDef(Feature)
GetWatchDef(Weapon)

SetWatchDef(Unit)
SetWatchDef(Feature)
SetWatchDef(Weapon)

#undef GetWatchDef
#undef SetWatchDef


/******************************************************************************/
/******************************************************************************/
//  ######  ##     ##    ###    ########  ######## ########
// ##    ## ##     ##   ## ##   ##     ## ##       ##     ##
// ##       ##     ##  ##   ##  ##     ## ##       ##     ##
//  ######  ######### ##     ## ########  ######   ##     ##
//       ## ##     ## ######### ##   ##   ##       ##     ##
// ##    ## ##     ## ##     ## ##    ##  ##       ##     ##
//  ######  ##     ## ##     ## ##     ## ######## ########

CLuaHandleSynced::CLuaHandleSynced(const string& _name, int _order)
	: syncedLuaHandle(this, _name, _order)
	, unsyncedLuaHandle(this, _name, _order + 1)
{
}


CLuaHandleSynced::~CLuaHandleSynced()
{
	// must be called before their dtors!!!
	syncedLuaHandle.KillLua();
	unsyncedLuaHandle.KillLua();
}


void CLuaHandleSynced::Init(const string& syncedFile, const string& unsyncedFile, const string& modes)
{
	if (!IsValid())
		return;

	const string syncedCode   = LoadFile(syncedFile, modes);
	const string unsyncedCode = LoadFile(unsyncedFile, modes);
	if (syncedCode.empty() && unsyncedCode.empty()) {
		KillLua();
		return;
	}

	const bool haveSynced   = syncedLuaHandle.Init(syncedCode, syncedFile);
	const bool haveUnsynced = unsyncedLuaHandle.Init(unsyncedCode, unsyncedFile);

	if (!IsValid() || (!haveSynced && !haveUnsynced)) {
		KillLua();
		return;
	}

	CheckStack();
}


string CLuaHandleSynced::LoadFile(const string& filename, const string& modes) const
{
	string vfsModes(modes);
	if (CSyncedLuaHandle::devMode) {
		vfsModes = SPRING_VFS_RAW + vfsModes;
	}
	CFileHandler f(filename, vfsModes);
	string code;
	if (!f.LoadStringData(code)) {
		code.clear();
	}
	return code;
}

//
// Call-Outs
//

int CLuaHandleSynced::LoadStringData(lua_State* L)
{
	size_t len;
	const char *str    = luaL_checklstring(L, 1, &len);
	const char *chunkname = luaL_optstring(L, 2, str);
	int status = luaL_loadbuffer(L, str, len, chunkname);
	if (status != 0) {
		lua_pushnil(L);
		lua_insert(L, -2);
		return 2; // nil, then the error message
	}
	// set the chunk's fenv to the current fenv
	if (lua_istable(L, 3)) {
		lua_pushvalue(L, 3);
	} else {
		LuaUtils::PushCurrentFuncEnv(L, __FUNCTION__);
	}
	if (lua_setfenv(L, -2) == 0) {
		luaL_error(L, "loadstring(): error with setfenv");
	}
	return 1;
}


int CLuaHandleSynced::CallAsTeam(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args < 2) || !lua_isfunction(L, 2)) {
		luaL_error(L, "Incorrect arguments to CallAsTeam()");
	}

	// save the current access
	const bool prevFullCtrl    = CLuaHandle::GetHandleFullCtrl(L);
	const bool prevFullRead    = CLuaHandle::GetHandleFullRead(L);
	const int prevCtrlTeam     = CLuaHandle::GetHandleCtrlTeam(L);
	const int prevReadTeam     = CLuaHandle::GetHandleReadTeam(L);
	const int prevReadAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);
	const int prevSelectTeam   = CLuaHandle::GetHandleSelectTeam(L);

	// parse the new access
	if (lua_isnumber(L, 1)) {
		const int teamID = lua_toint(L, 1);
		if ((teamID < CEventClient::MinSpecialTeam) || (teamID >= teamHandler->ActiveTeams())) {
			luaL_error(L, "Bad teamID in SetCtrlTeam");
		}
		// ctrl
		CLuaHandle::SetHandleCtrlTeam(L, teamID);
		CLuaHandle::SetHandleFullCtrl(L, prevCtrlTeam == CEventClient::AllAccessTeam);
		// read
		CLuaHandle::SetHandleReadTeam(L, teamID);
		CLuaHandle::SetHandleReadAllyTeam(L, (teamID < 0) ? teamID : teamHandler->AllyTeam(teamID));
		CLuaHandle::SetHandleFullRead(L, prevReadAllyTeam == CEventClient::AllAccessTeam);
		// select
		CLuaHandle::SetHandleSelectTeam(L, teamID);
	}
	else if (lua_istable(L, 1)) {
		const int table = 1;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1)) {
				continue;
			}
			const string key = lua_tostring(L, -2);
			const int teamID = lua_toint(L, -1);
			if ((teamID < CEventClient::MinSpecialTeam) || (teamID >= teamHandler->ActiveTeams())) {
				luaL_error(L, "Bad teamID in SetCtrlTeam");
			}

			if (key == "ctrl") {
				CLuaHandle::SetHandleCtrlTeam(L, teamID);
				CLuaHandle::SetHandleFullCtrl(L, prevCtrlTeam == CEventClient::AllAccessTeam);
			}
			else if (key == "read") {
				CLuaHandle::SetHandleReadTeam(L, teamID);
				CLuaHandle::SetHandleReadAllyTeam(L, (teamID < 0) ? teamID : teamHandler->AllyTeam(teamID));
				CLuaHandle::SetHandleFullRead(L, prevReadAllyTeam == CEventClient::AllAccessTeam);
			}
			else if (key == "select") {
				CLuaHandle::SetHandleSelectTeam(L, teamID);
			}
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to CallAsTeam()");
	}

	// call the function
	const int funcArgs = lua_gettop(L) - 2;

	// protected call so that the permissions are always reverted
	const int error = lua_pcall(L, funcArgs, LUA_MULTRET, 0);

	// revert the permissions
	CLuaHandle::SetHandleFullCtrl(L, prevFullCtrl);
	CLuaHandle::SetHandleFullRead(L, prevFullRead);
	CLuaHandle::SetHandleCtrlTeam(L, prevCtrlTeam);
	CLuaHandle::SetHandleReadTeam(L, prevReadTeam);
	CLuaHandle::SetHandleReadAllyTeam(L, prevReadAllyTeam);
	CLuaHandle::SetHandleSelectTeam(L, prevSelectTeam);

	if (error != 0) {
		LOG_L(L_ERROR, "error = %i, %s, %s",
				error, "CallAsTeam", lua_tostring(L, -1));
		lua_error(L);
	}

	return lua_gettop(L) - 1;	// the teamID/table is still on the stack
}



/******************************************************************************/
/******************************************************************************/
