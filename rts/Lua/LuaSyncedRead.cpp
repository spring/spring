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
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/GrassDrawer.h"
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
#include "Sim/Projectiles/ExplosionGenerator.h"
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
#include "Sim/Units/UnitToolTipMap.hpp"
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
#include "System/SpringMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/StringUtil.h"

#include <cctype>


using std::min;
using std::max;

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

	// READ routines, sync safe
	REGISTER_LUA_CFUNC(IsCheatingEnabled);
	REGISTER_LUA_CFUNC(IsGodModeEnabled);
	REGISTER_LUA_CFUNC(IsDevLuaEnabled);
	REGISTER_LUA_CFUNC(IsEditDefsEnabled);
	REGISTER_LUA_CFUNC(IsNoCostEnabled);
	REGISTER_LUA_CFUNC(GetGlobalLos);
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

	REGISTER_LUA_CFUNC(GetTidal);
	REGISTER_LUA_CFUNC(GetWind);

	REGISTER_LUA_CFUNC(GetHeadingFromVector);
	REGISTER_LUA_CFUNC(GetVectorFromHeading);

	REGISTER_LUA_CFUNC(GetSideData);

	REGISTER_LUA_CFUNC(GetAllyTeamStartBox);
	REGISTER_LUA_CFUNC(GetTeamStartPosition);
	REGISTER_LUA_CFUNC(GetMapStartPositions);

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
	REGISTER_LUA_CFUNC(GetUnitCurrentCommand);
	REGISTER_LUA_CFUNC(GetFactoryCounts);
	REGISTER_LUA_CFUNC(GetFactoryCommands);

	REGISTER_LUA_CFUNC(GetCommandQueue);
	REGISTER_LUA_CFUNC(GetFullBuildQueue);
	REGISTER_LUA_CFUNC(GetRealBuildQueue);

	REGISTER_LUA_CFUNC(GetUnitCmdDescs);
	REGISTER_LUA_CFUNC(FindUnitCmdDesc);

	REGISTER_LUA_CFUNC(GetUnitRulesParam);
	REGISTER_LUA_CFUNC(GetUnitRulesParams);

	REGISTER_LUA_CFUNC(GetCEGID);

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
	REGISTER_LUA_CFUNC(GetGrass);

	REGISTER_LUA_CFUNC(GetSmoothMeshHeight);

	REGISTER_LUA_CFUNC(TestMoveOrder);
	REGISTER_LUA_CFUNC(TestBuildOrder);
	REGISTER_LUA_CFUNC(Pos2BuildPos);
	REGISTER_LUA_CFUNC(ClosestBuildPos);

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

	return (teamHandler.AllyTeam(team) == CLuaHandle::GetHandleReadAllyTeam(L));
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

	// currently in LOS or not lost from radar since being visible means unit's type can be accessed
	return ((losStatus & LOS_INLOS) || ((losStatus & prevMask) == prevMask));
}


static inline const UnitDef* EffectiveUnitDef(lua_State* L, const CUnit* unit)
{
	const UnitDef* ud = unit->unitDef;

	if (IsAllyUnit(L, unit))
		return ud;

	if (ud->decoyDef)
		return ud->decoyDef;

	return ud;
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

	return !((CLuaHandle::GetHandleReadAllyTeam(L) != pro->GetAllyteamID()) &&
	        (!losHandler->InLos(pro->pos, CLuaHandle::GetHandleReadAllyTeam(L))));
}



#if 0
static inline bool IsPlayerSynced(lua_State* L, const CPlayer* player)
{
	const bool syncedHandle = CLuaHandle::GetHandleSynced(L);
	const bool onlyFromDemo = syncedHandle && gameSetup->hostDemo;
	return (!onlyFromDemo || player->isFromDemo);
}
#endif

static inline bool IsPlayerUnsynced(lua_State* L, const CPlayer* player)
{
	const bool syncedHandle = CLuaHandle::GetHandleSynced(L);
	const bool onlyFromDemo = syncedHandle && gameSetup->hostDemo;

	return (onlyFromDemo && !player->isFromDemo);
}



/******************************************************************************/
/******************************************************************************/

static int GetSolidObjectLastHitPiece(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;
	if (o->hitModelPieces[true] == nullptr)
		return 0;

	const LocalModelPiece* lmp = o->hitModelPieces[true];
	const S3DModelPiece* omp = lmp->original;

	if (lua_isboolean(L, 1) && lua_toboolean(L, 1)) {
		lua_pushnumber(L, lmp->GetLModelPieceIndex() + 1);
	} else {
		lua_pushsstring(L, omp->name);
	}

	lua_pushnumber(L, o->pieceHitFrames[true]);
	return 2;
}

static int PushPieceCollisionVolumeData(lua_State* L, const CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(L, o, 2);

	if (lmp == nullptr)
		return 0;

	return LuaUtils::PushColVolData(L, lmp->GetCollisionVolume());
}


static int PushTerrainTypeData(lua_State* L, const CMapInfo::TerrainType* tt, bool groundInfo) {
	lua_pushinteger(L, tt - &mapInfo->terrainTypes[0]); // index
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
	return (8 + groundInfo);
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

	return (unitHandler.GetUnit(lua_toint(L, index)));
}

static inline const CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	const CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	// include the vistest for LuaUnsyncedRead
	if (!IsUnitVisible(L, unit))
		return nullptr;

	return unit;
}

static inline const CUnit* ParseAllyUnit(lua_State* L, const char* caller, int index)
{
	const CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!IsAllyUnit(L, unit))
		return nullptr;

	return unit;
}

static inline const CUnit* ParseInLosUnit(lua_State* L, const char* caller, int index)
{
	const CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!IsUnitInLos(L, unit))
		return nullptr;

	return unit;
}


static inline const CUnit* ParseTypedUnit(lua_State* L, const char* caller, int index)
{
	const CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!IsUnitTyped(L, unit))
		return nullptr;

	return unit;
}


static const CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "[%s] featureID (arg #%d) not a number\n", caller, index);
		return nullptr;
	}

	const CFeature* feature = featureHandler.GetFeature(lua_toint(L, index));

	if (feature == nullptr)
		return nullptr;

	// include the vistest for LuaUnsyncedRead
	if (!IsFeatureVisible(L, feature))
		return nullptr;

	return feature;
}


static const CProjectile* ParseProjectile(lua_State* L, const char* caller, int index)
{
	const CProjectile* p = projectileHandler.GetProjectileBySyncedID(luaL_checkint(L, index));

	if (p == nullptr)
		return nullptr;

	if (!IsProjectileVisible(L, p))
		return nullptr;

	return p;
}


static inline const CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	const int teamID = luaL_checkint(L, index);

	if (!teamHandler.IsValidTeam(teamID))
		luaL_error(L, "Bad teamID in %s\n", caller);

	return teamHandler.Team(teamID);
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
	const std::string& key = luaL_checkstring(L, index);
	const auto it = params.find(key);
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
	lua_pushboolean(L, gs->godMode != 0);
	lua_pushboolean(L, (gs->godMode & GODMODE_ATC_BIT) != 0);
	lua_pushboolean(L, (gs->godMode & GODMODE_ETC_BIT) != 0);
	return 3;
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

int LuaSyncedRead::IsNoCostEnabled(lua_State* L)
{
	lua_pushboolean(L, unitDefHandler->GetNoCost());
	return 1;
}

int LuaSyncedRead::GetGlobalLos(lua_State* L)
{
	const int allyTeam = luaL_optint(L, 1, CLuaHandle::GetHandleReadAllyTeam(L));
	if (!teamHandler.IsValidAllyTeam(allyTeam))
		return 0;

	lua_pushboolean(L, losHandler->globalLOS[allyTeam]);
	return 1;
}


int LuaSyncedRead::AreHelperAIsEnabled(lua_State* L)
{
	lua_pushboolean(L, !gs->noHelperAIs);
	return 1;
}


int LuaSyncedRead::FixedAllies(lua_State* L)
{
	lua_pushboolean(L, gameSetup->fixedAllies);
	return 1;
}


int LuaSyncedRead::IsGameOver(lua_State* L)
{
	if (game == nullptr)
		return 0;

	lua_pushboolean(L, game->IsGameOver());
	return 1;
}


int LuaSyncedRead::GetGaiaTeamID(lua_State* L)
{
	if (!gs->useLuaGaia)
		return 0;

	lua_pushnumber(L, teamHandler.GaiaTeamID());
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


int LuaSyncedRead::GetTidal(lua_State* L)
{
	lua_pushnumber(L, envResHandler.GetCurrentTidalStrength());
	return 1;
}

int LuaSyncedRead::GetWind(lua_State* L)
{
	lua_pushnumber(L, envResHandler.GetCurrentWindVec().x);
	lua_pushnumber(L, envResHandler.GetCurrentWindVec().y);
	lua_pushnumber(L, envResHandler.GetCurrentWindVec().z);
	lua_pushnumber(L, envResHandler.GetCurrentWindStrength());
	lua_pushnumber(L, envResHandler.GetCurrentWindDir().x);
	lua_pushnumber(L, envResHandler.GetCurrentWindDir().y);
	lua_pushnumber(L, envResHandler.GetCurrentWindDir().z);
	return 7;
}


/******************************************************************************/

int LuaSyncedRead::GetGameRulesParams(lua_State* L)
{
	// always readable for all
	return PushRulesParams(L, __func__, CSplitLuaHandle::GetGameParams(), LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK);
}


int LuaSyncedRead::GetGameRulesParam(lua_State* L)
{
	// always readable for all
	return GetRulesParam(L, __func__, 1, CSplitLuaHandle::GetGameParams(), LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK);
}


/******************************************************************************/

int LuaSyncedRead::GetMapOptions(lua_State* L)
{
	const auto& mapOpts = CGameSetup::GetMapOptions();

	lua_createtable(L, 0, mapOpts.size());

	for (const auto& mapOpt : mapOpts) {
		lua_pushsstring(L, mapOpt.first);
		lua_pushsstring(L, mapOpt.second);
		lua_rawset(L, -3);
	}

	return 1;
}

int LuaSyncedRead::GetModOptions(lua_State* L)
{
	const auto& modOpts = CGameSetup::GetModOptions();

	lua_createtable(L, 0, modOpts.size());

	for (const auto& modOpt: modOpts) {
		lua_pushsstring(L, modOpt.first);
		lua_pushsstring(L, modOpt.second);
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
		if (startUnit.empty())
			return 0;

		lua_pushsstring(L, startUnit);
		lua_pushsstring(L, caseName);
		return 2;
	}
	if (lua_israwnumber(L, 1)) {
		const unsigned int index = lua_toint(L, 1) - 1;
		if (!sideParser.ValidSide(index))
			return 0;

		lua_pushsstring(L, sideParser.GetSideName(index));
		lua_pushsstring(L, sideParser.GetStartUnit(index));
		lua_pushsstring(L, sideParser.GetCaseName(index));
		return 3;
	}
	{
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
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
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


int LuaSyncedRead::GetMapStartPositions(lua_State* L)
{
	lua_createtable(L, MAX_TEAMS, 0);
	gameSetup->LoadStartPositionsFromMap(MAX_TEAMS, [&](MapParser& mapParser, int teamNum) {
		float3 pos;

		if (!mapParser.GetStartPos(teamNum, pos))
			return false;

		lua_createtable(L, 3, 0);
		lua_pushnumber(L, pos.x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, pos.y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, pos.z); lua_rawseti(L, -2, 3);
		lua_rawseti(L, -2, 1 + teamNum); // [i] = {x,y,z}
		return true;
	});

	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetAllyTeamList(lua_State* L)
{
	lua_createtable(L, teamHandler.ActiveAllyTeams(), 0);

	unsigned int allyCount = 1;

	for (int at = 0; at < teamHandler.ActiveAllyTeams(); at++) {
		lua_pushnumber(L, at);
		lua_rawseti(L, -2, allyCount++);
	}

	return 1;
}


int LuaSyncedRead::GetTeamList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments

	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1)))
		luaL_error(L, "Incorrect arguments to GetTeamList([allyTeamID])");


	int allyTeamID = -1;

	if (args == 1) {
		allyTeamID = lua_toint(L, 1);
		if (!teamHandler.IsValidAllyTeam(allyTeamID))
			return 0;
	}

	lua_createtable(L, teamHandler.ActiveTeams(), 0);

	unsigned int teamCount = 1;

	for (int t = 0; t < teamHandler.ActiveTeams(); t++) {
		if (teamHandler.Team(t) == nullptr)
			continue;

		if ((allyTeamID >= 0) && (allyTeamID != teamHandler.AllyTeam(t)))
			continue;

		lua_pushnumber(L, t);
		lua_rawseti(L, -2, teamCount++);
	}

	return 1;
}


int LuaSyncedRead::GetPlayerList(lua_State* L)
{
	int teamID = -1;
	bool active = false;

	if (lua_isnumber(L, 1)) {
		teamID = lua_toint(L, 1);
		active = lua_isboolean(L, 2)? lua_toboolean(L, 2): active;
	}
	else if (lua_isboolean(L, 1)) {
		active = lua_toboolean(L, 1);
		teamID = lua_isnumber(L, 2)? lua_toint(L, 2): teamID;
	}

	if (teamID >= teamHandler.ActiveTeams())
		return 0;

	lua_createtable(L, playerHandler.ActivePlayers(), 0);

	for (unsigned int p = 0, playerCount = 1; p < playerHandler.ActivePlayers(); p++) {
		const CPlayer* player = playerHandler.Player(p);

		if (player == nullptr)
			continue;

		if (IsPlayerUnsynced(L, player))
			continue;

		if (active && !player->active)
			continue;

		if (teamID >= 0) {
			// exclude specs for normal team ID's
			if (player->spectator)
				continue;
			if (player->team != teamID)
				continue;
		}

		lua_pushnumber(L, p);
		lua_rawseti(L, -2, playerCount++);
	}

	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetTeamInfo(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	const CTeam* team = teamHandler.Team(teamID);
	if (team == nullptr)
		return 0;

	// read before modifying stack
	const bool getTeamOpts = luaL_optboolean(L, 2, true);

	lua_pushnumber(L,  team->teamNum);
	lua_pushnumber(L,  team->GetLeader());
	lua_pushboolean(L, team->isDead);
	lua_pushboolean(L, skirmishAIHandler.HasSkirmishAIsInTeam(teamID));
	lua_pushstring(L, team->GetSideName());
	lua_pushnumber(L,  teamHandler.AllyTeam(team->teamNum));
	lua_pushnumber(L, team->GetIncomeMultiplier());

	if (getTeamOpts) {
		const TeamBase::customOpts& teamOpts(team->GetAllValues());

		lua_createtable(L, 0, teamOpts.size());

		for (const auto& pair: teamOpts) {
			lua_pushsstring(L, pair.first);
			lua_pushsstring(L, pair.second);
			lua_rawset(L, -3);
		}
	}

	return 7 + getTeamOpts;
}


int LuaSyncedRead::GetTeamResources(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID))
		return 0;

	switch (luaL_checkstring(L, 2)[0]) {
		case 'm': {
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
		} break;
		case 'e': {
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
		} break;
		default: {
		} break;
	}

	return 0;
}


int LuaSyncedRead::GetTeamUnitStats(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr || game == nullptr)
		return 0;

	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID) && !game->IsGameOver())
		return 0;

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
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr || game == nullptr)
		return 0;

	const int teamID = team->teamNum;

	if (!IsAlliedTeam(L, teamID) && !game->IsGameOver())
		return 0;

	const TeamStatistics& stats = team->GetCurrentStats();

	switch (luaL_checkstring(L, 2)[0]) {
		case 'm': {
			lua_pushnumber(L, stats.metalUsed);
			lua_pushnumber(L, stats.metalProduced);
			lua_pushnumber(L, stats.metalExcess);
			lua_pushnumber(L, stats.metalReceived);
			lua_pushnumber(L, stats.metalSent);
			return 5;
		} break;
		case 'e': {
			lua_pushnumber(L, stats.energyUsed);
			lua_pushnumber(L, stats.energyProduced);
			lua_pushnumber(L, stats.energyExcess);
			lua_pushnumber(L, stats.energyReceived);
			lua_pushnumber(L, stats.energySent);
			return 5;
		} break;
		default: {
		} break;
	}

	return 0;
}


int LuaSyncedRead::GetTeamRulesParams(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr || game == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC;

	if (IsAlliedTeam(L, team->teamNum) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler.AlliedTeams(team->teamNum, CLuaHandle::GetHandleReadTeam(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}

	return PushRulesParams(L, __func__, team->modParams, losMask);
}


int LuaSyncedRead::GetTeamRulesParam(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr || game == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC;

	if (IsAlliedTeam(L, team->teamNum) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler.AlliedTeams(team->teamNum, CLuaHandle::GetHandleReadTeam(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}

	return GetRulesParam(L, __func__, 2, team->modParams, losMask);
}


int LuaSyncedRead::GetTeamStatsHistory(lua_State* L)
{
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr || game == nullptr)
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
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr)
		return 0;

	const std::string* luaAIName = nullptr;
	const std::vector<uint8_t>& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(team->teamNum);

	for (uint8_t id: teamAIs) {
		const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(id);

		if (!aiData->isLuaAI)
			continue;

		luaAIName = &aiData->shortName;
		break;
	}

	if (luaAIName == nullptr)
		return 0;

	lua_pushsstring(L, *luaAIName);
	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetPlayerInfo(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler.IsValidPlayer(playerID))
		return 0;

	const CPlayer* player = playerHandler.Player(playerID);
	if (player == nullptr)
		return 0;

	if (IsPlayerUnsynced(L, player))
		return 0;

	// read before modifying stack
	const bool getPlayerOpts = luaL_optboolean(L, 2, true);

	lua_pushsstring(L, player->name);
	lua_pushboolean(L, player->active);
	lua_pushboolean(L, player->spectator);
	lua_pushnumber(L, player->team);
	lua_pushnumber(L, teamHandler.AllyTeam(player->team));
	lua_pushnumber(L, player->ping * 0.001f); // in seconds
	lua_pushnumber(L, player->cpuUsage);
	lua_pushsstring(L, player->countryCode);
	lua_pushnumber(L, player->rank);
	// same as select(4, GetTeamInfo(teamID=player->team))
	lua_pushboolean(L, skirmishAIHandler.HasSkirmishAIsInTeam(player->team));

	if (getPlayerOpts) {
		const PlayerBase::customOpts& playerOpts = player->GetAllValues();

		lua_createtable(L, 0, playerOpts.size());

		for (const auto& pair: playerOpts) {
			lua_pushsstring(L, pair.first);
			lua_pushsstring(L, pair.second);
			lua_rawset(L, -3);
		}
	}

	return 10 + getPlayerOpts;
}


int LuaSyncedRead::GetPlayerControlledUnit(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler.IsValidPlayer(playerID))
		return 0;

	const CPlayer* player = playerHandler.Player(playerID);
	if (player == nullptr)
		return 0;

	if (IsPlayerUnsynced(L, player))
		return 0;


	const FPSUnitController& con = player->fpsController;
	const CUnit* unit = con.GetControllee();

	if (unit == nullptr)
		return 0;

	if ((CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam) ||
	    ((CLuaHandle::GetHandleReadAllyTeam(L) >= 0) && !teamHandler.Ally(unit->allyteam, CLuaHandle::GetHandleReadAllyTeam(L)))) {
		return 0;
	}

	lua_pushnumber(L, unit->id);
	return 1;
}

int LuaSyncedRead::GetAIInfo(lua_State* L)
{
	int numVals = 0;

	const int teamId = luaL_checkint(L, 1);
	if (!teamHandler.IsValidTeam(teamId))
		return numVals;

	const std::vector<uint8_t>& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(teamId);
	if (teamAIs.empty())
		return numVals;

	const size_t skirmishAIId    = teamAIs[0];
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

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
	} else if (skirmishAIHandler.IsLocalSkirmishAI(skirmishAIId)) {
		lua_pushsstring(L, aiData->shortName);
		lua_pushsstring(L, aiData->version);

		lua_newtable(L);

		for (const auto& option: aiData->options) {
			lua_pushsstring(L, option.first);
			lua_pushsstring(L, option.second);
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
	if (!teamHandler.ValidAllyTeam(allyteam))
		return 0;

	const AllyTeam& ally = teamHandler.GetAllyTeam(allyteam);
	const AllyTeam::customOpts& allyTeamOpts = ally.GetAllValues();

	lua_createtable(L, 0, allyTeamOpts.size());

	for (const auto& pair: allyTeamOpts) {
		lua_pushsstring(L, pair.first);
		lua_pushsstring(L, pair.second);
		lua_rawset(L, -3);
	}
	return 1;
}

int LuaSyncedRead::AreTeamsAllied(lua_State* L)
{
	const int teamId1 = (int)luaL_checkint(L, -1);
	const int teamId2 = (int)luaL_checkint(L, -2);

	if (!teamHandler.IsValidTeam(teamId1) || !teamHandler.IsValidTeam(teamId2))
		return 0;

	lua_pushboolean(L, teamHandler.AlliedTeams(teamId1, teamId2));
	return 1;
}


int LuaSyncedRead::ArePlayersAllied(lua_State* L)
{
	const int player1 = luaL_checkint(L, -1);
	const int player2 = luaL_checkint(L, -2);

	if (!playerHandler.IsValidPlayer(player1) || !playerHandler.IsValidPlayer(player2))
		return 0;

	const CPlayer* p1 = playerHandler.Player(player1);
	const CPlayer* p2 = playerHandler.Player(player2);

	if ((p1 == nullptr) || (p2 == nullptr))
		return 0;

	if ((IsPlayerUnsynced(L, p1)) || (IsPlayerUnsynced(L, p2)))
		return 0;

	lua_pushboolean(L, teamHandler.AlliedTeams(p1->team, p2->team));
	return 1;
}


/******************************************************************************/
/******************************************************************************/
//
//  Grouped Unit Queries
//

int LuaSyncedRead::GetAllUnits(lua_State* L)
{
	lua_createtable(L, (unitHandler.GetActiveUnits()).size(), 0);

	unsigned int unitCount = 1;
	if (CLuaHandle::GetHandleFullRead(L)) {
		for (const CUnit* unit: unitHandler.GetActiveUnits()) {
			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, unitCount++);
		}
	} else {
		for (const CUnit* unit: unitHandler.GetActiveUnits()) {
			if (!IsUnitVisible(L, unit))
				continue;

			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, unitCount++);
		}
	}

	return 1;
}


int LuaSyncedRead::GetTeamUnits(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	unsigned int unitCount = 1;

	// raw push for allies
	if (IsAlliedTeam(L, teamID)) {
		lua_createtable(L, unitHandler.NumUnitsByTeam(teamID), 0);

		for (const CUnit* unit: unitHandler.GetUnitsByTeam(teamID)) {
			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, unitCount++);
		}

		return 1;
	}

	// check visibility for enemies
	lua_createtable(L, unitHandler.NumUnitsByTeam(teamID), 0);

	for (const CUnit* unit: unitHandler.GetUnitsByTeam(teamID)) {
		if (!IsUnitVisible(L, unit))
			continue;
		lua_pushnumber(L, unit->id);
		lua_rawseti(L, -2, unitCount++);
	}

	return 1;
}



// used by GetTeamUnitsSorted (PushVisibleUnits) and GetTeamUnitsByDefs (InsertSearchUnitDefs)
static std::vector<int> gtuObjectIDs;
// used by GetTeamUnitsCounts
static std::vector< std::pair<int, int> > gtuDefCounts;

static bool PushVisibleUnits(
	lua_State* L,
	const std::vector<CUnit*>& defUnits,
	int unitDefID,
	unsigned int* unitCount,
	unsigned int* defCount
) {
	bool createdTable = false;

	for (const CUnit* unit: defUnits) {
		if (!IsUnitVisible(L, unit))
			continue;

		if (!IsUnitTyped(L, unit)) {
			gtuObjectIDs.push_back(unit->id);
			continue;
		}

		// push new table for first unit of type <unitDefID> to be visible
		if (!createdTable) {
			createdTable = true;

			lua_pushnumber(L, unitDefID);
			lua_createtable(L, defUnits.size(), 0);

			(*defCount)++;
		}

		// add count-th unitID to table
		lua_pushnumber(L, unit->id);
		lua_rawseti(L, -2, (*unitCount)++);
	}

	return createdTable;
}

static inline void InsertSearchUnitDefs(const UnitDef* ud, bool allied)
{
	if (ud == nullptr)
		return;

	if (allied) {
		gtuObjectIDs.push_back(ud->id);
		return;
	}
	if (ud->decoyDef != nullptr)
		return;

	gtuObjectIDs.push_back(ud->id);

	// spring::unordered_map<int, std::vector<int> >
	const auto& decoyMap = unitDefHandler->GetDecoyDefIDs();
	const auto decoyMapIt = decoyMap.find(ud->id);

	if (decoyMapIt == decoyMap.end())
		return;

	for (int decoyDefID: decoyMapIt->second) {
		gtuObjectIDs.push_back(decoyDefID);
	}
}



int LuaSyncedRead::GetTeamUnitsSorted(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	unsigned int defCount = 0;
	unsigned int unitCount = 1;

	// table = {[unitDefID] = {[1] = unitID, [2] = unitID, ...}}
	lua_createtable(L, unitDefHandler->NumUnitDefs(), 0);

	if (IsAlliedTeam(L, teamID)) {
		// tally for allies
		for (unsigned int i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
			const auto& unitsByDef = unitHandler.GetUnitsByTeamAndDef(teamID, i + 1);

			if (unitsByDef.empty())
				continue;

			lua_pushnumber(L, i + 1);
			lua_createtable(L, unitsByDef.size(), 0);
			defCount++;

			for (const CUnit* unit: unitsByDef) {
				lua_pushnumber(L, unit->id);
				lua_rawseti(L, -2, unitCount++);
			}
			lua_rawset(L, -3);
		}
	} else {
		// tally for enemies
		gtuObjectIDs.clear();
		gtuObjectIDs.reserve(16);

		for (unsigned int i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
			const unsigned int unitDefID = i + 1;

			const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);

			// we deal with decoys later
			if (ud->decoyDef != nullptr)
				continue;

			bool createdTable = PushVisibleUnits(L, unitHandler.GetUnitsByTeamAndDef(teamID, unitDefID), unitDefID, &unitCount, &defCount);

			// for all decoy-defs of unitDefID, add decoy units under the same ID
			const auto& decoyMap = unitDefHandler->GetDecoyDefIDs();
			const auto decoyMapIt = decoyMap.find(unitDefID);

			if (decoyMapIt != decoyMap.end()) {
				for (int decoyDefID: decoyMapIt->second) {
					createdTable |= PushVisibleUnits(L, unitHandler.GetUnitsByTeamAndDef(teamID, decoyDefID), unitDefID, &unitCount, &defCount);
				}
			}

			if (createdTable)
				lua_rawset(L, -3);

		}

		if (!gtuObjectIDs.empty()) {
			HSTR_PUSH(L, "unknown");

			defCount += 1;
			unitCount = 1;

			lua_createtable(L, gtuObjectIDs.size(), 0);

			for (int unitID: gtuObjectIDs) {
				lua_pushnumber(L, unitID);
				lua_rawseti(L, -2, unitCount++);
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
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	unsigned int unknownCount = 0;
	unsigned int defCount = 0;

	// send the raw unitsByDefs counts for allies
	if (IsAlliedTeam(L, teamID)) {
		lua_createtable(L, unitDefHandler->NumUnitDefs(), 0);

		for (unsigned int i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
			const unsigned int unitDefID = i + 1;
			const unsigned int unitCount = unitHandler.NumUnitsByTeamAndDef(teamID, unitDefID);

			if (unitCount == 0)
				continue;

			lua_pushnumber(L, unitCount);
			lua_rawseti(L, -2, unitDefID);
			defCount++;
		}

		// keys are not necessarily consecutive here due to
		// the unitCount check, so add the "n" key manually
		hs_n.PushNumber(L, defCount);
		return 1;
	}

	// tally the counts for enemies
	gtuDefCounts.clear();
	gtuDefCounts.resize(unitDefHandler->NumUnitDefs() + 1, {0, 0});

	for (const CUnit* unit: unitHandler.GetUnitsByTeam(teamID)) {
		if (!IsUnitVisible(L, unit))
			continue;

		if (!IsUnitTyped(L, unit)) {
			unknownCount++;
		} else {
			const UnitDef* unitDef = EffectiveUnitDef(L, unit);

			gtuDefCounts[unitDef->id].first = unitDef->id;
			gtuDefCounts[unitDef->id].second += 1;
		}
	}

	// push the counts
	lua_createtable(L, 0, gtuDefCounts.size());

	for (const auto& gtuDefCount: gtuDefCounts) {
		if (gtuDefCount.second == 0)
			continue;
		lua_pushnumber(L, gtuDefCount.second);
		lua_rawseti(L, -2, gtuDefCount.first);
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


int LuaSyncedRead::GetTeamUnitsByDefs(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;
	const bool allied = IsAlliedTeam(L, teamID);

	// parse the unitDefs
	gtuObjectIDs.clear();
	gtuObjectIDs.reserve(16);

	if (lua_isnumber(L, 2)) {
		InsertSearchUnitDefs(unitDefHandler->GetUnitDefByID(lua_toint(L, 2)), allied);
	} else if (lua_istable(L, 2)) {
		const int tableIdx = 2;

		for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
			if (!lua_isnumber(L, -1))
				continue;

			InsertSearchUnitDefs(unitDefHandler->GetUnitDefByID(lua_toint(L, -1)), allied);
		}
	} else {
		luaL_error(L, "Incorrect arguments to GetTeamUnitsByDefs()");
	}

	// sort the ID's so duplicates can be skipped
	std::stable_sort(gtuObjectIDs.begin(), gtuObjectIDs.end());

	lua_createtable(L, gtuObjectIDs.size(), 0);

	unsigned int unitCount = 1;
	unsigned int prevUnitDefID = -1;

	for (const int unitDefID: gtuObjectIDs) {
		if (unitDefID == prevUnitDefID)
			continue;

		prevUnitDefID = unitDefID;

		for (const CUnit* unit: unitHandler.GetUnitsByTeamAndDef(teamID, unitDefID)) {
			if (!allied && !IsUnitTyped(L, unit))
				continue;

			lua_pushnumber(L, unit->id);
			lua_rawseti(L, -2, unitCount++);
		}
	}

	return 1;
}


int LuaSyncedRead::GetTeamUnitDefCount(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	const int teamID = team->teamNum;

	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(luaL_checkint(L, 2));

	if (unitDef == nullptr)
		luaL_error(L, "Bad unitDefID in GetTeamUnitDefCount()");

	// use the unitsByDefs count for allies
	if (IsAlliedTeam(L, teamID)) {
		lua_pushnumber(L, unitHandler.NumUnitsByTeamAndDef(teamID, unitDef->id));
		return 1;
	}

	// you can never count enemy decoys
	if (unitDef->decoyDef != nullptr) {
		lua_pushnumber(L, 0);
		return 1;
	}

	unsigned int unitCount = 0;

	// tally the given unitDef units
	for (const CUnit* unit: unitHandler.GetUnitsByTeamAndDef(teamID, unitDef->id)) {
		unitCount += (IsUnitTyped(L, unit));
	}

	// tally the decoy units for the given unitDef
	const auto& decoyMap = unitDefHandler->GetDecoyDefIDs();
	const auto decoyMapIt = decoyMap.find(unitDef->id);

	if (decoyMapIt != decoyMap.end()) {
		for (const int udID: decoyMapIt->second) {
			for (const CUnit* unit: unitHandler.GetUnitsByTeamAndDef(teamID, udID)) {
				unitCount += (IsUnitTyped(L, unit));
			}
		}
	}

	lua_pushnumber(L, unitCount);
	return 1;
}


int LuaSyncedRead::GetTeamUnitCount(lua_State* L)
{
	if (CLuaHandle::GetHandleReadAllyTeam(L) == CEventClient::NoAccessTeam)
		return 0;

	// parse the team
	const CTeam* team = ParseTeam(L, __func__, 1);

	if (team == nullptr)
		return 0;

	// use the raw team count for allies
	if (IsAlliedTeam(L, team->teamNum)) {
		lua_pushnumber(L, unitHandler.NumUnitsByTeam(team->teamNum));
		return 1;
	}

	// loop through the units for enemies
	unsigned int unitCount = 0;

	for (const CUnit* unit: unitHandler.GetUnitsByTeam(team->teamNum)) {
		unitCount += int(IsUnitVisible(L, unit));
	}

	lua_pushnumber(L, unitCount);
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
		if (NEWTABLE)                                               \
			lua_createtable(L, units.size(), 0);                    \
                                                                    \
		for (const CUnit* unit: units) {                            \
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
	if (!lua_isnumber(L, index))
		return AllUnits;

	const int teamID = lua_toint(L, index);

	// MyUnits, AllyUnits, and EnemyUnits do not apply to fullRead
	if (CLuaHandle::GetHandleFullRead(L) && (teamID < 0))
		return AllUnits;

	if (teamID < EnemyUnits) {
		luaL_error(L, "Bad teamID in %s (%d)", caller, teamID);
	} else if (teamID >= teamHandler.ActiveTeams()) {
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

	const int allegiance = ParseAllegiance(L, __func__, 5);

#define RECTANGLE_TEST ; // no test, GetUnitsExact is sufficient

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);

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

	const int allegiance = ParseAllegiance(L, __func__, 7);

#define BOX_TEST                  \
	const float y = unit->midPos.y; \
	if ((y < ymin) || (y > ymax)) { \
		continue;                     \
	}

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);

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

	const int allegiance = ParseAllegiance(L, __func__, 4);

#define CYLINDER_TEST                         \
	const float3& p = unit->midPos;             \
	const float dx = (p.x - x);                 \
	const float dz = (p.z - z);                 \
	const float dist = ((dx * dx) + (dz * dz)); \
	if (dist > radSqr) {                        \
		continue;                               \
	}                                           \

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);

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

	const int allegiance = ParseAllegiance(L, __func__, 5);

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

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);

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
	for (const Plane& p: planes) {
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

	const int allegiance = ParseAllegiance(L, __func__, 2);
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
		endTeam = teamHandler.ActiveTeams() - 1;
	}

#define PLANES_TEST                    \
	if (!UnitInPlanes(unit, planes)) { \
		continue;                      \
	}

	const int readTeam = CLuaHandle::GetHandleReadTeam(L);

	lua_newtable(L);

	for (int team = startTeam; team <= endTeam; team++) {
		const std::vector<CUnit*>& units = unitHandler.GetUnitsByTeam(team);

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
			if (CLuaHandle::GetHandleReadAllyTeam(L) == teamHandler.AllyTeam(team)) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST, false);
			}
		}
		else if (allegiance == EnemyUnits) {
			if (CLuaHandle::GetHandleReadAllyTeam(L) != teamHandler.AllyTeam(team)) {
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
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const float range = luaL_optnumber(L, 2, 1.0e9f);
	const CUnit* target =
		CGameHelper::GetClosestFriendlyUnit(unit, unit->pos, range, unit->allyteam);

	if (target != nullptr) {
		lua_pushnumber(L, target->id);
		return 1;
	}
	return 0;
}


int LuaSyncedRead::GetUnitNearestEnemy(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const bool wantLOS = !lua_isboolean(L, 3) || lua_toboolean(L, 3);
	const bool testLOS = !CLuaHandle::GetHandleFullRead(L) || wantLOS;

	const bool sphereDistTest = luaL_optboolean(L, 4, false);
	const bool checkSightDist = luaL_optboolean(L, 5, false);

	// if ignoring LOS, pass checkSightDist=false (by default)
	// such that enemies outside unit's los-range are included
	const CUnit* target = testLOS?
		CGameHelper::GetClosestEnemyUnit         (unit, unit->pos, luaL_optnumber(L, 2, 1.0e9f), unit->allyteam                                ):
		CGameHelper::GetClosestEnemyUnitNoLosTest(unit, unit->pos, luaL_optnumber(L, 2, 1.0e9f), unit->allyteam, sphereDistTest, checkSightDist);

	if (target == nullptr)
		return 0;

	lua_pushnumber(L, target->id);
	return 1;
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

	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, mins, maxs);
	ProcessFeatures(L, *qfQuery.features);
	return 1;
}

int LuaSyncedRead::GetFeaturesInSphere(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float y = luaL_checkfloat(L, 2);
	const float z = luaL_checkfloat(L, 3);
	const float rad = luaL_checkfloat(L, 4);

	const float3 pos(x, y, z);

	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, pos, rad, true);
	ProcessFeatures(L, *qfQuery.features);
	return 1;
}

int LuaSyncedRead::GetFeaturesInCylinder(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);
	const float rad = luaL_checkfloat(L, 3);

	const float3 pos(x, 0, z);

	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, pos, rad, false);
	ProcessFeatures(L, *qfQuery.features);
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

	QuadFieldQuery qfQuery;
	quadField.GetProjectilesExact(qfQuery, mins, maxs);
	const unsigned int rectProjectileCount = qfQuery.projectiles->size();
	unsigned int arrayIndex = 1;

	lua_createtable(L, rectProjectileCount, 0);

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		if (CLuaHandle::GetHandleFullRead(L)) {
			for (unsigned int i = 0; i < rectProjectileCount; i++) {
				const CProjectile* pro = (*qfQuery.projectiles)[i];

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
			const CProjectile* pro = (*qfQuery.projectiles)[i];

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
	lua_pushboolean(L, lua_isnumber(L, 1) && ParseUnit(L, __func__, 1) != nullptr);
	return 1;
}


int LuaSyncedRead::GetUnitStates(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const AMoveType* mt = unit->moveType; // never null
	const CMobileCAI* mCAI = dynamic_cast<const CMobileCAI*>(unit->commandAI);

	const bool retTable = luaL_optboolean(L, 2,     true); // return state as table?
	const bool binState = luaL_optboolean(L, 3, retTable); // include binary state? (activated, etc)
	const bool amtState = luaL_optboolean(L, 4, retTable); // include (Air)MoveType state?

	if (!retTable) {
		{
			lua_pushnumber(L, unit->fireState);
			lua_pushnumber(L, unit->moveState);
			lua_pushnumber(L, (mCAI != nullptr)? mCAI->repairBelowHealth: -1.0f);
		}

		if (binState) {
			lua_pushboolean(L, unit->commandAI->repeatOrders);
			lua_pushboolean(L, unit->wantCloak);
			lua_pushboolean(L, unit->activated);
			lua_pushboolean(L, unit->useHighTrajectory);
		}

		if (amtState) {
			const CHoverAirMoveType* hAMT = nullptr;
			const CStrafeAirMoveType* sAMT = nullptr;

			if ((hAMT = dynamic_cast<const CHoverAirMoveType*>(mt)) != nullptr) {
				lua_pushboolean(L, hAMT->autoLand);
				lua_pushboolean(L, false);
				return (3 + (binState * 4) + 2);
			}

			if ((sAMT = dynamic_cast<const CStrafeAirMoveType*>(mt)) != nullptr) {
				lua_pushboolean(L, sAMT->autoLand);
				lua_pushboolean(L, sAMT->loopbackAttack);
				return (3 + (binState * 4) + 2);
			}
		}

		// reached only if AMT vars were not pushed
		return (3 + (binState * 4));
	}

	{
		lua_createtable(L, 0, 9);

		{
			HSTR_PUSH_NUMBER(L, "firestate",  unit->fireState);
			HSTR_PUSH_NUMBER(L, "movestate",  unit->moveState);
			HSTR_PUSH_NUMBER(L, "autorepairlevel", (mCAI != nullptr)? mCAI->repairBelowHealth: -1.0f);
		}

		if (binState) {
			HSTR_PUSH_BOOL(L, "repeat",     unit->commandAI->repeatOrders);
			HSTR_PUSH_BOOL(L, "cloak",      unit->wantCloak);
			HSTR_PUSH_BOOL(L, "active",     unit->activated);
			HSTR_PUSH_BOOL(L, "trajectory", unit->useHighTrajectory);
		}

		if (amtState) {
			const CHoverAirMoveType* hAMT = nullptr;
			const CStrafeAirMoveType* sAMT = nullptr;

			if ((hAMT = dynamic_cast<const CHoverAirMoveType*>(mt)) != nullptr) {
				HSTR_PUSH_BOOL(L, "autoland",       hAMT->autoLand);
				HSTR_PUSH_BOOL(L, "loopbackattack", false);
				return 1;
			}

			if ((sAMT = dynamic_cast<const CStrafeAirMoveType*>(mt)) != nullptr) {
				HSTR_PUSH_BOOL(L, "autoland",       sAMT->autoLand);
				HSTR_PUSH_BOOL(L, "loopbackattack", sAMT->loopbackAttack);
				return 1;
			}
		}

		return 1;
	}
}


int LuaSyncedRead::GetUnitArmored(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->armoredState);
	lua_pushnumber(L, unit->armoredMultiple);
	return 2;
}


int LuaSyncedRead::GetUnitIsActive(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->activated);
	return 1;
}


int LuaSyncedRead::GetUnitIsCloaked(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->isCloaked);
	return 1;
}


int LuaSyncedRead::GetUnitSelfDTime(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->selfDCountdown);
	return 1;
}


int LuaSyncedRead::GetUnitStockpile(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (unit->stockpileWeapon == nullptr)
		return 0;

	lua_pushnumber(L, unit->stockpileWeapon->numStockpiled);
	lua_pushnumber(L, unit->stockpileWeapon->numStockpileQued);
	lua_pushnumber(L, unit->stockpileWeapon->buildPercent);
	return 3;
}


int LuaSyncedRead::GetUnitSensorRadius(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	switch (hashString(luaL_checkstring(L, 2))) {
		case hashString("los"): {
			lua_pushnumber(L, unit->losRadius);
		} break;
		case hashString("airLos"): {
			lua_pushnumber(L, unit->airLosRadius);
		} break;
		case hashString("radar"): {
			lua_pushnumber(L, unit->radarRadius);
		} break;
		case hashString("sonar"): {
			lua_pushnumber(L, unit->sonarRadius);
		} break;
		case hashString("seismic"): {
			lua_pushnumber(L, unit->seismicRadius);
		} break;
		case hashString("radarJammer"): {
			lua_pushnumber(L, unit->jammerRadius);
		} break;
		case hashString("sonarJammer"): {
			lua_pushnumber(L, unit->sonarJamRadius);
		} break;
		default: {
			luaL_error(L, "[%s] unknown sensor type \"%s\"", __func__, luaL_checkstring(L, 2));
		} break;
	}

	return 1;
}

int LuaSyncedRead::GetUnitPosErrorParams(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int optAllyTeam = luaL_optinteger(L, 2, 0);
	const int argAllyTeam = Clamp(optAllyTeam, 0, teamHandler.ActiveAllyTeams());

	lua_pushnumber(L, unit->posErrorVector.x);
	lua_pushnumber(L, unit->posErrorVector.y);
	lua_pushnumber(L, unit->posErrorVector.z);
	lua_pushnumber(L, unit->posErrorDelta.x);
	lua_pushnumber(L, unit->posErrorDelta.y);
	lua_pushnumber(L, unit->posErrorDelta.z);
	lua_pushnumber(L, unit->nextPosErrorUpdate);
	lua_pushboolean(L, unit->GetPosErrorBit(argAllyTeam));

	return (3 + 3 + 1 + 1);
}


int LuaSyncedRead::GetUnitTooltip(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	std::string tooltip;

	const CTeam* unitTeam = nullptr;
	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = IsAllyUnit(L, unit) ? nullptr : unitDef->decoyDef;
	const UnitDef* effectiveDef = EffectiveUnitDef(L, unit);

	if (effectiveDef->showPlayerName) {
		if (teamHandler.IsValidTeam(unit->team))
			unitTeam = teamHandler.Team(unit->team);

		if (unitTeam != nullptr && unitTeam->HasLeader()) {
			tooltip = playerHandler.Player(unitTeam->GetLeader())->name;
			tooltip = (skirmishAIHandler.HasSkirmishAIsInTeam(unit->team)? "AI@": "") + tooltip;
		}
	} else {
		if (decoyDef == nullptr) {
			tooltip = unitToolTipMap.Get(unit->id);
		} else {
			tooltip = decoyDef->humanName + " - " + decoyDef->tooltip;
		}
	}

	lua_pushsstring(L, tooltip);
	return 1;
}


int LuaSyncedRead::GetUnitDefID(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (IsAllyUnit(L, unit)) {
		lua_pushnumber(L, unit->unitDef->id);
		return 1;
	}

	if (!IsUnitTyped(L, unit))
		return 0;

	lua_pushnumber(L, EffectiveUnitDef(L, unit)->id);
	return 1;
}


int LuaSyncedRead::GetUnitTeam(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->team);
	return 1;
}


int LuaSyncedRead::GetUnitAllyTeam(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->allyteam);
	return 1;
}


int LuaSyncedRead::GetUnitNeutral(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->IsNeutral());
	return 1;
}


int LuaSyncedRead::GetUnitHealth(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const UnitDef* ud = unit->unitDef;
	const bool enemyUnit = IsEnemyUnit(L, unit);

	if (ud->hideDamage && enemyUnit) {
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	} else if (!enemyUnit || (ud->decoyDef == nullptr)) {
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
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->isDead);
	return 1;
}


int LuaSyncedRead::GetUnitIsStunned(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->IsStunned() || unit->beingBuilt);
	lua_pushboolean(L, unit->IsStunned());
	lua_pushboolean(L, unit->beingBuilt);
	return 3;
}


int LuaSyncedRead::GetUnitResources(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->resourcesMake.metal);
	lua_pushnumber(L, unit->resourcesUse.metal);
	lua_pushnumber(L, unit->resourcesMake.energy);
	lua_pushnumber(L, unit->resourcesUse.energy);
	return 4;
}


int LuaSyncedRead::GetUnitMetalExtraction(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (!unit->unitDef->extractsMetal)
		return 0;

	lua_pushnumber(L, unit->metalExtract);
	return 1;
}


int LuaSyncedRead::GetUnitExperience(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->experience);
	lua_pushnumber(L, unit->limExperience);
	return 2;
}


int LuaSyncedRead::GetUnitHeight(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->height);
	return 1;
}


int LuaSyncedRead::GetUnitRadius(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->radius);
	return 1;
}

int LuaSyncedRead::GetUnitMass(lua_State* L)
{
	return (GetSolidObjectMass(L, ParseInLosUnit(L, __func__, 1)));
}

int LuaSyncedRead::GetUnitPosition(lua_State* L)
{
	return (GetSolidObjectPosition(L, ParseUnit(L, __func__, 1), false));
}

int LuaSyncedRead::GetUnitBasePosition(lua_State* L)
{
	return (GetUnitPosition(L));
}

int LuaSyncedRead::GetUnitVectors(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

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
	return (GetSolidObjectRotation(L, ParseInLosUnit(L, __func__, 1)));
}

int LuaSyncedRead::GetUnitDirection(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->frontdir.x);
	lua_pushnumber(L, unit->frontdir.y);
	lua_pushnumber(L, unit->frontdir.z);
	return 3;
}


int LuaSyncedRead::GetUnitHeading(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->heading);
	return 1;
}


int LuaSyncedRead::GetUnitVelocity(lua_State* L)
{
	return (GetWorldObjectVelocity(L, ParseInLosUnit(L, __func__, 1)));
}


int LuaSyncedRead::GetUnitBuildFacing(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->buildFacing);
	return 1;
}


int LuaSyncedRead::GetUnitIsBuilding(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

	if (builder != nullptr && builder->curBuild) {
		lua_pushnumber(L, builder->curBuild->id);
		return 1;
	}

	const CFactory* factory = dynamic_cast<const CFactory*>(unit);

	if (factory != nullptr && factory->curBuild) {
		lua_pushnumber(L, factory->curBuild->id);
		return 1;
	}

	return 0;
}


int LuaSyncedRead::GetUnitCurrentBuildPower(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const NanoPieceCache* pieceCache = nullptr;

	{
		const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

		if (builder != nullptr)
			pieceCache = &builder->GetNanoPieceCache();

		const CFactory* factory = dynamic_cast<const CFactory*>(unit);

		if (factory != nullptr)
			pieceCache = &factory->GetNanoPieceCache();
	}

	if (pieceCache == nullptr)
		return 0;

	lua_pushnumber(L, pieceCache->GetBuildPower());
	return 1;
}


int LuaSyncedRead::GetUnitHarvestStorage(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
		lua_pushnumber(L, unit->harvested[i]);
		lua_pushnumber(L, unit->harvestStorage[i]);
	}
	return 2 * SResourcePack::MAX_RESOURCES;
}


int LuaSyncedRead::GetUnitNanoPieces(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const NanoPieceCache* pieceCache = nullptr;
	const std::vector<int>* nanoPieces = nullptr;

	{
		const CBuilder* builder = dynamic_cast<const CBuilder*>(unit);

		if (builder != nullptr) {
			pieceCache = &builder->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}

		const CFactory* factory = dynamic_cast<const CFactory*>(unit);

		if (factory != nullptr) {
			pieceCache = &factory->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}
	}

	if (nanoPieces == nullptr || nanoPieces->empty())
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
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (unit->transporter == nullptr)
		return 0;

	lua_pushnumber(L, unit->transporter->id);
	return 1;
}


int LuaSyncedRead::GetUnitIsTransporting(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr || !unit->unitDef->IsTransportUnit())
		return 0;

	lua_createtable(L, unit->transportedUnits.size(), 0);

	unsigned int unitCount = 1;

	for (const CUnit::TransportedUnit& tu: unit->transportedUnits) {
		const CUnit* carried = tu.unit;

		lua_pushnumber(L, carried->id);
		lua_rawseti(L, -2, unitCount++);
	}

	return 1;
}


int LuaSyncedRead::GetUnitShieldState(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const CPlasmaRepulser* shield = nullptr;
	const size_t idx = luaL_optint(L, 2, -1) - LUA_WEAPON_BASE_INDEX;

	if (idx >= unit->weapons.size()) {
		shield = static_cast<const CPlasmaRepulser*>(unit->shieldWeapon);
	} else {
		shield = dynamic_cast<const CPlasmaRepulser*>(unit->weapons[idx]);
	}

	if (shield == nullptr)
		return 0;

	lua_pushnumber(L, shield->IsEnabled());
	lua_pushnumber(L, shield->GetCurPower());
	return 2;
}


int LuaSyncedRead::GetUnitFlanking(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if (lua_israwstring(L, 2)) {
		const char* key = lua_tostring(L, 2);

		switch (hashString(key)) {
			case hashString("mode"): {
				lua_pushnumber(L, unit->flankingBonusMode);
				return 1;
			} break;
			case hashString("dir"): {
				lua_pushnumber(L, unit->flankingBonusDir.x);
				lua_pushnumber(L, unit->flankingBonusDir.y);
				lua_pushnumber(L, unit->flankingBonusDir.z);
				return 3;
			} break;
			case hashString("moveFactor"): {
				lua_pushnumber(L, unit->flankingBonusMobilityAdd);
				return 1;
			} break;
			case hashString("minDamage"): {
				lua_pushnumber(L, unit->flankingBonusAvgDamage - unit->flankingBonusDifDamage);
				return 1;
			} break;
			case hashString("maxDamage"): {
				lua_pushnumber(L, unit->flankingBonusAvgDamage + unit->flankingBonusDifDamage);
				return 1;
			} break;
			default: {
			} break;
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
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	lua_pushnumber(L, unit->maxRange);
	return 1;
}


int LuaSyncedRead::GetUnitWeaponState(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const char* key = luaL_optstring(L, 3, "");

	if (key[0] == 0) { // backwards compatible
		lua_pushboolean(L, weapon->angleGood);
		lua_pushboolean(L, weapon->reloadStatus <= gs->frameNum);
		lua_pushnumber(L,  weapon->reloadStatus);
		lua_pushnumber(L,  weapon->salvoLeft);
		lua_pushnumber(L,  weapon->numStockpiled);
		return 5;
	}

	switch (hashString(key)) {
		case hashString("reloadState"):
		case hashString("reloadFrame"): {
			lua_pushnumber(L, weapon->reloadStatus);
		} break;

		case hashString("reloadTime"): {
			// SetUnitWeaponState sets reloadTime to int(value * GAME_SPEED);
			// divide by 1.0 here since reloadTime / GAME_SPEED would itself
			// be an integer division
			lua_pushnumber(L, (weapon->reloadTime / 1.0f) / GAME_SPEED);
		} break;
		case hashString("reloadTimeXP"): {
			// reloadSpeed is affected by unit experience
			lua_pushnumber(L, (weapon->reloadTime / unit->reloadSpeed) / GAME_SPEED);
		} break;
		case hashString("reaimTime"): {
			lua_pushnumber(L, weapon->reaimTime);
		} break;

		case hashString("accuracy"): {
			lua_pushnumber(L, weapon->AccuracyExperience());
		} break;
		case hashString("sprayAngle"): {
			lua_pushnumber(L, weapon->SprayAngleExperience());
		} break;

		case hashString("range"): {
			lua_pushnumber(L, weapon->range);
		} break;
		case hashString("projectileSpeed"): {
			lua_pushnumber(L, weapon->projectileSpeed);
		} break;

		case hashString("autoTargetRangeBoost"): {
			lua_pushnumber(L, weapon->autoTargetRangeBoost);
		} break;

		case hashString("burst"): {
			lua_pushnumber(L, weapon->salvoSize);
		} break;
		case hashString("burstRate"): {
			lua_pushnumber(L, weapon->salvoDelay / GAME_SPEED);
		} break;

		case hashString("projectiles"): {
			lua_pushnumber(L, weapon->projectilesPerShot);
		} break;

		case hashString("salvoError"): {
			const float3 salvoError =  weapon->SalvoErrorExperience();

			lua_createtable(L, 3, 0);
			lua_pushnumber(L, salvoError.x); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, salvoError.y); lua_rawseti(L, -2, 2);
			lua_pushnumber(L, salvoError.z); lua_rawseti(L, -2, 3);
		} break;

		case hashString("salvoLeft"): {
			lua_pushnumber(L, weapon->salvoLeft);
		} break;
		case hashString("nextSalvo"): {
			lua_pushnumber(L, weapon->nextSalvo);
		} break;

		case hashString("targetMoveError"): {
			lua_pushnumber(L, weapon->MoveErrorExperience());
		} break;

		case hashString("avoidFlags"): {
			lua_pushnumber(L, weapon->avoidFlags);
		} break;
		case hashString("collisionFlags"): {
			lua_pushnumber(L, weapon->collisionFlags);
		} break;

		default: {
			return 0;
		} break;
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

	switch (hashString(luaL_checkstring(L, index))) {
		case hashString("paralyzeDamageTime"): {
			lua_pushnumber(L, damages.paralyzeDamageTime);
		} break;

		case hashString("impulseFactor"): {
			lua_pushnumber(L, damages.impulseFactor);
		} break;
		case hashString("impulseBoost"): {
			lua_pushnumber(L, damages.impulseBoost);
		} break;

		case hashString("craterMult"): {
			lua_pushnumber(L, damages.craterMult);
		} break;
		case hashString("craterBoost"): {
			lua_pushnumber(L, damages.craterBoost);
		} break;

		case hashString("dynDamageExp"): {
			lua_pushnumber(L, damages.dynDamageExp);
		} break;
		case hashString("dynDamageMin"): {
			lua_pushnumber(L, damages.dynDamageMin);
		} break;
		case hashString("dynDamageRange"): {
			lua_pushnumber(L, damages.dynDamageRange);
		} break;
		case hashString("dynDamageInverted"): {
			lua_pushboolean(L, damages.dynDamageInverted);
		} break;

		case hashString("craterAreaOfEffect"): {
			lua_pushnumber(L, damages.craterAreaOfEffect);
		} break;
		case hashString("damageAreaOfEffect"): {
			lua_pushnumber(L, damages.damageAreaOfEffect);
		} break;

		case hashString("edgeEffectiveness"): {
			lua_pushnumber(L, damages.edgeEffectiveness);
		} break;
		case hashString("explosionSpeed"): {
			lua_pushnumber(L, damages.explosionSpeed);
		} break;

		default: {
			return 0;
		} break;
	}

	return 1;
}


int LuaSyncedRead::GetUnitWeaponDamages(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const DynDamageArray* damages;

	if (lua_israwstring(L, 2)) {
		const char* key = lua_tostring(L, 2);

		switch (hashString(key)) {
			case hashString("explode"     ): { damages = unit->deathExpDamages; } break;
			case hashString("selfDestruct"): { damages = unit->selfdExpDamages; } break;
			default                        : {                        return 0; } break;
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
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const float3& pos = weapon->weaponMuzzlePos;
	const float3* dir = &weapon->wantedDir;

	switch (weapon->weaponDef->projectileType) {
		case WEAPON_MISSILE_PROJECTILE  : { dir = &weapon->weaponDir; } break;
		case WEAPON_TORPEDO_PROJECTILE  : { dir = &weapon->weaponDir; } break;
		case WEAPON_STARBURST_PROJECTILE: { dir = &weapon->weaponDir; } break;
		default                         : {                           } break;
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
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = nullptr;

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
		enemy = ParseUnit(L, __func__, 3);

		if (enemy == nullptr)
			return 0;
	}

	lua_pushboolean(L, weapon->TryTarget(SWeaponTarget(enemy, pos, true)));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponTestTarget(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = nullptr;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		if ((enemy = ParseUnit(L, __func__, 3)) == nullptr)
			return 0;

		pos = weapon->GetUnitLeadTargetPos(enemy);
	}

	lua_pushboolean(L, weapon->TestTarget(pos, SWeaponTarget(enemy, pos, true)));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponTestRange(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = nullptr;

	float3 pos;

	if (lua_gettop(L) >= 5) {
		pos.x = luaL_optnumber(L, 3, 0.0f);
		pos.y = luaL_optnumber(L, 4, 0.0f);
		pos.z = luaL_optnumber(L, 5, 0.0f);
	} else {
		if ((enemy = ParseUnit(L, __func__, 3)) == nullptr)
			return 0;

		pos = weapon->GetUnitLeadTargetPos(enemy);
	}

	lua_pushboolean(L, weapon->TestRange(pos, SWeaponTarget(enemy, pos, true)));
	return 1;
}


int LuaSyncedRead::GetUnitWeaponHaveFreeLineOfFire(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	const CWeapon* weapon = unit->weapons[weaponNum];
	const CUnit* enemy = nullptr;

	float3 srcPos = weapon->GetAimFromPos();
	float3 tgtPos;

	const auto ParsePos = [&L](int idx, int cnt, float* pos) {
		for (int i = 0; i < cnt; i++) {
			pos[i] = luaL_optnumber(L, idx + i, pos[i]);
		}
	};

	switch (lua_gettop(L)) {
		case 3: {
			// [3] := targetID
			if ((enemy = ParseUnit(L, __func__, 3)) == nullptr)
				return 0;

			tgtPos = weapon->GetUnitLeadTargetPos(enemy);
		} break;
		case 5: {
			// [3,4,5] := srcPos
			ParsePos(3, 3, &srcPos.x);
		} break;

		case 6: {
			// [3,4,5] := srcPos, [6] := targetID
			ParsePos(3, 3, &srcPos.x);

			if ((enemy = ParseUnit(L, __func__, 6)) == nullptr)
				return 0;

			tgtPos = weapon->GetUnitLeadTargetPos(enemy);
		} break;
		case 8: {
			// [3,4,5] := srcPos, [6,7,8] := tgtPos
			ParsePos(3, 3, &srcPos.x);
			ParsePos(6, 3, &tgtPos.x);
		} break;

		default: {
			return 0;
		} break;
	}

	lua_pushboolean(L, weapon->HaveFreeLineOfFire(srcPos, tgtPos, SWeaponTarget(enemy, tgtPos, true)));
	return 1;
}

int LuaSyncedRead::GetUnitWeaponCanFire(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
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
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
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


int LuaSyncedRead::GetUnitTravel(lua_State* L) { lua_pushnumber(L, 0.0f); lua_pushnumber(L, 0.0f); return 2; } // DEPRECATED
int LuaSyncedRead::GetUnitFuel(lua_State* L) { lua_pushnumber(L, 0.0f); return 1; } // DEPRECATED


int LuaSyncedRead::GetUnitEstimatedPath(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const CGroundMoveType* gmt = dynamic_cast<const CGroundMoveType*>(unit->moveType);

	if (gmt == nullptr)
		return 0;

	return (LuaPathFinder::PushPathNodes(L, gmt->GetPathID()));
}


int LuaSyncedRead::GetUnitLastAttacker(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	if ((unit->lastAttacker == nullptr) ||
	    !IsUnitVisible(L, unit->lastAttacker)) {
		return 0;
	}
	lua_pushnumber(L, unit->lastAttacker->id);
	return 1;
}


int LuaSyncedRead::GetUnitLastAttackedPiece(lua_State* L)
{
	return (GetSolidObjectLastHitPiece(L, ParseAllyUnit(L, __func__, 1)));
}

int LuaSyncedRead::GetUnitCollisionVolumeData(lua_State* L)
{
	const CUnit* unit = ParseInLosUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	return LuaUtils::PushColVolData(L, &unit->collisionVolume);
}

int LuaSyncedRead::GetUnitPieceCollisionVolumeData(lua_State* L)
{
	return (PushPieceCollisionVolumeData(L, ParseInLosUnit(L, __func__, 1)));
}


int LuaSyncedRead::GetUnitSeparation(lua_State* L)
{
	const CUnit* unit1 = ParseUnit(L, __func__, 1);
	const CUnit* unit2 = ParseUnit(L, __func__, 2);

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
	const CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CFeature* feature = ParseFeature(L, __func__, 2);

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
	if (ud == nullptr)
		return 0;

	const S3DModel* model = ud->LoadModel();
	if (model == nullptr)
		return 0;

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


int LuaSyncedRead::GetCEGID(lua_State* L)
{
	lua_pushnumber(L, explGenHandler.LoadCustomGeneratorID(luaL_checkstring(L, 1)));
	return 1;
}


int LuaSyncedRead::GetUnitBlocking(lua_State* L)
{
	return (GetSolidObjectBlocking(L, ParseTypedUnit(L, __func__, 1)));
}


int LuaSyncedRead::GetUnitMoveTypeData(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	AMoveType* amt = unit->moveType;

	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "maxSpeed", amt->GetMaxSpeed() * GAME_SPEED);
	HSTR_PUSH_NUMBER(L, "maxWantedSpeed", amt->GetMaxWantedSpeed() * GAME_SPEED);
	HSTR_PUSH_NUMBER(L, "goalx", amt->goalPos.x);
	HSTR_PUSH_NUMBER(L, "goaly", amt->goalPos.y);
	HSTR_PUSH_NUMBER(L, "goalz", amt->goalPos.z);

	switch (amt->progressState) {
		case AMoveType::Done:
			HSTR_PUSH_CSTRING(L, "progressState", "done");
			break;
		case AMoveType::Active:
			HSTR_PUSH_CSTRING(L, "progressState", "active");
			break;
		case AMoveType::Failed:
			HSTR_PUSH_CSTRING(L, "progressState", "failed");
			break;
	}

	const CGroundMoveType* groundmt = dynamic_cast<CGroundMoveType*>(unit->moveType);

	if (groundmt != nullptr) {
		HSTR_PUSH_CSTRING(L, "name", "ground");

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

	if (hAMT != nullptr) {
		HSTR_PUSH_CSTRING(L, "name", "gunship");

		HSTR_PUSH_NUMBER(L, "wantedHeight", hAMT->wantedHeight);
		HSTR_PUSH_BOOL(L, "collide", hAMT->collide);
		HSTR_PUSH_BOOL(L, "useSmoothMesh", hAMT->useSmoothMesh);

		switch (hAMT->aircraftState) {
			case AAirMoveType::AIRCRAFT_LANDED:
				HSTR_PUSH_CSTRING(L, "aircraftState", "landed");
				break;
			case AAirMoveType::AIRCRAFT_FLYING:
				HSTR_PUSH_CSTRING(L, "aircraftState", "flying");
				break;
			case AAirMoveType::AIRCRAFT_LANDING:
				HSTR_PUSH_CSTRING(L, "aircraftState", "landing");
				break;
			case AAirMoveType::AIRCRAFT_CRASHING:
				HSTR_PUSH_CSTRING(L, "aircraftState", "crashing");
				break;
			case AAirMoveType::AIRCRAFT_TAKEOFF:
				HSTR_PUSH_CSTRING(L, "aircraftState", "takeoff");
				break;
			case AAirMoveType::AIRCRAFT_HOVERING:
				HSTR_PUSH_CSTRING(L, "aircraftState", "hovering");
				break;
		};

		switch (hAMT->flyState) {
			case CHoverAirMoveType::FLY_CRUISING:
				HSTR_PUSH_CSTRING(L, "flyState", "cruising");
				break;
			case CHoverAirMoveType::FLY_CIRCLING:
				HSTR_PUSH_CSTRING(L, "flyState", "circling");
				break;
			case CHoverAirMoveType::FLY_ATTACKING:
				HSTR_PUSH_CSTRING(L, "flyState", "attacking");
				break;
			case CHoverAirMoveType::FLY_LANDING:
				HSTR_PUSH_CSTRING(L, "flyState", "landing");
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

	if (sAMT != nullptr) {
		HSTR_PUSH_CSTRING(L, "name", "airplane");

		switch (sAMT->aircraftState) {
			case AAirMoveType::AIRCRAFT_LANDED:
				HSTR_PUSH_CSTRING(L, "aircraftState", "landed");
				break;
			case AAirMoveType::AIRCRAFT_FLYING:
				HSTR_PUSH_CSTRING(L, "aircraftState", "flying");
				break;
			case AAirMoveType::AIRCRAFT_LANDING:
				HSTR_PUSH_CSTRING(L, "aircraftState", "landing");
				break;
			case AAirMoveType::AIRCRAFT_CRASHING:
				HSTR_PUSH_CSTRING(L, "aircraftState", "crashing");
				break;
			case AAirMoveType::AIRCRAFT_TAKEOFF:
				HSTR_PUSH_CSTRING(L, "aircraftState", "takeoff");
				break;
			case AAirMoveType::AIRCRAFT_HOVERING:
				HSTR_PUSH_CSTRING(L, "aircraftState", "hovering");
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

	if (staticmt != nullptr) {
		HSTR_PUSH_CSTRING(L, "name", "static");
		return 1;
	}

	const CScriptMoveType* scriptmt = dynamic_cast<CScriptMoveType*>(unit->moveType);

	if (scriptmt != nullptr) {
		HSTR_PUSH_CSTRING(L, "name", "script");
		return 1;
	}

	HSTR_PUSH_CSTRING(L, "name", "unknown");
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

	HSTR_PUSH_NUMBER(L, "tag", cmd.GetTag());
}


static void PackCommandQueue(lua_State* L, const CCommandQueue& commands, size_t count)
{
	size_t c = 0;

	// get the desired number of commands to return
	if (count == -1u)
		count = commands.size();

	// count can exceed the queue size, clamp
	lua_createtable(L, std::min(count, commands.size()), 0);

	// {[1] = cq[0], [2] = cq[1], ...}
	for (const auto& command: commands) {
		if (c >= count)
			break;

		PackCommand(L, command);
		lua_rawseti(L, -2, ++c);
	}
}

int LuaSyncedRead::GetUnitCurrentCommand(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI; // never null
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	const CCommandQueue* queue = (factoryCAI == nullptr)? &commandAI->commandQue : &factoryCAI->newUnitCommands;

	// - 1 to convert from lua index to C index
	const unsigned int cmdIndex = luaL_optint(L, 2, 1) - 1;

	if (cmdIndex >= queue->size())
		return 0;

	const Command& cmd = queue->at(cmdIndex);
	lua_pushnumber(L, cmd.GetID());
	lua_pushnumber(L, cmd.GetOpts());
	lua_pushnumber(L, cmd.GetTag());

	const unsigned int numParams = cmd.GetNumParams();
	for (unsigned int i = 0; i < numParams; ++i)
		lua_pushnumber(L, cmd.GetParam(i));

	return 3 + numParams;
}

int LuaSyncedRead::GetUnitCommands(lua_State* L)
{
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	// send the new unit commands for factories, otherwise the normal commands
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	const CCommandQueue* queue = (factoryCAI == nullptr)? &commandAI->commandQue : &factoryCAI->newUnitCommands;

	const int  numCmds   = luaL_checkint(L, 2); // must always be given, -1 is a performance pitfall
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
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);

	// bail if not a factory
	if (factoryCAI == nullptr)
		return 0;

	const CCommandQueue& commandQue = factoryCAI->commandQue;

	const int  numCmds   = luaL_checkint(L, 2);
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
		const int cmdID = it->GetID();
		if (noCmds && (cmdID >= 0))
			continue;

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
	const CUnit* unit = ParseAllyUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);

	if (factoryCAI == nullptr)
		return 0; // not a factory, bail

	const CCommandQueue& commandQue = factoryCAI->commandQue;

	// get the desired number of commands to return
	int count = luaL_optint(L, 2, -1);
	if (count < 0)
		count = (int)commandQue.size();

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
	const CUnit* unit = ParseAllyUnit(L, caller, 1);
	if (unit == nullptr)
		return 0;

	const CCommandAI* commandAI = unit->commandAI;
	const CCommandQueue& commandQue = commandAI->commandQue;

	lua_createtable(L, commandQue.size(), 0);

	int entry = 0;
	int currentType = -1;
	int currentCount = 0;

	for (const Command& cmd: commandQue) {
		// not a build command
		if (cmd.GetID() >= 0)
			continue;

		const int unitDefID = -cmd.GetID();

		if (canBuild) {
			// skip build orders that this unit can not start
			const UnitDef* buildeeDef = unitDefHandler->GetUnitDefByID(unitDefID);
			const UnitDef* builderDef = unit->unitDef;

			// if something is wrong, bail
			if ((buildeeDef == nullptr) || (builderDef == nullptr))
				continue;

			using P = decltype(UnitDef::buildOptions)::value_type;

			const auto& buildOptCmp = [&](const P& e) { return (strcasecmp(e.second.c_str(), buildeeDef->name.c_str()) == 0); };
			const auto& buildOpts = builderDef->buildOptions;
			const auto  buildOptIt = std::find_if(buildOpts.cbegin(), buildOpts.cend(), buildOptCmp);

			// didn't find a matching entry
			if (buildOptIt == buildOpts.end())
				continue;
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
	return PackBuildQueue(L, false, __func__);
}


int LuaSyncedRead::GetRealBuildQueue(lua_State* L)
{
	return PackBuildQueue(L, true, __func__);
}


/******************************************************************************/

static int GetUnitRulesParamLosMask(lua_State* L, const CUnit* unit)
{
	if (IsAllyUnit(L, unit) || game->IsGameOver())
		return LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	if (teamHandler.AlliedTeams(unit->team, CLuaHandle::GetHandleReadTeam(L)))
		return LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0)
		return LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	const auto losStatus = unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)];
	if (losStatus & LOS_INLOS)
		return LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	if (losStatus & (LOS_PREVLOS | LOS_CONTRADAR))
		return LuaRulesParams::RULESPARAMLOS_TYPED_MASK;
	if (losStatus & LOS_INRADAR)
		return LuaRulesParams::RULESPARAMLOS_INRADAR_MASK;

	return LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;
}

int LuaSyncedRead::GetUnitRulesParams(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr || game == nullptr)
		return 0;

	return PushRulesParams(L, __func__, unit->modParams, GetUnitRulesParamLosMask(L, unit));
}


int LuaSyncedRead::GetUnitRulesParam(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr || game == nullptr)
		return 0;

	return GetRulesParam(L, __func__, 2, unit->modParams, GetUnitRulesParamLosMask(L, unit));
}


/******************************************************************************/

int LuaSyncedRead::GetUnitCmdDescs(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

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
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

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
	lua_pushboolean(L, lua_isnumber(L, 1) && ParseFeature(L, __func__, 1) != nullptr);
	return 1;
}


int LuaSyncedRead::GetAllFeatures(lua_State* L)
{
	int count = 0;
	const auto& activeFeatureIDs = featureHandler.GetActiveFeatureIDs();

	lua_createtable(L, activeFeatureIDs.size(), 0);

	if (CLuaHandle::GetHandleFullRead(L)) {
		for (const int featureID: activeFeatureIDs) {
			lua_pushnumber(L, featureID);
			lua_rawseti(L, -2, ++count);
		}
	} else {
		for (const int featureID: activeFeatureIDs) {
			if (IsFeatureVisible(L, featureHandler.GetFeature(featureID))) {
				lua_pushnumber(L, featureID);
				lua_rawseti(L, -2, ++count);
			}
		}
	}
	return 1;
}


int LuaSyncedRead::GetFeatureDefID(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->def->id);
	return 1;
}


int LuaSyncedRead::GetFeatureTeam(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	if (feature->allyteam < 0) {
		lua_pushnumber(L, -1);
	} else {
		lua_pushnumber(L, feature->team);
	}
	return 1;
}


int LuaSyncedRead::GetFeatureAllyTeam(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->allyteam);
	return 1;
}


int LuaSyncedRead::GetFeatureHealth(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->health);
	lua_pushnumber(L, feature->def->health);
	lua_pushnumber(L, feature->resurrectProgress);
	return 3;
}


int LuaSyncedRead::GetFeatureHeight(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->height);
	return 1;
}


int LuaSyncedRead::GetFeatureRadius(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->radius);
	return 1;
}

int LuaSyncedRead::GetFeatureMass(lua_State* L)
{
	return (GetSolidObjectMass(L, ParseFeature(L, __func__, 1)));
}

int LuaSyncedRead::GetFeaturePosition(lua_State* L)
{
	return (GetSolidObjectPosition(L, ParseFeature(L, __func__, 1), true));
}

int LuaSyncedRead::GetFeatureSeparation(lua_State* L)
{
	const CFeature* feature1 = ParseFeature(L, __func__, 1);
	if (feature1 == nullptr || !IsFeatureVisible(L, feature1))
		return 0;

	const CFeature* feature2 = ParseFeature(L, __func__, 2);
	if (feature2 == nullptr || !IsFeatureVisible(L, feature2))
		return 0;

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
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	return GetSolidObjectRotation(L, feature);
}

int LuaSyncedRead::GetFeatureDirection(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr || !IsFeatureVisible(L, feature))
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
	return (GetWorldObjectVelocity(L, ParseFeature(L, __func__, 1)));
}


int LuaSyncedRead::GetFeatureHeading(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L, feature->heading);
	return 1;
}


int LuaSyncedRead::GetFeatureResources(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushnumber(L,  feature->resources.metal);
	lua_pushnumber(L,  feature->defResources.metal);
	lua_pushnumber(L,  feature->resources.energy);
	lua_pushnumber(L,  feature->defResources.energy);
	lua_pushnumber(L,  feature->reclaimLeft);
	lua_pushnumber(L,  feature->reclaimTime);
	return 6;
}


int LuaSyncedRead::GetFeatureBlocking(lua_State* L)
{
	return (GetSolidObjectBlocking(L, ParseFeature(L, __func__, 1)));
}


int LuaSyncedRead::GetFeatureNoSelect(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr || !IsFeatureVisible(L, feature))
		return 0;

	lua_pushboolean(L, feature->noSelect);
	return 1;
}


int LuaSyncedRead::GetFeatureResurrect(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	if (feature->udef == nullptr) {
		lua_pushliteral(L, "");
	} else {
		lua_pushsstring(L, feature->udef->name);
	}

	lua_pushnumber(L, feature->buildFacing);
	return 2;
}


int LuaSyncedRead::GetFeatureLastAttackedPiece(lua_State* L)
{
	return (GetSolidObjectLastHitPiece(L, ParseFeature(L, __func__, 1)));
}

int LuaSyncedRead::GetFeatureCollisionVolumeData(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	return LuaUtils::PushColVolData(L, &feature->collisionVolume);
}

int LuaSyncedRead::GetFeaturePieceCollisionVolumeData(lua_State* L)
{
	return (PushPieceCollisionVolumeData(L, ParseFeature(L, __func__, 1)));
}


int LuaSyncedRead::GetFeatureRulesParams(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (IsAlliedAllyTeam(L, feature->allyteam) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler.AlliedTeams(feature->team, CLuaHandle::GetHandleReadTeam(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		//! NoAccessTeam
	}
	else if (IsFeatureVisible(L, feature)) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	}

	const LuaRulesParams::Params&  params = feature->modParams;

	return PushRulesParams(L, __func__, params, losMask);
}


int LuaSyncedRead::GetFeatureRulesParam(lua_State* L)
{
	const CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (IsAlliedAllyTeam(L, feature->allyteam) || game->IsGameOver()) {
		losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
	}
	else if (teamHandler.AlliedTeams(feature->team, CLuaHandle::GetHandleReadTeam(L))) {
		losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
	}
	else if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		//! NoAccessTeam
	}
	else if (IsFeatureVisible(L, feature)) {
		losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
	}

	return GetRulesParam(L, __func__, 2, feature->modParams, losMask);
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::GetProjectilePosition(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	lua_pushnumber(L, pro->pos.x);
	lua_pushnumber(L, pro->pos.y);
	lua_pushnumber(L, pro->pos.z);
	return 3;
}

int LuaSyncedRead::GetProjectileDirection(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	lua_pushnumber(L, pro->dir.x);
	lua_pushnumber(L, pro->dir.y);
	lua_pushnumber(L, pro->dir.z);
	return 3;
}

int LuaSyncedRead::GetProjectileVelocity(lua_State* L)
{
	return (GetWorldObjectVelocity(L, ParseProjectile(L, __func__, 1)));
}


int LuaSyncedRead::GetProjectileGravity(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
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
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr || !pro->piece)
		return 0;

	const CPieceProjectile* ppro = static_cast<const CPieceProjectile*>(pro);

	lua_pushnumber(L, ppro->explFlags);
	lua_pushnumber(L, ppro->spinParams.y);
	lua_pushnumber(L, ppro->spinParams.x);
	lua_pushnumber(L, ppro->spinVector.x);
	lua_pushnumber(L, ppro->spinVector.y);
	lua_pushnumber(L, ppro->spinVector.z);
	return (1 + 1 + 1 + 3);
}

int LuaSyncedRead::GetProjectileTarget(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const CWorldObject* wtgt = wpro->GetTargetObject();

	if (wtgt == nullptr) {
		lua_pushnumber(L, int('g')); // ground
		lua_createtable(L, 3, 0);
		lua_pushnumber(L, (wpro->GetTargetPos()).x); lua_rawseti(L, -2, 1);
		lua_pushnumber(L, (wpro->GetTargetPos()).y); lua_rawseti(L, -2, 2);
		lua_pushnumber(L, (wpro->GetTargetPos()).z); lua_rawseti(L, -2, 3);
		return 2;
	}

	if (dynamic_cast<const CUnit*>(wtgt) != nullptr) {
		lua_pushnumber(L, int('u'));
		lua_pushnumber(L, wtgt->id);
		return 2;
	}
	if (dynamic_cast<const CFeature*>(wtgt) != nullptr) {
		lua_pushnumber(L, int('f'));
		lua_pushnumber(L, wtgt->id);
		return 2;
	}
	if (dynamic_cast<const CWeaponProjectile*>(wtgt) != nullptr) {
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
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);

	lua_pushboolean(L, wpro->IsBeingIntercepted());
	return 1;
}

int LuaSyncedRead::GetProjectileTimeToLive(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr || !pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);

	lua_pushnumber(L, wpro->GetTimeToLive());
	return 1;
}

int LuaSyncedRead::GetProjectileOwnerID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	const int unitID = pro->GetOwnerID();
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= unitHandler.MaxUnits()))
		return 0;

	lua_pushnumber(L, unitID);
	return 1;
}

int LuaSyncedRead::GetProjectileTeamID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	if (!teamHandler.IsValidTeam(pro->GetTeamID()))
		return 0;

	lua_pushnumber(L, pro->GetTeamID());
	return 1;
}

int LuaSyncedRead::GetProjectileType(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	lua_pushboolean(L, pro->weapon);
	lua_pushboolean(L, pro->piece);
	return 2;
}

int LuaSyncedRead::GetProjectileDefID(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;
	if (!pro->weapon)
		return 0;

	const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);
	const WeaponDef* wdef = wpro->GetWeaponDef();

	if (wdef == nullptr)
		return 0;

	lua_pushnumber(L, wdef->id);
	return 1;
}

int LuaSyncedRead::GetProjectileDamages(lua_State* L)
{
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

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
	const CProjectile* pro = ParseProjectile(L, __func__, 1);

	if (pro == nullptr)
		return 0;

	switch ((pro->weapon * 2) + (pro->piece * 1)) {
		case 2: {
			const CWeaponProjectile* wpro = static_cast<const CWeaponProjectile*>(pro);

			if (wpro != nullptr && wpro->GetWeaponDef() != nullptr) {
				// maybe CWeaponProjectile derivatives
				// should have actual names themselves?
				lua_pushsstring(L, wpro->GetWeaponDef()->name);
				return 1;
			}
		} break;
		case 1: {
			const CPieceProjectile* ppro = static_cast<const CPieceProjectile*>(pro);

			if (ppro != nullptr && ppro->modelPiece != nullptr) {
				lua_pushsstring(L, ppro->modelPiece->name);
				return 1;
			}
		} break;
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
	const int  ttIndex = readMap->GetTypeMapSynced()[sqrIndex];

	assert(ttIndex < CMapInfo::NUM_TERRAIN_TYPES);
	assert(lua_gettop(L) == 2);

	// LuaMetalMap::GetMetalAmount uses absolute indexing,
	// replace the top two elements (x and z) by ix and iz
	lua_pop(L, 2);
	lua_pushnumber(L, ix);
	lua_pushnumber(L, iz);

	return (PushTerrainTypeData(L, &mapInfo->terrainTypes[ttIndex], true));
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
}


int LuaSyncedRead::GetGroundBlocked(lua_State* L)
{
	if ((CLuaHandle::GetHandleReadAllyTeam(L) < 0) && !CLuaHandle::GetHandleFullRead(L))
		return 0;

	int tx1, tx2, tz1, tz2;
	ParseMapCoords(L, __func__, tx1, tz1, tx2, tz2);

	for (int z = tz1; z <= tz2; z++){
		for (int x = tx1; x <= tx2; x++){
			const CSolidObject* s = groundBlockingObjectMap.GroundBlocked(x, z);

			const CFeature* feature = dynamic_cast<const CFeature*>(s);
			if (feature != nullptr) {
				if (IsFeatureVisible(L, feature)) {
					HSTR_PUSH(L, "feature");
					lua_pushnumber(L, feature->id);
					return 2;
				}

				continue;
			}

			const CUnit* unit = dynamic_cast<const CUnit*>(s);
			if (unit != nullptr) {
				if (CLuaHandle::GetHandleFullRead(L) || (unit->losStatus[CLuaHandle::GetHandleReadAllyTeam(L)] & LOS_INLOS)) {
					HSTR_PUSH(L, "unit");
					lua_pushnumber(L, unit->id);
					return 2;
				}

				continue;
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

	if (tti < 0 || tti >= CMapInfo::NUM_TERRAIN_TYPES)
		return 0;

	return (PushTerrainTypeData(L, &mapInfo->terrainTypes[tti], false));
}

int LuaSyncedRead::GetGrass(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1), 0.0f, luaL_checkfloat(L, 2));
	lua_pushnumber(L, grassDrawer->GetGrass(pos.cClampInBounds()));
	return 1;
}

/******************************************************************************/

int LuaSyncedRead::GetSmoothMeshHeight(lua_State* L)
{
	const float x = luaL_checkfloat(L, 1);
	const float z = luaL_checkfloat(L, 2);

	lua_pushnumber(L, smoothGround.GetHeight(x, z));
	return 1;
}

/******************************************************************************/
/******************************************************************************/


int LuaSyncedRead::TestMoveOrder(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(unitDefID);

	if (unitDef == nullptr || unitDef->pathType == -1u) {
		lua_pushboolean(L, false);
		return 1;
	}

	const MoveDef* moveDef = moveDefHandler.GetMoveDefByPathType(unitDef->pathType);

	if (moveDef == nullptr) {
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

	if (los)
		ret = moveDef->TestMoveSquare(nullptr, pos, dir, testTerrain, testObjects, centerOnly);

	lua_pushboolean(L, ret);
	return 1;
}

int LuaSyncedRead::TestBuildOrder(lua_State* L)
{
	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(unitDefID);

	if (unitDef == nullptr) {
		lua_pushnumber(L, 0);
		return 1;
	}

	BuildInfo bi;
	bi.buildFacing = LuaUtils::ParseFacing(L, __func__, 5);
	bi.def = unitDef;
	bi.pos = {luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4)};
	bi.pos = CGameHelper::Pos2BuildPos(bi, CLuaHandle::GetHandleSynced(L));
	CFeature* feature;

	// negative allyTeam values have full visibility in TestUnitBuildSquare()
	// 0 = BUILDSQUARE_BLOCKED
	// 1 = BUILDSQUARE_OCCUPIED
	// 2 = BUILDSQUARE_RECLAIMABLE
	// 3 = BUILDSQUARE_OPEN
	int retval = CGameHelper::TestUnitBuildSquare(bi, feature, CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleSynced(L));

	// the output of TestUnitBuildSquare was changed after this API function was written
	// keep backward-compability by mapping BUILDSQUARE_OPEN to BUILDSQUARE_RECLAIMABLE
	if (retval == CGameHelper::BUILDSQUARE_OPEN)
		retval = CGameHelper::BUILDSQUARE_RECLAIMABLE;

	if (feature == nullptr) {
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
	if (ud == nullptr)
		return 0;

	const float3 worldPos = {luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4)};
	const float3 buildPos = CGameHelper::Pos2BuildPos({ud, worldPos, luaL_optint(L, 5, FACING_SOUTH)}, CLuaHandle::GetHandleSynced(L));

	lua_pushnumber(L, buildPos.x);
	lua_pushnumber(L, buildPos.y);
	lua_pushnumber(L, buildPos.z);
	return 3;
}


int LuaSyncedRead::ClosestBuildPos(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	const int udefID = luaL_checkint(L, 2);

	const float searchRadius = luaL_checkfloat(L, 6);

	const int minDistance = luaL_checkfloat(L, 7);
	const int buildFacing = luaL_checkint(L, 8);

	const float3 worldPos = {luaL_checkfloat(L, 3), luaL_checkfloat(L, 4), luaL_checkfloat(L, 5)};
	const float3 buildPos = CGameHelper::ClosestBuildPos(teamID, unitDefHandler->GetUnitDefByID(udefID), worldPos, searchRadius, minDistance, buildFacing, CLuaHandle::GetHandleSynced(L));

	lua_pushnumber(L, buildPos.x);
	lua_pushnumber(L, buildPos.y);
	lua_pushnumber(L, buildPos.z);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

static int GetEffectiveLosAllyTeam(lua_State* L, int arg)
{
	if (lua_isnoneornil(L, arg))
		return (CLuaHandle::GetHandleReadAllyTeam(L));

	const int aat = luaL_optint(L, arg, CEventClient::MinSpecialTeam - 1);

	if (aat == CEventClient::NoAccessTeam)
		return aat;

	if (CLuaHandle::GetHandleFullRead(L)) {
		if (teamHandler.IsValidAllyTeam(aat))
			return aat;

		if (aat == CEventClient::AllAccessTeam)
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
	const CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 2);
	unsigned short losStatus;
	if (allyTeamID < 0) {
		losStatus = (allyTeamID == CEventClient::AllAccessTeam) ? (LOS_ALL_MASK_BITS | LOS_ALL_BITS) : 0;
	} else {
		losStatus = unit->losStatus[allyTeamID];
	}

	constexpr int currMask = LOS_INLOS   | LOS_INRADAR;
	constexpr int prevMask = LOS_PREVLOS | LOS_CONTRADAR;

	const bool isTyped = ((losStatus & prevMask) == prevMask);

	if (luaL_optboolean(L, 3, false)) {
		// return a numeric value
		if (!CLuaHandle::GetHandleFullRead(L))
			losStatus &= ((prevMask * isTyped) | currMask);

		lua_pushnumber(L, losStatus);
		return 1;
	}

	lua_createtable(L, 0, 3);
	if (losStatus & LOS_INLOS) {
		HSTR_PUSH_BOOL(L, "los", true);
	}
	if (losStatus & LOS_INRADAR) {
		HSTR_PUSH_BOOL(L, "radar", true);
	}
	if ((losStatus & LOS_INLOS) || isTyped) {
		HSTR_PUSH_BOOL(L, "typed", true);
	}
	return 1;
}


int LuaSyncedRead::IsUnitInLos(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
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
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
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
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
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
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
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
	HSTR_PUSH_STRING(L, "name", op.name);
	HSTR_PUSH_STRING(L, "parent", ((op.parent != nullptr) ? op.parent->name : "[null]"));

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

	for (float mi: mat.m) {
		lua_pushnumber(L, mi);
	}

	return 16;
}



int LuaSyncedRead::GetUnitPieceMap(lua_State* L) {
	return (GetSolidObjectPieceMap(L, ParseTypedUnit(L, __func__, 1)));
}
int LuaSyncedRead::GetUnitPieceList(lua_State* L) {
	return (GetSolidObjectPieceList(L, ParseTypedUnit(L, __func__, 1)));
}
int LuaSyncedRead::GetUnitPieceInfo(lua_State* L) {
	return (GetSolidObjectPieceInfo(L, ParseTypedUnit(L, __func__, 1)));
}
int LuaSyncedRead::GetUnitPiecePosDir(lua_State* L) {
	return (GetSolidObjectPiecePosDir(L, ParseTypedUnit(L, __func__, 1)));
}
int LuaSyncedRead::GetUnitPiecePosition(lua_State* L) {
	return (GetSolidObjectPiecePosition(L, ParseTypedUnit(L, __func__, 1)));
}
int LuaSyncedRead::GetUnitPieceDirection(lua_State* L) {
	return (GetSolidObjectPieceDirection(L, ParseTypedUnit(L, __func__, 1)));
}
int LuaSyncedRead::GetUnitPieceMatrix(lua_State* L) {
	return (GetSolidObjectPieceMatrix(L, ParseTypedUnit(L, __func__, 1)));
}


int LuaSyncedRead::GetFeaturePieceMap(lua_State* L) {
	return (GetSolidObjectPieceMap(L, ParseFeature(L, __func__, 1)));
}
int LuaSyncedRead::GetFeaturePieceList(lua_State* L) {
	return (GetSolidObjectPieceList(L, ParseFeature(L, __func__, 1)));
}
int LuaSyncedRead::GetFeaturePieceInfo(lua_State* L) {
	return (GetSolidObjectPieceInfo(L, ParseFeature(L, __func__, 1)));
}
int LuaSyncedRead::GetFeaturePiecePosDir(lua_State* L) {
	return (GetSolidObjectPiecePosDir(L, ParseFeature(L, __func__, 1)));
}
int LuaSyncedRead::GetFeaturePiecePosition(lua_State* L) {
	return (GetSolidObjectPiecePosition(L, ParseFeature(L, __func__, 1)));
}
int LuaSyncedRead::GetFeaturePieceDirection(lua_State* L) {
	return (GetSolidObjectPieceDirection(L, ParseFeature(L, __func__, 1)));
}
int LuaSyncedRead::GetFeaturePieceMatrix(lua_State* L) {
	return (GetSolidObjectPieceMatrix(L, ParseFeature(L, __func__, 1)));
}



int LuaSyncedRead::GetUnitScriptPiece(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

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
	if (piece < 0)
		return 0;

	lua_pushnumber(L, piece + 1);
	return 1;
}

int LuaSyncedRead::GetUnitScriptNames(lua_State* L)
{
	const CUnit* unit = ParseTypedUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const vector<LocalModelPiece*>& pieces = unit->script->pieces;

	lua_createtable(L, pieces.size(), 0);

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

	if (!teamHandler.IsValidAllyTeam(allyTeamID))
		return 0;

	if (IsAlliedAllyTeam(L, allyTeamID)) {
		lua_pushnumber(L, losHandler->GetAllyTeamRadarErrorSize(allyTeamID));
	} else {
		lua_pushnumber(L, losHandler->GetBaseRadarErrorSize());
	}
	lua_pushnumber(L, losHandler->GetBaseRadarErrorSize());
	lua_pushnumber(L, losHandler->GetBaseRadarErrorMult());
	return 3;
}


/******************************************************************************/
/******************************************************************************/
