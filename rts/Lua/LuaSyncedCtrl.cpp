#include "StdAfx.h"
// LuaSyncedCtrl.cpp: implementation of the CLuaSyncedCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaSyncedCtrl.h"
#include <set>
#include <list>
#include <cctype>

#include "LuaInclude.h"

#include "LuaRules.h" // for MAX_LUA_COB_ARGS
#include "LuaHandleSynced.h"
#include "LuaHashString.h"
#include "LuaSyncedMoveCtrl.h"
#include "LuaUtils.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Game/Game.h"
#include "Game/GameServer.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/UnitTypes/ExtractorBuilding.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/myMath.h"
#include "System/LogOutput.h"

using namespace std;


/******************************************************************************/
/******************************************************************************/

bool LuaSyncedCtrl::inCreateUnit = false;
bool LuaSyncedCtrl::inDestroyUnit = false;
bool LuaSyncedCtrl::inTransferUnit = false;
bool LuaSyncedCtrl::inCreateFeature = false;
bool LuaSyncedCtrl::inDestroyFeature = false;
bool LuaSyncedCtrl::inGiveOrder = false;


/******************************************************************************/

inline void LuaSyncedCtrl::CheckAllowGameChanges(lua_State* L)
{
	const CLuaHandleSynced* lhs =
		dynamic_cast<const CLuaHandleSynced*>(CLuaHandle::GetActiveHandle());

	if (lhs == NULL) {
		luaL_error(L, "Internal lua error, unsynced script using synced calls");
	}
	if (!lhs->GetAllowChanges()) {
		luaL_error(L, "Unsafe attempt to change game state");
	}
}


/******************************************************************************/
/******************************************************************************/

bool LuaSyncedCtrl::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(AddTeamResource);
	REGISTER_LUA_CFUNC(UseTeamResource);
	REGISTER_LUA_CFUNC(SetTeamResource);
	REGISTER_LUA_CFUNC(SetTeamShareLevel);

	REGISTER_LUA_CFUNC(CreateUnit);
	REGISTER_LUA_CFUNC(DestroyUnit);
	REGISTER_LUA_CFUNC(TransferUnit);

	REGISTER_LUA_CFUNC(CreateFeature);
	REGISTER_LUA_CFUNC(DestroyFeature);
	REGISTER_LUA_CFUNC(TransferFeature);

	REGISTER_LUA_CFUNC(SetUnitCosts);
	REGISTER_LUA_CFUNC(SetUnitResourcing);
	REGISTER_LUA_CFUNC(SetUnitTooltip);
	REGISTER_LUA_CFUNC(SetUnitHealth);
	REGISTER_LUA_CFUNC(SetUnitMaxHealth);
	REGISTER_LUA_CFUNC(SetUnitStockpile);
	REGISTER_LUA_CFUNC(SetUnitWeaponState);
	REGISTER_LUA_CFUNC(SetUnitExperience);
	REGISTER_LUA_CFUNC(SetUnitCloak);
	REGISTER_LUA_CFUNC(SetUnitStealth);
	REGISTER_LUA_CFUNC(SetUnitAlwaysVisible);
	REGISTER_LUA_CFUNC(SetUnitMetalExtraction);
	REGISTER_LUA_CFUNC(SetUnitBuildSpeed);
	REGISTER_LUA_CFUNC(SetUnitBlocking);
	REGISTER_LUA_CFUNC(SetUnitShieldState);
	REGISTER_LUA_CFUNC(SetUnitTravel);
	REGISTER_LUA_CFUNC(SetUnitMoveGoal);
	REGISTER_LUA_CFUNC(SetUnitLineage);
	REGISTER_LUA_CFUNC(SetUnitNeutral);
	REGISTER_LUA_CFUNC(SetUnitTarget);
	REGISTER_LUA_CFUNC(SetUnitCollisionVolumeData);

	REGISTER_LUA_CFUNC(SetUnitPhysics);
	REGISTER_LUA_CFUNC(SetUnitPosition);
	REGISTER_LUA_CFUNC(SetUnitVelocity);
	REGISTER_LUA_CFUNC(SetUnitRotation);

	REGISTER_LUA_CFUNC(AddUnitDamage);
	REGISTER_LUA_CFUNC(AddUnitImpulse);

	REGISTER_LUA_CFUNC(AddUnitResource);
	REGISTER_LUA_CFUNC(UseUnitResource);

	REGISTER_LUA_CFUNC(RemoveBuildingDecal);

	REGISTER_LUA_CFUNC(SetFeatureHealth);
	REGISTER_LUA_CFUNC(SetFeatureReclaim);
	REGISTER_LUA_CFUNC(SetFeatureResurrect);
	REGISTER_LUA_CFUNC(SetFeaturePosition);
	REGISTER_LUA_CFUNC(SetFeatureDirection);
	REGISTER_LUA_CFUNC(SetFeatureNoSelect);
	REGISTER_LUA_CFUNC(SetFeatureCollisionVolumeData);

	REGISTER_LUA_CFUNC(CallCOBScript);
	REGISTER_LUA_CFUNC(CallCOBScriptCB);
	REGISTER_LUA_CFUNC(GetCOBScriptID);
	REGISTER_LUA_CFUNC(GetUnitCOBValue);
	REGISTER_LUA_CFUNC(SetUnitCOBValue);

	REGISTER_LUA_CFUNC(GiveOrderToUnit);
	REGISTER_LUA_CFUNC(GiveOrderToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderToUnitArray);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitArray);


	REGISTER_LUA_CFUNC(LevelHeightMap);
	REGISTER_LUA_CFUNC(AdjustHeightMap);
	REGISTER_LUA_CFUNC(RevertHeightMap);

	REGISTER_LUA_CFUNC(EditUnitCmdDesc);
	REGISTER_LUA_CFUNC(InsertUnitCmdDesc);
	REGISTER_LUA_CFUNC(RemoveUnitCmdDesc);

	REGISTER_LUA_CFUNC(SetNoPause);
	REGISTER_LUA_CFUNC(SetUnitToFeature);
	REGISTER_LUA_CFUNC(SetExperienceGrade);

	if (!LuaSyncedMoveCtrl::PushMoveCtrl(L)) {
		return false;
	}

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

static inline bool FullCtrl()
{
	return CLuaHandle::GetActiveHandle()->GetFullCtrl();
}


static inline int CtrlTeam()
{
	return CLuaHandle::GetActiveHandle()->GetCtrlTeam();
}


static inline int CtrlAllyTeam()
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return ctrlTeam;
	}
	return gs->AllyTeam(ctrlTeam);
}


static inline bool CanControlTeam(int teamID)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CLuaHandle::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == teamID);
}


static inline bool CanControlUnit(const CUnit* unit)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CLuaHandle::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == unit->team);
}


static inline bool CanControlFeatureAllyTeam(int allyTeam)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CLuaHandle::AllAccessTeam) ? true : false;
	}
	if (allyTeam < 0) {
		return (ctrlTeam == gs->gaiaTeamID);
	}
	return (gs->AllyTeam(ctrlTeam) == allyTeam);
}


static inline bool CanControlFeature(const CFeature* feature)
{
	return CanControlFeatureAllyTeam(feature->allyteam);
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = (int)lua_tonumber(L, index);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		luaL_error(L, "%s(): Bad unitID: %d\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	return unit;
}


static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	if (!CanControlUnit(unit)) {
		return NULL;
	}
	return unit;
}


static inline CFeature* ParseFeature(lua_State* L,
                                     const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "Incorrect arguments to %s(featureID)", caller);
	}
	const int featureID = (int)lua_tonumber(L, index);
	const CFeatureSet& fset = featureHandler->GetActiveFeatures();
	CFeatureSet::const_iterator it = fset.find(featureID);

	if (it == fset.end())
		return NULL;

	if (!CanControlFeature(*it)) {
		return NULL;
	}
	return *it;
}


static CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad teamID", caller);
		return NULL;
	}
	const int teamID = (int)lua_tonumber(L, index);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		luaL_error(L, "%s(): Bad teamID: %d", caller, teamID);
	}
	CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return NULL;
	}
	return team;
}


static CUnit* CheckUnitID(lua_State* L, int index)
{
	luaL_checknumber(L, index);
	const int unitID = (int)lua_tonumber(L, index);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		luaL_error(L, "Bad unitID: %d\n", unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		luaL_error(L, "Bad unitID: %d\n", unitID);
	}
	return unit;
}


static CPlayer* CheckPlayerID(lua_State* L, int index)
{
	luaL_checknumber(L, index);
	const int playerID = (int)lua_tonumber(L, index);
	if ((playerID < 0) || (playerID >= MAX_PLAYERS)) {
		luaL_error(L, "Bad playerID: %d\n", playerID);
	}
	CPlayer* player = gs->players[playerID];
	if (player == NULL) {
		luaL_error(L, "Bad playerID: %d\n", playerID);
	}
	return player;
}


static int ParseFloatArray(lua_State* L, float* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	int index = 0;
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -1)) {
			logOutput.Print("LUA: error parsing numeric array\n");
			lua_pop(L, 2); // pop the value and the key
			return -1;
		}
		if (index < size) {
			array[index] = (float)lua_tonumber(L, -1);
			index++;
		}
	}
	return index;
}


static int ParseFacing(lua_State* L, const char* caller, int index)
{
	if (lua_isnumber(L, index)) {
		return (int)lua_tonumber(L, index);
	}
	else if (lua_isstring(L, index)) {
		const string dir = StringToLower(lua_tostring(L, index));
		if (dir == "s") { return 0; }
		if (dir == "e") { return 1; }
		if (dir == "n") { return 2; }
		if (dir == "w") { return 3; }
		luaL_error(L, "%s(): bad facing string", caller);
	}
	luaL_error(L, "%s(): bad facing parameter", caller);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
//
// The call-outs
//

int LuaSyncedCtrl::AddTeamResource(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to AddTeamResource()");
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	if (!CanControlTeam(teamID)) {
		return 0;
	}
	CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	const string type = lua_tostring(L, 2);

	const float value = max(0.0f, float(lua_tonumber(L, 3)));

	if ((type == "m") || (type == "metal")) {
		team->AddMetal(value);
	}
	else if ((type == "e") || (type == "energy")) {
		team->AddEnergy(value);
	}
	return 0;
}


int LuaSyncedCtrl::UseTeamResource(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to UseTeamResource()");
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	if (!CanControlTeam(teamID)) {
		return 0;
	}
	CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	if (lua_isstring(L, 2)) {
		const string type = lua_tostring(L, 2);

		const float value = max(0.0f, float(lua_tonumber(L, 3)));

		if ((type == "m") || (type == "metal")) {
			team->metalPull += value;
			lua_pushboolean(L, team->UseMetal(value));
			return 1;
		}
		else if ((type == "e") || (type == "energy")) {
			team->energyPull += value;
			lua_pushboolean(L, team->UseEnergy(value));
			return 1;
		}
	}
	else if (lua_istable(L, 2)) {
		float metal  = 0.0f;
		float energy = 0.0f;
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				const string key = lua_tostring(L, -2);
				const float value = max(0.0f, float(lua_tonumber(L, -1)));
				if ((key == "m") || (key == "metal")) {
					metal = value;
				} else if ((key == "e") || (key == "energy")) {
					energy = value;
				}
			}
		}
		team->metalPull  += metal;
		team->energyPull += energy;
		if ((team->metal >= metal) && (team->energy >= energy)) {
			team->UseMetal(metal);
			team->UseEnergy(energy);
			lua_pushboolean(L, true);
		} else {
			lua_pushboolean(L, false);
		}
		return 1;
	}
	return 0;
}


int LuaSyncedCtrl::SetTeamResource(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to SetTeamResource()");
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	if (!CanControlTeam(teamID)) {
		return 0;
	}
	CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	const string type = lua_tostring(L, 2);

	const float value = max(0.0f, (float)lua_tonumber(L, 3));

	if ((type == "m") || (type == "metal")) {
		team->metal = min(team->metalStorage, value);
	}
	else if ((type == "e") || (type == "energy")) {
		team->energy = min(team->energyStorage, value);
	}
	else if ((type == "ms") || (type == "metalStorage")) {
		team->metalStorage = value;
		team->metal = min(team->metal, team->metalStorage);
	}
	else if ((type == "es") || (type == "energyStorage")) {
		team->energyStorage = value;
		team->energy = min(team->energy, team->energyStorage);
	}
	return 0;
}


int LuaSyncedCtrl::SetTeamShareLevel(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to SetTeamShareLevel()");
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	if (!CanControlTeam(teamID)) {
		return 0;
	}
	CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	const string type = lua_tostring(L, 2);

	const float value = (float)lua_tonumber(L, 3);

	if ((type == "m") || (type == "metal")) {
		team->metalShare = max(0.0f, min(1.0f, value));
	}
	else if ((type == "e") || (type == "energy")) {
		team->energyShare = max(0.0f, min(1.0f, value));
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

static inline void ParseCobArgs(lua_State* L,
                                int first, int last, vector<int>& args)
{
	for (int a = first; a <= last; a++) {
		if (lua_isnumber(L, a)) {
			args.push_back((int)lua_tonumber(L, a));
		}
		else if (lua_istable(L, a)) {
			lua_rawgeti(L, a, 1);
			lua_rawgeti(L, a, 2);
			if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
				const int x = (int)lua_tonumber(L, -2);
				const int z = (int)lua_tonumber(L, -1);
				args.push_back(PACKXZ(x, z));
			} else {
				args.push_back(0);
			}
			lua_pop(L, 2);
		}
		else if (lua_isboolean(L, a)) {
			args.push_back(lua_toboolean(L, a) ? 1 : 0);
		}
		else {
			args.push_back(0);
		}
	}
}


int LuaSyncedCtrl::CallCOBScript(lua_State* L)
{
//FIXME?	CheckAllowGameChanges(L);
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || // unitID
	    !lua_isnumber(L, 3)) { // number of returned parameters
		luaL_error(L, "Incorrect arguments to CallCOBScript()");
	}

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (unit->cob == NULL) {
		return 0;
	}

	// collect the arguments
	vector<int> cobArgs;
	ParseCobArgs(L, 4, args, cobArgs);
	const int retParams = min((int)lua_tonumber(L, 3),
	                          min(MAX_LUA_COB_ARGS, (int)cobArgs.size()));

	int retval;
	if (lua_isnumber(L, 2)) {
		const int funcId = (int)lua_tonumber(L, 2);
		retval = unit->cob->RawCall(funcId, cobArgs);
	}
	else if (lua_isstring(L, 2)) {
		const string funcName = lua_tostring(L, 2);
		retval = unit->cob->Call(funcName, cobArgs);
	}
	else {
		luaL_error(L, "Incorrect arguments to CallCOBScript()");
		retval = 0;
	}

	lua_settop(L, 0); // FIXME - ok?
	lua_pushnumber(L, retval);
	for (int i = 0; i < retParams; i++) {
		lua_pushnumber(L, cobArgs[i]);
	}
	return 1 + retParams;
}


int LuaSyncedCtrl::CallCOBScriptCB(lua_State* L)
{
//FIXME?	CheckAllowGameChanges(L);
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 1) || // unitID
	    !lua_isnumber(L, 3) || // number of returned parameters
	    !lua_isnumber(L, 4)) { // callback data (float)
		luaL_error(L, "Incorrect arguments to CallCOBScriptCB()");
	}

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (unit->cob == NULL) {
		return 0;
	}

	// collect the arguments
	vector<int> cobArgs;
	ParseCobArgs(L, 5, args, cobArgs);
	const int retParams = min((int)lua_tonumber(L, 3),
	                          min(MAX_LUA_COB_ARGS, (int)cobArgs.size()));

	// callback data
	float cbData = (float)lua_tonumber(L, 4);

	int retval;
	if (lua_isnumber(L, 2)) {
		const int funcId = (int)lua_tonumber(L, 2);
		retval = unit->cob->RawCall(funcId, cobArgs,
		                            CLuaHandle::GetActiveCallback(),
		                            (void*) unit->id, (void*) *((int*)&cbData));
	}
	else if (lua_isstring(L, 2)) {
		const string funcName = lua_tostring(L, 2);
		retval = unit->cob->Call(funcName, cobArgs,
		                         CLuaHandle::GetActiveCallback(),
			                       (void*) unit->id, (void*) *((int*)&cbData));
	}
	else {
		luaL_error(L, "Incorrect arguments to CallCOBScriptCB()");
		retval = 0;
	}

	lua_settop(L, 0); // FIXME - ok?
	lua_pushnumber(L, retval);
	for (int i = 0; i < retParams; i++) {
		lua_pushnumber(L, cobArgs[i]);
	}
	return 1 + retParams;
}


int LuaSyncedCtrl::GetCOBScriptID(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetCOBScriptID()");
	}

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (unit->cob == NULL) {
		return 0;
	}

	const string funcName = lua_tostring(L, 2);

	const int funcID = unit->cob->GetFunctionId(funcName);
	if (funcID >= 0) {
		lua_pushnumber(L, funcID);
		return 1;
	}

	return 0;
}


int LuaSyncedCtrl::GetUnitCOBValue(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->cob == NULL)) {
		return 0;
	}

	int arg = 2;
	bool splitData = false;
	if (lua_isboolean(L, arg)) {
		splitData = lua_toboolean(L, arg);
		arg++;
	}

	const int val = (int)luaL_checknumber(L, arg); arg++;

	int p[4];
	for (int a = 0; a < 4; a++, arg++) {
		if (lua_istable(L, arg)) {
			int x, z;
			lua_rawgeti(L, arg, 1); x = luaL_checkint(L, -1); lua_pop(L, 1);
			lua_rawgeti(L, arg, 2); z = luaL_checkint(L, -1); lua_pop(L, 1);
			p[a] = PACKXZ(x, z);
		}
		else {
			p[a] = (int)luaL_optnumber(L, arg, 0);
		}
	}

	const int result = unit->cob->GetUnitVal(val, p[0], p[1], p[2], p[3]);
	if (!splitData) {
		lua_pushnumber(L, result);
		return 1;
	}
	lua_pushnumber(L, UNPACKX(result));
	lua_pushnumber(L, UNPACKZ(result));
	return 2;
}


int LuaSyncedCtrl::SetUnitCOBValue(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->cob == NULL)) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	const int val = (int)luaL_checknumber(L, 2);
	int param;
	if (args == 3) {
		param = (int)luaL_checknumber(L, 3);
	}
	else {
		const int x = (int)luaL_checknumber(L, 3);
		const int z = (int)luaL_checknumber(L, 4);
		param = PACKXZ(x, z);
	}
	unit->cob->SetUnitVal(val, param);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::CreateUnit(lua_State* L)
{
	CheckAllowGameChanges(L);
	const int args = lua_gettop(L); // number of arguments
	if ((args < 5) ||
	    !lua_isstring(L, 1) || // name, pos, facing[, teamID]
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) ||
	    !(lua_isnumber(L, 5) || lua_isstring(L, 5))) {
		luaL_error(L, "Incorrect arguments to CreateUnit()");
	}

	const string& defName = lua_tostring(L, 1);
	const UnitDef* unitDef = unitDefHandler->GetUnitByName(defName);
	if (unitDef == NULL) {
		luaL_error(L, "CreateUnit() bad unit name: %s", defName.c_str());
	}

	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	const int facing = ParseFacing(L, __FUNCTION__, 5);

	int teamID = CtrlTeam();
	if ((args >= 6) && lua_isnumber(L, 6)) {
		teamID = (int)lua_tonumber(L, 6);
	}
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		luaL_error(L, "CreateUnit(): bad team number: %d", teamID);
	}

	if (gs->AllyTeam(teamID) >= gs->activeAllyTeams) {
		// FIXME: there's a segv in CLosHandler::LosAddAir,
		//        this is a dirty hack to avoid it
		luaL_error(L, "CreateUnit(): inactive team: %d", teamID);
	}


	if (!FullCtrl() && (CtrlTeam() != teamID)) {
		luaL_error(L, "Error in CreateUnit(), bad team %d", teamID);
		return 0;
	}

	if (uh->unitsByDefs[teamID][unitDef->id].size() >= unitDef->maxThisUnit) {
		return 0; // unit limit reached
	}

	// FIXME -- allow specifying the 'build' and 'builder' parameters?

	if (inCreateUnit) {
		luaL_error(L, "CreateUnit() recursion is not permitted");
	}
	inCreateUnit = true;
	CUnit* unit = unitLoader.LoadUnit(defName, pos, teamID, false, facing, NULL);
	inCreateUnit = false;

	if (unit) {
		unitLoader.FlattenGround(unit);
		lua_pushnumber(L, unit->id);
		return 1;
	}

	return 0;
}


int LuaSyncedCtrl::DestroyUnit(lua_State* L)
{
	CheckAllowGameChanges(L); // FIXME -- recursion protection
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments

	bool selfd = false;
	if ((args >= 2) && lua_isboolean(L, 2)) {
		selfd = lua_toboolean(L, 2);
	}
	bool reclaimed = false;
	if ((args >= 3) && lua_isboolean(L, 3)) {
		reclaimed = lua_toboolean(L, 3);
	}

	CUnit* attacker = NULL;
	if (args >= 4) {
		attacker = ParseUnit(L, __FUNCTION__, 4);
	}

	if (inDestroyUnit) {
		luaL_error(L, "DestroyUnit() recursion is not permitted");
	}
	inDestroyUnit = true;
	unit->KillUnit(selfd, reclaimed, attacker);
	inDestroyUnit = false;

	return 0;
}


int LuaSyncedCtrl::TransferUnit(lua_State* L)
{
	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to TransferUnit()");
	}

	const int newTeam = (int)lua_tonumber(L, 2);
	if ((newTeam < 0) || (newTeam >= gs->activeTeams)) {
		return 0;
	}
	const CTeam* team = gs->Team(newTeam);
	if (team == NULL) {
		return 0;
	}

	bool given = true;
	if (FullCtrl()) {
		if ((args >= 3) && lua_isboolean(L, 3)) {
			given = lua_toboolean(L, 3);
		}
	}

	if (inTransferUnit) {
		luaL_error(L, "TransferUnit() recursion is not permitted");
	}
	inTransferUnit = true;
	unit->ChangeTeam(newTeam, given ? CUnit::ChangeGiven : CUnit::ChangeCaptured);
	inTransferUnit = false;

	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::SetUnitCosts(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_istable(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitCosts");
	}
	const int table = 2;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1)) {
			continue;
		}
		const string key = lua_tostring(L, -2);
		const float value = lua_tonumber(L, -1);
		if (key == "buildTime") {
			unit->buildTime  = max(1.0f, value);
		} else if (key == "metalCost") {
			unit->metalCost  = max(1.0f, value);
		} else if (key == "energyCost") {
			unit->energyCost = max(1.0f, value);
		}
	}
	return 0;
}


static bool SetUnitResourceParam(CUnit* unit, const string& name, float value)
{
	if (name.size() != 3) {
		return false;
	}
	// [u|c][u|m][m|e]
	//
	// unconditional | conditional
	//           use | make
	//         metal | energy

	value *= 0.25f;

	if (name[0] == 'u') {
		if (name[1] == 'u') {
					 if (name[2] == 'm') { unit->uncondUseMetal = value;  return true; }
			else if (name[2] == 'e') { unit->uncondUseEnergy = value; return true; }
		}
		else if (name[1] == 'm') {
					 if (name[2] == 'm') { unit->uncondMakeMetal = value;  return true; }
			else if (name[2] == 'e') { unit->uncondMakeEnergy = value; return true; }
		}
	}
	else if (name[0] == 'c') {
		if (name[1] == 'u') {
					 if (name[2] == 'm') { unit->condUseMetal = value;  return true; }
			else if (name[2] == 'e') { unit->condUseEnergy = value; return true; }
		}
		else if (name[1] == 'm') {
					 if (name[2] == 'm') { unit->condMakeMetal = value;  return true; }
			else if (name[2] == 'e') { unit->condMakeEnergy = value; return true; }
		}
	}
	return false;
}


int LuaSyncedCtrl::SetUnitResourcing(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args == 3) && lua_isstring(L, 2) && lua_isnumber(L, 3)) {
		const string key = lua_tostring(L, 2);
		const float value = lua_tonumber(L, 3);
		SetUnitResourceParam(unit, key, value);
	}
	else if ((args == 2) && lua_istable(L, 2)) {
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1)) {
				continue;
			}
			const string key = lua_tostring(L, -2);
			const float value = lua_tonumber(L, -1);

			SetUnitResourceParam(unit, key, value);
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to SetUnitResourcing");
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitTooltip(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitTooltip()");
	}
	unit->tooltip = lua_tostring(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitHealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (args < 2) {
		luaL_error(L, "Incorrect arguments to SetUnitHealth()");
	}

	if (lua_isnumber(L, 2)) {
		float health = (float)lua_tonumber(L, 2);
		health = min(unit->maxHealth, health);
		unit->health = health;
	}
	else if (lua_istable(L, 2)) {
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				const string key = lua_tostring(L, -2);
				const float value = (float)lua_tonumber(L, -1);
				if (key == "health") {
					unit->health = min(unit->maxHealth, value);
				}
				else if (key == "capture") {
					unit->captureProgress = value;
				}
				else if (key == "paralyze") {
					unit->paralyzeDamage = max(0.0f, value);
					if (unit->paralyzeDamage >= unit->health) {
						unit->stunned = true;
					} else if (value < 0.0f) {
						unit->stunned = false;
					}
				}
				else if (key == "build") {
					unit->buildProgress = value;
					if (unit->beingBuilt && (value >= 1.0f)) {
						unit->FinishedBuilding();
					}
				}
			}
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to SetUnitHealth()");
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitMaxHealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	unit->maxHealth = (float)luaL_checknumber(L, 2);
	if (unit->maxHealth <= 0.0f) {
		unit->maxHealth = 1.0f;
	}

	if (unit->health > unit->maxHealth) {
		unit->health = unit->maxHealth;
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitStockpile(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	CWeapon* w = unit->stockpileWeapon;
	if (w == NULL) {
		return 0;
	}

	if (lua_isnumber(L, 2)) {
		w->numStockpiled = max(0, luaL_checkint(L, 2));
		unit->commandAI->UpdateStockpileIcon();
	}

	if (lua_isnumber(L, 3)) {
		const float percent = max(0.0f, min(1.0f, (float)lua_tonumber(L, 3)));
		unit->stockpileWeapon->buildPercent = percent;
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitWeaponState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const int weaponNum = (int)luaL_checknumber(L,2);
	if ((weaponNum < 0) || (weaponNum >= unit->weapons.size())) {
		return 0;
	}
	CWeapon* weapon = unit->weapons[weaponNum];

	if (lua_istable(L,3)) {
		const int table = 3;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				const string key = lua_tostring(L, -2);
				const float value = (float)lua_tonumber(L, -1);
				// FIXME: KDR -- missing checks and updates?
				if (key == "reloadstate") {
					weapon->reloadStatus = (int)value;
				}
				else if (key == "reloadtime") {
					weapon->reloadTime = (int)(value * GAME_SPEED);
				}
				else if (key == "accuracy") {
					weapon->accuracy = value;
				}
				else if (key == "sprayangle") {
					weapon->sprayangle = value;
				}
				else if (key == "range") {
					weapon->range = value;
				}
				else if (key == "projectilespeed") {
					weapon->projectileSpeed = value;
				}
			}
		}
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitExperience(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float experience = max(0.0f, (float)luaL_checknumber(L, 2));
	unit->AddExperience(experience - unit->experience);
	return 0;
}


int LuaSyncedCtrl::SetUnitCloak(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (args < 2) {
		luaL_error(L, "Incorrect arguments to SetUnitCloak()");
	}
	if (lua_isboolean(L, 2)) {
		unit->scriptCloak = lua_toboolean(L, 2) ? 1 : 0;
	} else if (lua_isnumber(L, 2)) {
		unit->scriptCloak = (int)lua_tonumber(L, 2);
	} else if (!lua_isnil(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitCloak()");
	}

	if (lua_israwnumber(L, 3)) {
		unit->decloakDistance = (float)lua_tonumber(L, 3);
	}
	else if (lua_isboolean(L, 3)) {
		const float defDist = unit->unitDef->decloakDistance;
		if (lua_toboolean(L, 3)) {
			unit->decloakDistance = streflop::fabsf(defDist);
		} else {
			unit->decloakDistance = defDist;
		}
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitStealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitStealth()");
	}
	unit->stealth = lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitAlwaysVisible(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitAlwaysVisible()");
	}
	unit->alwaysVisible = lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitMetalExtraction(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	CExtractorBuilding* mex = dynamic_cast<CExtractorBuilding*>(unit);
	if (mex == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitMetalExtraction()");
	}
	const float depth = (float)lua_tonumber(L, 2);
	float range = mex->GetExtractionRange();
	if ((args >= 3) && lua_isnumber(L, 3)) {
		range = (float)lua_tonumber(L, 3);
	}
	mex->ResetExtraction();
	mex->SetExtractionRangeAndDepth(range, depth);
	return 0;
}


int LuaSyncedCtrl::SetUnitBuildSpeed(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitBuildSpeed()");
	}
	const float buildScale = (1.0f / 32.0f);
	const float buildSpeed = buildScale * max(0.0f, (float)lua_tonumber(L, 2));
	CFactory* factory = dynamic_cast<CFactory*>(unit);
	if (factory) {
		factory->buildSpeed = buildSpeed;
		return 0;
	}
	CBuilder* builder = dynamic_cast<CBuilder*>(unit);
	if (!builder) {
		return 0;
	}
	builder->buildSpeed = buildSpeed;
	if ((args >= 3) && lua_isnumber(L, 3)) {
		builder->repairSpeed    = buildScale * max(0.0f, (float)lua_tonumber(L, 3));
	}
	if ((args >= 4) && lua_isnumber(L, 4)) {
		builder->reclaimSpeed   = buildScale * max(0.0f, (float)lua_tonumber(L, 4));
	}
	if ((args >= 5) && lua_isnumber(L, 5)) {
		builder->resurrectSpeed = buildScale * max(0.0f, (float)lua_tonumber(L, 5));
	}
	if ((args >= 6) && lua_isnumber(L, 6)) {
		builder->captureSpeed   = buildScale * max(0.0f, (float)lua_tonumber(L, 6));
	}
	if ((args >= 7) && lua_isnumber(L, 7)) {
		builder->terraformSpeed = buildScale * max(0.0f, (float)lua_tonumber(L, 7));
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitBlocking(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitBlocking()");
	}
	if (lua_toboolean(L, 2)) {
		unit->Block();
	} else {
		unit->UnBlock();
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitShieldState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	CPlasmaRepulser* shield = (CPlasmaRepulser*)unit->shieldWeapon;
	if (shield == NULL) {
		return 0;
	}
	if (lua_isboolean(L, 2)) {
		shield->isEnabled = lua_toboolean(L, 2);
	}
	if (lua_isnumber(L, 3)) {
		shield->curPower = (float)lua_tonumber(L, 3);
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitTravel(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (lua_isnumber(L, 2)) {
		unit->travel = (float)lua_tonumber(L, 2);
	}
	if (lua_isnumber(L, 3)) {
		unit->travelPeriod = (float)lua_tonumber(L, 3);
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitLineage(lua_State* L)
{
	if (!FullCtrl()) {
		return 0;
	}
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	CTeam* team = ParseTeam(L, __FUNCTION__, 2);
	if (team == NULL) {
		return 0;
	}
	unit->lineage = team->teamNum;
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		team->lineageRoot = unit->id;
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitNeutral(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (lua_isboolean(L, 2)) {
		unit->neutral = lua_toboolean(L, 2);
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitTarget(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L);
	if (args >= 4) {
		const float3 pos((float)lua_tonumber(L, 2),
		                 (float)lua_tonumber(L, 3),
		                 (float)lua_tonumber(L, 4));
		const bool dgun = lua_isboolean(L, 5) && lua_toboolean(L, 5);
		unit->AttackGround(pos, dgun);
	}
	else if (args >= 2) {
		CUnit* target = ParseRawUnit(L, __FUNCTION__, 2);
		const bool dgun = lua_isboolean(L, 3) && lua_toboolean(L, 3);
		unit->AttackUnit(target, dgun);
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitCollisionVolumeData(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL) {
		return 0;
	}
	if (unit->collisionVolume == NULL) {
		return 0;
	}

	const int args = lua_gettop(L);
	if (args == 10) {
		const float xs = float(lua_tonumber(L, 2));
		const float ys = float(lua_tonumber(L, 3));
		const float zs = float(lua_tonumber(L, 4));
		const float xo = float(lua_tonumber(L, 5));
		const float yo = float(lua_tonumber(L, 6));
		const float zo = float(lua_tonumber(L, 7));
		const int vType = int(lua_tonumber(L,  8));
		const int tType = int(lua_tonumber(L,  9));
		const int pAxis = int(lua_tonumber(L, 10));

		const float3 scales(xs, ys, zs);
		const float3 offsets(xo, yo, zo);

		unit->collisionVolume->Init(scales, offsets, vType, tType, pAxis);
	} else {
		luaL_error(L, "Incorrect arguments to SetUnitCollisionVolumeData()");
	}

	return 0;
}



int LuaSyncedCtrl::SetUnitMoveGoal(lua_State* L)
{
	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (unit->moveType == NULL) {
		return 0;
	}
	const float3 pos((float)luaL_checknumber(L, 2),
									 (float)luaL_checknumber(L, 3),
									 (float)luaL_checknumber(L, 4));
	const float radius = (float)luaL_optnumber(L, 5, 0.0f);
	const float speed  = (float)luaL_optnumber(L, 6, unit->maxSpeed * 2.0f);

	unit->moveType->StartMoving(pos, radius, speed);

	return 0;
}


int LuaSyncedCtrl::SetUnitPhysics(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	float3& pos = unit->pos;
	pos.x = luaL_checknumber(L, 2);
	pos.y = luaL_checknumber(L, 3);
	pos.z = luaL_checknumber(L, 4);
	float3& vel = unit->speed;
	vel.x = luaL_checknumber(L, 5);
	vel.y = luaL_checknumber(L, 6);
	vel.z = luaL_checknumber(L, 7);
	float3  rot;
	rot.x = luaL_checknumber(L, 8);
	rot.y = luaL_checknumber(L, 9);
	rot.z = luaL_checknumber(L, 10);

	CMatrix44f matrix;
	matrix.RotateZ(rot.z);
	matrix.RotateX(rot.x);
	matrix.RotateY(rot.y);
	unit->rightdir.x = matrix[ 0];
	unit->rightdir.y = matrix[ 1];
	unit->rightdir.z = matrix[ 2];
	unit->updir.x    = matrix[ 4];
	unit->updir.y    = matrix[ 5];
	unit->updir.z    = matrix[ 6];
	unit->frontdir.x = matrix[ 8];
	unit->frontdir.y = matrix[ 9];
	unit->frontdir.z = matrix[10];

	const shortint2 HandP = GetHAndPFromVector(unit->frontdir);
	unit->heading = HandP.x;

	unit->ForcedMove(pos);
	return 0;
}


int LuaSyncedCtrl::SetUnitPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to SetUnitPosition()");
	}

	if (lua_isnumber(L, 4)) {
		float3 pos((float)lua_tonumber(L, 2),
							 (float)lua_tonumber(L, 3),
							 (float)lua_tonumber(L, 4));
		unit->ForcedMove(pos);
		return 0;
	}

	float x, y, z;
	x = (float)lua_tonumber(L, 2);
	z = (float)lua_tonumber(L, 3);
	if (lua_isboolean(L, 4) && lua_toboolean(L, 4)) {
		y = ground->GetHeight(x, z);
	} else {
		y = ground->GetHeight2(x, z);
	}
	unit->ForcedMove(float3(x, y, z));

	return 0;
}


int LuaSyncedCtrl::SetUnitRotation(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetUnitRotation()");
	}
	const float3 rot((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	CMatrix44f matrix;
	matrix.RotateZ(rot.z);
	matrix.RotateX(rot.x);
	matrix.RotateY(rot.y);
	unit->rightdir.x = matrix[ 0];
	unit->rightdir.y = matrix[ 1];
	unit->rightdir.z = matrix[ 2];
	unit->updir.x    = matrix[ 4];
	unit->updir.y    = matrix[ 5];
	unit->updir.z    = matrix[ 6];
	unit->frontdir.x = matrix[ 8];
	unit->frontdir.y = matrix[ 9];
	unit->frontdir.z = matrix[10];

	const shortint2 HandP = GetHAndPFromVector(unit->frontdir);
	unit->heading = HandP.x;

	unit->ForcedMove(unit->pos);
	return 0;
}


int LuaSyncedCtrl::SetUnitVelocity(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetUnitVelocity()");
	}
	float3 dir((float)lua_tonumber(L, 2),
	           (float)lua_tonumber(L, 3),
	           (float)lua_tonumber(L, 4));
	unit->speed = dir;
	return 0;
}


int LuaSyncedCtrl::AddUnitDamage(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float damage   = (float)luaL_checknumber(L, 2);
	const int paralyze   = luaL_optint(L, 3, 0);
	const int attackerID = luaL_optint(L, 4, -1);
	const int weaponID   = luaL_optint(L, 5, -1);
	const float3 impulse = float3((float)luaL_optnumber(L, 6, 0.0f),
	                              (float)luaL_optnumber(L, 7, 0.0f),
	                              (float)luaL_optnumber(L, 8, 0.0f));

	CUnit* attacker = NULL;
	if (attackerID >= 0) {
		if (attackerID >= MAX_UNITS) {
			return 0;
		}
		attacker = uh->units[attackerID];
	}

	// numWeaponDefs has an extra slot
	if (weaponID > weaponDefHandler->numWeaponDefs) {
		return 0;
	}

	DamageArray damages;
	damages[unit->armorType] = damage;
	if (paralyze) {
		damages.paralyzeDamageTime = paralyze;
	}

	unit->DoDamage(damages, attacker, impulse, weaponID);

	return 0;
}


int LuaSyncedCtrl::AddUnitImpulse(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to AddUnitImpulse()");
	}
	float3 impulse((float)lua_tonumber(L, 2),
	               (float)lua_tonumber(L, 3),
	               (float)lua_tonumber(L, 4));
	unit->residualImpulse += impulse;
	unit->moveType->ImpulseAdded();
	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::AddUnitResource(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) || !lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to AddUnitResource()");
	}

	const string type = lua_tostring(L, 2);

	const float value = max(0.0f, float(lua_tonumber(L, 3)));

	if ((type == "m") || (type == "metal")) {
		unit->AddMetal(value);
	}
	else if ((type == "e") || (type == "energy")) {
		unit->AddEnergy(value);
	}
	return 0;
}


int LuaSyncedCtrl::UseUnitResource(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args < 2) {
		luaL_error(L, "Incorrect arguments to UseUnitResource()");
	}

	if (lua_isstring(L, 2)) {
		const string type = lua_tostring(L, 2);
		const float value = max(0.0f, float(lua_tonumber(L, 3)));

		if ((type == "m") || (type == "metal")) {
			lua_pushboolean(L, unit->UseMetal(value));
		}
		else if ((type == "e") || (type == "energy")) {
			lua_pushboolean(L, unit->UseEnergy(value));
		}
		return 1;
	}
	else if (lua_istable(L, 2)) {
		float metal  = 0.0f;
		float energy = 0.0f;
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				const string key = lua_tostring(L, -2);
				const float value = max(0.0f, float(lua_tonumber(L, -1)));
				if ((key == "m") || (key == "metal")) {
					metal = value;
				} else if ((key == "e") || (key == "energy")) {
					energy = value;
				}
			}
		}
		CTeam* team = gs->Team(unit->team);
		if ((team->metal >= metal) && (team->energy >= energy)) {
			unit->UseMetal(metal);
			unit->UseEnergy(energy);
			lua_pushboolean(L, true);
		} else {
			team->metalPull  += metal;
			team->energyPull += energy;
			lua_pushboolean(L, false);
		}
		return 1;
	}
	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::RemoveBuildingDecal(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	CBuilding* building = dynamic_cast<CBuilding*>(unit);
	if (building && building->buildingDecal) {
		std::vector<CGroundDecalHandler::BuildingDecalType*>& types =
			groundDecals->buildingDecalTypes;
		std::vector<CGroundDecalHandler::BuildingDecalType*>::iterator bdt;
		for (bdt = types.begin(); bdt != types.end(); ++bdt) {
			std::set<BuildingGroundDecal*>& decals = (*bdt)->buildingDecals;
			std::set<BuildingGroundDecal*>::iterator bgd;
			for (bgd = decals.begin(); bgd != decals.end(); ++bgd) {
				BuildingGroundDecal* decal = *bgd;
				if (decal == building->buildingDecal) {
					delete decal;
					decals.erase(bgd);
					building->buildingDecal = NULL;
					return 0;
				}
			}
		}
	}
	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::CreateFeature(lua_State* L)
{
	CheckAllowGameChanges(L);
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    // name, pos[, heading[, allyTeam]]
	    !lua_isstring(L, 1) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to CreateFeature()");
	}
	const string defName = lua_tostring(L, 1);
	const FeatureDef* featureDef = featureHandler->GetFeatureDef(defName);
	if (featureDef == NULL) {
		return 0; // do not error  (featureDefs are dynamic)
	}
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));

	short int heading = 0;
	if ((args >= 5) && lua_isnumber(L, 5)) {
		heading = (int)lua_tonumber(L, 5);
	}

	int team = CtrlTeam();
	if (team < 0) {
		team = -1; // default to global for AllAccessTeam
	}

	// FIXME -- separate teamcolor/allyteam arguments

	if ((args >= 6) && lua_isnumber(L, 6)) {
		team = (int)lua_tonumber(L, 6);
		if (team < -1) {
			team = -1;
		} else if (team >= MAX_TEAMS) {
			return 0;
		}
	}

	const int allyTeam = (team < 0) ? -1 : gs->AllyTeam(team);
	if (!CanControlFeatureAllyTeam(allyTeam)) {
		luaL_error(L, "CreateFeature() bad team permission %d", team);
	}

	if (inCreateFeature) {
		luaL_error(L, "CreateFeature() recursion is not permitted");
	}

	// use SetFeatureResurrect() to fill in the missing bits
	inCreateFeature = true;
	CFeature* feature = SAFE_NEW CFeature();
	feature->Initialize(pos, featureDef, heading, 0, team, "");
	inCreateFeature = false;

	lua_pushnumber(L, feature->id);

	return 1;
}


int LuaSyncedCtrl::DestroyFeature(lua_State* L)
{
	CheckAllowGameChanges(L);
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}

	if (inDestroyFeature) {
		luaL_error(L, "DestroyFeature() recursion is not permitted");
	}

	inDestroyFeature = true;
	featureHandler->DeleteFeature(feature);
	inDestroyFeature = false;

	return 0;
}


int LuaSyncedCtrl::TransferFeature(lua_State* L)
{
	CheckAllowGameChanges(L);
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to TransferFeature()");
	}
	const int team = (int)lua_tonumber(L, 2);
	if (team >= MAX_TEAMS) {
		return 0;
	}
	feature->ChangeTeam(team);
	return 0;
}

int LuaSyncedCtrl::SetFeatureHealth(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetFeatureHealth()");
	}
	feature->health = (float)lua_tonumber(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetFeatureReclaim(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetFeatureReclaim()");
	}
	feature->reclaimLeft = (float)lua_tonumber(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetFeaturePosition(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetFeaturePosition()");
	}
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	feature->ForcedMove(pos);
	return 0;
}


int LuaSyncedCtrl::SetFeatureDirection(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetFeatureDirection()");
	}
	float3 dir((float)lua_tonumber(L, 2),
	           (float)lua_tonumber(L, 3),
	           (float)lua_tonumber(L, 4));
	dir.Normalize();
	feature->ForcedSpin(dir);
	return 0;
}


int LuaSyncedCtrl::SetFeatureResurrect(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetFeatureResurrect()");
	}
	feature->createdFromUnit = lua_tostring(L, 2);
	if (args >= 3) {
		feature->buildFacing = ParseFacing(L, __FUNCTION__, 3);
	}
	return 0;
}


int LuaSyncedCtrl::SetFeatureNoSelect(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetFeatureNoSelect()");
	}
	feature->noSelect = !!lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetFeatureCollisionVolumeData(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	if (feature->collisionVolume == NULL) {
		return 0;
	}

	const int args = lua_gettop(L);
	if (args == 10) {
		const float xs = float(lua_tonumber(L, 2));
		const float ys = float(lua_tonumber(L, 3));
		const float zs = float(lua_tonumber(L, 4));
		const float xo = float(lua_tonumber(L, 5));
		const float yo = float(lua_tonumber(L, 6));
		const float zo = float(lua_tonumber(L, 7));
		const int vType = int(lua_tonumber(L,  8));
		const int tType = int(lua_tonumber(L,  9));
		const int pAxis = int(lua_tonumber(L, 10));

		const float3 scales(xs, ys, zs);
		const float3 offsets(xo, yo, zo);

		feature->collisionVolume->Init(scales, offsets, vType, tType, pAxis);
	} else {
		luaL_error(L, "Incorrect arguments to SetFeatureCollisionVolumeData()");
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/


static void ParseUnitMap(lua_State* L, const char* caller,
                         int table, vector<CUnit*>& unitIDs)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing unit map", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {
			CUnit* unit = ParseUnit(L, __FUNCTION__, -2);
			if (unit == NULL) {
				continue; // bad pointer
			}
			unitIDs.push_back(unit);
		}
	}
}


static void ParseUnitArray(lua_State* L, const char* caller,
                           int table, vector<CUnit*>& unitIDs)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing unit array", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2) && lua_isnumber(L, -1)) { // avoid 'n'
			CUnit* unit = ParseUnit(L, __FUNCTION__, -1);
			if (unit == NULL) {
				continue; // bad pointer
			}
			unitIDs.push_back(unit);
		}
	}
}


/******************************************************************************/

int LuaSyncedCtrl::GiveOrderToUnit(lua_State* L)
{
	CheckAllowGameChanges(L);

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		luaL_error(L, "Invalid unitID given to GiveOrderToUnit()");
	}

	Command cmd;
	LuaUtils::ParseCommand(L, __FUNCTION__, 2, cmd);

	if (!CanControlUnit(unit)) {
		lua_pushboolean(L, false);
		return 1;
	}

	if (inGiveOrder) {
		luaL_error(L, "GiveOrderToUnit() recursion is not permitted");
	}

	inGiveOrder = true;
	unit->commandAI->GiveCommand(cmd);
	inGiveOrder = false;

	lua_pushboolean(L, true);
	return 1;
}


int LuaSyncedCtrl::GiveOrderToUnitMap(lua_State* L)
{
	CheckAllowGameChanges(L);

	// units
	vector<CUnit*> units;
	ParseUnitMap(L, __FUNCTION__, 1, units);
	const int unitCount = (int)units.size();

	if (unitCount <= 0) {
		lua_pushnumber(L, 0);
		return 1;
	}

	Command cmd;
	LuaUtils::ParseCommand(L, __FUNCTION__, 2, cmd);

	if (inGiveOrder) {
		luaL_error(L, "GiveOrderToUnitMap() recursion is not permitted");
	}

	inGiveOrder = true;
	int count = 0;
	for (int i = 0; i < unitCount; i++) {
		CUnit* unit = units[i];
		if (CanControlUnit(unit)) {
			unit->commandAI->GiveCommand(cmd);
			count++;
		}
	}
	inGiveOrder = false;

	lua_pushnumber(L, count);
	return 1;
}


int LuaSyncedCtrl::GiveOrderToUnitArray(lua_State* L)
{
	CheckAllowGameChanges(L);

	// units
	vector<CUnit*> units;
	ParseUnitArray(L, __FUNCTION__, 1, units);
	const int unitCount = (int)units.size();

	if (unitCount <= 0) {
		lua_pushnumber(L, 0);
		return 1;
	}

	Command cmd;
	LuaUtils::ParseCommand(L, __FUNCTION__, 2, cmd);

	if (inGiveOrder) {
		luaL_error(L, "GiveOrderToUnitArray() recursion is not permitted");
	}
	inGiveOrder = true;

	int count = 0;
	for (int i = 0; i < unitCount; i++) {
		CUnit* unit = units[i];
		if (CanControlUnit(unit)) {
			unit->commandAI->GiveCommand(cmd);
			count++;
		}
	}

	inGiveOrder = false;

	lua_pushnumber(L, count);
	return 1;
}


int LuaSyncedCtrl::GiveOrderArrayToUnitMap(lua_State* L)
{
	CheckAllowGameChanges(L);

	// units
	vector<CUnit*> units;
	ParseUnitMap(L, __FUNCTION__, 1, units);
	const int unitCount = (int)units.size();

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __FUNCTION__, 2, commands);
	const int commandCount = (int)commands.size();

	if ((unitCount <= 0) || (commandCount <= 0)) {
		lua_pushnumber(L, 0);
		return 1;
	}

	if (inGiveOrder) {
		luaL_error(L, "GiveOrderArrayToUnitMap() recursion is not permitted");
	}
	inGiveOrder = true;

	int count = 0;
	for (int u = 0; u < unitCount; u++) {
		CUnit* unit = units[u];
		if (CanControlUnit(unit)) {
			for (int c = 0; c < commandCount; c++) {
				unit->commandAI->GiveCommand(commands[c]);
			}
			count++;
		}
	}
	inGiveOrder = false;

	lua_pushnumber(L, count);
	return 1;
}


int LuaSyncedCtrl::GiveOrderArrayToUnitArray(lua_State* L)
{
	CheckAllowGameChanges(L);

	// units
	vector<CUnit*> units;
	ParseUnitArray(L, __FUNCTION__, 1, units);
	const int unitCount = (int)units.size();

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __FUNCTION__, 2, commands);
	const int commandCount = (int)commands.size();

	if ((unitCount <= 0) || (commandCount <= 0)) {
		lua_pushnumber(L, 0);
		return 1;
	}

	if (inGiveOrder) {
		luaL_error(L, "GiveOrderArrayToUnitArray() recursion is not permitted");
	}
	inGiveOrder = true;

	int count = 0;
	for (int u = 0; u < unitCount; u++) {
		CUnit* unit = units[u];
		if (CanControlUnit(unit)) {
			for (int c = 0; c < commandCount; c++) {
				unit->commandAI->GiveCommand(commands[c]);
			}
			count++;
		}
	}
	inGiveOrder = false;

	lua_pushnumber(L, count);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static void ParseMapParams(lua_State* L, const char* caller, float& factor,
                           int& x1, int& z1, int& x2, int& z2)
{
	float fx1 = 0.0f;
	float fz1 = 0.0f;
	float fx2 = 0.0f;
	float fz2 = 0.0f;

	const int args = lua_gettop(L); // number of arguments
	if (args == 3) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)|| !lua_isnumber(L, 3)) {
			luaL_error(L, "Incorrect arguments to %s()", caller);
		}
		fx1 = fx2 = (float)lua_tonumber(L, 1);
		fz1 = fz2 = (float)lua_tonumber(L, 2);
		factor    = (float)lua_tonumber(L, 3);
	}
	else if (args == 5) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
		    !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
			luaL_error(L, "Incorrect arguments to %s()", caller);
		}
		fx1    = (float)lua_tonumber(L, 1);
		fz1    = (float)lua_tonumber(L, 2);
		fx2    = (float)lua_tonumber(L, 3);
		fz2    = (float)lua_tonumber(L, 4);
		factor = (float)lua_tonumber(L, 5);
	}
	else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	// quantize and clamp
	x1 = (int)max(0 , min(gs->mapx, (int)(fx1 / SQUARE_SIZE)));
	z1 = (int)max(0 , min(gs->mapy, (int)(fz1 / SQUARE_SIZE)));
	x2 = (int)max(0 , min(gs->mapx, (int)(fx2 / SQUARE_SIZE)));
	z2 = (int)max(0 , min(gs->mapy, (int)(fz2 / SQUARE_SIZE)));

	return;
}


int LuaSyncedCtrl::LevelHeightMap(lua_State* L)
{
	if (mapDamage->disabled) {
		return 0;
	}
	float height;
	int x1, x2, z1, z2;
	ParseMapParams(L, __FUNCTION__, height, x1, z1, x2, z2);

	float* heightMap = readmap->GetHeightmap();
	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			const int index = (z * (gs->mapx + 1)) + x;
			heightMap[index] = height;
		}
	}
	mapDamage->RecalcArea(x1, x2, z1, z2);
	return 0;
}


int LuaSyncedCtrl::AdjustHeightMap(lua_State* L)
{
	if (mapDamage->disabled) {
		return 0;
	}
	float height;
	int x1, x2, z1, z2;
	ParseMapParams(L, __FUNCTION__, height, x1, z1, x2, z2);

	float* heightMap = readmap->GetHeightmap();
	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			const int index = (z * (gs->mapx + 1)) + x;
			heightMap[index] += height;
		}
	}
	mapDamage->RecalcArea(x1, x2, z1, z2);
	return 0;
}


int LuaSyncedCtrl::RevertHeightMap(lua_State* L)
{
	if (mapDamage->disabled) {
		return 0;
	}
	float origFactor;
	int x1, x2, z1, z2;
	ParseMapParams(L, __FUNCTION__, origFactor, x1, z1, x2, z2);

	float* origMap = readmap->orgheightmap;
	float* currMap = readmap->GetHeightmap();
	if (origFactor == 1.0f) {
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int index = (z * (gs->mapx + 1)) + x;
				currMap[index] = origMap[index];
			}
		}
	}
	else {
		const float currFactor = (1.0f - origFactor);
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int index = (z * (gs->mapx + 1)) + x;
				currMap[index] = (origFactor * origMap[index]) +
				                 (currFactor * currMap[index]);
			}
		}
	}
	mapDamage->RecalcArea(x1, x2, z1, z2);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetNoPause(lua_State* L)
{
	if (!FullCtrl()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetNoPause()");
	}
	// Important: only works in server mode, has no effect in client mode
	if (gameServer)
		gameServer->SetGamePausable(!lua_toboolean(L, 1));

	return 0;
}


int LuaSyncedCtrl::SetUnitToFeature(lua_State* L)
{
	if (!FullCtrl()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetUnitToFeature()");
	}
	uh->morphUnitToFeature = lua_toboolean(L, 1);
	return 0;
}


int LuaSyncedCtrl::SetExperienceGrade(lua_State* L)
{
	if (!FullCtrl()) {
		return 0;
	}
	const float expGrade = (float)luaL_checknumber(L, 1);
	CUnit::SetExpGrade(expGrade);

	// NOTE: for testing, should be using modrules.tdf
	if (gs->cheatEnabled) {
		if (lua_isnumber(L, 2)) {
			CUnit::SetExpPowerScale((float)lua_tonumber(L, 2));
		}
		if (lua_isnumber(L, 3)) {
			CUnit::SetExpHealthScale((float)lua_tonumber(L, 3));
		}
		if (lua_isnumber(L, 4)) {
			CUnit::SetExpReloadScale((float)lua_tonumber(L, 4));
		}
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

static bool ParseNamedInt(lua_State* L, const string& key,
                          const string& name, int& value)
{
	if (key != name) {
		return false;
	}
	if (lua_isnumber(L, -1)) {
		value = (int)lua_tonumber(L, -1);
	} else {
		luaL_error(L, "bad %s argument", name.c_str());
	}
	return true;
}


static bool ParseNamedBool(lua_State* L, const string& key,
                           const string& name, bool& value)
{
	if (key != name) {
		return false;
	}
	if (lua_isboolean(L, -1)) {
		value = (int)lua_toboolean(L, -1);
	} else {
		luaL_error(L, "bad %s argument", name.c_str());
	}
	return true;
}


static bool ParseNamedString(lua_State* L, const string& key,
                             const string& name, string& value)
{
	if (key != name) {
		return false;
	}
	if (lua_isstring(L, -1)) {
		value = lua_tostring(L, -1);
	} else {
		luaL_error(L, "bad %s argument", name.c_str());
	}
	return true;
}


static int ParseStringVector(lua_State* L, int index, vector<string>& strvec)
{
	strvec.clear();
	int i = 1;
	while (true) {
		lua_rawgeti(L, index, i);
		if (lua_isstring(L, -1)) {
			strvec.push_back(lua_tostring(L, -1));
			lua_pop(L, 1);
			i++;
		} else {
			lua_pop(L, 1);
			return (i - 1);
		}
	}
}


static int ParseFloatVector(lua_State* L, int index, vector<float>& floatvec)
{
	floatvec.clear();
	int i = 1;
	while (true) {
		lua_rawgeti(L, index, i);
		if (lua_isnumber(L, -1)) {
			floatvec.push_back((float)lua_tonumber(L, -1));
			lua_pop(L, 1);
			i++;
		} else {
			lua_pop(L, 1);
			return (i - 1);
		}
	}
}


static int ParseIntVector(lua_State* L, int index, vector<float>& intvec)
{
	intvec.clear();
	int i = 1;
	while (true) {
		lua_rawgeti(L, index, i);
		if (lua_isnumber(L, -1)) {
			intvec.push_back((int)lua_tonumber(L, -1));
			lua_pop(L, 1);
			i++;
		} else {
			lua_pop(L, 1);
			return (i - 1);
		}
	}
}


/******************************************************************************/

static bool ParseCommandDescription(lua_State* L, int table,
                                    CommandDescription& cd)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "Can not parse CommandDescription");
		return false;
	}

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2)) {
			continue;
		}
		const string key = lua_tostring(L, -2);
		if (ParseNamedInt(L,    key, "id",          cd.id)         ||
		    ParseNamedInt(L,    key, "type",        cd.type)       ||
		    ParseNamedString(L, key, "name",        cd.name)       ||
		    ParseNamedString(L, key, "action",      cd.action)     ||
		    ParseNamedString(L, key, "tooltip",     cd.tooltip)    ||
		    ParseNamedString(L, key, "texture",     cd.iconname)   ||
		    ParseNamedString(L, key, "cursor",      cd.mouseicon)  ||
		    ParseNamedBool(L,   key, "hidden",      cd.onlyKey)    ||
		    ParseNamedBool(L,   key, "disabled",    cd.disabled)   ||
		    ParseNamedBool(L,   key, "showUnique",  cd.showUnique) ||
		    ParseNamedBool(L,   key, "onlyTexture", cd.onlyTexture)) {
			continue; // successfully parsed a parameter
		}
		if ((key != "params") || !lua_istable(L, -1)) {
			luaL_error(L, "Unknown cmdDesc parameter %s", key.c_str());
		}
		// collect the parameters
		const int paramTable = lua_gettop(L);
		ParseStringVector(L, paramTable, cd.params);
	}

	return true;
}


int LuaSyncedCtrl::EditUnitCmdDesc(lua_State* L)
{
	if (!FullCtrl()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_istable(L, 3)) {
		luaL_error(L, "Incorrect arguments to EditUnitCmdDesc");
	}

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	vector<CommandDescription>& cmdDescs = unit->commandAI->possibleCommands;

	const int cmdDescID = (int)lua_tonumber(L, 2) - 1;
	if ((cmdDescID < 0) || (cmdDescID >= (int)cmdDescs.size())) {
		return 0;
	}

	ParseCommandDescription(L, 3, cmdDescs[cmdDescID]);

	selectedUnits.PossibleCommandChange(unit);

	return 0;
}


int LuaSyncedCtrl::InsertUnitCmdDesc(lua_State* L)
{
	if (!FullCtrl()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (((args == 2) && !lua_istable(L, 2)) ||
	    ((args >= 3) && (!lua_isnumber(L, 2) || !lua_istable(L, 3)))) {
		luaL_error(L, "Incorrect arguments to InsertUnitCmdDesc");
	}

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	vector<CommandDescription>& cmdDescs = unit->commandAI->possibleCommands;

	int table = 2;
	if (args >= 3) {
		table = 3;
	}

	int cmdDescID = (int)cmdDescs.size();
	if (args >= 3) {
		cmdDescID = (int)lua_tonumber(L, 2) - 1;
		if (cmdDescID < 0) {
			cmdDescID = 0;
		}
	}

	CommandDescription cd;
	ParseCommandDescription(L, table, cd);

	if (cmdDescID >= (int)cmdDescs.size()) {
		cmdDescs.push_back(cd);
	} else {
		vector<CommandDescription>::iterator it = cmdDescs.begin();
		advance(it, cmdDescID);
		cmdDescs.insert(it, cd);
	}

	selectedUnits.PossibleCommandChange(unit);

	return 0;
}


int LuaSyncedCtrl::RemoveUnitCmdDesc(lua_State* L)
{
	if (!FullCtrl()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to RemoveUnitCmdDesc");
	}

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	vector<CommandDescription>& cmdDescs = unit->commandAI->possibleCommands;

	int cmdDescID = (int)cmdDescs.size() - 1;
	if ((args >= 2) && lua_isnumber(L, 2)) {
		cmdDescID = (int)lua_tonumber(L, 2) - 1;
	}

	if ((cmdDescID < 0) || (cmdDescID >= (int)cmdDescs.size())) {
		return 0;
	}

	vector<CommandDescription>::iterator it = cmdDescs.begin();
	advance(it, cmdDescID);
	cmdDescs.erase(it);

	selectedUnits.PossibleCommandChange(unit);

	return 0;
}


/******************************************************************************/
/******************************************************************************/
