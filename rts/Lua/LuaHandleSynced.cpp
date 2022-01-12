/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaHandleSynced.h"

#include "LuaInclude.h"

#include "LuaUtils.h"
#include "LuaArchive.h"
#include "LuaCallInCheck.h"
#include "LuaConfig.h"
#include "LuaConstGL.h"
#include "LuaConstCMD.h"
#include "LuaConstCMDTYPE.h"
#include "LuaConstCOB.h"
#include "LuaConstEngine.h"
#include "LuaConstGame.h"
#include "LuaConstPlatform.h"
#include "LuaInterCall.h"
#include "LuaSyncedCtrl.h"
#include "LuaSyncedRead.h"
#include "LuaSyncedTable.h"
#include "LuaUICommand.h"
#include "LuaUnsyncedCtrl.h"
#include "LuaUnsyncedRead.h"
#include "LuaFeatureDefs.h"
#include "LuaUnitDefs.h"
#include "LuaWeaponDefs.h"
#include "LuaScream.h"
#include "LuaMaterial.h"
#include "LuaOpenGL.h"
#include "LuaVFS.h"
#include "LuaZip.h"

#include "Game/Game.h"
#include "Game/WordCompletion.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/Scripts/LuaUnitScript.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/EventHandler.h"
#include "System/creg/SerializeLuaState.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"



LuaRulesParams::Params  CSplitLuaHandle::gameParams;



/******************************************************************************/
/******************************************************************************/
// ##     ## ##    ##  ######  ##    ## ##    ##  ######  ######## ########
// ##     ## ###   ## ##    ##  ##  ##  ###   ## ##    ## ##       ##     ##
// ##     ## ####  ## ##         ####   ####  ## ##       ##       ##     ##
// ##     ## ## ## ##  ######     ##    ## ## ## ##       ######   ##     ##
// ##     ## ##  ####       ##    ##    ##  #### ##       ##       ##     ##
// ##     ## ##   ### ##    ##    ##    ##   ### ##    ## ##       ##     ##
//  #######  ##    ##  ######     ##    ##    ##  ######  ######## ########

CUnsyncedLuaHandle::CUnsyncedLuaHandle(CSplitLuaHandle* _base, const std::string& _name, int _order)
	: CLuaHandle(_name, _order, false, false)
	, base(*_base)
{
	D.allowChanges = false;
}


CUnsyncedLuaHandle::~CUnsyncedLuaHandle() = default;


bool CUnsyncedLuaHandle::Init(const std::string& code, const std::string& file)
{
	if (!IsValid())
		return false;

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

	LuaPushNamedCFunc(L, "loadstring", CSplitLuaHandle::LoadStringData);
	LuaPushNamedCFunc(L, "CallAsTeam", CSplitLuaHandle::CallAsTeam);
	LuaPushNamedNumber(L, "COBSCALE",  COBSCALE);

	// load our libraries
	{
		#define KILL { KillLua(); return false; }
		if (!LuaSyncedTable::PushEntries(L)) KILL

		if (!AddEntriesToTable(L, "VFS",                   LuaVFS::PushUnsynced       )) KILL
		if (!AddEntriesToTable(L, "VFS",         LuaZipFileReader::PushUnsynced       )) KILL
		if (!AddEntriesToTable(L, "VFS",         LuaZipFileWriter::PushUnsynced       )) KILL
		if (!AddEntriesToTable(L, "VFS",               LuaArchive::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "UnitDefs",         LuaUnitDefs::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "WeaponDefs",     LuaWeaponDefs::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "FeatureDefs",   LuaFeatureDefs::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "Script",          LuaInterCall::PushEntriesUnsynced)) KILL
		if (!AddEntriesToTable(L, "Script",             LuaScream::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "Spring",         LuaSyncedRead::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "Spring",       LuaUnsyncedCtrl::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "Spring",       LuaUnsyncedRead::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "Spring",          LuaUICommand::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "gl",                 LuaOpenGL::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "GL",                LuaConstGL::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "Engine",        LuaConstEngine::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "Platform",      LuaConstPlatform::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "Game",            LuaConstGame::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "CMD",              LuaConstCMD::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "CMDTYPE",      LuaConstCMDTYPE::PushEntries        )) KILL
		if (!AddEntriesToTable(L, "LOG",                 LuaUtils::PushLogEntries     )) KILL
		#undef KILL
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
	luaL_checkstack(L, 2 + args, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return; // the call is not defined

	LuaUtils::CopyData(L, srcState, args);

	// call the routine
	RunCallIn(L, cmdStr, args, 0);
}


bool CUnsyncedLuaHandle::DrawUnit(const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __func__);

	static const LuaHashString cmdStr(__func__);
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

	const bool draw = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return draw;
}


bool CUnsyncedLuaHandle::DrawFeature(const CFeature* feature)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __func__);

	static const LuaHashString cmdStr(__func__);
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

	const bool draw = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return draw;
}


bool CUnsyncedLuaHandle::DrawShield(const CUnit* unit, const CWeapon* weapon)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	const bool oldDrawState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, weapon->weaponNum + LUA_WEAPON_BASE_INDEX);
	lua_pushnumber(L, game->GetDrawMode());

	const bool success = RunCallIn(L, cmdStr, 3, 1);
	LuaOpenGL::SetDrawingEnabled(L, oldDrawState);

	if (!success)
		return false;

	const bool draw = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return draw;
}


bool CUnsyncedLuaHandle::DrawProjectile(const CProjectile* projectile)
{
	assert(projectile->weapon || projectile->piece);

	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
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

	const bool draw = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return draw;
}


bool CUnsyncedLuaHandle::DrawMaterial(const LuaMaterial* material)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 4, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return false;

	const bool oldDrawState = LuaOpenGL::IsDrawingEnabled(L);
	LuaOpenGL::SetDrawingEnabled(L, true);

	lua_pushnumber(L, material->uuid);
	// lua_pushnumber(L, material->type);
	lua_pushnumber(L, game->GetDrawMode());

	const bool success = RunCallIn(L, cmdStr, 2, 1);
	LuaOpenGL::SetDrawingEnabled(L, oldDrawState);

	if (!success)
		return false;

	const bool draw = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return draw;
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

CSyncedLuaHandle::CSyncedLuaHandle(CSplitLuaHandle* _base, const std::string& _name, int _order)
	: CLuaHandle(_name, _order, false, true)
	, base(*_base)
	, origNextRef(-1)
{
	D.allowChanges = true;
}


CSyncedLuaHandle::~CSyncedLuaHandle()
{
	// kill all unitscripts running in this handle
	CLuaUnitScript::HandleFreed(this);
}


bool CSyncedLuaHandle::Init(const std::string& code, const std::string& file)
{
	if (!IsValid())
		return false;

	watchUnitDefs.resize(unitDefHandler->NumUnitDefs() + 1, false);
	watchFeatureDefs.resize(featureDefHandler->NumFeatureDefs() + 1, false);
	watchExplosionDefs.resize(weaponDefHandler->NumWeaponDefs(), false);
	watchProjectileDefs.resize(weaponDefHandler->NumWeaponDefs() + 1, false); // last bit controls piece-projectiles
	watchAllowTargetDefs.resize(weaponDefHandler->NumWeaponDefs(), false);

	// load the standard libraries
	SPRING_LUA_OPEN_LIB(L, luaopen_base);
	SPRING_LUA_OPEN_LIB(L, luaopen_math);
	SPRING_LUA_OPEN_LIB(L, luaopen_table);
	SPRING_LUA_OPEN_LIB(L, luaopen_string);
	//SPRING_LUA_OPEN_LIB(L, luaopen_io);
	//SPRING_LUA_OPEN_LIB(L, luaopen_os);
	//SPRING_LUA_OPEN_LIB(L, luaopen_package);
	//SPRING_LUA_OPEN_LIB(L, luaopen_debug);

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
	LuaPushNamedCFunc(L, "loadstring", CSplitLuaHandle::LoadStringData);
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
		LuaPushNamedCFunc(L, "GetWatchExplosion",    GetWatchExplosionDef);
		LuaPushNamedCFunc(L, "SetWatchExplosion",    SetWatchExplosionDef);
		LuaPushNamedCFunc(L, "GetWatchProjectile",   GetWatchProjectileDef);
		LuaPushNamedCFunc(L, "SetWatchProjectile",   SetWatchProjectileDef);
		LuaPushNamedCFunc(L, "GetWatchAllowTarget",  GetWatchAllowTargetDef);
		LuaPushNamedCFunc(L, "SetWatchAllowTarget",  SetWatchAllowTargetDef);
		LuaPushNamedCFunc(L, "GetWatchWeapon",       GetWatchWeaponDef);
		LuaPushNamedCFunc(L, "SetWatchWeapon",       SetWatchWeaponDef);
	lua_pop(L, 1);

	// add the custom file loader
	LuaPushNamedCFunc(L, "SendToUnsynced", SendToUnsynced);
	LuaPushNamedCFunc(L, "CallAsTeam",     CSplitLuaHandle::CallAsTeam);
	LuaPushNamedNumber(L, "COBSCALE",      COBSCALE);

	// load our libraries  (LuaSyncedCtrl overrides some LuaUnsyncedCtrl entries)
	{
		#define KILL { KillLua(); return false; }
		if (!AddEntriesToTable(L, "VFS",                   LuaVFS::PushSynced       )) KILL
		if (!AddEntriesToTable(L, "VFS",         LuaZipFileReader::PushSynced       )) KILL
		if (!AddEntriesToTable(L, "VFS",         LuaZipFileWriter::PushSynced       )) KILL
		if (!AddEntriesToTable(L, "UnitDefs",         LuaUnitDefs::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "WeaponDefs",     LuaWeaponDefs::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "FeatureDefs",   LuaFeatureDefs::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "Script",          LuaInterCall::PushEntriesSynced)) KILL
		if (!AddEntriesToTable(L, "Spring",       LuaUnsyncedCtrl::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "Spring",         LuaSyncedCtrl::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "Spring",         LuaSyncedRead::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "Spring",          LuaUICommand::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "Engine",        LuaConstEngine::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "Game",            LuaConstGame::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "CMD",              LuaConstCMD::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "CMDTYPE",      LuaConstCMDTYPE::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "COB",              LuaConstCOB::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "SFX",              LuaConstSFX::PushEntries      )) KILL
		if (!AddEntriesToTable(L, "LOG",                 LuaUtils::PushLogEntries   )) KILL
		#undef KILL
	}

	// add code from the sub-class
	if (!base.AddSyncedCode(L)) {
		KillLua();
		return false;
	}

	lua_settop(L, 0);
	creg::AutoRegisterCFunctions(GetName(), L);

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

bool CSyncedLuaHandle::SyncedActionFallback(const std::string& msg, int playerID)
{
	string cmd = msg;
	const std::string::size_type pos = cmd.find_first_of(" \t");
	if (pos != string::npos)
		cmd.resize(pos);

	if (textCommands.find(cmd) == textCommands.end())
		return false;

	// strip the leading '/'
	return GotChatMsg(msg.substr(1), playerID);
}


bool CSyncedLuaHandle::CommandFallback(const CUnit* unit, const Command& cmd)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 9, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	LuaUtils::PushUnitAndCommand(L, unit, cmd);

	// call the function
	if (!RunCallIn(L, cmdStr, 7, 1))
		return true;

	const bool remove = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return remove; // return 'true' to remove the command
}


bool CSyncedLuaHandle::AllowCommand(const CUnit* unit, const Command& cmd, int playerNum, bool fromSynced, bool fromLua)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7 + 3, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	const int argc = LuaUtils::PushUnitAndCommand(L, unit, cmd);

	lua_pushnumber(L, playerNum);
	lua_pushboolean(L, fromSynced);
	lua_pushboolean(L, fromLua);

	// call the function
	if (!RunCallIn(L, cmdStr, argc + 3, 1))
		return true;

	// get the results
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowUnitCreation(
	const UnitDef* unitDef,
	const CUnit* builder,
	const BuildInfo* buildInfo
) {
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 9, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, unitDef->id);
	lua_pushnumber(L, builder->id);
	lua_pushnumber(L, builder->team);

	if (buildInfo != nullptr) {
		lua_pushnumber(L, buildInfo->pos.x);
		lua_pushnumber(L, buildInfo->pos.y);
		lua_pushnumber(L, buildInfo->pos.z);
		lua_pushnumber(L, buildInfo->buildFacing);
	}

	// call the function
	if (!RunCallIn(L, cmdStr, (buildInfo != nullptr)? 7 : 3, 1))
		return true;

	// get the results
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}



bool CSyncedLuaHandle::AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7, __func__);

	static const LuaHashString cmdStr(__func__);
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
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7, __func__);

	static const LuaHashString cmdStr(__func__);
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
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowUnitTransport(const CUnit* transporter, const CUnit* transportee)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 6, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return true;

	lua_pushnumber(L, transporter->id);
	lua_pushnumber(L, transporter->unitDef->id);
	lua_pushnumber(L, transporter->team);
	lua_pushnumber(L, transportee->id);
	lua_pushnumber(L, transportee->unitDef->id);
	lua_pushnumber(L, transportee->team);

	if (!RunCallIn(L, cmdStr, 6, 1))
		return true;

	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}

bool CSyncedLuaHandle::AllowUnitTransportLoad(
	const CUnit* transporter,
	const CUnit* transportee,
	const float3& loadPos,
	bool allowed
) {
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 9, __func__);

	static const LuaHashString cmdStr(__func__);

	// use engine default if callin does not exist
	if (!cmdStr.GetGlobalFunc(L))
		return allowed;

	lua_pushnumber(L, transporter->id);
	lua_pushnumber(L, transporter->unitDef->id);
	lua_pushnumber(L, transporter->team);
	lua_pushnumber(L, transportee->id);
	lua_pushnumber(L, transportee->unitDef->id);
	lua_pushnumber(L, transportee->team);
	lua_pushnumber(L, loadPos.x);
	lua_pushnumber(L, loadPos.y);
	lua_pushnumber(L, loadPos.z);

	if (!RunCallIn(L, cmdStr, 9, 1))
		return true;

	// ditto if it does but returns nothing
	const bool allow = luaL_optboolean(L, -1, allowed);
	lua_pop(L, 1);
	return allow;
}

bool CSyncedLuaHandle::AllowUnitTransportUnload(
	const CUnit* transporter,
	const CUnit* transportee,
	const float3& unloadPos,
	bool allowed
) {
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 9, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return allowed;

	lua_pushnumber(L, transporter->id);
	lua_pushnumber(L, transporter->unitDef->id);
	lua_pushnumber(L, transporter->team);
	lua_pushnumber(L, transportee->id);
	lua_pushnumber(L, transportee->unitDef->id);
	lua_pushnumber(L, transportee->team);
	lua_pushnumber(L, unloadPos.x);
	lua_pushnumber(L, unloadPos.y);
	lua_pushnumber(L, unloadPos.z);

	if (!RunCallIn(L, cmdStr, 9, 1))
		return true;

	const bool allow = luaL_optboolean(L, -1, allowed);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowUnitCloak(const CUnit* unit, const CUnit* enemy)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 2, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return true;


	lua_pushnumber(L, unit->id);

	if (enemy != nullptr)
		lua_pushnumber(L, enemy->id);
	else
		lua_pushnil(L);


	if (!RunCallIn(L, cmdStr, 2, 1))
		return true;

	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}

bool CSyncedLuaHandle::AllowUnitDecloak(const CUnit* unit, const CSolidObject* object, const CWeapon* weapon)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 3, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return true;


	lua_pushnumber(L, unit->id);

	if (object != nullptr)
		lua_pushnumber(L, object->id);
	else
		lua_pushnil(L);

	if (weapon != nullptr)
		lua_pushnumber(L, weapon->weaponNum);
	else
		lua_pushnil(L);


	if (!RunCallIn(L, cmdStr, 3, 1))
		return true;

	assert(lua_isboolean(L, -1));

	const bool allow = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return allow;
}

bool CSyncedLuaHandle::AllowUnitKamikaze(const CUnit* unit, const CUnit* target, bool allowed)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 2, __func__);

	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return allowed;

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, target->id);

	if (!RunCallIn(L, cmdStr, 2, 1))
		return true;

	const bool allow = luaL_optboolean(L, -1, allowed);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowFeatureCreation(const FeatureDef* featureDef, int teamID, const float3& pos)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7, __func__);

	static const LuaHashString cmdStr(__func__);
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
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature, float part)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 7, __func__);

	static const LuaHashString cmdStr(__func__);
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
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowResourceLevel(int teamID, const std::string& type, float level)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 5, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, teamID);
	lua_pushsstring(L, type);
	lua_pushnumber(L, level);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return true;

	// get the results
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowResourceTransfer(int oldTeam, int newTeam, const char* type, float amount)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 6, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, oldTeam);
	lua_pushnumber(L, newTeam);
	lua_pushstring(L, type);
	lua_pushnumber(L, amount);

	// call the function
	if (!RunCallIn(L, cmdStr, 4, 1))
		return true;

	// get the results
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowDirectUnitControl(int playerID, const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 6, __func__);

	static const LuaHashString cmdStr(__func__);
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
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowBuilderHoldFire(const CUnit* unit, int action)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 3 + 1, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, action);

	// call the function
	if (!RunCallIn(L, cmdStr, 3, 1))
		return true;

	// get the results
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::AllowStartPosition(int playerID, int teamID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos)
{
	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 13, __func__);

	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return true; // the call is not defined

	// push the start position and playerID
	lua_pushnumber(L, playerID);
	lua_pushnumber(L, teamID);
	lua_pushnumber(L, readyState);
	lua_pushnumber(L, clampedPos.x);
	lua_pushnumber(L, clampedPos.y);
	lua_pushnumber(L, clampedPos.z);
	lua_pushnumber(L, rawPickPos.x);
	lua_pushnumber(L, rawPickPos.y);
	lua_pushnumber(L, rawPickPos.z);

	// call the function
	if (!RunCallIn(L, cmdStr, 9, 1))
		return true;

	// get the results
	const bool allow = luaL_optboolean(L, -1, true);
	lua_pop(L, 1);
	return allow;
}


bool CSyncedLuaHandle::MoveCtrlNotify(const CUnit* unit, int data)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 6, __func__);

	static const LuaHashString cmdStr(__func__);
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
	const bool disable = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return disable;
}


bool CSyncedLuaHandle::TerraformComplete(const CUnit* unit, const CUnit* build)
{
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 8, __func__);

	const LuaUtils::ScopedDebugTraceBack dbgTrace(L);
	static const LuaHashString cmdStr(__func__);

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
	if (!RunCallInTraceback(L, cmdStr, 6, 1, dbgTrace.GetErrFuncIdx(), false))
		return false;

	// get the results
	const bool stop = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return stop;
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
	float* impulseMult
) {
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 2 + 2 + 10, __func__);

	const LuaUtils::ScopedDebugTraceBack dbgTrace(L);
	static const LuaHashString cmdStr(__func__);

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

		if (attacker != nullptr) {
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
	if (!RunCallInTraceback(L, cmdStr, inArgCount, outArgCount, dbgTrace.GetErrFuncIdx(), false))
		return false;

	assert(newDamage != nullptr);
	assert(impulseMult != nullptr);

	if (lua_isnumber(L, -2)) {
		*newDamage = lua_tonumber(L, -2);
	} else if (!lua_isnumber(L, -2) || lua_isnil(L, -2)) {
		// first value is obligatory, so may not be nil
		LOG_L(L_WARNING, "%s(): 1st return-value should be a number (newDamage)", cmdStr.GetString());
	}

	if (lua_isnumber(L, -1)) {
		*impulseMult = lua_tonumber(L, -1);
	} else if (!lua_isnumber(L, -1) && !lua_isnil(L, -1)) {
		// second value is optional, so nils are OK
		LOG_L(L_WARNING, "%s(): 2nd return-value should be a number (impulseMult)", cmdStr.GetString());
	}

	lua_pop(L, outArgCount);
	// returns true to disable engine dmg handling
	return (*newDamage == 0.0f && *impulseMult == 0.0f);
}

bool CSyncedLuaHandle::FeaturePreDamaged(
	const CFeature* feature,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	float* newDamage,
	float* impulseMult
) {
	assert(newDamage != nullptr);
	assert(impulseMult != nullptr);

	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 2 + 9 + 2, __func__);

	const LuaUtils::ScopedDebugTraceBack dbgTrace(L);
	static const LuaHashString cmdStr(__func__);

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

		if (attacker != nullptr) {
			lua_pushnumber(L, attacker->id);
			lua_pushnumber(L, attacker->unitDef->id);
			lua_pushnumber(L, attacker->team);
			inArgCount += 3;
		}
	}

	// call the routine
	if (!RunCallInTraceback(L, cmdStr, inArgCount, outArgCount, dbgTrace.GetErrFuncIdx(), false))
		return false;

	if (lua_isnumber(L, -2)) {
		*newDamage = lua_tonumber(L, -2);
	} else if (!lua_isnumber(L, -2) || lua_isnil(L, -2)) {
		// first value is obligatory, so may not be nil
		LOG_L(L_WARNING, "%s(): 1st value returned should be a number (newDamage)", cmdStr.GetString());
	}

	if (lua_isnumber(L, -1)) {
		*impulseMult = lua_tonumber(L, -1);
	} else if (!lua_isnumber(L, -1) && !lua_isnil(L, -1)) {
		// second value is optional, so nils are OK
		LOG_L(L_WARNING, "%s(): 2nd value returned should be a number (impulseMult)", cmdStr.GetString());
	}

	lua_pop(L, outArgCount);
	// returns true to disable engine dmg handling
	return (*newDamage == 0.0f && *impulseMult == 0.0f);
}

bool CSyncedLuaHandle::ShieldPreDamaged(
	const CProjectile* projectile,
	const CWeapon* shieldEmitter,
	const CUnit* shieldCarrier,
	bool bounceProjectile,
	const CWeapon* beamEmitter,
	const CUnit* beamCarrier,
	const float3& startPos,
	const float3& hitPos
) {
	assert((projectile != nullptr) || ((beamEmitter != nullptr) && (beamCarrier != nullptr)));
	LUA_CALL_IN_CHECK(L, false);
	luaL_checkstack(L, 2 + 7 + 1, __func__);

	const LuaUtils::ScopedDebugTraceBack dbgTrace(L);
	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return false;

	// push the call-in arguments
	if (projectile != nullptr) {
		// Regular projectiles
		lua_pushnumber(L, projectile->id);
		lua_pushnumber(L, projectile->GetOwnerID());
		lua_pushnumber(L, shieldEmitter->weaponNum + LUA_WEAPON_BASE_INDEX);
		lua_pushnumber(L, shieldCarrier->id);
		lua_pushboolean(L, bounceProjectile);
		lua_pushnil(L);
		lua_pushnil(L);
	} else {
		// Beam projectiles
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushnumber(L, shieldEmitter->weaponNum + LUA_WEAPON_BASE_INDEX);
		lua_pushnumber(L, shieldCarrier->id);
		lua_pushboolean(L, bounceProjectile);
		lua_pushnumber(L, beamEmitter->weaponNum + LUA_WEAPON_BASE_INDEX);
		lua_pushnumber(L, beamCarrier->id);
	}

	lua_pushnumber(L, startPos.x);
	lua_pushnumber(L, startPos.y);
	lua_pushnumber(L, startPos.z);
	lua_pushnumber(L, hitPos.x);
	lua_pushnumber(L, hitPos.y);
	lua_pushnumber(L, hitPos.z);

	// call the routine
	if (!RunCallInTraceback(L, cmdStr, 13, 1, dbgTrace.GetErrFuncIdx(), false))
		return false;

	// pop the return-value; must be true or false
	const bool ret = luaL_optboolean(L, -1, false);
	lua_pop(L, 1);
	return ret;
}


int CSyncedLuaHandle::AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID)
{
	int ret = -1;

	if (!watchAllowTargetDefs[attackerWeaponDefID])
		return ret;

	LUA_CALL_IN_CHECK(L, -1);
	luaL_checkstack(L, 2 + 3 + 1, __func__);

	const LuaUtils::ScopedDebugTraceBack dbgTrace(L);
	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return ret;

	lua_pushnumber(L, attackerID);
	lua_pushnumber(L, attackerWeaponNum + LUA_WEAPON_BASE_INDEX);
	lua_pushnumber(L, attackerWeaponDefID);

	if (!RunCallInTraceback(L, cmdStr, 3, 1, dbgTrace.GetErrFuncIdx(), false))
		return ret;

	ret = lua_toint(L, -1);

	lua_pop(L, 1);
	return ret;
}


bool CSyncedLuaHandle::AllowWeaponTarget(
	unsigned int attackerID,
	unsigned int targetID,
	unsigned int attackerWeaponNum,
	unsigned int attackerWeaponDefID,
	float* targetPriority
) {
	bool ret = true;

	if (!watchAllowTargetDefs[attackerWeaponDefID])
		return ret;

	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 5 + 2, __func__);

	const LuaUtils::ScopedDebugTraceBack dbgTrace(L);
	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return ret;

	// casts are only here to preserve -1's passed from *CAI as floats
	const int siTargetID = static_cast<int>(targetID);
	const int siAttackerWN = static_cast<int>(attackerWeaponNum);

	lua_pushnumber(L, attackerID);
	lua_pushnumber(L, siTargetID);
	lua_pushnumber(L, siAttackerWN + LUA_WEAPON_BASE_INDEX * (siAttackerWN >= 0));
	lua_pushnumber(L, attackerWeaponDefID);

	if (targetPriority != nullptr) {
		// Weapon::AutoTarget
		lua_pushnumber(L, *targetPriority);
	} else {
		// {Air,Mobile}CAI::AutoGenerateTarget
		lua_pushnil(L);
	}

	if (!RunCallInTraceback(L, cmdStr, 5, 2, dbgTrace.GetErrFuncIdx(), false))
		return ret;

	if (targetPriority != nullptr)
		*targetPriority = luaL_optnumber(L, -1, *targetPriority);

	ret = luaL_optboolean(L, -2, false);

	lua_pop(L, 2);
	return ret;
}


bool CSyncedLuaHandle::AllowWeaponInterceptTarget(
	const CUnit* interceptorUnit,
	const CWeapon* interceptorWeapon,
	const CProjectile* interceptorTarget
) {
	bool ret = true;

	if (!watchAllowTargetDefs[interceptorWeapon->weaponDef->id])
		return ret;

	LUA_CALL_IN_CHECK(L, true);
	luaL_checkstack(L, 2 + 3 + 1, __func__);

	const LuaUtils::ScopedDebugTraceBack dbgTrace(L);
	static const LuaHashString cmdStr(__func__);

	if (!cmdStr.GetGlobalFunc(L))
		return ret;

	lua_pushnumber(L, interceptorUnit->id);
	lua_pushnumber(L, interceptorWeapon->weaponNum + LUA_WEAPON_BASE_INDEX);
	lua_pushnumber(L, interceptorTarget->id);

	if (!RunCallInTraceback(L, cmdStr, 3, 1, dbgTrace.GetErrFuncIdx(), false))
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
	#if 0
	spring_lua_synced_rand(L);
	return 1;
	#endif

	switch (lua_gettop(L)) {
		case 0: {
			lua_pushnumber(L, gsRNG.NextFloat());
			return 1;
		} break;

		case 1: {
			if (lua_isnumber(L, 1)) {
				const int u = lua_toint(L, 1);

				if (u < 1)
					luaL_error(L, "error: too small upper limit (%d) given to math.random(), should be >= 1 {synced}", u);

				lua_pushnumber(L, 1 + gsRNG.NextInt(u));
				return 1;
			}
		} break;

		case 2: {
			if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
				const int lower = lua_toint(L, 1);
				const int upper = lua_toint(L, 2);

				if (lower > upper)
					luaL_error(L, "Empty interval in math.random() {synced}");

				const float diff  = (upper - lower);
				const float r = gsRNG.NextFloat(); // [0,1], not [0,1) ?

				lua_pushnumber(L, Clamp(lower + int(r * (diff + 1)), lower, upper));
				return 1;
			}
		} break;

		default: {
		} break;
	}

	luaL_error(L, "Incorrect arguments to math.random() {synced}");
	return 0;
}

int CSyncedLuaHandle::SyncedRandomSeed(lua_State* L)
{
	gsRNG.SetSeed(luaL_checkint(L, -1), false);
	return 0;
}


int CSyncedLuaHandle::SyncedNext(lua_State* L)
{
	constexpr int whiteList[] = {
		LUA_TSTRING,
		LUA_TNUMBER,
		LUA_TBOOLEAN,
		LUA_TNIL,
		LUA_TTHREAD //FIXME LUA_TTHREAD is normally _not_ synced safe but LUS handler needs it atm (and uses it in a safe way)
	};

	const CSyncedLuaHandle* slh = GetSyncedHandle(L);
	assert(slh->origNextRef > 0);
	const int oldTop = lua_gettop(L);

	lua_rawgeti(L, LUA_REGISTRYINDEX, slh->origNextRef);
	lua_pushvalue(L, 1);

	if (oldTop >= 2) {
		lua_pushvalue(L, 2);
	} else {
		lua_pushnil(L);
	}

	lua_call(L, 2, LUA_MULTRET);
	const int retCount = lua_gettop(L) - oldTop;
	assert(retCount == 1 || retCount == 2);

	if (retCount >= 2) {
		const int keyType = lua_type(L, -2);
		const auto it = std::find(std::begin(whiteList), std::end(whiteList), keyType);

		if (it == std::end(whiteList)) {
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

	CUnsyncedLuaHandle* ulh = CSplitLuaHandle::GetUnsyncedHandle(L);
	ulh->RecvFromSynced(L, args);
	return 0;
}


int CSyncedLuaHandle::AddSyncedActionFallback(lua_State* L)
{
	std::string cmdRaw = "/" + std::string(luaL_checkstring(L, 1));
	std::string cmd = cmdRaw;

	const std::string::size_type pos = cmdRaw.find_first_of(" \t");

	if (pos != string::npos)
		cmd.resize(pos);

	if (cmd.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	auto lhs = GetSyncedHandle(L);
	lhs->textCommands[cmd] = luaL_checkstring(L, 2);
	wordCompletion.AddWord(cmdRaw, true, false, false);
	lua_pushboolean(L, true);
	return 1;
}


int CSyncedLuaHandle::RemoveSyncedActionFallback(lua_State* L)
{
	//TODO move to LuaHandle
	std::string cmdRaw = "/" + std::string(luaL_checkstring(L, 1));
	std::string cmd = cmdRaw;

	const std::string::size_type pos = cmdRaw.find_first_of(" \t");

	if (pos != string::npos)
		cmd.resize(pos);

	if (cmd.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	auto lhs = GetSyncedHandle(L);
	auto& cmds = lhs->textCommands;

	const auto it = cmds.find(cmd);

	if (it != cmds.end()) {
		cmds.erase(it);
		wordCompletion.RemoveWord(cmdRaw);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}

	return 1;
}



#define GetWatchDef(DefType)                                                \
	int CSyncedLuaHandle::GetWatch ## DefType ## Def(lua_State* L) {        \
		const CSyncedLuaHandle* lhs = GetSyncedHandle(L);                   \
		const auto& vec = lhs->watch ## DefType ## Defs;                    \
                                                                            \
		const uint32_t defIdx = luaL_checkint(L, 1);                        \
                                                                            \
		if ((defIdx == -1u) && (&vec == &lhs->watchProjectileDefs)) {       \
			lua_pushboolean(L, vec[vec.size() - 1]);                        \
			return 1;                                                       \
		}                                                                   \
                                                                            \
		if (defIdx >= vec.size())                                           \
			return 0;                                                       \
                                                                            \
		lua_pushboolean(L, vec[defIdx]);                                    \
		return 1;                                                           \
	}

#define SetWatchDef(DefType)                                                \
	int CSyncedLuaHandle::SetWatch ## DefType ## Def(lua_State* L) {        \
		CSyncedLuaHandle* lhs = GetSyncedHandle(L);                         \
		auto& vec = lhs->watch ## DefType ## Defs;                          \
                                                                            \
		const uint32_t defIdx = luaL_checkint(L, 1);                        \
                                                                            \
		if ((defIdx == -1u) && (&vec == &lhs->watchProjectileDefs)) {       \
			vec[vec.size() - 1] = luaL_checkboolean(L, 2);                  \
			return 0;                                                       \
		}                                                                   \
                                                                            \
		if (defIdx >= vec.size())                                           \
			return 0;                                                       \
                                                                            \
		vec[defIdx] = luaL_checkboolean(L, 2);                              \
		return 0;                                                           \
	}

int CSyncedLuaHandle::GetWatchWeaponDef(lua_State* L) {
	bool watched = false;

	// trickery to keep Script.GetWatchWeapon backward-compatible
	{
		GetWatchExplosionDef(L);
		watched |= luaL_checkboolean(L, -1);
		lua_pop(L, 1);
	}
	{
		GetWatchProjectileDef(L);
		watched |= luaL_checkboolean(L, -1);
		lua_pop(L, 1);
	}
	{
		GetWatchAllowTargetDef(L);
		watched |= luaL_checkboolean(L, -1);
		lua_pop(L, 1);
	}

	lua_pushboolean(L, watched);
	return 1;
}

GetWatchDef(Unit)
GetWatchDef(Feature)
GetWatchDef(Explosion)
GetWatchDef(Projectile)
GetWatchDef(AllowTarget)

SetWatchDef(Unit)
SetWatchDef(Feature)
SetWatchDef(Explosion)
SetWatchDef(Projectile)
SetWatchDef(AllowTarget)

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

CSplitLuaHandle::CSplitLuaHandle(const std::string& _name, int _order)
	: syncedLuaHandle(this, _name, _order)
	, unsyncedLuaHandle(this, _name, _order + 1)
{
}


CSplitLuaHandle::~CSplitLuaHandle()
{
	// must be called before their dtors!!!
	syncedLuaHandle.KillLua();
	unsyncedLuaHandle.KillLua();
}


bool CSplitLuaHandle::InitSynced()
{
	if (!IsValid()) {
		KillLua();
		return false;
	}

	const std::string syncedCode = LoadFile(GetSyncedFileName(), GetInitFileModes());
	if (syncedCode.empty()) {
		KillLua();
		return false;
	}

	const bool haveSynced = syncedLuaHandle.Init(syncedCode, GetUnsyncedFileName());

	if (!IsValid() || !haveSynced) {
		KillLua();
		return false;
	}

	syncedLuaHandle.CheckStack();
	return true;
}


bool CSplitLuaHandle::InitUnsynced()
{
	if (!IsValid()) {
		KillLua();
		return false;
	}

	const std::string unsyncedCode = LoadFile(GetUnsyncedFileName(), GetInitFileModes());
	if (unsyncedCode.empty()) {
		KillLua();
		return false;
	}

	const bool haveUnsynced = unsyncedLuaHandle.Init(unsyncedCode, GetUnsyncedFileName());

	if (!IsValid() || !haveUnsynced) {
		KillLua();
		return false;
	}

	unsyncedLuaHandle.CheckStack();
	return true;
}


bool CSplitLuaHandle::Init(bool onlySynced)
{
	SetFullCtrl(true);
	SetFullRead(true);
	SetCtrlTeam(CEventClient::AllAccessTeam);
	SetReadTeam(CEventClient::AllAccessTeam);
	SetReadAllyTeam(CEventClient::AllAccessTeam);
	SetSelectTeam(GetInitSelectTeam());

	return InitSynced() && (onlySynced || InitUnsynced());
}


bool CSplitLuaHandle::FreeUnsynced()
{
	if (!unsyncedLuaHandle.IsValid())
		return false;

	unsyncedLuaHandle.KillLua();
	unsyncedLuaHandle.~CUnsyncedLuaHandle();

	return true;
}


bool CSplitLuaHandle::LoadUnsynced()
{
	::new (&unsyncedLuaHandle) CUnsyncedLuaHandle(this, syncedLuaHandle.GetName(), syncedLuaHandle.GetOrder() + 1);

	if (!unsyncedLuaHandle.IsValid()) {
		unsyncedLuaHandle.KillLua();
		return false;
	}

	return InitUnsynced();
}


bool CSplitLuaHandle::SwapSyncedHandle(lua_State* L, lua_State* L_GC)
{
	LUA_CLOSE(&syncedLuaHandle.L);
	syncedLuaHandle.SetLuaStates(L, L_GC);
	return IsValid();
}


string CSplitLuaHandle::LoadFile(const std::string& filename, const std::string& modes) const
{
	string vfsModes(modes);
	if (CSyncedLuaHandle::devMode)
		vfsModes = SPRING_VFS_RAW + vfsModes;

	CFileHandler f(filename, vfsModes);
	string code;
	if (!f.LoadStringData(code))
		code.clear();

	return code;
}

//
// Call-Outs
//

int CSplitLuaHandle::LoadStringData(lua_State* L)
{
	size_t len;
	const char *str    = luaL_checklstring(L, 1, &len);
	const char *chunkname = luaL_optstring(L, 2, str);

	if (luaL_loadbuffer(L, str, len, chunkname) != 0) {
		lua_pushnil(L);
		lua_insert(L, -2);
		return 2; // nil, then the error message
	}

	// set the chunk's fenv to the current fenv
	if (lua_istable(L, 3)) {
		lua_pushvalue(L, 3);
	} else {
		LuaUtils::PushCurrentFuncEnv(L, __func__);
	}

	if (lua_setfenv(L, -2) == 0)
		luaL_error(L, "[%s] error with setfenv", __func__);

	return 1;
}


int CSplitLuaHandle::CallAsTeam(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args < 2) || !lua_isfunction(L, 2))
		luaL_error(L, "[%s] incorrect arguments", __func__);

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

		if ((teamID < CEventClient::MinSpecialTeam) || (teamID >= teamHandler.ActiveTeams()))
			luaL_error(L, "[%s] bad teamID %d", __func__, teamID);

		// ctrl
		CLuaHandle::SetHandleCtrlTeam(L, teamID);
		CLuaHandle::SetHandleFullCtrl(L, teamID == CEventClient::AllAccessTeam);
		// read
		CLuaHandle::SetHandleReadTeam(L, teamID);
		CLuaHandle::SetHandleReadAllyTeam(L, (teamID < 0) ? teamID : teamHandler.AllyTeam(teamID));
		CLuaHandle::SetHandleFullRead(L, teamID == CEventClient::AllAccessTeam);
		// select
		CLuaHandle::SetHandleSelectTeam(L, teamID);
	}
	else if (lua_istable(L, 1)) {
		const int table = 1;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
				continue;

			const int teamID = lua_toint(L, -1);

			if ((teamID < CEventClient::MinSpecialTeam) || (teamID >= teamHandler.ActiveTeams()))
				luaL_error(L, "[%s] bad teamID %d", __func__, teamID);

			switch (hashString(lua_tostring(L, -2))) {
				case hashString("ctrl"): {
					CLuaHandle::SetHandleCtrlTeam(L, teamID);
					CLuaHandle::SetHandleFullCtrl(L, teamID == CEventClient::AllAccessTeam);
				} break;
				case hashString("read"): {
					CLuaHandle::SetHandleReadTeam(L, teamID);
					CLuaHandle::SetHandleReadAllyTeam(L, (teamID < 0) ? teamID : teamHandler.AllyTeam(teamID));
					CLuaHandle::SetHandleFullRead(L, teamID == CEventClient::AllAccessTeam);
				} break;
				case hashString("select"): {
					CLuaHandle::SetHandleSelectTeam(L, teamID);
				} break;
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
		LOG_L(L_ERROR, "[%s] error %i (%s)", __func__, error, lua_tostring(L, -1));
		lua_error(L);
	}

	return lua_gettop(L) - 1;	// the teamID/table is still on the stack
}



/******************************************************************************/
/******************************************************************************/
