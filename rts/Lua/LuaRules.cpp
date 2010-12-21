/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <set>
#include <cctype>

#include "mmgr.h"

#include "LuaRules.h"

#include "LuaInclude.h"

#include "LuaCallInCheck.h"
#include "LuaUtils.h"
#include "LuaMaterial.h"
#include "LuaSyncedCtrl.h"
#include "LuaSyncedRead.h"
#include "LuaUnitRendering.h"
#include "LuaUnitDefs.h"
#include "LuaWeaponDefs.h"
#include "LuaOpenGL.h"

#include "Game/Game.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Weapons/Weapon.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

#include <assert.h>

CLuaRules* luaRules = NULL;

static const char* LuaRulesSyncedFilename   = "LuaRules/main.lua";
static const char* LuaRulesUnsyncedFilename = "LuaRules/draw.lua";

const int* CLuaRules::currentCobArgs = NULL;


/******************************************************************************/
/******************************************************************************/

void CLuaRules::LoadHandler()
{
	if (luaRules) {
		return;
	}

	new CLuaRules();

	if (luaRules->L == NULL) {
		delete luaRules;
	}
}


void CLuaRules::FreeHandler()
{
	delete luaRules;
}


/******************************************************************************/
/******************************************************************************/

CLuaRules::CLuaRules()
: CLuaHandleSynced("LuaRules", LUA_HANDLE_ORDER_RULES)
{
	luaRules = this;

	if (L == NULL) {
		return;
	}

	fullCtrl = true;
	fullRead = true;
	ctrlTeam = AllAccessTeam;
	readTeam = AllAccessTeam;
	readAllyTeam = AllAccessTeam;
	selectTeam = AllAccessTeam;

	Init(LuaRulesSyncedFilename, LuaRulesUnsyncedFilename, SPRING_VFS_MOD);

	if (L == NULL) {
		return;
	}

	haveCommandFallback        = HasCallIn("CommandFallback");
	haveAllowCommand           = HasCallIn("AllowCommand");
	haveAllowUnitCreation      = HasCallIn("AllowUnitCreation");
	haveAllowUnitTransfer      = HasCallIn("AllowUnitTransfer");
	haveAllowUnitBuildStep     = HasCallIn("AllowUnitBuildStep");
	haveAllowFeatureCreation   = HasCallIn("AllowFeatureCreation");
	haveAllowFeatureBuildStep  = HasCallIn("AllowFeatureBuildStep");
	haveAllowResourceLevel     = HasCallIn("AllowResourceLevel");
	haveAllowResourceTransfer  = HasCallIn("AllowResourceTransfer");
	haveAllowDirectUnitControl = HasCallIn("AllowDirectUnitControl");
	haveAllowStartPosition     = HasCallIn("AllowStartPosition");

	haveMoveCtrlNotify         = HasCallIn("MoveCtrlNotify");
	haveTerraformComplete      = HasCallIn("TerraformComplete");
	haveAllowWeaponTargetCheck = HasCallIn("AllowWeaponTargetCheck");
	haveAllowWeaponTarget      = HasCallIn("AllowWeaponTarget");
	haveUnitPreDamaged         = HasCallIn("UnitPreDamaged");
	haveShieldPreDamaged       = HasCallIn("ShieldPreDamaged");

	haveDrawUnit    = HasCallIn("DrawUnit");
	haveDrawFeature = HasCallIn("DrawFeature");
	haveAICallIn    = HasCallIn("AICallIn");

	SetupUnsyncedFunction("DrawUnit");
	SetupUnsyncedFunction("DrawFeature");
	SetupUnsyncedFunction("AICallIn");
}


CLuaRules::~CLuaRules()
{
	if (L != NULL) {
		Shutdown();
		KillLua();
	}
	luaRules = NULL;

	// clear all lods
	std::list<CUnit*>::iterator it;
	for (it = uh->activeUnits.begin(); it != uh->activeUnits.end(); ++it) {
		CUnit* unit = *it;
		unit->SetLODCount(0);
	}
}


bool CLuaRules::AddSyncedCode()
{
	lua_getglobal(L, "Script");
	LuaPushNamedCFunc(L, "PermitHelperAIs", PermitHelperAIs);
	lua_pop(L, 1);

	return true;
}


bool CLuaRules::AddUnsyncedCode()
{
	lua_pushstring(L, "UNSYNCED");
	lua_gettable(L, LUA_REGISTRYINDEX);

	lua_pushstring(L, "Spring");
	lua_rawget(L, -2);
	lua_pushstring(L, "UnitRendering");
	lua_newtable(L);
	LuaUnitRendering::PushEntries(L);
	lua_rawset(L, -3);
	lua_pop(L, 1); // Spring

	lua_pop(L, 1); // UNSYNCED

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
// LuaRules Call-Ins
//

bool CLuaRules::SyncedUpdateCallIn(const string& name)
{
	#define UPDATE_HAVE_CALLIN(callinName) \
		have ## callinName = HasCallIn( #callinName );

	     if (name == "CommandFallback"       ) { UPDATE_HAVE_CALLIN(CommandFallback); }
	else if (name == "AllowCommand"          ) { UPDATE_HAVE_CALLIN(AllowCommand); }
	else if (name == "AllowUnitCreation"     ) { UPDATE_HAVE_CALLIN(AllowUnitCreation); }
	else if (name == "AllowUnitTransfer"     ) { UPDATE_HAVE_CALLIN(AllowUnitTransfer); }
	else if (name == "AllowUnitBuildStep"    ) { UPDATE_HAVE_CALLIN(AllowUnitBuildStep); }
	else if (name == "AllowFeatureCreation"  ) { UPDATE_HAVE_CALLIN(AllowFeatureCreation); }
	else if (name == "AllowFeatureBuildStep" ) { UPDATE_HAVE_CALLIN(AllowFeatureBuildStep); }
	else if (name == "AllowResourceLevel"    ) { UPDATE_HAVE_CALLIN(AllowResourceLevel); }
	else if (name == "AllowResourceTransfer" ) { UPDATE_HAVE_CALLIN(AllowResourceTransfer); }
	else if (name == "AllowDirectUnitControl") { UPDATE_HAVE_CALLIN(AllowDirectUnitControl); }
	else if (name == "AllowStartPosition"    ) { UPDATE_HAVE_CALLIN(AllowStartPosition); }
	else if (name == "MoveCtrlNotify"        ) { UPDATE_HAVE_CALLIN(MoveCtrlNotify); }
	else if (name == "TerraformComplete"     ) { UPDATE_HAVE_CALLIN(TerraformComplete); }
	else if (name == "UnitPreDamaged"        ) { UPDATE_HAVE_CALLIN(UnitPreDamaged); }
	else if (name == "ShieldPreDamaged"      ) { UPDATE_HAVE_CALLIN(ShieldPreDamaged); }
	else if (name == "AllowWeaponTargetCheck") { UPDATE_HAVE_CALLIN(AllowWeaponTargetCheck); }
	else if (name == "AllowWeaponTarget"     ) { UPDATE_HAVE_CALLIN(AllowWeaponTarget); }
	else {
		return CLuaHandleSynced::SyncedUpdateCallIn(name);
	}

	#undef UPDATE_HAVE_CALLIN
	return true;
}


bool CLuaRules::UnsyncedUpdateCallIn(const string& name)
{
	     if (name == "DrawUnit"   ) { haveDrawUnit    = HasCallIn("DrawUnit"   ); }
	else if (name == "DrawFeature") { haveDrawFeature = HasCallIn("DrawFeature"); }
	else if (name == "AICallIn"   ) { haveAICallIn    = HasCallIn("AICallIn"   ); }

	return CLuaHandleSynced::UnsyncedUpdateCallIn(name);
}

/// pushes 7 items on the stack
static void PushUnitAndCommand(lua_State* L, const CUnit* unit, const Command& cmd)
{
	// push the unit info
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// push the command id
	lua_pushnumber(L, cmd.id);

	// push the params list
	lua_newtable(L);
	for (int p = 0; p < (int)cmd.params.size(); p++) {
		lua_pushnumber(L, p + 1);
		lua_pushnumber(L, cmd.params[p]);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", cmd.params.size());

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

bool CLuaRules::CommandFallback(const CUnit* unit, const Command& cmd)
{
	if (!haveCommandFallback) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);
	static const LuaHashString cmdStr("CommandFallback");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	PushUnitAndCommand(L, unit, cmd);

	// call the function
	if (!RunCallIn(cmdStr, 7, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);

	// return 'true' to remove the command
	return retval;
}


bool CLuaRules::AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced)
{
	if (!haveAllowCommand) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 10);
	static const LuaHashString cmdStr("AllowCommand");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	PushUnitAndCommand(L, unit, cmd);

	lua_pushboolean(L, fromSynced);

	// call the function
	if (!RunCallIn(cmdStr, 8, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::AllowUnitCreation(const UnitDef* unitDef,
                                  const CUnit* builder, const float3* pos)
{
	if (!haveAllowUnitCreation) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);
	static const LuaHashString cmdStr("AllowUnitCreation");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushnumber(L, unitDef->id);
	lua_pushnumber(L, builder->id);
	lua_pushnumber(L, builder->team);
	if (pos) {
		lua_pushnumber(L, pos->x);
		lua_pushnumber(L, pos->y);
		lua_pushnumber(L, pos->z);
	}

	// call the function
	if (!RunCallIn(cmdStr, pos ? 6 : 3, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}



bool CLuaRules::AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture)
{
	if (!haveAllowUnitTransfer) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	static const LuaHashString cmdStr("AllowUnitTransfer");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, newTeam);
	lua_pushboolean(L, capture);

	// call the function
	if (!RunCallIn(cmdStr, 5, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::AllowUnitBuildStep(const CUnit* builder,
                                   const CUnit* unit, float part)
{
	if (!haveAllowUnitBuildStep) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	static const LuaHashString cmdStr("AllowUnitBuildStep");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushnumber(L, builder->id);
	lua_pushnumber(L, builder->team);
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, part);

	// call the function
	if (!RunCallIn(cmdStr, 5, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::AllowFeatureCreation(const FeatureDef* featureDef,
                                     int teamID, const float3& pos)
{
	if (!haveAllowFeatureCreation) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	static const LuaHashString cmdStr("AllowFeatureCreation");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushnumber(L, featureDef->id);
	lua_pushnumber(L, teamID);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);

	// call the function
	if (!RunCallIn(cmdStr, 5, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::AllowFeatureBuildStep(const CUnit* builder,
                                      const CFeature* feature, float part)
{
	if (!haveAllowFeatureBuildStep) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	static const LuaHashString cmdStr("AllowFeatureBuildStep");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushnumber(L, builder->id);
	lua_pushnumber(L, builder->team);
	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->def->id);
	lua_pushnumber(L, part);

	// call the function
	if (!RunCallIn(cmdStr, 5, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::AllowResourceLevel(int teamID, const string& type, float level)
{
	if (!haveAllowResourceLevel) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr("AllowResourceLevel");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushnumber(L, teamID);
	lua_pushstring(L, type.c_str());
	lua_pushnumber(L, level);

	// call the function
	if (!RunCallIn(cmdStr, 3, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::AllowResourceTransfer(int oldTeam, int newTeam,
                                      const string& type, float amount)
{
	if (!haveAllowResourceTransfer) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr("AllowResourceTransfer");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushnumber(L, oldTeam);
	lua_pushnumber(L, newTeam);
	lua_pushstring(L, type.c_str());
	lua_pushnumber(L, amount);

	// call the function
	if (!RunCallIn(cmdStr, 4, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::AllowDirectUnitControl(int playerID, const CUnit* unit)
{
	if (!haveAllowDirectUnitControl) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr("AllowDirectUnitControl");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, playerID);

	// call the function
	if (!RunCallIn(cmdStr, 4, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::AllowStartPosition(int playerID, const float3& pos)
{
	if (!haveAllowStartPosition) {
		return true; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);
	static const LuaHashString cmdStr("AllowStartPosition");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	// push the start position and playerID
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushnumber(L, playerID);

	// call the function
	if (!RunCallIn(cmdStr, 4, 1)) {
		return true;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return true;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaRules::MoveCtrlNotify(const CUnit* unit, int data)
{
	if (!haveMoveCtrlNotify) {
		return false; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr("MoveCtrlNotify");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined
	}

	// push the unit info
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, data);

	// call the function
	if (!RunCallIn(cmdStr, 4, 1)) {
		return false;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return false;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);

	return retval;
}


bool CLuaRules::TerraformComplete(const CUnit* unit, const CUnit* build)
{
	if (!haveTerraformComplete) {
		return false; // the call is not defined
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);
	static const LuaHashString cmdStr("TerraformComplete");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined
	}

	// push the unit info
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// push the construction info
	lua_pushnumber(L, build->id);
	lua_pushnumber(L, build->unitDef->id);
	lua_pushnumber(L, build->team);

	// call the function
	if (!RunCallIn(cmdStr, 6, 1)) {
		return false;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return false;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);

	// return 'true' to remove the command
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
bool CLuaRules::UnitPreDamaged(const CUnit* unit, const CUnit* attacker,
                             float damage, int weaponID, bool paralyzer,
                             float* newDamage, float* impulseMult)
{
	if (!haveUnitPreDamaged) {
		return false;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 11);

	const int errfunc = SetupTraceback();
	static const LuaHashString cmdStr("UnitPreDamaged");

	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) { lua_pop(L, 1); }
		return false; // the call is not defined
	}

	int argCount = 5;
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, damage);
	lua_pushboolean(L, paralyzer);
	if (fullRead) {
		lua_pushnumber(L, weaponID);
		argCount += 1;
		if (attacker != NULL) {
			lua_pushnumber(L, attacker->id);
			lua_pushnumber(L, attacker->unitDef->id);
			lua_pushnumber(L, attacker->team);
			argCount += 3;
		}
	}

	// call the routine
	RunCallInTraceback(cmdStr, argCount, 2, errfunc);

	if (newDamage && lua_isnumber(L, -2)) {
		*newDamage = lua_tonumber(L, -2);
	} else if (!lua_isnumber(L, -2) || lua_isnil(L, -2)) {
		// first value is obligatory, so may not be nil
		logOutput.Print("%s(): 1st value returned should be a number (newDamage)\n", cmdStr.GetString().c_str());
	}

	if (impulseMult && lua_isnumber(L, -1)) {
		*impulseMult = lua_tonumber(L, -1);
	} else if (!lua_isnumber(L, -1) && !lua_isnil(L, -1)) {
		// second value is optional, so nils are OK
		logOutput.Print("%s(): 2nd value returned should be a number (impulseMult)\n", cmdStr.GetString().c_str());
	}

	lua_pop(L, 2);
	return true;
}

bool CLuaRules::ShieldPreDamaged(
	const CProjectile* projectile,
	const CWeapon* shieldEmitter,
	const CUnit* shieldCarrier,
	bool bounceProjectile
) {
	if (!haveShieldPreDamaged) {
		return false;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5 + 1);

	const int errfunc(SetupTraceback());
	static const LuaHashString cmdStr("ShieldPreDamaged");

	bool ret = false;

	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) { lua_pop(L, 1); }

		// undefined call-in
		return ret;
	}

	const CUnit* projectileOwner = projectile->owner();

	// push the call-in arguments
	lua_pushnumber(L, projectile->id);
	lua_pushnumber(L, ((projectileOwner != NULL)? projectileOwner->id: -1));
	lua_pushnumber(L, shieldEmitter->weaponNum);
	lua_pushnumber(L, shieldCarrier->id);
	lua_pushboolean(L, bounceProjectile);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 1, errfunc);

	// pop the return-value; must be true or false
	ret = (lua_isboolean(L, -1) && lua_toboolean(L, -1));
	lua_pop(L, -1);

	return ret;
}



/******************************************************************************/

bool CLuaRules::AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID)
{
	if (!haveAllowWeaponTargetCheck) {
		return false;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3 + 1);

	const int errfunc(SetupTraceback());
	static const LuaHashString cmdStr("AllowWeaponTargetCheck");

	bool ret = false;

	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) { lua_pop(L, 1); }
		return ret;
	}

	lua_pushnumber(L, attackerID);
	lua_pushnumber(L, attackerWeaponNum);
	lua_pushnumber(L, attackerWeaponDefID);

	RunCallInTraceback(cmdStr, 3, 1, errfunc);

	ret = (lua_isboolean(L, -1) && lua_toboolean(L, -1));
	lua_pop(L, -1);

	return ret;
}

bool CLuaRules::AllowWeaponTarget(
	unsigned int attackerID,
	unsigned int targetID,
	unsigned int attackerWeaponNum,
	unsigned int attackerWeaponDefID,
	float* targetPriority)
{
	if (!haveAllowWeaponTarget) {
		return false;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4 + 2);

	const int errfunc(SetupTraceback());
	static const LuaHashString cmdStr("AllowWeaponTarget");

	bool ret = false;

	if (!cmdStr.GetGlobalFunc(L)) {
		if (errfunc) { lua_pop(L, 1); }
		return ret;
	}

	lua_pushnumber(L, attackerID);
	lua_pushnumber(L, targetID);
	lua_pushnumber(L, attackerWeaponNum);
	lua_pushnumber(L, attackerWeaponDefID);

	RunCallInTraceback(cmdStr, 4, 2, errfunc);

	ret = (lua_isboolean(L, -2) && lua_toboolean(L, -2));

	if (targetPriority && lua_isnumber(L, -1)) {
		*targetPriority = lua_tonumber(L, -1);
	}

	lua_pop(L, 2);
	return ret;
}

/******************************************************************************/



bool CLuaRules::DrawUnit(int unitID)
{
	if (!haveDrawUnit) {
		return false;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("DrawUnit");
	if (!cmdStr.GetRegistryFunc(L)) {
		return false;
	}

	const bool oldGLState = LuaOpenGL::IsDrawingEnabled();
	LuaOpenGL::SetDrawingEnabled(true);

	lua_pushnumber(L, unitID);
	lua_pushnumber(L, game->GetDrawMode());

	const bool success = RunCallIn(cmdStr, 2, 1);

	LuaOpenGL::SetDrawingEnabled(oldGLState);

	if (!success) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return false;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}

bool CLuaRules::DrawFeature(int featureID)
{
	if (!haveDrawFeature) {
		return false;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr("DrawFeature");
	if (!cmdStr.GetRegistryFunc(L)) {
		return false;
	}

	const bool oldGLState = LuaOpenGL::IsDrawingEnabled();
	LuaOpenGL::SetDrawingEnabled(true);

	lua_pushnumber(L, featureID);
	lua_pushnumber(L, game->GetDrawMode());

	const bool success = RunCallIn(cmdStr, 2, 1);

	LuaOpenGL::SetDrawingEnabled(oldGLState);

	if (!success) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return false;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}



const char* CLuaRules::AICallIn(const char* data, int inSize)
{
	if (!haveAICallIn) {
		return NULL;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr("AICallIn");
	if (!cmdStr.GetRegistryFunc(L)) {
		return NULL;
	}

	int argCount = 0;
	if (data != NULL) {
		if (inSize < 0) {
			inSize = strlen(data);
		}
		lua_pushlstring(L, data, inSize);
		argCount = 1;
	}

	if (!RunCallIn(cmdStr, argCount, 1)) {
		return NULL;
	}

	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return NULL;
	}

	const char* outData = lua_tolstring(L, -1, NULL);

	lua_pop(L, 1);

	return outData;
}


/******************************************************************************/
/******************************************************************************/

int CLuaRules::UnpackCobArg(lua_State* L)
{
	if (currentCobArgs == NULL) {
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
		logOutput.Print("CLuaRules::Cob2Lua() call overflow: %s\n",
		                name.GetString().c_str());
		args[0] = 0; // failure
		return;
	}

	LUA_CALL_IN_CHECK(L);

	const int top = lua_gettop(L);

	if (!lua_checkstack(L, 1 + 3 + argsCount)) {
		logOutput.Print("CLuaRules::Cob2Lua() lua_checkstack() error: %s\n",
		                name.GetString().c_str());
		args[0] = 0; // failure
		lua_settop(L, top);
		return;
	}

	if (!name.GetGlobalFunc(L)) {
		logOutput.Print("CLuaRules::Cob2Lua() missing function: %s\n",
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
	const int* oldArgs = currentCobArgs;
	currentCobArgs = args;

	const bool error = !RunCallIn(name, 3 + argsCount, LUA_MULTRET);

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
	return;
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
	if (gs->noHelperAIs) {
		logOutput.Print("LuaRules has Disabled helper AIs");
	} else {
		logOutput.Print("LuaRules has Enabled helper AIs");
	}
	return 0;
}

/******************************************************************************/
/******************************************************************************/
