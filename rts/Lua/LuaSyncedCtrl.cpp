#include "StdAfx.h"
// LuaSyncedCtrl.cpp: implementation of the CLuaSyncedCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include <set>
#include <list>
#include <cctype>

#include "mmgr.h"

#include "LuaSyncedCtrl.h"

#include "LuaInclude.h"

#include "LuaRules.h" // for MAX_LUA_COB_ARGS
#include "LuaHandleSynced.h"
#include "LuaHashString.h"
#include "LuaSyncedMoveCtrl.h"
#include "LuaUtils.h"
#include "Game/Game.h"
#include "Game/GameServer.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/PlayerHandler.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/Team.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/ReadMap.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/UnitModels/IModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/LuaUnitScript.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/UnitTypes/ExtractorBuilding.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "myMath.h"
#include "LogOutput.h"

using namespace std;


/******************************************************************************/
/******************************************************************************/

bool LuaSyncedCtrl::inCreateUnit = false;
bool LuaSyncedCtrl::inDestroyUnit = false;
bool LuaSyncedCtrl::inTransferUnit = false;
bool LuaSyncedCtrl::inCreateFeature = false;
bool LuaSyncedCtrl::inDestroyFeature = false;
bool LuaSyncedCtrl::inGiveOrder = false;
bool LuaSyncedCtrl::inHeightMap = false;

static int heightMapx1;
static int heightMapx2;
static int heightMapz1;
static int heightMapz2;
static float heightMapAmountChanged;


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
	REGISTER_LUA_CFUNC(SetUnitArmored);
	REGISTER_LUA_CFUNC(SetUnitLosMask);
	REGISTER_LUA_CFUNC(SetUnitLosState);
	REGISTER_LUA_CFUNC(SetUnitCloak);
	REGISTER_LUA_CFUNC(SetUnitStealth);
	REGISTER_LUA_CFUNC(SetUnitSonarStealth);
	REGISTER_LUA_CFUNC(SetUnitAlwaysVisible);
	REGISTER_LUA_CFUNC(SetUnitMetalExtraction);
	REGISTER_LUA_CFUNC(SetUnitBuildSpeed);
	REGISTER_LUA_CFUNC(SetUnitBlocking);
	REGISTER_LUA_CFUNC(SetUnitShieldState);
	REGISTER_LUA_CFUNC(SetUnitFlanking);
	REGISTER_LUA_CFUNC(SetUnitTravel);
	REGISTER_LUA_CFUNC(SetUnitFuel);
	REGISTER_LUA_CFUNC(SetUnitMoveGoal);
	REGISTER_LUA_CFUNC(SetUnitLineage);
	REGISTER_LUA_CFUNC(SetUnitNeutral);
	REGISTER_LUA_CFUNC(SetUnitTarget);
	REGISTER_LUA_CFUNC(SetUnitCollisionVolumeData);
	REGISTER_LUA_CFUNC(SetUnitPieceCollisionVolumeData);
	REGISTER_LUA_CFUNC(SetUnitSensorRadius);

	REGISTER_LUA_CFUNC(SetUnitPhysics);
	REGISTER_LUA_CFUNC(SetUnitPosition);
	REGISTER_LUA_CFUNC(SetUnitVelocity);
	REGISTER_LUA_CFUNC(SetUnitRotation);

	REGISTER_LUA_CFUNC(AddUnitDamage);
	REGISTER_LUA_CFUNC(AddUnitImpulse);
	REGISTER_LUA_CFUNC(AddUnitSeismicPing);

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

	REGISTER_LUA_CFUNC(SetProjectilePosition);
	REGISTER_LUA_CFUNC(SetProjectileVelocity);
	REGISTER_LUA_CFUNC(SetProjectileCollision);

	REGISTER_LUA_CFUNC(CallCOBScript);
	REGISTER_LUA_CFUNC(GetCOBScriptID);
	//FIXME: REGISTER_LUA_CFUNC(GetUnitCOBValue);
	//FIXME: REGISTER_LUA_CFUNC(SetUnitCOBValue);

	REGISTER_LUA_CFUNC(GiveOrderToUnit);
	REGISTER_LUA_CFUNC(GiveOrderToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderToUnitArray);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitArray);

	REGISTER_LUA_CFUNC(LevelHeightMap);
	REGISTER_LUA_CFUNC(AdjustHeightMap);
	REGISTER_LUA_CFUNC(RevertHeightMap);

	REGISTER_LUA_CFUNC(AddHeightMap);
	REGISTER_LUA_CFUNC(SetHeightMap);
	REGISTER_LUA_CFUNC(SetHeightMapFunc);

	REGISTER_LUA_CFUNC(SpawnCEG);

	REGISTER_LUA_CFUNC(EditUnitCmdDesc);
	REGISTER_LUA_CFUNC(InsertUnitCmdDesc);
	REGISTER_LUA_CFUNC(RemoveUnitCmdDesc);

	REGISTER_LUA_CFUNC(SetNoPause);
	REGISTER_LUA_CFUNC(SetUnitToFeature);
	REGISTER_LUA_CFUNC(SetExperienceGrade);

	if (!LuaSyncedMoveCtrl::PushMoveCtrl(L)) {
		return false;
	}

	if (!CLuaUnitScript::PushEntries(L)) {
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
	return teamHandler->AllyTeam(ctrlTeam);
}


static inline bool CanControlTeam(int teamID)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == teamID);
}


static inline bool CanControlAllyTeam(int allyTeamID)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (teamHandler->AllyTeam(ctrlTeam) == allyTeamID);
}


static inline bool CanControlUnit(const CUnit* unit)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == unit->team);
}


static inline bool CanControlFeatureAllyTeam(int allyTeamID)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	if (allyTeamID < 0) {
		return (ctrlTeam == teamHandler->GaiaTeamID());
	}
	return (teamHandler->AllyTeam(ctrlTeam) == allyTeamID);
}


static inline bool CanControlFeature(const CFeature* feature)
{
	return CanControlFeatureAllyTeam(feature->allyteam);
}


static inline bool CanControlProjectileAllyTeam(int allyTeamID)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	if (allyTeamID < 0) {
		return false;
	}
	return (teamHandler->AllyTeam(ctrlTeam) == allyTeamID);
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
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
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
	const int featureID = lua_toint(L, index);
	const CFeatureSet& fset = featureHandler->GetActiveFeatures();
	CFeatureSet::const_iterator it = fset.find(featureID);

	if (it == fset.end())
		return NULL;

	if (!CanControlFeature(*it)) {
		return NULL;
	}
	return *it;
}


static inline CProjectile* ParseProjectile(lua_State* L,
                                           const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad projectile ID", caller);
	}
	const ProjectileMapPair* pmp = ph->GetMapPairByID(lua_toint(L, index));
	if (pmp == NULL) {
		return NULL;
	}
	if (!CanControlProjectileAllyTeam(pmp->second)) {
		return NULL;
	}
	return pmp->first;
}


static CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad teamID", caller);
		return NULL;
	}
	const int teamID = lua_toint(L, index);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		luaL_error(L, "%s(): Bad teamID: %d", caller, teamID);
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return NULL;
	}
	return team;
}


static CUnit* CheckUnitID(lua_State* L, int index)
{
	luaL_checknumber(L, index);
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
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
	const int playerID = lua_toint(L, index);
	if ((playerID < 0) || (playerID >= playerHandler->ActivePlayers())) {
		luaL_error(L, "Bad playerID: %d\n", playerID);
	}
	CPlayer* player = playerHandler->Player(playerID);
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
			array[index] = lua_tofloat(L, -1);
			index++;
		}
	}
	return index;
}


static int ParseFacing(lua_State* L, const char* caller, int index)
{
	if (lua_israwnumber(L, index)) {
		return lua_toint(L, index);
	}
	else if (lua_israwstring(L, index)) {
		const string dir = StringToLower(lua_tostring(L, index));
		if (dir == "s") { return 0; }
		if (dir == "e") { return 1; }
		if (dir == "n") { return 2; }
		if (dir == "w") { return 3; }
		if (dir == "south") { return 0; }
		if (dir == "east")  { return 1; }
		if (dir == "north") { return 2; }
		if (dir == "west")  { return 3; }
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
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	if (!CanControlTeam(teamID)) {
		return 0;
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	const string type = luaL_checkstring(L, 2);

	const float value = max(0.0f, luaL_checkfloat(L, 3));

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
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	if (!CanControlTeam(teamID)) {
		return 0;
	}
	CTeam* team = teamHandler->Team(teamID);
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
	else {
		luaL_error(L, "bad arguments");
	}
	return 0;
}


int LuaSyncedCtrl::SetTeamResource(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	if (!CanControlTeam(teamID)) {
		return 0;
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	const string type = luaL_checkstring(L, 2);

	const float value = max(0.0f, luaL_checkfloat(L, 3));

	if ((type == "m") || (type == "metal")) {
		team->metal = min(team->metalStorage, value);
	}
	else if ((type == "e") || (type == "energy")) {
		team->energy = min(team->energyStorage, value);
	}
	else if ((type == "ms") || (type == "metalStorage")) {
		team->metalStorage = value;
		team->metal = min((float)team->metal, team->metalStorage);
	}
	else if ((type == "es") || (type == "energyStorage")) {
		team->energyStorage = value;
		team->energy = min((float)team->energy, team->energyStorage);
	}
	return 0;
}


int LuaSyncedCtrl::SetTeamShareLevel(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	if (!CanControlTeam(teamID)) {
		return 0;
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	const string type = luaL_checkstring(L, 2);

	const float value = luaL_checkfloat(L, 3);

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
			args.push_back(lua_toint(L, a));
		}
		else if (lua_istable(L, a)) {
			lua_rawgeti(L, a, 1);
			lua_rawgeti(L, a, 2);
			if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
				const int x = lua_toint(L, -2);
				const int z = lua_toint(L, -1);
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
	CCobInstance* cob = dynamic_cast<CCobInstance*>(unit->script);
	if (cob == NULL) {
		luaL_error(L, "CallCOBScript(): unit is not running a COB script");
	}

	// collect the arguments
	vector<int> cobArgs;
	ParseCobArgs(L, 4, args, cobArgs);
	const int retParams = min(lua_toint(L, 3),
	                          min(MAX_LUA_COB_ARGS, (int)cobArgs.size()));

	int retval;
	if (lua_israwnumber(L, 2)) {
		const int funcId = lua_toint(L, 2);
		retval = cob->RawCall(funcId, cobArgs);
	}
	else if (lua_israwstring(L, 2)) {
		const string funcName = lua_tostring(L, 2);
		retval = cob->Call(funcName, cobArgs);
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
	CCobInstance* cob = dynamic_cast<CCobInstance*>(unit->script);
	if (cob == NULL) {
		luaL_error(L, "GetCOBScriptID(): unit is not running a COB script");
	}

	const string funcName = lua_tostring(L, 2);

	const int funcID = cob->GetFunctionId(funcName);
	if (funcID >= 0) {
		lua_pushnumber(L, funcID);
		return 1;
	}

	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::CreateUnit(lua_State* L)
{
	CheckAllowGameChanges(L);

	const UnitDef* unitDef = NULL;
	if (lua_israwstring(L, 1)) {
		const string defName = lua_tostring(L, 1);
		unitDef = unitDefHandler->GetUnitByName(defName);
		if (unitDef == NULL) {
			luaL_error(L, "CreateUnit(): bad unitDef name: %s", defName.c_str());
			return 0;
		}
	} else if (lua_israwnumber(L, 1)) {
		const int defID = lua_toint(L, 1);
		unitDef = unitDefHandler->GetUnitByID(defID);
		if (unitDef == NULL) {
			luaL_error(L, "CreateUnit(): bad unitDef ID: %i", defID);
			return 0;
		}
	} else {
		luaL_error(L, "Incorrect arguments to CreateUnit()");
	}

	float3 pos(luaL_checkfloat(L, 2),
	           luaL_checkfloat(L, 3),
	           luaL_checkfloat(L, 4));

	//clamps the pos in the map boundings
	pos.CheckInBounds(); //TODO: fix unit init code to work offmap

	const int facing = ParseFacing(L, __FUNCTION__, 5);

	int teamID = CtrlTeam();
	if (lua_israwnumber(L, 6)) {
		teamID = lua_toint(L, 6);
	}
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		luaL_error(L, "CreateUnit(): bad team number: %d", teamID);
		return 0;
	}

	if (teamHandler->AllyTeam(teamID) >= teamHandler->ActiveAllyTeams()) {
		// FIXME: there's a segv in CLosHandler::LosAddAir,
		//        this is a dirty hack to avoid it
		luaL_error(L, "CreateUnit(): inactive team: %d", teamID);
		return 0;
	}


	if (!FullCtrl() && (CtrlTeam() != teamID)) {
		luaL_error(L, "CreateUnit(): bad team %d", teamID);
		return 0;
	}

	if (!uh->CanBuildUnit(unitDef, teamID)) {
		return 0; // unit limit reached
	}

	// FIXME -- allow specifying the 'build' and 'builder' parameters?

	if (inCreateUnit) {
		luaL_error(L, "CreateUnit(): recursion is not permitted");
		return 0;
	}
	inCreateUnit = true;
	ASSERT_SYNCED_FLOAT3(pos);
	ASSERT_SYNCED_PRIMITIVE((int)facing);
	CUnit* unit = unitLoader.LoadUnit(unitDef, pos, teamID,
	                                  false, facing, NULL);
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
	ASSERT_SYNCED_PRIMITIVE(unit->id);
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

	const int newTeam = luaL_checkint(L, 2);
	if ((newTeam < 0) || (newTeam >= teamHandler->ActiveTeams())) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(newTeam);
	if (team == NULL) {
		return 0;
	}

	bool given = true;
	if (FullCtrl() && lua_isboolean(L, 3)) {
		given = lua_toboolean(L, 3);
	}

	if (inTransferUnit) {
		luaL_error(L, "TransferUnit() recursion is not permitted");
	}
	inTransferUnit = true;
	ASSERT_SYNCED_PRIMITIVE(unit->id);
	ASSERT_SYNCED_PRIMITIVE((int)newTeam);
	ASSERT_SYNCED_PRIMITIVE(given);
	unit->ChangeTeam(newTeam, given ? CUnit::ChangeGiven
	                                : CUnit::ChangeCaptured);
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
	if (!lua_istable(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitCosts");
	}
	const int table = 2;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1)) {
			continue;
		}
		const string key = lua_tostring(L, -2);
		const float value = lua_tonumber(L, -1);
		ASSERT_SYNCED_PRIMITIVE((float)value);

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

	if (lua_israwstring(L, 2)) {
		const string key = luaL_checkstring(L, 2);
		const float value = luaL_checkfloat(L, 3);
		SetUnitResourceParam(unit, key, value);
	}
	else if (lua_istable(L, 2)) {
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1)) {
				continue;
			}
			const string key = lua_tostring(L, -2);
			const float value = lua_tonumber(L, -1);
			ASSERT_SYNCED_PRIMITIVE((float)value);

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
	unit->tooltip = luaL_checkstring(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitHealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	if (lua_isnumber(L, 2)) {
		float health = lua_tofloat(L, 2);
		health = min(unit->maxHealth, health);
		unit->health = health;
	}
	else if (lua_istable(L, 2)) {
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				const string key = lua_tostring(L, -2);
				const float value = lua_tofloat(L, -1);
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
	unit->maxHealth = luaL_checkfloat(L, 2);
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
		const float percent = max(0.0f, min(1.0f, lua_tofloat(L, 3)));
		unit->stockpileWeapon->buildPercent = percent;
	}

	return 0;
}


static int SetSingleUnitWeaponState(lua_State* L, CWeapon* weapon, int index)
{
	const string key = lua_tostring(L, index);
	const float value = lua_tofloat(L, index + 1);
	// FIXME: KDR -- missing checks and updates?
	if (key == "reloadState") {
		weapon->reloadStatus = (int)value;
	}
	else if (key == "reloadTime") {
		weapon->reloadTime = (int)(value * GAME_SPEED);
	}
	else if (key == "accuracy") {
		weapon->accuracy = value;
	}
	else if (key == "sprayAngle") {
		weapon->sprayAngle = value;
	}
	else if (key == "range") {
		weapon->range = value;
	}
	else if (key == "projectileSpeed") {
		weapon->projectileSpeed = value;
	}
	else if (key == "burst") {
		weapon->salvoSize = (int)value;
	}
	else if (key == "burstRate") {
		weapon->salvoDelay = (int)(value * GAME_SPEED);
	}
	else if (key == "projectiles") {
		weapon->projectilesPerShot = (int)value;
	}
	else if (key == "aimReady") {
		// HACK, this should be set to result of lua_toboolean
		weapon->angleGood = (value != 0.0f);
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitWeaponState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const int weaponNum = luaL_checkint(L,2);
	if ((weaponNum < 0) || ((size_t)weaponNum >= unit->weapons.size())) {
		return 0;
	}
	CWeapon* weapon = unit->weapons[weaponNum];

	if (lua_istable(L,3)) {
		const int table = 3;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				SetSingleUnitWeaponState(L, weapon, -2);
			}
		}
	}
	else if (lua_israwstring(L, 3) && lua_isnumber(L, 4)) {
		SetSingleUnitWeaponState(L, weapon, 3);
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitExperience(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float experience = max(0.0f, luaL_checkfloat(L, 2));
	unit->AddExperience(experience - unit->experience);
	return 0;
}


int LuaSyncedCtrl::SetUnitArmored(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (lua_isboolean(L, 2)) {
		unit->armoredState = lua_toboolean(L, 2);
	}
	return 0;
}


static unsigned char ParseLosBits(lua_State* L, int index, unsigned char bits)
{
	if (lua_isnumber(L, index)) {
		return (unsigned char)lua_tonumber(L, index);
	}
	else if (lua_istable(L, index)) {
		for (lua_pushnil(L); lua_next(L, index) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2)) { luaL_error(L, "bad key type");   }
			if (!lua_isboolean(L, -1))   { luaL_error(L, "bad value type"); }
			const string key = lua_tostring(L, -2);
			const bool set = lua_toboolean(L, -1);
			if (key == "los") {
				if (set) { bits |=  LOS_INLOS; }
				else     { bits &= ~LOS_INLOS; }
			}
			else if (key == "radar") {
				if (set) { bits |=  LOS_INRADAR; }
				else     { bits &= ~LOS_INRADAR; }
			}
			else if (key == "prevLos") {
				if (set) { bits |=  LOS_PREVLOS; }
				else     { bits &= ~LOS_PREVLOS; }
			}
			else if (key == "contRadar") {
				if (set) { bits |=  LOS_CONTRADAR; }
				else     { bits &= ~LOS_CONTRADAR; }
			}
		}
		return bits;
	}
 	luaL_error(L, "ERROR: expected number or table");
	return 0;
}


int LuaSyncedCtrl::SetUnitLosMask(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int allyTeam = luaL_checkint(L, 2);
	if ((allyTeam < 0) || (allyTeam >= teamHandler->ActiveTeams())) {
		luaL_error(L, "bad allyTeam");
	}
	const unsigned short losStatus = unit->losStatus[allyTeam];
	const unsigned char  oldMask = losStatus >> 8;
	const unsigned char  newMask = ParseLosBits(L, 3, oldMask);
	const unsigned short state = (newMask << 8) | (losStatus & 0x00FF);

	unit->losStatus[allyTeam] = state;
	unit->SetLosStatus(allyTeam, unit->CalcLosStatus(allyTeam));

	return 0;
}


int LuaSyncedCtrl::SetUnitLosState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int allyTeam = luaL_checkint(L, 2);
	if ((allyTeam < 0) || (allyTeam >= teamHandler->ActiveTeams())) {
		luaL_error(L, "bad allyTeam");
	}
	const unsigned short losStatus = unit->losStatus[allyTeam];
	const unsigned char  oldState = losStatus & 0xFF;
	const unsigned char  newState = ParseLosBits(L, 3, oldState);
	const unsigned short state = (losStatus & 0xFF00) | newState;

	unit->SetLosStatus(allyTeam, state);

	return 0;
}


int LuaSyncedCtrl::SetUnitCloak(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	if (lua_isboolean(L, 2)) {
		unit->scriptCloak = lua_toboolean(L, 2) ? 1 : 0;
	} else if (lua_isnumber(L, 2)) {
		unit->scriptCloak = lua_toint(L, 2);
	} else if (!lua_isnil(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitCloak()");
	}

	if (lua_israwnumber(L, 3)) {
		unit->decloakDistance = lua_tofloat(L, 3);
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
	if (!lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitStealth()");
	}
	unit->stealth = lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitSonarStealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitSonarStealth()");
	}
	unit->sonarStealth = lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitAlwaysVisible(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 2)) {
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
	const float depth = luaL_checkfloat(L, 2);
	const float range = luaL_optfloat(L, 3, mex->GetExtractionRange());
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

	const float buildScale = (1.0f / 32.0f);
	const float buildSpeed = buildScale * max(0.0f, luaL_checkfloat(L, 2));

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
	if (lua_isnumber(L, 3)) {
		builder->repairSpeed    = buildScale * max(0.0f, lua_tofloat(L, 3));
	}
	if (lua_isnumber(L, 4)) {
		builder->reclaimSpeed   = buildScale * max(0.0f, lua_tofloat(L, 4));
	}
	if (lua_isnumber(L, 5)) {
		builder->resurrectSpeed = buildScale * max(0.0f, lua_tofloat(L, 5));
	}
	if (lua_isnumber(L, 6)) {
		builder->captureSpeed   = buildScale * max(0.0f, lua_tofloat(L, 6));
	}
	if (lua_isnumber(L, 7)) {
		builder->terraformSpeed = buildScale * max(0.0f, lua_tofloat(L, 7));
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitBlocking(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitBlocking()");
	}

	if (lua_toboolean(L, 2)) {
		unit->Block();
	} else {
		unit->UnBlock();
	}

	if (lua_isboolean(L, 3)) {
		// change the collidable state
		unit->blocking = lua_toboolean(L, 3);

		// run this again so that we are removed from
		// the blocking map if unit->blocking was set
		// to false but arg. #2 was true (no point in
		// being registered on the map then)
		unit->Block();
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitShieldState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	int args = lua_gettop(L);
	int arg = 2;

	CPlasmaRepulser* shield = (CPlasmaRepulser*) unit->shieldWeapon;
	if (lua_isnumber(L, 2) && args > 2) {
		arg++;
		const int idx = luaL_optint(L, 2, -1);
		if (idx >= 0 && idx < unit->weapons.size())
			shield = dynamic_cast<CPlasmaRepulser*>(unit->weapons[idx]);
	}

	if (shield == NULL) {
		return 0;
	}

	if (lua_isboolean(L, arg)) { shield->isEnabled = lua_toboolean(L, arg);	arg++; }
	if (lua_isnumber(L, arg)) { shield->curPower = lua_tofloat(L, arg); }
	return 0;
}


int LuaSyncedCtrl::SetUnitFlanking(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const string key = luaL_checkstring(L, 2);

	if (key == "mode") {
		unit->flankingBonusMode = luaL_checkint(L, 3);
	}
	else if (key == "dir") {
		float3 dir(luaL_checkfloat(L, 3),
		           luaL_checkfloat(L, 4),
		           luaL_checkfloat(L, 5));
		unit->flankingBonusDir = dir.Normalize();
	}
	else if (key == "moveFactor") {
		unit->flankingBonusMobilityAdd = luaL_checkfloat(L, 3);
	}
	else if (key == "minDamage") {
		const float minDamage = luaL_checkfloat(L, 3);
		const float maxDamage = unit->flankingBonusAvgDamage +
		                        unit->flankingBonusDifDamage;
		unit->flankingBonusAvgDamage = (maxDamage + minDamage) * 0.5f;
		unit->flankingBonusDifDamage = (maxDamage - minDamage) * 0.5f;
	}
	else if (key == "maxDamage") {
		const float maxDamage = luaL_checkfloat(L, 3);
		const float minDamage = unit->flankingBonusAvgDamage -
		                        unit->flankingBonusDifDamage;
		unit->flankingBonusAvgDamage = (maxDamage + minDamage) * 0.5f;
		unit->flankingBonusDifDamage = (maxDamage - minDamage) * 0.5f;
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
		unit->travel = lua_tofloat(L, 2);
	}
	if (lua_isnumber(L, 3)) {
		unit->travelPeriod = lua_tofloat(L, 3);
	}
	return 0;
}


int LuaSyncedCtrl::SetUnitFuel(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	unit->currentFuel = luaL_checkfloat(L, 2);
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
		const float3 pos(luaL_checkfloat(L, 2),
		                 luaL_checkfloat(L, 3),
		                 luaL_checkfloat(L, 4));
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

	const float xs = luaL_checkfloat(L, 2);
	const float ys = luaL_checkfloat(L, 3);
	const float zs = luaL_checkfloat(L, 4);
	const float xo = luaL_checkfloat(L, 5);
	const float yo = luaL_checkfloat(L, 6);
	const float zo = luaL_checkfloat(L, 7);
	const int vType = luaL_checkint(L,  8);
	const int tType = luaL_checkint(L,  9);
	const int pAxis = luaL_checkint(L, 10);

	const float3 scales(xs, ys, zs);
	const float3 offsets(xo, yo, zo);

	unit->collisionVolume->Init(scales, offsets, vType, tType, pAxis);

	return 0;
}

int LuaSyncedCtrl::SetUnitPieceCollisionVolumeData(lua_State* L)
{
	const int argc = lua_gettop(L);

	if (argc != 6 && argc != 14) {
		return 0;
	}

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL) {
		return 0;
	}

	if (!lua_isboolean(L, 3) || !lua_isboolean(L, 4)) { return 0; }
	if (!lua_isboolean(L, 5) || !lua_isboolean(L, 6)) { return 0; }

	LocalModel* localModel = unit->localmodel;

	const int  pieceIndex   = luaL_checkint(L, 2);
	const bool affectLocal  = lua_toboolean(L, 3);
	const bool affectGlobal = lua_toboolean(L, 4);
	const bool enableLocal  = lua_toboolean(L, 5);
	const bool enableGlobal = lua_toboolean(L, 6);

	if (pieceIndex < 0 || pieceIndex >= localModel->pieces.size()) {
		return 0;
	}

	if (!affectLocal && !affectGlobal) {
		return 0;
	}

	LocalModelPiece* lmp = localModel->pieces[pieceIndex];
	S3DModelPiece*   omp = lmp->original;

	const float xs  = luaL_checkfloat(L,  7);
	const float ys  = luaL_checkfloat(L,  8);
	const float zs  = luaL_checkfloat(L,  9);
	const float xo  = luaL_checkfloat(L, 10);
	const float yo  = luaL_checkfloat(L, 11);
	const float zo  = luaL_checkfloat(L, 12);
	const int vType = luaL_checkint(L, 13);
	const int pAxis = luaL_checkint(L, 14);

	const float3 scales(xs, ys, zs);
	const float3 offset(xo, yo, zo);

	if (affectLocal) {
		// affects this unit only
		if (enableLocal) {
			if (argc == 14) {
				lmp->colvol->Init(scales, offset, vType, COLVOL_TEST_CONT, pAxis);
			}
			lmp->colvol->Enable();
		} else {
			lmp->colvol->Disable();
		}
	}

	if (affectGlobal) {
		// affects all future units with this model
		if (enableGlobal) {
			if (argc == 14) {
				omp->colvol->Init(scales, offset, vType, COLVOL_TEST_CONT, pAxis);
			}
			omp->colvol->Enable();
		} else {
			omp->colvol->Disable();
		}
	}

	return 0;
}



int LuaSyncedCtrl::SetUnitSensorRadius(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const string key = luaL_checkstring(L, 2);
	const float radius = luaL_checkfloat(L, 3);

	const int radarDiv    = radarhandler->radarDiv;
	const int radarRadius = (int)(radius * radarhandler->invRadarDiv);

	if (key == "los") {
		const int losRange = (int)(radius * loshandler->invLosDiv);
		unit->ChangeLos(losRange, unit->realAirLosRadius);
		unit->realLosRadius = losRange;
		lua_pushnumber(L, unit->losRadius * loshandler->losDiv);
	} else if (key == "airLos") {
		const int airRange = (int)(radius * loshandler->invAirDiv);
		unit->ChangeLos(unit->realLosRadius, airRange);
		unit->realAirLosRadius = airRange;
		lua_pushnumber(L, unit->airLosRadius * loshandler->airDiv);
	} else if (key == "radar") {
		unit->ChangeSensorRadius(&unit->radarRadius, radarRadius);
		lua_pushnumber(L, unit->radarRadius * radarDiv);
	} else if (key == "sonar") {
		unit->ChangeSensorRadius(&unit->sonarRadius, radarRadius);
		lua_pushnumber(L, unit->sonarRadius * radarDiv);
	} else if (key == "seismic") {
		unit->ChangeSensorRadius(&unit->seismicRadius, radarRadius);
		lua_pushnumber(L, unit->seismicRadius * radarDiv);
	} else if (key == "radarJammer") {
		unit->ChangeSensorRadius(&unit->jammerRadius, radarRadius);
		lua_pushnumber(L, unit->jammerRadius * radarDiv);
	} else if (key == "sonarJammer") {
		unit->ChangeSensorRadius(&unit->sonarJamRadius, radarRadius);
		lua_pushnumber(L, unit->sonarJamRadius * radarDiv);
	} else {
		return 0; // unknown sensor type
	}

	return 1;
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
	const float3 pos(luaL_checkfloat(L, 2),
									 luaL_checkfloat(L, 3),
									 luaL_checkfloat(L, 4));
	const float radius = luaL_optfloat(L, 5, 0.0f);
	const float speed  = luaL_optfloat(L, 6, unit->maxSpeed * 2.0f);

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

	if (lua_isnumber(L, 4)) {
		float3 pos(luaL_checkfloat(L, 2),
							 luaL_checkfloat(L, 3),
							 luaL_checkfloat(L, 4));
		unit->ForcedMove(pos);
		return 0;
	}

	float x, y, z;
	x = luaL_checkfloat(L, 2);
	z = luaL_checkfloat(L, 3);
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
	const float3 rot(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
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
	float3 dir(luaL_checkfloat(L, 2),
	           luaL_checkfloat(L, 3),
	           luaL_checkfloat(L, 4));
	unit->speed = dir;
	return 0;
}


int LuaSyncedCtrl::AddUnitDamage(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float damage   = luaL_checkfloat(L, 2);
	const int paralyze   = luaL_optint(L, 3, 0);
	const int attackerID = luaL_optint(L, 4, -1);
	const int weaponID   = luaL_optint(L, 5, -1);
	const float3 impulse = float3(luaL_optfloat(L, 6, 0.0f),
	                              luaL_optfloat(L, 7, 0.0f),
	                              luaL_optfloat(L, 8, 0.0f));

	CUnit* attacker = NULL;
	if (attackerID >= 0) {
		if (static_cast<size_t>(attackerID) >= uh->MaxUnits()) {
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
	float3 impulse(luaL_checkfloat(L, 2),
	               luaL_checkfloat(L, 3),
	               luaL_checkfloat(L, 4));
	unit->residualImpulse += impulse;
	unit->moveType->ImpulseAdded();
	return 0;
}


int LuaSyncedCtrl::AddUnitSeismicPing(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float pingSize = luaL_checkfloat(L, 2);
	unit->DoSeismicPing(pingSize);
	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::AddUnitResource(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const string type = luaL_checkstring(L, 2);

	const float value = max(0.0f, luaL_checkfloat(L, 3));

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
		CTeam* team = teamHandler->Team(unit->team);
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
	else {
		luaL_error(L, "Incorrect arguments to UseUnitResource()");
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
	groundDecals->ForceRemoveBuilding(building);
	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::CreateFeature(lua_State* L)
{
	CheckAllowGameChanges(L);

	const FeatureDef* featureDef = NULL;
	if (lua_israwstring(L, 1)) {
		featureDef = featureHandler->GetFeatureDef(lua_tostring(L, 1));
	} else if (lua_israwnumber(L, 1)) {
		featureDef = featureHandler->GetFeatureDefByID(lua_toint(L, 1));
	}
	if (featureDef == NULL) {
		return 0; // do not error  (featureDefs are dynamic)
	}

	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));

	short int heading = 0;
	if (lua_isnumber(L, 5)) {
		heading = lua_toint(L, 5);
	}

	int team = CtrlTeam();
	if (team < 0) {
		team = -1; // default to global for AllAccessTeam
	}

	// FIXME -- separate teamcolor/allyteam arguments

	if (lua_isnumber(L, 6)) {
		team = lua_toint(L, 6);
		if (team < -1) {
			team = -1;
		} else if (team >= teamHandler->ActiveTeams()) {
			return 0;
		}
	}

	const int allyTeam = (team < 0) ? -1 : teamHandler->AllyTeam(team);
	if (!CanControlFeatureAllyTeam(allyTeam)) {
		luaL_error(L, "CreateFeature() bad team permission %d", team);
	}

	if (inCreateFeature) {
		luaL_error(L, "CreateFeature() recursion is not permitted");
	}

	// use SetFeatureResurrect() to fill in the missing bits
	inCreateFeature = true;
	CFeature* feature = new CFeature();
	feature->Initialize(pos, featureDef, heading, 0, team, allyTeam, "");
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
	const int team = luaL_checkint(L, 2);
	if (team >= teamHandler->ActiveTeams()) {
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
	feature->health = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetFeatureReclaim(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	feature->reclaimLeft = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetFeaturePosition(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	feature->ForcedMove(pos);
	return 0;
}


int LuaSyncedCtrl::SetFeatureDirection(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	float3 dir(luaL_checkfloat(L, 2),
	           luaL_checkfloat(L, 3),
	           luaL_checkfloat(L, 4));
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
	feature->createdFromUnit = luaL_checkstring(L, 2);

	const int args = lua_gettop(L); // number of arguments
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
	if (!lua_isboolean(L, 2)) {
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

	const float xs = luaL_checkfloat(L, 2);
	const float ys = luaL_checkfloat(L, 3);
	const float zs = luaL_checkfloat(L, 4);
	const float xo = luaL_checkfloat(L, 5);
	const float yo = luaL_checkfloat(L, 6);
	const float zo = luaL_checkfloat(L, 7);
	const int vType = luaL_checkint(L,  8);
	const int tType = luaL_checkint(L,  9);
	const int pAxis = luaL_checkint(L, 10);

	const float3 scales(xs, ys, zs);
	const float3 offsets(xo, yo, zo);

	feature->collisionVolume->Init(scales, offsets, vType, tType, pAxis);

	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetProjectilePosition(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);
	if (proj == NULL) {
		return 0;
	}

	proj->pos.x = luaL_optfloat(L, 2, 0.0f);
	proj->pos.y = luaL_optfloat(L, 3, 0.0f);
	proj->pos.z = luaL_optfloat(L, 4, 0.0f);

	return 0;
}


int LuaSyncedCtrl::SetProjectileVelocity(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);
	if (proj == NULL) {
		return 0;
	}

	proj->speed.x = luaL_optfloat(L, 2, 0.0f);
	proj->speed.y = luaL_optfloat(L, 3, 0.0f);
	proj->speed.z = luaL_optfloat(L, 4, 0.0f);

	return 0;
}

int LuaSyncedCtrl::SetProjectileCollision(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);
	if (proj == NULL) {
		return 0;
	}

	proj->Collision();

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
		fx1 = fx2 = luaL_checkfloat(L, 1);
		fz1 = fz2 = luaL_checkfloat(L, 2);
		factor    = luaL_checkfloat(L, 3);
	}
	else if (args == 5) {
		fx1    = luaL_checkfloat(L, 1);
		fz1    = luaL_checkfloat(L, 2);
		fx2    = luaL_checkfloat(L, 3);
		fz2    = luaL_checkfloat(L, 4);
		factor = luaL_checkfloat(L, 5);
		if (fx1 > fx2) {
			swap(fx1, fx2);
		}
		if (fz1 > fz2) {
			swap(fz1, fz2);
		}
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

	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			const int index = (z * (gs->mapx + 1)) + x;
			readmap->SetHeight(index, height);
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

	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			const int index = (z * (gs->mapx + 1)) + x;
			readmap->AddHeight(index, height);
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

	const float* origMap = readmap->orgheightmap;
	const float* currMap = readmap->GetHeightmap();

	if (origFactor == 1.0f) {
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int idx = (z * (gs->mapx + 1)) + x;

				readmap->SetHeight(idx, origMap[idx]);
			}
		}
	}
	else {
		const float currFactor = (1.0f - origFactor);
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int index = (z * (gs->mapx + 1)) + x;
				const float ofh = origFactor * origMap[index];
				const float cfh = currFactor * currMap[index];
				readmap->SetHeight(index, ofh + cfh);
			}
		}
	}

	mapDamage->RecalcArea(x1, x2, z1, z2);
	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::AddHeightMap(lua_State* L)
{
	if (!inHeightMap) {
		luaL_error(L, "AddHeightMap() can only be called in SetHeightMapFunc()");
	}

	const float xl = luaL_checkfloat(L, 1);
	const float zl = luaL_checkfloat(L, 2);
	const float h  = luaL_checkfloat(L, 3);

	// quantize
	const int x = (int)(xl / SQUARE_SIZE);
	const int z = (int)(zl / SQUARE_SIZE);

	// discard invalid coordinates
	if ((x < 0) || (x > gs->mapx) ||
	    (z < 0) || (z > gs->mapy)) {
		return 0;
	}

	const int index = (z * (gs->mapx + 1)) + x;
	const float oldHeight = readmap->GetHeightmap()[index];
	heightMapAmountChanged += streflop::fabsf(h);

	// update RecalcArea()
	if (x < heightMapx1) { heightMapx1 = x; }
	if (x > heightMapx2) { heightMapx2 = x; }
	if (z < heightMapz1) { heightMapz1 = z; }
	if (z > heightMapz2) { heightMapz2 = z; }

	readmap->AddHeight(index, h);
	// push the new height
	lua_pushnumber(L, oldHeight + h);
	return 1;
}


int LuaSyncedCtrl::SetHeightMap(lua_State* L)
{
	if (!inHeightMap) {
		luaL_error(L, "SetHeightMap() can only be called in SetHeightMapFunc()");
	}

	const float xl = luaL_checkfloat(L, 1);
	const float zl = luaL_checkfloat(L, 2);
	const float h  = luaL_checkfloat(L, 3);

	// quantize
	const int x = (int)(xl / SQUARE_SIZE);
	const int z = (int)(zl / SQUARE_SIZE);

	// discard invalid coordinates
	if ((x < 0) || (x > gs->mapx) ||
	    (z < 0) || (z > gs->mapy)) {
		return 0;
	}

	const int index = (z * (gs->mapx + 1)) + x;
	const float oldHeight = readmap->GetHeightmap()[index];
	float height = oldHeight;

	if (lua_israwnumber(L, 4)) {
		const float t = lua_tofloat(L, 4);
		height += (h - oldHeight) * t;
	} else{
		height = h;
	}

	const float heightDiff = (height - oldHeight);
	heightMapAmountChanged += streflop::fabsf(heightDiff);

	// update RecalcArea()
	if (x < heightMapx1) { heightMapx1 = x; }
	if (x > heightMapx2) { heightMapx2 = x; }
	if (z < heightMapz1) { heightMapz1 = z; }
	if (z > heightMapz2) { heightMapz2 = z; }

	readmap->SetHeight(index, height);
	lua_pushnumber(L, heightDiff);
	return 1;
}


int LuaSyncedCtrl::SetHeightMapFunc(lua_State* L)
{
	if (mapDamage->disabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isfunction(L, 1)) {
		luaL_error(L, "Incorrect arguments to Spring.SetHeightMapFunc(func, ...)");
	}

	if (inHeightMap) {
		luaL_error(L, "SetHeightMapFunc() recursion is not permitted");
	}

	heightMapx1 = gs->mapx;
	heightMapx2 = -1;
	heightMapz1 = gs->mapy;
	heightMapz2 = 0;
	heightMapAmountChanged = 0.0f;

	inHeightMap = true;
	const int error = lua_pcall(L, (args - 1), 0, 0);
	inHeightMap = false;

	if (error != 0) {
		logOutput.Print("Spring.SetHeightMapFunc: error(%i) = %s",
		                error, lua_tostring(L, -1));
		lua_error(L);
	}

	if (heightMapx2 > -1) {
		mapDamage->RecalcArea(heightMapx1, heightMapx2, heightMapz1, heightMapz2);
	}

	lua_pushnumber(L, heightMapAmountChanged);
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SpawnCEG(lua_State* L)
{
	const string name = luaL_checkstring(L, 1);
	const float3 pos(luaL_optfloat(L, 2, 0.0f),
									 luaL_optfloat(L, 3, 0.0f),
									 luaL_optfloat(L, 4, 0.0f));
	const float3 dir(luaL_optfloat(L, 5, 0.0f),
									 luaL_optfloat(L, 6, 0.0f),
									 luaL_optfloat(L, 7, 0.0f));
	const float rad = luaL_optfloat(L, 8, 0.0f);
	const float dmg = luaL_optfloat(L, 9, 0.0f);
	const float dmgMod = 1.0f;
	if (name.empty()) {
		return 0;
	}

	CCustomExplosionGenerator g;
	g.Load(explGenHandler, name);
	g.Explosion(pos, dmg, rad, 0x0, dmgMod, 0x0, dir);

	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetNoPause(lua_State* L)
{
	if (!FullCtrl()) {
		return 0;
	}
	if (!lua_isboolean(L, 1)) {
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
	if (!lua_isboolean(L, 1)) {
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
	const float expGrade = luaL_checkfloat(L, 1);
	CUnit::SetExpGrade(expGrade);

	// NOTE: for testing, should be using modrules.tdf
	if (gs->cheatEnabled) {
		if (lua_isnumber(L, 2)) {
			CUnit::SetExpPowerScale(lua_tofloat(L, 2));
		}
		if (lua_isnumber(L, 3)) {
			CUnit::SetExpHealthScale(lua_tofloat(L, 3));
		}
		if (lua_isnumber(L, 4)) {
			CUnit::SetExpReloadScale(lua_tofloat(L, 4));
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
		value = lua_toint(L, -1);
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
			floatvec.push_back(lua_tofloat(L, -1));
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
			intvec.push_back(lua_toint(L, -1));
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
		    ParseNamedBool(L,   key, "hidden",      cd.hidden)     ||
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

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	vector<CommandDescription>& cmdDescs = unit->commandAI->possibleCommands;

	const int cmdDescID = luaL_checkint(L, 2) - 1;
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
		cmdDescID = lua_toint(L, 2) - 1;
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

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	vector<CommandDescription>& cmdDescs = unit->commandAI->possibleCommands;

	int cmdDescID = (int)cmdDescs.size() - 1;
	if (lua_isnumber(L, 2)) {
		cmdDescID = lua_toint(L, 2) - 1;
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
