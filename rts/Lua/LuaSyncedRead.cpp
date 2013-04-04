/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaSyncedRead.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaMetalMap.h"
#include "LuaPathFinder.h"
#include "LuaRules.h"
#include "LuaRulesParams.h"
#include "LuaUtils.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/MoveTypes/StaticMoveType.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/bitops.h"
#include "System/myMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"

#include <set>
#include <list>
#include <map>
#include <cctype>


using std::min;
using std::max;
using std::map;
using std::set;

static const LuaHashString hs_n("n");

// 0 and positive numbers are teams (not allyTeams)
enum UnitAllegiance {
	AllUnits   = -1,
	MyUnits    = -2,
	AllyUnits  = -3,
	EnemyUnits = -4
};


/******************************************************************************/
/******************************************************************************/

bool LuaSyncedRead::PushEntries(lua_State* L)
{
	// allegiance constants
	LuaPushNamedNumber(L, "ALL_UNITS",   AllUnits);
	LuaPushNamedNumber(L, "MY_UNITS",    MyUnits);
	LuaPushNamedNumber(L, "ALLY_UNITS",  AllyUnits);
	LuaPushNamedNumber(L, "ENEMY_UNITS", EnemyUnits);

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	// READ routines, sync safe

	REGISTER_LUA_CFUNC(IsCheatingEnabled);
	REGISTER_LUA_CFUNC(IsGodModeEnabled);
	REGISTER_LUA_CFUNC(IsDevLuaEnabled);
	REGISTER_LUA_CFUNC(IsEditDefsEnabled);
	REGISTER_LUA_CFUNC(AreHelperAIsEnabled);
	REGISTER_LUA_CFUNC(FixedAllies);

	REGISTER_LUA_CFUNC(IsGameOver);

	REGISTER_LUA_CFUNC(GetGaiaTeamID);

	REGISTER_LUA_CFUNC(GetGameSpeed);
	REGISTER_LUA_CFUNC(GetGameFrame);
	REGISTER_LUA_CFUNC(GetGameSeconds);

	REGISTER_LUA_CFUNC(GetGameRulesParam);
	REGISTER_LUA_CFUNC(GetGameRulesParams);

	REGISTER_LUA_CFUNC(GetMapOptions);
	REGISTER_LUA_CFUNC(GetModOptions);

	REGISTER_LUA_CFUNC(GetWind);

	REGISTER_LUA_CFUNC(GetHeadingFromVector);
	REGISTER_LUA_CFUNC(GetVectorFromHeading);

	REGISTER_LUA_CFUNC(GetSideData);

	REGISTER_LUA_CFUNC(GetAllyTeamStartBox);
	REGISTER_LUA_CFUNC(GetTeamStartPosition);

	REGISTER_LUA_CFUNC(GetPlayerList);
	REGISTER_LUA_CFUNC(GetTeamList);
	REGISTER_LUA_CFUNC(GetAllyTeamList);

	REGISTER_LUA_CFUNC(GetPlayerInfo);
	REGISTER_LUA_CFUNC(GetPlayerControlledUnit);
	REGISTER_LUA_CFUNC(GetAIInfo);

	REGISTER_LUA_CFUNC(GetTeamInfo);
	REGISTER_LUA_CFUNC(GetTeamResources);
	REGISTER_LUA_CFUNC(GetTeamUnitStats);
	REGISTER_LUA_CFUNC(GetTeamResourceStats);
	REGISTER_LUA_CFUNC(GetTeamRulesParam);
	REGISTER_LUA_CFUNC(GetTeamRulesParams);
	REGISTER_LUA_CFUNC(GetTeamStatsHistory);
	REGISTER_LUA_CFUNC(GetTeamLuaAI);

	REGISTER_LUA_CFUNC(GetAllyTeamInfo);
	REGISTER_LUA_CFUNC(AreTeamsAllied);
	REGISTER_LUA_CFUNC(ArePlayersAllied);

	REGISTER_LUA_CFUNC(ValidUnitID);
	REGISTER_LUA_CFUNC(ValidFeatureID);

	REGISTER_LUA_CFUNC(GetAllUnits);
	REGISTER_LUA_CFUNC(GetTeamUnits);
	REGISTER_LUA_CFUNC(GetTeamUnitsSorted);
	REGISTER_LUA_CFUNC(GetTeamUnitsCounts);
	REGISTER_LUA_CFUNC(GetTeamUnitsByDefs);
	REGISTER_LUA_CFUNC(GetTeamUnitDefCount);
	REGISTER_LUA_CFUNC(GetTeamUnitCount);

	REGISTER_LUA_CFUNC(GetUnitsInRectangle);
	REGISTER_LUA_CFUNC(GetUnitsInBox);
	REGISTER_LUA_CFUNC(GetUnitsInPlanes);
	REGISTER_LUA_CFUNC(GetUnitsInSphere);
	REGISTER_LUA_CFUNC(GetUnitsInCylinder);

	REGISTER_LUA_CFUNC(GetFeaturesInRectangle);
	REGISTER_LUA_CFUNC(GetFeaturesInSphere);
	REGISTER_LUA_CFUNC(GetFeaturesInCylinder);
	REGISTER_LUA_CFUNC(GetProjectilesInRectangle);

	REGISTER_LUA_CFUNC(GetUnitNearestAlly);
	REGISTER_LUA_CFUNC(GetUnitNearestEnemy);

	REGISTER_LUA_CFUNC(GetUnitTooltip);
	REGISTER_LUA_CFUNC(GetUnitDefID);
	REGISTER_LUA_CFUNC(GetUnitTeam);
	REGISTER_LUA_CFUNC(GetUnitAllyTeam);
	REGISTER_LUA_CFUNC(GetUnitNeutral);
	REGISTER_LUA_CFUNC(GetUnitHealth);
	REGISTER_LUA_CFUNC(GetUnitIsDead);
	REGISTER_LUA_CFUNC(GetUnitIsStunned);
	REGISTER_LUA_CFUNC(GetUnitResources);
	REGISTER_LUA_CFUNC(GetUnitMetalExtraction);
	REGISTER_LUA_CFUNC(GetUnitExperience);
	REGISTER_LUA_CFUNC(GetUnitStates);
	REGISTER_LUA_CFUNC(GetUnitArmored);
	REGISTER_LUA_CFUNC(GetUnitIsActive);
	REGISTER_LUA_CFUNC(GetUnitIsCloaked);
	REGISTER_LUA_CFUNC(GetUnitSelfDTime);
	REGISTER_LUA_CFUNC(GetUnitStockpile);
	REGISTER_LUA_CFUNC(GetUnitSensorRadius);
	REGISTER_LUA_CFUNC(GetUnitHeight);
	REGISTER_LUA_CFUNC(GetUnitRadius);
	REGISTER_LUA_CFUNC(GetUnitPosition);
	REGISTER_LUA_CFUNC(GetUnitBasePosition);
	REGISTER_LUA_CFUNC(GetUnitVectors);
	REGISTER_LUA_CFUNC(GetUnitDirection);
	REGISTER_LUA_CFUNC(GetUnitHeading);
	REGISTER_LUA_CFUNC(GetUnitVelocity);
	REGISTER_LUA_CFUNC(GetUnitBuildFacing);
	REGISTER_LUA_CFUNC(GetUnitIsBuilding);
	REGISTER_LUA_CFUNC(GetUnitCurrentBuildPower);
	REGISTER_LUA_CFUNC(GetUnitNanoPieces);
	REGISTER_LUA_CFUNC(GetUnitTransporter);
	REGISTER_LUA_CFUNC(GetUnitIsTransporting);
	REGISTER_LUA_CFUNC(GetUnitShieldState);
	REGISTER_LUA_CFUNC(GetUnitFlanking);
	REGISTER_LUA_CFUNC(GetUnitWeaponState);
	REGISTER_LUA_CFUNC(GetUnitWeaponVectors);
	REGISTER_LUA_CFUNC(GetUnitWeaponTryTarget);
	REGISTER_LUA_CFUNC(GetUnitWeaponTestTarget);
	REGISTER_LUA_CFUNC(GetUnitWeaponTestRange);
	REGISTER_LUA_CFUNC(GetUnitWeaponHaveFreeLineOfFire);
	REGISTER_LUA_CFUNC(GetUnitTravel);
	REGISTER_LUA_CFUNC(GetUnitFuel);
	REGISTER_LUA_CFUNC(GetUnitEstimatedPath);
	REGISTER_LUA_CFUNC(GetUnitLastAttacker);
	REGISTER_LUA_CFUNC(GetUnitLastAttackedPiece);
	REGISTER_LUA_CFUNC(GetUnitLosState);
	REGISTER_LUA_CFUNC(GetUnitSeparation);
	REGISTER_LUA_CFUNC(GetUnitDefDimensions);
	REGISTER_LUA_CFUNC(GetUnitCollisionVolumeData);
	REGISTER_LUA_CFUNC(GetUnitPieceCollisionVolumeData);

	REGISTER_LUA_CFUNC(GetUnitBlocking);
	REGISTER_LUA_CFUNC(GetUnitMoveTypeData);

	REGISTER_LUA_CFUNC(GetUnitCommands);
	REGISTER_LUA_CFUNC(GetFactoryCounts);
	REGISTER_LUA_CFUNC(GetFactoryCommands);

	REGISTER_LUA_CFUNC(GetCommandQueue);
	REGISTER_LUA_CFUNC(GetFullBuildQueue);
	REGISTER_LUA_CFUNC(GetRealBuildQueue);

	REGISTER_LUA_CFUNC(GetUnitCmdDescs);
	REGISTER_LUA_CFUNC(FindUnitCmdDesc);

	REGISTER_LUA_CFUNC(GetUnitRulesParam);
	REGISTER_LUA_CFUNC(GetUnitRulesParams);

	REGISTER_LUA_CFUNC(GetAllFeatures);
	REGISTER_LUA_CFUNC(GetFeatureDefID);
	REGISTER_LUA_CFUNC(GetFeatureTeam);
	REGISTER_LUA_CFUNC(GetFeatureAllyTeam);
	REGISTER_LUA_CFUNC(GetFeatureHealth);
	REGISTER_LUA_CFUNC(GetFeatureHeight);
	REGISTER_LUA_CFUNC(GetFeatureRadius);
	REGISTER_LUA_CFUNC(GetFeaturePosition);
	REGISTER_LUA_CFUNC(GetFeatureDirection);
	REGISTER_LUA_CFUNC(GetFeatureHeading);
	REGISTER_LUA_CFUNC(GetFeatureResources);
	REGISTER_LUA_CFUNC(GetFeatureNoSelect);
	REGISTER_LUA_CFUNC(GetFeatureResurrect);
	REGISTER_LUA_CFUNC(GetFeatureCollisionVolumeData);

	REGISTER_LUA_CFUNC(GetProjectilePosition);
	REGISTER_LUA_CFUNC(GetProjectileVelocity);
	REGISTER_LUA_CFUNC(GetProjectileGravity);
	REGISTER_LUA_CFUNC(GetProjectileSpinAngle);
	REGISTER_LUA_CFUNC(GetProjectileSpinSpeed);
	REGISTER_LUA_CFUNC(GetProjectileSpinVec);
	REGISTER_LUA_CFUNC(GetProjectileTarget);
	REGISTER_LUA_CFUNC(GetProjectileType);
	REGISTER_LUA_CFUNC(GetProjectileName);

	REGISTER_LUA_CFUNC(GetGroundHeight);
	REGISTER_LUA_CFUNC(GetGroundOrigHeight);
	REGISTER_LUA_CFUNC(GetGroundNormal);
	REGISTER_LUA_CFUNC(GetGroundInfo);
	REGISTER_LUA_CFUNC(GetGroundBlocked);
	REGISTER_LUA_CFUNC(GetGroundExtremes);
	REGISTER_LUA_CFUNC(GetTerrainTypeData);

	REGISTER_LUA_CFUNC(GetSmoothMeshHeight);

	REGISTER_LUA_CFUNC(TestMoveOrder);
	REGISTER_LUA_CFUNC(TestBuildOrder);
	REGISTER_LUA_CFUNC(Pos2BuildPos);
	REGISTER_LUA_CFUNC(GetPositionLosState);
	REGISTER_LUA_CFUNC(IsPosInLos);
	REGISTER_LUA_CFUNC(IsPosInRadar);
	REGISTER_LUA_CFUNC(IsPosInAirLos);
	REGISTER_LUA_CFUNC(GetClosestValidPosition);

	REGISTER_LUA_CFUNC(GetUnitPieceMap);
	REGISTER_LUA_CFUNC(GetUnitPieceList);
	REGISTER_LUA_CFUNC(GetUnitPieceInfo);
	REGISTER_LUA_CFUNC(GetUnitPiecePosition);
	REGISTER_LUA_CFUNC(GetUnitPieceDirection);
	REGISTER_LUA_CFUNC(GetUnitPiecePosDir);
	REGISTER_LUA_CFUNC(GetUnitPieceMatrix);
	REGISTER_LUA_CFUNC(GetUnitScriptPiece);
	REGISTER_LUA_CFUNC(GetUnitScriptNames);

	REGISTER_LUA_CFUNC(GetCOBUnitVar);
	REGISTER_LUA_CFUNC(GetCOBTeamVar);
	REGISTER_LUA_CFUNC(GetCOBAllyTeamVar);
	REGISTER_LUA_CFUNC(GetCOBGlobalVar);

	if (!LuaMetalMap::PushReadEntries(L))
		return false;

	if (!LuaPathFinder::PushEntries(L))
		return false;

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

static inline bool IsAlliedTeam(lua_State* L, int team)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		return CLuaHandle::GetHandleFullRead(L);
	}
	return (teamHandler->AllyTeam(team) == CLuaHandle::GetHandleReadAllyTeam(L));
}


static inline bool IsAlliedAllyTeam(lua_State* L, int allyTeam)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		return CLuaHandle::GetHandleFullRead(L);
	}
	return (allyTeam == CLuaHandle::GetHandleReadAllyTeam(L));
}


static inline bool IsAllyUnit(lua_State* L, const CUnit* unit)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		return CLuaHandle::GetHandleFullRead(L);
	}
	return (unit->allyteam == CLuaHandle::GetHandleReadAllyTeam(L));
}


static inline bool IsEnemyUnit(lua_State* L, const CUnit* unit)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		return !CLuaHandle::GetHandleFullRead(L);
	}
	return (unit->allyteam != CLuaHandle::GetHandleReadAllyTeam(L));
}


static inline bool IsUnitVisible(lua_State* L, const CUnit* unit)
{
	if (IsAllyUnit(L, unit)) {
		return true;
	}
	return !!(unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & (LOS_INLOS | LOS_INRADAR));
}


static inline bool IsUnitInLos(lua_State* L, const CUnit* unit)
{
	if (IsAllyUnit(L, unit)) {
		return true;
	}
	return (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INLOS);
}


static inline bool IsUnitTyped(lua_State* L, const CUnit* unit)
{
	if (IsAllyUnit(L, unit)) {
		return true;
	}
	const unsigned short losStatus = unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
	if ((losStatus & LOS_INLOS) ||
	    ((losStatus & prevMask) == prevMask)) {
		return true;
	}
	return false;
}


static inline const UnitDef* EffectiveUnitDef(lua_State* L, const CUnit* unit)
{
	const UnitDef* ud = unit->unitDef;
	if (IsAllyUnit(L, unit)) {
		return ud;
	} else if (ud->decoyDef) {
		return ud->decoyDef;
	} else {
		return ud;
	}
}


static inline bool IsFeatureVisible(lua_State* L, const CFeature* feature)
{
	if (CLuaHandle::GetHandleFullRead(L))
		return true;
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0)
		return false;

	return feature->IsInLosForAllyTeam(CLuaHandle::GetHandleReadAllyTeam(L));
}

static inline bool IsProjectileVisible(lua_State* L, const ProjectileMapValPair& pp)
{
	const CProjectile* pro = pp.first;
	const int proAllyteam = pp.second;

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		return CLuaHandle::GetHandleFullRead(L);
	}
	if ((CLuaHandle::GetHandleReadAllyTeam(L) != proAllyteam) &&
	    (!loshandler->InLos(pro->pos, CLuaHandle::GetHandleReadAllyTeam(L)))) {
		return false;
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/


static int PushCollisionVolumeData(lua_State* L, const CollisionVolume* vol) {
	lua_pushnumber(L, vol->GetScales().x);
	lua_pushnumber(L, vol->GetScales().y);
	lua_pushnumber(L, vol->GetScales().z);
	lua_pushnumber(L, vol->GetOffsets().x);
	lua_pushnumber(L, vol->GetOffsets().y);
	lua_pushnumber(L, vol->GetOffsets().z);
	lua_pushnumber(L, vol->GetVolumeType());
	lua_pushnumber(L, int(vol->UseContHitTest()));
	lua_pushnumber(L, vol->GetPrimaryAxis());
	lua_pushboolean(L, vol->IgnoreHits());
	return 10;
}

static int PushTerrainTypeData(lua_State* L, const CMapInfo::TerrainType* tt, bool groundInfo) {
	lua_pushsstring(L, tt->name);

	if (groundInfo) {
		assert(lua_isnumber(L, 1));
		assert(lua_isnumber(L, 2));
		// WTF is this still doing here?
		LuaMetalMap::GetMetalAmount(L);
	}

	lua_pushnumber(L, tt->hardness);
	lua_pushnumber(L, tt->tankSpeed);
	lua_pushnumber(L, tt->kbotSpeed);
	lua_pushnumber(L, tt->hoverSpeed);
	lua_pushnumber(L, tt->shipSpeed);
	lua_pushboolean(L, tt->receiveTracks);
	return (7 + int(groundInfo));
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		if (caller != NULL) {
			luaL_error(L, "Bad unitID parameter in %s()\n", caller);
		} else {
			return NULL;
		}
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= unitHandler->MaxUnits())) {
		if (caller != NULL) {
			luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
		} else {
			return NULL;
		}
	}
	CUnit* unit = unitHandler->units[unitID];
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
	return IsUnitVisible(L, unit) ? unit : NULL;
}


static inline CUnit* ParseAllyUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	return IsAllyUnit(L, unit) ? unit : NULL;
}


static inline CUnit* ParseInLosUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	return IsUnitInLos(L, unit) ? unit : NULL;
}


static inline CUnit* ParseTypedUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	return IsUnitTyped(L, unit) ? unit : NULL;
}


static CProjectile* ParseProjectile(lua_State* L, const char* caller, int index)
{
	const int proID = luaL_checkint(L, index);
	const ProjectileMapValPair* pp = projectileHandler->GetMapPairBySyncedID(proID);
	if (!pp) {
		return NULL;
	}
	return IsProjectileVisible(L, *pp)? pp->first: NULL;
}


/******************************************************************************/


static inline CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "Bad teamID type in %s()\n", caller);
	}
	const int teamID = lua_toint(L, index);
	if (!teamHandler->IsValidTeam(teamID)) {
		luaL_error(L, "Bad teamID in %s\n", caller);
	}
	return teamHandler->Team(teamID);
}


static int ParseFloatArray(lua_State* L, float* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	// FIXME: changed this, test GetUnitsInPlanes() ...
	const int table = lua_gettop(L);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = lua_tofloat(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
}


static inline void CheckNoArgs(lua_State* L, const char* funcName)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "%s() takes no arguments", funcName);
	}
}


/******************************************************************************/

static int PushRulesParams(lua_State* L, const char* caller,
                          const LuaRulesParams::Params& params,
                          const LuaRulesParams::HashMap& paramsMap,
                          const int losStatus)
{
	lua_newtable(L);
	const int pCount = (int)params.size();
	for (int i = 0; i < pCount; i++) {
		const LuaRulesParams::Param& param = params[i];
		if (!(param.los & losStatus))
			continue;

		lua_pushnumber(L, i + 1);
		lua_newtable(L);

		LuaRulesParams::HashMap::const_iterator it;
		string name = "";
		for (it = paramsMap.begin(); it != paramsMap.end(); ++it) {
			if (it->second == i) {
				name = it->first;
				break;
			}
		}

		LuaPushNamedNumber(L, name, param.value);
		lua_rawset(L, -3);
	}

	// <i> is not consecutive due to the "continue"
	hs_n.PushNumber(L, pCount);

	return 1;
}


static int GetRulesParam(lua_State* L, const char* caller, int index,
                          const LuaRulesParams::Params& params,
                          const LuaRulesParams::HashMap& paramsMap,
                          const int& losStatus)
{
	int pIndex = -1;

	if (lua_israwnumber(L, index)) {
		pIndex = lua_toint(L, index) - 1;
	}
	else if (lua_israwstring(L, index)) {
		const string pName = lua_tostring(L, index);
		LuaRulesParams::HashMap::const_iterator it = paramsMap.find(pName);
		if (it != paramsMap.end()) {
			pIndex = it->second;
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	if ((pIndex < 0) || (pIndex >= (int)params.size())) {
		return 0;
	}

	const LuaRulesParams::Param& param = params[pIndex];

	if (param.los & losStatus) {
		lua_pushnumber(L, param.value);
		return 1;
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/
//
// The call-outs
//

int LuaSyncedRead::IsCheatingEnabled(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, gs->cheatEnabled);
	return 1;
}


int LuaSyncedRead::IsGodModeEnabled(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, gs->godMode);
	return 1;
}


int LuaSyncedRead::IsDevLuaEnabled(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, CLuaHandle::GetDevMode());
	return 1;
}


int LuaSyncedRead::IsEditDefsEnabled(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, gs->editDefsEnabled);
	return 1;
}


int LuaSyncedRead::AreHelperAIsEnabled(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (!game) {
		return 0;
	}
	lua_pushboolean(L, !gs->noHelperAIs);
	return 1;
}


int LuaSyncedRead::FixedAllies(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (!game) {
		return 0;
	}
	lua_pushboolean(L, !(gameSetup && !gameSetup->fixedAllies));
	return 1;
}


int LuaSyncedRead::IsGameOver(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, game->gameOver);
	return 1;
}


int LuaSyncedRead::GetGaiaTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (!gs->useLuaGaia) {
		return 0;
	}
	lua_pushnumber(L, teamHandler->GaiaTeamID());
	return 1;
}


int LuaSyncedRead::GetGameSpeed(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gs->wantedSpeedFactor);
	lua_pushnumber(L, gs->speedFactor);
	lua_pushboolean(L, gs->paused);
	return 3;
}


int LuaSyncedRead::GetGameFrame(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	const int dayFrames = GAME_SPEED * (24 * 60 * 60);
	lua_pushnumber(L, gs->frameNum % dayFrames);
	lua_pushnumber(L, gs->frameNum / dayFrames);
	return 2;
}


int LuaSyncedRead::GetGameSeconds(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	const float seconds = gs->frameNum / (float)GAME_SPEED;
	lua_pushnumber(L, seconds);
	return 1;
}


int LuaSyncedRead::GetWind(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, wind.GetCurrentWind().x);
	lua_pushnumber(L, wind.GetCurrentWind().y);
	lua_pushnumber(L, wind.GetCurrentWind().z);
	lua_pushnumber(L, wind.GetCurrentStrength());
	lua_pushnumber(L, wind.GetCurrentDirection().x);
	lua_pushnumber(L, wind.GetCurrentDirection().y);
	lua_pushnumber(L, wind.GetCurrentDirection().z);
	return 7;
}


/******************************************************************************/

int LuaSyncedRead::GetGameRulesParams(lua_State* L)
{
	const LuaRulesParams::Params&  params    = CLuaHandleSynced::GetGameParams();
	const LuaRulesParams::HashMap& paramsMap = CLuaHandleSynced::GetGameParamsMap();

	//! always readable for all
	const int losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;

	return PushRulesParams(L, __FUNCTION__, params, paramsMap, losMask);
}


int LuaSyncedRead::GetGameRulesParam(lua_State* L)
{
	const LuaRulesParams::Params&  params    = CLuaHandleSynced::GetGameParams();
	const LuaRulesParams::HashMap& paramsMap = CLuaHandleSynced::GetGameParamsMap();

	//! always readable for all
	const int losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;

	return GetRulesParam(L, __FUNCTION__, 1, params, paramsMap, losMask);
}


/******************************************************************************/

int LuaSyncedRead::GetMapOptions(lua_State* L)
{
	lua_newtable(L);
	if (!gameSetup) {
		return 1;
	}
	const map<string, string>& mapOpts = gameSetup->mapOptions;
	map<string, string>::const_iterator it;
	for (it = mapOpts.begin(); it != mapOpts.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_pushsstring(L, it->second);
		lua_rawset(L, -3);
	}
	return 1;
}


int LuaSyncedRead::GetModOptions(lua_State* L)
{
	lua_newtable(L);
	if (!gameSetup) {
		return 1;
	}
	const map<string, string>& modOpts = gameSetup->modOptions;
	map<string, string>::const_iterator it;
	for (it = modOpts.begin(); it != modOpts.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_pushsstring(L, it->second);
		lua_rawset(L, -3);
	}
	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetHeadingFromVector(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	const short int heading = ::GetHeadingFromVector(x, z);
	lua_pushnumber(L, heading);
	return 1;
}


int LuaSyncedRead::GetVectorFromHeading(lua_State* L)
{
	const short int h = (short int)luaL_checknumber(L, 1);
	const float3& vec = ::GetVectorFromHeading(h);
	lua_pushnumber(L, vec.x);
	lua_pushnumber(L, vec.z);
	return 2;
}


/******************************************************************************/

int LuaSyncedRead::GetSideData(lua_State* L)
{
	if (lua_israwstring(L, 1)) {
		const string sideName = lua_tostring(L, 1);
		const string& startUnit = sideParser.GetStartUnit(sideName);
		const string& caseName  = sideParser.GetCaseName(sideName);
		if (startUnit.empty()) {
			return 0;
		}
		lua_pushsstring(L, startUnit);
		lua_pushsstring(L, caseName);
		return 2;
	}
	else if (lua_israwnumber(L, 1)) {
		const unsigned int index = lua_toint(L, 1) - 1;
		if (!sideParser.ValidSide(index)) {
			return 0;
		}
		lua_pushsstring(L, sideParser.GetSideName(index));
		lua_pushsstring(L, sideParser.GetStartUnit(index));
		lua_pushsstring(L, sideParser.GetCaseName(index));
		return 3;
	}
	else {
		lua_newtable(L);
		const unsigned int sideCount = sideParser.GetCount();
		for (unsigned int i = 0; i < sideCount; i++) {
			lua_pushnumber(L, i + 1);
			lua_newtable(L); {
				LuaPushNamedString(L, "sideName",  sideParser.GetSideName(i));
				LuaPushNamedString(L, "caseName",  sideParser.GetCaseName(i));
				LuaPushNamedString(L, "startUnit", sideParser.GetStartUnit(i));
			}
			lua_rawset(L, -3);
		}
		return 1;
	}
	return 0;
}


/******************************************************************************/

int LuaSyncedRead::GetAllyTeamStartBox(lua_State* L)
{
	if (gameSetup == NULL) {
		return 0;
	}

	const int allyTeam = (int)luaL_checkint(L, 1);
	if ((allyTeam < 0) || ((size_t)allyTeam >= gameSetup->allyStartingData.size())) {
		return 0;
	}
	const float xmin = (gs->mapx * 8.0f) * gameSetup->allyStartingData[allyTeam].startRectLeft;
	const float zmin = (gs->mapy * 8.0f) * gameSetup->allyStartingData[allyTeam].startRectTop;
	const float xmax = (gs->mapx * 8.0f) * gameSetup->allyStartingData[allyTeam].startRectRight;
	const float zmax = (gs->mapy * 8.0f) * gameSetup->allyStartingData[allyTeam].startRectBottom;
	lua_pushnumber(L, xmin);
	lua_pushnumber(L, zmin);
	lua_pushnumber(L, xmax);
	lua_pushnumber(L, zmax);
	return 4;
}


int LuaSyncedRead::GetTeamStartPosition(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID)) {
		return 0;
	}
	const float3& pos = team->startPos;
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}


/******************************************************************************/

int LuaSyncedRead::GetAllyTeamList(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	int count = 0;
	for (int at = 0; at < teamHandler->ActiveAllyTeams(); at++) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, at);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaSyncedRead::GetTeamList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		luaL_error(L, "Incorrect arguments to GetTeamList([allyTeamID])");
	}

	int allyTeamID = -1;
	if (args == 1) {
		allyTeamID = lua_toint(L, 1);
		if (!teamHandler->IsValidAllyTeam(allyTeamID)) {
			return 0;
		}
	}

	lua_newtable(L);
	int count = 0;
	for (int t = 0; t < teamHandler->ActiveTeams(); t++) {
		if (teamHandler->Team(t) == NULL) {
			continue;
		}
		if ((allyTeamID < 0) || (allyTeamID == teamHandler->AllyTeam(t))) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, t);
			lua_rawset(L, -3);
		}
	}

	return 1;
}


int LuaSyncedRead::GetPlayerList(lua_State* L)
{
	int teamID = -1;
	bool active = false;

	if (lua_isnumber(L, 1)) {
		teamID = lua_toint(L, 1);
		if (lua_isboolean(L, 2)) {
			active = lua_toboolean(L, 2);
		}
	}
	else if (lua_isboolean(L, 1)) {
		active = lua_toboolean(L, 1);
		if (lua_isnumber(L, 2)) {
			teamID = lua_toint(L, 2);
		}
	}

	if (teamID >= teamHandler->ActiveTeams()) {
		return 0;
	}

	lua_newtable(L);
	int count = 0;
	for (int p = 0; p < playerHandler->ActivePlayers(); p++) {
		const CPlayer* player = playerHandler->Player(p);
		if (player == NULL) {
			continue;
		}
		if (active && !player->active) {
			continue;
		}
		if ((teamID < 0) || (player->team == teamID)) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, p);
			lua_rawset(L, -3);
		}
	}

	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetTeamInfo(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler->IsValidTeam(teamID)) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	const bool hasAIs = (skirmishAIHandler.GetSkirmishAIsInTeam(teamID).size() > 0);

	lua_pushnumber(L,  team->teamNum);
	lua_pushnumber(L,  team->leader);
	lua_pushboolean(L, team->isDead);
	lua_pushboolean(L, hasAIs);
	lua_pushsstring(L,  team->side);
	lua_pushnumber(L,  teamHandler->AllyTeam(team->teamNum));

	lua_newtable(L);
	const TeamBase::customOpts& popts(team->GetAllValues());
	for (TeamBase::customOpts::const_iterator it = popts.begin(); it != popts.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_pushsstring(L, it->second);
		lua_rawset(L, -3);
	}
	lua_pushnumber(L, team->GetIncomeMultiplier());
	return 8;
}


int LuaSyncedRead::GetTeamResources(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID)) {
		return 0;
	}

	const string type = luaL_checkstring(L, 2);
	if (type == "metal") {
		lua_pushnumber(L, team->metal);
		lua_pushnumber(L, team->metalStorage);
		lua_pushnumber(L, team->prevMetalPull);
		lua_pushnumber(L, team->prevMetalIncome);
		lua_pushnumber(L, team->prevMetalExpense);
		lua_pushnumber(L, team->metalShare);
		lua_pushnumber(L, team->prevMetalSent);
		lua_pushnumber(L, team->prevMetalReceived);
		lua_pushnumber(L, team->prevMetalExcess);
		return 9;
	}
	else if (type == "energy") {
		lua_pushnumber(L, team->energy);
		lua_pushnumber(L, team->energyStorage);
		lua_pushnumber(L, team->prevEnergyPull);
		lua_pushnumber(L, team->prevEnergyIncome);
		lua_pushnumber(L, team->prevEnergyExpense);
		lua_pushnumber(L, team->energyShare);
		lua_pushnumber(L, team->prevEnergySent);
		lua_pushnumber(L, team->prevEnergyReceived);
		lua_pushnumber(L, team->prevEnergyExcess);
		return 9;
	}

	return 0;
}


int LuaSyncedRead::GetTeamUnitStats(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID) && !game->gameOver) {
		return 0;
	}

	const CTeam::Statistics& stats = *team->currentStats;
	lua_pushnumber(L, stats.unitsKilled);
	lua_pushnumber(L, stats.unitsDied);
	lua_pushnumber(L, stats.unitsCaptured);
	lua_pushnumber(L, stats.unitsOutCaptured);
	lua_pushnumber(L, stats.unitsReceived);
	lua_pushnumber(L, stats.unitsSent);

	return 6;
}


int LuaSyncedRead::GetTeamResourceStats(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID) && !game->gameOver) {
		return 0;
	}

	const CTeam::Statistics& stats = *team->currentStats;

	const string type = luaL_checkstring(L, 2);
	if (type == "metal") {
		lua_pushnumber(L, stats.metalUsed);
		lua_pushnumber(L, stats.metalProduced);
		lua_pushnumber(L, stats.metalExcess);
		lua_pushnumber(L, stats.metalReceived);
		lua_pushnumber(L, stats.metalSent);
		return 5;
	}
	else if (type == "energy") {
		lua_pushnumber(L, stats.energyUsed);
		lua_pushnumber(L, stats.energyProduced);
		lua_pushnumber(L, stats.energyExcess);
		lua_pushnumber(L, stats.energyReceived);
		lua_pushnumber(L, stats.energySent);
		return 5;
	}

	return 0;
}


int LuaSyncedRead::GetTeamRulesParams(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC;

	if (IsAlliedTeam(L, team->teamNum) || game->gameOver) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler->AlliedTeams(team->teamNum, CLuaHandle::GetHandleReadTeam(L)) || ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && CLuaHandle::GetHandleFullRead(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}

	const LuaRulesParams::Params&  params    = team->modParams;
	const LuaRulesParams::HashMap& paramsMap = team->modParamsMap;

	return PushRulesParams(L, __FUNCTION__, params, paramsMap, losMask);
}


int LuaSyncedRead::GetTeamRulesParam(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC;

	if (IsAlliedTeam(L, team->teamNum) || game->gameOver) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler->AlliedTeams(team->teamNum, CLuaHandle::GetHandleReadTeam(L)) || ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && CLuaHandle::GetHandleFullRead(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}

	const LuaRulesParams::Params&  params    = team->modParams;
	const LuaRulesParams::HashMap& paramsMap = team->modParamsMap;

	return GetRulesParam(L, __FUNCTION__, 2, params, paramsMap, losMask);
}


int LuaSyncedRead::GetTeamStatsHistory(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID) && !game->gameOver) {
		return 0;
	}

	const int args = lua_gettop(L);
	if (args == 1) {
		lua_pushnumber(L, team->statHistory.size());
		return 1;
	}

	const std::list<CTeam::Statistics>& teamStats = team->statHistory;
	std::list<CTeam::Statistics>::const_iterator it = teamStats.begin();
	const int statCount = teamStats.size();

	int start = 0;
	if ((args >= 2) && lua_isnumber(L, 2)) {
		start = lua_toint(L, 2) - 1;
		start = max(0, min(statCount - 1, start));
	}

	int end = start;
	if ((args >= 3) && lua_isnumber(L, 3)) {
		end = lua_toint(L, 3) - 1;
		end = max(0, min(statCount - 1, end));
	}

	std::advance(it, start);

	lua_newtable(L);
	if (statCount > 0) {
		int count = 0;
		for (int i = start; i <= end; ++i, ++it) {
			const CTeam::Statistics& stats = *it;
			count++;
			lua_pushnumber(L, count);
			lua_newtable(L); {
				if (i+1 == teamStats.size()) {
					//! the `stats.frame` var indicates the frame when a new entry needs to get added,
					//! for the most recent stats entry this lies obviously in the future,
					//! so we just output the current frame here
					HSTR_PUSH_NUMBER(L, "time",             gs->frameNum / GAME_SPEED);
					HSTR_PUSH_NUMBER(L, "frame",            gs->frameNum);
				} else {
					HSTR_PUSH_NUMBER(L, "time",             stats.frame / GAME_SPEED);
					HSTR_PUSH_NUMBER(L, "frame",            stats.frame);
				}
				HSTR_PUSH_NUMBER(L, "metalUsed",        stats.metalUsed);
				HSTR_PUSH_NUMBER(L, "metalProduced",    stats.metalProduced);
				HSTR_PUSH_NUMBER(L, "metalExcess",      stats.metalExcess);
				HSTR_PUSH_NUMBER(L, "metalReceived",    stats.metalReceived);
				HSTR_PUSH_NUMBER(L, "metalSent",        stats.metalSent);

				HSTR_PUSH_NUMBER(L, "energyUsed",       stats.energyUsed);
				HSTR_PUSH_NUMBER(L, "energyProduced",   stats.energyProduced);
				HSTR_PUSH_NUMBER(L, "energyExcess",     stats.energyExcess);
				HSTR_PUSH_NUMBER(L, "energyReceived",   stats.energyReceived);
				HSTR_PUSH_NUMBER(L, "energySent",       stats.energySent);

				HSTR_PUSH_NUMBER(L, "damageDealt",      stats.damageDealt);
				HSTR_PUSH_NUMBER(L, "damageReceived",   stats.damageReceived);

				HSTR_PUSH_NUMBER(L, "unitsProduced",    stats.unitsProduced);
				HSTR_PUSH_NUMBER(L, "unitsDied",        stats.unitsDied);
				HSTR_PUSH_NUMBER(L, "unitsReceived",    stats.unitsReceived);
				HSTR_PUSH_NUMBER(L, "unitsSent",        stats.unitsSent);
				HSTR_PUSH_NUMBER(L, "unitsCaptured",    stats.unitsCaptured);
				HSTR_PUSH_NUMBER(L, "unitsOutCaptured", stats.unitsOutCaptured);
				HSTR_PUSH_NUMBER(L, "unitsKilled",      stats.unitsKilled);
			}
			lua_rawset(L, -3);
		}
	}

	return 1;
}


int LuaSyncedRead::GetTeamLuaAI(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}

	std::string luaAIName = "";
	CSkirmishAIHandler::ids_t saids = skirmishAIHandler.GetSkirmishAIsInTeam(team->teamNum);
	for (CSkirmishAIHandler::ids_t::const_iterator ai = saids.begin(); ai != saids.end(); ++ai) {
		const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(*ai);
		if (aiData->isLuaAI) {
			luaAIName = aiData->shortName;
		}
	}
	lua_pushsstring(L, luaAIName);
	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetPlayerInfo(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler->IsValidPlayer(playerID)) {
		return 0;
	}

	const CPlayer* player = playerHandler->Player(playerID);
	if (player == NULL) {
		return 0;
	}

	lua_pushsstring(L, player->name);
	lua_pushboolean(L, player->active);
	lua_pushboolean(L, player->spectator);
	lua_pushnumber(L, player->team);
	lua_pushnumber(L, teamHandler->AllyTeam(player->team));
	const float pingScale = (GAME_SPEED * gs->speedFactor);
	const float pingSecs = float(player->ping) / pingScale;
	lua_pushnumber(L, pingSecs);
	lua_pushnumber(L, player->cpuUsage);
	lua_pushsstring(L, player->countryCode);
	lua_pushnumber(L, player->rank);

	lua_newtable(L);
	const PlayerBase::customOpts& popts(player->GetAllValues());
	for (PlayerBase::customOpts::const_iterator it = popts.begin(); it != popts.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_pushsstring(L, it->second);
		lua_rawset(L, -3);
	}
	return 10;
}


int LuaSyncedRead::GetPlayerControlledUnit(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler->IsValidPlayer(playerID)) {
		return 0;
	}

	const CPlayer* player = playerHandler->Player(playerID);
	if (player == NULL) {
		return 0;
	}

	const FPSUnitController& con = player->fpsController;
	const CUnit* unit = con.GetControllee();

	if (unit == NULL) {
		return 0;
	}

	if ((CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) ||
	    ((CLuaHandle::GetHandleReadAllyTeam(L) >= 0) && !teamHandler->Ally(unit->allyteam, CLuaHandle::GetHandleReadAllyTeam(L)))) {
		return 0;
	}

	lua_pushnumber(L, unit->id);
	return 1;
}

int LuaSyncedRead::GetAIInfo(lua_State* L)
{
	int numVals = 0;

	const int teamId = luaL_checkint(L, 1);
	if (!teamHandler->IsValidTeam(teamId)) {
		return numVals;
	}

	CSkirmishAIHandler::ids_t saids = skirmishAIHandler.GetSkirmishAIsInTeam(teamId);
	if (saids.empty()) {
		return numVals;
	}
	const size_t skirmishAIId    = *(saids.begin());
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
	const bool isLocal           = skirmishAIHandler.IsLocalSkirmishAI(skirmishAIId);

	// this is synced AI info
	lua_pushnumber(L, skirmishAIId);
	lua_pushsstring(L, aiData->name);
	lua_pushnumber(L, aiData->hostPlayer);
	numVals += 3;

	// no unsynced Skirmish AI info for synchronized scripts
	if (CLuaHandle::GetHandleSynced(L)) {
		HSTR_PUSH(L, "SYNCED_NOSHORTNAME");
		HSTR_PUSH(L, "SYNCED_NOVERSION");
		lua_newtable(L);
	} else if (isLocal) {
		lua_pushsstring(L, aiData->shortName);
		lua_pushsstring(L, aiData->version);

		lua_newtable(L);
		std::map<std::string, std::string>::const_iterator o;
		for (o = aiData->options.begin(); o != aiData->options.end(); ++o) {
			lua_pushsstring(L, o->first);
			lua_pushsstring(L, o->second);
			lua_rawset(L, -3);
		}
	} else {
		HSTR_PUSH(L, "UKNOWN");
		HSTR_PUSH(L, "UKNOWN");
		lua_newtable(L);
	}
	numVals += 3;

	return numVals;
}

int LuaSyncedRead::GetAllyTeamInfo(lua_State* L)
{
	const size_t allyteam = (size_t)luaL_checkint(L, -1);
	if (!teamHandler->ValidAllyTeam(allyteam))
		return 0;

	const AllyTeam& ally = teamHandler->GetAllyTeam(allyteam);
	lua_newtable(L);
	const AllyTeam::customOpts& popts(ally.GetAllValues());
	for (AllyTeam::customOpts::const_iterator it = popts.begin(); it != popts.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_pushsstring(L, it->second);
		lua_rawset(L, -3);
	}
	return 1;
}

int LuaSyncedRead::AreTeamsAllied(lua_State* L)
{
	const int teamId1 = (int)luaL_checkint(L, -1);
	const int teamId2 = (int)luaL_checkint(L, -2);
	if (!teamHandler->IsValidTeam(teamId1) ||
	    !teamHandler->IsValidTeam(teamId2)) {
		return 0;
	}
	lua_pushboolean(L, teamHandler->AlliedTeams(teamId1, teamId2));
	return 1;
}


int LuaSyncedRead::ArePlayersAllied(lua_State* L)
{
	const int player1 = luaL_checkint(L, -1);
	const int player2 = luaL_checkint(L, -2);
	if (!playerHandler->IsValidPlayer(player1) ||
	    !playerHandler->IsValidPlayer(player2)) {
		return 0;
	}
	const CPlayer* p1 = playerHandler->Player(player1);
	const CPlayer* p2 = playerHandler->Player(player2);
	if ((p1 == NULL) || (p2 == NULL)) {
		return 0;
	}
	lua_pushboolean(L, teamHandler->AlliedTeams(p1->team, p2->team));
	return 1;
}


/******************************************************************************/
/******************************************************************************/
//
//  Grouped Unit Queries
//

int LuaSyncedRead::GetAllUnits(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	int count = 0;
	std::list<CUnit*>::const_iterator uit;
	if (CLuaHandle::GetHandleFullRead(L)) {
		lua_createtable(L, unitHandler->activeUnits.size(), 0);
		for (uit = unitHandler->activeUnits.begin(); uit != unitHandler->activeUnits.end(); ++uit) {
			// t[count] = id
			lua_pushnumber(L, (*uit)->id);
			lua_rawseti(L, -2, ++count);
		}
	} else {
		lua_newtable(L);
		for (uit = unitHandler->activeUnits.begin(); uit != unitHandler->activeUnits.end(); ++uit) {
			if (IsUnitVisible(L, *uit)) {
				count++;
				lua_pushnumber(L, count);
				lua_pushnumber(L, (*uit)->id);
				lua_rawset(L, -3);
			}
		}
	}

	return 1;
}


int LuaSyncedRead::GetTeamUnits(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) {
		return 0;
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	const CUnitSet& units = team->units;
	CUnitSet::const_iterator uit;

	// raw push for allies
	if (IsAlliedTeam(L, teamID)) {
		lua_newtable(L);
		int count = 0;
		for (uit = units.begin(); uit != units.end(); ++uit) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, (*uit)->id);
			lua_rawset(L, -3);
		}

		return 1;
	}

	// check visibility for enemies
	lua_newtable(L);
	int count = 0;
	for (uit = units.begin(); uit != units.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitVisible(L, unit)) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, unit->id);
			lua_rawset(L, -3);
		}
	}

	return 1;
}


int LuaSyncedRead::GetTeamUnitsSorted(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) {
		return 0;
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	map<int, vector<CUnit*> > unitDefMap;
	const CUnitSet& units = team->units;
	CUnitSet::const_iterator uit;

	// tally for allies
	if (IsAlliedTeam(L, teamID)) {
		for (uit = units.begin(); uit != units.end(); ++uit) {
			CUnit* unit = *uit;
			unitDefMap[unit->unitDef->id].push_back(unit);
		}
	}
	// tally for enemies
	else {
		// NOTE: (using unitsByDefs[] might be faster late-game)
		for (uit = units.begin(); uit != units.end(); ++uit) {
			CUnit* unit = *uit;
			if (IsUnitVisible(L, unit)) {
				const UnitDef* ud = EffectiveUnitDef(L, unit);
				if (IsUnitTyped(L, unit)) {
					unitDefMap[ud->id].push_back(unit);
				} else {
					unitDefMap[-1].push_back(unit); // unknown
				}
			}
			unitDefMap[unit->unitDef->id].push_back(unit);
		}
	}

	// push the table
	lua_newtable(L);
	map<int, vector<CUnit*> >::const_iterator mit;
	for (mit = unitDefMap.begin(); mit != unitDefMap.end(); ++mit) {
		const int unitDefID = mit->first;
		if (unitDefID < 0) {
			HSTR_PUSH(L, "unknown");
		} else {
			lua_pushnumber(L, unitDefID); // push the UnitDef index
		}
		lua_newtable(L); {
			const vector<CUnit*>& v = mit->second;
			for (int i = 0; i < (int)v.size(); i++) {
				CUnit* unit = v[i];
				lua_pushnumber(L, i + 1);
				lua_pushnumber(L, unit->id);
				lua_rawset(L, -3);
			}
		}
		lua_rawset(L, -3);
	}

	// UnitDef ID keys are not consecutive, so add the "n"
	hs_n.PushNumber(L, unitDefMap.size());
	return 1;
}


int LuaSyncedRead::GetTeamUnitsCounts(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) {
		return 0;
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	// send the raw unitsByDefs counts for allies
	if (IsAlliedTeam(L, teamID)) {
		lua_newtable(L);
		int defCount = 0;

		for (int udID = 0; udID < unitDefHandler->unitDefs.size(); udID++) {
			const int unitCount = unitHandler->unitsByDefs[teamID][udID].size();
			if (unitCount > 0) {
				lua_pushnumber(L, udID);
				lua_pushnumber(L, unitCount);
				lua_rawset(L, -3);
				defCount++;
			}
		}
		// keys are not necessarily consecutive here
		// due to the unitCount check, so add the "n"
		hs_n.PushNumber(L, defCount);
		return 1;
	}

	// tally the counts for enemies
	map<int, int> unitDefCounts;
	map<int, int>::const_iterator mit;

	const CUnitSet& unitSet = team->units;
	CUnitSet::const_iterator uit;

	int unknownCount = 0;
	for (uit = unitSet.begin(); uit != unitSet.end(); ++uit) {
		const CUnit* unit = *uit;

		if (!IsUnitVisible(L, unit)) {
			continue;
		}
		if (!IsUnitTyped(L, unit)) {
			unknownCount++;
		} else {
			const UnitDef* ud = EffectiveUnitDef(L, unit);
			map<int, int>::iterator mit = unitDefCounts.find(ud->id);
			if (mit == unitDefCounts.end()) {
				unitDefCounts[ud->id] = 1;
			} else {
				unitDefCounts[ud->id] = mit->second + 1;
			}
		}
	}

	// push the counts
	lua_newtable(L);
	int defCount = 0;

	for (mit = unitDefCounts.begin(); mit != unitDefCounts.end(); ++mit) {
		lua_pushnumber(L, mit->first);
		lua_pushnumber(L, mit->second);
		lua_rawset(L, -3);
		defCount++;
	}
	if (unknownCount > 0) {
		HSTR_PUSH_NUMBER(L, "unknown", unknownCount);
		defCount++;
	}

	// unitDef->id is used for ordering, so not consecutive
	hs_n.PushNumber(L, defCount);
	return 1;
}


static inline void InsertSearchUnitDefs(const UnitDef* ud, bool allied,
                                        set<int>& defs)
{
	if (ud != NULL) {
		if (allied) {
			defs.insert(ud->id);
		}
		else if (ud->decoyDef == NULL) {
			defs.insert(ud->id);
			map<int, set<int> >::const_iterator dmit;
			dmit = unitDefHandler->decoyMap.find(ud->id);
			if (dmit != unitDefHandler->decoyMap.end()) {
				const set<int>& decoys = dmit->second;
				set<int>::const_iterator dit;
				for (dit = decoys.begin(); dit != decoys.end(); ++dit) {
					defs.insert(*dit);
				}
			}
		}
	}
}


int LuaSyncedRead::GetTeamUnitsByDefs(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) {
		return 0;
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	const bool allied = IsAlliedTeam(L, teamID);

	// parse the unitDefs
	set<int> defs;
	if (lua_isnumber(L, 2)) {
		const int unitDefID = lua_toint(L, 2);
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
		InsertSearchUnitDefs(ud, allied, defs);
	}
	else if (lua_istable(L, 2)) {
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_isnumber(L, -1)) {
				const int unitDefID = lua_toint(L, -1);
				const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
				InsertSearchUnitDefs(ud, allied, defs);
			}
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to GetTeamUnitsByDefs()");
	}

	lua_newtable(L);
	int count = 0;

	set<int>::const_iterator udit;
	for (udit = defs.begin(); udit != defs.end(); ++udit) {
		const CUnitSet& units = unitHandler->unitsByDefs[teamID][*udit];
		CUnitSet::const_iterator uit;
		for (uit = units.begin(); uit != units.end(); ++uit) {
			const CUnit* unit = *uit;
			if (allied || IsUnitTyped(L, unit)) {
				count++;
				lua_pushnumber(L, count);
				lua_pushnumber(L, unit->id);
				lua_rawset(L, -3);
			}
		}
	}

	return 1;
}


int LuaSyncedRead::GetTeamUnitDefCount(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) {
		return 0;
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	// parse the unitDef
	const int unitDefID = luaL_checkint(L, 2);
	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(unitDefID);
	if (unitDef == NULL) {
		luaL_error(L, "Bad unitDefID in GetTeamUnitDefCount()");
	}

	// use the unitsByDefs count for allies
	if (IsAlliedTeam(L, teamID)) {
		lua_pushnumber(L, unitHandler->unitsByDefs[teamID][unitDefID].size());
		return 1;
	}

	// you can never count enemy decoys
	if (unitDef->decoyDef != NULL) {
		lua_pushnumber(L, 0);
		return 1;
	}

	int count = 0;

	// tally the given unitDef units
	const CUnitSet& units = unitHandler->unitsByDefs[teamID][unitDef->id];
	CUnitSet::const_iterator uit;
	for (uit = units.begin(); uit != units.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitTyped(L, unit)) {
			count++;
		}
	}

	// tally the decoy units for the given unitDef
	map<int, set<int> >::const_iterator dmit
		= unitDefHandler->decoyMap.find(unitDef->id);

	if (dmit != unitDefHandler->decoyMap.end()) {
		const set<int>& decoyDefIDs = dmit->second;
		set<int>::const_iterator dit;
		for (dit = decoyDefIDs.begin(); dit != decoyDefIDs.end(); ++dit) {
			const CUnitSet& units = unitHandler->unitsByDefs[teamID][*dit];
			CUnitSet::const_iterator uit;
			for (uit = units.begin(); uit != units.end(); ++uit) {
				const CUnit* unit = *uit;
				if (IsUnitTyped(L, unit)) {
					count++;
				}
			}
		}
	}

	lua_pushnumber(L, count);
	return 1;
}


int LuaSyncedRead::GetTeamUnitCount(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) {
		return 0;
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	// use the raw team count for allies
	if (IsAlliedTeam(L, teamID)) {
		lua_pushnumber(L, team->units.size());
		return 1;
	}

	// loop through the units for enemies
	int count = 0;
	const CUnitSet& units = team->units;
	CUnitSet::const_iterator uit;
	for (uit = units.begin(); uit != units.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitVisible(L, unit)) {
			count++;
		}
	}
	lua_pushnumber(L, count);
	return 1;
}


/******************************************************************************/
/******************************************************************************/
//
//  Spatial Unit Queries
//

// Macro Requirements:
//   L, it, units, and count

#define LOOP_UNIT_CONTAINER(ALLEGIANCE_TEST, CUSTOM_TEST) \
	for (it = units.begin(); it != units.end(); ++it) {     \
		const CUnit* unit = *it;                              \
		ALLEGIANCE_TEST;                                      \
		CUSTOM_TEST;                                          \
		count++;                                              \
		lua_pushnumber(L, count);                             \
		lua_pushnumber(L, unit->id);                          \
		lua_rawset(L, -3);                                    \
	}

// Macro Requirements:
//   unit
//   readTeam   for MY_UNIT_TEST
//   allegiance for SIMPLE_TEAM_TEST and VISIBLE_TEAM_TEST

#define NULL_TEST  ;  // always passes

#define VISIBLE_TEST \
	if (!IsUnitVisible(L, unit))     { continue; }

#define SIMPLE_TEAM_TEST \
	if (unit->team != allegiance) { continue; }

#define VISIBLE_TEAM_TEST \
	if (unit->team != allegiance) { continue; } \
	if (!IsUnitVisible(L, unit))     { continue; }

#define MY_UNIT_TEST \
	if (unit->team != readTeam)   { continue; }

#define ALLY_UNIT_TEST \
	if (unit->allyteam != CLuaHandle::GetHandleReadAllyTeam(L)) { continue; }

#define ENEMY_UNIT_TEST \
	if (unit->allyteam == CLuaHandle::GetHandleReadAllyTeam(L)) { continue; } \
	if (!IsUnitVisible(L, unit))           { continue; }


static int ParseAllegiance(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		return AllUnits;
	}
	const int teamID = lua_toint(L, index);
	if (CLuaHandle::GetHandleFullRead(L) && (teamID < 0)) {
		// MyUnits, AllyUnits, and EnemyUnits do not apply to fullRead
		return AllUnits;
	}
	if (teamID < EnemyUnits) {
		luaL_error(L, "Bad teamID in %s (%i)", caller, teamID);
	} else if (teamID >= teamHandler->ActiveTeams()) {
		luaL_error(L, "Bad teamID in %s (%i)", caller, teamID);
	}
	return teamID;
}


int LuaSyncedRead::GetUnitsInRectangle(lua_State* L)
{
	const float xmin = luaL_checkfloat(L, 1);
	const float zmin = luaL_checkfloat(L, 2);
	const float xmax = luaL_checkfloat(L, 3);
	const float zmax = luaL_checkfloat(L, 4);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	const int allegiance = ParseAllegiance(L, __FUNCTION__, 5);

#define RECTANGLE_TEST ; // no test, GetUnitsExact is sufficient

	vector<CUnit*>::const_iterator it;
	const vector<CUnit*> &units = quadField->GetUnitsExact(mins, maxs);

	lua_newtable(L);
	int count = 0;

	if (allegiance >= 0) {
		if (IsAlliedTeam(L, allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, RECTANGLE_TEST);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, RECTANGLE_TEST);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		LOOP_UNIT_CONTAINER(MY_UNIT_TEST, RECTANGLE_TEST);
	}
	else if (allegiance == AllyUnits) {
		LOOP_UNIT_CONTAINER(ALLY_UNIT_TEST, RECTANGLE_TEST);
	}
	else if (allegiance == EnemyUnits) {
		LOOP_UNIT_CONTAINER(ENEMY_UNIT_TEST, RECTANGLE_TEST);
	}
	else { // AllUnits
		LOOP_UNIT_CONTAINER(VISIBLE_TEST, RECTANGLE_TEST);
	}

	return 1;
}


int LuaSyncedRead::GetUnitsInBox(lua_State* L)
{
	const float xmin = luaL_checkfloat(L, 1);
	const float ymin = luaL_checkfloat(L, 2);
	const float zmin = luaL_checkfloat(L, 3);
	const float xmax = luaL_checkfloat(L, 4);
	const float ymax = luaL_checkfloat(L, 5);
	const float zmax = luaL_checkfloat(L, 6);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	const int allegiance = ParseAllegiance(L, __FUNCTION__, 7);

#define BOX_TEST                  \
	const float y = unit->midPos.y; \
	if ((y < ymin) || (y > ymax)) { \
		continue;                     \
	}

	vector<CUnit*>::const_iterator it;
	const vector<CUnit*> &units = quadField->GetUnitsExact(mins, maxs);

	lua_newtable(L);
	int count = 0;

	if (allegiance >= 0) {
		if (IsAlliedTeam(L, allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, BOX_TEST);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, BOX_TEST);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		LOOP_UNIT_CONTAINER(MY_UNIT_TEST, BOX_TEST);
	}
	else if (allegiance == AllyUnits) {
		LOOP_UNIT_CONTAINER(ALLY_UNIT_TEST, BOX_TEST);
	}
	else if (allegiance == EnemyUnits) {
		LOOP_UNIT_CONTAINER(ENEMY_UNIT_TEST, BOX_TEST);
	}
	else { // AllUnits
		LOOP_UNIT_CONTAINER(VISIBLE_TEST, BOX_TEST);
	}

	return 1;
}


int LuaSyncedRead::GetUnitsInCylinder(lua_State* L)
{
	const float x      = luaL_checkfloat(L, 1);
	const float z      = luaL_checkfloat(L, 2);
	const float radius = luaL_checkfloat(L, 3);
	const float radSqr = (radius * radius);

	const float3 mins(x - radius, 0.0f, z - radius);
	const float3 maxs(x + radius, 0.0f, z + radius);

	const int allegiance = ParseAllegiance(L, __FUNCTION__, 4);

#define CYLINDER_TEST                         \
	const float3& p = unit->midPos;             \
	const float dx = (p.x - x);                 \
	const float dz = (p.z - z);                 \
	const float dist = ((dx * dx) + (dz * dz)); \
	if (dist > radSqr) {                        \
		continue;                                 \
	}                                           \

	vector<CUnit*>::const_iterator it;
	const vector<CUnit*> &units = quadField->GetUnitsExact(mins, maxs);

	lua_newtable(L);
	int count = 0;

	if (allegiance >= 0) {
		if (IsAlliedTeam(L, allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, CYLINDER_TEST);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, CYLINDER_TEST);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		LOOP_UNIT_CONTAINER(MY_UNIT_TEST, CYLINDER_TEST);
	}
	else if (allegiance == AllyUnits) {
		LOOP_UNIT_CONTAINER(ALLY_UNIT_TEST, CYLINDER_TEST);
	}
	else if (allegiance == EnemyUnits) {
		LOOP_UNIT_CONTAINER(ENEMY_UNIT_TEST, CYLINDER_TEST);
	}
	else { // AllUnits
		LOOP_UNIT_CONTAINER(VISIBLE_TEST, CYLINDER_TEST);
	}

	return 1;
}


int LuaSyncedRead::GetUnitsInSphere(lua_State* L)
{
	const float x      = luaL_checkfloat(L, 1);
	const float y      = luaL_checkfloat(L, 2);
	const float z      = luaL_checkfloat(L, 3);
	const float radius = luaL_checkfloat(L, 4);
	const float radSqr = (radius * radius);

	const float3 pos(x, y, z);
	const float3 mins(x - radius, 0.0f, z - radius);
	const float3 maxs(x + radius, 0.0f, z + radius);

	const int allegiance = ParseAllegiance(L, __FUNCTION__, 5);

#define SPHERE_TEST                           \
	const float3& p = unit->midPos;             \
	const float dx = (p.x - x);                 \
	const float dy = (p.y - y);                 \
	const float dz = (p.z - z);                 \
	const float dist =                          \
		((dx * dx) + (dy * dy) + (dz * dz));      \
	if (dist > radSqr) {                        \
		continue;                                 \
	}                                           \

	vector<CUnit*>::const_iterator it;
	const vector<CUnit*> &units = quadField->GetUnitsExact(mins, maxs);

	lua_newtable(L);
	int count = 0;

	if (allegiance >= 0) {
		if (IsAlliedTeam(L, allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, SPHERE_TEST);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, SPHERE_TEST);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		LOOP_UNIT_CONTAINER(MY_UNIT_TEST, SPHERE_TEST);
	}
	else if (allegiance == AllyUnits) {
		LOOP_UNIT_CONTAINER(ALLY_UNIT_TEST, SPHERE_TEST);
	}
	else if (allegiance == EnemyUnits) {
		LOOP_UNIT_CONTAINER(ENEMY_UNIT_TEST, SPHERE_TEST);
	}
	else { // AllUnits
		LOOP_UNIT_CONTAINER(VISIBLE_TEST, SPHERE_TEST);
	}

	return 1;
}


struct Plane {
	float x, y, z, d;  // ax + by + cz + d = 0
};


static inline bool UnitInPlanes(const CUnit* unit, const vector<Plane>& planes)
{
	const float3& pos = unit->midPos;
	for (int i = 0; i < (int)planes.size(); i++) {
		const Plane& p = planes[i];
		const float dist = (pos.x * p.x) + (pos.y * p.y) + (pos.z * p.z) + p.d;
		if ((dist - unit->radius) > 0.0f) {
			return false; // outside
		}
	}
	return true;
}


int LuaSyncedRead::GetUnitsInPlanes(lua_State* L)
{
	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetUnitsInPlanes()");
	}

	// parse the planes
	vector<Plane> planes;
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_istable(L, -1)) {
			float values[4];
			const int v = ParseFloatArray(L, values, 4);
			if (v == 4) {
				Plane plane = { values[0], values[1], values[2], values[3] };
				planes.push_back(plane);
			}
		}
	}

	int startTeam, endTeam;

	const int allegiance = ParseAllegiance(L, __FUNCTION__, 2);
	if (allegiance >= 0) {
		startTeam = allegiance;
		endTeam = allegiance;
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		startTeam = readTeam;
		endTeam = readTeam;
	}
	else {
		startTeam = 0;
		endTeam = teamHandler->ActiveTeams() - 1;
	}

#define PLANES_TEST                  \
	if (!UnitInPlanes(unit, planes)) { \
		continue;                        \
	}

	lua_newtable(L);
	int count = 0;

	const int readTeam = CLuaHandle::GetHandleReadTeam(L);

	for (int team = startTeam; team <= endTeam; team++) {
		const CUnitSet& units = teamHandler->Team(team)->units;
		CUnitSet::const_iterator it;

		if (allegiance >= 0) {
			if (allegiance == team) {
				if (IsAlliedTeam(L, allegiance)) {
					LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST);
				} else {
					LOOP_UNIT_CONTAINER(VISIBLE_TEST, PLANES_TEST);
				}
			}
		}
		else if (allegiance == MyUnits) {
			if (readTeam == team) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST);
			}
		}
		else if (allegiance == AllyUnits) {
			if (CLuaHandle::GetHandleReadAllyTeam(L) == teamHandler->AllyTeam(team)) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST);
			}
		}
		else if (allegiance == EnemyUnits) {
			if (CLuaHandle::GetHandleReadAllyTeam(L) != teamHandler->AllyTeam(team)) {
				LOOP_UNIT_CONTAINER(VISIBLE_TEST, PLANES_TEST);
			}
		}
		else { // AllUnits
			if (IsAlliedTeam(L, team)) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST);
			} else {
				LOOP_UNIT_CONTAINER(VISIBLE_TEST, PLANES_TEST);
			}
		}
	}

	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetUnitNearestAlly(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float range = luaL_optnumber(L, 2, 1.0e9f);
	const CUnit* target =
		CGameHelper::GetClosestFriendlyUnit(unit, unit->pos, range, unit->allyteam);

	if (target != NULL) {
		lua_pushnumber(L, target->id);
		return 1;
	}
	return 0;
}


int LuaSyncedRead::GetUnitNearestEnemy(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float range = luaL_optnumber(L, 2, 1.0e9f);
	const bool useLos =
		!CLuaHandle::GetHandleFullRead(L) || !lua_isboolean(L, 3) || lua_toboolean(L, 3);
	const CUnit* target = NULL;

	if (useLos) {
		target = CGameHelper::GetClosestEnemyUnit(unit, unit->pos, range, unit->allyteam);
	} else {
		target = CGameHelper::GetClosestEnemyUnitNoLosTest(unit, unit->pos, range, unit->allyteam, false, true);
	}

	if (target != NULL) {
		lua_pushnumber(L, target->id);
		return 1;
	}
	return 0;
}


/******************************************************************************/

inline void ProcessFeatures(lua_State* L, const vector<CFeature*>& features) {
	const unsigned int featureCount = features.size();
	unsigned int arrayIndex = 1;

	lua_createtable(L, featureCount, 0);

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		if (CLuaHandle::GetHandleFullRead(L)) {
			for (unsigned int i = 0; i < featureCount; i++) {
				const CFeature* feature = features[i];

				lua_pushnumber(L, arrayIndex++);
				lua_pushnumber(L, feature->id);
				lua_rawset(L, -3);
			}
		}
	} else {
		for (unsigned int i = 0; i < featureCount; i++) {
			const CFeature* feature = features[i];

			if (!IsFeatureVisible(L, feature)) {
				continue;
			}

			lua_pushnumber(L, arrayIndex++);
			lua_pushnumber(L, feature->id);
			lua_rawset(L, -3);
		}
	}
}

int LuaSyncedRead::GetFeaturesInRectangle(lua_State* L)
{
	const float xmin = luaL_checkfloat(L, 1);
	const float zmin = luaL_checkfloat(L, 2);
	const float xmax = luaL_checkfloat(L, 3);
	const float zmax = luaL_checkfloat(L, 4);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	const vector<CFeature*>& rectFeatures = quadField->GetFeaturesExact(mins, maxs);
	ProcessFeatures(L, rectFeatures);
	return 1;
}

int LuaSyncedRead::GetFeaturesInSphere(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
	const float rad = luaL_checkfloat(L, 4);

	const float3 pos(x, y, z);

	const vector<CFeature*>& sphFeatures = quadField->GetFeaturesExact(pos, rad, true);
	ProcessFeatures(L, sphFeatures);
	return 1;
}

int LuaSyncedRead::GetFeaturesInCylinder(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	const float rad = luaL_checkfloat(L, 3);

	const float3 pos(x, 0, z);

	const vector<CFeature*>& cylFeatures = quadField->GetFeaturesExact(pos, rad, false);
	ProcessFeatures(L, cylFeatures);
	return 1;
}

int LuaSyncedRead::GetProjectilesInRectangle(lua_State* L)
{
	const float xmin = luaL_checkfloat(L, 1);
	const float zmin = luaL_checkfloat(L, 2);
	const float xmax = luaL_checkfloat(L, 3);
	const float zmax = luaL_checkfloat(L, 4);

	const bool excludeWeaponProjectiles = lua_isboolean(L, 5)? lua_toboolean(L, 5): false;
	const bool excludePieceProjectiles = lua_isboolean(L, 6)? lua_toboolean(L, 6): false;

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	bool renderAccess = GML::SimEnabled() && !Threading::IsSimThread();

	const vector<CProjectile*>& rectProjectiles = quadField->GetProjectilesExact(mins, maxs);
	const unsigned int rectProjectileCount = rectProjectiles.size();
	unsigned int arrayIndex = 1;

	lua_createtable(L, rectProjectileCount, 0);

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		if (CLuaHandle::GetHandleFullRead(L)) {
			for (unsigned int i = 0; i < rectProjectileCount; i++) {
				const CProjectile* pro = rectProjectiles[i];
				if (renderAccess && !projectileHandler->RenderAccess(pro))
					continue;

				if (pro->weapon && excludeWeaponProjectiles) { continue; }
				if (pro->piece && excludePieceProjectiles) { continue; }

				lua_pushnumber(L, arrayIndex++);
				lua_pushnumber(L, pro->id);
				lua_rawset(L, -3);
			}
		}
	} else {
		for (unsigned int i = 0; i < rectProjectileCount; i++) {
			const CProjectile* pro = rectProjectiles[i];
			if (renderAccess && !projectileHandler->RenderAccess(pro))
				continue;
			const CUnit* unit = pro->owner();
			const ProjectileMapValPair proPair(const_cast<CProjectile*>(pro), ((unit != NULL)? unit->allyteam: CLuaHandle::GetHandleReadAllyTeam(L)));

			if (pro->weapon && excludeWeaponProjectiles) { continue; }
			if (pro->piece && excludePieceProjectiles) { continue; }

			if (!IsProjectileVisible(L, proPair)) {
				continue;
			}

			lua_pushnumber(L, arrayIndex++);
			lua_pushnumber(L, pro->id);
			lua_rawset(L, -3);
		}
	}

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::ValidUnitID(lua_State* L)
{
	CUnit* unit = ParseUnit(L, NULL, 1); // note the NULL
	lua_pushboolean(L, unit != NULL);
	return 1;
}


int LuaSyncedRead::GetUnitStates(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	lua_createtable(L, 0, 9);
	HSTR_PUSH_NUMBER(L, "firestate",  unit->fireState);
	HSTR_PUSH_NUMBER(L, "movestate",  unit->moveState);
	HSTR_PUSH_BOOL  (L, "repeat",     unit->commandAI->repeatOrders);
	HSTR_PUSH_BOOL  (L, "cloak",      unit->wantCloak);
	HSTR_PUSH_BOOL  (L, "active",     unit->activated);
	HSTR_PUSH_BOOL  (L, "trajectory", unit->useHighTrajectory);

	const AMoveType* mt = unit->moveType;
	if (mt) {
		const CHoverAirMoveType* hAMT = dynamic_cast<const CHoverAirMoveType*>(mt);
		if (hAMT) {
			HSTR_PUSH_BOOL  (L, "autoland",        hAMT->autoLand);
			HSTR_PUSH_NUMBER(L, "autorepairlevel", hAMT->GetRepairBelowHealth());
		} else {
			const CStrafeAirMoveType* sAMT = dynamic_cast<const CStrafeAirMoveType*>(mt);
			if (sAMT) {
				HSTR_PUSH_BOOL  (L, "autoland",        sAMT->autoLand);
				HSTR_PUSH_BOOL  (L, "loopbackattack",  sAMT->loopbackAttack);
				HSTR_PUSH_NUMBER(L, "autorepairlevel", sAMT->GetRepairBelowHealth());
			}
		}
	}

	return 1;
}


int LuaSyncedRead::GetUnitArmored(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->armoredState);
	lua_pushnumber(L, unit->armoredMultiple);
	return 2;
}


int LuaSyncedRead::GetUnitIsActive(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->activated);
	return 1;
}


int LuaSyncedRead::GetUnitIsCloaked(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->isCloaked);
	return 1;
}


int LuaSyncedRead::GetUnitSelfDTime(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->selfDCountdown);
	return 1;
}


int LuaSyncedRead::GetUnitStockpile(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (unit->stockpileWeapon == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->stockpileWeapon->numStockpiled);
	lua_pushnumber(L, unit->stockpileWeapon->numStockpileQued);
	lua_pushnumber(L, unit->stockpileWeapon->buildPercent);
	return 3;
}


int LuaSyncedRead::GetUnitSensorRadius(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const string key = luaL_checkstring(L, 2);

	const int radarDiv = radarhandler->radarDiv;

	if (key == "los") {
		lua_pushnumber(L, unit->losRadius * loshandler->losDiv);
	} else if (key == "airLos") {
		lua_pushnumber(L, unit->airLosRadius * loshandler->airDiv);
	} else if (key == "radar") {
		lua_pushnumber(L, unit->radarRadius * radarDiv);
	} else if (key == "sonar") {
		lua_pushnumber(L, unit->sonarRadius * radarDiv);
	} else if (key == "seismic") {
		lua_pushnumber(L, unit->seismicRadius * radarDiv);
	} else if (key == "radarJammer") {
		lua_pushnumber(L, unit->jammerRadius * radarDiv);
	} else if (key == "sonarJammer") {
		lua_pushnumber(L, unit->sonarJamRadius * radarDiv);
	} else {
		return 0; // unknown sensor type
	}

	return 1;
}


int LuaSyncedRead::GetUnitTooltip(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	string tooltip;

	const CTeam* unitTeam = NULL;
	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = IsAllyUnit(L, unit) ? NULL : unitDef->decoyDef;
	const UnitDef* effectiveDef = EffectiveUnitDef(L, unit);

	if (effectiveDef->showPlayerName) {
		if (teamHandler->IsValidTeam(unit->team)) {
			unitTeam = teamHandler->Team(unit->team);
		}

		if (unitTeam != NULL && unitTeam->leader != -1) {
			tooltip = playerHandler->Player(unitTeam->leader)->name;
			CSkirmishAIHandler::ids_t teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(unit->team);

			if (!teamAIs.empty()) {
				tooltip = std::string("AI@") + tooltip;
			}
		}
	} else {
		if (!decoyDef) {
			tooltip = unit->tooltip;
		} else {
			tooltip = decoyDef->humanName + " - " + decoyDef->tooltip;
		}
	}
	lua_pushsstring(L, tooltip);
	return 1;
}


int LuaSyncedRead::GetUnitDefID(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (IsAllyUnit(L, unit)) {
		lua_pushnumber(L, unit->unitDef->id);
	}
	else {
		if (!IsUnitTyped(L, unit)) {
			return 0;
		}
		lua_pushnumber(L, EffectiveUnitDef(L, unit)->id);
	}
	return 1;
}


int LuaSyncedRead::GetUnitTeam(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->team);
	return 1;
}


int LuaSyncedRead::GetUnitAllyTeam(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->allyteam);
	return 1;
}


int LuaSyncedRead::GetUnitNeutral(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->IsNeutral());
	return 1;
}


int LuaSyncedRead::GetUnitHealth(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const UnitDef* ud = unit->unitDef;
	const bool enemyUnit = IsEnemyUnit(L, unit);

	if (ud->hideDamage && enemyUnit) {
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	} else if (!enemyUnit || (ud->decoyDef == NULL)) {
		lua_pushnumber(L, unit->health);
		lua_pushnumber(L, unit->maxHealth);
		lua_pushnumber(L, unit->paralyzeDamage);
	} else {
		const float scale = (ud->decoyDef->health / ud->health);
		lua_pushnumber(L, scale * unit->health);
		lua_pushnumber(L, scale * unit->maxHealth);
		lua_pushnumber(L, scale * unit->paralyzeDamage);
	}
	lua_pushnumber(L, unit->captureProgress);
	lua_pushnumber(L, unit->buildProgress);
	return 5;
}


int LuaSyncedRead::GetUnitIsDead(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->isDead);
	return 1;
}


int LuaSyncedRead::GetUnitIsStunned(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->IsStunned() || unit->beingBuilt);
	lua_pushboolean(L, unit->IsStunned());
	lua_pushboolean(L, unit->beingBuilt);
	return 3;
}


int LuaSyncedRead::GetUnitResources(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->metalMake);
	lua_pushnumber(L, unit->metalUse);
	lua_pushnumber(L, unit->energyMake);
	lua_pushnumber(L, unit->energyUse);
	return 4;
}


int LuaSyncedRead::GetUnitMetalExtraction(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (!unit->unitDef->extractsMetal) {
		return 0;
	}
	lua_pushnumber(L, unit->metalExtract);
	return 1;
}


int LuaSyncedRead::GetUnitExperience(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->experience);
	lua_pushnumber(L, unit->limExperience);
	return 2;
}


int LuaSyncedRead::GetUnitHeight(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->height);
	return 1;
}


int LuaSyncedRead::GetUnitRadius(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->radius);
	return 1;
}


int LuaSyncedRead::GetUnitPosition(lua_State* L)
{
	int argc = 0;
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL) {
		return argc;
	}

	float3 err = ZeroVector;

	if (!IsAllyUnit(L, unit)) {
		err += CGameHelper::GetUnitErrorPos(unit, CLuaHandle::GetHandleReadAllyTeam(L));
		err -= unit->midPos;
	}

	// base-position
	lua_pushnumber(L, unit->pos.x + err.x);
	lua_pushnumber(L, unit->pos.y + err.y);
	lua_pushnumber(L, unit->pos.z + err.z);
	argc += 3;

	if (lua_isboolean(L, 2) && lua_toboolean(L, 2)) {
		lua_pushnumber(L, unit->midPos.x + err.x);
		lua_pushnumber(L, unit->midPos.y + err.y);
		lua_pushnumber(L, unit->midPos.z + err.z);
		argc += 3;
	}
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		lua_pushnumber(L, unit->aimPos.x + err.x);
		lua_pushnumber(L, unit->aimPos.y + err.y);
		lua_pushnumber(L, unit->aimPos.z + err.z);
		argc += 3;
	}

	return argc;
}


int LuaSyncedRead::GetUnitBasePosition(lua_State* L)
{
	return (GetUnitPosition(L));
}

int LuaSyncedRead::GetUnitVectors(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

#define PACK_VECTOR(n) \
	lua_createtable(L, 3, 0);            \
	lua_pushnumber(L, unit-> n .x); lua_rawseti(L, -2, 1); \
	lua_pushnumber(L, unit-> n .y); lua_rawseti(L, -2, 2); \
	lua_pushnumber(L, unit-> n .z); lua_rawseti(L, -2, 3)

	PACK_VECTOR(frontdir);
	PACK_VECTOR(updir);
	PACK_VECTOR(rightdir);

	return 3;
}


int LuaSyncedRead::GetUnitDirection(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->frontdir.x);
	lua_pushnumber(L, unit->frontdir.y);
	lua_pushnumber(L, unit->frontdir.z);
	return 3;
}


int LuaSyncedRead::GetUnitHeading(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->heading);
	return 1;
}


int LuaSyncedRead::GetUnitVelocity(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->speed.x);
	lua_pushnumber(L, unit->speed.y);
	lua_pushnumber(L, unit->speed.z);

	if (lua_isboolean(L, 2) && lua_toboolean(L, 2)) {
		lua_pushnumber(L, unit->speed.Length());
		return 4;
	}

	return 3;
}


int LuaSyncedRead::GetUnitBuildFacing(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->buildFacing);
	return 1;
}


int LuaSyncedRead::GetUnitIsBuilding(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const CBuilder* builder = dynamic_cast<CBuilder*>(unit);
	if (builder && builder->curBuild) {
		lua_pushnumber(L, builder->curBuild->id);
		return 1;
	}
	const CFactory* factory = dynamic_cast<CFactory*>(unit);
	if (factory && factory->curBuild) {
		lua_pushnumber(L, factory->curBuild->id);
		return 1;
	}
	return 0;
}


int LuaSyncedRead::GetUnitCurrentBuildPower(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const NanoPieceCache* pieceCache = NULL;

	{
		const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

		if (builder != NULL) {
			pieceCache = &builder->GetNanoPieceCache();
		}

		const CFactory* factory = dynamic_cast<const CFactory*>(unit);

		if (factory != NULL) {
			pieceCache = &factory->GetNanoPieceCache();
		}
	}

	if (pieceCache == NULL)
		return 0;

	lua_pushnumber(L, pieceCache->GetBuildPower());
	return 1;
}


int LuaSyncedRead::GetUnitNanoPieces(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const NanoPieceCache* pieceCache = NULL;
	const std::vector<int>* nanoPieces = NULL;

	{
		const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

		if (builder != NULL) {
			pieceCache = &builder->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}

		const CFactory* factory = dynamic_cast<const CFactory*>(unit);

		if (factory != NULL) {
			pieceCache = &factory->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}
	}

	if (nanoPieces == NULL || nanoPieces->empty())
		return 0;

	lua_createtable(L, nanoPieces->size(), 0);

	for (size_t p = 0; p < nanoPieces->size(); p++) {
		const int modelPieceNum = (*nanoPieces)[p];

		lua_pushnumber(L, p + 1);
		lua_pushnumber(L, modelPieceNum + 1); //lua 1-indexed, c++ 0-indexed
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaSyncedRead::GetUnitTransporter(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (unit->transporter == 0) {
		return 0;
	}
	lua_pushnumber(L, unit->transporter->id);
	return 1;
}


int LuaSyncedRead::GetUnitIsTransporting(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const CTransportUnit* tu = dynamic_cast<CTransportUnit*>(unit);
	if (tu == NULL) {
		return 0;
	}
	lua_newtable(L);
	std::list<CTransportUnit::TransportedUnit>::const_iterator it;
	int count = 0;
	for (it = tu->GetTransportedUnits().begin(); it != tu->GetTransportedUnits().end(); ++it) {
		const CUnit* carried = it->unit;
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, carried->id);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaSyncedRead::GetUnitShieldState(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	CPlasmaRepulser* shield = NULL;
	const int idx = luaL_optint(L, 2, -1);

	if (idx < 0 || idx >= unit->weapons.size()) {
		shield = static_cast<CPlasmaRepulser*>(unit->shieldWeapon);
	} else {
		shield = dynamic_cast<CPlasmaRepulser*>(unit->weapons[idx]);
	}

	if (shield == NULL) {
		return 0;
	}

	lua_pushnumber(L, shield->isEnabled);
	lua_pushnumber(L, shield->curPower);
	return 2;
}


int LuaSyncedRead::GetUnitFlanking(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (lua_israwstring(L, 2)) {
		const string key = lua_tostring(L, 2);
		if (key == "mode") {
			lua_pushnumber(L, unit->flankingBonusMode);
			return 1;
		}
		else if (key == "dir") {
			lua_pushnumber(L, unit->flankingBonusDir.x);
			lua_pushnumber(L, unit->flankingBonusDir.y);
			lua_pushnumber(L, unit->flankingBonusDir.z);
			return 3;
		}
		else if (key == "moveFactor") {
			lua_pushnumber(L, unit->flankingBonusMobilityAdd);
			return 1;
		}
		else if (key == "minDamage") {
			lua_pushnumber(L, unit->flankingBonusAvgDamage -
												unit->flankingBonusDifDamage);
			return 1;
		}
		else if (key == "maxDamage") {
			lua_pushnumber(L, unit->flankingBonusAvgDamage +
												unit->flankingBonusDifDamage);
			return 1;
		}
	}
	else if (lua_isnoneornil(L, 2)) {
		lua_pushnumber(L, unit->flankingBonusMode);
		lua_pushnumber(L, unit->flankingBonusMobilityAdd);
		lua_pushnumber(L, unit->flankingBonusAvgDamage - // min
		                  unit->flankingBonusDifDamage);
		lua_pushnumber(L, unit->flankingBonusAvgDamage + // max
		                  unit->flankingBonusDifDamage);
		lua_pushnumber(L, unit->flankingBonusDir.x);
		lua_pushnumber(L, unit->flankingBonusDir.y);
		lua_pushnumber(L, unit->flankingBonusDir.z);
		return 7;
	}

	return 0;
}


int LuaSyncedRead::GetUnitWeaponState(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int weaponNum = luaL_checkint(L, 2) -1;
	if ((weaponNum < 0) || ((size_t)weaponNum >= unit->weapons.size())) {
		return 0;
	}
	const CWeapon* weapon = unit->weapons[weaponNum];
	const string key = luaL_optstring(L, 3, "");
	if (key.empty()) { // backwards compatible
		lua_pushboolean(L, weapon->angleGood);
		lua_pushboolean(L, weapon->reloadStatus <= gs->frameNum);
		lua_pushnumber(L,  weapon->reloadStatus);
		lua_pushnumber(L,  weapon->salvoLeft);
		lua_pushnumber(L,  weapon->numStockpiled);
		return 5;
	}
	else if (key == "reloadState" || key == "reloadFrame") {
		lua_pushnumber(L, weapon->reloadStatus);
	}
	else if (key == "reloadTime") {
		lua_pushnumber(L, (weapon->reloadTime / unit->reloadSpeed) / GAME_SPEED);
	}
	else if (key == "accuracy") {
		lua_pushnumber(L, weapon->accuracy);
	}
	else if (key == "sprayAngle") {
		lua_pushnumber(L, weapon->sprayAngle);
	}
	else if (key == "range") {
		lua_pushnumber(L, weapon->range);
	}
	else if (key == "projectileSpeed") {
		lua_pushnumber(L, weapon->projectileSpeed);
	}
	else if (key == "burst") {
		lua_pushnumber(L, weapon->salvoSize);
	}
	else if (key == "burstRate") {
		lua_pushnumber(L, weapon->salvoDelay / GAME_SPEED);
	}
	else if (key == "projectiles") {
		lua_pushnumber(L, weapon->projectilesPerShot);
	}
	else {
		return 0;
	}
	return 1;
}


int LuaSyncedRead::GetUnitWeaponVectors(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int weaponNum = luaL_checkint(L, 2) -1;
	if ((weaponNum < 0) || ((size_t)weaponNum >= unit->weapons.size())) {
		return 0;
	}

	const CWeapon* weapon = unit->weapons[weaponNum];
	const float3& pos = weapon->weaponMuzzlePos;
	const float3* dir = &weapon->wantedDir;

	const string& type = weapon->weaponDef->type;
	if ((type == "TorpedoLauncher") ||
	    (type == "MissileLauncher") ||
	    (type == "StarburstLauncher")) {
		dir = &weapon->weaponDir;
	}

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);

	lua_pushnumber(L, dir->x);
	lua_pushnumber(L, dir->y);
	lua_pushnumber(L, dir->z);

	return 6;
}


int LuaSyncedRead::GetUnitWeaponTryTarget(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int weaponNum = luaL_checkint(L, 2) -1;
	if ((weaponNum < 0) || ((size_t)weaponNum >= unit->weapons.size())) {
		return 0;
	}

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = NULL;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		enemy = ParseUnit(L, __FUNCTION__, 3);
	}

	lua_pushboolean(L, weapon->TryTarget(pos, true, enemy));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponTestTarget(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int weaponNum = luaL_checkint(L, 2) -1;
	if ((weaponNum < 0) || ((size_t)weaponNum >= unit->weapons.size())) {
		return 0;
	}

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = NULL;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		enemy = ParseUnit(L, __FUNCTION__, 3);
	}

	lua_pushboolean(L, weapon->TestTarget(pos, true, enemy));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponTestRange(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int weaponNum = luaL_checkint(L, 2) -1;
	if ((weaponNum < 0) || ((size_t)weaponNum >= unit->weapons.size())) {
		return 0;
	}

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = NULL;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		enemy = ParseUnit(L, __FUNCTION__, 3);
	}

	lua_pushboolean(L, weapon->TestRange(pos, true, enemy));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponHaveFreeLineOfFire(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int weaponNum = luaL_checkint(L, 2) -1;
	if ((weaponNum < 0) || ((size_t)weaponNum >= unit->weapons.size())) {
		return 0;
	}

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = NULL;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		enemy = ParseUnit(L, __FUNCTION__, 3);
	}

	lua_pushboolean(L, weapon->HaveFreeLineOfFire(pos, true, enemy));
	return 1;
}



int LuaSyncedRead::GetUnitTravel(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->travel);
	lua_pushnumber(L, unit->travelPeriod);
	return 2;
}


int LuaSyncedRead::GetUnitFuel(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->currentFuel);
	return 1;
}


int LuaSyncedRead::GetUnitEstimatedPath(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const CGroundMoveType* gmt = dynamic_cast<const CGroundMoveType*>(unit->moveType);

	if (gmt == NULL) {
		return 0;
	}

	return (LuaPathFinder::PushPathNodes(L, gmt->pathId));
}


int LuaSyncedRead::GetUnitLastAttacker(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if ((unit->lastAttacker == NULL) ||
	    !IsUnitVisible(L, unit->lastAttacker)) {
		return 0;
	}
	lua_pushnumber(L, unit->lastAttacker->id);
	return 1;
}

int LuaSyncedRead::GetUnitLastAttackedPiece(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1); // ?
	if (unit == NULL) {
		return 0;
	}
	if (unit->lastAttackedPiece == NULL) {
		return 0;
	}

	const LocalModelPiece* lmp = unit->lastAttackedPiece;
	const S3DModelPiece* omp = lmp->original;

	lua_pushsstring(L, omp->name);
	lua_pushnumber(L, unit->lastAttackedPieceFrame);
	return 2;
}


int LuaSyncedRead::GetUnitCollisionVolumeData(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	return (PushCollisionVolumeData(L, unit->collisionVolume));
}

int LuaSyncedRead::GetUnitPieceCollisionVolumeData(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const LocalModel* lm = unit->localModel;
	const int pieceIndex = luaL_checkint(L, 2);

	if (pieceIndex < 0 || pieceIndex >= lm->pieces.size()) {
		return 0;
	}

	const LocalModelPiece* lmp = lm->pieces[pieceIndex];
	const CollisionVolume* vol = lmp->GetCollisionVolume();

	return (PushCollisionVolumeData(L, vol));
}


int LuaSyncedRead::GetUnitLosState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	int allyTeam = CLuaHandle::GetHandleReadAllyTeam(L);
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		if (!CLuaHandle::GetHandleFullRead(L)) {
			return 0;
		}
		allyTeam = luaL_checkint(L, 2);
	}
	if (!teamHandler->IsValidAllyTeam(allyTeam)) {
		return 0;
	}
	const unsigned short losStatus = unit->losStatus[allyTeam];

	if (CLuaHandle::GetHandleFullRead(L) && lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		lua_pushnumber(L, losStatus); // return a numeric value
		return 1;
	}

	lua_createtable(L, 0, 3);
	if (losStatus & LOS_INLOS) {
		HSTR_PUSH_BOOL(L, "los", true);
	}
	if (losStatus & LOS_INRADAR) {
		HSTR_PUSH_BOOL(L, "radar", true);
	}
	const int prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
	if ((losStatus & LOS_INLOS) ||
	    ((losStatus & prevMask) == prevMask)) {
		HSTR_PUSH_BOOL(L, "typed", true);
	}
	return 1;
}


int LuaSyncedRead::GetUnitSeparation(lua_State* L)
{
	CUnit* unit1 = ParseUnit(L, __FUNCTION__, 1);
	if (unit1 == NULL) {
		return 0;
	}
	CUnit* unit2 = ParseUnit(L, __FUNCTION__, 2);
	if (unit2 == NULL) {
		return 0;
	}

	float3 pos1;
	if (IsAllyUnit(L, unit1)) {
		pos1 = unit1->midPos;
	} else {
		pos1 = CGameHelper::GetUnitErrorPos(unit1, CLuaHandle::GetHandleReadAllyTeam(L));
	}
	float3 pos2;
	if (IsAllyUnit(L, unit1)) {
		pos2 = unit2->midPos;
	} else {
		pos2 = CGameHelper::GetUnitErrorPos(unit2, CLuaHandle::GetHandleReadAllyTeam(L));
	}

	float dist;
	//const int args = lua_gettop(L); // number of arguments
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		dist = pos1.distance2D(pos2);
	} else {
		dist = pos1.distance(pos2);
	}
	lua_pushnumber(L, dist);

	return 1;
}


int LuaSyncedRead::GetUnitDefDimensions(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const S3DModel* model = ud->LoadModel();
	if (model == NULL) {
		return 0;
	}
	const S3DModel& m = *model;
	const float3& mid = model->relMidPos;
	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "height", m.height);
	HSTR_PUSH_NUMBER(L, "radius", m.radius);
	HSTR_PUSH_NUMBER(L, "midx",   mid.x);
	HSTR_PUSH_NUMBER(L, "minx",   m.mins.x);
	HSTR_PUSH_NUMBER(L, "maxx",   m.maxs.x);
	HSTR_PUSH_NUMBER(L, "midy",   mid.y);
	HSTR_PUSH_NUMBER(L, "miny",   m.mins.y);
	HSTR_PUSH_NUMBER(L, "maxy",   m.maxs.y);
	HSTR_PUSH_NUMBER(L, "midz",   mid.z);
	HSTR_PUSH_NUMBER(L, "minz",   m.mins.z);
	HSTR_PUSH_NUMBER(L, "maxz",   m.maxs.z);
	return 1;
}


int LuaSyncedRead::GetUnitBlocking(lua_State *L)
{
	const CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	lua_pushboolean(L, unit->blocking);
	lua_pushboolean(L, unit->crushable);
	return 2;
}


int LuaSyncedRead::GetUnitMoveTypeData(lua_State *L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	AMoveType* amt = unit->moveType;

	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "maxSpeed", amt->GetMaxSpeed() * GAME_SPEED);
	HSTR_PUSH_NUMBER(L, "maxWantedSpeed", amt->GetMaxWantedSpeed() * GAME_SPEED);
	HSTR_PUSH_NUMBER(L, "goalx", amt->goalPos.x);
	HSTR_PUSH_NUMBER(L, "goaly", amt->goalPos.y);
	HSTR_PUSH_NUMBER(L, "goalz", amt->goalPos.z);

	const CGroundMoveType* groundmt = dynamic_cast<CGroundMoveType*>(unit->moveType);

	if (groundmt != NULL) {
		HSTR_PUSH_STRING(L, "name", "ground");

		HSTR_PUSH_NUMBER(L, "turnRate", groundmt->turnRate);
		HSTR_PUSH_NUMBER(L, "accRate", groundmt->accRate);
		HSTR_PUSH_NUMBER(L, "decRate", groundmt->decRate);

		HSTR_PUSH_NUMBER(L, "maxReverseSpeed", groundmt->maxReverseSpeed * GAME_SPEED);
		HSTR_PUSH_NUMBER(L, "wantedSpeed", groundmt->wantedSpeed * GAME_SPEED);
		HSTR_PUSH_NUMBER(L, "currentSpeed", groundmt->currentSpeed * GAME_SPEED);

		HSTR_PUSH_NUMBER(L, "goalRadius", groundmt->goalRadius);

		HSTR_PUSH_NUMBER(L, "currwaypointx", groundmt->currWayPoint.x);
		HSTR_PUSH_NUMBER(L, "currwaypointy", groundmt->currWayPoint.y);
		HSTR_PUSH_NUMBER(L, "currwaypointz", groundmt->currWayPoint.z);
		HSTR_PUSH_NUMBER(L, "nextwaypointx", groundmt->nextWayPoint.x);
		HSTR_PUSH_NUMBER(L, "nextwaypointy", groundmt->nextWayPoint.y);
		HSTR_PUSH_NUMBER(L, "nextwaypointz", groundmt->nextWayPoint.z);

		HSTR_PUSH_NUMBER(L, "requestedSpeed", 0.0f);

		HSTR_PUSH_NUMBER(L, "pathFailures", 0);

		return 1;
	}

	const AAirMoveType* aamt = dynamic_cast<AAirMoveType*>(unit->moveType);

	if (aamt != NULL) {
		HSTR_PUSH_NUMBER(L, "padStatus", aamt->GetPadStatus());
		HSTR_PUSH_NUMBER(L, "repairBelowHealth", aamt->GetRepairBelowHealth());
	}

	const CHoverAirMoveType* hAMT = dynamic_cast<CHoverAirMoveType*>(unit->moveType);

	if (hAMT != NULL) {
		HSTR_PUSH_STRING(L, "name", "gunship");

		HSTR_PUSH_NUMBER(L, "wantedHeight", hAMT->wantedHeight);
		HSTR_PUSH_BOOL(L, "collide", hAMT->collide);
		HSTR_PUSH_BOOL(L, "useSmoothMesh", hAMT->useSmoothMesh);

		switch (hAMT->aircraftState) {
			case AAirMoveType::AIRCRAFT_LANDED:
				HSTR_PUSH_STRING(L, "aircraftState", "landed");
				break;
			case AAirMoveType::AIRCRAFT_FLYING:
				HSTR_PUSH_STRING(L, "aircraftState", "flying");
				break;
			case AAirMoveType::AIRCRAFT_LANDING:
				HSTR_PUSH_STRING(L, "aircraftState", "landing");
				break;
			case AAirMoveType::AIRCRAFT_CRASHING:
				HSTR_PUSH_STRING(L, "aircraftState", "crashing");
				break;
			case AAirMoveType::AIRCRAFT_TAKEOFF:
				HSTR_PUSH_STRING(L, "aircraftState", "takeoff");
				break;
			case AAirMoveType::AIRCRAFT_HOVERING:
				HSTR_PUSH_STRING(L, "aircraftState", "hovering");
				break;
		};

		switch (hAMT->flyState) {
			case CHoverAirMoveType::FLY_CRUISING:
				HSTR_PUSH_STRING(L, "flyState", "cruising");
				break;
			case CHoverAirMoveType::FLY_CIRCLING:
				HSTR_PUSH_STRING(L, "flyState", "circling");
				break;
			case CHoverAirMoveType::FLY_ATTACKING:
				HSTR_PUSH_STRING(L, "flyState", "attacking");
				break;
			case CHoverAirMoveType::FLY_LANDING:
				HSTR_PUSH_STRING(L, "flyState", "landing");
				break;
		}

		HSTR_PUSH_NUMBER(L, "goalDistance", hAMT->goalDistance);

		HSTR_PUSH_BOOL(L, "bankingAllowed", hAMT->bankingAllowed);
		HSTR_PUSH_NUMBER(L, "currentBank", hAMT->currentBank);
		HSTR_PUSH_NUMBER(L, "currentPitch", hAMT->currentPitch);

		HSTR_PUSH_NUMBER(L, "turnRate", hAMT->turnRate);
		HSTR_PUSH_NUMBER(L, "accRate", hAMT->accRate);
		HSTR_PUSH_NUMBER(L, "decRate", hAMT->decRate);
		HSTR_PUSH_NUMBER(L, "altitudeRate", hAMT->altitudeRate);

		HSTR_PUSH_NUMBER(L, "brakeDistance", hAMT->brakeDistance);
		HSTR_PUSH_BOOL(L, "dontLand", hAMT->dontLand);
		HSTR_PUSH_NUMBER(L, "maxDrift", hAMT->maxDrift);

		return 1;
	}

	const CStrafeAirMoveType* sAMT = dynamic_cast<CStrafeAirMoveType*>(unit->moveType);

	if (sAMT != NULL) {
		HSTR_PUSH_STRING(L, "name", "airplane");

		switch (sAMT->aircraftState) {
			case AAirMoveType::AIRCRAFT_LANDED:
				HSTR_PUSH_STRING(L, "aircraftState", "landed");
				break;
			case AAirMoveType::AIRCRAFT_FLYING:
				HSTR_PUSH_STRING(L, "aircraftState", "flying");
				break;
			case AAirMoveType::AIRCRAFT_LANDING:
				HSTR_PUSH_STRING(L, "aircraftState", "landing");
				break;
			case AAirMoveType::AIRCRAFT_CRASHING:
				HSTR_PUSH_STRING(L, "aircraftState", "crashing");
				break;
			case AAirMoveType::AIRCRAFT_TAKEOFF:
				HSTR_PUSH_STRING(L, "aircraftState", "takeoff");
				break;
			case AAirMoveType::AIRCRAFT_HOVERING:
				HSTR_PUSH_STRING(L, "aircraftState", "hovering");
				break;
		};
		HSTR_PUSH_NUMBER(L, "wantedHeight", sAMT->wantedHeight);
		HSTR_PUSH_BOOL(L, "collide", sAMT->collide);
		HSTR_PUSH_BOOL(L, "useSmoothMesh", sAMT->useSmoothMesh);

		HSTR_PUSH_NUMBER(L, "myGravity", sAMT->myGravity);

		HSTR_PUSH_NUMBER(L, "maxBank", sAMT->maxBank);
		HSTR_PUSH_NUMBER(L, "maxPitch", sAMT->maxBank);
		HSTR_PUSH_NUMBER(L, "turnRadius", sAMT->turnRadius);

		HSTR_PUSH_NUMBER(L, "maxAcc", sAMT->maxAcc);
		HSTR_PUSH_NUMBER(L, "maxAileron", sAMT->maxAileron);
		HSTR_PUSH_NUMBER(L, "maxElevator", sAMT->maxElevator);
		HSTR_PUSH_NUMBER(L, "maxRudder", sAMT->maxRudder);

		return 1;
	}

	const CStaticMoveType* staticmt = dynamic_cast<CStaticMoveType*>(unit->moveType);

	if (staticmt != NULL) {
		HSTR_PUSH_STRING(L, "name", "static");
		return 1;
	}

	const CScriptMoveType* scriptmt = dynamic_cast<CScriptMoveType*>(unit->moveType);

	if (scriptmt != NULL) {
		HSTR_PUSH_STRING(L, "name", "script");
		return 1;
	}

	HSTR_PUSH_STRING(L, "name", "unknown");
	return 1;
}



/******************************************************************************/

static void PackCommand(lua_State* L, const Command& cmd)
{
	lua_createtable(L, 0, 4);

	HSTR_PUSH_NUMBER(L, "id", cmd.GetID());
	HSTR_PUSH(L, "params");

	lua_createtable(L, cmd.params.size(), 0);
	for (size_t p = 0; p < cmd.params.size(); p++) {
		lua_pushnumber(L, p + 1);
		lua_pushnumber(L, cmd.params[p]);
		lua_rawset(L, -3);
	}

	// t["params"] = {[1] = param1, ...}
	lua_rawset(L, -3);

	HSTR_PUSH(L, "options");
	lua_createtable(L, 0, 7);
	HSTR_PUSH_NUMBER(L, "coded", cmd.options);
	if (cmd.options & ALT_KEY)         { HSTR_PUSH_BOOL(L, "alt",      true); }
	if (cmd.options & CONTROL_KEY)     { HSTR_PUSH_BOOL(L, "ctrl",     true); }
	if (cmd.options & SHIFT_KEY)       { HSTR_PUSH_BOOL(L, "shift",    true); }
	if (cmd.options & RIGHT_MOUSE_KEY) { HSTR_PUSH_BOOL(L, "right",    true); }
	if (cmd.options & INTERNAL_ORDER)  { HSTR_PUSH_BOOL(L, "internal", true); }
	if (cmd.options & META_KEY)        { HSTR_PUSH_BOOL(L, "meta",     true); }
	lua_rawset(L, -3); // options table

	HSTR_PUSH_NUMBER(L, "tag", cmd.tag);
}


static void PackCommandQueue(lua_State* L, const CCommandQueue& commands, size_t count)
{
	lua_createtable(L, commands.size(), 0);

	size_t c = 0;
	CCommandQueue::const_iterator ci;
	for (ci = commands.begin(); ci != commands.end(); ++ci) {
		if (c >= count) {
			break;
		}
		c++;
		lua_pushnumber(L, c);
		PackCommand(L, *ci);
		lua_rawset(L, -3);
	}
}


int LuaSyncedRead::GetUnitCommands(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args >= 2) && !lua_isnumber(L, 2)) {
		luaL_error(L,
			"Incorrect arguments to GetUnitCommands(unitID [, count])");
	}

	GML_STDMUTEX_LOCK(cai); // GetUnitCommands

	const CCommandAI* commandAI = unit->commandAI;
	if (commandAI == NULL) {
		return 0;
	}

	// send the new unit commands for factories, otherwise the normal commands
	const CCommandQueue* queue;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	if (factoryCAI == NULL) {
		queue = &commandAI->commandQue;
	} else {
		queue = &factoryCAI->newUnitCommands;
	}

	// get the desired number of commands to return
	int count = -1;
	if (args >= 2) {
 		count = lua_toint(L, 2);
	}
	if (count < 0) {
		count = (int)queue->size();
	}

	PackCommandQueue(L, *queue, count);

	return 1;
}


int LuaSyncedRead::GetFactoryCommands(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	GML_STDMUTEX_LOCK(cai); // GetFactoryCommands

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	if (!factoryCAI) {
		return 0; // not a factory, bail
	}
	const CCommandQueue& commandQue = factoryCAI->commandQue;

	const int args = lua_gettop(L); // number of arguments
	if ((args >= 2) && !lua_isnumber(L, 2)) {
		luaL_error(L,
			"Incorrect arguments to GetFactoryCommands(unitID [, count])");
	}

	// get the desired number of commands to return
	int count = -1;
	if (args >= 2) {
 		count = lua_toint(L, 2);
	}
	if (count < 0) {
		count = (int)commandQue.size();
	}

	PackCommandQueue(L, commandQue, count);

	return 1;
}


static void PackFactoryCounts(lua_State* L,
                              const CCommandQueue& q, int count, bool noCmds)
{
	lua_newtable(L);

	int entry = 0;
	int currentCmd = 0;
	int currentCount = 0;

	CCommandQueue::const_iterator it = q.begin();
	for (it = q.begin(); it != q.end(); ++it) {
		if (entry >= count) {
			currentCount = 0;
			break;
		}
		const int& cmdID = it->GetID();
		if (noCmds && (cmdID >= 0)) {
			continue;
		}

		if (entry == 0) {
			currentCmd = cmdID;
			currentCount = 1;
			entry = 1;
		}
		else if (cmdID == currentCmd) {
			currentCount++;
		}
		else {
			entry++;
			lua_pushnumber(L, entry);
			lua_newtable(L); {
				lua_pushnumber(L, -currentCmd); // invert
				lua_pushnumber(L, currentCount);
				lua_rawset(L, -3);
			}
			lua_rawset(L, -3);
			currentCmd = cmdID;
			currentCount = 1;
		}
	}
	if (currentCount > 0) {
		entry++;
		lua_pushnumber(L, entry);
		lua_newtable(L); {
			lua_pushnumber(L, -currentCmd); // invert
			lua_pushnumber(L, currentCount);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}

	hs_n.PushNumber(L, entry);
}


int LuaSyncedRead::GetFactoryCounts(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	GML_STDMUTEX_LOCK(cai); // GetFactoryCounts

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	if (!factoryCAI) {
		return 0; // not a factory, bail
	}
	const CCommandQueue& commandQue = factoryCAI->commandQue;

	const int args = lua_gettop(L); // number of arguments
	if ((args >= 2) && !lua_isnumber(L, 2)) {
		luaL_error(L,
			"Incorrect arguments to GetFactoryCounts(unitID[, count[, addCmds]])");
	}

	// get the desired number of commands to return
	int count = -1;
	if (args >= 2) {
 		count = lua_toint(L, 2);
	}
	if (count < 0) {
		count = (int)commandQue.size();
	}

	const bool noCmds = (args < 3) ? true : !lua_toboolean(L, 3);

	PackFactoryCounts(L, commandQue, count, noCmds);

	return 1;
}


int LuaSyncedRead::GetCommandQueue(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args >= 2) && !lua_isnumber(L, 2)) {
		luaL_error(L,
			"Incorrect arguments to GetCommandQueue(unitID [, count])");
	}

	GML_STDMUTEX_LOCK(cai); // GetCommandQueue

	const CCommandAI* commandAI = unit->commandAI;
	if (commandAI == NULL) {
		return 0;
	}

	// send the new unit commands for factories, otherwise the normal commands
	const CCommandQueue* queue;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	if (factoryCAI == NULL) {
		queue = &commandAI->commandQue;
	} else {
		queue = &factoryCAI->newUnitCommands;
	}

	// get the desired number of commands to return
	int count = -1;
	if (args >= 2) {
 		count = lua_toint(L, 2);
	}
	if (count < 0) {
		count = (int)queue->size();
	}

	PackCommandQueue(L, *queue, count);

	return 1;
}


static int PackBuildQueue(lua_State* L, bool canBuild, const char* caller)
{
	CUnit* unit = ParseAllyUnit(L, caller, 1);
	if (unit == NULL) {
		return 0;
	}
	const CCommandAI* commandAI = unit->commandAI;
	if (commandAI == NULL) {
		return 0;
	}
	const CCommandQueue& commandQue = commandAI->commandQue;

	lua_newtable(L);

	int entry = 0;
	int currentType = -1;
	int currentCount = 0;
	CCommandQueue::const_iterator it;
	for (it = commandQue.begin(); it != commandQue.end(); ++it) {
		if (it->GetID() < 0) { // a build command
			const int unitDefID = -(it->GetID());

			if (canBuild) {
				// skip build orders that this unit can not start
				const UnitDef* order_ud = unitDefHandler->GetUnitDefByID(unitDefID);
				const UnitDef* builder_ud = unit->unitDef;
				if ((order_ud == NULL) || (builder_ud == NULL)) {
					continue; // something is wrong, bail
				}
				const string& orderName = StringToLower(order_ud->name);
				const map<int, std::string>& bOpts = builder_ud->buildOptions;
				map<int, std::string>::const_iterator it;
				for (it = bOpts.begin(); it != bOpts.end(); ++it) {
					if (it->second == orderName) {
						break;
					}
				}
				if (it == bOpts.end()) {
					continue; // didn't find a matching entry
				}
			}

			if (currentType == unitDefID) {
				currentCount++;
			} else if (currentType == -1) {
				currentType = unitDefID;
				currentCount = 1;
			} else {
				entry++;
				lua_pushnumber(L, entry);
				lua_newtable(L);
				lua_pushnumber(L, currentType);
				lua_pushnumber(L, currentCount);
				lua_rawset(L, -3);
				lua_rawset(L, -3);
				currentType = unitDefID;
				currentCount = 1;
			}
		}
	}

	if (currentCount > 0) {
		entry++;
		lua_pushnumber(L, entry);
		lua_newtable(L);
		lua_pushnumber(L, currentType);
		lua_pushnumber(L, currentCount);
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}

	lua_pushnumber(L, entry);

	return 2;
}


int LuaSyncedRead::GetFullBuildQueue(lua_State* L)
{
	GML_STDMUTEX_LOCK(cai); // GetFullBuildQueue

	return PackBuildQueue(L, false, __FUNCTION__);
}


int LuaSyncedRead::GetRealBuildQueue(lua_State* L)
{
	GML_STDMUTEX_LOCK(cai); // GetRealBuildQueue

	return PackBuildQueue(L, true, __FUNCTION__);
}


/******************************************************************************/

int LuaSyncedRead::GetUnitRulesParams(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (IsAllyUnit(L, unit) || game->gameOver) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler->AlliedTeams(unit->team, CLuaHandle::GetHandleReadTeam(L)) || ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && CLuaHandle::GetHandleFullRead(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		//! NoAccessTeam
	}
	else if (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INLOS) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	}
	else if (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INRADAR) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INRADAR_MASK;
	}

	const LuaRulesParams::Params&  params    = unit->modParams;
	const LuaRulesParams::HashMap& paramsMap = unit->modParamsMap;

	return PushRulesParams(L, __FUNCTION__, params, paramsMap, losMask);
}


int LuaSyncedRead::GetUnitRulesParam(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (IsAllyUnit(L, unit) || game->gameOver) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler->AlliedTeams(unit->team, CLuaHandle::GetHandleReadTeam(L)) || ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && CLuaHandle::GetHandleFullRead(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		//! NoAccessTeam
	}
	else if (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INLOS) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	}
	else if (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INRADAR) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INRADAR_MASK;
	}

	const LuaRulesParams::Params&  params    = unit->modParams;
	const LuaRulesParams::HashMap& paramsMap = unit->modParamsMap;

	return GetRulesParam(L, __FUNCTION__, 2, params, paramsMap, losMask);
}


/******************************************************************************/

int LuaSyncedRead::GetUnitCmdDescs(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	vector<CommandDescription>& cmdDescs = unit->commandAI->possibleCommands;
	const int lastDesc = (int)cmdDescs.size() - 1;

	const int args = lua_gettop(L); // number of arguments
	int startIndex = 0;
	int endIndex = lastDesc;
	if ((args >= 2) && lua_isnumber(L, 2)) {
		startIndex = lua_toint(L, 2) - 1;
		startIndex = max(0, min(lastDesc, startIndex));
		if ((args >= 3) && lua_isnumber(L, 3)) {
			endIndex = lua_toint(L, 3) - 1;
			endIndex = max(0, min(lastDesc, endIndex));
		} else {
			endIndex = startIndex;
		}
	}
	lua_createtable(L, endIndex - startIndex, 0);
	int count = 0;
	for (int i = startIndex; i <= endIndex; i++) {
		count++;
		lua_pushnumber(L, count);
		LuaUtils::PushCommandDesc(L, cmdDescs[i]);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaSyncedRead::FindUnitCmdDesc(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int cmdID = luaL_checkint(L, 2);

	vector<CommandDescription>& cmdDescs = unit->commandAI->possibleCommands;
	for (int i = 0; i < (int)cmdDescs.size(); i++) {
		if (cmdDescs[i].id == cmdID) {
			lua_pushnumber(L, i + 1);
			return 1;
		}
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

static CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		if (caller != NULL) {
			luaL_error(L, "Incorrect arguments to %s(featureID)", caller);
		} else {
			return NULL;
		}
	}
	const int featureID = lua_toint(L, index);
	CFeature* feature = featureHandler->GetFeature(featureID);

	if (!feature) {
		return NULL;
	}

	if (!IsFeatureVisible(L, feature)) {
		return NULL;
	}
	return feature;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::ValidFeatureID(lua_State* L)
{
	CFeature* feature = ParseFeature(L, NULL, 1); // note the NULL
	lua_pushboolean(L, feature != NULL);
	return 1;
}


int LuaSyncedRead::GetAllFeatures(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	int count = 0;
	const CFeatureSet& activeFeatures = featureHandler->GetActiveFeatures();
	CFeatureSet::const_iterator fit;
	if (CLuaHandle::GetHandleFullRead(L)) {
		lua_createtable(L, activeFeatures.size(), 0);
		for (fit = activeFeatures.begin(); fit != activeFeatures.end(); ++fit) {
			lua_pushnumber(L, (*fit)->id);
			lua_rawseti(L, -2, ++count);
		}
	}
	else {
		lua_newtable(L);
		for (fit = activeFeatures.begin(); fit != activeFeatures.end(); ++fit) {
			if (IsFeatureVisible(L, *fit)) {
				lua_pushnumber(L, (*fit)->id);
				lua_rawseti(L, -2, ++count);
			}
		}
	}
	return 1;
}


int LuaSyncedRead::GetFeatureDefID(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	lua_pushnumber(L, feature->def->id);
	return 1;
}


int LuaSyncedRead::GetFeatureTeam(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	if (feature->allyteam < 0) {
		lua_pushnumber(L, -1);
	} else {
		lua_pushnumber(L, feature->team);
	}
	return 1;
}


int LuaSyncedRead::GetFeatureAllyTeam(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	lua_pushnumber(L, feature->allyteam);
	return 1;
}


int LuaSyncedRead::GetFeatureHealth(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	lua_pushnumber(L, feature->health);
	lua_pushnumber(L, feature->def->health);
	lua_pushnumber(L, feature->resurrectProgress);
	return 3;
}


int LuaSyncedRead::GetFeatureHeight(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	lua_pushnumber(L, feature->height);
	return 1;
}


int LuaSyncedRead::GetFeatureRadius(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	lua_pushnumber(L, feature->radius);
	return 1;
}


int LuaSyncedRead::GetFeaturePosition(lua_State* L)
{
	int argc = 0;
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return argc;
	}

	lua_pushnumber(L, feature->pos.x);
	lua_pushnumber(L, feature->pos.y);
	lua_pushnumber(L, feature->pos.z);
	argc += 3;

	if (lua_isboolean(L, 2) && lua_toboolean(L, 2)) {
		lua_pushnumber(L, feature->midPos.x);
		lua_pushnumber(L, feature->midPos.y);
		lua_pushnumber(L, feature->midPos.z);
		argc += 3;
	}
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		lua_pushnumber(L, feature->aimPos.x);
		lua_pushnumber(L, feature->aimPos.y);
		lua_pushnumber(L, feature->aimPos.z);
		argc += 3;
	}

	return argc;
}


int LuaSyncedRead::GetFeatureDirection(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	const CMatrix44f& mat = feature->GetTransformMatrixRef();
	lua_pushnumber(L, mat[ 8]);
	lua_pushnumber(L, mat[ 9]);
	lua_pushnumber(L, mat[10]);
	return 3;
}


int LuaSyncedRead::GetFeatureHeading(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	lua_pushnumber(L, feature->heading);
	return 1;
}


int LuaSyncedRead::GetFeatureResources(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	lua_pushnumber(L,  feature->RemainingMetal());
	lua_pushnumber(L,  feature->def->metal);
	lua_pushnumber(L,  feature->RemainingEnergy());
	lua_pushnumber(L,  feature->def->energy);
	lua_pushnumber(L,  feature->reclaimLeft);
	return 5;
}


int LuaSyncedRead::GetFeatureNoSelect(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	lua_pushboolean(L, feature->noSelect);
	return 1;
}


int LuaSyncedRead::GetFeatureResurrect(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}

	if (feature->udef == NULL) {
		lua_pushliteral(L, "");
	} else {
		lua_pushsstring(L, feature->udef->name);
	}

	lua_pushnumber(L, feature->buildFacing);
	return 2;
}


int LuaSyncedRead::GetFeatureCollisionVolumeData(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}

	return (PushCollisionVolumeData(L, feature->collisionVolume));
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::GetProjectilePosition(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	lua_pushnumber(L, pro->pos.x);
	lua_pushnumber(L, pro->pos.y);
	lua_pushnumber(L, pro->pos.z);
	return 3;
}

int LuaSyncedRead::GetProjectileVelocity(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	lua_pushnumber(L, pro->speed.x);
	lua_pushnumber(L, pro->speed.y);
	lua_pushnumber(L, pro->speed.z);
	return 3;
}


int LuaSyncedRead::GetProjectileGravity(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	lua_pushnumber(L, pro->mygravity);
	return 1;
}

int LuaSyncedRead::GetProjectileSpinAngle(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL || !pro->piece)
		return 0;

	const CPieceProjectile* ppro = static_cast<const CPieceProjectile*>(pro);
	lua_pushnumber(L, ppro->spinAngle);
	return 1;
}

int LuaSyncedRead::GetProjectileSpinSpeed(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL || !pro->piece)
		return 0;

	const CPieceProjectile* ppro = static_cast<const CPieceProjectile*>(pro);
	lua_pushnumber(L, ppro->spinSpeed);
	return 1;
}

int LuaSyncedRead::GetProjectileSpinVec(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL || !pro->piece)
		return 0;

	const CPieceProjectile* ppro = static_cast<const CPieceProjectile*>(pro);

	lua_pushnumber(L, ppro->spinVec.x);
	lua_pushnumber(L, ppro->spinVec.y);
	lua_pushnumber(L, ppro->spinVec.z);
	return 3;
}


int LuaSyncedRead::GetProjectileTarget(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const CWorldObject* wtgt = wpro->GetTargetObject();

	if (wtgt == NULL)
		return 0;

	if (dynamic_cast<const CUnit*>(wtgt) != NULL) {
		lua_pushnumber(L, wtgt->id);
		lua_pushstring(L, "u");
		return 2;
	}
	if (dynamic_cast<const CFeature*>(wtgt) != NULL) {
		lua_pushnumber(L, wtgt->id);
		lua_pushstring(L, "f");
		return 2;
	}
	if (dynamic_cast<const CWeaponProjectile*>(wtgt) != NULL) {
		lua_pushnumber(L, wtgt->id);
		lua_pushstring(L, "p");
		return 2;
	}

	// projectile target cannot be anything else
	assert(false);
	return 0;
}

int LuaSyncedRead::GetProjectileType(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	lua_pushboolean(L, pro->weapon);
	lua_pushboolean(L, pro->piece);
	return 2;
}

int LuaSyncedRead::GetProjectileName(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	if (pro->weapon) {
		const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);

		if (wpro != NULL && wpro->GetWeaponDef() != NULL) {
			// maybe CWeaponProjectile derivatives
			// should have actual names themselves?
			lua_pushsstring(L, wpro->GetWeaponDef()->name);
			return 1;
		}
	}
	if (pro->piece) {
		const CPieceProjectile* ppro = static_cast<const CPieceProjectile*>(pro);

		if (ppro != NULL && ppro->omp != NULL) {
			lua_pushsstring(L, ppro->omp->name);
			return 1;
		}
	}

	// neither weapon nor piece likely means the projectile is CExpGenSpawner, should we return any name in this case?
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::GetGroundHeight(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	lua_pushnumber(L, ground->GetHeightReal(x, z, CLuaHandle::GetHandleSynced(L)));
	return 1;
}


int LuaSyncedRead::GetGroundOrigHeight(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	lua_pushnumber(L, ground->GetOrigHeight(x, z));
	return 1;
}


int LuaSyncedRead::GetGroundNormal(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	const float3 normal = ground->GetSmoothNormal(x, z, CLuaHandle::GetHandleSynced(L));
	lua_pushnumber(L, normal.x);
	lua_pushnumber(L, normal.y);
	lua_pushnumber(L, normal.z);
	return 3;
}


int LuaSyncedRead::GetGroundInfo(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	const int ix = Clamp(x, 0.0f, float3::maxxpos) / (SQUARE_SIZE * 2);
	const int iz = Clamp(z, 0.0f, float3::maxzpos) / (SQUARE_SIZE * 2);

	const int maxIndex = (gs->hmapx * gs->hmapy) - 1;
	const int sqrIndex = std::min(maxIndex, (gs->hmapx * iz) + ix);
	const int typeIndex = readmap->GetTypeMapSynced()[sqrIndex];

	// arguments to LuaMetalMap::GetMetalAmount in half-heightmap coors
	lua_pop(L, 2);
	lua_pushnumber(L, ix);
	lua_pushnumber(L, iz);

	return (PushTerrainTypeData(L, &mapInfo->terrainTypes[typeIndex], true));
}


// similar to ParseMapParams in LuaSyncedCtrl
static void ParseMapCoords(lua_State* L, const char* caller,
                           int& tx1, int& tz1, int& tx2, int& tz2)
{
	float fx1 = 0, fz1 = 0, fx2 = 0, fz2 = 0;

	const int args = lua_gettop(L); // number of arguments
	if (args == 2) {
		fx1 = fx2 = luaL_checkfloat(L, 1);
		fz1 = fz2 = luaL_checkfloat(L, 2);
	}
	else if (args == 4) {
		fx1 = luaL_checkfloat(L, 1);
		fz1 = luaL_checkfloat(L, 2);
		fx2 = luaL_checkfloat(L, 3);
		fz2 = luaL_checkfloat(L, 4);
	}
	else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	// quantize and clamp
	tx1 = Clamp((int)(fx1 / SQUARE_SIZE), 0, gs->mapxm1);
	tx2 = Clamp((int)(fx2 / SQUARE_SIZE), 0, gs->mapxm1);
	tz1 = Clamp((int)(fz1 / SQUARE_SIZE), 0, gs->mapym1);
	tz2 = Clamp((int)(fz2 / SQUARE_SIZE), 0, gs->mapym1);

	return;
}


int LuaSyncedRead::GetGroundBlocked(lua_State* L)
{
	if ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && !CLuaHandle::GetHandleFullRead(L)) {
		return 0;
	}

	int tx1, tx2, tz1, tz2;
	ParseMapCoords(L, __FUNCTION__, tx1, tz1, tx2, tz2);

	for(int z = tz1; z <= tz2; z++){
		for(int x = tx1; x <= tx2; x++){
			const CSolidObject* s = groundBlockingObjectMap->GroundBlocked(x, z);

			const CFeature* feature = dynamic_cast<const CFeature*>(s);
			if (feature) {
				if (IsFeatureVisible(L, feature)) {
					HSTR_PUSH(L, "feature");
					lua_pushnumber(L, feature->id);
					return 2;
				} else {
					continue;
				}
			}

			const CUnit* unit = dynamic_cast<const CUnit*>(s);
			if (unit) {
				if (CLuaHandle::GetHandleFullRead(L) || (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INLOS)) {
					HSTR_PUSH(L, "unit");
					lua_pushnumber(L, unit->id);
					return 2;
				} else {
					continue;
				}
			}
		}
	}

	lua_pushboolean(L, false);
	return 1;
}


int LuaSyncedRead::GetGroundExtremes(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, readmap->initMinHeight);
	lua_pushnumber(L, readmap->initMaxHeight);
	return 2;
}


int LuaSyncedRead::GetTerrainTypeData(lua_State* L)
{
	const int tti = luaL_checkint(L, 1);

	if (tti < 0 || tti >= CMapInfo::NUM_TERRAIN_TYPES) {
		return 0;
	}

	return (PushTerrainTypeData(L, &mapInfo->terrainTypes[tti], false));
}

/******************************************************************************/

int LuaSyncedRead::GetSmoothMeshHeight(lua_State *L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	lua_pushnumber(L, smoothGround->GetHeight(x, z));
	return 1;
}

/******************************************************************************/
/******************************************************************************/


int LuaSyncedRead::TestMoveOrder(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(unitDefID);

	if (unitDef == NULL || unitDef->pathType == -1U) {
		lua_pushboolean(L, false);
		return 1;
	}

	const MoveDef* moveDef = moveDefHandler->GetMoveDefByPathType(unitDef->pathType);

	if (moveDef == NULL) {
		lua_pushboolean(L, !unitDef->IsImmobileUnit());
		return 1;
	}

	const float3 pos(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));

	bool los = false;
	bool ret = false;

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		los = CLuaHandle::GetHandleFullRead(L);
	} else {
		los = loshandler->InLos(pos, CLuaHandle::GetHandleReadAllyTeam(L));
	}

	if (los) {
		ret = moveDef->TestMoveSquare(NULL, pos.x / SQUARE_SIZE, pos.z / SQUARE_SIZE);
	}

	lua_pushboolean(L, ret);
	return 1;
}

int LuaSyncedRead::TestBuildOrder(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(unitDefID);

	if (unitDef == NULL) {
		lua_pushnumber(L, 0);
		return 1;
	}

	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	const int facing = LuaUtils::ParseFacing(L, __FUNCTION__, 5);

	BuildInfo bi;
	bi.buildFacing = facing;
	bi.def = unitDef;
	bi.pos = pos;
	bi.pos = CGameHelper::Pos2BuildPos(bi, CLuaHandle::GetHandleSynced(L));
	CFeature* feature;

	// negative allyTeam values have full visibility in TestUnitBuildSquare()
	// 0 = BUILDSQUARE_BLOCKED
	// 1 = BUILDSQUARE_OCCUPIED
	// 2 = BUILDSQUARE_RECLAIMABLE
	// 3 = BUILDSQUARE_OPEN
	int retval = CGameHelper::TestUnitBuildSquare(bi, feature, CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleSynced(L));

	// output of TestUnitBuildSquare was changed after this lua function was writen
	// keep backward-compability by mapping:
	// BUILDSQUARE_OPEN = BUILDSQUARE_RECLAIMABLE = 2
	if (retval == CGameHelper::BUILDSQUARE_OPEN)
		retval = 2;

	if (feature == NULL) {
		lua_pushnumber(L, retval);
		return 1;
	}
	lua_pushnumber(L, retval);
	lua_pushnumber(L, feature->id);
	return 2;
}


int LuaSyncedRead::Pos2BuildPos(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}

	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	const int facing = luaL_optint(L, 5, FACING_SOUTH);

	const BuildInfo buildInfo(ud, pos, facing);
	const float3 buildPos = CGameHelper::Pos2BuildPos(buildInfo, CLuaHandle::GetHandleSynced(L));

	lua_pushnumber(L, buildPos.x);
	lua_pushnumber(L, buildPos.y);
	lua_pushnumber(L, buildPos.z);

	return 3;
}


/******************************************************************************/
/******************************************************************************/

static int GetEffectiveLosAllyTeam(lua_State* L, int arg)
{
	if (lua_isnoneornil(L, arg)) {
		return CLuaHandle::GetHandleReadAllyTeam(L);
	}
	const bool isGaia = (CLuaHandle::GetHandleReadAllyTeam(L) == teamHandler->GaiaAllyTeamID());
	if (CLuaHandle::GetHandleFullRead(L) || isGaia) {
		const int at = luaL_checkint(L, arg);
		if (at >= teamHandler->ActiveAllyTeams()) {
			luaL_error(L, "Invalid allyTeam");
		}
		if (isGaia && (at >= 0) && (at != teamHandler->GaiaAllyTeamID())) {
			luaL_error(L, "Invalid gaia access");
		}
		return at;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		luaL_error(L, "Invalid access");
	}
	return CLuaHandle::GetHandleReadAllyTeam(L);
}


int LuaSyncedRead::GetPositionLosState(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);

	bool inLos    = false;
	bool inRadar  = false;

	if (allyTeamID >= 0) {
		inLos   = loshandler->InLos(pos, allyTeamID);
		inRadar = radarhandler->InRadar(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < teamHandler->ActiveAllyTeams(); at++) {
			if (loshandler->InLos(pos, at)) {
				inLos = true;
				break;
			}
		}
		for (int at = 0; at < teamHandler->ActiveAllyTeams(); at++) {
			if (radarhandler->InRadar(pos, at)) {
				inRadar = true;
				break;
			}
		}
	}
	lua_pushboolean(L, inLos || inRadar);
	lua_pushboolean(L, inLos);
	lua_pushboolean(L, inRadar);

	return 3;
}


int LuaSyncedRead::IsPosInLos(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);

	bool state = false;
	if (allyTeamID >= 0) {
		state = loshandler->InLos(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < teamHandler->ActiveAllyTeams(); at++) {
			if (loshandler->InLos(pos, at)) {
				state = true;
				break;
			}
		}
	}
	lua_pushboolean(L, state);

	return 1;
}


int LuaSyncedRead::IsPosInRadar(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);

	bool state = false;
	if (allyTeamID >= 0) {
		state = radarhandler->InRadar(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < teamHandler->ActiveAllyTeams(); at++) {
			if (radarhandler->InRadar(pos, at)) {
				state = true;
				break;
			}
		}
	}
	lua_pushboolean(L, state);

	return 1;
}


int LuaSyncedRead::IsPosInAirLos(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);

	bool state = false;
	if (allyTeamID >= 0) {
		state = loshandler->InAirLos(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < teamHandler->ActiveAllyTeams(); at++) {
			if (loshandler->InAirLos(pos, at)) {
				state = true;
				break;
			}
		}
	}
	lua_pushboolean(L, state);

	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetClosestValidPosition(lua_State* L)
{
	// FIXME -- finish this
	/*const int unitDefID = luaL_checkint(L, 1);
	const float x     = luaL_checkfloat(L, 2);
	const float z     = luaL_checkfloat(L, 3);
	const float r     = luaL_checkfloat(L, 4);*/
	//const int mx = (int)max(0 , min(gs->mapxm1, (int)(x / SQUARE_SIZE)));
	//const int mz = (int)max(0 , min(gs->mapym1, (int)(z / SQUARE_SIZE)));
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::GetUnitPieceMap(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localModel;
	lua_newtable(L);
	for (size_t i = 0; i < localModel->pieces.size(); i++) {
		const LocalModelPiece& lp = *localModel->pieces[i];
		lua_pushsstring(L, lp.original->name);
		lua_pushnumber(L, i + 1);
		lua_rawset(L, -3);
	}
	return 1;
}


int LuaSyncedRead::GetUnitPieceList(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localModel;
	lua_newtable(L);
	for (size_t i = 0; i < localModel->pieces.size(); i++) {
		const LocalModelPiece& lp = *localModel->pieces[i];
		lua_pushnumber(L, i + 1);
		lua_pushsstring(L, lp.original->name);
		lua_rawset(L, -3);
	}
	return 1;
}


template<class ModelType>
static int GetUnitPieceInfo(lua_State* L, const ModelType& op)
{
	lua_newtable(L);
	HSTR_PUSH_STRING(L, "name", op.name.c_str());

	HSTR_PUSH(L, "children");
	lua_newtable(L);
	for (int c = 0; c < (int)op.children.size(); c++) {
		lua_pushnumber(L, c + 1);
		lua_pushsstring(L, op.children[c]->name);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	HSTR_PUSH(L, "isEmpty");
	lua_pushboolean(L, op.isEmpty);
	lua_rawset(L, -3);

	HSTR_PUSH(L, "min");
	lua_newtable(L); {
		lua_pushnumber(L, 1); lua_pushnumber(L, op.mins.x); lua_rawset(L, -3);
		lua_pushnumber(L, 2); lua_pushnumber(L, op.mins.y); lua_rawset(L, -3);
		lua_pushnumber(L, 3); lua_pushnumber(L, op.mins.z); lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	HSTR_PUSH(L, "max");
	lua_newtable(L); {
		lua_pushnumber(L, 1); lua_pushnumber(L, op.maxs.x); lua_rawset(L, -3);
		lua_pushnumber(L, 2); lua_pushnumber(L, op.maxs.y); lua_rawset(L, -3);
		lua_pushnumber(L, 3); lua_pushnumber(L, op.maxs.z); lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	HSTR_PUSH(L, "offset");
	lua_newtable(L); {
		lua_pushnumber(L, 1); lua_pushnumber(L, op.offset.x); lua_rawset(L, -3);
		lua_pushnumber(L, 2); lua_pushnumber(L, op.offset.y); lua_rawset(L, -3);
		lua_pushnumber(L, 3); lua_pushnumber(L, op.offset.z); lua_rawset(L, -3);
	}
	lua_rawset(L, -3);
	return 1;
}


int LuaSyncedRead::GetUnitPieceInfo(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localModel;

	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= localModel->pieces.size())) {
		return 0;
	}

	const S3DModelPiece& op = *localModel->pieces[piece]->original;
	return ::GetUnitPieceInfo(L, op);
}


int LuaSyncedRead::GetUnitPiecePosition(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localModel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= localModel->pieces.size())) {
		return 0;
	}
	const float3 pos = localModel->GetRawPiecePos(piece);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}


int LuaSyncedRead::GetUnitPiecePosDir(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localModel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= localModel->pieces.size())) {
		return 0;
	}
	float3 dir(0,0,0);
	float3 pos(0,0,0);
	localModel->GetRawEmitDirPos(piece, pos, dir);
	pos = unit->pos + unit->frontdir * pos.z
	                + unit->updir    * pos.y
	                + unit->rightdir * pos.x;
	dir = unit->frontdir * dir.z
	    + unit->updir    * dir.y
	    + unit->rightdir * dir.x;
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 6;
}


int LuaSyncedRead::GetUnitPieceDirection(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localModel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= localModel->pieces.size())) {
		return 0;
	}
	const float3 dir = localModel->GetRawPieceDirection(piece);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 3;
}


int LuaSyncedRead::GetUnitPieceMatrix(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalModel* localModel = unit->localModel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= localModel->pieces.size())) {
		return 0;
	}
	const CMatrix44f mat = localModel->GetRawPieceMatrix(piece);
	for (int m = 0; m < 16; m++) {
		lua_pushnumber(L, mat.m[m]);
	}
	return 16;
}


int LuaSyncedRead::GetUnitScriptPiece(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const CUnitScript* script = unit->script;

	if (!lua_isnumber(L, 2)) {
		// return the whole script->piece map
		lua_newtable(L);
		for (size_t sp = 0; sp < script->pieces.size(); sp++) {
			const int piece = script->ScriptToModel(sp);
			if (piece != -1) {
				lua_pushnumber(L, sp);
				lua_pushnumber(L, piece + 1);
				lua_rawset(L, -3);
			}
		}
		return 1;
	}

	const int scriptPiece = lua_toint(L, 2);
	const int piece = script->ScriptToModel(scriptPiece);
	if (piece < 0) {
		return 0;
	}

	lua_pushnumber(L, piece + 1);
	return 1;
}


int LuaSyncedRead::GetUnitScriptNames(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const vector<LocalModelPiece*>& pieces = unit->script->pieces;

	lua_newtable(L);
	for (size_t sp = 0; sp < pieces.size(); sp++) {
		lua_pushsstring(L, pieces[sp]->original->name);
		lua_pushnumber(L, sp);
		lua_rawset(L, -3);
	}

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::GetCOBUnitVar(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int varID = luaL_checkint(L, 2);
	if ((varID < 0) || (varID >= CUnitScript::UNIT_VAR_COUNT)) {
		return 0;
	}
	const int value = unit->script->GetUnitVars()[varID];
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		lua_pushnumber(L, UNPACKX(value));
		lua_pushnumber(L, UNPACKZ(value));
		return 2;
	}
	lua_pushnumber(L, value);
	return 1;
}


int LuaSyncedRead::GetCOBTeamVar(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler->IsValidTeam(teamID)) {
		return 0;
	}
	if (!IsAlliedTeam(L, teamID)) {
		return 0;
	}
	const int varID = luaL_checkint(L, 2);
	if ((varID < 0) || (varID >= CUnitScript::TEAM_VAR_COUNT)) {
		return 0;
	}
	const int value = CUnitScript::GetTeamVars(teamID)[varID];
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		lua_pushnumber(L, UNPACKX(value));
		lua_pushnumber(L, UNPACKZ(value));
		return 2;
	}
	lua_pushnumber(L, value);
	return 1;

}


int LuaSyncedRead::GetCOBAllyTeamVar(lua_State* L)
{
	const int allyTeamID = luaL_checkint(L, 1);
	if (!teamHandler->IsValidAllyTeam(allyTeamID)) {
		return 0;
	}
	if (!IsAlliedAllyTeam(L, allyTeamID)) {
		return 0;
	}
	const int varID = luaL_checkint(L, 2);
	if ((varID < 0) || (varID >= CUnitScript::ALLY_VAR_COUNT)) {
		return 0;
	}
	const int value = CUnitScript::GetAllyVars(allyTeamID)[varID];
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		lua_pushnumber(L, UNPACKX(value));
		lua_pushnumber(L, UNPACKZ(value));
		return 2;
	}
	lua_pushnumber(L, value);
	return 1;
}


int LuaSyncedRead::GetCOBGlobalVar(lua_State* L)
{
	const int varID = luaL_checkint(L, 1);
	if ((varID < 0) || (varID >= CUnitScript::GLOBAL_VAR_COUNT)) {
		return 0;
	}
	const int value = CUnitScript::GetGlobalVars()[varID];
	if (lua_isboolean(L, 2) && lua_toboolean(L, 2)) {
		lua_pushnumber(L, UNPACKX(value));
		lua_pushnumber(L, UNPACKZ(value));
		return 2;
	}
	lua_pushnumber(L, value);
	return 1;
}


/******************************************************************************/
/******************************************************************************/
