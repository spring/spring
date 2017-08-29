/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <cctype>

#include "LuaSyncedCtrl.h"

#include "LuaInclude.h"
#include "LuaConfig.h"
#include "LuaRules.h" // for MAX_LUA_COB_ARGS
#include "LuaHandleSynced.h"
#include "LuaHashString.h"
#include "LuaMetalMap.h"
#include "LuaSyncedMoveCtrl.h"
#include "LuaUtils.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/Players/Player.h"
#include "Net/GameServer.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/BuildingMaskMap.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/StarburstProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/TorpedoProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitToolTipMap.hpp"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/Scripts/LuaUnitScript.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/UnitTypes/ExtractorBuilding.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponTarget.h"
#include "System/EventHandler.h"
#include "System/ObjectDependenceTypes.h"
#include "System/Log/ILog.h"

using std::max;
using std::min;


/******************************************************************************/
/******************************************************************************/

bool LuaSyncedCtrl::inCreateUnit = false;
bool LuaSyncedCtrl::inDestroyUnit = false;
bool LuaSyncedCtrl::inTransferUnit = false;
bool LuaSyncedCtrl::inCreateFeature = false;
bool LuaSyncedCtrl::inDestroyFeature = false;
bool LuaSyncedCtrl::inGiveOrder = false;
bool LuaSyncedCtrl::inHeightMap = false;
bool LuaSyncedCtrl::inSmoothMesh = false;

static int heightMapx1 = 0;
static int heightMapx2 = 0;
static int heightMapz1 = 0;
static int heightMapz2 = 0;

static float heightMapAmountChanged = 0.0f;
static float smoothMeshAmountChanged = 0.0f;


/******************************************************************************/

inline void LuaSyncedCtrl::CheckAllowGameChanges(lua_State* L)
{
	if (!CLuaHandle::GetHandleAllowChanges(L)) {
		luaL_error(L, "Unsafe attempt to change game state");
	}
}


/******************************************************************************/
/******************************************************************************/

bool LuaSyncedCtrl::PushEntries(lua_State* L)
{
	{
		// these need to be re-initialized here since we might have reloaded
		inCreateUnit = false;
		inDestroyUnit = false;
		inTransferUnit = false;
		inCreateFeature = false;
		inDestroyFeature = false;
		inGiveOrder = false;
		inHeightMap = false;
		inSmoothMesh = false;

		heightMapx1 = 0;
		heightMapx2 = 0;
		heightMapz1 = 0;
		heightMapz2 = 0;

		heightMapAmountChanged = 0.0f;
		smoothMeshAmountChanged = 0.0f;
	}


	REGISTER_LUA_CFUNC(SetAlly);
	REGISTER_LUA_CFUNC(KillTeam);
	REGISTER_LUA_CFUNC(AssignPlayerToTeam);
	REGISTER_LUA_CFUNC(GameOver);
	REGISTER_LUA_CFUNC(SetGlobalLos);

	REGISTER_LUA_CFUNC(AddTeamResource);
	REGISTER_LUA_CFUNC(UseTeamResource);
	REGISTER_LUA_CFUNC(SetTeamResource);
	REGISTER_LUA_CFUNC(SetTeamShareLevel);
	REGISTER_LUA_CFUNC(ShareTeamResource);

	REGISTER_LUA_CFUNC(SetGameRulesParam);
	REGISTER_LUA_CFUNC(SetTeamRulesParam);
	REGISTER_LUA_CFUNC(SetUnitRulesParam);
	REGISTER_LUA_CFUNC(SetFeatureRulesParam);

	REGISTER_LUA_CFUNC(CreateUnit);
	REGISTER_LUA_CFUNC(DestroyUnit);
	REGISTER_LUA_CFUNC(TransferUnit);

	REGISTER_LUA_CFUNC(CreateFeature);
	REGISTER_LUA_CFUNC(DestroyFeature);
	REGISTER_LUA_CFUNC(TransferFeature);

	REGISTER_LUA_CFUNC(CreateUnitIKChain);
	REGISTER_LUA_CFUNC(SetUnitIKActive);
	REGISTER_LUA_CFUNC(SetUnitIKGoal);
	REGISTER_LUA_CFUNC(SetUnitIKPieceLimits);
	REGISTER_LUA_CFUNC(SetUnitIKPieceSpeed);

	REGISTER_LUA_CFUNC(SetUnitCosts);
	REGISTER_LUA_CFUNC(SetUnitResourcing);
	REGISTER_LUA_CFUNC(SetUnitTooltip);
	REGISTER_LUA_CFUNC(SetUnitHealth);
	REGISTER_LUA_CFUNC(SetUnitMaxHealth);
	REGISTER_LUA_CFUNC(SetUnitStockpile);
	REGISTER_LUA_CFUNC(SetUnitWeaponState);
	REGISTER_LUA_CFUNC(SetUnitWeaponDamages);
	REGISTER_LUA_CFUNC(SetUnitMaxRange);
	REGISTER_LUA_CFUNC(SetUnitExperience);
	REGISTER_LUA_CFUNC(SetUnitArmored);
	REGISTER_LUA_CFUNC(SetUnitLosMask);
	REGISTER_LUA_CFUNC(SetUnitLosState);
	REGISTER_LUA_CFUNC(SetUnitCloak);
	REGISTER_LUA_CFUNC(SetUnitStealth);
	REGISTER_LUA_CFUNC(SetUnitSonarStealth);
	REGISTER_LUA_CFUNC(SetUnitAlwaysVisible);
	REGISTER_LUA_CFUNC(SetUnitMetalExtraction);
	REGISTER_LUA_CFUNC(SetUnitHarvestStorage);
	REGISTER_LUA_CFUNC(SetUnitBuildSpeed);
	REGISTER_LUA_CFUNC(SetUnitNanoPieces);

	REGISTER_LUA_CFUNC(SetUnitBlocking);
	REGISTER_LUA_CFUNC(SetUnitCrashing);
	REGISTER_LUA_CFUNC(SetUnitShieldState);
	REGISTER_LUA_CFUNC(SetUnitFlanking);
	REGISTER_LUA_CFUNC(SetUnitTravel);
	REGISTER_LUA_CFUNC(SetUnitFuel);
	REGISTER_LUA_CFUNC(SetUnitMoveGoal);
	REGISTER_LUA_CFUNC(SetUnitLandGoal);
	REGISTER_LUA_CFUNC(ClearUnitGoal);
	REGISTER_LUA_CFUNC(SetUnitNeutral);
	REGISTER_LUA_CFUNC(SetUnitTarget);
	REGISTER_LUA_CFUNC(SetUnitMidAndAimPos);
	REGISTER_LUA_CFUNC(SetUnitRadiusAndHeight);

	REGISTER_LUA_CFUNC(SetUnitCollisionVolumeData);
	REGISTER_LUA_CFUNC(SetUnitPieceCollisionVolumeData);
	REGISTER_LUA_CFUNC(SetUnitPieceParent);
	REGISTER_LUA_CFUNC(SetUnitPieceMatrix);
	REGISTER_LUA_CFUNC(SetUnitSensorRadius);
	REGISTER_LUA_CFUNC(SetUnitPosErrorParams);
	REGISTER_LUA_CFUNC(SetUnitPhysics);
	REGISTER_LUA_CFUNC(SetUnitMass);
	REGISTER_LUA_CFUNC(SetUnitPosition);
	REGISTER_LUA_CFUNC(SetUnitVelocity);
	REGISTER_LUA_CFUNC(SetUnitRotation);
	REGISTER_LUA_CFUNC(SetUnitDirection);

	REGISTER_LUA_CFUNC(AddUnitDamage);
	REGISTER_LUA_CFUNC(AddUnitImpulse);
	REGISTER_LUA_CFUNC(AddUnitSeismicPing);

	REGISTER_LUA_CFUNC(AddUnitResource);
	REGISTER_LUA_CFUNC(UseUnitResource);

	REGISTER_LUA_CFUNC(RemoveBuildingDecal);
	REGISTER_LUA_CFUNC(AddGrass);
	REGISTER_LUA_CFUNC(RemoveGrass);

	REGISTER_LUA_CFUNC(SetFeatureAlwaysVisible);
	REGISTER_LUA_CFUNC(SetFeatureHealth);
	REGISTER_LUA_CFUNC(SetFeatureMaxHealth);
	REGISTER_LUA_CFUNC(SetFeatureReclaim);
	REGISTER_LUA_CFUNC(SetFeatureResources);
	REGISTER_LUA_CFUNC(SetFeatureResurrect);

	REGISTER_LUA_CFUNC(SetFeatureMoveCtrl);
	REGISTER_LUA_CFUNC(SetFeaturePhysics);
	REGISTER_LUA_CFUNC(SetFeatureMass);
	REGISTER_LUA_CFUNC(SetFeaturePosition);
	REGISTER_LUA_CFUNC(SetFeatureVelocity);
	REGISTER_LUA_CFUNC(SetFeatureRotation);
	REGISTER_LUA_CFUNC(SetFeatureDirection);

	REGISTER_LUA_CFUNC(SetFeatureBlocking);
	REGISTER_LUA_CFUNC(SetFeatureNoSelect);
	REGISTER_LUA_CFUNC(SetFeatureMidAndAimPos);
	REGISTER_LUA_CFUNC(SetFeatureRadiusAndHeight);
	REGISTER_LUA_CFUNC(SetFeatureCollisionVolumeData);
	REGISTER_LUA_CFUNC(SetFeaturePieceCollisionVolumeData);


	REGISTER_LUA_CFUNC(SetProjectileAlwaysVisible);
	REGISTER_LUA_CFUNC(SetProjectileMoveControl);
	REGISTER_LUA_CFUNC(SetProjectilePosition);
	REGISTER_LUA_CFUNC(SetProjectileVelocity);
	REGISTER_LUA_CFUNC(SetProjectileCollision);
	REGISTER_LUA_CFUNC(SetProjectileTarget);
	REGISTER_LUA_CFUNC(SetProjectileIsIntercepted);
	REGISTER_LUA_CFUNC(SetProjectileDamages);
	REGISTER_LUA_CFUNC(SetProjectileIgnoreTrackingError);

	REGISTER_LUA_CFUNC(SetProjectileGravity);
	REGISTER_LUA_CFUNC(SetProjectileSpinAngle);
	REGISTER_LUA_CFUNC(SetProjectileSpinSpeed);
	REGISTER_LUA_CFUNC(SetProjectileSpinVec);
	REGISTER_LUA_CFUNC(SetPieceProjectileParams);

	REGISTER_LUA_CFUNC(SetProjectileCEG);


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

	REGISTER_LUA_CFUNC(LevelSmoothMesh);
	REGISTER_LUA_CFUNC(AdjustSmoothMesh);
	REGISTER_LUA_CFUNC(RevertSmoothMesh);

	REGISTER_LUA_CFUNC(AddSmoothMesh);
	REGISTER_LUA_CFUNC(SetSmoothMesh);
	REGISTER_LUA_CFUNC(SetSmoothMeshFunc);

	REGISTER_LUA_CFUNC(SetMapSquareTerrainType);
	REGISTER_LUA_CFUNC(SetTerrainTypeData);

	REGISTER_LUA_CFUNC(SetSquareBuildingMask);

	REGISTER_LUA_CFUNC(UnitWeaponFire);
	REGISTER_LUA_CFUNC(UnitWeaponHoldFire);

	REGISTER_LUA_CFUNC(UnitAttach);
	REGISTER_LUA_CFUNC(UnitDetach);
	REGISTER_LUA_CFUNC(UnitDetachFromAir);
	REGISTER_LUA_CFUNC(SetUnitLoadingTransport);

	REGISTER_LUA_CFUNC(SpawnProjectile);
	REGISTER_LUA_CFUNC(DeleteProjectile);
	REGISTER_LUA_CFUNC(SpawnExplosion);
	REGISTER_LUA_CFUNC(SpawnCEG);
	REGISTER_LUA_CFUNC(SpawnSFX);

	REGISTER_LUA_CFUNC(EditUnitCmdDesc);
	REGISTER_LUA_CFUNC(InsertUnitCmdDesc);
	REGISTER_LUA_CFUNC(RemoveUnitCmdDesc);

	REGISTER_LUA_CFUNC(SetNoPause);
	REGISTER_LUA_CFUNC(SetUnitToFeature);
	REGISTER_LUA_CFUNC(SetExperienceGrade);

	REGISTER_LUA_CFUNC(SetRadarErrorParams);

	if (!LuaSyncedMoveCtrl::PushMoveCtrl(L))
		return false;

	if (!CLuaUnitScript::PushEntries(L))
		return false;

	if (!LuaMetalMap::PushCtrlEntries(L))
		return false;

	return true;
}




/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	return (unitHandler.GetUnit(luaL_checkint(L, index)));
}

static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;
	if (!CanControlUnit(L, unit))
		return nullptr;

	return unit;
}

static inline CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	CFeature* f = featureHandler.GetFeature(luaL_checkint(L, index));

	if (f == nullptr)
		return nullptr;
	if (!CanControlFeature(L, f))
		return nullptr;

	return f;
}

static inline CProjectile* ParseProjectile(lua_State* L, const char* caller, int index)
{
	CProjectile* p = projectileHandler.GetProjectileBySyncedID(luaL_checkint(L, index));

	if (p == nullptr)
		return nullptr;

	if (!CanControlProjectileAllyTeam(L, p->GetAllyteamID()))
		return nullptr;

	return p;
}

static bool ParseProjectileParams(lua_State* L, ProjectileParams& params, const int tblIdx, const char* caller)
{
	if (!lua_istable(L, tblIdx)) {
		luaL_error(L, "[%s] argument %d must be a table!", caller, tblIdx);
		return false;
	}

	params.teamID = teamHandler.GaiaTeamID();

	for (lua_pushnil(L); lua_next(L, tblIdx) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const char* key = lua_tostring(L, -2);

		if (lua_istable(L, -1)) {
			float array[3] = {0.0f, 0.0f, 0.0f};

			if (LuaUtils::ParseFloatArray(L, -1, array, 3) == 3) {
				switch (hashString(key)) {
					case hashString("pos"   ): { params.pos    = array; } break;
					case hashString("end"   ): { params.end    = array; } break;
					case hashString("speed" ): { params.speed  = array; } break;
					case hashString("spread"): { params.spread = array; } break;
					case hashString("error" ): { params.error  = array; } break;
				}
			}

			continue;
		}

		if (lua_isnumber(L, -1)) {
			switch (hashString(key)) {
				case hashString("owner" ): { params.ownerID   = lua_toint(L, -1)                        ; } break;
				case hashString("weapon"): { params.weaponNum = lua_toint(L, -1) - LUA_WEAPON_BASE_INDEX; } break;
				case hashString("team"  ): { params.teamID    = lua_toint(L, -1)                        ; } break;

				case hashString("ttl"): { params.ttl = lua_tofloat(L, -1); } break;

				case hashString("gravity" ): { params.gravity  = lua_tofloat(L, -1); } break;
				case hashString("tracking"): { params.tracking = lua_tofloat(L, -1); } break;

				case hashString("maxRange"): { params.maxRange = lua_tofloat(L, -1); } break;
				case hashString("upTime"  ): { params.upTime   = lua_tofloat(L, -1); } break;

				case hashString("startAlpha"): { params.startAlpha = lua_tofloat(L, -1); } break;
				case hashString("endAlpha"  ): { params.endAlpha   = lua_tofloat(L, -1); } break;
			}

			continue;
		}

		if (lua_isstring(L, -1)) {
			switch (hashString(key)) {
				case hashString("model"): {
					params.model = modelLoader.LoadModel(lua_tostring(L, -1));
				} break;
				case hashString("cegtag"): {
					params.cegID = explGenHandler.LoadGeneratorID(lua_tostring(L, -1));
				} break;
			}

			continue;
		}
	}

	return true;
}

static CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad teamID", caller);
		return nullptr;
	}

	const int teamID = lua_toint(L, index);

	if (!teamHandler.IsValidTeam(teamID))
		luaL_error(L, "%s(): Bad teamID: %d", caller, teamID);


	return (teamHandler.Team(teamID));
}

static int SetSolidObjectCollisionVolumeData(lua_State* L, CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	return LuaUtils::ParseColVolData(L, 2, &o->collisionVolume);
}

static int SetSolidObjectBlocking(lua_State* L, CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	// update SO-bit of collidable state
	if (lua_isboolean(L, 3)) {
		if (lua_toboolean(L, 3)) {
			o->SetCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS);
		} else {
			o->ClearCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS);
		}
	}

	// update blocking-bit of physical state (do this
	// after changing the SO-bit so it is reversable)
	if (lua_isboolean(L, 2)) {
		if (lua_toboolean(L, 2)) {
			o->Block();
		} else {
			o->UnBlock();
		}
	}

	o->UpdateCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES, luaL_optboolean(L, 4, o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES)));
	o->UpdateCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS, luaL_optboolean(L, 5, o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS)));

	o->crushable = luaL_optboolean(L, 6, o->crushable);
	o->blockEnemyPushing = luaL_optboolean(L, 7, o->blockEnemyPushing);
	o->blockHeightChanges = luaL_optboolean(L, 8, o->blockHeightChanges);

	lua_pushboolean(L, o->IsBlocking());
	return 1;
}

static int SetSolidObjectRotation(lua_State* L, CSolidObject* o, bool isFeature)
{
	if (o == nullptr)
		return 0;

	o->SetDirVectorsEuler(float3(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4)));

	if (isFeature) {
		// not a hack: ForcedSpin() and CalculateTransform() calculate a
		// transform based only on frontdir and assume the helper y-axis
		// points up
		static_cast<CFeature*>(o)->UpdateTransform(o->pos, true);
	}

	return 0;
}

static int SetSolidObjectDirection(lua_State* L, CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	o->ForcedSpin((float3(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4))).SafeNormalize());
	return 0;
}

static int SetWorldObjectVelocity(lua_State* L, CWorldObject* o)
{
	if (o == nullptr)
		return 0;

	float3 speed;
	speed.x = Clamp(luaL_checkfloat(L, 2), -MAX_UNIT_SPEED, MAX_UNIT_SPEED);
	speed.y = Clamp(luaL_checkfloat(L, 3), -MAX_UNIT_SPEED, MAX_UNIT_SPEED);
	speed.z = Clamp(luaL_checkfloat(L, 4), -MAX_UNIT_SPEED, MAX_UNIT_SPEED);

	o->SetVelocityAndSpeed(speed);
	return 0;
}

static int SetSolidObjectMass(lua_State* L, CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	o->SetMass(luaL_checknumber(L, 2));

	return 0;
}

static int SetSolidObjectPhysicalState(lua_State* L, CSolidObject* o)
{
	if (o == nullptr)
		return 0;

	float3 pos;
	float3 rot;
	float3 speed;
	float3& drag = o->dragScales;

	pos.x = luaL_checknumber(L, 2);
	pos.y = luaL_checknumber(L, 3);
	pos.z = luaL_checknumber(L, 4);

	speed.x = luaL_checknumber(L, 5);
	speed.y = luaL_checknumber(L, 6);
	speed.z = luaL_checknumber(L, 7);

	rot.x = luaL_checknumber(L, 8);
	rot.y = luaL_checknumber(L, 9);
	rot.z = luaL_checknumber(L, 10);

	drag.x = Clamp(luaL_optnumber(L, 11, drag.x), 0.0f, 1.0f);
	drag.y = Clamp(luaL_optnumber(L, 12, drag.y), 0.0f, 1.0f);
	drag.z = Clamp(luaL_optnumber(L, 13, drag.z), 0.0f, 1.0f);

	o->Move(pos, false);
	o->SetDirVectorsEuler(rot);
	// do not need ForcedSpin, above three calls cover it
	o->ForcedMove(pos);
	o->SetVelocityAndSpeed(speed);
	return 0;
}

static int SetSolidObjectPieceCollisionVolumeData(lua_State* L, CSolidObject* obj)
{
	if (obj == nullptr)
		return 0;

	LocalModelPiece* lmp = ParseObjectLocalModelPiece(L, obj, 2);

	if (lmp == nullptr)
		luaL_argerror(L, 2, "invalid piece");

	CollisionVolume* vol = lmp->GetCollisionVolume();

	const float3 scales(luaL_checkfloat(L, 4), luaL_checkfloat(L, 5), luaL_checkfloat(L, 6));
	const float3 offset(luaL_checkfloat(L, 7), luaL_checkfloat(L, 8), luaL_checkfloat(L, 9));

	const unsigned int vType = luaL_optint(L, 10, vol->GetVolumeType());
	const unsigned int pAxis = luaL_optint(L, 11, vol->GetPrimaryAxis());

	// piece volumes are not allowed to use discrete hit-testing
	vol->InitShape(scales, offset, vType, CollisionVolume::COLVOL_HITTEST_CONT, pAxis);
	vol->SetIgnoreHits(!luaL_checkboolean(L, 3));
	return 0;
}

static int SetWorldObjectAlwaysVisible(lua_State* L, CWorldObject* o, const char* caller)
{
	if (o == nullptr)
		return 0;
	o->alwaysVisible = luaL_checkboolean(L, 2);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

static inline bool IsPlayerSynced(const CPlayer* player)
{
	return (!gameSetup->hostDemo || player->isFromDemo);
}

/******************************************************************************/
/******************************************************************************/
//
// The call-outs
//

int LuaSyncedCtrl::SetAlly(lua_State* L)
{
	const int firstAllyTeamID = luaL_checkint(L, 1);
	const int secondAllyTeamID = luaL_checkint(L, 2);

	if (!teamHandler.IsValidAllyTeam(firstAllyTeamID))
		return 0;
	if (!teamHandler.IsValidAllyTeam(secondAllyTeamID))
		return 0;

	teamHandler.SetAlly(firstAllyTeamID, secondAllyTeamID, luaL_checkboolean(L, 3));
	return 0;
}

int LuaSyncedCtrl::KillTeam(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);

	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	//FIXME either we disallow it here or it needs modifications in GameServer.cpp (it creates a `teams` vector w/o gaia)
	//  possible fix would be to always create the Gaia team (currently it's conditional on gs->useLuaGaia)
	if (teamID == teamHandler.GaiaTeamID())
		return 0;

	CTeam* team = teamHandler.Team(teamID);

	if (team == nullptr)
		return 0;

	team->Died();
	return 0;
}


int LuaSyncedCtrl::AssignPlayerToTeam(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	const int teamID = luaL_checkint(L, 2);

	const CPlayer* player = playerHandler.IsValidPlayer(playerID)? playerHandler.Player(playerID): nullptr;

	if (player == nullptr)
		return 0;

	if (!IsPlayerSynced(player))
		return 0;

	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	teamHandler.Team(teamID)->AddPlayer(playerID);
	return 0;
}


int LuaSyncedCtrl::GameOver(lua_State* L)
{
	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to GameOver()");
		return 0;
	}

	std::vector<unsigned char> winningAllyTeams;

	static const int tableIdx = 1;

	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -1))
			continue;

		const unsigned char allyTeamID = lua_toint(L, -1);

		if (!teamHandler.ValidAllyTeam(allyTeamID))
			continue;

		winningAllyTeams.push_back(allyTeamID);
	}

	game->GameEnd(winningAllyTeams);
	// push the number of accepted allyteams
	lua_pushnumber(L, winningAllyTeams.size());
	return 1;
}


int LuaSyncedCtrl::SetGlobalLos(lua_State* L)
{
	const int allyTeam = luaL_checkint(L, 1);

	if (!teamHandler.IsValidAllyTeam(allyTeam))
		luaL_error(L, "bad allyTeam");

	losHandler->globalLOS[allyTeam] = luaL_checkboolean(L, 2);
	return 0;
}




int LuaSyncedCtrl::AddTeamResource(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);

	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	if (!CanControlTeam(L, teamID))
		return 0;

	CTeam* team = teamHandler.Team(teamID);

	if (team == nullptr)
		return 0;

	const char* type = luaL_checkstring(L, 2);

	const float value = max(0.0f, luaL_checkfloat(L, 3));

	switch (type[0]) {
		case 'm': { team->AddMetal (value); } break;
		case 'e': { team->AddEnergy(value); } break;
		default : {                         } break;
	}

	return 0;
}


int LuaSyncedCtrl::UseTeamResource(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);

	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	if (!CanControlTeam(L, teamID))
		return 0;

	CTeam* team = teamHandler.Team(teamID);

	if (team == nullptr)
		return 0;

	if (lua_isstring(L, 2)) {
		const char* type = lua_tostring(L, 2);

		const float value = std::max(0.0f, luaL_checkfloat(L, 3));

		switch (type[0]) {
			case 'm': {
				team->resPull.metal += value;
				lua_pushboolean(L, team->UseMetal(value));
				return 1;
			} break;
			case 'e': {
				team->resPull.energy += value;
				lua_pushboolean(L, team->UseEnergy(value));
				return 1;
			} break;
			default: {
			} break;
		}

		return 0;
	}

	if (lua_istable(L, 2)) {
		float metal  = 0.0f;
		float energy = 0.0f;

		constexpr int tableIdx = 2;

		for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
				continue;

			const char* key = lua_tostring(L, -2);
			const float value = lua_tofloat(L, -1);

			switch (key[0]) {
				case 'm': { metal  = std::max(0.0f, value); } break;
				case 'e': { energy = std::max(0.0f, value); } break;
				default : {                                 } break;
			}
		}

		team->resPull.metal  += metal;
		team->resPull.energy += energy;

		if ((team->res.metal >= metal) && (team->res.energy >= energy)) {
			team->UseMetal(metal);
			team->UseEnergy(energy);
			lua_pushboolean(L, true);
		} else {
			lua_pushboolean(L, false);
		}

		return 1;
	}

	luaL_error(L, "bad arguments");
	return 0;
}


int LuaSyncedCtrl::SetTeamResource(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);

	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	if (!CanControlTeam(L, teamID))
		return 0;

	CTeam* team = teamHandler.Team(teamID);

	if (team == nullptr)
		return 0;

	const float value = std::max(0.0f, luaL_checkfloat(L, 3));

	switch (hashString(luaL_checkstring(L, 2))) {
		case hashString("m"):
		case hashString("metal"): {
			team->res.metal = std::min<float>(team->resStorage.metal, value);
		} break;

		case hashString("e"):
		case hashString("energy"): {
			team->res.energy = std::min<float>(team->resStorage.energy, value);
		} break;

		case hashString("ms"):
		case hashString("metalStorage"): {
			team->res.metal = std::min<float>(team->res.metal, team->resStorage.metal = value);
		} break;

		case hashString("es"):
		case hashString("energyStorage"): {
			team->res.energy = std::min<float>(team->res.energy, team->resStorage.energy = value);
		} break;
	}

	return 0;
}


int LuaSyncedCtrl::SetTeamShareLevel(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);

	if (!teamHandler.IsValidTeam(teamID))
		return 0;

	if (!CanControlTeam(L, teamID))
		return 0;

	CTeam* team = teamHandler.Team(teamID);

	if (team == nullptr)
		return 0;

	const char* type = luaL_checkstring(L, 2);

	const float value = luaL_checkfloat(L, 3);

	switch (type[0]) {
		case 'm': { team->resShare.metal  = Clamp(value, 0.0f, 1.0f); } break;
		case 'e': { team->resShare.energy = Clamp(value, 0.0f, 1.0f); } break;
		default : {                                                   } break;
	}

	return 0;
}


int LuaSyncedCtrl::ShareTeamResource(lua_State* L)
{
	const int teamID1 = luaL_checkint(L, 1);

	if (!teamHandler.IsValidTeam(teamID1))
		luaL_error(L, "Incorrect arguments to ShareTeamResource(teamID1, teamID2, type, amount)");

	if (!CanControlTeam(L, teamID1))
		return 0;

	CTeam* team1 = teamHandler.Team(teamID1);

	if (team1 == nullptr)
		return 0;

	const int teamID2 = luaL_checkint(L, 2);

	if (!teamHandler.IsValidTeam(teamID2))
		luaL_error(L, "Incorrect arguments to ShareTeamResource(teamID1, teamID2, type, amount)");

	CTeam* team2 = teamHandler.Team(teamID2);

	if (team2 == nullptr)
		return 0;

	const char* type = luaL_checkstring(L, 3);
	float amount = luaL_checkfloat(L, 4);

	switch (type[0]) {
		case 'm': {
			amount = std::min(amount, (float)team1->res.metal);

			if (eventHandler.AllowResourceTransfer(teamID1, teamID2, "m", amount)) { //FIXME can cause an endless loop
				team1->res.metal                       -= amount;
				team1->resSent.metal                   += amount;
				team1->GetCurrentStats().metalSent     += amount;
				team2->res.metal                       += amount;
				team2->resReceived.metal               += amount;
				team2->GetCurrentStats().metalReceived += amount;
			}
		} break;
		case 'e': {
			amount = std::min(amount, (float)team1->res.energy);

			if (eventHandler.AllowResourceTransfer(teamID1, teamID2, "e", amount)) { //FIXME can cause an endless loop
				team1->res.energy                       -= amount;
				team1->resSent.energy                   += amount;
				team1->GetCurrentStats().energySent     += amount;
				team2->res.energy                       += amount;
				team2->resReceived.energy               += amount;
				team2->GetCurrentStats().energyReceived += amount;
			}
		} break;
		default: {
		} break;
	}

	return 0;
}



/******************************************************************************/

void SetRulesParam(lua_State* L, const char* caller, int offset,
				LuaRulesParams::Params& params)
{
	const int index = offset + 1;
	const int valIndex = offset + 2;
	const int losIndex = offset + 3; // table

	const std::string& key = luaL_checkstring(L, index);

	LuaRulesParams::Param& param = params[key];

	// set the value of the parameter
	if (lua_isnumber(L, valIndex)) {
		param.valueInt = lua_tofloat(L, valIndex);
		param.valueString.resize(0);
	} else if (lua_isstring(L, valIndex)) {
		param.valueString = lua_tostring(L, valIndex);
	} else if (lua_isnoneornil(L, valIndex)) {
		params.erase(key);
		return; //no need to set los if param was erased
	} else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	// set the los checking of the parameter
	if (lua_istable(L, losIndex)) {
		int losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE;

		for (lua_pushnil(L); lua_next(L, losIndex) != 0; lua_pop(L, 1)) {
			// ignore if the value is false
			if (!luaL_optboolean(L, -1, true))
				continue;

			// read the losType from the key
			if (!lua_isstring(L, -2))
				continue;

			switch (hashString(lua_tostring(L, -2))) {
				case hashString("public" ): { losMask |= LuaRulesParams::RULESPARAMLOS_PUBLIC;  } break;
				case hashString("inlos"  ): { losMask |= LuaRulesParams::RULESPARAMLOS_INLOS;   } break;
				case hashString("typed"  ): { losMask |= LuaRulesParams::RULESPARAMLOS_TYPED;   } break;
				case hashString("inradar"): { losMask |= LuaRulesParams::RULESPARAMLOS_INRADAR; } break;
				case hashString("allied" ): { losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED;  } break;
				// case hashString("private"): { losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE; } break;
				default                   : {                                                   } break;
			}
		}

		param.los = losMask;
	} else {
		param.los = luaL_optint(L, losIndex, param.los);
	}

	return;
}


int LuaSyncedCtrl::SetGameRulesParam(lua_State* L)
{
	SetRulesParam(L, __func__, 0, CSplitLuaHandle::gameParams);
	return 0;
}


int LuaSyncedCtrl::SetTeamRulesParam(lua_State* L)
{
	CTeam* team = ParseTeam(L, __func__, 1);
	if (team == nullptr)
		return 0;

	SetRulesParam(L, __func__, 1, team->modParams);
	return 0;
}


int LuaSyncedCtrl::SetUnitRulesParam(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	SetRulesParam(L, __func__, 1, unit->modParams);
	return 0;
}


int LuaSyncedCtrl::SetFeatureRulesParam(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr)
		return 0;

	SetRulesParam(L, __func__, 1, feature->modParams);
	return 0;
}

/******************************************************************************/
/******************************************************************************/

static inline void ParseCobArgs(
	lua_State* L,
	int first,
	int last,
	std::array<int, 1 + MAX_COB_ARGS>& args
) {
	args[0] = 0;

	for (int a = first; a <= last; a++) {
		if (lua_isnumber(L, a)) {
			args[1 + (args[0]++)] = lua_toint(L, a);
			continue;
		}
		if (lua_istable(L, a)) {
			lua_rawgeti(L, a, 1);
			lua_rawgeti(L, a, 2);

			if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
				const int x = lua_toint(L, -2);
				const int z = lua_toint(L, -1);

				args[1 + (args[0]++)] = PACKXZ(x, z);
			} else {
				args[1 + (args[0]++)] = 0;
			}

			lua_pop(L, 2);
			continue;
		}
		if (lua_isboolean(L, a)) {
			args[1 + (args[0]++)] = int(lua_toboolean(L, a));
			continue;
		}

		args[1 + (args[0]++)] = 0;
	}
}


int LuaSyncedCtrl::CallCOBScript(lua_State* L)
{
//FIXME?	CheckAllowGameChanges(L);
	const int numArgs = lua_gettop(L);

	if (numArgs < 3)
		luaL_error(L, "[%s] too few arguments", __func__);
	if (numArgs > MAX_COB_ARGS)
		luaL_error(L, "[%s] too many arguments", __func__);

	// unitID
	if (!lua_isnumber(L, 1))
		luaL_error(L, "[%s] unitID not a number", __func__);
	// number of returned parameters
	if (!lua_isnumber(L, 3))
		luaL_error(L, "[%s] retval-count not a number", __func__);

	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	CCobInstance* cob = dynamic_cast<CCobInstance*>(unit->script);

	if (cob == nullptr)
		luaL_error(L, "[%s] unit is not running a COB script", __func__);

	static_assert(MAX_LUA_COB_ARGS <= MAX_COB_ARGS, "");

	// collect the arguments; cobArgs[0] holds the final count
	std::array<int, 1 + MAX_COB_ARGS> cobArgs;
	ParseCobArgs(L, 4, numArgs, cobArgs);

	const int numRetVals = std::min(lua_toint(L, 3), std::min(MAX_LUA_COB_ARGS, cobArgs[0]));
	int retCode = 0;

	if (lua_israwnumber(L, 2)) {
		retCode = cob->RawCall(lua_toint(L, 2), cobArgs);
	} else if (lua_israwstring(L, 2)) {
		retCode = cob->Call(lua_tostring(L, 2), cobArgs);
	} else {
		luaL_error(L, "[%s] bad function id or name", __func__);
	}

	lua_settop(L, 0); // FIXME - ok?
	lua_pushnumber(L, retCode);
	for (int i = 0; i < numRetVals; i++) {
		lua_pushnumber(L, cobArgs[i]);
	}
	return (1 + numRetVals);
}


int LuaSyncedCtrl::GetCOBScriptID(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments

	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2))
		luaL_error(L, "[%s] incorrect arguments", __func__);

	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	CCobInstance* cob = dynamic_cast<CCobInstance*>(unit->script);

	// no error - allows using this to determine whether unit runs COB or LUS
	if (cob == nullptr)
		return 0;

	const int funcID = cob->GetFunctionId(lua_tostring(L, 2));

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

	if (inCreateUnit) {
		luaL_error(L, "[%s()]: recursion is not permitted", __func__);
		return 0;
	}

	const UnitDef* unitDef = nullptr;

	if (lua_israwstring(L, 1)) {
		unitDef = unitDefHandler->GetUnitDefByName(lua_tostring(L, 1));
	} else if (lua_israwnumber(L, 1)) {
		unitDef = unitDefHandler->GetUnitDefByID(lua_toint(L, 1));
	} else {
		luaL_error(L, "[%s()] incorrect type for first argument", __func__);
		return 0;
	}

	if (unitDef == nullptr) {
		if (lua_israwstring(L, 1)) {
			luaL_error(L, "[%s()]: bad unitDef name: %s", __func__, lua_tostring(L, 1));
		} else {
			luaL_error(L, "[%s()]: bad unitDef ID: %d", __func__, lua_toint(L, 1));
		}
		return 0;
	}

	// CUnit::PreInit will clamp the position
	// TODO: also allow off-map unit creation?
	const float3 pos(
		luaL_checkfloat(L, 2),
		luaL_checkfloat(L, 3),
		luaL_checkfloat(L, 4)
	);
	const int facing = LuaUtils::ParseFacing(L, __func__, 5);
	const int teamID = luaL_optint(L, 6, CtrlTeam(L));

	const bool beingBuilt = luaL_optboolean(L, 7, false);
	const bool flattenGround = luaL_optboolean(L, 8, true);

	if (!teamHandler.IsValidTeam(teamID)) {
		luaL_error(L, "[%s()]: invalid team number (%d)", __func__, teamID);
		return 0;
	}
	if (!FullCtrl(L) && (CtrlTeam(L) != teamID)) {
		luaL_error(L, "[%s()]: not a controllable team (%d)", __func__, teamID);
		return 0;
	}
	if (!unitHandler.CanBuildUnit(unitDef, teamID))
		return 0; // unit limit reached

	ASSERT_SYNCED(pos);
	ASSERT_SYNCED(facing);

	inCreateUnit = true;

	CUnit* builder = unitHandler.GetUnit(luaL_optint(L, 10, -1));

	UnitLoadParams params;
	params.unitDef = unitDef; /// must be non-NULL
	params.builder = builder; /// may be NULL
	params.pos     = pos;
	params.speed   = ZeroVector;
	params.unitID  = luaL_optint(L, 9, -1);
	params.teamID  = teamID;
	params.facing  = facing;
	params.beingBuilt = beingBuilt;
	params.flattenGround = flattenGround;

	CUnit* unit = unitLoader->LoadUnit(params);
	inCreateUnit = false;

	if (unit == nullptr)
		return 0;

	unit->SetSoloBuilder(builder, unitDef);

	lua_pushnumber(L, unit->id);
	return 1;
}


int LuaSyncedCtrl::DestroyUnit(lua_State* L)
{
	CheckAllowGameChanges(L); // FIXME -- recursion protection
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int args = lua_gettop(L); // number of arguments

	const bool selfDestr = luaL_optboolean(L, 2, false);
	const bool reclaimed = luaL_optboolean(L, 3, false);
	const bool recycleID = luaL_optboolean(L, 5, false);

	CUnit* attacker = nullptr;
	if (args >= 4)
		attacker = ParseUnit(L, __func__, 4);

	if (inDestroyUnit)
		luaL_error(L, "DestroyUnit() recursion is not permitted");

	inDestroyUnit = true;

	ASSERT_SYNCED(unit->id);
	unit->ForcedKillUnit(attacker, selfDestr, reclaimed);

	if (recycleID)
		unitHandler.GarbageCollectUnit(unit->id);

	inDestroyUnit = false;

	return 0;
}


int LuaSyncedCtrl::TransferUnit(lua_State* L)
{
	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int newTeam = luaL_checkint(L, 2);
	if (!teamHandler.IsValidTeam(newTeam))
		return 0;

	const CTeam* team = teamHandler.Team(newTeam);
	if (team == nullptr)
		return 0;

	bool given = true;
	if (FullCtrl(L) && lua_isboolean(L, 3))
		given = lua_toboolean(L, 3);

	if (inTransferUnit)
		luaL_error(L, "TransferUnit() recursion is not permitted");

	inTransferUnit = true;
	ASSERT_SYNCED(unit->id);
	ASSERT_SYNCED((int)newTeam);
	ASSERT_SYNCED(given);
	unit->ChangeTeam(newTeam, given ? CUnit::ChangeGiven
	                                : CUnit::ChangeCaptured);
	inTransferUnit = false;
	return 0;
}

/******************************************************************************/
//Gets UnitID, startPiece, endPiece
int LuaSyncedCtrl::CreateUnitIKChain(lua_State* L){

	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL) {
		return 0;
	}

	if (lua_isnumber(L,2) && lua_isnumber(L,3) )
	{
		LocalModelPiece* startPiece = ParseObjectLocalModelPiece(L, unit, 2); 
		LocalModelPiece* endPiece = ParseObjectLocalModelPiece(L, unit, 3); 

		if (startPiece == NULL){
			luaL_error(L, "Startpiece of kinematikChain incorrekt");	
			return 0;
		}
		if (endPiece == NULL){
			luaL_error(L, "Endpiece of kinematikChain incorrekt");
			return 0;	
		}

	lua_pushnumber(L,  unit->CreateIKChain(startPiece, startPiece->scriptPieceIndex, endPiece->scriptPieceIndex));
	return 1;
	}
	
	luaL_error(L, "CreateUnitIKChain has missing/ wrong arguments");	
	return 0;
}

int LuaSyncedCtrl::SetUnitIKActive (lua_State* L){

	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		luaL_error(L, "SetUnitIKActive not a valid UnitID");	
		return 0;
	}

	if (lua_isnumber(L,2) == false || unit->isValidIKChain(lua_tofloat(L,2)) == false){
		luaL_error(L, "Invalid IK ID in SetUnitIKActive");	
		return 0;
	}

	if (lua_isnumber(L,2) && lua_isboolean(L,3)){
			unit->SetIKActive(lua_tofloat(L,2),lua_toboolean(L,3));	
			return 0;
		}else{
			luaL_error(L, "SetUnitIKActive has missing/ wrong arguments");	
		}
	return 0;
}


//Sets IKChainID, Active, Goal
int LuaSyncedCtrl::SetUnitIKGoal(lua_State* L)
{
	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		luaL_error(L, "SetUnitIKGoal not a valid UnitID");	
		return 0;
	}

	if (lua_isnumber(L,2) &&lua_isnumber(L,3)	&&  lua_isnumber(L,4) && lua_isnumber(L,5))
	{
		
	if (lua_isnumber(L,2) == false || unit->isValidIKChain(lua_tofloat(L,2)) == false){
		luaL_error(L, "Invalid IK ID as #2 argument");	
		return 0;
	}
	bool isWorldCoordinate= false;
	if (lua_isboolean(L,6)==true ){	isWorldCoordinate = true;}


		unit->SetIKGoal(lua_tofloat(L,2),lua_tofloat(L,3),lua_tofloat(L,4),lua_tofloat(L,5), isWorldCoordinate);
		return 0;
	}
	luaL_error(L, "SetUnitIKGoal has missing/ wrong arguments");	
	return 0;
}

int LuaSyncedCtrl::SetUnitIKPieceSpeed(lua_State* L)
{
	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	if ( lua_isnumber(L,2) && //IkchainID
		 lua_isnumber(L,3) && //PieceID
		(lua_isnumber(L,4) &&  lua_isnumber(L,5) && lua_isnumber(L,6)))
		{
			//check wether this is a correct ikID
			if (unit->isValidIKChain(lua_tofloat(L,2)) == false){
				luaL_error(L, "Invalid IK ID as #2 argument");	
				return 0;
			}

			//check wether this is a correct piece
			if (unit->isValidIKChainPiece(lua_tofloat(L,2),lua_tofloat(L,3)) == false) {
				luaL_error(L, "Invalid piece ID in SetUnitIKPieceSpeed");	
				return 0;
			}


			unit->SetIKPieceSpeed(lua_tofloat(L,2),lua_tofloat(L,3), lua_tofloat(L,4),lua_tofloat(L,5),lua_tofloat(L,6));
			return 0;
		}
	luaL_error(L, "SetUnitIKPieceSpeed has missing/ wrong arguments");	
	return 0;
}


//Sets the Pieces Limitation regarding what joint type it is 
int LuaSyncedCtrl::SetUnitIKPieceLimits(lua_State* L)
{
	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}


	if (lua_isnumber(L,2) && //IkchainID
		//IK-Node Limitations
		lua_isnumber(L,3)) //PieceIDs
	{
		//check wether this is a correct ikID
		if (unit->isValidIKChain(lua_tofloat(L,2)) == false) {
			luaL_error(L, "SetUnitIKPieceLimits #2 argument is not valid ikID");	
			return 0;
		}
		//check wether this is a correct piece
		if (unit->isValidIKChainPiece(lua_tofloat(L,2),lua_tofloat(L,3)) == false) {
			luaL_error(L, "SetUnitIKPieceLimits #3 argument is invalid Piece");	

			return 0;
		}

		if ( lua_isnumber(L,4) &&  	lua_isnumber(L,5)  	//Limitx and upX
		  && lua_isnumber(L,6) && 	lua_isnumber(L,7)	//Limity and upy
		  && lua_isnumber(L,8) && 	lua_isnumber(L,9))  //Limitz and upz
		{
		unit->SetIKPieceLimit(	lua_tofloat(L,2),lua_tofloat(L,3), //ikID, pieceID
								lua_tofloat(L,4),lua_tofloat(L,5), 
								lua_tofloat(L,6),lua_tofloat(L,7),
								lua_tofloat(L,8),lua_tofloat(L,9)
								);
		return 0;
		}

	}	
	luaL_error(L, "SetUnitIKPieceLimits has missing/ wrong arguments");	
	return 0;
}



int LuaSyncedCtrl::SetUnitCosts(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (!lua_istable(L, 2))
		luaL_error(L, "Incorrect arguments to SetUnitCosts");

	constexpr int tableIdx = 2;

	for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
			continue;

		switch (hashString(lua_tolstring(L, -2, nullptr))) {
			case hashString("buildTime"): {
				unit->buildTime = std::max(1.0f, lua_tofloat(L, -1));
			} break;
			case hashString("metalCost"): {
				unit->cost.metal = std::max(1.0f, lua_tofloat(L, -1));
			} break;
			case hashString("energyCost"): {
				unit->cost.energy = std::max(1.0f, lua_tofloat(L, -1));
			} break;
			default: {
			} break;
 		}

		ASSERT_SYNCED(unit->buildTime);
		ASSERT_SYNCED(unit->metalCost);
		ASSERT_SYNCED(unit->energyCost);
	}

	return 0;
}


static bool SetUnitResourceParam(CUnit* unit, const char* name, float value)
{
	// [u|c][u|m][m|e]
	//
	// unconditional | conditional
	//           use | make
	//         metal | energy

	value *= 0.5f;

	switch (name[0]) {
		case 'u': {
			switch (name[1]) {
				case 'u': {
					if (name[2] == 'm') { unit->resourcesUncondUse.metal  = value; return true; }
					if (name[2] == 'e') { unit->resourcesUncondUse.energy = value; return true; }
				} break;

				case 'm': {
					if (name[2] == 'm') { unit->resourcesUncondMake.metal  = value; return true; }
					if (name[2] == 'e') { unit->resourcesUncondMake.energy = value; return true; }
				} break;

				default: {
				} break;
			}
		} break;

		case 'c': {
			switch (name[1]) {
				case 'u': {
					if (name[2] == 'm') { unit->resourcesCondUse.metal  = value; return true; }
					if (name[2] == 'e') { unit->resourcesCondUse.energy = value; return true; }
				} break;

				case 'm': {
					if (name[2] == 'm') { unit->resourcesCondMake.metal  = value; return true; }
					if (name[2] == 'e') { unit->resourcesCondMake.energy = value; return true; }
				} break;

				default: {
				} break;
			}
		} break;

		default: {
		} break;
	}

	return false;
}


int LuaSyncedCtrl::SetUnitResourcing(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (lua_israwstring(L, 2)) {
		SetUnitResourceParam(unit, lua_tostring(L, 2), luaL_checkfloat(L, 3));
	} else if (lua_istable(L, 2)) {
		constexpr int tableIdx = 2;

		for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
				continue;

			SetUnitResourceParam(unit, lua_tostring(L, -2), lua_tofloat(L, -1));
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to SetUnitResourcing");
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitTooltip(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const char* tmp = luaL_checkstring(L, 2);

	if (tmp != nullptr)
		unitToolTipMap.Set(unit->id, std::string(tmp, lua_strlen(L, 2)));
	else
		unitToolTipMap.Set(unit->id, "");

	return 0;
}


int LuaSyncedCtrl::SetUnitHealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (lua_isnumber(L, 2)) {
		unit->health = std::min(unit->maxHealth, lua_tofloat(L, 2));
	} else if (lua_istable(L, 2)) {
		constexpr int tableIdx = 2;

		for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2) || !lua_isnumber(L, -1))
				continue;

			switch (hashString(lua_tolstring(L, -2, nullptr))) {
				case hashString("health"): {
					unit->health = std::min(unit->maxHealth, lua_tofloat(L, -1));
				} break;
				case hashString("capture"): {
					unit->captureProgress = lua_tofloat(L, -1);
				} break;
				case hashString("paralyze"): {
					const float argValue = lua_tofloat(L, -1);
					const float refValue = modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health;

					if ((unit->paralyzeDamage = std::max(0.0f, argValue)) > refValue) {
						unit->SetStunned(true);
					} else if (argValue < 0.0f) {
						unit->SetStunned(false);
					}
				} break;
				case hashString("build"): {
					if ((unit->buildProgress = lua_tofloat(L, -1)) >= 1.0f)
						unit->FinishedBuilding(false);
				} break;
				default: {
				} break;
			}
		}
	} else {
		luaL_error(L, "Incorrect arguments to SetUnitHealth()");
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitMaxHealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->maxHealth = std::max(0.1f, luaL_checkfloat(L, 2));
	unit->health = std::min(unit->maxHealth, unit->health);
	return 0;
}


int LuaSyncedCtrl::SetUnitStockpile(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	CWeapon* w = unit->stockpileWeapon;

	if (w == nullptr)
		return 0;

	if (lua_isnumber(L, 2)) {
		w->numStockpiled = std::max(0, luaL_checkint(L, 2));
		unit->commandAI->UpdateStockpileIcon();
	}

	if (lua_isnumber(L, 3))
		unit->stockpileWeapon->buildPercent = Clamp(lua_tofloat(L, 3), 0.0f, 1.0f);

	return 0;
}


static bool SetSingleUnitWeaponState(lua_State* L, CWeapon* weapon, int index)
{
	// FIXME: missing checks and updates?
	switch (hashString(lua_tolstring(L, index, nullptr))) {
		case hashString("reloadState"):
		case hashString("reloadFrame"): {
			weapon->reloadStatus = lua_toint(L, index + 1);
		} break;

		case hashString("reloadTime"): {
			weapon->reloadTime = std::max(1, (int) (lua_tofloat(L, index + 1) * GAME_SPEED));
		} break;
		case hashString("reaimTime"): {
			weapon->reaimTime = std::max(1, lua_toint(L, index + 1));
		} break;

		case hashString("accuracy"): {
			weapon->accuracyError = lua_tofloat(L, index + 1);
		} break;
		case hashString("sprayAngle"): {
			weapon->sprayAngle = lua_tofloat(L, index + 1);
		} break;

		case hashString("range"): {
			weapon->UpdateRange(lua_tofloat(L, index + 1));
		} break;
		case hashString("projectileSpeed"): {
			weapon->UpdateProjectileSpeed(lua_tofloat(L, index + 1));
		} break;

		case hashString("autoTargetRangeBoost"): {
			weapon->autoTargetRangeBoost = std::max(0.0f, lua_tofloat(L, index + 1));
		} break;

		case hashString("burst"): {
			weapon->salvoSize = lua_toint(L, index + 1);
		} break;
		case hashString("burstRate"): {
			weapon->salvoDelay = (int) (lua_tofloat(L, index + 1) * GAME_SPEED);
		} break;

		case hashString("projectiles"): {
			weapon->projectilesPerShot = lua_toint(L, index + 1);
		} break;

		case hashString("salvoLeft"): {
			weapon->salvoLeft = lua_toint(L, index + 1);
		} break;
		case hashString("nextSalvo"): {
			weapon->nextSalvo = lua_toint(L, index + 1);
		} break;

		case hashString("aimReady"): {
			weapon->angleGood = (lua_tofloat(L, index + 1) != 0.0f);
		} break;

		case hashString("forceAim"): {
			// move into the past by default s.t. Weapon::CallAimingScript runs the callin next Update
			weapon->lastAimedFrame -= luaL_optint(L, index + 1, weapon->reaimTime);
		} break;

		case hashString("avoidFlags"): {
			weapon->avoidFlags = lua_toint(L, index + 1);
		} break;
		case hashString("collisionFlags"): {
			weapon->collisionFlags = lua_toint(L, index + 1);
		} break;

		default: {
			return false;
		} break;
	}

	return true;
}


int LuaSyncedCtrl::SetUnitWeaponState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size())
		return 0;

	CWeapon* weapon = unit->weapons[weaponNum];

	if (lua_istable(L, 3)) {
		// {key1 = value1, ...}
		for (lua_pushnil(L); lua_next(L, 3) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				SetSingleUnitWeaponState(L, weapon, -2);
			}
		}
	} else {
		// key, value
		if (lua_israwstring(L, 3) && lua_isnumber(L, 4)) {
			SetSingleUnitWeaponState(L, weapon, 3);
		}
	}

	return 0;
}


static int SetSingleDynDamagesKey(lua_State* L, DynDamageArray* damages, int index)
{
	const float value = lua_tofloat(L, index + 1);

	if (lua_isnumber(L, index)) {
		const unsigned armType = lua_toint(L, index);

		if (armType < damages->GetNumTypes())
			damages->Set(armType, std::max(value, 0.0001f));

		return 0;
	}

	// FIXME: missing checks and updates?
	switch (hashString(lua_tostring(L, index))) {
		case hashString("paralyzeDamageTime"): {
			damages->paralyzeDamageTime = std::max((int)value, 0);
		} break;

		case hashString("impulseFactor"): {
			damages->impulseFactor = value;
		} break;
		case hashString("impulseBoost"): {
			damages->impulseBoost = value;
		} break;

		case hashString("craterMult"): {
			damages->craterMult = value;
		} break;
		case hashString("craterBoost"): {
			damages->craterBoost = value;
		} break;

		case hashString("dynDamageExp"): {
			damages->dynDamageExp = value;
		} break;
		case hashString("dynDamageMin"): {
			damages->dynDamageMin = value;
		} break;
		case hashString("dynDamageRange"): {
			damages->dynDamageRange = value;
		} break;
		case hashString("dynDamageInverted"): {
			// HACK, this should be set to result of lua_toboolean
			damages->dynDamageInverted = (value != 0.0f);
		} break;

		case hashString("craterAreaOfEffect"): {
			damages->craterAreaOfEffect = value;
		} break;
		case hashString("damageAreaOfEffect"): {
			damages->damageAreaOfEffect = value;
		} break;

		case hashString("edgeEffectiveness"): {
			damages->edgeEffectiveness = std::min(value, 0.999f);
		} break;
		case hashString("explosionSpeed"): {
			damages->explosionSpeed = value;
		} break;

		default: {
		} break;
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitWeaponDamages(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	DynDamageArray* damages;

	if (lua_israwstring(L, 2)) {
		switch (hashString(lua_tostring(L, 2))) {
			case hashString("explode"     ): { damages = DynDamageArray::GetMutable(unit->deathExpDamages); } break;
			case hashString("selfDestruct"): { damages = DynDamageArray::GetMutable(unit->selfdExpDamages); } break;
			default                        : {                                                    return 0; } break;
		}
	} else {
		const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

		if (weaponNum >= unit->weapons.size())
			return 0;

		CWeapon* weapon = unit->weapons[weaponNum];

		damages = DynDamageArray::GetMutable(weapon->damages);
	}


	if (lua_istable(L, 3)) {
		// {key1 = value1, ...}
		for (lua_pushnil(L); lua_next(L, 3) != 0; lua_pop(L, 1)) {
			if ((lua_isnumber(L, -2) || lua_israwstring(L, -2)) && lua_isnumber(L, -1)) {
				SetSingleDynDamagesKey(L, damages, -2);
			}
		}
	} else {
		// key, value
		if ((lua_isnumber(L, 3) || lua_israwstring(L, 3)) && lua_isnumber(L, 4)) {
			SetSingleDynDamagesKey(L, damages, 3);
		}
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitMaxRange(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->maxRange = std::max(0.0f, luaL_checkfloat(L, 2));
	return 0;
}


int LuaSyncedCtrl::SetUnitExperience(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->AddExperience(std::max(0.0f, luaL_checkfloat(L, 2)) - unit->experience);
	return 0;
}


int LuaSyncedCtrl::SetUnitArmored(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (lua_isboolean(L, 2))
		unit->armoredState = lua_toboolean(L, 2);

	// armored multiple of 0 will crash spring
	unit->armoredMultiple = std::max(0.0001f, luaL_optfloat(L, 3, unit->armoredMultiple));

	if (lua_toboolean(L, 2)) {
		unit->curArmorMultiple = unit->armoredMultiple;
	} else {
		unit->curArmorMultiple = 1.0f;
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
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int allyTeam = luaL_checkint(L, 2);

	if (!teamHandler.IsValidAllyTeam(allyTeam))
		luaL_error(L, "bad allyTeam");

	const unsigned char losStatus = unit->losStatus[allyTeam];
	const unsigned char  oldMask = losStatus >> LOS_MASK_SHIFT;
	const unsigned char  newMask = ParseLosBits(L, 3, oldMask);
	const unsigned char state = (newMask << LOS_MASK_SHIFT) | (losStatus & 0x0F);

	unit->losStatus[allyTeam] = state;
	unit->SetLosStatus(allyTeam, unit->CalcLosStatus(allyTeam));

	return 0;
}


int LuaSyncedCtrl::SetUnitLosState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int allyTeam = luaL_checkint(L, 2);

	if (!teamHandler.IsValidAllyTeam(allyTeam))
		luaL_error(L, "bad allyTeam");

	const unsigned char losStatus = unit->losStatus[allyTeam];
	const unsigned char  oldState = losStatus & 0x0F;
	const unsigned char  newState = ParseLosBits(L, 3, oldState);

	unit->SetLosStatus(allyTeam, (losStatus & 0xF0) | newState);
	return 0;
}


int LuaSyncedCtrl::SetUnitCloak(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	// make unit {de}cloak at next SlowUpdate
	if (lua_isboolean(L, 2))
		unit->wantCloak = lua_toboolean(L, 2);
	if (lua_isnumber(L, 2))
		unit->wantCloak = (lua_tonumber(L, 2) != 0);

	if (lua_israwnumber(L, 3)) {
		unit->decloakDistance = lua_tofloat(L, 3);
		return 0;
	}

	if (lua_isboolean(L, 3)) {
		if (lua_toboolean(L, 3)) {
			unit->decloakDistance = math::fabsf(unit->unitDef->decloakDistance);
		} else {
			unit->decloakDistance = unit->unitDef->decloakDistance;
		}
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitStealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->stealth = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitSonarStealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->sonarStealth = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitAlwaysVisible(lua_State* L)
{
	return (SetWorldObjectAlwaysVisible(L, ParseUnit(L, __func__, 1), __func__));
}


int LuaSyncedCtrl::SetUnitMetalExtraction(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	CExtractorBuilding* mex = dynamic_cast<CExtractorBuilding*>(unit);

	if (mex == nullptr)
		return 0;

	const float depth = luaL_checkfloat(L, 2);
	const float range = luaL_optfloat(L, 3, mex->GetExtractionRange());
	mex->ResetExtraction();
	mex->SetExtractionRangeAndDepth(range, depth);
	return 0;
}


int LuaSyncedCtrl::SetUnitHarvestStorage(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
		unit->harvested[i]       = luaL_optfloat(L, 2 + i * 2,     unit->harvested[i]);
		unit->harvestStorage[i]  = luaL_optfloat(L, 2 + i * 2 + 1, unit->harvestStorage[i]);
	}
	return 0;
}

int LuaSyncedCtrl::SetUnitBuildSpeed(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const float buildScale = (1.0f / TEAM_SLOWUPDATE_RATE);
	const float buildSpeed = buildScale * max(0.0f, luaL_checkfloat(L, 2));

	CFactory* factory = dynamic_cast<CFactory*>(unit);

	if (factory != nullptr) {
		factory->buildSpeed = buildSpeed;
		return 0;
	}

	CBuilder* builder = dynamic_cast<CBuilder*>(unit);

	if (builder == nullptr)
		return 0;

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


int LuaSyncedCtrl::SetUnitNanoPieces(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	NanoPieceCache* pieceCache = nullptr;
	std::vector<int>* nanoPieces = nullptr;

	{
		CBuilder* builder = dynamic_cast<CBuilder*>(unit);

		if (builder != nullptr) {
			pieceCache = &builder->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}

		CFactory* factory = dynamic_cast<CFactory*>(unit);

		if (factory != nullptr) {
			pieceCache = &factory->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}
	}

	if (nanoPieces == nullptr)
		return 0;

	nanoPieces->clear();
	pieceCache->StopPolling();
	luaL_checktype(L, 2, LUA_TTABLE);

	for (lua_pushnil(L); lua_next(L, 2) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -1)) {
			const int modelPieceNum = lua_toint(L, -1) - 1; //lua 1-indexed, c++ 0-indexed

			if (unit->localModel.HasPiece(modelPieceNum)) {
				nanoPieces->push_back(modelPieceNum);
			} else {
				luaL_error(L, "[SetUnitNanoPieces] incorrect model-piece number %d", modelPieceNum);
			}
		}
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitBlocking(lua_State* L)
{
	return (SetSolidObjectBlocking(L, ParseUnit(L, __func__, 1)));
}


int LuaSyncedCtrl::SetUnitCrashing(lua_State* L) {
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	AAirMoveType* amt = dynamic_cast<AAirMoveType*>(unit->moveType);
	bool ret = false;

	if (amt != nullptr) {
		const bool wantCrash = luaL_optboolean(L, 2, false);
		const AAirMoveType::AircraftState aircraftState = amt->aircraftState;

		// for simplicity, this can only set a non-landed aircraft to
		// start crashing, or a crashing aircraft to start flying
		if ( wantCrash && (aircraftState != AAirMoveType::AIRCRAFT_LANDED))
			amt->SetState(AAirMoveType::AIRCRAFT_CRASHING);

		if (!wantCrash && (aircraftState == AAirMoveType::AIRCRAFT_CRASHING))
			amt->SetState(AAirMoveType::AIRCRAFT_FLYING);

		ret = (amt->aircraftState != aircraftState);
	}

	lua_pushboolean(L, ret);
	return 1;
}


int LuaSyncedCtrl::SetUnitShieldState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	int args = lua_gettop(L);
	int arg = 2;

	CPlasmaRepulser* shield = static_cast<CPlasmaRepulser*>(unit->shieldWeapon);

	if (lua_isnumber(L, 2) && args > 2) {
		const size_t idx = luaL_optint(L, 2, -1) - LUA_WEAPON_BASE_INDEX;

		if (idx < unit->weapons.size()) {
			shield = dynamic_cast<CPlasmaRepulser*>(unit->weapons[idx]);
		}

		arg++;
	}

	if (shield == nullptr)
		return 0;

	if (lua_isboolean(L, arg)) { shield->SetEnabled(lua_toboolean(L, arg)); arg++; }
	if (lua_isnumber(L, arg)) { shield->SetCurPower(lua_tofloat(L, arg)); }
	return 0;
}


int LuaSyncedCtrl::SetUnitFlanking(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

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
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (lua_isnumber(L, 2))
		unit->travel = lua_tofloat(L, 2);

	if (lua_isnumber(L, 3))
		unit->travelPeriod = lua_tofloat(L, 3);

	return 0;
}


int LuaSyncedCtrl::SetUnitFuel(lua_State* L) { return 0; } // DEPRECATED


int LuaSyncedCtrl::SetUnitNeutral(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->neutral = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitTarget(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int args = lua_gettop(L);
	if (lua_isnil(L, 2)) {
		unit->DropCurrentAttackTarget();
		lua_pushboolean(L, true);
		return 1;
	} if (args >= 4 && !lua_isboolean(L, 3)) {
		const float3 pos(luaL_checkfloat(L, 2),
		                 luaL_checkfloat(L, 3),
		                 luaL_checkfloat(L, 4));
		const bool manualFire = luaL_optboolean(L, 5, false);
		const bool userTarget = luaL_optboolean(L, 6, false);
		const int weaponNum = luaL_optint(L, 7, 0) - LUA_WEAPON_BASE_INDEX;
		bool ret = false;
		if (weaponNum < 0) {
			ret = unit->AttackGround(pos, userTarget, manualFire);
		} else if (weaponNum < unit->weapons.size()) {
			SWeaponTarget trg(pos, userTarget);
			trg.isManualFire = manualFire;
			ret = unit->weapons[weaponNum]->Attack(trg);
		}
		lua_pushboolean(L, ret);
		return 1;
	}
	else if (args >= 2) {
		CUnit* target = ParseRawUnit(L, __func__, 2);

		if (target == unit) {
			luaL_error(L, "[%s()]: unit tried to attack itself", __func__);
			return 0;
		}

		const bool manualFire = luaL_optboolean(L, 3, false);
		const bool userTarget = luaL_optboolean(L, 4, false);
		const int weaponNum = luaL_optint(L, 5, -1) - LUA_WEAPON_BASE_INDEX;
		bool ret = false;
		if (weaponNum < 0) {
			ret = unit->AttackUnit(target, userTarget, manualFire);
		} else if (weaponNum < unit->weapons.size()) {
			SWeaponTarget trg(target, userTarget);
			trg.isManualFire = manualFire;
			ret = unit->weapons[weaponNum]->Attack(trg);
		}
		lua_pushboolean(L, ret);
		return 1;
	}
	return 0;
}



int LuaSyncedCtrl::SetUnitMidAndAimPos(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}

	#define FLOAT(i) luaL_checkfloat(L, i)
	#define FLOAT3(i, j, k) float3(FLOAT(i), FLOAT(j), FLOAT(k))

	const int argc = lua_gettop(L);
	const float3 newMidPos = (argc >= 4)? FLOAT3(2, 3, 4): float3(unit->midPos);
	const float3 newAimPos = (argc >= 7)? FLOAT3(5, 6, 7): float3(unit->aimPos);
	const bool setRelative = luaL_optboolean(L, 8, false);
	const bool updateQuads = (newMidPos != unit->midPos);

	#undef FLOAT3
	#undef FLOAT

	if (updateQuads) {
		// safety, possibly just need MovedUnit
		quadField.RemoveUnit(unit);
	}

	unit->SetMidAndAimPos(newMidPos, newAimPos, setRelative);

	if (updateQuads) {
		quadField.MovedUnit(unit);
	}

	lua_pushboolean(L, true);
	return 1;
}

int LuaSyncedCtrl::SetUnitRadiusAndHeight(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}

	const float newRadius = std::max(1.0f, luaL_optfloat(L, 2, unit->radius));
	const float newHeight = std::max(1.0f, luaL_optfloat(L, 3, unit->height));
	const bool updateQuads = (newRadius != unit->radius);

	if (updateQuads) {
		// safety, possibly just need MovedUnit
		quadField.RemoveUnit(unit);
	}

	unit->SetRadiusAndHeight(newRadius, newHeight);

	if (updateQuads) {
		quadField.MovedUnit(unit);
	}

	lua_pushboolean(L, true);
	return 1;
}

int LuaSyncedCtrl::SetUnitPieceParent(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	LocalModelPiece* childPiece = ParseObjectLocalModelPiece(L, unit, 2);

	if (childPiece == nullptr) {
		luaL_error(L, "invalid piece");
		return 0;
	}

	LocalModelPiece* parentPiece = ParseObjectLocalModelPiece(L, unit, 3);

	if (parentPiece == nullptr) {
		luaL_error(L, "invalid parent piece");
		return 0;
	}

	if (childPiece == unit->localModel.GetRoot()) {
		luaL_error(L, "Can't change a root piece's parent");
		return 0;
	}

	childPiece->parent->RemoveChild(childPiece);
	childPiece->SetParent(parentPiece);
	parentPiece->AddChild(childPiece);
	return 0;
}

int LuaSyncedCtrl::SetUnitPieceMatrix(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	LocalModelPiece* lmp = ParseObjectLocalModelPiece(L, unit, 2);

	if (lmp == nullptr)
		return 0;

	CMatrix44f mat;

	if (LuaUtils::ParseFloatArray(L, 3, &mat.m[0], 16) == -1)
		return 0;

	if (lmp->SetPieceSpaceMatrix(mat))
		lmp->SetDirty();

	lua_pushboolean(L, lmp->blockScriptAnims);
	return 1;
}


int LuaSyncedCtrl::SetUnitCollisionVolumeData(lua_State* L)
{
	return (SetSolidObjectCollisionVolumeData(L, ParseUnit(L, __func__, 1)));
}

int LuaSyncedCtrl::SetUnitPieceCollisionVolumeData(lua_State* L)
{
	return (SetSolidObjectPieceCollisionVolumeData(L, ParseUnit(L, __func__, 1)));
}



int LuaSyncedCtrl::SetUnitSensorRadius(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const std::string& key = luaL_checkstring(L, 2);
	const int radius = Clamp(luaL_checkint(L, 3), 0, MAX_UNIT_SENSOR_RADIUS);

	if (key == "los") {
		unit->ChangeLos(unit->realLosRadius = radius, unit->realAirLosRadius);
		lua_pushnumber(L, unit->losRadius);
	} else if (key == "airLos") {
		unit->ChangeLos(unit->realAirLosRadius = radius, radius);
		lua_pushnumber(L, unit->airLosRadius);
	} else if (key == "radar") {
		unit->radarRadius = radius;
		lua_pushnumber(L, unit->radarRadius);
	} else if (key == "sonar") {
		unit->sonarRadius = radius;
		lua_pushnumber(L, unit->sonarRadius);
	} else if (key == "seismic") {
		unit->seismicRadius = radius;
		lua_pushnumber(L, unit->seismicRadius);
	} else if (key == "radarJammer") {
		unit->jammerRadius = radius;
		lua_pushnumber(L, unit->jammerRadius);
	} else if (key == "sonarJammer") {
		unit->sonarJamRadius = radius;
		lua_pushnumber(L, unit->sonarJamRadius);
	} else {
		luaL_error(L, "Unknown sensor type to SetUnitSensorRadius()");
	}

	return 1;
}


int LuaSyncedCtrl::SetUnitPosErrorParams(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->posErrorVector = float3(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));
	unit->posErrorDelta = float3(luaL_checkfloat(L, 5), luaL_checkfloat(L, 6), luaL_checkfloat(L, 7));
	unit->nextPosErrorUpdate = luaL_optint(L, 8, unit->nextPosErrorUpdate);
	return 0;
}


int LuaSyncedCtrl::SetUnitMoveGoal(lua_State* L)
{
	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;
	if (unit->moveType == nullptr)
		return 0;

	const float3 pos(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));

	const float radius = luaL_optfloat(L, 5, 0.0f);
	const float speed  = luaL_optfloat(L, 6, unit->moveType->GetMaxSpeed());

	if (luaL_optboolean(L, 7, false)) {
		unit->moveType->StartMovingRaw(pos, radius);
	} else {
		unit->moveType->StartMoving(pos, radius, speed);
	}

	return 0;
}


int LuaSyncedCtrl::SetUnitLandGoal(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	AAirMoveType* amt = dynamic_cast<AAirMoveType*>(unit->moveType);

	if (amt == nullptr)
		luaL_error(L, "Not a flying unit");

	const float3 landPos(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));
	const float radiusSq = lua_isnumber(L, 5)? Square(lua_tonumber(L, 5)): -1.0f;

	amt->LandAt(landPos, radiusSq);
	return 0;
}


int LuaSyncedCtrl::ClearUnitGoal(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->moveType->StopMoving(false, false, luaL_optboolean(L, 2, true));
	return 0;
}


int LuaSyncedCtrl::SetUnitPhysics(lua_State* L)
{
	return (SetSolidObjectPhysicalState(L, ParseUnit(L, __func__, 1)));
}

int LuaSyncedCtrl::SetUnitMass(lua_State* L)
{
	return (SetSolidObjectMass(L, ParseUnit(L, __func__, 1)));
}

int LuaSyncedCtrl::SetUnitPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	float3 pos;

	if (lua_isnumber(L, 4)) {
		// 2=x, 3=y, 4=z
		pos.x = luaL_checkfloat(L, 2);
		pos.y = luaL_checkfloat(L, 3);
		pos.z = luaL_checkfloat(L, 4);
	} else {
		// 2=x, 3=z, 4=bool
		pos.x = luaL_checkfloat(L, 2);
		pos.z = luaL_checkfloat(L, 3);

		if (luaL_optboolean(L, 4, false)) {
			pos.y = CGround::GetHeightAboveWater(pos.x, pos.z);
		} else {
			pos.y = CGround::GetHeightReal(pos.x, pos.z);
		}
	}

	unit->ForcedMove(pos);
	return 0;
}


int LuaSyncedCtrl::SetUnitRotation(lua_State* L)
{
	return (SetSolidObjectRotation(L, ParseUnit(L, __func__, 1), false));
}

int LuaSyncedCtrl::SetUnitDirection(lua_State* L)
{
	return (SetSolidObjectDirection(L, ParseUnit(L, __func__, 1)));
}

int LuaSyncedCtrl::SetUnitVelocity(lua_State* L)
{
	return (SetWorldObjectVelocity(L, ParseUnit(L, __func__, 1)));
}


int LuaSyncedCtrl::AddUnitDamage(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const float damage    = luaL_checkfloat(L, 2);
	const int paralyze    = luaL_optint(L, 3, 0);
	const int attackerID  = luaL_optint(L, 4, -1);
	const int weaponDefID = luaL_optint(L, 5, -1);
	const float3 impulse  = float3(Clamp(luaL_optfloat(L, 6, 0.0f), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE),
	                               Clamp(luaL_optfloat(L, 7, 0.0f), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE),
	                               Clamp(luaL_optfloat(L, 8, 0.0f), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE));

	CUnit* attacker = nullptr;

	if (attackerID >= 0) {
		if (static_cast<size_t>(attackerID) >= unitHandler.MaxUnits())
			return 0;

		attacker = unitHandler.GetUnit(attackerID);
	}

	// -1 is allowed
	if (weaponDefID >= int(weaponDefHandler->NumWeaponDefs()))
		return 0;

	DamageArray damages;
	damages.Set(unit->armorType, damage);

	if (paralyze)
		damages.paralyzeDamageTime = paralyze;

	unit->DoDamage(damages, impulse, attacker, weaponDefID, -1);
	return 0;
}


int LuaSyncedCtrl::AddUnitImpulse(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const float3 impulse(Clamp(luaL_checkfloat(L, 2), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE),
	                     Clamp(luaL_checkfloat(L, 3), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE),
	                     Clamp(luaL_checkfloat(L, 4), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE));

	unit->ApplyImpulse(impulse);
	return 0;
}


int LuaSyncedCtrl::AddUnitSeismicPing(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	unit->DoSeismicPing(luaL_checkfloat(L, 2));
	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::AddUnitResource(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const string& type = luaL_checkstring(L, 2);

	if (type.empty())
		return 0;

	switch (type[0]) {
		case 'm': { unit->AddMetal (std::max(0.0f, luaL_checkfloat(L, 3))); } break;
		case 'e': { unit->AddEnergy(std::max(0.0f, luaL_checkfloat(L, 3))); } break;
		default: {} break;
	}

	return 0;
}


int LuaSyncedCtrl::UseUnitResource(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (lua_isstring(L, 2)) {
		const char* type = lua_tostring(L, 2);

		switch (type[0]) {
			case 'm': { lua_pushboolean(L, unit->UseMetal (std::max(0.0f, lua_tofloat(L, 3)))); return 1; } break;
			case 'e': { lua_pushboolean(L, unit->UseEnergy(std::max(0.0f, lua_tofloat(L, 3)))); return 1; } break;
			default : {                                                                                   } break;
		}

		return 0;
	}

	if (lua_istable(L, 2)) {
		float metal  = 0.0f;
		float energy = 0.0f;

		constexpr int tableIdx = 2;

		for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				const char* key = lua_tostring(L, -2);
				const float val = std::max(0.0f, lua_tofloat(L, -1));

				switch (key[0]) {
					case 'm': {  metal = val; } break;
					case 'e': { energy = val; } break;
					default : {               } break;
				}
			}
		}

		CTeam* team = teamHandler.Team(unit->team);

		if ((team->res.metal >= metal) && (team->res.energy >= energy)) {
			unit->UseMetal(metal);
			unit->UseEnergy(energy);
			lua_pushboolean(L, true);
		} else {
			team->resPull.metal  += metal;
			team->resPull.energy += energy;
			lua_pushboolean(L, false);
		}

		return 1;
	}

	luaL_error(L, "Incorrect arguments to UseUnitResource()");
	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::RemoveBuildingDecal(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	groundDecals->ForceRemoveSolidObject(unit);
	return 0;
}


int LuaSyncedCtrl::AddGrass(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1), 0.0f, luaL_checkfloat(L, 2));

	grassDrawer->AddGrass(pos.cClampInBounds());
	return 0;
}

int LuaSyncedCtrl::RemoveGrass(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1), 0.0f, luaL_checkfloat(L, 2));

	grassDrawer->RemoveGrass(pos.cClampInBounds());
	return 0;
}


/******************************************************************************/

int LuaSyncedCtrl::CreateFeature(lua_State* L)
{
	CheckAllowGameChanges(L);

	const FeatureDef* featureDef = nullptr;

	if (lua_israwstring(L, 1)) {
		featureDef = featureDefHandler->GetFeatureDef(lua_tostring(L, 1));
	} else if (lua_israwnumber(L, 1)) {
		featureDef = featureDefHandler->GetFeatureDefByID(lua_toint(L, 1));
	}

	if (featureDef == nullptr)
		return 0; // do not error (featureDefs are dynamic)

	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));

	short int heading = 0;

	if (lua_isnumber(L, 5))
		heading = lua_toint(L, 5);

	int facing = GetFacingFromHeading(heading);
	int team = CtrlTeam(L);

	if (team < 0)
		team = -1; // default to global for AllAccessTeam

	// FIXME -- separate teamcolor/allyteam arguments

	if (lua_isnumber(L, 6)) {
		if ((team = lua_toint(L, 6)) < -1) {
			team = -1;
		} else if (team >= teamHandler.ActiveTeams()) {
			return 0;
		}
	}

	const int allyTeam = (team < 0) ? -1 : teamHandler.AllyTeam(team);

	if (!CanControlFeatureAllyTeam(L, allyTeam))
		luaL_error(L, "CreateFeature() bad team permission %d", team);

	if (inCreateFeature)
		luaL_error(L, "CreateFeature() recursion is not permitted");

	// use SetFeatureResurrect() to fill in the missing bits
	inCreateFeature =  true;
	FeatureLoadParams  params;
	params.featureDef  = featureDef;
	params.unitDef     = nullptr;
	params.pos         = pos;
	params.speed       = ZeroVector;
	params.featureID   = luaL_optint(L, 7, -1);
	params.teamID      = team;
	params.allyTeamID  = allyTeam;
	params.heading     = heading,
	params.facing      = facing,
	params.wreckLevels = 0;
	params.smokeTime   = 0;

	CFeature* feature = featureHandler.LoadFeature(params);
	inCreateFeature = false;

	if (feature != nullptr) {
		lua_pushnumber(L, feature->id);
		return 1;
	}

	return 0;
}


int LuaSyncedCtrl::DestroyFeature(lua_State* L)
{
	CheckAllowGameChanges(L);
	CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr)
		return 0;

	if (inDestroyFeature)
		luaL_error(L, "DestroyFeature() recursion is not permitted");

	inDestroyFeature = true;
	featureHandler.DeleteFeature(feature);
	inDestroyFeature = false;

	return 0;
}


int LuaSyncedCtrl::TransferFeature(lua_State* L)
{
	CheckAllowGameChanges(L);
	CFeature* feature = ParseFeature(L, __func__, 1);
	if (feature == nullptr)
		return 0;

	const int teamId = luaL_checkint(L, 2);
	if (!teamHandler.IsValidTeam(teamId))
		return 0;

	feature->ChangeTeam(teamId);
	return 0;
}


int LuaSyncedCtrl::SetFeatureAlwaysVisible(lua_State* L)
{
	return (SetWorldObjectAlwaysVisible(L, ParseFeature(L, __func__, 1), __func__));
}


int LuaSyncedCtrl::SetFeatureHealth(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	feature->health = std::min(feature->maxHealth, luaL_checkfloat(L, 2));
	return 0;
}


int LuaSyncedCtrl::SetFeatureMaxHealth(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	feature->maxHealth = std::max(0.1f, luaL_checkfloat(L, 2));
	feature->health = std::min(feature->health, feature->maxHealth);
	return 0;
}


int LuaSyncedCtrl::SetFeatureReclaim(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	feature->reclaimLeft = luaL_checkfloat(L, 2);
	return 0;
}

int LuaSyncedCtrl::SetFeatureResources(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	feature->resources.metal  = std::max(0.0f, luaL_checknumber(L, 2));
	feature->resources.energy = std::max(0.0f, luaL_checknumber(L, 3));

	feature->reclaimTime = Clamp(luaL_optnumber(L, 4, feature->reclaimTime), 1.0f, 1000000.0f);
	feature->reclaimLeft = Clamp(luaL_optnumber(L, 5, feature->reclaimLeft), 0.0f,       1.0f);
	return 0;
}

int LuaSyncedCtrl::SetFeatureResurrect(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	if (!lua_isnoneornil(L, 2)) {
		const UnitDef* ud = nullptr;

		// which type of unit this feature should turn into if resurrected, by id or name
		if (lua_israwnumber(L, 2))
			ud = unitDefHandler->GetUnitDefByID(lua_toint(L, 2));
		else if (lua_israwstring(L, 2))
			ud = unitDefHandler->GetUnitDefByName(lua_tostring(L, 2));

		// nullptr is also accepted, allows unsetting the target via id=-1
		feature->udef = ud;
	}

	if (!lua_isnoneornil(L, 3))
		feature->buildFacing = LuaUtils::ParseFacing(L, __func__, 3);

	feature->resurrectProgress = Clamp(luaL_optnumber(L, 4, feature->resurrectProgress), 0.0f, 1.0f);
	return 0;
}


int LuaSyncedCtrl::SetFeatureMoveCtrl(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	CFeature::MoveCtrl& moveCtrl = feature->moveCtrl;

	if ((moveCtrl.enabled = luaL_optboolean(L, 2, moveCtrl.enabled))) {
		featureHandler.SetFeatureUpdateable(feature);

		// set vectors
		for (int i = 0; i < 3; i++) {
			moveCtrl.velVector[i] = luaL_optfloat(L, 3 + i, moveCtrl.velVector[i]);
			moveCtrl.accVector[i] = luaL_optfloat(L, 6 + i, moveCtrl.accVector[i]);
		}
	} else {
		// set masks
		for (int i = 0; i < 3; i++) {
			moveCtrl.velocityMask[i] = (luaL_optfloat(L, 3 + i, moveCtrl.velocityMask[i]) != 0.0f);
			moveCtrl. impulseMask[i] = (luaL_optfloat(L, 6 + i, moveCtrl. impulseMask[i]) != 0.0f);
			moveCtrl.movementMask[i] = (luaL_optfloat(L, 9 + i, moveCtrl.movementMask[i]) != 0.0f);
		}
	}

	return 0;
}


int LuaSyncedCtrl::SetFeaturePhysics(lua_State* L)
{
	return (SetSolidObjectPhysicalState(L, ParseFeature(L, __func__, 1)));
}

int LuaSyncedCtrl::SetFeatureMass(lua_State* L)
{
	return (SetSolidObjectMass(L, ParseFeature(L, __func__, 1)));
}

int LuaSyncedCtrl::SetFeaturePosition(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	float3 pos;
	pos.x = luaL_checkfloat(L, 2);
	pos.y = luaL_checkfloat(L, 3);
	pos.z = luaL_checkfloat(L, 4);

	feature->ForcedMove(pos);
	return 0;
}

int LuaSyncedCtrl::SetFeatureRotation(lua_State* L)
{
	return (SetSolidObjectRotation(L, ParseFeature(L, __func__, 1), true));
}

int LuaSyncedCtrl::SetFeatureDirection(lua_State* L)
{
	return (SetSolidObjectDirection(L, ParseFeature(L, __func__, 1)));
}

int LuaSyncedCtrl::SetFeatureVelocity(lua_State* L)
{
	return (SetWorldObjectVelocity(L, ParseFeature(L, __func__, 1)));
}


int LuaSyncedCtrl::SetFeatureBlocking(lua_State* L)
{
	return (SetSolidObjectBlocking(L, ParseFeature(L, __func__, 1)));
}

int LuaSyncedCtrl::SetFeatureNoSelect(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr)
		return 0;

	feature->noSelect = !!luaL_checkboolean(L, 2);
	return 0;
}



int LuaSyncedCtrl::SetFeatureMidAndAimPos(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}

	#define FLOAT(i) luaL_checkfloat(L, i)
	#define FLOAT3(i, j, k) float3(FLOAT(i), FLOAT(j), FLOAT(k))

	const int argc = lua_gettop(L);
	const float3 newMidPos = (argc >= 4)? FLOAT3(2, 3, 4): float3(feature->midPos);
	const float3 newAimPos = (argc >= 7)? FLOAT3(5, 6, 7): float3(feature->aimPos);
	const bool setRelative = luaL_optboolean(L, 8, false);
	const bool updateQuads = (newMidPos != feature->midPos);

	#undef FLOAT3
	#undef FLOAT

	if (updateQuads) {
		quadField.RemoveFeature(feature);
	}

	feature->SetMidAndAimPos(newMidPos, newAimPos, setRelative);

	if (updateQuads) {
		quadField.AddFeature(feature);
	}

	lua_pushboolean(L, true);
	return 1;
}

int LuaSyncedCtrl::SetFeatureRadiusAndHeight(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __func__, 1);

	if (feature == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}

	const float newRadius = std::max(1.0f, luaL_optfloat(L, 2, feature->radius));
	const float newHeight = std::max(1.0f, luaL_optfloat(L, 3, feature->height));
	const bool updateQuads = (newRadius != feature->radius);

	if (updateQuads) {
		quadField.RemoveFeature(feature);
	}

	feature->SetRadiusAndHeight(newRadius, newHeight);

	if (updateQuads) {
		quadField.AddFeature(feature);
	}

	lua_pushboolean(L, true);
	return 1;
}


int LuaSyncedCtrl::SetFeatureCollisionVolumeData(lua_State* L)
{
	return (SetSolidObjectCollisionVolumeData(L, ParseFeature(L, __func__, 1)));
}

int LuaSyncedCtrl::SetFeaturePieceCollisionVolumeData(lua_State* L)
{
	return (SetSolidObjectPieceCollisionVolumeData(L, ParseFeature(L, __func__, 1)));
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetProjectileAlwaysVisible(lua_State* L)
{
	return (SetWorldObjectAlwaysVisible(L, ParseProjectile(L, __func__, 1), __func__));
}

int LuaSyncedCtrl::SetProjectileMoveControl(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr)
		return 0;
	if (!proj->weapon && !proj->piece)
		return 0;

	proj->luaMoveCtrl = luaL_optboolean(L, 2, false);
	return 0;
}

int LuaSyncedCtrl::SetProjectilePosition(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr)
		return 0;

	proj->pos.x = luaL_optfloat(L, 2, 0.0f);
	proj->pos.y = luaL_optfloat(L, 3, 0.0f);
	proj->pos.z = luaL_optfloat(L, 4, 0.0f);

	return 0;
}

int LuaSyncedCtrl::SetProjectileVelocity(lua_State* L)
{
	return (SetWorldObjectVelocity(L, ParseProjectile(L, __func__, 1)));
}

int LuaSyncedCtrl::SetProjectileCollision(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr)
		return 0;

	proj->Collision();
	return 0;
}

int LuaSyncedCtrl::SetProjectileTarget(lua_State* L)
{
	CProjectile* pro = ParseProjectile(L, __func__, 1);
	CWeaponProjectile* wpro = NULL;

	if (pro == nullptr)
		return 0;
	if (!pro->weapon)
		return 0;

	struct ProjectileTarget {
		static DependenceType GetObjectDepType(const CWorldObject* o) {
			if (dynamic_cast<const      CSolidObject*>(o) != nullptr) return DEPENDENCE_WEAPONTARGET;
			if (dynamic_cast<const CWeaponProjectile*>(o) != nullptr) return DEPENDENCE_INTERCEPTTARGET;
			return DEPENDENCE_NONE;
		}
	};

	wpro = static_cast<CWeaponProjectile*>(pro);

	switch (lua_gettop(L)) {
		case 3: {
			const int id = luaL_checkint(L, 2);
			const int type = luaL_checkint(L, 3);

			CWorldObject* oldTargetObject = wpro->GetTargetObject();
			CWorldObject* newTargetObject = NULL;

			switch (type) {
				case 'u': { newTargetObject = ParseUnit(L, __func__, 2); } break;
				case 'f': { newTargetObject = ParseFeature(L, __func__, 2); } break;
				case 'p': { newTargetObject = ParseProjectile(L, __func__, 2); } break;
				case 'g': { /* fall-through, needs four arguments (todo: or a table?) */ }
				default: { /* if invalid type-argument, current target will be cleared */ } break;
			}

			const DependenceType oldDepType = ProjectileTarget::GetObjectDepType(oldTargetObject);
			const DependenceType newDepType = ProjectileTarget::GetObjectDepType(newTargetObject);

			if (oldTargetObject != nullptr) {
				wpro->DeleteDeathDependence(oldTargetObject, oldDepType);
				wpro->SetTargetObject(nullptr);
			}
			if (newTargetObject != nullptr) {
				wpro->AddDeathDependence(newTargetObject, newDepType);
				wpro->SetTargetObject(newTargetObject);
			}

			assert(newTargetObject == NULL || newTargetObject->id == id);
			lua_pushboolean(L, oldTargetObject != NULL || newTargetObject != nullptr);
			return 1;
		} break;

		case 4: {
			if (wpro->GetTargetObject() != nullptr) {
				wpro->DeleteDeathDependence(wpro->GetTargetObject(), ProjectileTarget::GetObjectDepType(wpro->GetTargetObject()));
			}

			wpro->SetTargetObject(nullptr);
			wpro->SetTargetPos(float3(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4)));

			lua_pushboolean(L, wpro->GetTargetObject() == nullptr);
			return 1;
		} break;
	}

	return 0;
}


int LuaSyncedCtrl::SetProjectileIsIntercepted(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr || !proj->weapon)
		return 0;

	CWeaponProjectile* wpro = static_cast<CWeaponProjectile*>(proj);

	wpro->SetBeingIntercepted(luaL_checkboolean(L, 2));
	return 0;
}


int LuaSyncedCtrl::SetProjectileDamages(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr || !proj->weapon)
		return 0;

	CWeaponProjectile* wpro = static_cast<CWeaponProjectile*>(proj);
	DynDamageArray* damages = DynDamageArray::GetMutable(wpro->damages);


	if (lua_istable(L, 3)) {
		// {key1 = value1, ...}
		for (lua_pushnil(L); lua_next(L, 3) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2) && lua_isnumber(L, -1)) {
				SetSingleDynDamagesKey(L, damages, -2);
			}
		}
	} else {
		// key, value
		if (lua_israwstring(L, 3) && lua_isnumber(L, 4)) {
			SetSingleDynDamagesKey(L, damages, 3);
		}
	}

	return 0;
}


int LuaSyncedCtrl::SetProjectileIgnoreTrackingError(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr)
		return 0;

	switch(proj->GetProjectileType()) {
		case WEAPON_MISSILE_PROJECTILE: {
			static_cast<CMissileProjectile*>(proj)->SetIgnoreError(luaL_checkboolean(L, 2));
		} break;
		case WEAPON_STARBURST_PROJECTILE: {
			static_cast<CStarburstProjectile*>(proj)->SetIgnoreError(luaL_checkboolean(L, 2));
		} break;
		case WEAPON_TORPEDO_PROJECTILE: {
			static_cast<CTorpedoProjectile*>(proj)->SetIgnoreError(luaL_checkboolean(L, 2));
		} break;
		default: break;
	}

	return 0;
}


int LuaSyncedCtrl::SetProjectileGravity(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr)
		return 0;

	proj->mygravity = luaL_optfloat(L, 2, 0.0f);
	return 0;
}

int LuaSyncedCtrl::SetProjectileSpinAngle(lua_State* L) { return 0; } // DEPRECATED
int LuaSyncedCtrl::SetProjectileSpinSpeed(lua_State* L) { return 0; } // DEPRECATED
int LuaSyncedCtrl::SetProjectileSpinVec(lua_State* L) { return 0; } // DEPRECATED

int LuaSyncedCtrl::SetPieceProjectileParams(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr || !proj->piece)
		return 0;

	CPieceProjectile* pproj = static_cast<CPieceProjectile*>(proj);

	pproj->explFlags = luaL_optint(L, 2, pproj->explFlags);
	pproj->spinParams.y = luaL_optfloat(L, 3, pproj->spinParams.y);
	pproj->spinParams.x = luaL_optfloat(L, 4, pproj->spinParams.x);
	pproj->spinVector.x = luaL_optfloat(L, 5, pproj->spinVector.x);
	pproj->spinVector.y = luaL_optfloat(L, 6, pproj->spinVector.y);
	pproj->spinVector.z = luaL_optfloat(L, 7, pproj->spinVector.z);
	return 0;
}

//
// TODO: move this and SpawnCEG to LuaUnsyncedCtrl
//
int LuaSyncedCtrl::SetProjectileCEG(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr)
		return 0;
	if (!proj->weapon && !proj->piece)
		return 0;

	unsigned int cegID = CExplosionGeneratorHandler::EXPGEN_ID_INVALID;

	if (lua_isstring(L, 2)) {
		cegID = explGenHandler.LoadGeneratorID(std::string(CEG_PREFIX_STRING) + lua_tostring(L, 2));
	} else {
		cegID = luaL_checknumber(L, 2);
	}

	// if cegID is EXPGEN_ID_INVALID, this also returns NULL
	if (explGenHandler.GetGenerator(cegID) != nullptr)
		proj->SetCustomExplosionGeneratorID(cegID);

	lua_pushnumber(L, cegID);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static void ParseUnitMap(lua_State* L, const char* caller,
                         int table, vector<CUnit*>& unitIDs)
{
	if (!lua_istable(L, table))
		luaL_error(L, "%s(): error parsing unit map", caller);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -2))
			continue;

		CUnit* unit = ParseUnit(L, __func__, -2);

		if (unit == nullptr)
			continue; // bad pointer

		unitIDs.push_back(unit);
	}
}


static void ParseUnitArray(lua_State* L, const char* caller,
                           int table, vector<CUnit*>& unitIDs)
{
	if (!lua_istable(L, table))
		luaL_error(L, "%s(): error parsing unit array", caller);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwnumber(L, -2) || !lua_isnumber(L, -1)) // avoid 'n'
			continue;

		CUnit* unit = ParseUnit(L, __func__, -1);

		if (unit == nullptr)
			continue; // bad pointer

		unitIDs.push_back(unit);
	}
}


/******************************************************************************/

int LuaSyncedCtrl::GiveOrderToUnit(lua_State* L)
{
	CheckAllowGameChanges(L);

	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		luaL_error(L, "Invalid unitID given to GiveOrderToUnit()");

	Command cmd = LuaUtils::ParseCommand(L, __func__, 2);

	if (!CanControlUnit(L, unit)) {
		lua_pushboolean(L, false);
		return 1;
	}

	if (inGiveOrder)
		luaL_error(L, "GiveOrderToUnit() recursion is not permitted");

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
	ParseUnitMap(L, __func__, 1, units);
	const int unitCount = (int)units.size();

	if (unitCount <= 0) {
		lua_pushnumber(L, 0);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __func__, 2);

	if (inGiveOrder) {
		luaL_error(L, "GiveOrderToUnitMap() recursion is not permitted");
	}

	inGiveOrder = true;
	int count = 0;
	for (int i = 0; i < unitCount; i++) {
		CUnit* unit = units[i];
		if (CanControlUnit(L, unit)) {
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
	ParseUnitArray(L, __func__, 1, units);
	const int unitCount = (int)units.size();

	if (unitCount <= 0) {
		lua_pushnumber(L, 0);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __func__, 2);

	if (inGiveOrder) {
		luaL_error(L, "GiveOrderToUnitArray() recursion is not permitted");
	}
	inGiveOrder = true;

	int count = 0;
	for (int i = 0; i < unitCount; i++) {
		CUnit* unit = units[i];
		if (CanControlUnit(L, unit)) {
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
	ParseUnitMap(L, __func__, 1, units);
	const int unitCount = (int)units.size();

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __func__, 2, commands);
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
		if (CanControlUnit(L, unit)) {
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
	ParseUnitArray(L, __func__, 1, units);
	const int unitCount = (int)units.size();

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __func__, 2, commands);
	const int commandCount = (int)commands.size();

	bool pairwise = luaL_optboolean(L, 3, false);

	if ((unitCount <= 0) || (commandCount <= 0)) {
		lua_pushnumber(L, 0);
		return 1;
	}

	if (inGiveOrder) {
		luaL_error(L, "GiveOrderArrayToUnitArray() recursion is not permitted");
	}
	inGiveOrder = true;

	int count = 0;
	if (pairwise) {
		for (int x = 0; x < std::min(unitCount, commandCount); ++x) {
			CUnit* unit = units[x];
			if (CanControlUnit(L, unit)) {
				unit->commandAI->GiveCommand(commands[x]);
				count++;
			}
		}
	}
	else {
		for (int u = 0; u < unitCount; u++) {
			CUnit* unit = units[u];
			if (CanControlUnit(L, unit)) {
				for (int c = 0; c < commandCount; c++) {
					unit->commandAI->GiveCommand(commands[c]);
				}
				count++;
			}
		}
	}
	inGiveOrder = false;

	lua_pushnumber(L, count);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static void ParseParams(lua_State* L, const char* caller, float& factor,
		int& x1, int& z1, int& x2, int& z2, int resolution, int maxX, int maxZ)
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
			std::swap(fx1, fx2);
		}
		if (fz1 > fz2) {
			std::swap(fz1, fz2);
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	// quantize and clamp
	x1 = Clamp((int)(fx1 / resolution), 0, maxX);
	x2 = Clamp((int)(fx2 / resolution), 0, maxX);
	z1 = Clamp((int)(fz1 / resolution), 0, maxZ);
	z2 = Clamp((int)(fz2 / resolution), 0, maxZ);

}

static inline void ParseMapParams(lua_State* L, const char* caller,
		float& factor, int& x1, int& z1, int& x2, int& z2)
{
	ParseParams(L, caller, factor, x1, z1, x2, z2, SQUARE_SIZE, mapDims.mapx, mapDims.mapy);
}


int LuaSyncedCtrl::LevelHeightMap(lua_State* L)
{
	if (mapDamage->Disabled()) {
		return 0;
	}
	float height;
	int x1, x2, z1, z2;
	ParseMapParams(L, __func__, height, x1, z1, x2, z2);

	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			readMap->SetHeight((z * mapDims.mapxp1) + x, height);
		}
	}

	mapDamage->RecalcArea(x1, x2, z1, z2);
	return 0;
}


int LuaSyncedCtrl::AdjustHeightMap(lua_State* L)
{
	if (mapDamage->Disabled()) {
		return 0;
	}

	float height;
	int x1, x2, z1, z2;

	ParseMapParams(L, __func__, height, x1, z1, x2, z2);

	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			readMap->AddHeight((z * mapDims.mapxp1) + x, height);
		}
	}

	mapDamage->RecalcArea(x1, x2, z1, z2);
	return 0;
}


int LuaSyncedCtrl::RevertHeightMap(lua_State* L)
{
	if (mapDamage->Disabled()) {
		return 0;
	}
	float origFactor;
	int x1, x2, z1, z2;
	ParseMapParams(L, __func__, origFactor, x1, z1, x2, z2);

	const float* origMap = readMap->GetOriginalHeightMapSynced();
	const float* currMap = readMap->GetCornerHeightMapSynced();

	if (origFactor == 1.0f) {
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int idx = (z * mapDims.mapxp1) + x;

				readMap->SetHeight(idx, origMap[idx]);
			}
		}
	}
	else {
		const float currFactor = (1.0f - origFactor);
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int index = (z * mapDims.mapxp1) + x;
				const float ofh = origFactor * origMap[index];
				const float cfh = currFactor * currMap[index];
				readMap->SetHeight(index, ofh + cfh);
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
	if ((x < 0) || (x > mapDims.mapx) ||
	    (z < 0) || (z > mapDims.mapy)) {
		return 0;
	}

	const int index = (z * mapDims.mapxp1) + x;
	const float oldHeight = readMap->GetCornerHeightMapSynced()[index];
	heightMapAmountChanged += math::fabsf(h);

	// update RecalcArea()
	if (x < heightMapx1) { heightMapx1 = x; }
	if (x > heightMapx2) { heightMapx2 = x; }
	if (z < heightMapz1) { heightMapz1 = z; }
	if (z > heightMapz2) { heightMapz2 = z; }

	readMap->AddHeight(index, h);
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
	if ((x < 0) || (x > mapDims.mapx) ||
	    (z < 0) || (z > mapDims.mapy)) {
		return 0;
	}

	const int index = (z * mapDims.mapxp1) + x;
	const float oldHeight = readMap->GetCornerHeightMapSynced()[index];
	float height = oldHeight;

	if (lua_israwnumber(L, 4)) {
		const float t = lua_tofloat(L, 4);
		height += (h - oldHeight) * t;
	} else{
		height = h;
	}

	const float heightDiff = (height - oldHeight);
	heightMapAmountChanged += math::fabsf(heightDiff);

	// update RecalcArea()
	if (x < heightMapx1) { heightMapx1 = x; }
	if (x > heightMapx2) { heightMapx2 = x; }
	if (z < heightMapz1) { heightMapz1 = z; }
	if (z > heightMapz2) { heightMapz2 = z; }

	readMap->SetHeight(index, height);
	lua_pushnumber(L, heightDiff);
	return 1;
}


int LuaSyncedCtrl::SetHeightMapFunc(lua_State* L)
{
	if (mapDamage->Disabled()) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isfunction(L, 1)) {
		luaL_error(L, "Incorrect arguments to Spring.SetHeightMapFunc(func, ...)");
	}

	if (inHeightMap) {
		luaL_error(L, "SetHeightMapFunc() recursion is not permitted");
	}

	heightMapx1 = mapDims.mapx;
	heightMapx2 = -1;
	heightMapz1 = mapDims.mapy;
	heightMapz2 = 0;
	heightMapAmountChanged = 0.0f;

	inHeightMap = true;
	const int error = lua_pcall(L, (args - 1), 0, 0);
	inHeightMap = false;

	if (error != 0) {
		LOG_L(L_ERROR, "Spring.SetHeightMapFunc: error(%i) = %s",
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
/* smooth mesh manipulation                                                   */
/******************************************************************************/

static inline void ParseSmoothMeshParams(lua_State* L, const char* caller,
		float& factor, int& x1, int& z1, int& x2, int& z2)
{
	ParseParams(L, caller, factor, x1, z1, x2, z2,
			smoothGround.GetResolution(), smoothGround.GetMaxX(),
			smoothGround.GetMaxY());
}


int LuaSyncedCtrl::LevelSmoothMesh(lua_State* L)
{
	float height;
	int x1, x2, z1, z2;
	ParseSmoothMeshParams(L, __func__, height, x1, z1, x2, z2);

	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			const int index = (z * smoothGround.GetMaxX()) + x;
			smoothGround.SetHeight(index, height);
		}
	}
	return 0;
}

int LuaSyncedCtrl::AdjustSmoothMesh(lua_State* L)
{
	float height;
	int x1, x2, z1, z2;
	ParseSmoothMeshParams(L, __func__, height, x1, z1, x2, z2);

	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			const int index = (z * smoothGround.GetMaxX()) + x;
			smoothGround.AddHeight(index, height);
		}
	}

	return 0;
}

int LuaSyncedCtrl::RevertSmoothMesh(lua_State* L)
{
	float origFactor;
	int x1, x2, z1, z2;
	ParseSmoothMeshParams(L, __func__, origFactor, x1, z1, x2, z2);

	const float* origMap = smoothGround.GetOriginalMeshData();
	const float* currMap = smoothGround.GetMeshData();

	if (origFactor == 1.0f) {
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int idx = (z * smoothGround.GetMaxX()) + x;
				smoothGround.SetHeight(idx, origMap[idx]);
			}
		}
	}
	else {
		const float currFactor = (1.0f - origFactor);
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int index = (z * smoothGround.GetMaxX()) + x;
				const float ofh = origFactor * origMap[index];
				const float cfh = currFactor * currMap[index];
				smoothGround.SetHeight(index, ofh + cfh);
			}
		}
	}

	return 0;
}


int LuaSyncedCtrl::AddSmoothMesh(lua_State* L)
{
	if (!inSmoothMesh) {
		luaL_error(L, "AddSmoothMesh() can only be called in SetSmoothMeshFunc()");
	}

	const float xl = luaL_checkfloat(L, 1);
	const float zl = luaL_checkfloat(L, 2);
	const float h  = luaL_checkfloat(L, 3);

	// quantize
	const int x = (int)(xl / smoothGround.GetResolution());
	const int z = (int)(zl / smoothGround.GetResolution());

	// discard invalid coordinates
	if ((x < 0) || (x > smoothGround.GetMaxX()) ||
	    (z < 0) || (z > smoothGround.GetMaxY())) {
		return 0;
	}

	const int index = (z * smoothGround.GetMaxX()) + x;
	const float oldHeight = smoothGround.GetMeshData()[index];
	smoothMeshAmountChanged += math::fabsf(h);

	smoothGround.AddHeight(index, h);
	// push the new height
	lua_pushnumber(L, oldHeight + h);
	return 1;
}

int LuaSyncedCtrl::SetSmoothMesh(lua_State* L)
{
	if (!inSmoothMesh) {
		luaL_error(L, "SetSmoothMesh() can only be called in SetSmoothMeshFunc()");
	}

	const float xl = luaL_checkfloat(L, 1);
	const float zl = luaL_checkfloat(L, 2);
	const float h  = luaL_checkfloat(L, 3);

	// quantize
	const int x = (int)(xl / smoothGround.GetResolution());
	const int z = (int)(zl / smoothGround.GetResolution());

	// discard invalid coordinates
	if ((x < 0) || (x > smoothGround.GetMaxX()) ||
	    (z < 0) || (z > smoothGround.GetMaxY())) {
		return 0;
	}

	const int index = (z * (smoothGround.GetMaxX())) + x;
	const float oldHeight = smoothGround.GetMeshData()[index];
	float height = oldHeight;

	if (lua_israwnumber(L, 4)) {
		const float t = lua_tofloat(L, 4);
		height += (h - oldHeight) * t;
	} else{
		height = h;
	}

	const float heightDiff = (height - oldHeight);
	smoothMeshAmountChanged += math::fabsf(heightDiff);

	smoothGround.SetHeight(index, height);
	lua_pushnumber(L, heightDiff);
	return 1;
}

int LuaSyncedCtrl::SetSmoothMeshFunc(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isfunction(L, 1)) {
		luaL_error(L, "Incorrect arguments to Spring.SetSmoothMeshFunc(func, ...)");
	}

	if (inSmoothMesh) {
		luaL_error(L, "SetHeightMapFunc() recursion is not permitted");
	}

	heightMapAmountChanged = 0.0f;

	inSmoothMesh = true;
	const int error = lua_pcall(L, (args - 1), 0, 0);
	inSmoothMesh = false;

	if (error != 0) {
		LOG_L(L_ERROR, "Spring.SetSmoothMeshFunc: error(%i) = %s",
				error, lua_tostring(L, -1));
		lua_error(L);
	}

	lua_pushnumber(L, smoothMeshAmountChanged);
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetMapSquareTerrainType(lua_State* L)
{
	const int hx = int(luaL_checkfloat(L, 1) / SQUARE_SIZE);
	const int hz = int(luaL_checkfloat(L, 2) / SQUARE_SIZE);

	if ((hx < 0) || (hx >= mapDims.mapx) || (hz < 0) || (hz >= mapDims.mapy)) {
		luaL_error(L, "Out of range: x = %d z = %d!", hx, hz);
		return 0;
	}

	const int tx = hx >> 1;
	const int tz = hz >> 1;

	const int ott = readMap->GetTypeMapSynced()[tz * mapDims.hmapx + tx];
	const int ntt = luaL_checkint(L, 3);

	readMap->GetTypeMapSynced()[tz * mapDims.hmapx + tx] = std::max(0, std::min(ntt, (CMapInfo::NUM_TERRAIN_TYPES - 1)));
	pathManager->TerrainChange(hx, hz,  hx + 1, hz + 1,  TERRAINCHANGE_SQUARE_TYPEMAP_INDEX);

	lua_pushnumber(L, ott);
	return 1;
}

int LuaSyncedCtrl::SetTerrainTypeData(lua_State* L)
{
	const int args = lua_gettop(L);
	const int tti = luaL_checkint(L, 1);

	if (tti < 0 || tti >= CMapInfo::NUM_TERRAIN_TYPES) {
		lua_pushboolean(L, false);
		return 1;
	}

	CMapInfo::TerrainType* tt = const_cast<CMapInfo::TerrainType*>(&mapInfo->terrainTypes[tti]);
	const CMapInfo::TerrainType ctt = *tt; // copy

	bool ttSpeedModChanged = false;
	bool ttHardnessChanged = false;

	if (args >= 2 && lua_isnumber(L, 2)) ttSpeedModChanged |= (ctt. tankSpeed != (tt-> tankSpeed = lua_tofloat(L, 2)));
	if (args >= 3 && lua_isnumber(L, 3)) ttSpeedModChanged |= (ctt. kbotSpeed != (tt-> kbotSpeed = lua_tofloat(L, 3)));
	if (args >= 4 && lua_isnumber(L, 4)) ttSpeedModChanged |= (ctt.hoverSpeed != (tt->hoverSpeed = lua_tofloat(L, 4)));
	if (args >= 5 && lua_isnumber(L, 5)) ttSpeedModChanged |= (ctt. shipSpeed != (tt-> shipSpeed = lua_tofloat(L, 5)));
	if (args >= 6 && lua_isnumber(L, 6)) ttHardnessChanged |= (ctt.  hardness != (tt->  hardness = lua_tofloat(L, 6)));

	if (args >= 7 && lua_isboolean(L, 7))
		tt->receiveTracks = lua_toboolean(L, 7);
	if (args >= 8 && lua_isstring(L, 8))
		tt->name = lua_tostring(L, 8);

	// hardness changes do not require repathing
	if (ttHardnessChanged)
		mapDamage->TerrainTypeHardnessChanged(tti);
	if (ttSpeedModChanged)
		mapDamage->TerrainTypeSpeedModChanged(tti);

	lua_pushboolean(L, true);
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetSquareBuildingMask(lua_State* L)
{
	const int x = luaL_checkint(L, 1);
	const int z = luaL_checkint(L, 2);
	const int mask = luaL_checkint(L, 3);

	if (mask < 0 || mask > USHRT_MAX) {
		luaL_error(L, "Incorrect value of mask: %s(%d, %d, %d)", __func__, x, z, mask);
		return 0;
	}

	if (!buildingMaskMap.SetTileMask(x, z, (std::uint16_t)mask)) {
		luaL_error(L, "Invalid values supplied: %s(%d, %d, %d)", __func__, x, z, mask);
		return 0;
	}

	return 0; //no error = success
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::UnitWeaponFire(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t idx = static_cast<size_t>(luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX);
	if (idx >= unit->weapons.size())
		return 0;

	unit->weapons[idx]->Fire(false);
	return 0;
}

int LuaSyncedCtrl::UnitWeaponHoldFire(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const size_t idx = static_cast<size_t>(luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX);
	if (idx >= unit->weapons.size())
		return 0;

	unit->weapons[idx]->DropCurrentTarget();
	return 0;
}

int LuaSyncedCtrl::UnitAttach(lua_State* L)
{
	CUnit* transporter = ParseUnit(L, __func__, 1);

	if (transporter == nullptr)
		return 0;

	CUnit* transportee = ParseUnit(L, __func__, 2);

	if (transportee == nullptr)
		return 0;

	int piece = luaL_checkint(L, 3) - 1;
	const auto& pieces = transporter->localModel.pieces;

	if (piece >= (int) pieces.size()) {
		luaL_error(L,  "invalid piece number");
		return 0;
	}

	if (piece >= 0)
		piece = pieces[piece].scriptPieceIndex;

	transporter->AttachUnit(transportee, piece, !transporter->unitDef->IsTransportUnit());
	return 0;
}

int LuaSyncedCtrl::UnitDetach(lua_State* L)
{
	CUnit* transportee = ParseUnit(L, __func__, 1);

	if (transportee == nullptr)
		return 0;

	CUnit* transporter = transportee->GetTransporter();

	if (transporter == nullptr)
		return 0;

	transporter->DetachUnit(transportee);
	return 0;
}


int LuaSyncedCtrl::UnitDetachFromAir(lua_State* L)
{
	CUnit* transportee = ParseUnit(L, __func__, 1);

	if (transportee == nullptr)
		return 0;

	CUnit* transporter = transportee->GetTransporter();

	if (transporter == nullptr)
		return 0;

	const int args = lua_gettop(L);
	float3 pos;
	if (args >= 4) {
		pos = float3(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));
	} else {
		pos = transportee->pos;
		pos.y = CGround::GetHeightAboveWater(pos.x, pos.z);
	}

	transporter->DetachUnitFromAir(transportee, pos);

	return 0;
}


int LuaSyncedCtrl::SetUnitLoadingTransport(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (lua_isnil(L, 2)) {
		unit->loadingTransportId = -1;
		return 0;
	}

	CUnit* transport = ParseUnit(L, __func__, 2);

	if (transport == nullptr)
		return 0;

	unit->loadingTransportId = transport->id;

	return 0;
}


int LuaSyncedCtrl::SpawnProjectile(lua_State* L)
{
	ProjectileParams params;

	if ((params.weaponDef = weaponDefHandler->GetWeaponDefByID(luaL_checkint(L, 1))) == nullptr)
		return 0;

	if (!ParseProjectileParams(L, params, 2, __func__))
		return 0;

	lua_pushnumber(L, WeaponProjectileFactory::LoadProjectile(params));
	return 1;
}


int LuaSyncedCtrl::DeleteProjectile(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __func__, 1);

	if (proj == nullptr)
		return 0;

	proj->Delete();
	return 0;
}

// Slight repetition with SetSingleDynDamagesKey, but it may be ugly to combine them
static int SetSingleDamagesKey(lua_State* L, DamageArray& damages, int index)
{
	const float value = lua_tofloat(L, index + 1);

	if (lua_isnumber(L, index)) {
		const unsigned armType = lua_toint(L, index);
		if (armType < damages.GetNumTypes())
			damages.Set(armType, std::max(value, 0.0001f));
		return 0;
	}

	// FIXME: missing checks and updates?
	switch (hashString(lua_tostring(L, index))) {
		case hashString("paralyzeDamageTime"): {
			damages.paralyzeDamageTime = std::max((int)value, 0);
		} break;

		case hashString("impulseFactor"): {
			damages.impulseFactor = value;
		} break;
		case hashString("impulseBoost"): {
			damages.impulseBoost = value;
		} break;

		case hashString("craterMult"): {
			damages.craterMult = value;
		} break;
		case hashString("craterBoost"): {
			damages.craterBoost = value;
		} break;
		default: {
		} break;
	}

	return 0;
}


static int SetExplosionParam(lua_State* L, CExplosionParams& params, DamageArray& damages, int index)
{
	switch (hashString(lua_tostring(L, index))) {
		case hashString("damages"): {
			if (lua_istable(L, index + 1)) {
				// {key1 = value1, ...}
				for (lua_pushnil(L); lua_next(L, index + 1) != 0; lua_pop(L, 1)) {
					if ((lua_isnumber(L, -2) || lua_israwstring(L, -2)) && lua_isnumber(L, -1)) {
						SetSingleDamagesKey(L, damages, -2);
					}
				}
			} else {
				damages.SetDefaultDamage(std::max(lua_tofloat(L, index + 1), 0.0001f));
			}
		} break;
		case hashString("weaponDef"): {
			params.weaponDef = weaponDefHandler->GetWeaponDefByID(lua_tofloat(L, index + 1));
		} break;
		case hashString("owner"): {
			params.owner = ParseUnit(L, __func__, index + 1);
		} break;

		case hashString("hitUnit"): {
			params.hitUnit = ParseUnit(L, __func__, index + 1);
		} break;
		case hashString("hitFeature"): {
			params.hitFeature = ParseFeature(L, __func__, index + 1);
		} break;

		case hashString("craterAreaOfEffect"): {
			params.craterAreaOfEffect = lua_tofloat(L, index + 1);
		} break;
		case hashString("damageAreaOfEffect"): {
			params.damageAreaOfEffect = lua_tofloat(L, index + 1);
		} break;

		case hashString("edgeEffectiveness"): {
			params.edgeEffectiveness = lua_tofloat(L, index + 1);
		} break;
		case hashString("explosionSpeed"): {
			params.explosionSpeed = lua_tofloat(L, index + 1);
		} break;

		case hashString("gfxMod"): {
			params.gfxMod = lua_tofloat(L, index + 1);
		} break;

		case hashString("projectileID"): {
			params.projectileID = static_cast<unsigned int>(lua_toint(L, index + 1));
		} break;

		case hashString("impactOnly"): {
			params.impactOnly = lua_toboolean(L, index + 1);
		} break;
		case hashString("ignoreOwner"): {
			params.ignoreOwner = lua_toboolean(L, index + 1);
		} break;
		case hashString("damageGround"): {
			params.damageGround = lua_toboolean(L, index + 1);
		} break;

		default: {
			luaL_error(L, "[%s] illegal explosion param \"%s\"", __func__, lua_tostring(L, index));
		} break;
	}

	return 0;
}


int LuaSyncedCtrl::SpawnExplosion(lua_State* L)
{
	const float3 pos = {luaL_checkfloat(L, 1      ), luaL_checkfloat(L, 2      ), luaL_checkfloat(L, 3      )};
	const float3 dir = {luaL_optfloat  (L, 4, 0.0f), luaL_optfloat  (L, 5, 0.0f), luaL_optfloat  (L, 6, 0.0f)};

	if (lua_istable(L, 7)) {
		DamageArray damages(1.0f);
		CExplosionParams params = {
			pos,
			dir,
			damages,
			nullptr,           // weaponDef
			nullptr,           // owner
			nullptr,           // hitUnit
			nullptr,           // hitFeature
			0.0f,              // craterAreaOfEffect
			0.0f,              // damageAreaOfEffect
			0.0f,              // edgeEffectiveness
			0.0f,              // explosionSpeed
			0.0f,              // gfxMod (scale-mult for *S*EG's)
			false,             // impactOnly
			false,             // ignoreOwner
			false,             // damageGround
			static_cast<unsigned int>(-1)
		};

		for (lua_pushnil(L); lua_next(L, 7) != 0; lua_pop(L, 1)) {
			SetExplosionParam(L, params, damages, -2);
		}

		helper->Explosion(params);
	} else {
		DamageArray damages(luaL_optfloat(L, 7, 1.0f));
		CExplosionParams params = {pos, dir, damages};

		// parse remaining arguments in order of expected usage frequency
		params.weaponDef  = weaponDefHandler->GetWeaponDefByID(luaL_optint(L, 16, -1));
		params.owner      = ParseUnit   (L, __func__, 18);
		params.hitUnit    = ParseUnit   (L, __func__, 19);
		params.hitFeature = ParseFeature(L, __func__, 20);

		params.craterAreaOfEffect = luaL_optfloat(L,  8, 0.0f);
		params.damageAreaOfEffect = luaL_optfloat(L,  9, 0.0f);
		params.edgeEffectiveness  = luaL_optfloat(L, 10, 0.0f);
		params.explosionSpeed     = luaL_optfloat(L, 11, 0.0f);
		params.gfxMod             = luaL_optfloat(L, 12, 0.0f);

		params.impactOnly   = luaL_optboolean(L, 13, false);
		params.ignoreOwner  = luaL_optboolean(L, 14, false);
		params.damageGround = luaL_optboolean(L, 15, false);

		params.projectileID = static_cast<unsigned int>(luaL_optint(L, 17, -1));

		helper->Explosion(params);
	}

	return 0;
}

int LuaSyncedCtrl::SpawnCEG(lua_State* L)
{
	const float3 pos = {luaL_optfloat(L, 2, 0.0f), luaL_optfloat(L, 3, 0.0f), luaL_optfloat(L, 4, 0.0f)};
	const float3 dir = {luaL_optfloat(L, 5, 0.0f), luaL_optfloat(L, 6, 0.0f), luaL_optfloat(L, 7, 0.0f)};

	const float radius = luaL_optfloat(L,  8, 0.0f);
	const float damage = luaL_optfloat(L,  9, 0.0f);
	const float dmgMod = luaL_optfloat(L, 10, 1.0f);

	unsigned int cegID = CExplosionGeneratorHandler::EXPGEN_ID_INVALID;

	if (lua_isstring(L, 1)) {
		// args from Lua are assumed not to include the prefix
		// (Spawn*C*EG implies only custom generators can fire)
		cegID = explGenHandler.LoadGeneratorID(std::string(CEG_PREFIX_STRING) + lua_tostring(L, 1));
	} else {
		cegID = luaL_checknumber(L, 1);
	}

	lua_pushboolean(L, explGenHandler.GenExplosion(cegID, pos, dir, damage, radius, dmgMod, nullptr, nullptr));
	lua_pushnumber(L, cegID);
	return 2;
}

int LuaSyncedCtrl::SpawnSFX(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const float3 pos = {luaL_checkfloat(L, 3), luaL_checkfloat(L, 4), luaL_checkfloat(L, 5)};
	const float3 dir = {luaL_checkfloat(L, 6), luaL_checkfloat(L, 7), luaL_checkfloat(L, 8)};

	if (luaL_optboolean(L, 9, true)) {
		lua_pushboolean(L, unit->script->EmitAbsSFX(luaL_checkint(L, 2), pos, dir));
	} else {
		lua_pushboolean(L, unit->script->EmitRelSFX(luaL_checkint(L, 2), pos, dir));
	}

	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetNoPause(lua_State* L)
{
	if (!FullCtrl(L))
		return 0;

	// Important: only works in server mode, has no effect in client mode
	if (gameServer != nullptr)
		gameServer->SetGamePausable(!luaL_checkboolean(L, 1));

	return 0;
}


int LuaSyncedCtrl::SetUnitToFeature(lua_State* L)
{
	if (!FullCtrl(L))
		return 0;

	CUnit::SetSpawnFeature(luaL_checkboolean(L, 1));
	return 0;
}


int LuaSyncedCtrl::SetExperienceGrade(lua_State* L)
{
	if (!FullCtrl(L))
		return 0;

	CUnit::SetExpGrade(luaL_checkfloat(L, 1));

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


int LuaSyncedCtrl::SetRadarErrorParams(lua_State* L)
{
	const int allyTeamID = lua_tonumber(L, 1);

	if (!teamHandler.IsValidAllyTeam(allyTeamID))
		return 0;

	losHandler->SetAllyTeamRadarErrorSize(allyTeamID, luaL_checknumber(L, 2));
	losHandler->SetBaseRadarErrorSize(luaL_optnumber(L, 3, losHandler->GetBaseRadarErrorSize()));
	losHandler->SetBaseRadarErrorMult(luaL_optnumber(L, 4, losHandler->GetBaseRadarErrorMult()));
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


/******************************************************************************/

static bool ParseCommandDescription(lua_State* L, int table,
                                    SCommandDescription& cd)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "Can not parse CommandDescription");
		return false;
	}

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const string key = lua_tostring(L, -2);

		if (ParseNamedInt(L,    key, "id",          cd.id)         ||
		    ParseNamedInt(L,    key, "type",        cd.type)       ||
		    ParseNamedString(L, key, "name",        cd.name)       ||
		    ParseNamedString(L, key, "action",      cd.action)     ||
		    ParseNamedString(L, key, "tooltip",     cd.tooltip)    ||
		    ParseNamedString(L, key, "texture",     cd.iconname)   ||
		    ParseNamedString(L, key, "cursor",      cd.mouseicon)  ||
		    ParseNamedBool(L,   key, "queueing",    cd.queueing)   ||
		    ParseNamedBool(L,   key, "hidden",      cd.hidden)     ||
		    ParseNamedBool(L,   key, "disabled",    cd.disabled)   ||
		    ParseNamedBool(L,   key, "showUnique",  cd.showUnique) ||
		    ParseNamedBool(L,   key, "onlyTexture", cd.onlyTexture)) {
			continue; // successfully parsed a parameter
		}

		if ((key != "params") || !lua_istable(L, -1))
			luaL_error(L, "Unknown cmdDesc parameter %s", key.c_str());

		// collect the parameters
		const int paramTable = lua_gettop(L);
		ParseStringVector(L, paramTable, cd.params);
	}

	return true;
}


int LuaSyncedCtrl::EditUnitCmdDesc(lua_State* L)
{
	if (!FullCtrl(L))
		return 0;

	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const auto& cmdDescs = unit->commandAI->GetPossibleCommands();

	const unsigned int cmdDescIdx = luaL_checkint(L, 2) - 1;

	if (cmdDescIdx >= cmdDescs.size())
		return 0;

	// note: must be a copy
	SCommandDescription cmdDesc = *cmdDescs[cmdDescIdx];

	ParseCommandDescription(L, 3, cmdDesc);
	unit->commandAI->UpdateCommandDescription(cmdDescIdx, std::move(cmdDesc));
	return 0;
}


int LuaSyncedCtrl::InsertUnitCmdDesc(lua_State* L)
{
	if (!FullCtrl(L))
		return 0;

	const int args = lua_gettop(L); // number of arguments

	if ((args == 2) && !lua_istable(L, 2))
		luaL_error(L, "Incorrect arguments to InsertUnitCmdDesc/2");

	if ((args >= 3) && (!lua_isnumber(L, 2) || !lua_istable(L, 3)))
		luaL_error(L, "Incorrect arguments to InsertUnitCmdDesc/3");

	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int tableIdx = 2 + int(args >= 3);

	// insert behind last by default
	unsigned int cmdDescIdx = -1u;

	if (args >= 3)
		cmdDescIdx = lua_toint(L, 2) - 1;

	SCommandDescription cd;

	ParseCommandDescription(L, tableIdx, cd);
	unit->commandAI->InsertCommandDescription(cmdDescIdx, std::move(cd));
	return 0;
}

int LuaSyncedCtrl::RemoveUnitCmdDesc(lua_State* L)
{
	if (!FullCtrl(L))
		return 0;

	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	// remove last by default
	unsigned int cmdDescIdx = unit->commandAI->possibleCommands.size() - 1;

	if (lua_isnumber(L, 2))
		cmdDescIdx = lua_toint(L, 2) - 1;

	unit->commandAI->RemoveCommandDescription(cmdDescIdx);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
