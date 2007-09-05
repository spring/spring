#ifndef LUA_SYNCED_INFO_H
#define LUA_SYNCED_INFO_H
// LuaSyncedRead.h: interface for the LuaSyncedRead class.
//
//////////////////////////////////////////////////////////////////////

struct lua_State;


class LuaSyncedRead {
	public:
		static bool PushEntries(lua_State* L);

		static void AllowGameChanges(bool value);

	private:
		static int IsCheatingEnabled(lua_State* L);
		static int IsDevLuaEnabled(lua_State* L);
		static int IsEditDefsEnabled(lua_State* L);
		static int AreHelperAIsEnabled(lua_State* L);

		static int IsGameOver(lua_State* L);

		static int GetGaiaTeamID(lua_State* L);
		static int GetRulesInfoMap(lua_State* L);

		static int GetGameSpeed(lua_State* L);
		static int GetGameFrame(lua_State* L);
		static int GetGameSeconds(lua_State* L);

		static int GetGameRulesParam(lua_State* L);
		static int GetGameRulesParams(lua_State* L);

		static int GetWind(lua_State* L);

		static int GetHeadingFromVector(lua_State* L);
		static int GetVectorFromHeading(lua_State* L);

		static int GetAllyTeamStartBox(lua_State* L);
		static int GetTeamStartPosition(lua_State* L);

		static int GetAllyTeamList(lua_State* L);
		static int GetTeamList(lua_State* L);
		static int GetPlayerList(lua_State* L);

		static int GetPlayerInfo(lua_State* L); // no name for synced scripts
		static int GetPlayerControlledUnit(lua_State* L);

		static int GetTeamInfo(lua_State* L);
		static int GetTeamResources(lua_State* L);
		static int GetTeamUnitStats(lua_State* L);
		static int GetTeamRulesParam(lua_State* L);
		static int GetTeamRulesParams(lua_State* L);
		static int GetTeamStatsHistory(lua_State* L);

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

		static int GetUnitTooltip(lua_State* L);
		static int GetUnitDefID(lua_State* L);
		static int GetUnitTeam(lua_State* L);
		static int GetUnitAllyTeam(lua_State* L);
		static int GetUnitLineage(lua_State* L);
		static int GetUnitHealth(lua_State* L);
		static int GetUnitIsStunned(lua_State* L);
		static int GetUnitResources(lua_State* L);
		static int GetUnitExperience(lua_State* L);
		static int GetUnitStates(lua_State* L);
		static int GetUnitIsActive(lua_State* L);
		static int GetUnitIsCloaked(lua_State* L);
		static int GetUnitSelfDTime(lua_State* L);
		static int GetUnitStockpile(lua_State* L);
		static int GetUnitHeight(lua_State* L);
		static int GetUnitRadius(lua_State* L);
		static int GetUnitPosition(lua_State* L);
		static int GetUnitBasePosition(lua_State* L);
		static int GetUnitDirection(lua_State* L);
		static int GetUnitHeading(lua_State* L);
		static int GetUnitVelocity(lua_State* L);
		static int GetUnitBuildFacing(lua_State* L);
		static int GetUnitIsBuilding(lua_State* L);
		static int GetUnitTransporter(lua_State* L);
		static int GetUnitIsTransporting(lua_State* L);
		static int GetUnitShieldState(lua_State* L);
		static int GetUnitWeaponState(lua_State* L);
		static int GetUnitTravel(lua_State* L);

		static int GetUnitCommands(lua_State* L);
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
		static int GetUnitDefDimensions(lua_State* L);

		static int GetAllFeatures(lua_State* L);
		static int GetFeatureList(lua_State* L);

		static int GetFeatureDefID(lua_State* L);
		static int GetFeatureTeam(lua_State* L);
		static int GetFeatureAllyTeam(lua_State* L);
		static int GetFeatureHealth(lua_State* L);
		static int GetFeatureHeight(lua_State* L);
		static int GetFeatureRadius(lua_State* L);
		static int GetFeaturePosition(lua_State* L);
		static int GetFeatureDirection(lua_State* L);
		static int GetFeatureHeading(lua_State* L);
		static int GetFeatureResources(lua_State* L);
		static int GetFeatureNoSelect(lua_State* L);
		static int GetFeatureResurrect(lua_State* L);

		static int GetGroundHeight(lua_State* L);
		static int GetGroundNormal(lua_State* L);
		static int GetGroundInfo(lua_State* L);
		static int GetGroundBlocked(lua_State* L);
		static int GetGroundExtremes(lua_State* L);

		static int TestBuildOrder(lua_State* L);
		static int Pos2BuildPos(lua_State* L);

		static int GetPositionLosState(lua_State* L);
		static int GetClosestValidPosition(lua_State* L);

		static int GetUnitPiecePosition(lua_State* L);
		static int GetUnitPieceDirection(lua_State* L);
		static int GetUnitPieceMatrix(lua_State* L);

		static int TraceRay(lua_State* L); // not implemented
		static int TraceRayUnits(lua_State* L); // not implemented
		static int TraceRayFeatures(lua_State* L); // not implemented
		static int TraceRayGround(lua_State* L); // not implemented
};


#endif /* LUA_SYNCED_INFO_H */
