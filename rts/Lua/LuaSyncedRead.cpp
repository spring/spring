/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaSyncedRead.h"

#include "LuaInclude.h"

#include "LuaConfig.h"
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
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
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
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/bitops.h"
#include "System/myMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"

#include <set>
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
	REGISTER_LUA_CFUNC(GetUnitMaxRange);
	REGISTER_LUA_CFUNC(GetUnitExperience);
	REGISTER_LUA_CFUNC(GetUnitStates);
	REGISTER_LUA_CFUNC(GetUnitArmored);
	REGISTER_LUA_CFUNC(GetUnitIsActive);
	REGISTER_LUA_CFUNC(GetUnitIsCloaked);
	REGISTER_LUA_CFUNC(GetUnitSelfDTime);
	REGISTER_LUA_CFUNC(GetUnitStockpile);
	REGISTER_LUA_CFUNC(GetUnitSensorRadius);
	REGISTER_LUA_CFUNC(GetUnitPosErrorParams);
	REGISTER_LUA_CFUNC(GetUnitHeight);
	REGISTER_LUA_CFUNC(GetUnitRadius);
	REGISTER_LUA_CFUNC(GetUnitMass);
	REGISTER_LUA_CFUNC(GetUnitPosition);
	REGISTER_LUA_CFUNC(GetUnitBasePosition);
	REGISTER_LUA_CFUNC(GetUnitVectors);
	REGISTER_LUA_CFUNC(GetUnitRotation);
	REGISTER_LUA_CFUNC(GetUnitDirection);
	REGISTER_LUA_CFUNC(GetUnitHeading);
	REGISTER_LUA_CFUNC(GetUnitVelocity);
	REGISTER_LUA_CFUNC(GetUnitBuildFacing);
	REGISTER_LUA_CFUNC(GetUnitIsBuilding);
	REGISTER_LUA_CFUNC(GetUnitCurrentBuildPower);
	REGISTER_LUA_CFUNC(GetUnitHarvestStorage);
	REGISTER_LUA_CFUNC(GetUnitNanoPieces);
	REGISTER_LUA_CFUNC(GetUnitTransporter);
	REGISTER_LUA_CFUNC(GetUnitIsTransporting);
	REGISTER_LUA_CFUNC(GetUnitShieldState);
	REGISTER_LUA_CFUNC(GetUnitFlanking);
	REGISTER_LUA_CFUNC(GetUnitWeaponState);
	REGISTER_LUA_CFUNC(GetUnitWeaponDamages);
	REGISTER_LUA_CFUNC(GetUnitWeaponVectors);
	REGISTER_LUA_CFUNC(GetUnitWeaponTryTarget);
	REGISTER_LUA_CFUNC(GetUnitWeaponTestTarget);
	REGISTER_LUA_CFUNC(GetUnitWeaponTestRange);
	REGISTER_LUA_CFUNC(GetUnitWeaponHaveFreeLineOfFire);
	REGISTER_LUA_CFUNC(GetUnitWeaponCanFire);
	REGISTER_LUA_CFUNC(GetUnitWeaponTarget);
	REGISTER_LUA_CFUNC(GetUnitTravel);
	REGISTER_LUA_CFUNC(GetUnitFuel);
	REGISTER_LUA_CFUNC(GetUnitEstimatedPath);
	REGISTER_LUA_CFUNC(GetUnitLastAttacker);
	REGISTER_LUA_CFUNC(GetUnitLastAttackedPiece);
	REGISTER_LUA_CFUNC(GetUnitLosState);
	REGISTER_LUA_CFUNC(GetUnitSeparation);
	REGISTER_LUA_CFUNC(GetUnitFeatureSeparation);
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
	REGISTER_LUA_CFUNC(GetFeatureMass);
	REGISTER_LUA_CFUNC(GetFeatureRotation);
	REGISTER_LUA_CFUNC(GetFeatureDirection);
	REGISTER_LUA_CFUNC(GetFeatureVelocity);
	REGISTER_LUA_CFUNC(GetFeatureHeading);
	REGISTER_LUA_CFUNC(GetFeatureResources);
	REGISTER_LUA_CFUNC(GetFeatureBlocking);
	REGISTER_LUA_CFUNC(GetFeatureNoSelect);
	REGISTER_LUA_CFUNC(GetFeatureResurrect);
	REGISTER_LUA_CFUNC(GetFeatureLastAttackedPiece);
	REGISTER_LUA_CFUNC(GetFeatureCollisionVolumeData);
	REGISTER_LUA_CFUNC(GetFeaturePieceCollisionVolumeData);
	REGISTER_LUA_CFUNC(GetFeatureSeparation);

	REGISTER_LUA_CFUNC(GetFeatureRulesParam);
	REGISTER_LUA_CFUNC(GetFeatureRulesParams);

	REGISTER_LUA_CFUNC(GetProjectilePosition);
	REGISTER_LUA_CFUNC(GetProjectileDirection);
	REGISTER_LUA_CFUNC(GetProjectileVelocity);
	REGISTER_LUA_CFUNC(GetProjectileGravity);
	REGISTER_LUA_CFUNC(GetProjectileSpinAngle);
	REGISTER_LUA_CFUNC(GetProjectileSpinSpeed);
	REGISTER_LUA_CFUNC(GetProjectileSpinVec);
	REGISTER_LUA_CFUNC(GetPieceProjectileParams);
	REGISTER_LUA_CFUNC(GetProjectileTarget);
	REGISTER_LUA_CFUNC(GetProjectileIsIntercepted);
	REGISTER_LUA_CFUNC(GetProjectileTimeToLive);
	REGISTER_LUA_CFUNC(GetProjectileOwnerID);
	REGISTER_LUA_CFUNC(GetProjectileTeamID);
	REGISTER_LUA_CFUNC(GetProjectileType);
	REGISTER_LUA_CFUNC(GetProjectileDefID);
	REGISTER_LUA_CFUNC(GetProjectileName);
	REGISTER_LUA_CFUNC(GetProjectileDamages);

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
	REGISTER_LUA_CFUNC(IsUnitInLos);
	REGISTER_LUA_CFUNC(IsUnitInAirLos);
	REGISTER_LUA_CFUNC(IsUnitInRadar);
	REGISTER_LUA_CFUNC(IsUnitInJammer);
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

	REGISTER_LUA_CFUNC(GetFeaturePieceMap);
	REGISTER_LUA_CFUNC(GetFeaturePieceList);
	REGISTER_LUA_CFUNC(GetFeaturePieceInfo);
	REGISTER_LUA_CFUNC(GetFeaturePiecePosition);
	REGISTER_LUA_CFUNC(GetFeaturePieceDirection);
	REGISTER_LUA_CFUNC(GetFeaturePiecePosDir);
	REGISTER_LUA_CFUNC(GetFeaturePieceMatrix);

	REGISTER_LUA_CFUNC(GetRadarErrorParams);

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
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0)
		return CLuaHandle::GetHandleFullRead(L);

	return (teamHandler->AllyTeam(team) == CLuaHandle::GetHandleReadAllyTeam(L));
}

static inline bool IsAlliedAllyTeam(lua_State* L, int allyTeam)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0)
		return CLuaHandle::GetHandleFullRead(L);

	return (allyTeam == CLuaHandle::GetHandleReadAllyTeam(L));
}


static inline bool IsAllyUnit(lua_State* L, const CUnit* unit)
{
	return (IsAlliedAllyTeam(L, unit->allyteam));
}

static inline bool IsEnemyUnit(lua_State* L, const CUnit* unit)
{
	return (!IsAllyUnit(L, unit));
}


static inline bool IsUnitVisible(lua_State* L, const CUnit* unit)
{
	if (IsAllyUnit(L, unit))
		return true;

	return (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & (LOS_INLOS | LOS_INRADAR));
}

static inline bool IsUnitInLos(lua_State* L, const CUnit* unit)
{
	if (IsAllyUnit(L, unit))
		return true;

	return (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INLOS);
}


static inline bool IsUnitTyped(lua_State* L, const CUnit* unit)
{
	if (IsAllyUnit(L, unit))
		return true;

	const unsigned short losStatus = unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);

	return ((losStatus & LOS_INLOS) || ((losStatus & prevMask) == prevMask));
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

static inline bool IsProjectileVisible(lua_State* L, const CProjectile* pro)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0)
		return CLuaHandle::GetHandleFullRead(L);

	if ((CLuaHandle::GetHandleReadAllyTeam(L) != pro->GetAllyteamID()) &&
	    (!losHandler->InLos(pro->pos, CLuaHandle::GetHandleReadAllyTeam(L)))) {
		return false;
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/

static int GetSolidObjectLastHitPiece(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;
	if (o->lastHitPiece == nullptr)
		return 0;

	const LocalModelPiece* lmp = o->lastHitPiece;
	const S3DModelPiece* omp = lmp->original;

	if (lua_isboolean(L, 1) && lua_toboolean(L, 1)) {
		lua_pushnumber(L, lmp->GetLModelPieceIndex() + 1);
	} else {
		lua_pushsstring(L, omp->name);
	}

	lua_pushnumber(L, o->lastHitPieceFrame);
	return 2;
}

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

static int PushPieceCollisionVolumeData(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	return (PushCollisionVolumeData(L, lmp->GetCollisionVolume()));
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

static int GetWorldObjectVelocity(lua_State* L, const CWorldObject* o)
{
	if (o == nullptr)
		return 0;

	lua_pushnumber(L, o->speed.x);
	lua_pushnumber(L, o->speed.y);
	lua_pushnumber(L, o->speed.z);
	lua_pushnumber(L, o->speed.w);
	return 4;
}

static int GetSolidObjectMass(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	lua_pushnumber(L, o->mass);

	return 1;
}

static int GetSolidObjectPosition(lua_State* L, const CSolidObject* o, bool isFeature)
{
	if (o == nullptr)
		return 0;

	float3 errorVec;

	// no error for features
	if (!isFeature && !IsAllyUnit(L, static_cast<const CUnit*>(o)))
		errorVec = static_cast<const CUnit*>(o)->GetLuaErrorVector(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));

	// NOTE:
	//   must be called before any pushing to the stack, else
	//   in case of noneornil it will read the pushed items.
	const bool returnMidPos = luaL_optboolean(L, 2, false);
	const bool returnAimPos = luaL_optboolean(L, 3, false);

	// base-position
	lua_pushnumber(L, o->pos.x + errorVec.x);
	lua_pushnumber(L, o->pos.y + errorVec.y);
	lua_pushnumber(L, o->pos.z + errorVec.z);

	if (returnMidPos) {
		lua_pushnumber(L, o->midPos.x + errorVec.x);
		lua_pushnumber(L, o->midPos.y + errorVec.y);
		lua_pushnumber(L, o->midPos.z + errorVec.z);
	}
	if (returnAimPos) {
		lua_pushnumber(L, o->aimPos.x + errorVec.x);
		lua_pushnumber(L, o->aimPos.y + errorVec.y);
		lua_pushnumber(L, o->aimPos.z + errorVec.z);
	}

	return (3 + (3 * returnMidPos) + (3 * returnAimPos));
}

static int GetSolidObjectRotation(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const CMatrix44f& matrix = o->GetTransformMatrix(CLuaHandle::GetHandleSynced(L));
	const float3 angles = matrix.GetEulerAnglesLftHand();

	assert(matrix.IsOrthoNormal());

	lua_pushnumber(L, angles[CMatrix44f::ANGLE_P]);
	lua_pushnumber(L, angles[CMatrix44f::ANGLE_Y]);
	lua_pushnumber(L, angles[CMatrix44f::ANGLE_R]);
	return 3;
}

static int GetSolidObjectBlocking(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	lua_pushboolean(L, o->HasPhysicalStateBit(CSolidObject::PSTATE_BIT_BLOCKING));
	lua_pushboolean(L, o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS));
	lua_pushboolean(L, o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES ));
	lua_pushboolean(L, o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS ));

	lua_pushboolean(L, o->crushable);
	lua_pushboolean(L, o->blockEnemyPushing);
	lua_pushboolean(L, o->blockHeightChanges);

	return 7;
}




/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "[%s] unitID (arg #%d) not a number\n", caller, index);
		return nullptr;
	}

	return (unitHandler->GetUnit(lua_toint(L, index)));
}

static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	// include the vistest for LuaUnsyncedRead
	if (!IsUnitVisible(L, unit))
		return nullptr;

	return unit;
}

static inline CUnit* ParseAllyUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!IsAllyUnit(L, unit))
		return nullptr;

	return unit;
}

static inline CUnit* ParseInLosUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!IsUnitInLos(L, unit))
		return nullptr;

	return unit;
}


static inline CUnit* ParseTypedUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!IsUnitTyped(L, unit))
		return nullptr;

	return unit;
}


static CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "[%s] featureID (arg #%d) not a number\n", caller, index);
		return nullptr;
	}

	CFeature* feature = featureHandler->GetFeature(lua_toint(L, index));

	if (feature == nullptr)
		return nullptr;

	// include the vistest for LuaUnsyncedRead
	if (!IsFeatureVisible(L, feature))
		return nullptr;

	return feature;
}


static CProjectile* ParseProjectile(lua_State* L, const char* caller, int index)
{
	CProjectile* p = projectileHandler->GetProjectileBySyncedID(luaL_checkint(L, index));

	if (p == nullptr)
		return nullptr;

	if (!IsProjectileVisible(L, p))
		return nullptr;

	return p;
}


static inline CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	const int teamID = luaL_checkint(L, index);
	if (!teamHandler->IsValidTeam(teamID)) {
		luaL_error(L, "Bad teamID in %s\n", caller);
	}
	return teamHandler->Team(teamID);
}


/******************************************************************************/

static int PushRulesParams(lua_State* L, const char* caller,
                          const LuaRulesParams::Params& params,
                          const int losStatus)
{
	lua_createtable(L, 0, params.size());

	for (auto& it: params) {
		const std::string& name = it.first;
		const LuaRulesParams::Param& param = it.second;
		if (!(param.los & losStatus))
			continue;

		if (!param.valueString.empty()) {
			LuaPushNamedString(L, name, param.valueString);
		} else {
			LuaPushNamedNumber(L, name, param.valueInt);
		}
	}

	return 1;
}


static int GetRulesParam(lua_State* L, const char* caller, int index,
                          const LuaRulesParams::Params& params,
                          const int& losStatus)
{
	const string key = luaL_checkstring(L, index);
	auto it = params.find(key);
	if (it == params.end())
		return 0;

	const LuaRulesParams::Param& param = it->second;

	if (param.los & losStatus) {
		if (!param.valueString.empty()) {
			lua_pushsstring(L, param.valueString);
		} else {
			lua_pushnumber(L, param.valueInt);
		}
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
	lua_pushboolean(L, gs->cheatEnabled);
	return 1;
}


int LuaSyncedRead::IsGodModeEnabled(lua_State* L)
{
	lua_pushboolean(L, gs->godMode);
	return 1;
}


int LuaSyncedRead::IsDevLuaEnabled(lua_State* L)
{
	lua_pushboolean(L, CLuaHandle::GetDevMode());
	return 1;
}


int LuaSyncedRead::IsEditDefsEnabled(lua_State* L)
{
	lua_pushboolean(L, gs->editDefsEnabled);
	return 1;
}


int LuaSyncedRead::AreHelperAIsEnabled(lua_State* L)
{
	if (!game) {
		return 0;
	}
	lua_pushboolean(L, !gs->noHelperAIs);
	return 1;
}


int LuaSyncedRead::FixedAllies(lua_State* L)
{
	if (!game) {
		return 0;
	}
	lua_pushboolean(L, !(gameSetup != NULL && !gameSetup->fixedAllies));
	return 1;
}


int LuaSyncedRead::IsGameOver(lua_State* L)
{
	if (!game) {
		return 0;
	}
	lua_pushboolean(L, game->IsGameOver());
	return 1;
}


int LuaSyncedRead::GetGaiaTeamID(lua_State* L)
{
	if (!gs->useLuaGaia) {
		return 0;
	}
	lua_pushnumber(L, teamHandler->GaiaTeamID());
	return 1;
}


int LuaSyncedRead::GetGameFrame(lua_State* L)
{
	const int simFrames = gs->GetLuaSimFrame();
	const int dayFrames = GAME_SPEED * (24 * 60 * 60);

	lua_pushnumber(L, simFrames % dayFrames);
	lua_pushnumber(L, simFrames / dayFrames);
	return 2;
}


int LuaSyncedRead::GetGameSeconds(lua_State* L)
{
	lua_pushnumber(L, gs->GetLuaSimFrame() / (1.0f * GAME_SPEED));
	return 1;
}


int LuaSyncedRead::GetWind(lua_State* L)
{
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

	//! always readable for all
	const int losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;

	return PushRulesParams(L, __FUNCTION__, params, losMask);
}


int LuaSyncedRead::GetGameRulesParam(lua_State* L)
{
	const LuaRulesParams::Params&  params    = CLuaHandleSynced::GetGameParams();

	//! always readable for all
	const int losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;

	return GetRulesParam(L, __FUNCTION__, 1, params, losMask);
}


/******************************************************************************/

int LuaSyncedRead::GetMapOptions(lua_State* L)
{
	lua_newtable(L);

	const map<string, string>& mapOpts = CGameSetup::GetMapOptions();
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

	const map<string, string>& modOpts = CGameSetup::GetModOptions();
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
			lua_newtable(L); {
				LuaPushNamedString(L, "sideName",  sideParser.GetSideName(i));
				LuaPushNamedString(L, "caseName",  sideParser.GetCaseName(i));
				LuaPushNamedString(L, "startUnit", sideParser.GetStartUnit(i));
			}
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	return 0;
}


/******************************************************************************/

int LuaSyncedRead::GetAllyTeamStartBox(lua_State* L)
{
	const std::vector<AllyTeam>& allyData = CGameSetup::GetAllyStartingData();
	const unsigned int allyTeam = luaL_checkint(L, 1);

	if (allyTeam >= allyData.size())
		return 0;

	const float xmin = (mapDims.mapx * SQUARE_SIZE) * allyData[allyTeam].startRectLeft;
	const float zmin = (mapDims.mapy * SQUARE_SIZE) * allyData[allyTeam].startRectTop;
	const float xmax = (mapDims.mapx * SQUARE_SIZE) * allyData[allyTeam].startRectRight;
	const float zmax = (mapDims.mapy * SQUARE_SIZE) * allyData[allyTeam].startRectBottom;

	lua_pushnumber(L, xmin);
	lua_pushnumber(L, zmin);
	lua_pushnumber(L, xmax);
	lua_pushnumber(L, zmax);
	return 4;
}


int LuaSyncedRead::GetTeamStartPosition(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);

	if (team == NULL)
		return 0;
	if (!IsAlliedTeam(L, team->teamNum))
		return 0;

	const float3& pos = team->GetStartPos();

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushboolean(L, team->HasValidStartPos());
	return 4;
}


/******************************************************************************/

int LuaSyncedRead::GetAllyTeamList(lua_State* L)
{
	lua_newtable(L);
	int count = 1;
	for (int at = 0; at < teamHandler->ActiveAllyTeams(); at++) {
		lua_pushnumber(L, at);
		lua_rawseti(L, -2, count++);
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
	int count = 1;
	for (int t = 0; t < teamHandler->ActiveTeams(); t++) {
		if (teamHandler->Team(t) == NULL) {
			continue;
		}
		if ((allyTeamID < 0) || (allyTeamID == teamHandler->AllyTeam(t))) {
			lua_pushnumber(L, t);
			lua_rawseti(L, -2, count++);
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
	int count = 1;
	for (int p = 0; p < playerHandler->ActivePlayers(); p++) {
		const CPlayer* player = playerHandler->Player(p);
		if (player == NULL) {
			continue;
		}
		if (active && !player->active) {
			continue;
		}
		if ((teamID < 0) || (player->team == teamID)) {
			lua_pushnumber(L, p);
			lua_rawseti(L, -2, count++);
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

	lua_pushnumber(L,  team->teamNum);
	lua_pushnumber(L,  team->GetLeader());
	lua_pushboolean(L, team->isDead);
	lua_pushboolean(L, !skirmishAIHandler.GetSkirmishAIsInTeam(teamID).empty()); // hasAIs
	lua_pushsstring(L, team->GetSide());
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
		lua_pushnumber(L, team->res.metal);
		lua_pushnumber(L, team->resStorage.metal);
		lua_pushnumber(L, team->resPrevPull.metal);
		lua_pushnumber(L, team->resPrevIncome.metal);
		lua_pushnumber(L, team->resPrevExpense.metal);
		lua_pushnumber(L, team->resShare.metal);
		lua_pushnumber(L, team->resPrevSent.metal);
		lua_pushnumber(L, team->resPrevReceived.metal);
		lua_pushnumber(L, team->resPrevExcess.metal);
		return 9;
	}
	else if (type == "energy") {
		lua_pushnumber(L, team->res.energy);
		lua_pushnumber(L, team->resStorage.energy);
		lua_pushnumber(L, team->resPrevPull.energy);
		lua_pushnumber(L, team->resPrevIncome.energy);
		lua_pushnumber(L, team->resPrevExpense.energy);
		lua_pushnumber(L, team->resShare.energy);
		lua_pushnumber(L, team->resPrevSent.energy);
		lua_pushnumber(L, team->resPrevReceived.energy);
		lua_pushnumber(L, team->resPrevExcess.energy);
		return 9;
	}

	return 0;
}


int LuaSyncedRead::GetTeamUnitStats(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL || game == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID) && !game->IsGameOver()) {
		return 0;
	}

	const TeamStatistics& stats = team->GetCurrentStats();
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
	if (team == NULL || game == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID) && !game->IsGameOver()) {
		return 0;
	}

	const TeamStatistics& stats = team->GetCurrentStats();

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
	if (team == NULL || game == NULL) {
		return 0;
	}

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC;

	if (IsAlliedTeam(L, team->teamNum) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler->AlliedTeams(team->teamNum, CLuaHandle::GetHandleReadTeam(L)) || ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && CLuaHandle::GetHandleFullRead(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}

	const LuaRulesParams::Params&  params    = team->modParams;

	return PushRulesParams(L, __FUNCTION__, params, losMask);
}


int LuaSyncedRead::GetTeamRulesParam(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL || game == NULL) {
		return 0;
	}

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC;

	if (IsAlliedTeam(L, team->teamNum) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler->AlliedTeams(team->teamNum, CLuaHandle::GetHandleReadTeam(L)) || ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && CLuaHandle::GetHandleFullRead(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}

	const LuaRulesParams::Params&  params    = team->modParams;

	return GetRulesParam(L, __FUNCTION__, 2, params, losMask);
}


int LuaSyncedRead::GetTeamStatsHistory(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);

	if (team == NULL || game == NULL)
		return 0;

	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID) && !game->IsGameOver())
		return 0;

	const int args = lua_gettop(L);

	if (args == 1) {
		lua_pushnumber(L, team->statHistory.size());
		return 1;
	}

	const auto& teamStats = team->statHistory;
	auto it = teamStats.cbegin();
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
		int count = 1;
		for (int i = start; i <= end; ++i, ++it) {
			const TeamStatistics& stats = *it;
			lua_newtable(L); {
				if (i+1 == teamStats.size()) {
					// the `stats.frame` var indicates the frame when a new entry needs to get added,
					// for the most recent stats entry this lies obviously in the future,
					// so we just output the current frame here
					HSTR_PUSH_NUMBER(L, "time",         gs->GetLuaSimFrame() / GAME_SPEED);
					HSTR_PUSH_NUMBER(L, "frame",        gs->GetLuaSimFrame());
				} else {
					HSTR_PUSH_NUMBER(L, "time",         stats.frame / GAME_SPEED);
					HSTR_PUSH_NUMBER(L, "frame",        stats.frame);
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
			lua_rawseti(L, -2, count++);
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
	lua_pushnumber(L, player->ping * 0.001f); // in seconds
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
		HSTR_PUSH(L, "UNKNOWN");
		HSTR_PUSH(L, "UNKNOWN");
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
	int count = 1;

	if (CLuaHandle::GetHandleFullRead(L)) {
		lua_createtable(L, unitHandler->activeUnits.size(), 0);

		for (CUnit* unit: unitHandler->activeUnits) {
			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, count++);
		}
	} else {
		lua_newtable(L);

		for (CUnit* unit: unitHandler->activeUnits) {
			if (IsUnitVisible(L, unit)) {
				lua_pushnumber(L, unit->id);
				lua_rawseti(L, -2, count++);
			}
		}
	}

	return 1;
}


int LuaSyncedRead::GetTeamUnits(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;


	// raw push for allies
	if (IsAlliedTeam(L, teamID)) {
		lua_newtable(L);
		int count = 1;
		for (const CUnit* unit: team->units) {
			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, count++);
		}

		return 1;
	}

	// check visibility for enemies
	lua_newtable(L);
	int count = 1;
	for (const CUnit* unit: team->units) {
		if (IsUnitVisible(L, unit)) {
			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, count++);
		}
	}

	return 1;
}


static inline void PushUnitsVector(lua_State* L, const std::vector<CUnit*>& vec, int unitDefID, bool& createdTable, unsigned& count, unsigned& defCount, std::vector<int>& unknownIDs)
{
	for(const CUnit* unit: vec) {
		if (!IsUnitVisible(L, unit))
			continue;

		if (!IsUnitTyped(L, unit)) {
			unknownIDs.push_back(unit->id);
			continue;
		}

		if (!createdTable) {
			createdTable = true;
			lua_pushnumber(L, unitDefID);
			lua_newtable(L);
			defCount++;
		}

		lua_pushnumber(L, unit->id);
		lua_rawseti(L, -2, count++);
	}
}

int LuaSyncedRead::GetTeamUnitsSorted(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	static vector<int> unknownIDs;

	lua_newtable(L);
	unsigned defCount = 0;

	const auto& unitsByDefs = unitHandler->unitsByDefs[teamID];

	// tally for allies
	if (IsAlliedTeam(L, teamID)) {
		for (int unitDefID = 0; unitDefID < unitsByDefs.size(); unitDefID++) {

			if (unitsByDefs[unitDefID].empty())
				continue;

			lua_pushnumber(L, unitDefID);
			lua_newtable(L);
			defCount++;

			unsigned count = 1;
			for (const CUnit* unit: unitsByDefs[unitDefID]) {
				lua_pushnumber(L, unit->id);
				lua_rawseti(L, -2, count++);
			}
			lua_rawset(L, -3);
		}
	} else {// tally for enemies
		unknownIDs.clear();

		for (int unitDefID = 0; unitDefID < unitsByDefs.size(); unitDefID++) {
			const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
			//We deal with decoys later
			if (ud->decoyDef != nullptr)
				continue;

			bool createdTable = false;
			unsigned count = 1;
			PushUnitsVector(L, unitsByDefs[unitDefID], unitDefID, createdTable, count, defCount, unknownIDs);

			auto dmit = unitDefHandler->decoyMap.find(unitDefID);
			if (dmit != unitDefHandler->decoyMap.end()) {
				for (int decoyDefID: dmit->second) {
					PushUnitsVector(L, unitsByDefs[decoyDefID], unitDefID, createdTable, count, defCount, unknownIDs);
				}
			}

			if (createdTable)
				lua_rawset(L, -3);

		}

		if (!unknownIDs.empty()) {
			HSTR_PUSH(L, "unknown");
			lua_newtable(L);
			defCount++;

			unsigned count = 1;
			for (int unitID: unknownIDs) {
				lua_pushnumber(L, unitID);
				lua_rawseti(L, -2, count++);
			}
			lua_rawset(L, -3);
		}
	}

	// UnitDef ID keys are not consecutive, so add the "n"
	hs_n.PushNumber(L, defCount);
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
				lua_pushnumber(L, unitCount);
				lua_rawseti(L, -2, udID);
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

	int unknownCount = 0;
	for(CUnit* unit: team->units) {

		if (!IsUnitVisible(L, unit))
			continue;

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
		lua_pushnumber(L, mit->second);
		lua_rawseti(L, -2, mit->first);
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
	int count = 1;

	for (const int ud: defs) {
		for (const CUnit* unit: unitHandler->unitsByDefs[teamID][ud]) {
			if (allied || IsUnitTyped(L, unit)) {
				lua_pushnumber(L, unit->id);
				lua_rawseti(L, -2, count++);
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
	for (const CUnit* unit: unitHandler->unitsByDefs[teamID][unitDef->id]) {
		count += (IsUnitTyped(L, unit));
	}

	// tally the decoy units for the given unitDef
	auto dmit = unitDefHandler->decoyMap.find(unitDef->id);

	if (dmit != unitDefHandler->decoyMap.end()) {
		for (const int ud: dmit->second) {
			for (const CUnit* unit: unitHandler->unitsByDefs[teamID][ud]) {
				count += (IsUnitTyped(L, unit));
			}
		}
	}

	lua_pushnumber(L, count);
	return 1;
}


int LuaSyncedRead::GetTeamUnitCount(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);

	if (team == NULL)
		return 0;

	// use the raw team count for allies
	if (IsAlliedTeam(L, team->teamNum)) {
		lua_pushnumber(L, team->units.size());
		return 1;
	}

	// loop through the units for enemies
	int count = 0;

	for(CUnit* unit: team->units) {
		count += int(IsUnitVisible(L, unit));
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
//   L, units

#define LOOP_UNIT_CONTAINER(ALLEGIANCE_TEST, CUSTOM_TEST, NEWTABLE) \
	{                                                               \
		unsigned int count = 0;                                     \
                                                                    \
		if (NEWTABLE) {                                             \
			lua_createtable(L, units.size(), 0);                    \
		}                                                           \
                                                                    \
		for (auto it = units.cbegin(); it != units.cend(); ++it) {  \
			const CUnit* unit = *it;                                \
                                                                    \
			ALLEGIANCE_TEST;                                        \
			CUSTOM_TEST;                                            \
                                                                    \
			lua_pushnumber(L, unit->id);                            \
			lua_rawseti(L, -2, ++count);                            \
		}                                                           \
	}

// Macro Requirements:
//   unit
//   readTeam   for MY_UNIT_TEST
//   allegiance for SIMPLE_TEAM_TEST and VISIBLE_TEAM_TEST

#define NULL_TEST  ;  // always passes

#define VISIBLE_TEST \
	if (!IsUnitVisible(L, unit)) { continue; }

#define SIMPLE_TEAM_TEST \
	if (unit->team != allegiance) { continue; }

#define VISIBLE_TEAM_TEST \
	if (unit->team != allegiance) { continue; } \
	if (!IsUnitVisible(L, unit)) { continue; }

#define MY_UNIT_TEST \
	if (unit->team != readTeam) { continue; }

#define ALLY_UNIT_TEST \
	if (unit->allyteam != CLuaHandle::GetHandleReadAllyTeam(L)) { continue; }

#define ENEMY_UNIT_TEST \
	if (unit->allyteam == CLuaHandle::GetHandleReadAllyTeam(L)) { continue; } \
	if (!IsUnitVisible(L, unit)) { continue; }


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
		luaL_error(L, "Bad teamID in %s (%d)", caller, teamID);
	} else if (teamID >= teamHandler->ActiveTeams()) {
		luaL_error(L, "Bad teamID in %s (%d)", caller, teamID);
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

	const vector<CUnit*>& units = quadField->GetUnitsExact(mins, maxs);

	if (allegiance >= 0) {
		if (IsAlliedTeam(L, allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, RECTANGLE_TEST, true);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, RECTANGLE_TEST, true);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		LOOP_UNIT_CONTAINER(MY_UNIT_TEST, RECTANGLE_TEST, true);
	}
	else if (allegiance == AllyUnits) {
		LOOP_UNIT_CONTAINER(ALLY_UNIT_TEST, RECTANGLE_TEST, true);
	}
	else if (allegiance == EnemyUnits) {
		LOOP_UNIT_CONTAINER(ENEMY_UNIT_TEST, RECTANGLE_TEST, true);
	}
	else { // AllUnits
		LOOP_UNIT_CONTAINER(VISIBLE_TEST, RECTANGLE_TEST, true);
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

	const vector<CUnit*>& units = quadField->GetUnitsExact(mins, maxs);

	if (allegiance >= 0) {
		if (IsAlliedTeam(L, allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, BOX_TEST, true);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, BOX_TEST, true);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		LOOP_UNIT_CONTAINER(MY_UNIT_TEST, BOX_TEST, true);
	}
	else if (allegiance == AllyUnits) {
		LOOP_UNIT_CONTAINER(ALLY_UNIT_TEST, BOX_TEST, true);
	}
	else if (allegiance == EnemyUnits) {
		LOOP_UNIT_CONTAINER(ENEMY_UNIT_TEST, BOX_TEST, true);
	}
	else { // AllUnits
		LOOP_UNIT_CONTAINER(VISIBLE_TEST, BOX_TEST, true);
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

	const vector<CUnit*>& units = quadField->GetUnitsExact(mins, maxs);

	if (allegiance >= 0) {
		if (IsAlliedTeam(L, allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, CYLINDER_TEST, true);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, CYLINDER_TEST, true);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		LOOP_UNIT_CONTAINER(MY_UNIT_TEST, CYLINDER_TEST, true);
	}
	else if (allegiance == AllyUnits) {
		LOOP_UNIT_CONTAINER(ALLY_UNIT_TEST, CYLINDER_TEST, true);
	}
	else if (allegiance == EnemyUnits) {
		LOOP_UNIT_CONTAINER(ENEMY_UNIT_TEST, CYLINDER_TEST, true);
	}
	else { // AllUnits
		LOOP_UNIT_CONTAINER(VISIBLE_TEST, CYLINDER_TEST, true);
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

	const vector<CUnit*>& units = quadField->GetUnitsExact(mins, maxs);

	if (allegiance >= 0) {
		if (IsAlliedTeam(L, allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, SPHERE_TEST, true);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, SPHERE_TEST, true);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetHandleReadTeam(L);
		LOOP_UNIT_CONTAINER(MY_UNIT_TEST, SPHERE_TEST, true);
	}
	else if (allegiance == AllyUnits) {
		LOOP_UNIT_CONTAINER(ALLY_UNIT_TEST, SPHERE_TEST, true);
	}
	else if (allegiance == EnemyUnits) {
		LOOP_UNIT_CONTAINER(ENEMY_UNIT_TEST, SPHERE_TEST, true);
	}
	else { // AllUnits
		LOOP_UNIT_CONTAINER(VISIBLE_TEST, SPHERE_TEST, true);
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
			const int v = LuaUtils::ParseFloatArray(L, -1, values, 4);
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

#define PLANES_TEST                    \
	if (!UnitInPlanes(unit, planes)) { \
		continue;                      \
	}

	const int readTeam = CLuaHandle::GetHandleReadTeam(L);

	lua_newtable(L);

	for (int team = startTeam; team <= endTeam; team++) {
		const std::vector<CUnit*>& units = teamHandler->Team(team)->units;

		if (allegiance >= 0) {
			if (allegiance == team) {
				if (IsAlliedTeam(L, allegiance)) {
					LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST, false);
				} else {
					LOOP_UNIT_CONTAINER(VISIBLE_TEST, PLANES_TEST, false);
				}
			}
		}
		else if (allegiance == MyUnits) {
			if (readTeam == team) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST, false);
			}
		}
		else if (allegiance == AllyUnits) {
			if (CLuaHandle::GetHandleReadAllyTeam(L) == teamHandler->AllyTeam(team)) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST, false);
			}
		}
		else if (allegiance == EnemyUnits) {
			if (CLuaHandle::GetHandleReadAllyTeam(L) != teamHandler->AllyTeam(team)) {
				LOOP_UNIT_CONTAINER(VISIBLE_TEST, PLANES_TEST, false);
			}
		}
		else { // AllUnits
			if (IsAlliedTeam(L, team)) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST, false);
			} else {
				LOOP_UNIT_CONTAINER(VISIBLE_TEST, PLANES_TEST, false);
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

				lua_pushnumber(L, feature->id);
				lua_rawseti(L, -2, arrayIndex++);
			}
		}
	} else {
		for (unsigned int i = 0; i < featureCount; i++) {
			const CFeature* feature = features[i];

			if (!IsFeatureVisible(L, feature)) {
				continue;
			}

			lua_pushnumber(L, feature->id);
			lua_rawseti(L, -2, arrayIndex++);
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

	const bool excludeWeaponProjectiles = luaL_optboolean(L, 5, false);
	const bool excludePieceProjectiles = luaL_optboolean(L, 6, false);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	const vector<CProjectile*>& rectProjectiles = quadField->GetProjectilesExact(mins, maxs);
	const unsigned int rectProjectileCount = rectProjectiles.size();
	unsigned int arrayIndex = 1;

	lua_createtable(L, rectProjectileCount, 0);

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		if (CLuaHandle::GetHandleFullRead(L)) {
			for (unsigned int i = 0; i < rectProjectileCount; i++) {
				const CProjectile* pro = rectProjectiles[i];

				// filter out unsynced projectiles, the SyncedRead
				// projecile Get* functions accept only synced ID's
				// (specifically they interpret all ID's as synced)
				if (!pro->synced)
					continue;

				if (pro->weapon && excludeWeaponProjectiles)
					continue;
				if (pro->piece && excludePieceProjectiles)
					continue;

				lua_pushnumber(L, pro->id);
				lua_rawseti(L, -2, arrayIndex++);
			}
		}
	} else {
		for (unsigned int i = 0; i < rectProjectileCount; i++) {
			const CProjectile* pro = rectProjectiles[i];

			// see above
			if (!pro->synced)
				continue;

			if (pro->weapon && excludeWeaponProjectiles)
				continue;
			if (pro->piece && excludePieceProjectiles)
				continue;

			if (!IsProjectileVisible(L, pro))
				continue;

			lua_pushnumber(L, pro->id);
			lua_rawseti(L, -2, arrayIndex++);
		}
	}

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::ValidUnitID(lua_State* L)
{
	lua_pushboolean(L, lua_isnumber(L, 1) && ParseUnit(L, __FUNCTION__, 1) != nullptr);
	return 1;
}


int LuaSyncedRead::GetUnitStates(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == nullptr)
		return 0;

	lua_createtable(L, 0, 9);
	HSTR_PUSH_NUMBER(L, "firestate",  unit->fireState);
	HSTR_PUSH_NUMBER(L, "movestate",  unit->moveState);
	HSTR_PUSH_BOOL  (L, "repeat",     unit->commandAI->repeatOrders);
	HSTR_PUSH_BOOL  (L, "cloak",      unit->wantCloak);
	HSTR_PUSH_BOOL  (L, "active",     unit->activated);
	HSTR_PUSH_BOOL  (L, "trajectory", unit->useHighTrajectory);

	const CMobileCAI* mCAI = dynamic_cast<const CMobileCAI*>(unit->commandAI);
	if (mCAI != nullptr)
		HSTR_PUSH_NUMBER(L, "autorepairlevel", mCAI->repairBelowHealth);

	const AMoveType* mt = unit->moveType;
	if (mt != nullptr) {
		const CHoverAirMoveType* hAMT = dynamic_cast<const CHoverAirMoveType*>(mt);
		if (hAMT != nullptr) {
			HSTR_PUSH_BOOL  (L, "autoland",        hAMT->autoLand);
		} else {
			const CStrafeAirMoveType* sAMT = dynamic_cast<const CStrafeAirMoveType*>(mt);
			if (sAMT != nullptr) {
				HSTR_PUSH_BOOL  (L, "autoland",        sAMT->autoLand);
				HSTR_PUSH_BOOL  (L, "loopbackattack",  sAMT->loopbackAttack);
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

	if (key == "los") {
		lua_pushnumber(L, unit->losRadius);
	} else if (key == "airLos") {
		lua_pushnumber(L, unit->airLosRadius);
	} else if (key == "radar") {
		lua_pushnumber(L, unit->radarRadius);
	} else if (key == "sonar") {
		lua_pushnumber(L, unit->sonarRadius);
	} else if (key == "seismic") {
		lua_pushnumber(L, unit->seismicRadius);
	} else if (key == "radarJammer") {
		lua_pushnumber(L, unit->jammerRadius);
	} else if (key == "sonarJammer") {
		lua_pushnumber(L, unit->sonarJamRadius);
	} else {
		luaL_error(L, "Unknown sensor type to GetUnitSensorRadius()");
	}

	return 1;
}

int LuaSyncedRead::GetUnitPosErrorParams(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	lua_pushnumber(L, unit->posErrorVector.x);
	lua_pushnumber(L, unit->posErrorVector.y);
	lua_pushnumber(L, unit->posErrorVector.z);
	lua_pushnumber(L, unit->posErrorDelta.x);
	lua_pushnumber(L, unit->posErrorDelta.y);
	lua_pushnumber(L, unit->posErrorDelta.z);
	lua_pushnumber(L, unit->nextPosErrorUpdate);
	return (3 + 3 + 1);
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

		if (unitTeam != NULL && unitTeam->HasLeader()) {
			tooltip = playerHandler->Player(unitTeam->GetLeader())->name;
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
	lua_pushnumber(L, unit->resourcesMake.metal);
	lua_pushnumber(L, unit->resourcesUse.metal);
	lua_pushnumber(L, unit->resourcesMake.energy);
	lua_pushnumber(L, unit->resourcesUse.energy);
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

int LuaSyncedRead::GetUnitMass(lua_State* L)
{
	return (GetSolidObjectMass(L, ParseUnit(L, __FUNCTION__, 1)));
}

int LuaSyncedRead::GetUnitPosition(lua_State* L)
{
	return (GetSolidObjectPosition(L, ParseUnit(L, __FUNCTION__, 1), false));
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


int LuaSyncedRead::GetUnitRotation(lua_State* L)
{
	return (GetSolidObjectRotation(L, ParseInLosUnit(L, __FUNCTION__, 1)));
}

int LuaSyncedRead::GetUnitDirection(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

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
	return (GetWorldObjectVelocity(L, ParseInLosUnit(L, __FUNCTION__, 1)));
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


int LuaSyncedRead::GetUnitHarvestStorage(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
		lua_pushnumber(L, unit->harvested[i]);
		lua_pushnumber(L, unit->harvestStorage[i]);
	}
	return 2 * SResourcePack::MAX_RESOURCES;
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

		lua_pushnumber(L, modelPieceNum + 1); //lua 1-indexed, c++ 0-indexed
		lua_rawseti(L, -2, p + 1);
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
	if (unit == nullptr || !unit->unitDef->IsTransportUnit()) {
		return 0;
	}

	lua_newtable(L);

	int count = 1;
	for (const CUnit::TransportedUnit& tu: unit->transportedUnits) {
		const CUnit* carried = tu.unit;
		lua_pushnumber(L, carried->id);
		lua_rawseti(L, -2, count++);
	}

	return 1;
}


int LuaSyncedRead::GetUnitShieldState(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const CPlasmaRepulser* shield = NULL;
	const size_t idx = luaL_optint(L, 2, -1) - LUA_WEAPON_BASE_INDEX;

	if (idx >= unit->weapons.size()) {
		shield = static_cast<const CPlasmaRepulser*>(unit->shieldWeapon);
	} else {
		shield = dynamic_cast<const CPlasmaRepulser*>(unit->weapons[idx]);
	}

	if (shield == NULL) {
		return 0;
	}

	lua_pushnumber(L, shield->IsEnabled());
	lua_pushnumber(L, shield->GetCurPower());
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


int LuaSyncedRead::GetUnitMaxRange(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->maxRange);
	return 1;
}


int LuaSyncedRead::GetUnitWeaponState(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

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
		lua_pushnumber(L, weapon->AccuracyExperience());
	}
	else if (key == "sprayAngle") {
		lua_pushnumber(L, weapon->SprayAngleExperience());
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
	else if (key == "salvoError") {
		float3 salvoError =  weapon->SalvoErrorExperience();
		lua_createtable(L, 3, 0);
		lua_pushnumber(L, salvoError.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, salvoError.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, salvoError.z); lua_rawseti(L, -2, 3);
	}
	else if (key == "targetMoveError") {
		lua_pushnumber(L, weapon->MoveErrorExperience());
	}
	else {
		return 0;
	}
	return 1;
}


static inline int PushDamagesKey(lua_State* L, const DynDamageArray& damages, int index)
{
	if (lua_isnumber(L, index)) {
		const unsigned armType = lua_toint(L, index);
		if (armType >= damages.GetNumTypes())
			return 0;

		lua_pushnumber(L, damages.Get(armType));
		return 1;
	}

	const std::string key = luaL_checkstring(L, index);

	if (key == "paralyzeDamageTime") {
		lua_pushnumber(L, damages.paralyzeDamageTime);
	} else if (key == "impulseFactor") {
		lua_pushnumber(L, damages.impulseFactor);
	} else if (key == "impulseBoost") {
		lua_pushnumber(L, damages.impulseBoost);
	} else if (key == "craterMult") {
		lua_pushnumber(L, damages.craterMult);
	} else if (key == "craterBoost") {
		lua_pushnumber(L, damages.craterBoost);
	} else if (key == "dynDamageExp") {
		lua_pushnumber(L, damages.dynDamageExp);
	} else if (key == "dynDamageMin") {
		lua_pushnumber(L, damages.dynDamageMin);
	} else if (key == "dynDamageRange") {
		lua_pushnumber(L, damages.dynDamageRange);
	} else if (key == "dynDamageInverted") {
		lua_pushboolean(L, damages.dynDamageInverted);
	} else if (key == "craterAreaOfEffect") {
		lua_pushnumber(L, damages.craterAreaOfEffect);
	} else if (key == "damageAreaOfEffect") {
		lua_pushnumber(L, damages.damageAreaOfEffect);
	} else if (key == "edgeEffectiveness") {
		lua_pushnumber(L, damages.edgeEffectiveness);
	} else if (key == "explosionSpeed") {
		lua_pushnumber(L, damages.explosionSpeed);
	} else {
		return 0;
	}
	return 1;
}


int LuaSyncedRead::GetUnitWeaponDamages(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == nullptr)
		return 0;

	const DynDamageArray* damages;

	if (lua_israwstring(L, 2)) {
		const string key = lua_tostring(L, 2);
		if (key == "explode") {
			damages = unit->deathExpDamages;
		} else if (key == "selfDestruct") {
			damages = unit->selfdExpDamages;
		} else {
			return 0;
		}
	} else {
		const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

		if (weaponNum >= unit->weapons.size())
			return 0;

		CWeapon* weapon = unit->weapons[weaponNum];

		damages = weapon->damages;
	}

	if (damages == nullptr)
		return 0;

	return PushDamagesKey(L, *damages, 3);
}


int LuaSyncedRead::GetUnitWeaponVectors(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size()) {
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
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = NULL;

	float3 pos;

	//we cannot test calling TryTarget/TestTarget/HaveFreeLineOfFire directly
	// by passing a position thatis not approximately the wanted checked position, because
	//the checks for target using passed position for checking both free line of fire and range
	//which would result in wrong test unless target was by chance near coords <0,0,0>
	//while position alone works because NULL target omits target class validity checks

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);

	} else {
		enemy = ParseUnit(L, __FUNCTION__, 3);

		if (enemy == NULL)
			return 0;
	}

	lua_pushboolean(L, weapon->TryTarget(SWeaponTarget(enemy, pos, true)));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponTestTarget(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = NULL;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		if ((enemy = ParseUnit(L, __FUNCTION__, 3)) == NULL)
			return 0;

		pos = weapon->GetUnitLeadTargetPos(enemy);
	}

	lua_pushboolean(L, weapon->TestTarget(pos, SWeaponTarget(enemy, pos, true)));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponTestRange(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = NULL;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		if ((enemy = ParseUnit(L, __FUNCTION__, 3)) == NULL)
			return 0;

		pos = weapon->GetUnitLeadTargetPos(enemy);
	}

	lua_pushboolean(L, weapon->TestRange(pos, SWeaponTarget(enemy, pos, true)));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponHaveFreeLineOfFire(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = NULL;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		if ((enemy = ParseUnit(L, __FUNCTION__, 3)) == NULL)
			return 0;

		pos = weapon->GetUnitLeadTargetPos(enemy);
	}

	lua_pushboolean(L, weapon->HaveFreeLineOfFire(pos, SWeaponTarget(enemy, pos, true)));
	return 1;
}

int LuaSyncedRead::GetUnitWeaponCanFire(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const bool ignoreAngleGood = luaL_optboolean(L, 3, false);
	const bool ignoreTargetType = luaL_optboolean(L, 4, false);
	const bool ignoreRequestedDir = luaL_optboolean(L, 5, false);

	lua_pushboolean(L, unit->weapons[weaponNum]->CanFire(ignoreAngleGood, ignoreTargetType, ignoreRequestedDir));
	return 1;
}

int LuaSyncedRead::GetUnitWeaponTarget(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	auto curTarget = weapon->GetCurrentTarget();

	lua_pushnumber(L, curTarget.type);

	switch (curTarget.type) {
		case Target_None:
			return 1;
			break;
		case Target_Unit: {
			lua_pushboolean(L, curTarget.isUserTarget);
			lua_pushnumber(L, curTarget.unit->id);
			break;
		}
		case Target_Pos: {
			lua_pushboolean(L, curTarget.isUserTarget);
			lua_createtable(L, 3, 0);
			lua_pushnumber(L, curTarget.groundPos.x); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, curTarget.groundPos.y); lua_rawseti(L, -2, 2);
			lua_pushnumber(L, curTarget.groundPos.z); lua_rawseti(L, -2, 3);
			break;
		}
		case Target_Intercept: {
			lua_pushboolean(L, curTarget.isUserTarget);
			lua_pushnumber(L, curTarget.intercept->id);
			break;
		}
	}

	return 3;
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


int LuaSyncedRead::GetUnitFuel(lua_State* L) { lua_pushnumber(L, 0.0f); return 1; } // DEPRECATED


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

	return (LuaPathFinder::PushPathNodes(L, gmt->GetPathID()));
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
	return (GetSolidObjectLastHitPiece(L, ParseAllyUnit(L, __FUNCTION__, 1)));
}

int LuaSyncedRead::GetUnitCollisionVolumeData(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	return (PushCollisionVolumeData(L, &unit->collisionVolume));
}

int LuaSyncedRead::GetUnitPieceCollisionVolumeData(lua_State* L)
{
	return (PushPieceCollisionVolumeData(L, ParseInLosUnit(L, __FUNCTION__, 1)));
}


int LuaSyncedRead::GetUnitSeparation(lua_State* L)
{
	const CUnit* unit1 = ParseUnit(L, __FUNCTION__, 1);
	const CUnit* unit2 = ParseUnit(L, __FUNCTION__, 2);

	if (unit1 == nullptr || unit2 == nullptr)
		return 0;

	float3 pos1 = unit1->midPos;
	float3 pos2 = unit2->midPos;

	if (!IsAllyUnit(L, unit1))
		pos1 = unit1->GetLuaErrorPos(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));
	if (!IsAllyUnit(L, unit2))
		pos2 = unit2->GetLuaErrorPos(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));

	#if 0
	const float3 mask = XZVector + UpVector * (1 - luaL_optboolean(L, 3, false));
	const float3 diff = (pos1 - pos2) * mask;
	const float  dist = diff.Length();
	#else
	const float dist = (luaL_optboolean(L, 3, false))? pos1.distance2D(pos2): pos1.distance(pos2);
	#endif

	if (luaL_optboolean(L, 4, false)) {
		lua_pushnumber(L, std::max(0.0f, dist - unit1->radius - unit2->radius));
	} else {
		lua_pushnumber(L, dist);
	}

	return 1;
}

int LuaSyncedRead::GetUnitFeatureSeparation(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == nullptr)
		return 0;

	const CFeature* feature = ParseFeature(L, __FUNCTION__, 2);

	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	float3 pos1 =    unit->midPos;
	float3 pos2 = feature->midPos;

	if (!IsAllyUnit(L, unit))
		pos1 = unit->GetLuaErrorPos(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));

	#if 0
	const float3 mask = XZVector + UpVector * (1 - luaL_optboolean(L, 3, false));
	const float3 diff = (pos1 - pos2) * mask;

	lua_pushnumber(L, diff.Length());
	#else
	lua_pushnumber(L, (luaL_optboolean(L, 3, false))? pos1.distance2D(pos2): pos1.distance(pos2));
	#endif
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


int LuaSyncedRead::GetUnitBlocking(lua_State* L)
{
	return (GetSolidObjectBlocking(L, ParseTypedUnit(L, __FUNCTION__, 1)));
}


int LuaSyncedRead::GetUnitMoveTypeData(lua_State* L)
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

	switch (amt->progressState) {
		case AMoveType::Done:
			HSTR_PUSH_STRING(L, "progressState", "done");
			break;
		case AMoveType::Active:
			HSTR_PUSH_STRING(L, "progressState", "active");
			break;
		case AMoveType::Failed:
			HSTR_PUSH_STRING(L, "progressState", "failed");
			break;
	}

	const CGroundMoveType* groundmt = dynamic_cast<CGroundMoveType*>(unit->moveType);

	if (groundmt != NULL) {
		HSTR_PUSH_STRING(L, "name", "ground");

		HSTR_PUSH_NUMBER(L, "turnRate", groundmt->GetTurnRate());
		HSTR_PUSH_NUMBER(L, "accRate", groundmt->GetAccRate());
		HSTR_PUSH_NUMBER(L, "decRate", groundmt->GetDecRate());

		HSTR_PUSH_NUMBER(L, "maxReverseSpeed", groundmt->GetMaxReverseSpeed() * GAME_SPEED);
		HSTR_PUSH_NUMBER(L, "wantedSpeed", groundmt->GetWantedSpeed() * GAME_SPEED);
		HSTR_PUSH_NUMBER(L, "currentSpeed", groundmt->GetCurrentSpeed() * GAME_SPEED);

		HSTR_PUSH_NUMBER(L, "goalRadius", groundmt->GetGoalRadius());

		HSTR_PUSH_NUMBER(L, "currwaypointx", (groundmt->GetCurrWayPoint()).x);
		HSTR_PUSH_NUMBER(L, "currwaypointy", (groundmt->GetCurrWayPoint()).y);
		HSTR_PUSH_NUMBER(L, "currwaypointz", (groundmt->GetCurrWayPoint()).z);
		HSTR_PUSH_NUMBER(L, "nextwaypointx", (groundmt->GetNextWayPoint()).x);
		HSTR_PUSH_NUMBER(L, "nextwaypointy", (groundmt->GetNextWayPoint()).y);
		HSTR_PUSH_NUMBER(L, "nextwaypointz", (groundmt->GetNextWayPoint()).z);

		HSTR_PUSH_NUMBER(L, "requestedSpeed", 0.0f);

		HSTR_PUSH_NUMBER(L, "pathFailures", 0);

		return 1;
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

		HSTR_PUSH_NUMBER(L, "brakeDistance", -1.0f); // DEPRECATED
		HSTR_PUSH_BOOL(L, "dontLand", hAMT->GetAllowLanding());
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

		HSTR_PUSH_NUMBER(L, "maxAcc", sAMT->accRate);
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

	// t["params"] = {[1] = param1, ...}
	LuaUtils::PushCommandParamsTable(L, cmd, true);
	// t["options"] = {key1 = val1, ...}
	LuaUtils::PushCommandOptionsTable(L, cmd, true);

	HSTR_PUSH_NUMBER(L, "tag", cmd.tag);
}


static void PackCommandQueue(lua_State* L, const CCommandQueue& commands, size_t count)
{
	lua_createtable(L, commands.size(), 0);

	size_t c = 0;

	// get the desired number of commands to return
	if (count == -1u) {
		count = commands.size();
	}

	// {[1] = cq[0], [2] = cq[1], ...}
	for (auto ci = commands.begin(); ci != commands.end(); ++ci) {
		if (c >= count)
			break;

		PackCommand(L, *ci);
		lua_rawseti(L, -2, ++c);
	}
}


int LuaSyncedRead::GetUnitCommands(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;

	if (commandAI == NULL)
		return 0;

	// send the new unit commands for factories, otherwise the normal commands
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	const CCommandQueue* queue = (factoryCAI == NULL) ? &commandAI->commandQue : &factoryCAI->newUnitCommands;

	// performance relevant `debug` message
	if (lua_isnoneornil(L, 2)) {
		static int calls = 0;
		static spring_time nextWarning = spring_gettime();
		calls++;
		if (spring_gettime() >= nextWarning) { //spring_gettime() isn't sync-safe!
			nextWarning = spring_gettime() + spring_secs(5);
			if (calls > 1000) {
				LOG_L(L_ERROR,
					"[%s] called too often without a 2nd argument to define maxNumCmds returned in the table, please check your code!\n"
					"Especially when you only read the first cmd or want to check if the queue is non-empty, this can be a huge performance leak!\n", __FUNCTION__);
			}
			calls = 0;
		}
	}

	const int  numCmds   = luaL_optint(L, 2, -1);
	const bool cmdsTable = luaL_optboolean(L, 3, true); // deprecated, prefer to set 2nd arg to 0

	if (cmdsTable && (numCmds != 0)) {
		// *get wants the actual commands
		PackCommandQueue(L, *queue, numCmds);
	} else {
		// *get just wants the queue's size
		lua_pushnumber(L, queue->size());
	}

	return 1;
}

int LuaSyncedRead::GetFactoryCommands(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);

	// bail if not a factory
	if (factoryCAI == NULL)
		return 0;

	const CCommandQueue& commandQue = factoryCAI->commandQue;

	const int  numCmds   = luaL_optint(L, 2, -1);
	const bool cmdsTable = luaL_optboolean(L, 3, true); // deprecated, prefer to set 2nd arg to 0

	if (cmdsTable && (numCmds != 0)) {
		PackCommandQueue(L, commandQue, numCmds);
	} else {
		lua_pushnumber(L, commandQue.size());
	}

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
			lua_newtable(L); {
				lua_pushnumber(L, currentCount);
				lua_rawseti(L, -2, -currentCmd);
			}
			lua_rawseti(L, -2, entry);
			currentCmd = cmdID;
			currentCount = 1;
		}
	}
	if (currentCount > 0) {
		entry++;
		lua_newtable(L); {
			lua_pushnumber(L, currentCount);
			lua_rawseti(L, -2, -currentCmd);
		}
		lua_rawseti(L, -2, entry);
	}

	hs_n.PushNumber(L, entry);
}


int LuaSyncedRead::GetFactoryCounts(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	if (!factoryCAI) {
		return 0; // not a factory, bail
	}
	const CCommandQueue& commandQue = factoryCAI->commandQue;

	// get the desired number of commands to return
	int count = luaL_optint(L, 2, -1);
	if (count < 0) {
		count = (int)commandQue.size();
	}

	const bool noCmds = !luaL_optboolean(L, 3, false);

	PackFactoryCounts(L, commandQue, count, noCmds);

	return 1;
}


int LuaSyncedRead::GetCommandQueue(lua_State* L)
{
	return (GetUnitCommands(L));
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
				lua_newtable(L);
				lua_pushnumber(L, currentCount);
				lua_rawseti(L, -2, currentType);
				lua_rawseti(L, -2, entry);
				currentType = unitDefID;
				currentCount = 1;
			}
		}
	}

	if (currentCount > 0) {
		entry++;
		lua_newtable(L);
		lua_pushnumber(L, currentCount);
		lua_rawseti(L, -2, currentType);
		lua_rawseti(L, -2, entry);
	}

	lua_pushnumber(L, entry);

	return 2;
}


int LuaSyncedRead::GetFullBuildQueue(lua_State* L)
{
	return PackBuildQueue(L, false, __FUNCTION__);
}


int LuaSyncedRead::GetRealBuildQueue(lua_State* L)
{
	return PackBuildQueue(L, true, __FUNCTION__);
}


/******************************************************************************/

int LuaSyncedRead::GetUnitRulesParams(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL || game == NULL) {
		return 0;
	}

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (IsAllyUnit(L, unit) || game->IsGameOver()) {
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

	return PushRulesParams(L, __FUNCTION__, params, losMask);
}


int LuaSyncedRead::GetUnitRulesParam(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL || game == NULL) {
		return 0;
	}

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (IsAllyUnit(L, unit) || game->IsGameOver()) {
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

	return GetRulesParam(L, __FUNCTION__, 2, params, losMask);
}


/******************************************************************************/

int LuaSyncedRead::GetUnitCmdDescs(lua_State* L)
{
	CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const std::vector<const SCommandDescription*>& cmdDescs = unit->commandAI->GetPossibleCommands();
	const int lastDesc = (int)cmdDescs.size() - 1;

	const int args = lua_gettop(L); // number of arguments
	int startIndex = 0;
	int endIndex = lastDesc;
	if ((args >= 2) && lua_isnumber(L, 2)) {
		startIndex = lua_toint(L, 2) - 1;
		if ((args >= 3) && lua_isnumber(L, 3)) {
			endIndex = lua_toint(L, 3) - 1;
		} else {
			endIndex = startIndex;
		}
	}
	startIndex = Clamp(startIndex, 0, lastDesc);
	endIndex   = Clamp(endIndex  , 0, lastDesc);

	lua_createtable(L, endIndex - startIndex, 0);
	int count = 1;
	for (int i = startIndex; i <= endIndex; i++) {
		LuaUtils::PushCommandDesc(L, *cmdDescs[i]);
		lua_rawseti(L, -2, count++);
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

	const std::vector<const SCommandDescription*>& cmdDescs = unit->commandAI->GetPossibleCommands();
	for (int i = 0; i < (int)cmdDescs.size(); i++) {
		if (cmdDescs[i]->id == cmdID) {
			lua_pushnumber(L, i + 1);
			return 1;
		}
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::ValidFeatureID(lua_State* L)
{
	lua_pushboolean(L, ParseFeature(L, __FUNCTION__, 1) != nullptr);
	return 1;
}


int LuaSyncedRead::GetAllFeatures(lua_State* L)
{
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

int LuaSyncedRead::GetFeatureMass(lua_State* L)
{
	return (GetSolidObjectMass(L, ParseFeature(L, __FUNCTION__, 1)));
}

int LuaSyncedRead::GetFeaturePosition(lua_State* L)
{
	return (GetSolidObjectPosition(L, ParseFeature(L, __FUNCTION__, 1), true));
}

int LuaSyncedRead::GetFeatureSeparation(lua_State* L)
{
	CFeature* feature1 = ParseFeature(L, __FUNCTION__, 1);
	if (feature1 == NULL  || !IsFeatureVisible(L, feature1)) {
		return 0;
	}
	CFeature* feature2 = ParseFeature(L, __FUNCTION__, 2);
	if (feature2 == NULL  || !IsFeatureVisible(L, feature2)) {
		return 0;
	}

	float3 pos1 = feature1->pos;
	float3 pos2 = feature2->pos;

	float dist;
	if (lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		dist = pos1.distance2D(pos2);
	} else {
		dist = pos1.distance(pos2);
	}
	lua_pushnumber(L, dist);

	return 1;
}

int LuaSyncedRead::GetFeatureRotation(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL || !IsFeatureVisible(L, feature)) {
		return 0;
	}
	return GetSolidObjectRotation(L, feature);
}

int LuaSyncedRead::GetFeatureDirection(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == NULL || !IsFeatureVisible(L, feature))
		return 0;

	const CMatrix44f& mat = feature->GetTransformMatrixRef(true);
	const float3& dir = mat.GetZ();

	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 3;
}

int LuaSyncedRead::GetFeatureVelocity(lua_State* L)
{
	return (GetWorldObjectVelocity(L, ParseFeature(L, __FUNCTION__, 1)));
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
	lua_pushnumber(L,  feature->resources.metal);
	lua_pushnumber(L,  feature->def->metal);
	lua_pushnumber(L,  feature->resources.energy);
	lua_pushnumber(L,  feature->def->energy);
	lua_pushnumber(L,  feature->reclaimLeft);
	return 5;
}


int LuaSyncedRead::GetFeatureBlocking(lua_State* L)
{
	return (GetSolidObjectBlocking(L, ParseFeature(L, __FUNCTION__, 1)));
}


int LuaSyncedRead::GetFeatureNoSelect(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == NULL || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushboolean(L, feature->noSelect);
	return 1;
}


int LuaSyncedRead::GetFeatureResurrect(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == NULL)
		return 0;

	if (feature->udef == NULL) {
		lua_pushliteral(L, "");
	} else {
		lua_pushsstring(L, feature->udef->name);
	}

	lua_pushnumber(L, feature->buildFacing);
	return 2;
}


int LuaSyncedRead::GetFeatureLastAttackedPiece(lua_State* L)
{
	return (GetSolidObjectLastHitPiece(L, ParseFeature(L, __FUNCTION__, 1)));
}

int LuaSyncedRead::GetFeatureCollisionVolumeData(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == nullptr)
		return 0;

	return (PushCollisionVolumeData(L, &feature->collisionVolume));
}

int LuaSyncedRead::GetFeaturePieceCollisionVolumeData(lua_State* L)
{
	return (PushPieceCollisionVolumeData(L, ParseFeature(L, __FUNCTION__, 1)));
}


int LuaSyncedRead::GetFeatureRulesParams(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (IsAlliedAllyTeam(L, feature->allyteam) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler->AlliedTeams(feature->team, CLuaHandle::GetHandleReadTeam(L)) || ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && CLuaHandle::GetHandleFullRead(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		//! NoAccessTeam
	}
	else if (IsFeatureVisible(L, feature)) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	}

	const LuaRulesParams::Params&  params    = feature->modParams;

	return PushRulesParams(L, __FUNCTION__, params, losMask);
}


int LuaSyncedRead::GetFeatureRulesParam(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (IsAlliedAllyTeam(L, feature->allyteam) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler->AlliedTeams(feature->team, CLuaHandle::GetHandleReadTeam(L)) || ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && CLuaHandle::GetHandleFullRead(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		//! NoAccessTeam
	}
	else if (IsFeatureVisible(L, feature)) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	}

	const LuaRulesParams::Params&  params    = feature->modParams;

	return GetRulesParam(L, __FUNCTION__, 2, params, losMask);
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

int LuaSyncedRead::GetProjectileDirection(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	lua_pushnumber(L, pro->dir.x);
	lua_pushnumber(L, pro->dir.y);
	lua_pushnumber(L, pro->dir.z);
	return 3;
}

int LuaSyncedRead::GetProjectileVelocity(lua_State* L)
{
	return (GetWorldObjectVelocity(L, ParseProjectile(L, __FUNCTION__, 1)));
}


int LuaSyncedRead::GetProjectileGravity(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	lua_pushnumber(L, pro->mygravity);
	return 1;
}

int LuaSyncedRead::GetProjectileSpinAngle(lua_State* L) { lua_pushnumber(L, 0.0f); return 1; } // DEPRECATED
int LuaSyncedRead::GetProjectileSpinSpeed(lua_State* L) { lua_pushnumber(L, 0.0f); return 1; } // DEPRECATED
int LuaSyncedRead::GetProjectileSpinVec(lua_State* L) {
	lua_pushnumber(L, 0.0f);
	lua_pushnumber(L, 0.0f);
	lua_pushnumber(L, 0.0f);
	return 3;
} // DEPRECATED

int LuaSyncedRead::GetPieceProjectileParams(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL || !pro->piece)
		return 0;

	const CPieceProjectile* ppro = static_cast<const CPieceProjectile*>(pro);

	lua_pushnumber(L, ppro->explFlags);
	lua_pushnumber(L, ppro->spinAngle);
	lua_pushnumber(L, ppro->spinSpeed);
	lua_pushnumber(L, ppro->spinVec.x);
	lua_pushnumber(L, ppro->spinVec.y);
	lua_pushnumber(L, ppro->spinVec.z);
	return (1 + 1 + 1 + 3);
}

int LuaSyncedRead::GetProjectileTarget(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const CWorldObject* wtgt = wpro->GetTargetObject();

	if (wtgt == NULL) {
		lua_pushnumber(L, int('g')); // ground
		lua_createtable(L, 3, 0);
		lua_pushnumber(L, (wpro->GetTargetPos()).x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, (wpro->GetTargetPos()).y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, (wpro->GetTargetPos()).z); lua_rawseti(L, -2, 3);
		return 2;
	}

	if (dynamic_cast<const CUnit*>(wtgt) != NULL) {
		lua_pushnumber(L, int('u'));
		lua_pushnumber(L, wtgt->id);
		return 2;
	}
	if (dynamic_cast<const CFeature*>(wtgt) != NULL) {
		lua_pushnumber(L, int('f'));
		lua_pushnumber(L, wtgt->id);
		return 2;
	}
	if (dynamic_cast<const CWeaponProjectile*>(wtgt) != NULL) {
		lua_pushnumber(L, int('p'));
		lua_pushnumber(L, wtgt->id);
		return 2;
	}

	// projectile target cannot be anything else
	assert(false);
	return 0;
}

int LuaSyncedRead::GetProjectileIsIntercepted(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);

	lua_pushboolean(L, wpro->IsBeingIntercepted());
	return 1;
}

int LuaSyncedRead::GetProjectileTimeToLive(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);

	lua_pushnumber(L, wpro->GetTimeToLive());
	return 1;
}

int LuaSyncedRead::GetProjectileOwnerID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	const int unitID = pro->GetOwnerID();
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= unitHandler->MaxUnits()))
		return 0;

	lua_pushnumber(L, unitID);
	return 1;
}

int LuaSyncedRead::GetProjectileTeamID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;

	if (!teamHandler->IsValidTeam(pro->GetTeamID()))
		return 0;

	lua_pushnumber(L, pro->GetTeamID());
	return 1;
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

int LuaSyncedRead::GetProjectileDefID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == NULL)
		return 0;
	if (!pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const WeaponDef* wdef = wpro->GetWeaponDef();

	if (wdef == NULL)
		return 0;

	lua_pushnumber(L, wdef->id);
	return 1;
}

int LuaSyncedRead::GetProjectileDamages(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);

	if (pro == nullptr)
		return 0;

	if (!pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const std::string key = luaL_checkstring(L, 2);

	return PushDamagesKey(L, *wpro->damages, 2);
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
	lua_pushnumber(L, CGround::GetHeightReal(x, z, CLuaHandle::GetHandleSynced(L)));
	return 1;
}


int LuaSyncedRead::GetGroundOrigHeight(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	lua_pushnumber(L, CGround::GetOrigHeight(x, z));
	return 1;
}


int LuaSyncedRead::GetGroundNormal(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	// raw or smoothed center normal
	const float3& normal = luaL_optboolean(L, 3, false)?
		CGround::GetNormal(x, z, CLuaHandle::GetHandleSynced(L)):
		CGround::GetSmoothNormal(x, z, CLuaHandle::GetHandleSynced(L));

	lua_pushnumber(L, normal.x);
	lua_pushnumber(L, normal.y);
	lua_pushnumber(L, normal.z);
	// slope derives from face normals, include it here
	lua_pushnumber(L, CGround::GetSlope(x, z, CLuaHandle::GetHandleSynced(L)));
	return 4;
}


int LuaSyncedRead::GetGroundInfo(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	const int ix = Clamp(x, 0.0f, float3::maxxpos) / (SQUARE_SIZE * 2);
	const int iz = Clamp(z, 0.0f, float3::maxzpos) / (SQUARE_SIZE * 2);

	const int maxIndex = (mapDims.hmapx * mapDims.hmapy) - 1;
	const int sqrIndex = std::min(maxIndex, (mapDims.hmapx * iz) + ix);
	const int typeIndex = readMap->GetTypeMapSynced()[sqrIndex];

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
	tx1 = Clamp((int)(fx1 / SQUARE_SIZE), 0, mapDims.mapxm1);
	tx2 = Clamp((int)(fx2 / SQUARE_SIZE), 0, mapDims.mapxm1);
	tz1 = Clamp((int)(fz1 / SQUARE_SIZE), 0, mapDims.mapym1);
	tz2 = Clamp((int)(fz2 / SQUARE_SIZE), 0, mapDims.mapym1);

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
	lua_pushnumber(L, readMap->GetInitMinHeight());
	lua_pushnumber(L, readMap->GetInitMaxHeight());
	lua_pushnumber(L, readMap->GetCurrMinHeight());
	lua_pushnumber(L, readMap->GetCurrMaxHeight());
	return 4;
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

int LuaSyncedRead::GetSmoothMeshHeight(lua_State* L)
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
	const float3 dir(luaL_optfloat(L, 5, 0.0f), luaL_optfloat(L, 6, 0.0f), luaL_optfloat(L, 7, 0.0f));

	const bool testTerrain = luaL_optboolean(L, 8, true);
	const bool testObjects = luaL_optboolean(L, 9, true);
	const bool centerOnly = luaL_optboolean(L, 10, false);

	bool los = false;
	bool ret = false;

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		los = CLuaHandle::GetHandleFullRead(L);
	} else {
		los = losHandler->InLos(pos, CLuaHandle::GetHandleReadAllyTeam(L));
	}

	if (los) {
		ret = moveDef->TestMoveSquare(NULL, pos, dir, testTerrain, testObjects, centerOnly);
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
		const int rat = CLuaHandle::GetHandleReadAllyTeam(L);
		return rat;
	}

	const int aat = luaL_optint(L, arg, CEventClient::MinSpecialTeam - 1);
	if (CLuaHandle::GetHandleFullRead(L)) {
		if (teamHandler->IsValidAllyTeam(aat))
			return aat;

		if (aat == CEventClient::AllAccessTeam || aat == CEventClient::NoAccessTeam)
			return aat;
	} else {
		if (aat == CLuaHandle::GetHandleReadAllyTeam(L))
			return aat;
	}

	// never returns
	return (luaL_argerror(L, arg, "Invalid allyTeam"));
}


int LuaSyncedRead::GetPositionLosState(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);
	if (allyTeamID < 0) {
		const bool fullView = (allyTeamID == CEventClient::AllAccessTeam);
		lua_pushboolean(L, fullView);
		lua_pushboolean(L, fullView);
		lua_pushboolean(L, fullView);
		lua_pushboolean(L, fullView);
		return 4;
	}

	const bool inLos    = losHandler->InLos(pos, allyTeamID);
	const bool inRadar  = losHandler->InRadar(pos, allyTeamID);
	const bool inJammer = losHandler->InJammer(pos, allyTeamID);

	lua_pushboolean(L, inLos || inRadar);
	lua_pushboolean(L, inLos);
	lua_pushboolean(L, inRadar);
	lua_pushboolean(L, inJammer);
	return 4;
}


int LuaSyncedRead::IsPosInLos(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InLos(pos, allyTeamID));
	return 1;
}


int LuaSyncedRead::IsPosInRadar(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InRadar(pos, allyTeamID));
	return 1;
}


int LuaSyncedRead::IsPosInAirLos(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InAirLos(pos, allyTeamID));
	return 1;
}


int LuaSyncedRead::GetUnitLosState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	unsigned short losStatus;
	if (allyTeamID < 0) {
		losStatus = (allyTeamID == CEventClient::AllAccessTeam) ? LOS_ALL_MASK_BITS : 0;
	} else {
		losStatus = unit->losStatus[allyTeamID];
	}

	if (CLuaHandle::GetHandleFullRead(L) && luaL_optboolean(L, 3, false)) {
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


int LuaSyncedRead::IsUnitInLos(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InLos(unit, allyTeamID));
	return 1;
}


int LuaSyncedRead::IsUnitInAirLos(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InAirLos(unit, allyTeamID));
	return 1;
}


int LuaSyncedRead::IsUnitInRadar(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	if (allyTeamID < 0) {
		lua_pushboolean(L, (allyTeamID == CEventClient::AllAccessTeam));
		return 1;
	}

	lua_pushboolean(L, losHandler->InRadar(unit, allyTeamID));
	return 1;
}


int LuaSyncedRead::IsUnitInJammer(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __FUNCTION__, 1);
	if (unit == NULL)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	if (allyTeamID < 0) {
		luaL_argerror(L, 2, "Invalid allyTeam");
		return 0;
	}

	lua_pushboolean(L, losHandler->InJammer(unit, allyTeamID)); //FIXME
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
	//const int mx = (int)max(0 , min(mapDims.mapxm1, (int)(x / SQUARE_SIZE)));
	//const int mz = (int)max(0 , min(mapDims.mapym1, (int)(z / SQUARE_SIZE)));
	return 0;
}


/******************************************************************************/
/******************************************************************************/

static int GetSolidObjectPieceMap(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModel& localModel = o->localModel;

	lua_createtable(L, 0, localModel.pieces.size());

	// {"piece" = 123, ...}
	for (size_t i = 0; i < localModel.pieces.size(); i++) {
		const LocalModelPiece& lp = localModel.pieces[i];
		lua_pushsstring(L, lp.original->name);
		lua_pushnumber(L, i + 1);
		lua_rawset(L, -3);
	}

	return 1;
}

static int GetSolidObjectPieceList(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModel& localModel = o->localModel;

	lua_createtable(L, localModel.pieces.size(), 0);

	// {[1] = "piece", ...}
	for (size_t i = 0; i < localModel.pieces.size(); i++) {
		const LocalModelPiece& lp = localModel.pieces[i];
		lua_pushsstring(L, lp.original->name);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}



static int GetSolidObjectPieceInfoHelper(lua_State* L, const S3DModelPiece& op)
{
	lua_newtable(L);
	HSTR_PUSH_STRING(L, "name", op.name.c_str());
	HSTR_PUSH_STRING(L, "parent", ((op.parent != nullptr)? op.parent->name.c_str(): "[null]"));

	HSTR_PUSH(L, "children");
	lua_newtable(L);
	for (size_t c = 0; c < op.children.size(); c++) {
		lua_pushsstring(L, op.children[c]->name);
		lua_rawseti(L, -2, c + 1);
	}
	lua_rawset(L, -3);

	HSTR_PUSH(L, "isEmpty");
	lua_pushboolean(L, !op.HasGeometryData());
	lua_rawset(L, -3);

	HSTR_PUSH(L, "min");
	lua_newtable(L); {
		lua_pushnumber(L, op.mins.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, op.mins.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, op.mins.z); lua_rawseti(L, -2, 3);
	}
	lua_rawset(L, -3);

	HSTR_PUSH(L, "max");
	lua_newtable(L); {
		lua_pushnumber(L, op.maxs.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, op.maxs.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, op.maxs.z); lua_rawseti(L, -2, 3);
	}
	lua_rawset(L, -3);

	HSTR_PUSH(L, "offset");
	lua_newtable(L); {
		lua_pushnumber(L, op.offset.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, op.offset.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, op.offset.z); lua_rawseti(L, -2, 3);
	}
	lua_rawset(L, -3);
	return 1;
}

static int GetSolidObjectPieceInfo(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	return (::GetSolidObjectPieceInfoHelper(L, *(lmp->original)));
}

static int GetSolidObjectPiecePosition(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	const float3 pos = lmp->GetAbsolutePos();

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}

static int GetSolidObjectPieceDirection(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	const float3 dir = lmp->GetDirection();

	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 3;
}

static int GetSolidObjectPiecePosDir(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	float3 dir;
	float3 pos;
	lmp->GetEmitDirPos(pos, dir);

	// transform to object's space
	pos = o->GetObjectSpacePos(pos);
	dir = o->GetObjectSpaceVec(dir);

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 6;
}

static int GetSolidObjectPieceMatrix(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	const CMatrix44f& mat = lmp->GetModelSpaceMatrix();

	for (int m = 0; m < 16; m++) {
		lua_pushnumber(L, mat.m[m]);
	}

	return 16;
}



int LuaSyncedRead::GetUnitPieceMap(lua_State* L) {
	return (GetSolidObjectPieceMap(L, ParseTypedUnit(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetUnitPieceList(lua_State* L) {
	return (GetSolidObjectPieceList(L, ParseTypedUnit(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetUnitPieceInfo(lua_State* L) {
	return (GetSolidObjectPieceInfo(L, ParseTypedUnit(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetUnitPiecePosDir(lua_State* L) {
	return (GetSolidObjectPiecePosDir(L, ParseTypedUnit(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetUnitPiecePosition(lua_State* L) {
	return (GetSolidObjectPiecePosition(L, ParseTypedUnit(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetUnitPieceDirection(lua_State* L) {
	return (GetSolidObjectPieceDirection(L, ParseTypedUnit(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetUnitPieceMatrix(lua_State* L) {
	return (GetSolidObjectPieceMatrix(L, ParseTypedUnit(L, __FUNCTION__, 1)));
}


int LuaSyncedRead::GetFeaturePieceMap(lua_State* L) {
	return (GetSolidObjectPieceMap(L, ParseFeature(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetFeaturePieceList(lua_State* L) {
	return (GetSolidObjectPieceList(L, ParseFeature(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetFeaturePieceInfo(lua_State* L) {
	return (GetSolidObjectPieceInfo(L, ParseFeature(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetFeaturePiecePosDir(lua_State* L) {
	return (GetSolidObjectPiecePosDir(L, ParseFeature(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetFeaturePiecePosition(lua_State* L) {
	return (GetSolidObjectPiecePosition(L, ParseFeature(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetFeaturePieceDirection(lua_State* L) {
	return (GetSolidObjectPieceDirection(L, ParseFeature(L, __FUNCTION__, 1)));
}
int LuaSyncedRead::GetFeaturePieceMatrix(lua_State* L) {
	return (GetSolidObjectPieceMatrix(L, ParseFeature(L, __FUNCTION__, 1)));
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
				lua_pushnumber(L, piece + 1);
				lua_rawseti(L, -2, sp);
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


int LuaSyncedRead::GetRadarErrorParams(lua_State* L)
{
	const int allyTeamID = lua_tonumber(L, 1);

	if (!teamHandler->IsValidAllyTeam(allyTeamID))
		return 0;

	lua_pushnumber(L, losHandler->GetAllyTeamRadarErrorSize(allyTeamID));
	lua_pushnumber(L, losHandler->GetBaseRadarErrorSize());
	lua_pushnumber(L, losHandler->GetBaseRadarErrorMult());
	return 3;
}


/******************************************************************************/
/******************************************************************************/
