/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SYNCED_CTRL_H
#define LUA_SYNCED_CTRL_H

struct lua_State;

class LuaSyncedCtrl
{
	public:
		static bool PushEntries(lua_State* L);
		static void CheckAllowGameChanges(lua_State* L);

	private:
		static bool inCreateUnit;
		static bool inDestroyUnit;
		static bool inTransferUnit;
		static bool inCreateFeature;
		static bool inDestroyFeature;
		static bool inGiveOrder;
		static bool inHeightMap;
		static bool inSmoothMesh;

	private:
		// all LuaHandleSynced
		static int SetAlly(lua_State* L);
		static int KillTeam(lua_State* L);
		static int AssignPlayerToTeam(lua_State* L);
		static int GameOver(lua_State* L);
		static int SetGlobalLos(lua_State* L);

		static int AddTeamResource(lua_State* L);
		static int UseTeamResource(lua_State* L);
		static int SetTeamResource(lua_State* L);
		static int SetTeamShareLevel(lua_State* L);
		static int ShareTeamResource(lua_State* L);

		static int CallCOBScript(lua_State* L);
		static int GetCOBScriptID(lua_State* L);

		static int SetGameRulesParam(lua_State* L);
		static int SetTeamRulesParam(lua_State* L);
		static int SetUnitRulesParam(lua_State* L);
		static int SetFeatureRulesParam(lua_State* L);

		static int GiveOrderToUnit(lua_State* L);
		static int GiveOrderToUnitMap(lua_State* L);
		static int GiveOrderToUnitArray(lua_State* L);
		static int GiveOrderArrayToUnitMap(lua_State* L);
		static int GiveOrderArrayToUnitArray(lua_State* L);

		static int CreateUnit(lua_State* L);
		static int DestroyUnit(lua_State* L);
		static int TransferUnit(lua_State* L);

		static int CreateFeature(lua_State* L);
		static int DestroyFeature(lua_State* L);
		static int TransferFeature(lua_State* L);

		static int CreateUnitIKChain(lua_State* L);
		static int SetUnitIKActive(lua_State* L);
		static int SetUnitIKGoal(lua_State* L);
		static int SetUnitIKPieceLimits(lua_State* L);
		static int SetUnitIKPieceSpeed(lua_State* L);

		static int SetUnitCosts(lua_State* L);
		static int SetUnitTooltip(lua_State* L);
		static int SetUnitHealth(lua_State* L);
		static int SetUnitMaxHealth(lua_State* L);
		static int SetUnitStockpile(lua_State* L);
		static int SetUnitWeaponState(lua_State* L);
		static int SetUnitWeaponDamages(lua_State* L);
		static int SetUnitMaxRange(lua_State* L);
		static int SetUnitExperience(lua_State* L);
		static int SetUnitArmored(lua_State* L);
		static int SetUnitLosMask(lua_State* L);
		static int SetUnitLosState(lua_State* L);
		static int SetUnitCloak(lua_State* L);
		static int SetUnitStealth(lua_State* L);
		static int SetUnitSonarStealth(lua_State* L);
		static int SetUnitAlwaysVisible(lua_State* L);
		static int SetUnitResourcing(lua_State* L);
		static int SetUnitMetalExtraction(lua_State* L);
		static int SetUnitHarvestStorage(lua_State* L);
		static int SetUnitBuildSpeed(lua_State* L);
		static int SetUnitNanoPieces(lua_State* L);
		static int SetUnitBlocking(lua_State* L);
		static int SetUnitCrashing(lua_State* L);
		static int SetUnitShieldState(lua_State* L);
		static int SetUnitFlanking(lua_State* L);
		static int SetUnitTravel(lua_State* L);
		static int SetUnitFuel(lua_State* L);
		static int SetUnitMoveGoal(lua_State* L);
		static int SetUnitLandGoal(lua_State* L);
		static int ClearUnitGoal(lua_State* L);
		static int SetUnitNeutral(lua_State* L);
		static int SetUnitTarget(lua_State* L);
		static int SetUnitMidAndAimPos(lua_State* L);
		static int SetUnitRadiusAndHeight(lua_State* L);
		static int SetUnitCollisionVolumeData(lua_State* L);
		static int SetUnitPieceCollisionVolumeData(lua_State* L);
		static int SetUnitPieceParent(lua_State* L);
		static int SetUnitPieceMatrix(lua_State* L);
		static int SetUnitSensorRadius(lua_State* L);
		static int SetUnitPosErrorParams(lua_State* L);

		static int SetUnitPhysics(lua_State* L);
		static int SetUnitMass(lua_State* L);
		static int SetUnitPosition(lua_State* L);
		static int SetUnitRotation(lua_State* L);
		static int SetUnitDirection(lua_State* L);
		static int SetUnitVelocity(lua_State* L);

		static int AddUnitDamage(lua_State* L);
		static int AddUnitImpulse(lua_State* L);
		static int AddUnitSeismicPing(lua_State* L);

		static int AddUnitResource(lua_State* L);
		static int UseUnitResource(lua_State* L);

		static int RemoveBuildingDecal(lua_State* L);
		static int AddGrass(lua_State* L);
		static int RemoveGrass(lua_State* L);

		static int SetFeatureAlwaysVisible(lua_State* L);
		static int SetFeatureHealth(lua_State* L);
		static int SetFeatureMaxHealth(lua_State* L);
		static int SetFeatureReclaim(lua_State* L);
		static int SetFeatureResources(lua_State* L);
		static int SetFeatureResurrect(lua_State* L);

		static int SetFeatureMoveCtrl(lua_State* L);
		static int SetFeaturePhysics(lua_State* L);
		static int SetFeatureMass(lua_State* L);
		static int SetFeaturePosition(lua_State* L);
		static int SetFeatureRotation(lua_State* L);
		static int SetFeatureDirection(lua_State* L);
		static int SetFeatureVelocity(lua_State* L);

		static int SetFeatureBlocking(lua_State* L);
		static int SetFeatureNoSelect(lua_State* L);
		static int SetFeatureMidAndAimPos(lua_State* L);
		static int SetFeatureRadiusAndHeight(lua_State* L);
		static int SetFeatureCollisionVolumeData(lua_State* L);
		static int SetFeaturePieceCollisionVolumeData(lua_State* L);

		static int SetProjectileAlwaysVisible(lua_State* L);
		static int SetProjectileMoveControl(lua_State* L);
		static int SetProjectilePosition(lua_State* L);
		static int SetProjectileVelocity(lua_State* L);
		static int SetProjectileCollision(lua_State* L);
		static int SetProjectileTarget(lua_State* L);
		static int SetProjectileIsIntercepted(lua_State* L);
		static int SetProjectileDamages(lua_State* L);
		static int SetProjectileIgnoreTrackingError(lua_State* L);

		static int SetProjectileGravity(lua_State* L);
		static int SetProjectileSpinAngle(lua_State* L); // DEPRECATED
		static int SetProjectileSpinSpeed(lua_State* L); // DEPRECATED
		static int SetProjectileSpinVec(lua_State* L); // DEPRECATED
		static int SetPieceProjectileParams(lua_State* L);
		static int SetProjectileCEG(lua_State* L);

		static int LevelHeightMap(lua_State* L);
		static int AdjustHeightMap(lua_State* L);
		static int RevertHeightMap(lua_State* L);

		static int AddHeightMap(lua_State* L);
		static int SetHeightMap(lua_State* L);
		static int SetHeightMapFunc(lua_State* L);

		static int LevelSmoothMesh(lua_State* L);
		static int AdjustSmoothMesh(lua_State* L);
		static int RevertSmoothMesh(lua_State* L);

		static int AddSmoothMesh(lua_State* L);
		static int SetSmoothMesh(lua_State* L);
		static int SetSmoothMeshFunc(lua_State* L);

		static int SetMapSquareTerrainType(lua_State* L);
		static int SetTerrainTypeData(lua_State* L);

		static int SetSquareBuildingMask(lua_State* L);

		static int UnitWeaponFire(lua_State* L);
		static int UnitWeaponHoldFire(lua_State* L);

		static int UnitAttach(lua_State* L);
		static int UnitDetach(lua_State* L);
		static int UnitDetachFromAir(lua_State* L);
		static int SetUnitLoadingTransport(lua_State* L);

		static int SpawnProjectile(lua_State* L);
		static int DeleteProjectile(lua_State* L);
		static int SpawnExplosion(lua_State* L);
		static int SpawnCEG(lua_State* L);
		static int SpawnSFX(lua_State* L);

		// LuaRules  (fullCtrl)
		static int EditUnitCmdDesc(lua_State* L);
		static int InsertUnitCmdDesc(lua_State* L);
		static int RemoveUnitCmdDesc(lua_State* L);

		static int SetNoPause(lua_State* L);
		static int SetUnitToFeature(lua_State* L);
		static int SetExperienceGrade(lua_State* L);

		static int SetRadarErrorParams(lua_State* L);
};


#endif /* LUA_SYNCED_CTRL_H */
