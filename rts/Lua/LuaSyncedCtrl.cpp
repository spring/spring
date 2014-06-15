/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <set>
#include <list>
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
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/Scripts/LuaUnitScript.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/UnitTypes/ExtractorBuilding.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/ObjectDependenceTypes.h"
#include "System/Log/ILog.h"
#include "System/Sync/HsiehHash.h"

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

static int heightMapx1;
static int heightMapx2;
static int heightMapz1;
static int heightMapz2;
static float heightMapAmountChanged;

static float smoothMeshAmountChanged;


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
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(SetAlly);
	REGISTER_LUA_CFUNC(KillTeam);
	REGISTER_LUA_CFUNC(AssignPlayerToTeam);
	REGISTER_LUA_CFUNC(GameOver);

	REGISTER_LUA_CFUNC(AddTeamResource);
	REGISTER_LUA_CFUNC(UseTeamResource);
	REGISTER_LUA_CFUNC(SetTeamResource);
	REGISTER_LUA_CFUNC(SetTeamShareLevel);
	REGISTER_LUA_CFUNC(ShareTeamResource);

	REGISTER_LUA_CFUNC(SetUnitRulesParam);
	REGISTER_LUA_CFUNC(SetTeamRulesParam);
	REGISTER_LUA_CFUNC(SetGameRulesParam);

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
	REGISTER_LUA_CFUNC(SetUnitNeutral);
	REGISTER_LUA_CFUNC(SetUnitTarget);
	REGISTER_LUA_CFUNC(SetUnitMidAndAimPos);
	REGISTER_LUA_CFUNC(SetUnitRadiusAndHeight);
	REGISTER_LUA_CFUNC(SetUnitCollisionVolumeData);
	REGISTER_LUA_CFUNC(SetUnitPieceCollisionVolumeData);
	REGISTER_LUA_CFUNC(SetUnitSensorRadius);
	REGISTER_LUA_CFUNC(SetUnitPosErrorParams);

	REGISTER_LUA_CFUNC(SetUnitPhysics);
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
	REGISTER_LUA_CFUNC(SetFeatureReclaim);
	REGISTER_LUA_CFUNC(SetFeatureResurrect);
	REGISTER_LUA_CFUNC(SetFeaturePhysics);
	REGISTER_LUA_CFUNC(SetFeaturePosition);
	REGISTER_LUA_CFUNC(SetFeatureVelocity);
	REGISTER_LUA_CFUNC(SetFeatureDirection);
	REGISTER_LUA_CFUNC(SetFeatureBlocking);
	REGISTER_LUA_CFUNC(SetFeatureNoSelect);
	REGISTER_LUA_CFUNC(SetFeatureMidAndAimPos);
	REGISTER_LUA_CFUNC(SetFeatureRadiusAndHeight);
	REGISTER_LUA_CFUNC(SetFeatureCollisionVolumeData);


	REGISTER_LUA_CFUNC(SetProjectileAlwaysVisible);
	REGISTER_LUA_CFUNC(SetProjectileMoveControl);
	REGISTER_LUA_CFUNC(SetProjectilePosition);
	REGISTER_LUA_CFUNC(SetProjectileVelocity);
	REGISTER_LUA_CFUNC(SetProjectileCollision);
	REGISTER_LUA_CFUNC(SetProjectileTarget);

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

	REGISTER_LUA_CFUNC(UnitWeaponFire);
	REGISTER_LUA_CFUNC(UnitWeaponHoldFire);

	REGISTER_LUA_CFUNC(SpawnProjectile);
	REGISTER_LUA_CFUNC(SpawnCEG);

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
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= unitHandler->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %d\n", caller, unitID);
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
	if (!CanControlUnit(L, unit)) {
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
	CFeature* f = featureHandler->GetFeature(featureID);

	if (!f)
		return NULL;

	if (!CanControlFeature(L, f)) {
		return NULL;
	}
	return f;
}


static inline CProjectile* ParseProjectile(lua_State* L,
                                           const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad projectile ID", caller);
	}
	const ProjectileMapValPair* pmp = projectileHandler->GetMapPairBySyncedID(lua_toint(L, index));
	if (pmp == NULL) {
		return NULL;
	}
	if (!CanControlProjectileAllyTeam(L, pmp->second)) {
		return NULL;
	}
	return pmp->first;
}

static bool ParseProjectileParams(lua_State* L, ProjectileParams& params, const int tblIdx, const char* caller)
{
	if (!lua_istable(L, tblIdx)) {
		luaL_error(L, "[%s] argument %d must be a table!", caller, tblIdx);
		return false;
	}

	for (lua_pushnil(L); lua_next(L, tblIdx) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2)) {
			const std::string& key = lua_tostring(L, -2);

			if (lua_istable(L, -1)) {
				float array[3] = {0.0f, 0.0f, 0.0f};

				if (LuaUtils::ParseFloatArray(L, -1, array, 3) == 3) {
				    if (key == "pos") {
						params.pos = array;
					} else if (key == "end") {
						params.end = array;
					} else if (key == "speed") {
						params.speed = array;
					} else if (key == "spread") {
						params.spread = array;
					} else if (key == "error") {
						params.error = array;
					}
				}

				continue;
			}

			if (lua_isnumber(L, -1)) {
				if (key == "owner") {
					params.ownerID = lua_toint(L, -1);
				} else if (key == "team") {
					params.teamID = lua_toint(L, -1);
				} else if (key == "ttl") {
					params.ttl = lua_tofloat(L, -1);
				} else if (key == "gravity") {
					params.gravity = lua_tofloat(L, -1);
				} else if (key == "tracking") {
					params.tracking = lua_tofloat(L, -1);
				} else if (key == "maxRange") {
					params.maxRange = lua_tofloat(L, -1);
				} else if (key == "startAlpha") {
					params.startAlpha = lua_tofloat(L, -1);
				} else if (key == "endAlpha") {
					params.endAlpha = lua_tofloat(L, -1);
				}

				continue;
			}

			if (lua_isstring(L, -1)) {
				if (key == "model") {
					params.model = modelParser->Load3DModel(lua_tostring(L, -1));
				} else if (key == "cegtag") {
					params.cegID = explGenHandler->LoadGeneratorID(lua_tostring(L, -1));
				}

				continue;
			}
		}
	}

	return true;
}

static CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad teamID", caller);
		return NULL;
	}
	const int teamID = lua_toint(L, index);
	if (!teamHandler->IsValidTeam(teamID)) {
		luaL_error(L, "%s(): Bad teamID: %d", caller, teamID);
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return NULL;
	}
	return team;
}

static int SetSolidObjectCollisionVolumeData(lua_State* L, CSolidObject* o)
{
	if (o == NULL)
		return 0;

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

	o->collisionVolume->InitShape(scales, offsets, vType, tType, pAxis);
	return 0;
}

static int SetSolidObjectBlocking(lua_State* L, CSolidObject* o)
{
	if (o == NULL)
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

static int SetSolidObjectDirection(lua_State* L, CSolidObject* o)
{
	if (o == NULL)
		return 0;

	o->ForcedSpin((float3(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4))).SafeNormalize());
	return 0;
}

static int SetWorldObjectVelocity(lua_State* L, CWorldObject* o)
{
	if (o == NULL)
		return 0;

	float3 speed;
	speed.x = Clamp(luaL_checkfloat(L, 2), -MAX_UNIT_SPEED, MAX_UNIT_SPEED);
	speed.y = Clamp(luaL_checkfloat(L, 3), -MAX_UNIT_SPEED, MAX_UNIT_SPEED);
	speed.z = Clamp(luaL_checkfloat(L, 4), -MAX_UNIT_SPEED, MAX_UNIT_SPEED);

	o->SetVelocityAndSpeed(speed);
	return 0;
}

static int SetSolidObjectPhysicalState(lua_State* L, CSolidObject* o)
{
	if (o == NULL)
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

	CMatrix44f matrix;
	matrix.RotateZ(rot.z);
	matrix.RotateX(rot.x);
	matrix.RotateY(rot.y);

	o->Move(pos, false);
	o->SetDirVectors(matrix);
	o->UpdateMidAndAimPos();
	o->SetHeadingFromDirection();
	// do not need ForcedSpin, above three calls cover it
	o->ForcedMove(pos);
	o->SetVelocityAndSpeed(speed);
	return 0;
}

static int SetWorldObjectAlwaysVisible(lua_State* L, CWorldObject* o, const char* caller)
{
	if (o == NULL)
		return 0;
	o->alwaysVisible = luaL_checkboolean(L, 2);
	return 0;
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

	if (!teamHandler->IsValidAllyTeam(firstAllyTeamID))
		return 0;
	if (!teamHandler->IsValidAllyTeam(secondAllyTeamID))
		return 0;

	teamHandler->SetAlly(firstAllyTeamID, secondAllyTeamID, luaL_checkboolean(L, 3));
	return 0;
}

int LuaSyncedCtrl::KillTeam(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);

	if (!teamHandler->IsValidTeam(teamID))
		return 0;

	//FIXME either we disallow it here or it needs modifications in GameServer.cpp (it creates a `teams` vector w/o gaia)
	//  possible fix would be to always create the Gaia team (currently it's conditional on gs->useLuaGaia)
	if (teamID == teamHandler->GaiaTeamID())
		return 0;

	CTeam* team = teamHandler->Team(teamID);

	if (team == NULL)
		return 0;

	team->Died();
	return 0;
}

int LuaSyncedCtrl::AssignPlayerToTeam(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	const int teamID = luaL_checkint(L, 2);

	if (!playerHandler->IsValidPlayer(playerID))
		return 0;
	if (!teamHandler->IsValidTeam(teamID))
		return 0;

	teamHandler->Team(teamID)->AddPlayer(playerID);
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
		if (!lua_israwnumber(L, -1)) {
			continue;
		}

		const unsigned char allyTeamID = lua_toint(L, -1);

		if (!teamHandler->ValidAllyTeam(allyTeamID)) {
			continue;
		}

		winningAllyTeams.push_back(allyTeamID);
	}

	game->GameEnd(winningAllyTeams);
	// push the number of accepted allyteams
	lua_pushnumber(L, winningAllyTeams.size());
	return 1;
}


int LuaSyncedCtrl::AddTeamResource(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler->IsValidTeam(teamID)) {
		return 0;
	}
	if (!CanControlTeam(L, teamID)) {
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
	if (!teamHandler->IsValidTeam(teamID)) {
		return 0;
	}
	if (!CanControlTeam(L, teamID)) {
		return 0;
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	if (lua_isstring(L, 2)) {
		const string type = lua_tostring(L, 2);

		const float value = max(0.0f, luaL_checkfloat(L, 3));

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
				const float value = max(0.0f, lua_tofloat(L, -1));
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
	if (!teamHandler->IsValidTeam(teamID)) {
		return 0;
	}
	if (!CanControlTeam(L, teamID)) {
		return 0;
	}
	CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	const string type = luaL_checkstring(L, 2);

	const float value = max(0.0f, luaL_checkfloat(L, 3));

	if ((type == "m") || (type == "metal")) {
		team->metal = min<float>(team->metalStorage, value);
	}
	else if ((type == "e") || (type == "energy")) {
		team->energy = min<float>(team->energyStorage, value);
	}
	else if ((type == "ms") || (type == "metalStorage")) {
		team->metalStorage = value;
		team->metal = min<float>(team->metal, team->metalStorage);
	}
	else if ((type == "es") || (type == "energyStorage")) {
		team->energyStorage = value;
		team->energy = min<float>(team->energy, team->energyStorage);
	}
	return 0;
}


int LuaSyncedCtrl::SetTeamShareLevel(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if (!teamHandler->IsValidTeam(teamID)) {
		return 0;
	}
	if (!CanControlTeam(L, teamID)) {
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


int LuaSyncedCtrl::ShareTeamResource(lua_State* L)
{
	const int teamID1 = luaL_checkint(L, 1);
	if (!teamHandler->IsValidTeam(teamID1)) {
		luaL_error(L, "Incorrect arguments to ShareTeamResource(teamID1, teamID2, type, amount)");
	}
	if (!CanControlTeam(L, teamID1)) {
		return 0;
	}
	CTeam* team1 = teamHandler->Team(teamID1);
	if (team1 == NULL) {
		return 0;
	}

	const int teamID2 = luaL_checkint(L, 2);
	if (!teamHandler->IsValidTeam(teamID2)) {
		luaL_error(L, "Incorrect arguments to ShareTeamResource(teamID1, teamID2, type, amount)");
	}
	CTeam* team2 = teamHandler->Team(teamID2);
	if (team2 == NULL) {
		return 0;
	}

	const string type = luaL_checkstring(L, 3);
	float amount = luaL_checkfloat(L, 4);

	if (type == "metal") {
		amount = std::min(amount, (float)team1->metal);
		if (eventHandler.AllowResourceTransfer(teamID1, teamID2, "m", amount)) { //FIXME can cause an endless loop
			team1->metal                       -= amount;
			team1->metalSent                   += amount;
			team1->currentStats->metalSent     += amount;
			team2->metal                       += amount;
			team2->metalReceived               += amount;
			team2->currentStats->metalReceived += amount;
		}
	} else if (type == "energy") {
		amount = std::min(amount, (float)team1->energy);
		if (eventHandler.AllowResourceTransfer(teamID1, teamID2, "e", amount)) { //FIXME can cause an endless loop
			team1->energy                       -= amount;
			team1->energySent                   += amount;
			team1->currentStats->energySent     += amount;
			team2->energy                       += amount;
			team2->energyReceived               += amount;
			team2->currentStats->energyReceived += amount;
		}
	}
	return 0;
}



/******************************************************************************/

void SetRulesParam(lua_State* L, const char* caller, int offset,
				LuaRulesParams::Params& params,
				LuaRulesParams::HashMap& paramsMap)
{
	const int index = offset + 1;
	const int valIndex = offset + 2;
	const int losIndex = offset + 3;
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
			paramsMap[pName] = pIndex;
			params.push_back(LuaRulesParams::Param());
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	if ((pIndex < 0)
		|| (pIndex >= (int)params.size())
		|| !(lua_isnumber(L, valIndex) || lua_isstring(L, valIndex))
	) {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	LuaRulesParams::Param& param = params[pIndex];

	//! set the value of the parameter
	if (lua_isnumber(L, valIndex)) {
		param.valueInt = lua_tofloat(L, valIndex);
		param.valueString.resize(0);
	} else {
		param.valueString = lua_tostring(L, valIndex);
	}

	//! set the los checking of the parameter
	if (lua_istable(L, losIndex)) {
		const int table = losIndex;
		int losMask = LuaRulesParams::RULESPARAMLOS_PRIVATE;

		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			//! ignore if the value is false
			if (!luaL_optboolean(L, -1, true)) {
				continue;
			}

			//! read the losType from the key
			if (lua_isstring(L, -2)) {
				const string losType = lua_tostring(L, -2);

				if (losType == "public") {
					losMask |= LuaRulesParams::RULESPARAMLOS_PUBLIC;
				}
				else if (losType == "inlos") {
					losMask |= LuaRulesParams::RULESPARAMLOS_INLOS;
				}
				else if (losType == "inradar") {
					losMask |= LuaRulesParams::RULESPARAMLOS_INRADAR;
				}
				else if (losType == "allied") {
					losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED;
				}
				/*else if (losType == "private") {
					losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE; //! default
				}*/
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
	SetRulesParam(L, __FUNCTION__, 0, CLuaHandleSynced::gameParams, CLuaHandleSynced::gameParamsMap);
	return 0;
}


int LuaSyncedCtrl::SetTeamRulesParam(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	SetRulesParam(L, __FUNCTION__, 1, team->modParams, team->modParamsMap);
	return 0;
}


int LuaSyncedCtrl::SetUnitRulesParam(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	SetRulesParam(L, __FUNCTION__, 1, unit->modParams, unit->modParamsMap);
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
		// no error - allows using this to determine whether unit runs COB or LUS
		return 0;
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

	if (inCreateUnit) {
		luaL_error(L, "[%s()]: recursion is not permitted", __FUNCTION__);
		return 0;
	}

	const UnitDef* unitDef = NULL;

	if (lua_israwstring(L, 1)) {
		unitDef = unitDefHandler->GetUnitDefByName(lua_tostring(L, 1));
	} else if (lua_israwnumber(L, 1)) {
		unitDef = unitDefHandler->GetUnitDefByID(lua_toint(L, 1));
	} else {
		luaL_error(L, "[%s()] incorrect type for first argument", __FUNCTION__);
		return 0;
	}

	if (unitDef == NULL) {
		if (lua_israwstring(L, 1)) {
			luaL_error(L, "[%s()]: bad unitDef name: %s", __FUNCTION__, lua_tostring(L, 1));
		} else {
			luaL_error(L, "[%s()]: bad unitDef ID: %d", __FUNCTION__, lua_toint(L, 1));
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
	const int facing = LuaUtils::ParseFacing(L, __FUNCTION__, 5);
	const bool beingBuilt = luaL_optboolean(L, 7, false);
	const bool flattenGround = luaL_optboolean(L, 8, true);
	int teamID = luaL_optint(L, 6, CtrlTeam(L));

	if (!teamHandler->IsValidTeam(teamID)) {
		luaL_error(L, "[%s()]: invalid team number (%d)", __FUNCTION__, teamID);
		return 0;
	}
	if (!FullCtrl(L) && (CtrlTeam(L) != teamID)) {
		luaL_error(L, "[%s()]: not a controllable team (%d)", __FUNCTION__, teamID);
		return 0;
	}
	if (!unitHandler->CanBuildUnit(unitDef, teamID)) {
		return 0; // unit limit reached
	}

	ASSERT_SYNCED(pos);
	ASSERT_SYNCED(facing);

	inCreateUnit = true;
	UnitLoadParams params;
	params.unitDef = unitDef; /// must be non-NULL
	params.builder = unitHandler->GetUnit(luaL_optint(L, 10, -1)); /// may be NULL
	params.pos     = pos;
	params.speed   = ZeroVector;
	params.unitID  = luaL_optint(L, 9, -1);
	params.teamID  = teamID;
	params.facing  = facing;
	params.beingBuilt = beingBuilt;
	params.flattenGround = flattenGround;

	CUnit* unit = unitLoader->LoadUnit(params);
	inCreateUnit = false;

	if (unit != NULL) {
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

	bool selfd = luaL_optboolean(L, 2, false);
	bool reclaimed = luaL_optboolean(L, 3, false);

	CUnit* attacker = NULL;
	if (args >= 4) {
		attacker = ParseUnit(L, __FUNCTION__, 4);
	}

	if (inDestroyUnit) {
		luaL_error(L, "DestroyUnit() recursion is not permitted");
	}
	inDestroyUnit = true;
	ASSERT_SYNCED(unit->id);
	unit->KillUnit(attacker, selfd, reclaimed);
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
	if (!teamHandler->IsValidTeam(newTeam)) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(newTeam);
	if (team == NULL) {
		return 0;
	}

	bool given = true;
	if (FullCtrl(L) && lua_isboolean(L, 3)) {
		given = lua_toboolean(L, 3);
	}

	if (inTransferUnit) {
		luaL_error(L, "TransferUnit() recursion is not permitted");
	}
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
		const float value = lua_tofloat(L, -1);
		ASSERT_SYNCED((float)value);

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

	value *= 0.5f;

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
			const float value = lua_tofloat(L, -1);
			ASSERT_SYNCED((float)value);

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
	const char *tmp = luaL_checkstring(L, 2);
	if (tmp)
		unit->tooltip = string(tmp, lua_strlen(L, 2));
	else
		unit->tooltip = "";
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
					if (unit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health)) {
						unit->SetStunned(true);
					} else if (value < 0.0f) {
						unit->SetStunned(false);
					}
				}
				else if (key == "build") {
					unit->buildProgress = value;

					if (unit->buildProgress >= 1.0f) {
						unit->FinishedBuilding(false);
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
	unit->maxHealth = std::max(0.1f, luaL_checkfloat(L, 2));

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
	if (key == "reloadState" || key == "reloadFrame") {
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
		weapon->UpdateRange(value);
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

	const size_t weaponNum = luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX;

	if (weaponNum >= unit->weapons.size()) {
		return 0;
	}

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
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int allyTeam = luaL_checkint(L, 2);
	if (!teamHandler->IsValidAllyTeam(allyTeam)) {
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
	if (!teamHandler->IsValidAllyTeam(allyTeam)) {
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
	} else if (!lua_isnoneornil(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitCloak()");
	}

	if (lua_israwnumber(L, 3)) {
		unit->decloakDistance = lua_tofloat(L, 3);
	}
	else if (lua_isboolean(L, 3)) {
		const float defDist = unit->unitDef->decloakDistance;
		if (lua_toboolean(L, 3)) {
			unit->decloakDistance = math::fabsf(defDist);
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
	unit->stealth = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitSonarStealth(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	unit->sonarStealth = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedCtrl::SetUnitAlwaysVisible(lua_State* L)
{
	return (SetWorldObjectAlwaysVisible(L, ParseUnit(L, __FUNCTION__, 1), __FUNCTION__));
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


int LuaSyncedCtrl::SetUnitHarvestStorage(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	unit->harvestStorage = luaL_checkfloat(L, 2);
	return 0;
}

int LuaSyncedCtrl::SetUnitBuildSpeed(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const float buildScale = (1.0f / TEAM_SLOWUPDATE_RATE);
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


int LuaSyncedCtrl::SetUnitNanoPieces(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	NanoPieceCache* pieceCache = NULL;
	std::vector<int>* nanoPieces = NULL;

	{
		CBuilder* builder = dynamic_cast<CBuilder*>(unit);

		if (builder != NULL) {
			pieceCache = &builder->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}

		CFactory* factory = dynamic_cast<CFactory*>(unit);

		if (factory != NULL) {
			pieceCache = &factory->GetNanoPieceCache();
			nanoPieces = &pieceCache->GetNanoPieces();
		}
	}

	if (nanoPieces == NULL)
		return 0;

	nanoPieces->clear();
	pieceCache->StopPolling();
	luaL_checktype(L, 2, LUA_TTABLE);

	for (lua_pushnil(L); lua_next(L, 2) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -1)) {
			const int modelPieceNum = lua_toint(L, -1) - 1; //lua 1-indexed, c++ 0-indexed

			if (unit->localModel->HasPiece(modelPieceNum)) {
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
	return (SetSolidObjectBlocking(L, ParseUnit(L, __FUNCTION__, 1)));
}


int LuaSyncedCtrl::SetUnitCrashing(lua_State* L) {
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL) {
		return 0;
	}

	AAirMoveType* amt = dynamic_cast<AAirMoveType*>(unit->moveType);
	bool ret = false;

	if (amt != NULL) {
		const bool wantCrash = luaL_optboolean(L, 2, false);
		const AAirMoveType::AircraftState aircraftState = amt->aircraftState;

		// for simplicity, this can only set a flying aircraft to
		// start crashing, or a crashing aircraft to start flying
		if ( wantCrash && (aircraftState == AAirMoveType::AIRCRAFT_FLYING))
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
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

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

	if (shield == NULL) {
		return 0;
	}

	if (lua_isboolean(L, arg)) { shield->SetEnabled(lua_toboolean(L, arg)); arg++; }
	if (lua_isnumber(L, arg)) { shield->SetCurPower(lua_tofloat(L, arg)); }
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
	if (args >= 4 && !lua_isboolean(L, 3)) {
		const float3 pos(luaL_checkfloat(L, 2),
		                 luaL_checkfloat(L, 3),
		                 luaL_checkfloat(L, 4));
		const bool manualFire = lua_isboolean(L, 5) && lua_toboolean(L, 5);
		const bool userTarget = lua_isboolean(L, 6) && lua_toboolean(L, 6);
		lua_pushboolean(L,unit->AttackGround(pos, userTarget, manualFire));
		return 1;
	}
	else if (args >= 2) {
		CUnit* target = ParseRawUnit(L, __FUNCTION__, 2);
		const bool manualFire = lua_isboolean(L, 3) && lua_toboolean(L, 3);
		const bool userTarget = lua_isboolean(L, 4) && lua_toboolean(L, 4);
		lua_pushboolean(L,unit->AttackUnit(target, userTarget, manualFire));
		return 1;
	}
	else {
		return 0;
	}
}



int LuaSyncedCtrl::SetUnitMidAndAimPos(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL) {
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
		quadField->RemoveUnit(unit);
	}

	unit->SetMidAndAimPos(newMidPos, newAimPos, setRelative);

	if (updateQuads) {
		quadField->MovedUnit(unit);
	}

	lua_pushboolean(L, true);
	return 1;
}

int LuaSyncedCtrl::SetUnitRadiusAndHeight(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL) {
		lua_pushboolean(L, false);
		return 1;
	}

	const float newRadius = std::max(1.0f, luaL_optfloat(L, 2, unit->radius));
	const float newHeight = std::max(1.0f, luaL_optfloat(L, 3, unit->height));
	const bool updateQuads = (newRadius != unit->radius);

	if (updateQuads) {
		// safety, possibly just need MovedUnit
		quadField->RemoveUnit(unit);
	}

	unit->SetRadiusAndHeight(newRadius, newHeight);

	if (updateQuads) {
		quadField->MovedUnit(unit);
	}

	lua_pushboolean(L, true);
	return 1;
}

int LuaSyncedCtrl::SetUnitCollisionVolumeData(lua_State* L)
{
	return (SetSolidObjectCollisionVolumeData(L, ParseUnit(L, __FUNCTION__, 1)));
}

int LuaSyncedCtrl::SetUnitPieceCollisionVolumeData(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	const LocalModel* localModel = unit->localModel;
	const unsigned int pieceIndex = luaL_checkint(L, 2);

	if (pieceIndex >= localModel->pieces.size())
		luaL_argerror(L, 2, "invalid piece index");

	LocalModelPiece* lmp = localModel->pieces[pieceIndex];
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



int LuaSyncedCtrl::SetUnitSensorRadius(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const string key = luaL_checkstring(L, 2);
	const float radius = luaL_checkfloat(L, 3);

	const int radarDiv    = radarHandler->radarDiv;
	const int radarRadius = (int)(radius * radarHandler->invRadarDiv);

	if (key == "los") {
		const int losRange = (int)(radius * losHandler->invLosDiv);
		unit->ChangeLos(losRange, unit->realAirLosRadius);
		unit->realLosRadius = losRange;
		lua_pushnumber(L, unit->losRadius * losHandler->losDiv);
	} else if (key == "airLos") {
		const int airRange = (int)(radius * losHandler->invAirDiv);
		unit->ChangeLos(unit->realLosRadius, airRange);
		unit->realAirLosRadius = airRange;
		lua_pushnumber(L, unit->airLosRadius * losHandler->airDiv);
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

int LuaSyncedCtrl::SetUnitPosErrorParams(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	unit->posErrorVector = float3(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));
	unit->posErrorDelta = float3(luaL_checkfloat(L, 5), luaL_checkfloat(L, 6), luaL_checkfloat(L, 7));
	unit->nextPosErrorUpdate = luaL_optint(L, 8, unit->nextPosErrorUpdate);
	return 0;
}


int LuaSyncedCtrl::SetUnitMoveGoal(lua_State* L)
{
	CheckAllowGameChanges(L);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;
	if (unit->moveType == NULL)
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


int LuaSyncedCtrl::SetUnitPhysics(lua_State* L)
{
	return (SetSolidObjectPhysicalState(L, ParseUnit(L, __FUNCTION__, 1)));
}

int LuaSyncedCtrl::SetUnitPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
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
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	CMatrix44f matrix;
	matrix.RotateZ(ClampRad(luaL_checkfloat(L, 4))); // .z := roll
	matrix.RotateX(ClampRad(luaL_checkfloat(L, 2))); // .x := pitch
	matrix.RotateY(ClampRad(luaL_checkfloat(L, 3))); // .y := yaw

	assert(matrix.IsOrthoNormal() == 0);

	// do not need ForcedSpin, below three calls cover it
	unit->SetDirVectors(matrix);
	unit->UpdateMidAndAimPos();
	unit->SetHeadingFromDirection();
	return 0;
}


int LuaSyncedCtrl::SetUnitDirection(lua_State* L)
{
	return (SetSolidObjectDirection(L, ParseUnit(L, __FUNCTION__, 1)));
}

int LuaSyncedCtrl::SetUnitVelocity(lua_State* L)
{
	return (SetWorldObjectVelocity(L, ParseUnit(L, __FUNCTION__, 1)));
}


int LuaSyncedCtrl::AddUnitDamage(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float damage    = luaL_checkfloat(L, 2);
	const int paralyze    = luaL_optint(L, 3, 0);
	const int attackerID  = luaL_optint(L, 4, -1);
	const int weaponDefID = luaL_optint(L, 5, -1);
	const float3 impulse  = float3(Clamp(luaL_optfloat(L, 6, 0.0f), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE),
	                               Clamp(luaL_optfloat(L, 7, 0.0f), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE),
	                               Clamp(luaL_optfloat(L, 8, 0.0f), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE));

	CUnit* attacker = NULL;
	if (attackerID >= 0) {
		if (static_cast<size_t>(attackerID) >= unitHandler->MaxUnits()) {
			return 0;
		}
		attacker = unitHandler->units[attackerID];
	}

	if (weaponDefID >= int(weaponDefHandler->weaponDefs.size())) {
		return 0;
	}

	DamageArray damages;
	damages[unit->armorType] = damage;
	if (paralyze) {
		damages.paralyzeDamageTime = paralyze;
	}

	unit->DoDamage(damages, impulse, attacker, weaponDefID, -1);
	return 0;
}


int LuaSyncedCtrl::AddUnitImpulse(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const float3 impulse(Clamp(luaL_checkfloat(L, 2), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE),
	                     Clamp(luaL_checkfloat(L, 3), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE),
	                     Clamp(luaL_checkfloat(L, 4), -MAX_EXPLOSION_IMPULSE, MAX_EXPLOSION_IMPULSE));

	unit->ApplyImpulse(impulse);
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
		const float value = max(0.0f, lua_tofloat(L, 3));

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
				const float value = max(0.0f, lua_tofloat(L, -1));
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

	if (unit == NULL)
		return 0;

	groundDecals->ForceRemoveSolidObject(unit);
	return 0;
}


int LuaSyncedCtrl::AddGrass(lua_State* L)
{
	float3 pos(luaL_checkfloat(L, 1), 0.0f, luaL_checkfloat(L, 2));
	pos.ClampInBounds();

	treeDrawer->AddGrass(pos);
	return 0;
}


int LuaSyncedCtrl::RemoveGrass(lua_State* L)
{
	float3 pos(luaL_checkfloat(L, 1), 0.0f, luaL_checkfloat(L, 2));
	pos.ClampInBounds();

	treeDrawer->RemoveGrass(pos);
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

	int facing = GetFacingFromHeading(heading);
	int team = CtrlTeam(L);
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
	if (!CanControlFeatureAllyTeam(L, allyTeam)) {
		luaL_error(L, "CreateFeature() bad team permission %d", team);
	}

	if (inCreateFeature) {
		luaL_error(L, "CreateFeature() recursion is not permitted");
	}

	// use SetFeatureResurrect() to fill in the missing bits
	inCreateFeature = true;
	FeatureLoadParams params;
	params.featureDef = featureDef;
	params.unitDef    = NULL;
	params.pos        = pos;
	params.speed      = ZeroVector;
	params.featureID  = luaL_optint(L, 7, -1);
	params.teamID     = team;
	params.allyTeamID = allyTeam;
	params.heading    = heading,
	params.facing     = facing,
	params.smokeTime  = 0; // smokeTime

	CFeature* feature = featureHandler->LoadFeature(params);
	inCreateFeature = false;

	if (feature != NULL) {
		lua_pushnumber(L, feature->id);
		return 1;
	}

	return 0;
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
	const int teamId = luaL_checkint(L, 2);
	if (!teamHandler->IsValidTeam(teamId)) {
		return 0;
	}
	feature->ChangeTeam(teamId);
	return 0;
}


int LuaSyncedCtrl::SetFeatureAlwaysVisible(lua_State* L)
{
	return (SetWorldObjectAlwaysVisible(L, ParseFeature(L, __FUNCTION__, 1), __FUNCTION__));
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


int LuaSyncedCtrl::SetFeaturePhysics(lua_State* L)
{
	return (SetSolidObjectPhysicalState(L, ParseFeature(L, __FUNCTION__, 1)));
}

int LuaSyncedCtrl::SetFeaturePosition(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}

	const float3 pos(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4));
	const bool snap(luaL_optboolean(L, 5, true));

	feature->ForcedMove(pos);
	feature->UpdateFinalHeight(snap);

	return 0;
}


int LuaSyncedCtrl::SetFeatureDirection(lua_State* L)
{
	return (SetSolidObjectDirection(L, ParseFeature(L, __FUNCTION__, 1)));
}

int LuaSyncedCtrl::SetFeatureVelocity(lua_State* L)
{
	return (SetWorldObjectVelocity(L, ParseFeature(L, __FUNCTION__, 1)));
}


int LuaSyncedCtrl::SetFeatureResurrect(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}

	const UnitDef* ud = unitDefHandler->GetUnitDefByName(luaL_checkstring(L, 2));

	if (ud != NULL) {
		feature->udef = ud;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args >= 3) {
		feature->buildFacing = LuaUtils::ParseFacing(L, __FUNCTION__, 3);
	}
	return 0;
}


int LuaSyncedCtrl::SetFeatureBlocking(lua_State* L)
{
	return (SetSolidObjectBlocking(L, ParseFeature(L, __FUNCTION__, 1)));
}

int LuaSyncedCtrl::SetFeatureNoSelect(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	feature->noSelect = !!luaL_checkboolean(L, 2);
	return 0;
}



int LuaSyncedCtrl::SetFeatureMidAndAimPos(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == NULL) {
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
		quadField->RemoveFeature(feature);
	}

	feature->SetMidAndAimPos(newMidPos, newAimPos, setRelative);

	if (updateQuads) {
		quadField->AddFeature(feature);
	}

	lua_pushboolean(L, true);
	return 1;
}

int LuaSyncedCtrl::SetFeatureRadiusAndHeight(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);

	if (feature == NULL) {
		lua_pushboolean(L, false);
		return 1;
	}

	const float newRadius = std::max(1.0f, luaL_optfloat(L, 2, feature->radius));
	const float newHeight = std::max(1.0f, luaL_optfloat(L, 3, feature->height));
	const bool updateQuads = (newRadius != feature->radius);

	if (updateQuads) {
		quadField->RemoveFeature(feature);
	}

	feature->SetRadiusAndHeight(newRadius, newHeight);

	if (updateQuads) {
		quadField->AddFeature(feature);
	}

	lua_pushboolean(L, true);
	return 1;
}

int LuaSyncedCtrl::SetFeatureCollisionVolumeData(lua_State* L)
{
	return (SetSolidObjectCollisionVolumeData(L, ParseFeature(L, __FUNCTION__, 1)));
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetProjectileAlwaysVisible(lua_State* L)
{
	return (SetWorldObjectAlwaysVisible(L, ParseProjectile(L, __FUNCTION__, 1), __FUNCTION__));
}

int LuaSyncedCtrl::SetProjectileMoveControl(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);

	if (proj == NULL)
		return 0;
	if (!proj->weapon && !proj->piece)
		return 0;

	proj->luaMoveCtrl = luaL_optboolean(L, 2, false);
	return 0;
}

int LuaSyncedCtrl::SetProjectilePosition(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);

	if (proj == NULL)
		return 0;

	proj->pos.x = luaL_optfloat(L, 2, 0.0f);
	proj->pos.y = luaL_optfloat(L, 3, 0.0f);
	proj->pos.z = luaL_optfloat(L, 4, 0.0f);

	return 0;
}

int LuaSyncedCtrl::SetProjectileVelocity(lua_State* L)
{
	return (SetWorldObjectVelocity(L, ParseProjectile(L, __FUNCTION__, 1)));
}

int LuaSyncedCtrl::SetProjectileCollision(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);

	if (proj == NULL)
		return 0;

	proj->Collision();
	return 0;
}

int LuaSyncedCtrl::SetProjectileTarget(lua_State* L)
{
	CProjectile* pro = ParseProjectile(L, __FUNCTION__, 1);
	CWeaponProjectile* wpro = NULL;

	if (pro == NULL)
		return 0;
	if (!pro->weapon)
		return 0;

	struct ProjectileTarget {
		static DependenceType GetObjectDepType(const CWorldObject* o) {
			if (dynamic_cast<const      CSolidObject*>(o) != NULL) return DEPENDENCE_WEAPONTARGET;
			if (dynamic_cast<const CWeaponProjectile*>(o) != NULL) return DEPENDENCE_INTERCEPTTARGET;
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
				case 'u': { newTargetObject = ParseUnit(L, __FUNCTION__, 2); } break;
				case 'f': { newTargetObject = ParseFeature(L, __FUNCTION__, 2); } break;
				case 'p': { newTargetObject = ParseProjectile(L, __FUNCTION__, 2); } break;
				case 'g': { /* fall-through, needs four arguments (todo: or a table?) */ }
				default: { /* if invalid type-argument, current target will be cleared */ } break;
			}

			const DependenceType oldDepType = ProjectileTarget::GetObjectDepType(oldTargetObject);
			const DependenceType newDepType = ProjectileTarget::GetObjectDepType(newTargetObject);

			if (oldTargetObject != NULL) {
				wpro->DeleteDeathDependence(oldTargetObject, oldDepType);
				wpro->SetTargetObject(NULL);
			}
			if (newTargetObject != NULL) {
				wpro->AddDeathDependence(newTargetObject, newDepType);
				wpro->SetTargetObject(newTargetObject);
			}

			assert(newTargetObject == NULL || newTargetObject->id == id);
			lua_pushboolean(L, oldTargetObject != NULL || newTargetObject != NULL);
			return 1;
		} break;

		case 4: {
			if (wpro->GetTargetObject() != NULL) {
				wpro->DeleteDeathDependence(wpro->GetTargetObject(), ProjectileTarget::GetObjectDepType(wpro->GetTargetObject()));
			}

			wpro->SetTargetObject(NULL);
			wpro->SetTargetPos(float3(luaL_checkfloat(L, 2), luaL_checkfloat(L, 3), luaL_checkfloat(L, 4)));

			lua_pushboolean(L, wpro->GetTargetObject() == NULL);
			return 1;
		} break;
	}

	return 0;
}


int LuaSyncedCtrl::SetProjectileGravity(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);

	if (proj == NULL)
		return 0;

	proj->mygravity = luaL_optfloat(L, 2, 0.0f);
	return 0;
}

int LuaSyncedCtrl::SetProjectileSpinAngle(lua_State* L) { return 0; } // DEPRECATED
int LuaSyncedCtrl::SetProjectileSpinSpeed(lua_State* L) { return 0; } // DEPRECATED
int LuaSyncedCtrl::SetProjectileSpinVec(lua_State* L) { return 0; } // DEPRECATED

int LuaSyncedCtrl::SetPieceProjectileParams(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);

	if (proj == NULL || !proj->piece)
		return 0;

	CPieceProjectile* pproj = static_cast<CPieceProjectile*>(proj);

	pproj->explFlags = luaL_optint(L, 2, pproj->explFlags);
	pproj->spinAngle = luaL_optfloat(L, 3, pproj->spinAngle);
	pproj->spinSpeed = luaL_optfloat(L, 4, pproj->spinSpeed);
	pproj->spinVec.x = luaL_optfloat(L, 5, pproj->spinVec.x);
	pproj->spinVec.y = luaL_optfloat(L, 6, pproj->spinVec.y);
	pproj->spinVec.z = luaL_optfloat(L, 7, pproj->spinVec.z);
	return 0;
}

//
// TODO: move this and SpawnCEG to LuaUnsyncedCtrl
//
int LuaSyncedCtrl::SetProjectileCEG(lua_State* L)
{
	CProjectile* proj = ParseProjectile(L, __FUNCTION__, 1);

	if (proj == NULL)
		return 0;
	if (!proj->weapon && !proj->piece)
		return 0;

	unsigned int cegID = CExplosionGeneratorHandler::EXPGEN_ID_INVALID;

	if (lua_isstring(L, 2)) {
		cegID = explGenHandler->LoadGeneratorID(std::string(CEG_PREFIX_STRING) + lua_tostring(L, 2));
	} else {
		cegID = luaL_checknumber(L, 2);
	}

	// if cegID is EXPGEN_ID_INVALID, this also returns NULL
	if (explGenHandler->GetGenerator(cegID) != NULL) {
		proj->SetCustomExplosionGeneratorID(cegID);
	}

	lua_pushnumber(L, cegID);
	return 1;
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

	Command cmd = LuaUtils::ParseCommand(L, __FUNCTION__, 2);

	if (!CanControlUnit(L, unit)) {
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

	Command cmd = LuaUtils::ParseCommand(L, __FUNCTION__, 2);

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
	ParseUnitArray(L, __FUNCTION__, 1, units);
	const int unitCount = (int)units.size();

	if (unitCount <= 0) {
		lua_pushnumber(L, 0);
		return 1;
	}

	Command cmd = LuaUtils::ParseCommand(L, __FUNCTION__, 2);

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
	ParseUnitArray(L, __FUNCTION__, 1, units);
	const int unitCount = (int)units.size();

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __FUNCTION__, 2, commands);
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
	ParseParams(L, caller, factor, x1, z1, x2, z2, SQUARE_SIZE, gs->mapx, gs->mapy);
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
			readMap->SetHeight((z * gs->mapxp1) + x, height);
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
			readMap->AddHeight((z * gs->mapxp1) + x, height);
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

	const float* origMap = readMap->GetOriginalHeightMapSynced();
	const float* currMap = readMap->GetCornerHeightMapSynced();

	if (origFactor == 1.0f) {
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int idx = (z * gs->mapxp1) + x;

				readMap->SetHeight(idx, origMap[idx]);
			}
		}
	}
	else {
		const float currFactor = (1.0f - origFactor);
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int index = (z * gs->mapxp1) + x;
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
	if ((x < 0) || (x > gs->mapx) ||
	    (z < 0) || (z > gs->mapy)) {
		return 0;
	}

	const int index = (z * gs->mapxp1) + x;
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
	if ((x < 0) || (x > gs->mapx) ||
	    (z < 0) || (z > gs->mapy)) {
		return 0;
	}

	const int index = (z * gs->mapxp1) + x;
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
			smoothGround->GetResolution(), smoothGround->GetMaxX(),
			smoothGround->GetMaxY());
}


int LuaSyncedCtrl::LevelSmoothMesh(lua_State *L)
{
	float height;
	int x1, x2, z1, z2;
	ParseSmoothMeshParams(L, __FUNCTION__, height, x1, z1, x2, z2);

	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			const int index = (z * smoothGround->GetMaxX()) + x;
			smoothGround->SetHeight(index, height);
		}
	}
	return 0;
}

int LuaSyncedCtrl::AdjustSmoothMesh(lua_State *L)
{
	float height;
	int x1, x2, z1, z2;
	ParseSmoothMeshParams(L, __FUNCTION__, height, x1, z1, x2, z2);

	for (int z = z1; z <= z2; z++) {
		for (int x = x1; x <= x2; x++) {
			const int index = (z * smoothGround->GetMaxX()) + x;
			smoothGround->AddHeight(index, height);
		}
	}

	return 0;
}

int LuaSyncedCtrl::RevertSmoothMesh(lua_State *L)
{
	float origFactor;
	int x1, x2, z1, z2;
	ParseSmoothMeshParams(L, __FUNCTION__, origFactor, x1, z1, x2, z2);

	const float* origMap = smoothGround->GetOriginalMeshData();
	const float* currMap = smoothGround->GetMeshData();

	if (origFactor == 1.0f) {
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int idx = (z * smoothGround->GetMaxX()) + x;
				smoothGround->SetHeight(idx, origMap[idx]);
			}
		}
	}
	else {
		const float currFactor = (1.0f - origFactor);
		for (int z = z1; z <= z2; z++) {
			for (int x = x1; x <= x2; x++) {
				const int index = (z * smoothGround->GetMaxX()) + x;
				const float ofh = origFactor * origMap[index];
				const float cfh = currFactor * currMap[index];
				smoothGround->SetHeight(index, ofh + cfh);
			}
		}
	}

	return 0;
}


int LuaSyncedCtrl::AddSmoothMesh(lua_State *L)
{
	if (!inSmoothMesh) {
		luaL_error(L, "AddSmoothMesh() can only be called in SetSmoothMeshFunc()");
	}

	const float xl = luaL_checkfloat(L, 1);
	const float zl = luaL_checkfloat(L, 2);
	const float h  = luaL_checkfloat(L, 3);

	// quantize
	const int x = (int)(xl / smoothGround->GetResolution());
	const int z = (int)(zl / smoothGround->GetResolution());

	// discard invalid coordinates
	if ((x < 0) || (x > smoothGround->GetMaxX()) ||
	    (z < 0) || (z > smoothGround->GetMaxY())) {
		return 0;
	}

	const int index = (z * smoothGround->GetMaxX()) + x;
	const float oldHeight = smoothGround->GetMeshData()[index];
	smoothMeshAmountChanged += math::fabsf(h);

	smoothGround->AddHeight(index, h);
	// push the new height
	lua_pushnumber(L, oldHeight + h);
	return 1;
}

int LuaSyncedCtrl::SetSmoothMesh(lua_State *L)
{
	if (!inSmoothMesh) {
		luaL_error(L, "SetSmoothMesh() can only be called in SetSmoothMeshFunc()");
	}

	const float xl = luaL_checkfloat(L, 1);
	const float zl = luaL_checkfloat(L, 2);
	const float h  = luaL_checkfloat(L, 3);

	// quantize
	const int x = (int)(xl / smoothGround->GetResolution());
	const int z = (int)(zl / smoothGround->GetResolution());

	// discard invalid coordinates
	if ((x < 0) || (x > smoothGround->GetMaxX()) ||
	    (z < 0) || (z > smoothGround->GetMaxY())) {
		return 0;
	}

	const int index = (z * (smoothGround->GetMaxX())) + x;
	const float oldHeight = smoothGround->GetMeshData()[index];
	float height = oldHeight;

	if (lua_israwnumber(L, 4)) {
		const float t = lua_tofloat(L, 4);
		height += (h - oldHeight) * t;
	} else{
		height = h;
	}

	const float heightDiff = (height - oldHeight);
	smoothMeshAmountChanged += math::fabsf(heightDiff);

	smoothGround->SetHeight(index, height);
	lua_pushnumber(L, heightDiff);
	return 1;
}

int LuaSyncedCtrl::SetSmoothMeshFunc(lua_State *L)
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

	if ((hx < 0) || (hx > gs->mapx) || (hz < 0) || (hz > gs->mapy)) {
		return 0;
	}

	const int tx = hx >> 1;
	const int tz = hz >> 1;

	const int ott = readMap->GetTypeMapSynced()[tz * gs->hmapx + tx];
	const int ntt = luaL_checkint(L, 3);

	readMap->GetTypeMapSynced()[tz * gs->hmapx + tx] = std::max(0, std::min(ntt, (CMapInfo::NUM_TERRAIN_TYPES - 1)));
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

	const int checksumOld = HsiehHash(tt, sizeof(CMapInfo::TerrainType), 0);

	if (args >= 2 && lua_isnumber(L, 2)) { tt->tankSpeed  = lua_tofloat(L, 2); }
	if (args >= 3 && lua_isnumber(L, 3)) { tt->kbotSpeed  = lua_tofloat(L, 3); }
	if (args >= 4 && lua_isnumber(L, 4)) { tt->hoverSpeed = lua_tofloat(L, 4); }
	if (args >= 5 && lua_isnumber(L, 5)) { tt->shipSpeed  = lua_tofloat(L, 5); }

	const int checksumNew = HsiehHash(tt, sizeof(CMapInfo::TerrainType), 0);
	if (checksumOld == checksumNew) {
		// no change, no need to repath
		lua_pushboolean(L, true);
		return 1;
	}

	/*
	if (!mapDamage->disabled) {
		CBasicMapDamage* bmd = dynamic_cast<CBasicMapDamage*>(mapDamage);

		if (bmd != NULL) {
			tt->hardness = luaL_checkfloat(L, 6);
			bmd->invHardness[tti] = 1.0f / tt->hardness;
		}
	}
	*/

	const unsigned char* typeMap = readMap->GetTypeMapSynced();

	// update all map-squares set to this terrain-type (slow)
	for (int tx = 0; tx < gs->hmapx; tx++) {
		for (int tz = 0; tz < gs->hmapy; tz++) {
			if (typeMap[tz * gs->hmapx + tx] == tti) {
				pathManager->TerrainChange((tx << 1), (tz << 1),  (tx << 1) + 1, (tz << 1) + 1,  TERRAINCHANGE_TYPEMAP_SPEED_VALUES);
			}
		}
	}

	lua_pushboolean(L, true);
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::UnitWeaponFire(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;
	if (static_cast<uint32_t>(luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX) >= unit->weapons.size())
		return 0;

	unit->weapons[luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX]->Fire(false);
	return 0;
}
int LuaSyncedCtrl::UnitWeaponHoldFire(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;
	if (static_cast<uint32_t>(luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX) >= unit->weapons.size())
		return 0;

	unit->weapons[luaL_checkint(L, 2) - LUA_WEAPON_BASE_INDEX]->HoldFire();
	return 0;
}

int LuaSyncedCtrl::SpawnProjectile(lua_State* L)
{
	ProjectileParams params;

	if ((params.weaponDef = weaponDefHandler->GetWeaponDefByID(luaL_checkint(L, 1))) == NULL) {
		return 0;
	}

	ParseProjectileParams(L, params, 2, __FUNCTION__);

	lua_pushnumber(L, WeaponProjectileFactory::LoadProjectile(params));
	return 1;
}

int LuaSyncedCtrl::SpawnCEG(lua_State* L)
{
	const float3 pos(luaL_optfloat(L, 2, 0.0f), luaL_optfloat(L, 3, 0.0f), luaL_optfloat(L, 4, 0.0f));
	const float3 dir(luaL_optfloat(L, 5, 0.0f), luaL_optfloat(L, 6, 0.0f), luaL_optfloat(L, 7, 0.0f));

	const float rad = luaL_optfloat(L, 8, 0.0f);
	const float dmg = luaL_optfloat(L, 9, 0.0f);
	const float dmgMod = luaL_optfloat(L, 10, 1.0f);

	unsigned int cegID = CExplosionGeneratorHandler::EXPGEN_ID_INVALID;

	if (lua_isstring(L, 1)) {
		// args from Lua are assumed not to include the prefix
		// (Spawn*C*EG implies only custom generators can fire)
		cegID = explGenHandler->LoadGeneratorID(std::string(CEG_PREFIX_STRING) + lua_tostring(L, 1));
	} else {
		cegID = luaL_checknumber(L, 1);
	}

	lua_pushboolean(L, explGenHandler->GenExplosion(cegID, pos, dir, dmg, rad, dmgMod, NULL, NULL));
	lua_pushnumber(L, cegID);
	return 2;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedCtrl::SetNoPause(lua_State* L)
{
	if (!FullCtrl(L)) {
		return 0;
	}
	// Important: only works in server mode, has no effect in client mode
	if (gameServer)
		gameServer->SetGamePausable(!luaL_checkboolean(L, 1));

	return 0;
}


int LuaSyncedCtrl::SetUnitToFeature(lua_State* L)
{
	if (!FullCtrl(L)) {
		return 0;
	}
	CUnit::SetSpawnFeature(luaL_checkboolean(L, 1));
	return 0;
}


int LuaSyncedCtrl::SetExperienceGrade(lua_State* L)
{
	if (!FullCtrl(L)) {
		return 0;
	}

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

	if (!teamHandler->IsValidAllyTeam(allyTeamID))
		return 0;

	radarHandler->SetAllyTeamRadarErrorSize(allyTeamID, luaL_checknumber(L, 2));
	radarHandler->SetBaseRadarErrorSize(luaL_optnumber(L, 3, radarHandler->GetBaseRadarErrorSize()));
	radarHandler->SetBaseRadarErrorMult(luaL_optnumber(L, 4, radarHandler->GetBaseRadarErrorMult()));
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
	if (!FullCtrl(L)) {
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

	selectedUnitsHandler.PossibleCommandChange(unit);

	return 0;
}


int LuaSyncedCtrl::InsertUnitCmdDesc(lua_State* L)
{
	if (!FullCtrl(L)) {
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

	selectedUnitsHandler.PossibleCommandChange(unit);

	return 0;
}


int LuaSyncedCtrl::RemoveUnitCmdDesc(lua_State* L)
{
	if (!FullCtrl(L)) {
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

	selectedUnitsHandler.PossibleCommandChange(unit);

	return 0;
}


/******************************************************************************/
/******************************************************************************/
