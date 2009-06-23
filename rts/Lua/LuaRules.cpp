#include "StdAfx.h"
// LuaRules.cpp: implementation of the CLuaRules class.
//
//////////////////////////////////////////////////////////////////////

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

#include "Sim/Units/CommandAI/Command.h"
#include "Game/Game.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/UnitModels/IModelParser.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/COB/CobInstance.h"
#include "LogOutput.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"
#include <assert.h>

CLuaRules* luaRules = NULL;

string CLuaRules::configString;

static const char* LuaRulesSyncedFilename   = "LuaRules/main.lua";
static const char* LuaRulesUnsyncedFilename = "LuaRules/draw.lua";

const int* CLuaRules::currentCobArgs = NULL;

vector<float>    CLuaRules::gameParams;
map<string, int> CLuaRules::gameParamsMap;


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


void CLuaRules::SetConfigString(const string& cfg)
{
	configString = cfg;
}


/******************************************************************************/
/******************************************************************************/

CLuaRules::CLuaRules()
: CLuaHandleSynced("LuaRules", LUA_HANDLE_ORDER_RULES, ".luarules ")
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
	haveDrawUnit               = HasCallIn("DrawUnit");
	haveAICallIn               = HasCallIn("AICallIn");
	haveUnitPreDamaged         = HasCallIn("UnitPreDamaged");

	SetupUnsyncedFunction("DrawUnit");
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
	LuaPushNamedCFunc(L, "GetConfigString", GetConfigString);
	LuaPushNamedCFunc(L, "PermitHelperAIs", PermitHelperAIs);
	lua_pop(L, 1);

	lua_getglobal(L, "Spring");
	LuaPushNamedCFunc(L, "SetRulesInfoMap",       SetRulesInfoMap);
	LuaPushNamedCFunc(L, "SetUnitRulesParam",     SetUnitRulesParam);
	LuaPushNamedCFunc(L, "SetTeamRulesParam",     SetTeamRulesParam);
	LuaPushNamedCFunc(L, "SetGameRulesParam",     SetGameRulesParam);
	LuaPushNamedCFunc(L, "CreateUnitRulesParams", CreateUnitRulesParams);
	LuaPushNamedCFunc(L, "CreateTeamRulesParams", CreateTeamRulesParams);
	LuaPushNamedCFunc(L, "CreateGameRulesParams", CreateGameRulesParams);
	lua_pop(L, 1);

	return true;
}


bool CLuaRules::AddUnsyncedCode()
{
	lua_pushstring(L, "UNSYNCED");
	lua_gettable(L, LUA_REGISTRYINDEX);

	lua_pushstring(L, "Script");
	lua_rawget(L, -2);
	LuaPushNamedCFunc(L, "GetConfigString", GetConfigString);
	lua_pop(L, 1); // Script

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

const vector<float>& CLuaRules::GetGameParams()
{
	return gameParams;
}


const map<string, int>& CLuaRules::GetGameParamsMap()
{
	return gameParamsMap;
}


/******************************************************************************/
/******************************************************************************/
//
// LuaRules Call-Ins
//

bool CLuaRules::SyncedUpdateCallIn(const string& name)
{
	if (name == "CommandFallback") {
		haveCommandFallback        = HasCallIn("CommandFallback");
	} else if (name == "AllowCommand") {
		haveAllowCommand           = HasCallIn("AllowCommand");
	} else if (name == "AllowUnitCreation") {
		haveAllowUnitCreation      = HasCallIn("AllowUnitCreation");
	} else if (name == "AllowUnitTransfer") {
		haveAllowUnitTransfer      = HasCallIn("AllowUnitTransfer");
	} else if (name == "AllowUnitBuildStep") {
		haveAllowUnitBuildStep     = HasCallIn("AllowUnitBuildStep");
	} else if (name == "AllowFeatureCreation") {
		haveAllowFeatureCreation   = HasCallIn("AllowFeatureCreation");
	} else if (name == "AllowFeatureBuildStep") {
		haveAllowFeatureBuildStep  = HasCallIn("AllowFeatureBuildStep");
	} else if (name == "AllowResourceLevel") {
		haveAllowResourceLevel     = HasCallIn("AllowResourceLevel");
	} else if (name == "AllowResourceTransfer") {
		haveAllowResourceTransfer  = HasCallIn("AllowResourceTransfer");
	} else if (name == "AllowDirectUnitControl") {
		haveAllowDirectUnitControl = HasCallIn("AllowDirectUnitControl");
	} else if (name == "AllowStartPosition") {
		haveAllowStartPosition     = HasCallIn("AllowStartPosition");
	} else if (name == "MoveCtrlNotify") {
		haveMoveCtrlNotify         = HasCallIn("MoveCtrlNotify");
	} else if (name == "TerraformComplete") {
		haveTerraformComplete      = HasCallIn("TerraformComplete");
	} else {
		return CLuaHandleSynced::SyncedUpdateCallIn(name);
	}
	return true;
}


bool CLuaRules::UnsyncedUpdateCallIn(const string& name)
{
	if (name == "DrawUnit") {
		haveDrawUnit = HasCallIn("DrawUnit");
	}
	else if (name == "AICallIn") {
		haveAICallIn = HasCallIn("AICallIn");
	}
	return CLuaHandleSynced::UnsyncedUpdateCallIn(name);
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
 * expects a number returned by lua code; it is stored under *newDamage if
 * newDamage != NULL.
 */
bool CLuaRules::UnitPreDamaged(const CUnit* unit, const CUnit* attacker,
                             float damage, int weaponID, bool paralyzer,
                             float* newDamage)
{
	if (!haveUnitPreDamaged) {
		return false;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 11);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr("UnitPreDamaged");
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
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
	RunCallInTraceback(cmdStr, argCount, 1, errfunc);

	// get the results
	if (!lua_isnumber(L, -1)) {
		logOutput.Print("%s() bad return value, expected number\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return false;
	}

	const float retval = lua_tonumber(L, -1);
	lua_pop(L, 1);
	if (newDamage) {
		*newDamage = retval;
	}
	return true;
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


const char* CLuaRules::AICallIn(const char* data, int inSize, int* outSize)
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

	size_t len;
	const char* outData = lua_tolstring(L, -1, &len);
	if (outSize != NULL) {
		*outSize = len;
	}

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

	if (!lua_checkstack(L, top + 1 + 3 + argsCount)) {
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

int CLuaRules::GetConfigString(lua_State* L)
{
	lua_pushlstring(L, configString.c_str(), configString.size());
	return 1;
}


/******************************************************************************/

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

int CLuaRules::SetRulesInfoMap(lua_State* L)
{
	assert(luaRules != NULL);
	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetRulesInfoMap(teamID)");
	}
	map<string, string>& infoMap = luaRules->infoMap;
	infoMap.clear();
	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2) && lua_isstring(L, -1)) {
			const string key = lua_tostring(L, -2);
			const string value = lua_tostring(L, -1);
			infoMap[key] = value;
		}
	}
	lua_pushnumber(L, infoMap.size());
	return 2;
}


/******************************************************************************/

void CLuaRules::SetRulesParam(lua_State* L, const char* caller, int offset,
                              vector<float>& params,
		                          map<string, int>& paramsMap)
{
	const int index = offset + 1;
	const int valIndex = offset + 2;
	int pIndex = -1;

	if (lua_israwnumber(L, index)) {
		pIndex = lua_toint(L, index) - 1;
	}
	else if (lua_israwstring(L, index)) {
		const string pName = lua_tostring(L, index);
		map<string, int>::const_iterator it = paramsMap.find(pName);
		if (it != paramsMap.end()) {
			pIndex = it->second;
		}
		else {
			// create a new parameter
			pIndex = params.size();
			paramsMap[pName] = params.size();
			params.push_back(0.0f); // dummy value
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	if ((pIndex < 0) || (pIndex >= (int)params.size())) {
		return;
	}

	if (!lua_isnumber(L, valIndex)) {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}
	params[pIndex] = lua_tofloat(L, valIndex);
	return;
}


void CLuaRules::CreateRulesParams(lua_State* L, const char* caller, int offset,
		                              vector<float>& params,
		                              map<string, int>& paramsMap)
{
	const int table = offset + 1;
	if (!lua_istable(L, table)) {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	params.clear();
	paramsMap.clear();

	for (int i = 1; /* no test */; i++) {
		lua_rawgeti(L, table, i);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			return;
		}
		else if (lua_israwnumber(L, -1)) {
			const float value = lua_tofloat(L, -1);
			params.push_back(value);
		}
		else if (lua_istable(L, -1)) {
			lua_pushnil(L);
			if (lua_next(L, -2)) {
				if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
					const string name = lua_tostring(L, -2);
					const float value = lua_tonumber(L, -1);
					paramsMap[name] = params.size();
					params.push_back(value);
				}
				lua_pop(L, 2);
			}
		}
		lua_pop(L, 1);
	}

	return;
}


/******************************************************************************/

int CLuaRules::SetGameRulesParam(lua_State* L)
{
	CLuaRules* lr = (CLuaRules*)activeHandle;
	SetRulesParam(L, __FUNCTION__, 0, lr->gameParams, lr->gameParamsMap);
	return 0;
}


int CLuaRules::CreateGameRulesParams(lua_State* L)
{
	CLuaRules* lr = (CLuaRules*)activeHandle;
	CreateRulesParams(L, __FUNCTION__, 0, lr->gameParams, lr->gameParamsMap);
	return 0;
}


/******************************************************************************/

static CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad teamID type\n", caller);
	}
	const int teamID = lua_toint(L, index);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		luaL_error(L, "%s(): Bad teamID: %i\n", teamID);
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return NULL;
	}
	const CLuaHandle* lh = CLuaHandle::GetActiveHandle();
	const int ctrlTeam = lh->GetCtrlTeam();
	if (ctrlTeam < 0) {
		return lh->GetFullCtrl() ? team : NULL;
	}
	if (ctrlTeam != teamID) {
		luaL_error(L, "%s(): No permission to control team %i\n", caller, teamID);
	}
	return team;
}


int CLuaRules::SetTeamRulesParam(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	SetRulesParam(L, __FUNCTION__, 1, team->modParams, team->modParamsMap);
	return 0;
}


int CLuaRules::CreateTeamRulesParams(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	CreateRulesParams(L, __FUNCTION__, 1, team->modParams, team->modParamsMap);
	return 0;
}


/******************************************************************************/

static CUnit* ParseUnit(lua_State* L, const char* caller, int luaIndex)
{
	if (!lua_isnumber(L, luaIndex)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = lua_toint(L, luaIndex);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	const CLuaHandle* lh = CLuaHandle::GetActiveHandle();
	const int ctrlTeam = lh->GetCtrlTeam();
	if (ctrlTeam < 0) {
		return lh->GetFullCtrl() ? unit : NULL;
	}
	if (ctrlTeam != unit->team) {
		luaL_error(L, "%s(): No permission to control unit %i\n", caller, unitID);
	}
	return unit;
}


int CLuaRules::SetUnitRulesParam(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	SetRulesParam(L, __FUNCTION__, 1, unit->modParams, unit->modParamsMap);
	return 0;
}


int CLuaRules::CreateUnitRulesParams(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	CreateRulesParams(L, __FUNCTION__, 1, unit->modParams, unit->modParamsMap);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
