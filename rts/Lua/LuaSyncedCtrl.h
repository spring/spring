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
		static int KillTeam(lua_State* L);
		static int GameOver(lua_State* L);

		static int AddTeamResource(lua_State* L);
		static int UseTeamResource(lua_State* L);
		static int SetTeamResource(lua_State* L);
		static int SetTeamShareLevel(lua_State* L);
		static int ShareTeamResource(lua_State* L);

		static int CallCOBScript(lua_State* L);
		static int GetCOBScriptID(lua_State* L);

		static int SetUnitRulesParam(lua_State* L);
		static int SetTeamRulesParam(lua_State* L);
		static int SetGameRulesParam(lua_State* L);

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

		static int SetUnitCosts(lua_State* L);
		static int SetUnitResourcing(lua_State* L);
		static int SetUnitTooltip(lua_State* L);
		static int SetUnitHealth(lua_State* L);
		static int SetUnitMaxHealth(lua_State* L);
		static int SetUnitStockpile(lua_State* L);
		static int SetUnitWeaponState(lua_State* L);
		static int SetUnitExperience(lua_State* L);
		static int SetUnitArmored(lua_State* L);
		static int SetUnitLosMask(lua_State* L);
		static int SetUnitLosState(lua_State* L);
		static int SetUnitCloak(lua_State* L);
		static int SetUnitStealth(lua_State* L);
		static int SetUnitSonarStealth(lua_State* L);
		static int SetUnitAlwaysVisible(lua_State* L);
		static int SetUnitMetalExtraction(lua_State* L);
		static int SetUnitBuildSpeed(lua_State* L);
		static int SetUnitBlocking(lua_State* L);
		static int SetUnitShieldState(lua_State* L);
		static int SetUnitFlanking(lua_State* L);
		static int SetUnitTravel(lua_State* L);
		static int SetUnitFuel(lua_State* L);
		static int SetUnitMoveGoal(lua_State* L);
		static int SetUnitNeutral(lua_State* L);
		static int SetUnitTarget(lua_State* L);
		static int SetUnitCollisionVolumeData(lua_State* L);
		static int SetUnitPieceCollisionVolumeData(lua_State* L);
		static int SetUnitSensorRadius(lua_State* L);

		static int SetUnitPhysics(lua_State* L);
		static int SetUnitPosition(lua_State* L);
		static int SetUnitRotation(lua_State* L);
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
		static int SetFeatureReclaim(lua_State* L);
		static int SetFeatureResurrect(lua_State* L);
		static int SetFeaturePosition(lua_State* L);
		static int SetFeatureDirection(lua_State* L);
		static int SetFeatureNoSelect(lua_State* L);
		static int SetFeatureCollisionVolumeData(lua_State* L);

		static int SetProjectileMoveControl(lua_State* L);
		static int SetProjectilePosition(lua_State* L);
		static int SetProjectileVelocity(lua_State* L);
		static int SetProjectileCollision(lua_State* L);
		static int SetProjectileGravity(lua_State* L);
		static int SetProjectileSpinAngle(lua_State* L);
		static int SetProjectileSpinSpeed(lua_State* L);
		static int SetProjectileSpinVec(lua_State* L);
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

		static int SpawnCEG(lua_State* L);

		// LuaRules  (fullCtrl)
		static int EditUnitCmdDesc(lua_State* L);
		static int InsertUnitCmdDesc(lua_State* L);
		static int RemoveUnitCmdDesc(lua_State* L);

		static int SetNoPause(lua_State* L);
		static int SetUnitToFeature(lua_State* L);
		static int SetExperienceGrade(lua_State* L);
};


#endif /* LUA_SYNCED_CTRL_H */
