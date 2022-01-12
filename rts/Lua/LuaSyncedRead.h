/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SYNCED_READ_H
#define LUA_SYNCED_READ_H

struct lua_State;


class LuaSyncedRead {
	friend class CLuaIntro;

	public:
		static bool PushEntries(lua_State* L);

		static void AllowGameChanges(bool value);

	public: // also used with LuaParser clients
		static int GetMapOptions(lua_State* L);
		static int GetModOptions(lua_State* L);

	private:
		static int IsCheatingEnabled(lua_State* L);
		static int IsGodModeEnabled(lua_State* L);
		static int IsDevLuaEnabled(lua_State* L);
		static int IsEditDefsEnabled(lua_State* L);
		static int IsNoCostEnabled(lua_State* L);
		static int GetGlobalLos(lua_State* L);
		static int AreHelperAIsEnabled(lua_State* L);
		static int FixedAllies(lua_State* L);

		static int IsGameOver(lua_State* L);

		static int GetGaiaTeamID(lua_State* L);

		static int GetGameFrame(lua_State* L);
		static int GetGameSeconds(lua_State* L);

		static int GetGameRulesParam(lua_State* L);
		static int GetGameRulesParams(lua_State* L);

		static int GetTidal(lua_State* L);
		static int GetWind(lua_State* L);

		static int GetHeadingFromVector(lua_State* L);
		static int GetVectorFromHeading(lua_State* L);

		static int GetSideData(lua_State* L);

		static int GetAllyTeamStartBox(lua_State* L);
		static int GetTeamStartPosition(lua_State* L);
		static int GetMapStartPositions(lua_State* L);

		static int GetAllyTeamList(lua_State* L);
		static int GetTeamList(lua_State* L);
		static int GetPlayerList(lua_State* L);

		static int GetPlayerInfo(lua_State* L); // no name for synced scripts
		static int GetPlayerControlledUnit(lua_State* L);
		static int GetAIInfo(lua_State* L);

		static int GetTeamInfo(lua_State* L);
		static int GetTeamResources(lua_State* L);
		static int GetTeamUnitStats(lua_State* L);
		static int GetTeamResourceStats(lua_State* L);
		static int GetTeamRulesParam(lua_State* L);
		static int GetTeamRulesParams(lua_State* L);
		static int GetTeamStatsHistory(lua_State* L);
		static int GetTeamLuaAI(lua_State* L);

		static int GetAllyTeamInfo(lua_State* L);
		static int AreTeamsAllied(lua_State* L);
		static int ArePlayersAllied(lua_State* L);

		static int GetAllUnits(lua_State* L);
		static int GetTeamUnits(lua_State* L);
		static int GetTeamUnitsSorted(lua_State* L);
		static int GetTeamUnitsCounts(lua_State* L);
		static int GetTeamUnitsByDefs(lua_State* L);
		static int GetTeamUnitDefCount(lua_State* L);
		static int GetTeamUnitCount(lua_State* L);

		static int GetUnitsInRectangle(lua_State* L);
		static int GetUnitsInBox(lua_State* L);
		static int GetUnitsInPlanes(lua_State* L);
		static int GetUnitsInSphere(lua_State* L);
		static int GetUnitsInCylinder(lua_State* L);

		static int GetUnitNearestAlly(lua_State* L);
		static int GetUnitNearestEnemy(lua_State* L);

		static int GetFeaturesInRectangle(lua_State* L);
		static int GetFeaturesInSphere(lua_State* L);
		static int GetFeaturesInCylinder(lua_State* L);
		static int GetProjectilesInRectangle(lua_State* L);

		static int ValidUnitID(lua_State* L);
		static int GetUnitTooltip(lua_State* L);
		static int GetUnitDefID(lua_State* L);
		static int GetUnitTeam(lua_State* L);
		static int GetUnitAllyTeam(lua_State* L);
		static int GetUnitNeutral(lua_State* L);
		static int GetUnitHealth(lua_State* L);
		static int GetUnitIsDead(lua_State* L);
		static int GetUnitIsStunned(lua_State* L);
		static int GetUnitResources(lua_State* L);
		static int GetUnitMetalExtraction(lua_State* L);
		static int GetUnitExperience(lua_State* L);
		static int GetUnitStates(lua_State* L);
		static int GetUnitArmored(lua_State* L);
		static int GetUnitIsActive(lua_State* L);
		static int GetUnitIsCloaked(lua_State* L);
		static int GetUnitSelfDTime(lua_State* L);
		static int GetUnitStockpile(lua_State* L);
		static int GetUnitSensorRadius(lua_State* L);
		static int GetUnitPosErrorParams(lua_State* L);
		static int GetUnitHeight(lua_State* L);
		static int GetUnitRadius(lua_State* L);
		static int GetUnitMass(lua_State* L);
		static int GetUnitPosition(lua_State* L);
		static int GetUnitBasePosition(lua_State* L);
		static int GetUnitVectors(lua_State* L);
		static int GetUnitRotation(lua_State* L);
		static int GetUnitDirection(lua_State* L);
		static int GetUnitHeading(lua_State* L);
		static int GetUnitVelocity(lua_State* L);
		static int GetUnitBuildFacing(lua_State* L);
		static int GetUnitIsBuilding(lua_State* L);
		static int GetUnitCurrentBuildPower(lua_State* L);
		static int GetUnitHarvestStorage(lua_State* L);
		static int GetUnitNanoPieces(lua_State* L);
		static int GetUnitTransporter(lua_State* L);
		static int GetUnitIsTransporting(lua_State* L);
		static int GetUnitShieldState(lua_State* L);
		static int GetUnitFlanking(lua_State* L);
		static int GetUnitMaxRange(lua_State* L);
		static int GetUnitWeaponState(lua_State* L);
		static int GetUnitWeaponDamages(lua_State* L);
		static int GetUnitWeaponVectors(lua_State* L);
		static int GetUnitWeaponTryTarget(lua_State* L);
		static int GetUnitWeaponTestTarget(lua_State* L);
		static int GetUnitWeaponTestRange(lua_State* L);
		static int GetUnitWeaponHaveFreeLineOfFire(lua_State* L);
		static int GetUnitWeaponCanFire(lua_State* L);
		static int GetUnitWeaponTarget(lua_State* L);
		static int GetUnitTravel(lua_State* L);
		static int GetUnitFuel(lua_State* L);
		static int GetUnitEstimatedPath(lua_State* L);
		static int GetUnitLastAttacker(lua_State* L);
		static int GetUnitLastAttackedPiece(lua_State* L);
		static int GetUnitCollisionVolumeData(lua_State* L);
		static int GetUnitPieceCollisionVolumeData(lua_State* L);

		static int GetUnitBlocking(lua_State* L);
		static int GetUnitMoveTypeData(lua_State* L);

		static int GetUnitCommands(lua_State* L);
		static int GetUnitCurrentCommand(lua_State* L);
		static int GetFactoryCounts(lua_State* L);
		static int GetFactoryCommands(lua_State* L);

		static int GetCommandQueue(lua_State* L);
		static int GetFullBuildQueue(lua_State* L);
		static int GetRealBuildQueue(lua_State* L);

		static int GetUnitCmdDescs(lua_State* L);
		static int FindUnitCmdDesc(lua_State* L);

		static int GetUnitRulesParam(lua_State* L);
		static int GetUnitRulesParams(lua_State* L);

		static int GetUnitLosState(lua_State* L);
		static int GetUnitSeparation(lua_State* L);
		static int GetUnitFeatureSeparation(lua_State* L);
		static int GetUnitDefDimensions(lua_State* L);

		static int GetCEGID(lua_State* L);

		static int GetAllFeatures(lua_State* L);

		static int ValidFeatureID(lua_State* L);
		static int GetFeatureDefID(lua_State* L);
		static int GetFeatureTeam(lua_State* L);
		static int GetFeatureAllyTeam(lua_State* L);
		static int GetFeatureHealth(lua_State* L);
		static int GetFeatureHeight(lua_State* L);
		static int GetFeatureRadius(lua_State* L);
		static int GetFeatureMass(lua_State* L);
		static int GetFeaturePosition(lua_State* L);
		static int GetFeatureSeparation(lua_State* L);
		static int GetFeatureRotation(lua_State* L);
		static int GetFeatureDirection(lua_State* L);
		static int GetFeatureVelocity(lua_State* L);
		static int GetFeatureHeading(lua_State* L);
		static int GetFeatureResources(lua_State* L);
		static int GetFeatureBlocking(lua_State* L);
		static int GetFeatureNoSelect(lua_State* L);
		static int GetFeatureResurrect(lua_State* L);
		static int GetFeatureLastAttackedPiece(lua_State* L);
		static int GetFeatureCollisionVolumeData(lua_State* L);
		static int GetFeaturePieceCollisionVolumeData(lua_State* L);

		static int GetFeatureRulesParam(lua_State* L);
		static int GetFeatureRulesParams(lua_State* L);

		static int GetProjectilePosition(lua_State* L);
		static int GetProjectileDirection(lua_State* L);
		static int GetProjectileVelocity(lua_State* L);
		static int GetProjectileGravity(lua_State* L);
		static int GetProjectileSpinAngle(lua_State* L); // DEPRECATED
		static int GetProjectileSpinSpeed(lua_State* L); // DEPRECATED
		static int GetProjectileSpinVec(lua_State* L); // DEPRECATED
		static int GetPieceProjectileParams(lua_State* L);
		static int GetProjectileTarget(lua_State* L);
		static int GetProjectileIsIntercepted(lua_State* L);
		static int GetProjectileTimeToLive(lua_State* L);
		static int GetProjectileOwnerID(lua_State* L);
		static int GetProjectileTeamID(lua_State* L);
		static int GetProjectileType(lua_State* L);
		static int GetProjectileDefID(lua_State* L);
		static int GetProjectileDamages(lua_State* L);
		static int GetProjectileName(lua_State* L); // DEPRECATE ME?

		static int GetGroundHeight(lua_State* L);
		static int GetGroundOrigHeight(lua_State* L);
		static int GetGroundNormal(lua_State* L);
		static int GetGroundInfo(lua_State* L);
		static int GetGroundBlocked(lua_State* L);
		static int GetGroundExtremes(lua_State* L);
		static int GetTerrainTypeData(lua_State* L);
		static int GetGrass(lua_State* L);

		static int GetSmoothMeshHeight(lua_State* L);

		static int TestMoveOrder(lua_State* L);
		static int TestBuildOrder(lua_State* L);
		static int Pos2BuildPos(lua_State* L);
		static int ClosestBuildPos(lua_State* L);

		static int GetPositionLosState(lua_State* L);
		static int IsPosInLos(lua_State* L);
		static int IsPosInRadar(lua_State* L);
		static int IsPosInAirLos(lua_State* L);
		static int IsUnitInLos(lua_State* L);
		static int IsUnitInAirLos(lua_State* L);
		static int IsUnitInRadar(lua_State* L);
		static int IsUnitInJammer(lua_State* L);
		static int GetClosestValidPosition(lua_State* L);

		static int GetUnitPieceMap(lua_State* L);
		static int GetUnitPieceList(lua_State* L);
		static int GetUnitPieceInfo(lua_State* L);
		static int GetUnitPiecePosition(lua_State* L);
		static int GetUnitPieceDirection(lua_State* L);
		static int GetUnitPiecePosDir(lua_State* L);
		static int GetUnitPieceMatrix(lua_State* L);

		static int GetUnitScriptPiece(lua_State* L);
		static int GetUnitScriptNames(lua_State* L);

		static int GetFeaturePieceMap(lua_State* L);
		static int GetFeaturePieceList(lua_State* L);
		static int GetFeaturePieceInfo(lua_State* L);
		static int GetFeaturePiecePosition(lua_State* L);
		static int GetFeaturePieceDirection(lua_State* L);
		static int GetFeaturePiecePosDir(lua_State* L);
		static int GetFeaturePieceMatrix(lua_State* L);

		static int GetRadarErrorParams(lua_State* L);

		static int TraceRay(lua_State* L);           //TODO: not implemented
		static int TraceRayUnits(lua_State* L);      //TODO: not implemented
		static int TraceRayFeatures(lua_State* L);   //TODO: not implemented
		static int TraceRayGround(lua_State* L);     //TODO: not implemented
};

#endif /* LUA_SYNCED_READ_H */
