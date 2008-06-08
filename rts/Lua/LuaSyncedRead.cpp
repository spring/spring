#include "StdAfx.h"
// LuaSyncedRead.cpp: implementation of the CLuaSyncedRead class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaSyncedRead.h"
#include <set>
#include <list>
#include <cctype>

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
//FIXME #include "LuaMetalMap.h"
#include "LuaPathFinder.h"
#include "LuaRules.h"
#include "LuaUtils.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "System/myMath.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Platform/FileSystem.h"

using namespace std;


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

static const bool& fullRead     = CLuaHandle::GetActiveFullRead();
static const int&  readAllyTeam = CLuaHandle::GetActiveReadAllyTeam();


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
	REGISTER_LUA_CFUNC(GetRulesInfoMap);

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

	REGISTER_LUA_CFUNC(GetAllyTeamStartBox);
	REGISTER_LUA_CFUNC(GetTeamStartPosition);

	REGISTER_LUA_CFUNC(GetPlayerList);
	REGISTER_LUA_CFUNC(GetTeamList);
	REGISTER_LUA_CFUNC(GetAllyTeamList);

	REGISTER_LUA_CFUNC(GetPlayerInfo);
	REGISTER_LUA_CFUNC(GetPlayerControlledUnit);

	REGISTER_LUA_CFUNC(GetTeamInfo);
	REGISTER_LUA_CFUNC(GetTeamResources);
	REGISTER_LUA_CFUNC(GetTeamUnitStats);
	REGISTER_LUA_CFUNC(GetTeamRulesParam);
	REGISTER_LUA_CFUNC(GetTeamRulesParams);
	REGISTER_LUA_CFUNC(GetTeamStatsHistory);
	REGISTER_LUA_CFUNC(GetTeamLuaAI);

	REGISTER_LUA_CFUNC(AreTeamsAllied);
	REGISTER_LUA_CFUNC(ArePlayersAllied);

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

	REGISTER_LUA_CFUNC(GetUnitNearestAlly);
	REGISTER_LUA_CFUNC(GetUnitNearestEnemy);

	REGISTER_LUA_CFUNC(GetUnitTooltip);
	REGISTER_LUA_CFUNC(GetUnitDefID);
	REGISTER_LUA_CFUNC(GetUnitTeam);
	REGISTER_LUA_CFUNC(GetUnitAllyTeam);
	REGISTER_LUA_CFUNC(GetUnitLineage);
	REGISTER_LUA_CFUNC(GetUnitNeutral);
	REGISTER_LUA_CFUNC(GetUnitHealth);
	REGISTER_LUA_CFUNC(GetUnitIsDead);
	REGISTER_LUA_CFUNC(GetUnitIsStunned);
	REGISTER_LUA_CFUNC(GetUnitResources);
	REGISTER_LUA_CFUNC(GetUnitExperience);
	REGISTER_LUA_CFUNC(GetUnitStates);
	REGISTER_LUA_CFUNC(GetUnitIsActive);
	REGISTER_LUA_CFUNC(GetUnitIsCloaked);
	REGISTER_LUA_CFUNC(GetUnitSelfDTime);
	REGISTER_LUA_CFUNC(GetUnitStockpile);
	REGISTER_LUA_CFUNC(GetUnitHeight);
	REGISTER_LUA_CFUNC(GetUnitRadius);
	REGISTER_LUA_CFUNC(GetUnitPosition);
	REGISTER_LUA_CFUNC(GetUnitBasePosition);
	REGISTER_LUA_CFUNC(GetUnitDirection);
	REGISTER_LUA_CFUNC(GetUnitHeading);
	REGISTER_LUA_CFUNC(GetUnitVelocity);
	REGISTER_LUA_CFUNC(GetUnitBuildFacing);
	REGISTER_LUA_CFUNC(GetUnitIsBuilding);
	REGISTER_LUA_CFUNC(GetUnitTransporter);
	REGISTER_LUA_CFUNC(GetUnitIsTransporting);
	REGISTER_LUA_CFUNC(GetUnitShieldState);
	REGISTER_LUA_CFUNC(GetUnitWeaponState);
	REGISTER_LUA_CFUNC(GetUnitTravel);
	REGISTER_LUA_CFUNC(GetUnitEstimatedPath);
	REGISTER_LUA_CFUNC(GetUnitLosState);
	REGISTER_LUA_CFUNC(GetUnitSeparation);
	REGISTER_LUA_CFUNC(GetUnitDefDimensions);

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
	REGISTER_LUA_CFUNC(GetFeatureList);
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

	REGISTER_LUA_CFUNC(GetGroundHeight);
	REGISTER_LUA_CFUNC(GetGroundOrigHeight);
	REGISTER_LUA_CFUNC(GetGroundNormal);
	REGISTER_LUA_CFUNC(GetGroundInfo);
	REGISTER_LUA_CFUNC(GetGroundBlocked);
	REGISTER_LUA_CFUNC(GetGroundExtremes);

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

//FIXME	LuaMetalMap::PushEntries(L);
	LuaPathFinder::PushEntries(L);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

static inline bool IsAlliedTeam(int team)
{
	if (readAllyTeam < 0) {
		return fullRead;
	}
	return (gs->AllyTeam(team) == readAllyTeam);
}


static inline bool IsAlliedAllyTeam(int allyTeam)
{
	if (readAllyTeam < 0) {
		return fullRead;
	}
	return (allyTeam == readAllyTeam);
}


static inline bool IsAllyUnit(const CUnit* unit)
{
	if (readAllyTeam < 0) {
		return fullRead;
	}
	return (unit->allyteam == readAllyTeam);
}


static inline bool IsEnemyUnit(const CUnit* unit)
{
	if (readAllyTeam < 0) {
		return !fullRead;
	}
	return (unit->allyteam != readAllyTeam);
}


static inline bool IsUnitVisible(const CUnit* unit)
{
	if (readAllyTeam < 0) {
		return fullRead;
	}
	return !!(unit->losStatus[readAllyTeam] & (LOS_INLOS | LOS_INRADAR));
}


static inline bool IsUnitInLos(const CUnit* unit)
{
	if (readAllyTeam < 0) {
		return fullRead;
	}
	return (unit->losStatus[readAllyTeam] & LOS_INLOS);
}


static inline bool IsUnitTyped(const CUnit* unit)
{
	if (readAllyTeam < 0) {
		return fullRead;
	}
	const unsigned short losStatus = unit->losStatus[readAllyTeam];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
	if ((losStatus & LOS_INLOS) ||
	    ((losStatus & prevMask) == prevMask)) {
		return true;
	}
	return false;
}


static inline const UnitDef* EffectiveUnitDef(const CUnit* unit)
{
	const UnitDef* ud = unit->unitDef;
	if (IsAllyUnit(unit)) {
		return ud;
	} else if (ud->decoyDef) {
		return ud->decoyDef;
	} else {
		return ud;
	}
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
	const int unitID = (int)lua_tonumber(L, index);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
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
	return IsUnitVisible(unit) ? unit : NULL;
}


static inline CUnit* ParseAllyUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	return IsAllyUnit(unit) ? unit : NULL;
}


static inline CUnit* ParseInLosUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	return IsUnitInLos(unit) ? unit : NULL;
}


static inline CUnit* ParseTypedUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	return IsUnitTyped(unit) ? unit : NULL;
}


/******************************************************************************/

static inline CPlayer* ParsePlayer(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "Bad playerID type in %s()\n", caller);
	}
	const int playerID = (int)lua_tonumber(L, index);
	if ((playerID < 0) || (playerID >= MAX_PLAYERS)) {
		luaL_error(L, "Bad playerID in %s\n", caller);
	}
	CPlayer* player = gs->players[playerID];
	if (player == NULL) {
		luaL_error(L, "Bad player in %s\n", caller);
	}
	return player;
}


static inline CTeam* ParseTeam(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "Bad teamID type in %s()\n", caller);
	}
	const int teamID = (int)lua_tonumber(L, index);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		luaL_error(L, "Bad teamID in %s\n", caller);
	}
	return gs->Team(teamID);
}


static inline int ParseTeamID(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "Bad teamID type in %s()\n", caller);
	}
	const int teamID = (int)lua_tonumber(L, index);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		luaL_error(L, "Bad teamID in %s\n", caller);
	}
	CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		luaL_error(L, "Bad teamID in %s\n", caller);
	}
	return teamID;
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
			array[i] = (float)lua_tonumber(L, -1);
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
                          const vector<float>& params,
                          const map<string, int>& paramsMap)
{
	lua_newtable(L);
	const int pCount = (int)params.size();
	for (int i = 0; i < pCount; i++) {
		lua_pushnumber(L, i + 1);
		lua_newtable(L);
		map<string, int>::const_iterator it;
		string name = "";
		for (it = paramsMap.begin(); it != paramsMap.end(); ++it) {
			if (it->second == i) {
				name = it->first;
				break;
			}
		}
		LuaPushNamedNumber(L, name, params[i]);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", pCount);

	return 1;
}


static int GetRulesParam(lua_State* L, const char* caller, int index,
                         const vector<float>& params,
                         const map<string, int>& paramsMap)
{
	int pIndex = -1;

	if (lua_israwnumber(L, index)) {
		pIndex = (int)lua_tonumber(L, index) - 1;
	}
	else if (lua_israwstring(L, index)) {
		const string pName = lua_tostring(L, index);
		map<string, int>::const_iterator it = paramsMap.find(pName);
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
	lua_pushnumber(L, params[pIndex]);
	return 1;
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
	lua_pushnumber(L, gs->gaiaTeamID);
	return 1;
}


int LuaSyncedRead::GetRulesInfoMap(lua_State* L)
{
	if (luaRules == NULL) {
		return 0;
	}
	const map<string, string>& infoMap = luaRules->GetInfoMap();
	map<string, string>::const_iterator it;

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isstring(L, 1)) {
			luaL_error(L, "Incorrect arguments to GetRulesInfoMap");
		}
		const string key = lua_tostring(L, 1);
		it = infoMap.find(key);
		if (it == infoMap.end()) {
			return 0;
		}
		lua_pushstring(L, it->second.c_str());
		return 1;
	}

	lua_newtable(L);
	for (it = infoMap.begin(); it != infoMap.end(); ++it) {
		LuaPushNamedString(L, it->first.c_str(), it->second.c_str());
	}
	return 1;
}


int LuaSyncedRead::GetGameSpeed(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gs->userSpeedFactor);
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
	const float seconds = ((float)gs->frameNum / 30.0f);
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
	const vector<float>& params       = CLuaRules::GetGameParams();
	const map<string, int>& paramsMap = CLuaRules::GetGameParamsMap();

	return PushRulesParams(L, __FUNCTION__, params, paramsMap);
}


int LuaSyncedRead::GetGameRulesParam(lua_State* L)
{
	const CLuaRules* lr = (const CLuaRules*)CLuaHandle::GetActiveHandle();
	const vector<float>& params       = lr->GetGameParams();
	const map<string, int>& paramsMap = lr->GetGameParamsMap();

	return GetRulesParam(L, __FUNCTION__, 1, params, paramsMap);
}


/******************************************************************************/

int LuaSyncedRead::GetMapOptions(lua_State* L)
{
	lua_newtable(L);
	if (gameSetup == NULL) {
		return 1;
	}
	const map<string, string>& mapOpts = gameSetup->mapOptions;
	map<string, string>::const_iterator it;
	for (it = mapOpts.begin(); it != mapOpts.end(); ++it) {
		lua_pushstring(L, it->first.c_str());
		lua_pushstring(L, it->second.c_str());
		lua_rawset(L, -3);
	}
	return 1;
}


int LuaSyncedRead::GetModOptions(lua_State* L)
{
	lua_newtable(L);
	if (gameSetup == NULL) {
		return 1;
	}
	const map<string, string>& modOpts = gameSetup->modOptions;
	map<string, string>::const_iterator it;
	for (it = modOpts.begin(); it != modOpts.end(); ++it) {
		lua_pushstring(L, it->first.c_str());
		lua_pushstring(L, it->second.c_str());
		lua_rawset(L, -3);
	}
	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetHeadingFromVector(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetHeadingFromVector()");
	}
	const float x = (float)lua_tonumber(L, 1);
	const float z = (float)lua_tonumber(L, 2);
	const short int heading = ::GetHeadingFromVector(x, z);
	lua_pushnumber(L, heading);
	return 1;
}


int LuaSyncedRead::GetVectorFromHeading(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetVectorFromHeading()");
	}
	const short int h = (short int)lua_tonumber(L, 1);
	const float3& vec = ::GetVectorFromHeading(h);
	lua_pushnumber(L, vec.x);
	lua_pushnumber(L, vec.z);
	return 2;
}


/******************************************************************************/

int LuaSyncedRead::GetAllyTeamStartBox(lua_State* L)
{
	if (gameSetup == NULL) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetAllyTeamStartBox()");
	}

	const int allyTeam = (int)lua_tonumber(L, 1);
	if ((allyTeam < 0) || (allyTeam >= gs->activeAllyTeams)) {
		luaL_error(L, "Bad allyTeam (%i) in GetAllyTeamStartBox()", allyTeam);
	}
	const float xmin = (gs->mapx * 8.0f) * gameSetup->startRectLeft[allyTeam];
	const float zmin = (gs->mapy * 8.0f) * gameSetup->startRectTop[allyTeam];
	const float xmax = (gs->mapx * 8.0f) * gameSetup->startRectRight[allyTeam];
	const float zmax = (gs->mapy * 8.0f) * gameSetup->startRectBottom[allyTeam];
	lua_pushnumber(L, xmin);
	lua_pushnumber(L, zmin);
	lua_pushnumber(L, xmax);
	lua_pushnumber(L, zmax);
	return 4;
}


int LuaSyncedRead::GetTeamStartPosition(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetTeamStartPosition()");
	}

	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(teamID)) {
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
	for (int at = 0; at < gs->activeAllyTeams; at++) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, at);
		lua_rawset(L, -3);
	}
	hs_n.PushNumber(L, count);
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
		allyTeamID = (int)lua_tonumber(L, 1);
		if ((allyTeamID < 0) || (allyTeamID >= gs->activeAllyTeams)) {
			return 0;
		}
	}

	lua_newtable(L);
	int count = 0;
	for (int t = 0; t < gs->activeTeams; t++) {
		if (gs->Team(t) == NULL) {
			continue;
		}
		if ((allyTeamID < 0) || (allyTeamID == gs->AllyTeam(t))) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, t);
			lua_rawset(L, -3);
		}
	}
	hs_n.PushNumber(L, count);

	return 1;
}


int LuaSyncedRead::GetPlayerList(lua_State* L)
{
	int teamID = -1;
	bool active = false;

	if (lua_isnumber(L, 1)) {
		teamID = (int)lua_tonumber(L, 1);
		if (lua_isboolean(L, 2)) {
			active = lua_toboolean(L, 2);
		}
	}
	else if (lua_isboolean(L, 1)) {
		active = lua_toboolean(L, 1);
		if (lua_isnumber(L, 2)) {
			teamID = (int)lua_tonumber(L, 2);
		}
	}

	if (teamID >= gs->activeTeams) {
		return 0;
	}

	lua_newtable(L);
	int count = 0;
	for (int p = 0; p < gs->activePlayers; p++) {
		const CPlayer* player = gs->players[p];
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
	hs_n.PushNumber(L, count);

	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetTeamInfo(lua_State* L)
{
	const int teamID = (int)luaL_checknumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	bool isAiTeam = false;
	if (!team->luaAI.empty() ||
	    ((globalAI != NULL) && (globalAI->ais[teamID] != NULL))) {
		isAiTeam = true;
	}

	lua_pushnumber(L,  team->teamNum);
	lua_pushnumber(L,  team->leader);
	lua_pushboolean(L, team->isDead);
	lua_pushboolean(L, isAiTeam);
	lua_pushstring(L,  team->side.c_str());
	lua_pushnumber(L,  gs->AllyTeam(team->teamNum));

	return 6;
}


int LuaSyncedRead::GetTeamResources(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetTeamResources(teamID, \"type\")");
	}

	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(teamID)) {
		return 0;
	}

	const string type = lua_tostring(L, 2);
	if (type == "metal") {
		lua_pushnumber(L, team->metal);
		lua_pushnumber(L, team->metalStorage);
		lua_pushnumber(L, team->prevMetalPull);
		lua_pushnumber(L, team->prevMetalIncome);
		lua_pushnumber(L, team->prevMetalExpense);
		lua_pushnumber(L, team->metalShare);
		lua_pushnumber(L, team->metalSent);
		lua_pushnumber(L, team->metalReceived);
		return 8;
	}
	else if (type == "energy") {
		lua_pushnumber(L, team->energy);
		lua_pushnumber(L, team->energyStorage);
		lua_pushnumber(L, team->prevEnergyPull);
		lua_pushnumber(L, team->prevEnergyIncome);
		lua_pushnumber(L, team->prevEnergyExpense);
		lua_pushnumber(L, team->energyShare);
		lua_pushnumber(L, team->energySent);
		lua_pushnumber(L, team->energyReceived);
		return 8;
	}

	return 0;
}


int LuaSyncedRead::GetTeamUnitStats(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetTeamUnitStats(teamID)");
	}

	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(teamID)) {
		return 0;
	}

	lua_pushnumber(L, team->currentStats.unitsKilled);
	lua_pushnumber(L, team->currentStats.unitsDied);
	lua_pushnumber(L, team->currentStats.unitsCaptured);
	lua_pushnumber(L, team->currentStats.unitsOutCaptured);
	lua_pushnumber(L, team->currentStats.unitsReceived);
	lua_pushnumber(L, team->currentStats.unitsSent);

	return 6;
}


int LuaSyncedRead::GetTeamRulesParams(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(teamID)) {
		return 0;
	}

	const vector<float>& params       = team->modParams;
	const map<string, int>& paramsMap = team->modParamsMap;

	return PushRulesParams(L, __FUNCTION__, params, paramsMap);
}


int LuaSyncedRead::GetTeamRulesParam(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}

	const vector<float>& params       = team->modParams;
	const map<string, int>& paramsMap = team->modParamsMap;

	return GetRulesParam(L, __FUNCTION__, 2, params, paramsMap);
}


int LuaSyncedRead::GetTeamStatsHistory(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(teamID) && !game->gameOver) {
		return 0;
	}

	const int args = lua_gettop(L);
	if (args == 1) {
		lua_pushnumber(L, team->statHistory.size());
		return 1;
	}

	const list<CTeam::Statistics>& teamStats = team->statHistory;
	std::list<CTeam::Statistics>::const_iterator it = teamStats.begin();
	const int statCount = teamStats.size();

	int start = 0;
	if ((args >= 2) && lua_isnumber(L, 2)) {
		start = (int)lua_tonumber(L, 2) - 1;
		start = max(0, min(statCount - 1, start));
	}

	int end = start;
	if ((args >= 3) && lua_isnumber(L, 3)) {
		end = (int)lua_tonumber(L, 3) - 1;
		end = max(0, min(statCount - 1, end));
	}

	std::advance(it, start);

	const int statsFrames = (CTeam::statsPeriod * GAME_SPEED);

	lua_newtable(L);
	int count = 0;
	if (statCount > 0) {
		for (int i = start; i <= end; ++i, ++it) {
			const CTeam::Statistics& stats = *it;
			count++;
			lua_pushnumber(L, count);
			lua_newtable(L); {
				HSTR_PUSH_NUMBER(L, "time",             i * CTeam::statsPeriod);
				HSTR_PUSH_NUMBER(L, "frame",            i * statsFrames);
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
	hs_n.PushNumber(L, count);
	return 1;
}


int LuaSyncedRead::GetTeamLuaAI(lua_State* L)
{
	CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	lua_pushstring(L, team->luaAI.c_str());
	return 1;
}


/******************************************************************************/

int LuaSyncedRead::GetPlayerInfo(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetPlayerInfo(playerID)");
	}

	const int playerID = (int)lua_tonumber(L, 1);
	if ((playerID < 0) || (playerID >= MAX_PLAYERS)) {
		return 0;
	}

	const CPlayer* player = gs->players[playerID];
	if (player == NULL) {
		return 0;
	}

	// no player names for synchronized scripts
	if (CLuaHandle::GetActiveHandle()->GetSynced()) {
		HSTR_PUSH(L, "SYNCED_NONAME");
	} else {
		lua_pushstring(L, player->playerName.c_str());
	}
	lua_pushboolean(L, player->active);
	lua_pushboolean(L, player->spectator);
	lua_pushnumber(L, player->team);
	lua_pushnumber(L, gs->AllyTeam(player->team));
	const float pingScale = (GAME_SPEED * gs->speedFactor);
	const float pingSecs = float(player->ping - 1) / pingScale;
	lua_pushnumber(L, pingSecs);
	lua_pushnumber(L, player->cpuUsage);
	lua_pushstring(L, player->countryCode.c_str());
	lua_pushnumber(L, player->rank);

	return 9;
}


int LuaSyncedRead::GetPlayerControlledUnit(lua_State* L)
{
#ifndef DIRECT_CONTROL_ALLOWED
	return 0;
#else
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetPlayerControlledUnit(playerID)");
	}

	const int playerID = (int)lua_tonumber(L, 1);
	if ((playerID < 0) || (playerID >= MAX_PLAYERS)) {
		return 0;
	}

	const CPlayer* player = gs->players[playerID];
	if (player == NULL) {
		return 0;
	}

	CUnit* unit = player->playerControlledUnit;
	if (unit == NULL) {
		return 0;
	}

	if ((readAllyTeam == CLuaHandle::NoAccessTeam) ||
	    ((readAllyTeam >= 0) && !gs->Ally(unit->allyteam, readAllyTeam))) {
		return 0;
	}

	lua_pushnumber(L, unit->id);
	return 1;
#endif
}


int LuaSyncedRead::AreTeamsAllied(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, -1) || !lua_isnumber(L, -2)) {
		luaL_error(L, "Incorrect arguments to AreTeamsAllied(team1, team2)");
	}
	const int team1 = (int)lua_tonumber(L, -1);
	const int team2 = (int)lua_tonumber(L, -2);
	if ((team1 < 0) || (team1 >= gs->activeTeams) ||
	    (team2 < 0) || (team2 >= gs->activeTeams)) {
		return 0;
	}
	lua_pushboolean(L, gs->AlliedTeams(team1, team2));
	return 1;
}


int LuaSyncedRead::ArePlayersAllied(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, -1) || !lua_isnumber(L, -2)) {
		luaL_error(L, "Incorrect arguments to ArePlayersAllied(p1, p2)");
	}
	const int player1 = (int)lua_tonumber(L, -1);
	const int player2 = (int)lua_tonumber(L, -2);
	if ((player1 < 0) || (player1 >= MAX_PLAYERS) ||
	    (player2 < 0) || (player2 >= MAX_PLAYERS)) {
		return 0;
	}
	const CPlayer* p1 = gs->players[player1];
	const CPlayer* p2 = gs->players[player2];
	if ((p1 == NULL) || (p2 == NULL)) {
		return 0;
	}
	lua_pushboolean(L, gs->AlliedTeams(p1->team, p2->team));
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
	lua_newtable(L);
	int count = 0;
	std::list<CUnit*>::const_iterator uit;
	if (fullRead) {
		for (uit = uh->activeUnits.begin(); uit != uh->activeUnits.end(); ++uit) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, (*uit)->id);
			lua_rawset(L, -3);
		}
	}
	else {
		for (uit = uh->activeUnits.begin(); uit != uh->activeUnits.end(); ++uit) {
			if (IsUnitVisible(*uit)) {
				count++;
				lua_pushnumber(L, count);
				lua_pushnumber(L, (*uit)->id);
				lua_rawset(L, -3);
			}
		}
	}
	hs_n.PushNumber(L, count);
	return 1;
}


int LuaSyncedRead::GetTeamUnits(lua_State* L)
{
	if (readAllyTeam == CLuaHandle::NoAccessTeam) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetTeamUnits(teamID)");
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
	if (IsAlliedTeam(teamID)) {
		lua_newtable(L);
		int count = 0;
		for (uit = units.begin(); uit != units.end(); ++uit) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, (*uit)->id);
			lua_rawset(L, -3);
		}
		hs_n.PushNumber(L, count);
		return 1;
	}

	// check visibility for enemies
	lua_newtable(L);
	int count = 0;
	for (uit = units.begin(); uit != units.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitVisible(unit)) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, unit->id);
			lua_rawset(L, -3);
		}
	}
	hs_n.PushNumber(L, count);
	return 1;
}


int LuaSyncedRead::GetTeamUnitsSorted(lua_State* L)
{
	if (readAllyTeam == CLuaHandle::NoAccessTeam) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetTeamUnitsSorted(teamID)");
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
	if (IsAlliedTeam(teamID)) {
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
			if (IsUnitVisible(unit)) {
				const UnitDef* ud = EffectiveUnitDef(unit);
				if (IsUnitTyped(unit)) {
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
	for (mit = unitDefMap.begin(); mit != unitDefMap.end(); mit++) {
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
			hs_n.PushNumber(L, v.size());
		}
		lua_rawset(L, -3);
	}
	hs_n.PushNumber(L, unitDefMap.size());
	return 1;
}


int LuaSyncedRead::GetTeamUnitsCounts(lua_State* L)
{
	if (readAllyTeam == CLuaHandle::NoAccessTeam) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetTeamUnitsCounts(teamID)");
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	// send the raw unitsByDefs counts for allies
	if (IsAlliedTeam(teamID)) {
		lua_newtable(L);
		int defCount = 0;
		for (int udID = 0; udID < (unitDefHandler->numUnitDefs + 1); udID++) {
			const int unitCount = uh->unitsByDefs[teamID][udID].size();
			if (unitCount > 0) {
				lua_pushnumber(L, udID);
				lua_pushnumber(L, unitCount);
				lua_rawset(L, -3);
				defCount++;
			}
		}
		hs_n.PushNumber(L, defCount);
		return 1;
	}

	// tally the counts for enemies
	map<int, int> unitDefCounts; // use the unitDef->id for ordering
	const CUnitSet& unitSet = team->units;
	CUnitSet::const_iterator uit;
	int unknownCount = 0;
	for (uit = unitSet.begin(); uit != unitSet.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitVisible(unit)) {
			if (!IsUnitTyped(unit)) {
				unknownCount++;
			} else {
				const UnitDef* ud = EffectiveUnitDef(unit);
				map<int, int>::iterator mit = unitDefCounts.find(ud->id);
				if (mit == unitDefCounts.end()) {
					unitDefCounts[ud->id] = 1;
				} else {
					unitDefCounts[ud->id] = mit->second + 1;
				}
			}
		}
	}

	// push the counts
	lua_newtable(L);
	int defCount = 0;
	map<int, int>::const_iterator mit;
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
	if (readAllyTeam == CLuaHandle::NoAccessTeam) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) ||
	    !(lua_isnumber(L, 2) || lua_istable(L, 2))) {
		luaL_error(L, "Incorrect arguments to GetTeamUnitsByDefs()");
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	const bool allied = IsAlliedTeam(teamID);

	// parse the unitDefs
	set<int> defs;
	if (lua_isnumber(L, 2)) {
		const int unitDefID = (int)lua_tonumber(L, 2);
		const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
		InsertSearchUnitDefs(ud, allied, defs);
	}
	else if (lua_istable(L, 2)) {
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_isnumber(L, -1)) {
				const int unitDefID = (int)lua_tonumber(L, -1);
				const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
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
		const CUnitSet& units = uh->unitsByDefs[teamID][*udit];
		CUnitSet::const_iterator uit;
		for (uit = units.begin(); uit != units.end(); ++uit) {
			const CUnit* unit = *uit;
			if (allied || IsUnitTyped(unit)) {
				count++;
				lua_pushnumber(L, count);
				lua_pushnumber(L, unit->id);
				lua_rawset(L, -3);
			}
		}
	}
	hs_n.PushNumber(L, count);

	return 1;
}


int LuaSyncedRead::GetTeamUnitDefCount(lua_State* L)
{
	if (readAllyTeam == CLuaHandle::NoAccessTeam) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetTeamUnitDefCount()");
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	// parse the unitDef
	const int unitDefID = (int)lua_tonumber(L, 2);
	const UnitDef* unitDef = unitDefHandler->GetUnitByID(unitDefID);
	if (unitDef == NULL) {
		luaL_error(L, "Bad unitDefID in GetTeamUnitDefCount()");
	}

	// use the unitsByDefs count for allies
	if (IsAlliedTeam(teamID)) {
		lua_pushnumber(L, uh->unitsByDefs[teamID][unitDefID].size());
		return 1;
	}

	// you can never count enemy decoys
	if (unitDef->decoyDef != NULL) {
		lua_pushnumber(L, 0);
		return 1;
	}

	int count = 0;

	// tally the given unitDef units
	const CUnitSet& units = uh->unitsByDefs[teamID][unitDef->id];
	CUnitSet::const_iterator uit;
	for (uit = units.begin(); uit != units.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitTyped(unit)) {
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
			const CUnitSet& units = uh->unitsByDefs[teamID][*dit];
			CUnitSet::const_iterator uit;
			for (uit = units.begin(); uit != units.end(); ++uit) {
				const CUnit* unit = *uit;
				if (IsUnitTyped(unit)) {
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
	if (readAllyTeam == CLuaHandle::NoAccessTeam) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetTeamUnitCount()");
	}

	// parse the team
	const CTeam* team = ParseTeam(L, __FUNCTION__, 1);
	if (team == NULL) {
		return 0;
	}
	const int teamID = team->teamNum;

	// use the raw team count for allies
	if (IsAlliedTeam(teamID)) {
		lua_pushnumber(L, team->units.size());
		return 1;
	}

	// loop through the units for enemies
	int count = 0;
	const CUnitSet& units = team->units;
	CUnitSet::const_iterator uit;
	for (uit = units.begin(); uit != units.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitVisible(unit)) {
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
	if (!IsUnitVisible(unit))     { continue; }

#define SIMPLE_TEAM_TEST \
	if (unit->team != allegiance) { continue; }

#define VISIBLE_TEAM_TEST \
	if (unit->team != allegiance) { continue; } \
	if (!IsUnitVisible(unit))     { continue; }

#define MY_UNIT_TEST \
	if (unit->team != readTeam)   { continue; }

#define ALLY_UNIT_TEST \
	if (unit->allyteam != readAllyTeam) { continue; }

#define ENEMY_UNIT_TEST \
	if (unit->allyteam == readAllyTeam) { continue; } \
	if (!IsUnitVisible(unit))           { continue; }


static int ParseAllegiance(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		return AllUnits;
	}
	const int teamID = (int)lua_tonumber(L, index);
	if (fullRead && (teamID < 0)) {
		// MyUnits, AllyUnits, and EnemyUnits do not apply to fullRead
		return AllUnits;
	}
	if (teamID < EnemyUnits) {
		luaL_error(L, "Bad teamID in %s (%i)", caller, teamID);
	} else if (teamID >= gs->activeTeams) {
		luaL_error(L, "Bad teamID in %s (%i)", caller, teamID);
	}
	return teamID;
}


int LuaSyncedRead::GetUnitsInRectangle(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to GetUnitsInRectangle()");
	}
	const float xmin = (float)lua_tonumber(L, 1);
	const float zmin = (float)lua_tonumber(L, 2);
	const float xmax = (float)lua_tonumber(L, 3);
	const float zmax = (float)lua_tonumber(L, 4);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	const int allegiance = ParseAllegiance(L, __FUNCTION__, 5);

#define RECTANGLE_TEST ; // no test, GetUnitsExact is sufficient

	vector<CUnit*>::const_iterator it;
	vector<CUnit*> units = qf->GetUnitsExact(mins, maxs);

	lua_newtable(L);
	int count = 0;

	if (allegiance >= 0) {
		if (IsAlliedTeam(allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, RECTANGLE_TEST);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, RECTANGLE_TEST);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetActiveHandle()->GetReadTeam();
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

	hs_n.PushNumber(L, count);

	return 1;
}


int LuaSyncedRead::GetUnitsInBox(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 6) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
	    !lua_isnumber(L, 4) || !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
		luaL_error(L,
			"Incorrect arguments to GetUnitsInBox(xmin,ymin,zmin, xmax,ymax,zmax");
	}
	const float xmin = (float)lua_tonumber(L, 1);
	const float ymin = (float)lua_tonumber(L, 2);
	const float zmin = (float)lua_tonumber(L, 3);
	const float xmax = (float)lua_tonumber(L, 4);
	const float ymax = (float)lua_tonumber(L, 5);
	const float zmax = (float)lua_tonumber(L, 6);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	const int allegiance = ParseAllegiance(L, __FUNCTION__, 7);

#define BOX_TEST                  \
	const float y = unit->midPos.y; \
	if ((y < ymin) || (y > ymax)) { \
		continue;                     \
	}

	vector<CUnit*>::const_iterator it;
	vector<CUnit*> units = qf->GetUnitsExact(mins, maxs);

	lua_newtable(L);
	int count = 0;

	if (allegiance >= 0) {
		if (IsAlliedTeam(allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, BOX_TEST);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, BOX_TEST);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetActiveHandle()->GetReadTeam();
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

	hs_n.PushNumber(L, count);

	return 1;
}


int LuaSyncedRead::GetUnitsInCylinder(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to GetUnitsInCylinder()");
	}
	const float x = (float)lua_tonumber(L, 1);
	const float z = (float)lua_tonumber(L, 2);
	const float radius = (float)lua_tonumber(L, 3);
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
	vector<CUnit*> units = qf->GetUnitsExact(mins, maxs);

	lua_newtable(L);
	int count = 0;

	if (allegiance >= 0) {
		if (IsAlliedTeam(allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, CYLINDER_TEST);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, CYLINDER_TEST);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetActiveHandle()->GetReadTeam();
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

	hs_n.PushNumber(L, count);

	return 1;
}


int LuaSyncedRead::GetUnitsInSphere(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to GetUnitsInSphere()");
	}
	const float x = (float)lua_tonumber(L, 1);
	const float y = (float)lua_tonumber(L, 2);
	const float z = (float)lua_tonumber(L, 3);
	const float radius = (float)lua_tonumber(L, 4);
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
	vector<CUnit*> units = qf->GetUnitsExact(mins, maxs);

	lua_newtable(L);
	int count = 0;

	if (allegiance >= 0) {
		if (IsAlliedTeam(allegiance)) {
			LOOP_UNIT_CONTAINER(SIMPLE_TEAM_TEST, SPHERE_TEST);
		} else {
			LOOP_UNIT_CONTAINER(VISIBLE_TEAM_TEST, SPHERE_TEST);
		}
	}
	else if (allegiance == MyUnits) {
		const int readTeam = CLuaHandle::GetActiveHandle()->GetReadTeam();
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

	hs_n.PushNumber(L, count);

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
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_istable(L, 1)) {
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
		const int readTeam = CLuaHandle::GetActiveHandle()->GetReadTeam();
		startTeam = readTeam;
		endTeam = readTeam;
	}
	else {
		startTeam = 0;
		endTeam = gs->activeTeams - 1;
	}

#define PLANES_TEST                  \
	if (!UnitInPlanes(unit, planes)) { \
		continue;                        \
	}

	lua_newtable(L);
	int count = 0;

	const int readTeam = CLuaHandle::GetActiveHandle()->GetReadTeam();

	for (int team = startTeam; team <= endTeam; team++) {
		const CUnitSet& units = gs->Team(team)->units;
		CUnitSet::const_iterator it;

		if (allegiance >= 0) {
			if (allegiance == team) {
				if (IsAlliedTeam(allegiance)) {
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
			if (readAllyTeam == gs->AllyTeam(team)) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST);
			}
		}
		else if (allegiance == EnemyUnits) {
			if (readAllyTeam != gs->AllyTeam(team)) {
				LOOP_UNIT_CONTAINER(VISIBLE_TEST, PLANES_TEST);
			}
		}
		else { // AllUnits
			if (IsAlliedTeam(team)) {
				LOOP_UNIT_CONTAINER(NULL_TEST, PLANES_TEST);
			} else {
				LOOP_UNIT_CONTAINER(VISIBLE_TEST, PLANES_TEST);
			}
		}
	}

	hs_n.PushNumber(L, count);

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
	CUnit* target =
		helper->GetClosestFriendlyUnit(unit->pos, range, unit->allyteam);
	if (target) {
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
		!fullRead || !lua_isboolean(L, 3) || lua_toboolean(L, 3);
	CUnit* target = NULL;
	if (useLos) {
		target = helper->GetClosestEnemyUnit(unit->pos, range, unit->allyteam);
	} else {
		target = helper->GetClosestEnemyUnitNoLosTest(unit->pos, range,
		                                              unit->allyteam, false);
	}
	if (target) {
		lua_pushnumber(L, target->id);
		return 1;
	}
	return 0;
}


/******************************************************************************/

int LuaSyncedRead::GetFeaturesInRectangle(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to GetFeaturesInRectangle()");
	}
	const float xmin = (float)lua_tonumber(L, 1);
	const float zmin = (float)lua_tonumber(L, 2);
	const float xmax = (float)lua_tonumber(L, 3);
	const float zmax = (float)lua_tonumber(L, 4);

	const float3 mins(xmin, 0.0f, zmin);
	const float3 maxs(xmax, 0.0f, zmax);

	vector<CFeature*> rectFeatures = qf->GetFeaturesExact(mins, maxs);
	const int rectFeatureCount = (int)rectFeatures.size();

	lua_newtable(L);
	int count = 0;
	if (readAllyTeam < 0) {
		if (fullRead) {
			for (int i = 0; i < rectFeatureCount; i++) {
				const CFeature* feature = rectFeatures[i];
				count++;
				lua_pushnumber(L, count);
				lua_pushnumber(L, feature->id);
				lua_rawset(L, -3);
			}
		}
	}
	else {
		for (int i = 0; i < rectFeatureCount; i++) {
			const CFeature* feature = rectFeatures[i];
			if ((feature->allyteam != -1) &&
			    (feature->allyteam != readAllyTeam) &&
			    !loshandler->InLos(feature->pos, readAllyTeam)) {
				continue;
			}
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, feature->id);
			lua_rawset(L, -3);
		}
	}

	hs_n.PushNumber(L, count);

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::GetUnitStates(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "firestate",  unit->fireState);
	HSTR_PUSH_NUMBER(L, "movestate",  unit->moveState);
	HSTR_PUSH_BOOL  (L, "repeat",     unit->commandAI->repeatOrders);
	HSTR_PUSH_BOOL  (L, "cloak",      unit->wantCloak);
	HSTR_PUSH_BOOL  (L, "active",     unit->activated);
	HSTR_PUSH_BOOL  (L, "trajectory", unit->useHighTrajectory);

	const AMoveType* mt = unit->moveType;
	if (mt) {
		const CTAAirMoveType* taAirMove = dynamic_cast<const CTAAirMoveType*>(mt);
		if (taAirMove) {
			HSTR_PUSH_BOOL  (L, "autoland",        taAirMove->autoLand);
			HSTR_PUSH_NUMBER(L, "autorepairlevel", taAirMove->repairBelowHealth);
		}
		else {
			const CAirMoveType* airMove = dynamic_cast<const CAirMoveType*>(mt);
			if (airMove) {
				HSTR_PUSH_BOOL  (L, "autoland",        airMove->autoLand);
				HSTR_PUSH_BOOL  (L, "loopbackattack",  airMove->loopbackAttack);
				HSTR_PUSH_NUMBER(L, "autorepairlevel", airMove->repairBelowHealth);
			}
		}
	}

	return 1;
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


int LuaSyncedRead::GetUnitTooltip(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	string tooltip;
	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = IsAllyUnit(unit) ? NULL : unitDef->decoyDef;
	const UnitDef* effectiveDef = EffectiveUnitDef(unit);
	if (effectiveDef->showPlayerName) {
		tooltip = gs->players[gs->Team(unit->team)->leader]->playerName;
	} else {
		if (!decoyDef) {
			tooltip = unit->tooltip;
		} else {
			tooltip = decoyDef->humanName + " - " + decoyDef->tooltip;
		}
	}
	lua_pushstring(L, tooltip.c_str());
	return 1;
}


int LuaSyncedRead::GetUnitDefID(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (IsAllyUnit(unit)) {
		lua_pushnumber(L, unit->unitDef->id);
	}
	else {
		const int losStatus = unit->losStatus[readAllyTeam];
		const int prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
		if (((losStatus & LOS_INLOS) == 0) &&
				((losStatus & prevMask) != prevMask)) {
			return 0;
		}
		lua_pushnumber(L, EffectiveUnitDef(unit)->id);
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


int LuaSyncedRead::GetUnitLineage(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->lineage);
	if (IsEnemyUnit(unit)) {
		return 1;
	}
	const CTeam* team = gs->Team(unit->team);
	if (team == NULL) {
		return 1;
	}
	lua_pushboolean(L, team->lineageRoot == unit->id);
	return 2;
}


int LuaSyncedRead::GetUnitNeutral(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->neutral);
	return 1;
}


int LuaSyncedRead::GetUnitHealth(lua_State* L)
{
	CUnit* unit = ParseInLosUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const UnitDef* ud = unit->unitDef;
	const bool enemyUnit = IsEnemyUnit(unit);
	if (ud->hideDamage && enemyUnit) {
		return 0;
	}
	if (!enemyUnit || (ud->decoyDef == NULL)) {
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
	lua_pushboolean(L, unit->stunned || unit->beingBuilt);
	lua_pushboolean(L, unit->stunned);
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


int LuaSyncedRead::GetUnitExperience(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->experience);
	return 1;
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
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	float3 pos;
	if (IsAllyUnit(unit)) {
		pos = unit->midPos;
	} else {
		pos = helper->GetUnitErrorPos(unit, readAllyTeam);
	}
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}


int LuaSyncedRead::GetUnitBasePosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	float3 pos;
	if (IsAllyUnit(unit)) {
		pos = unit->pos;
	} else {
		pos = helper->GetUnitErrorPos(unit, readAllyTeam);
		pos = pos - (unit->midPos - unit->pos);
	}
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
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


int LuaSyncedRead::GetUnitTransporter(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
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
	list<CTransportUnit::TransportedUnit>::const_iterator it;
	int count = 0;
	for (it = tu->transported.begin(); it != tu->transported.end(); ++it) {
		const CUnit* carried = it->unit;
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, carried->id);
		lua_rawset(L, -3);
	}
	hs_n.PushNumber(L, count);
	return 1;
}


int LuaSyncedRead::GetUnitShieldState(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const CPlasmaRepulser* shield = (CPlasmaRepulser*)unit->shieldWeapon;
	if (shield == NULL) {
		return 0;
	}
	lua_pushnumber(L, shield->isEnabled);
	lua_pushnumber(L, shield->curPower);
	return 2;
}


int LuaSyncedRead::GetUnitWeaponState(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetUnitWeaponState()");
	}
	const int weaponNum = (int)lua_tonumber(L, 2);
	if ((weaponNum < 0) || (weaponNum >= unit->weapons.size())) {
		return 0;
	}
	const CWeapon* weapon = unit->weapons[weaponNum];
	lua_pushboolean(L, weapon->angleGood);
	lua_pushboolean(L, weapon->reloadStatus <= gs->frameNum);
	lua_pushnumber(L,  weapon->reloadStatus);
	lua_pushnumber(L,  weapon->salvoLeft);
	lua_pushnumber(L,  weapon->numStockpiled);
	return 5;
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


int LuaSyncedRead::GetUnitEstimatedPath(lua_State* L)
{
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const CGroundMoveType* gndMove =
		dynamic_cast<const CGroundMoveType*>(unit->moveType);
	if (gndMove == NULL) {
		return 0;
	}

	if (gndMove->pathId == 0) {
		return 0;
	}

	vector<float3> points;
	vector<int>    starts;
	pathManager->GetEstimatedPath(gndMove->pathId, points, starts);

	const int pointCount = (int)points.size();

	lua_newtable(L);
	for (int i = 0; i < pointCount; i++) {
		lua_pushnumber(L, i + 1);
		lua_newtable(L); {
			const float3& p = points[i];
			lua_pushnumber(L, 1); lua_pushnumber(L, p.x); lua_rawset(L, -3);
			lua_pushnumber(L, 2); lua_pushnumber(L, p.y); lua_rawset(L, -3);
			lua_pushnumber(L, 3); lua_pushnumber(L, p.z); lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}

	const int startCount = (int)starts.size();

	lua_newtable(L);
	for (int i = 0; i < startCount; i++) {
		lua_pushnumber(L, i + 1);
		lua_pushnumber(L, starts[i] + 1);
		lua_rawset(L, -3);
	}

	return 2;
}


int LuaSyncedRead::GetUnitLosState(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	int allyTeam = readAllyTeam;
	if (readAllyTeam < 0) {
		if (!fullRead) {
			return 0;
		}
		allyTeam = luaL_checkint(L, 2);
	}
	if ((allyTeam < 0) || (allyTeam >= gs->activeAllyTeams)) {
		return 0;
	}
	const unsigned short losStatus = unit->losStatus[allyTeam];

	if (fullRead && lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		lua_pushnumber(L, losStatus);
		return 1;
	}
	
	lua_newtable(L);
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
	if (IsAllyUnit(unit1)) {
		pos1 = unit1->midPos;
	} else {
		pos1 = helper->GetUnitErrorPos(unit1, readAllyTeam);
	}
	float3 pos2;
	if (IsAllyUnit(unit1)) {
		pos2 = unit2->midPos;
	} else {
		pos2 = helper->GetUnitErrorPos(unit2, readAllyTeam);
	}

	float dist;
	const int args = lua_gettop(L); // number of arguments
	if ((args >= 3) && lua_isboolean(L, 3) && lua_toboolean(L, 3)) {
		dist = pos1.distance2D(pos2);
	} else {
		dist = pos1.distance(pos2);
	}
	lua_pushnumber(L, dist);

	return 1;
}


int LuaSyncedRead::GetUnitDefDimensions(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetUnitDefDimensions()");
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const S3DOModel* model = ud->LoadModel(0);
	if (model == NULL) {
		return 0;
	}
	const S3DOModel& m = *model;
	const float3& mid = model->relMidPos;
	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "height", m.height);
	HSTR_PUSH_NUMBER(L, "radius", m.radius);
	HSTR_PUSH_NUMBER(L, "midx",   mid.x);
	HSTR_PUSH_NUMBER(L, "minx",   m.minx);
	HSTR_PUSH_NUMBER(L, "maxx",   m.maxx);
	HSTR_PUSH_NUMBER(L, "midy",   mid.y);
	HSTR_PUSH_NUMBER(L, "miny",   m.miny);
	HSTR_PUSH_NUMBER(L, "maxy",   m.maxy);
	HSTR_PUSH_NUMBER(L, "midz",   mid.z);
	HSTR_PUSH_NUMBER(L, "minz",   m.minz);
	HSTR_PUSH_NUMBER(L, "maxz",   m.maxz);
	return 1;
}


/******************************************************************************/

static void PackCommand(lua_State* L, const Command& cmd)
{
	lua_newtable(L);

	HSTR_PUSH_NUMBER(L, "id", cmd.id);

	HSTR_PUSH(L, "params");
	lua_newtable(L);
	for (int p = 0; p < cmd.params.size(); p++) {
		lua_pushnumber(L, p + 1);
		lua_pushnumber(L, cmd.params[p]);
		lua_rawset(L, -3);
	}
	hs_n.PushNumber(L, cmd.params.size());
	lua_rawset(L, -3); // params table

	HSTR_PUSH(L, "options");
	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "coded", cmd.options);
	if (cmd.options & ALT_KEY)         { HSTR_PUSH_BOOL(L, "alt",      true); }
	if (cmd.options & CONTROL_KEY)     { HSTR_PUSH_BOOL(L, "ctrl",     true); }
	if (cmd.options & SHIFT_KEY)       { HSTR_PUSH_BOOL(L, "shift",    true); }
	if (cmd.options & RIGHT_MOUSE_KEY) { HSTR_PUSH_BOOL(L, "right",    true); }
	if (cmd.options & INTERNAL_ORDER)  { HSTR_PUSH_BOOL(L, "internal", true); }
	lua_rawset(L, -3); // options table

	HSTR_PUSH_NUMBER(L, "tag", cmd.tag);
}


static int PackCommandQueue(lua_State* L, const CCommandQueue& q, int count)
{
	lua_newtable(L);

	int i = 0;
	CCommandQueue::const_iterator it;
	for (it = q.begin(); it != q.end(); it++) {
		if (i >= count) {
			break;
		}
		i++;
		lua_pushnumber(L, i);
		PackCommand(L, *it);
		lua_rawset(L, -3);
	}
	hs_n.PushNumber(L, i);

	return 1;
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
 		count = (int)lua_tonumber(L, 2);
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
 		count = (int)lua_tonumber(L, 2);
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
		const int cmdID = it->id;
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
 		count = (int)lua_tonumber(L, 2);
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
 		count = (int)lua_tonumber(L, 2);
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
	for (it = commandQue.begin(); it != commandQue.end(); it++) {
		if (it->id < 0) { // a build command
			const int unitDefID = -(it->id);

			if (canBuild) {
				// skip build orders that this unit can not start
				const UnitDef* order_ud = unitDefHandler->GetUnitByID(unitDefID);
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
	if (unit == NULL) {
		luaL_error(L, "Incorrect arguments to GetUnitRulesParams()");
	}

	const vector<float>& params       = unit->modParams;
	const map<string, int>& paramsMap = unit->modParamsMap;

	return PushRulesParams(L, __FUNCTION__, params, paramsMap);
}


int LuaSyncedRead::GetUnitRulesParam(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	const vector<float>& params       = unit->modParams;
	const map<string, int>& paramsMap = unit->modParamsMap;

	return GetRulesParam(L, __FUNCTION__, 2, params, paramsMap);
}


/******************************************************************************/

static void PushCommandDesc(lua_State* L, const CommandDescription& cd)
{
	lua_newtable(L);

	HSTR_PUSH_NUMBER(L, "id",          cd.id);
	HSTR_PUSH_NUMBER(L, "type",        cd.type);
	HSTR_PUSH_STRING(L, "name",        cd.name);
	HSTR_PUSH_STRING(L, "action",      cd.action);
	HSTR_PUSH_STRING(L, "tooltip",     cd.tooltip);
	HSTR_PUSH_STRING(L, "texture",     cd.iconname);
	HSTR_PUSH_STRING(L, "cursor",      cd.mouseicon);
	HSTR_PUSH_BOOL(L,   "hidden",      cd.onlyKey);
	HSTR_PUSH_BOOL(L,   "disabled",    cd.disabled);
	HSTR_PUSH_BOOL(L,   "showUnique",  cd.showUnique);
	HSTR_PUSH_BOOL(L,   "onlyTexture", cd.onlyTexture);

	HSTR_PUSH(L, "params");
	lua_newtable(L);
	const int pCount = (int)cd.params.size();
	for (int p = 0; p < pCount; p++) {
		lua_pushnumber(L, p + 1);
		lua_pushstring(L, cd.params[p].c_str());
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", pCount);
	lua_rawset(L, -3);
}


int LuaSyncedRead::GetUnitCmdDescs(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	vector<CommandDescription>& cmdDescs = unit->commandAI->possibleCommands;
	const int lastDesc = (int)cmdDescs.size() - 1;

	const int args = lua_gettop(L); // number of arguments
	int startIndex = 0;
	int endIndex = lastDesc;
	if ((args >= 2) && lua_isnumber(L, 2)) {
		startIndex = (int)lua_tonumber(L, 2) - 1;
		startIndex = max(0, min(lastDesc, startIndex));
		if ((args >= 3) && lua_isnumber(L, 3)) {
			endIndex = (int)lua_tonumber(L, 3) - 1;
			endIndex = max(0, min(lastDesc, endIndex));
		} else {
			endIndex = startIndex;
		}
	}
	lua_newtable(L);
	int count = 0;
	for (int i = startIndex; i <= endIndex; i++) {
		count++;
		lua_pushnumber(L, count);
		PushCommandDesc(L, cmdDescs[i]);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", count);

	return 1;
}


int LuaSyncedRead::FindUnitCmdDesc(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to FindUnitCmdDesc()");
	}
	const int cmdID = (int)lua_tonumber(L, 2);

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

static inline bool IsFeatureVisible(const CFeature* feature)
{
	if (feature->allyteam < 0) {
		return true; // global feature has allyteam -1
	}
	if (readAllyTeam < 0) {
		return fullRead;
	}
	if ((readAllyTeam != feature->allyteam) &&
	    (!loshandler->InLos(feature->pos, readAllyTeam))) {
		return false;
	}
	return true;
}


static CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, index)) {
		luaL_error(L, "Incorrect arguments to %s(featureID)", caller);
	}
	const int featureID = (int)lua_tonumber(L, index);
	const CFeatureSet& fset = featureHandler->GetActiveFeatures();
	CFeatureSet::const_iterator it = fset.find(featureID);

	if (it == fset.end())
		return NULL;

	if (!IsFeatureVisible(*it)) {
		return NULL;
	}
	return *it;
}


int LuaSyncedRead::GetAllFeatures(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	int count = 0;
	const CFeatureSet& activeFeatures = featureHandler->GetActiveFeatures();
	CFeatureSet::const_iterator fit;
	if (fullRead) {
		for (fit = activeFeatures.begin(); fit != activeFeatures.end(); ++fit) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, (*fit)->id);
			lua_rawset(L, -3);
		}
	}
	else {
		for (fit = activeFeatures.begin(); fit != activeFeatures.end(); ++fit) {
			if (IsFeatureVisible(*fit)) {
				count++;
				lua_pushnumber(L, count);
				lua_pushnumber(L, (*fit)->id);
				lua_rawset(L, -3);
			}
		}
	}
	hs_n.PushNumber(L, count);
	return 1;
}


int LuaSyncedRead::GetFeatureList(lua_State* L)
{
	// NOTE: this is not really required now the all FeatureDefs are pre-loaded
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L); {
		const map<string, const FeatureDef*>& defs = featureHandler->GetFeatureDefs();
		map<string, const FeatureDef*>::const_iterator it;
		for (it = defs.begin(); it != defs.end(); ++it) {
			LuaPushNamedNumber(L, it->first, it->second->id);
		}
	}
	return 1;
}


int LuaSyncedRead::GetFeatureDefID(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	lua_pushnumber(L, feature->def->id);
	return 1;
}


int LuaSyncedRead::GetFeatureTeam(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
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
	if (feature == NULL) {
		return 0;
	}
	lua_pushnumber(L, feature->allyteam);
	return 1;
}


int LuaSyncedRead::GetFeatureHealth(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	lua_pushnumber(L, feature->health);
	lua_pushnumber(L, feature->def->maxHealth);
	lua_pushnumber(L, feature->resurrectProgress);
	return 3;
}


int LuaSyncedRead::GetFeatureHeight(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	lua_pushnumber(L, feature->height);
	return 1;
}


int LuaSyncedRead::GetFeatureRadius(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	lua_pushnumber(L, feature->radius);
	return 1;
}


int LuaSyncedRead::GetFeaturePosition(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	lua_pushnumber(L, feature->pos.x);
	lua_pushnumber(L, feature->pos.y);
	lua_pushnumber(L, feature->pos.z);
	return 3;
}


int LuaSyncedRead::GetFeatureDirection(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	const float3 dir = ::GetVectorFromHeading(feature->heading);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 3;
}


int LuaSyncedRead::GetFeatureHeading(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	lua_pushnumber(L, feature->heading);
	return 1;
}


int LuaSyncedRead::GetFeatureResources(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
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
	if (feature == NULL) {
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
	lua_pushstring(L, feature->createdFromUnit.c_str());
	lua_pushnumber(L, feature->buildFacing);
	return 2;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::GetGroundHeight(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetGroundHeight(x, z)");
	}
	const float x = lua_tonumber(L, 1);
	const float y = lua_tonumber(L, 2);
	// GetHeight2() does not clamp the value to (>= 0)
	lua_pushnumber(L, ground->GetHeight2(x, y));
	return 1;
}


int LuaSyncedRead::GetGroundOrigHeight(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetGroundOrigHeight(x, z)");
	}
	const float x = lua_tonumber(L, 1);
	const float y = lua_tonumber(L, 2);
	lua_pushnumber(L, ground->GetOrigHeight(x, y));
	return 1;
}


int LuaSyncedRead::GetGroundNormal(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetGroundNormal(x, z)");
	}
	const float x = lua_tonumber(L, 1);
	const float y = lua_tonumber(L, 2);
	const float3 normal = ground->GetSmoothNormal(x, y);
	lua_pushnumber(L, normal.x);
	lua_pushnumber(L, normal.y);
	lua_pushnumber(L, normal.z);
	return 3;
}


int LuaSyncedRead::GetGroundInfo(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetGroundInfo(x, z)");
	}

	const float x = lua_tonumber(L, 1);
	const float z = lua_tonumber(L, 2);

	const int ix = (int)(max(0.0f, min(float3::maxxpos, x)) / 16.0f);
	const int iz = (int)(max(0.0f, min(float3::maxzpos, z)) / 16.0f);

	const float metal = readmap->metalMap->getMetalAmount(ix, iz);

	const int maxIndex = (gs->hmapx * gs->hmapy) - 1;
	const int index = min(maxIndex, (gs->hmapx * iz) + ix);
	const int typeIndex = readmap->typemap[index];
	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[typeIndex];

	lua_pushstring(L, tt.name.c_str());
	lua_pushnumber(L, metal);
	lua_pushnumber(L, tt.hardness * mapDamage->mapHardness);
	lua_pushnumber(L, tt.tankSpeed);
	lua_pushnumber(L, tt.kbotSpeed);
	lua_pushnumber(L, tt.hoverSpeed);
	lua_pushnumber(L, tt.shipSpeed);
	lua_pushboolean(L, tt.receiveTracks);
	return 8;
}


// similar to ParseMapParams in LuaSyncedCtrl
static void ParseMapCoords(lua_State* L, const char* caller,
                           int& tx1, int& tz1, int& tx2, int& tz2)
{
	float fx1 = 0, fz1 = 0, fx2 = 0, fz2 = 0;

	const int args = lua_gettop(L); // number of arguments
	if (args == 2) {
		fx1 = fx2 = (float)luaL_checknumber(L, 1);
		fz1 = fz2 = (float)luaL_checknumber(L, 2);
	}
	else if (args == 4) {
		fx1 = (float)luaL_checknumber(L, 1);
		fz1 = (float)luaL_checknumber(L, 2);
		fx2 = (float)luaL_checknumber(L, 3);
		fz2 = (float)luaL_checknumber(L, 4);
	}
	else {
		luaL_error(L, "Incorrect arguments to %s()", caller);
	}

	// quantize and clamp
	tx1 = (int)max(0 , min(gs->mapx - 1, (int)(fx1 / SQUARE_SIZE)));
	tx2 = (int)max(0 , min(gs->mapx - 1, (int)(fx2 / SQUARE_SIZE)));
	tz1 = (int)max(0 , min(gs->mapy - 1, (int)(fz1 / SQUARE_SIZE)));
	tz2 = (int)max(0 , min(gs->mapy - 1, (int)(fz2 / SQUARE_SIZE)));

	return;
}


int LuaSyncedRead::GetGroundBlocked(lua_State* L)
{
	if ((readAllyTeam < 0) && !fullRead) {
		return 0;
	}

	int tx1, tx2, tz1, tz2;
	ParseMapCoords(L, __FUNCTION__, tx1, tz1, tx2, tz2);

	for(int z = tz1; z <= tz2; z++){
		for(int x = tx1; x <= tx2; x++){
			const CSolidObject* s = groundBlockingObjectMap->GroundBlocked((z * gs->mapx) + x);

			const CFeature* feature = dynamic_cast<const CFeature*>(s);
			if (feature) {
				if (fullRead || loshandler->InLos(feature, readAllyTeam)) {
					HSTR_PUSH(L, "feature");
					lua_pushnumber(L, feature->id);
					return 2;
				} else {
					continue;
				}
			}

			const CUnit* unit = dynamic_cast<const CUnit*>(s);
			if (unit) {
				if (fullRead || (unit->losStatus[readAllyTeam] & LOS_INLOS)) {
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
	lua_pushnumber(L, readmap->minheight);
	lua_pushnumber(L, readmap->maxheight);
	return 2;
}


/******************************************************************************/
/******************************************************************************/

static int ParseFacing(lua_State* L, const char* caller, int index)
{
	// FIXME -- duplicate in LuaSyncedCtrl.cpp
	if (lua_isnumber(L, index)) {
		return max(0, min(3, (int)lua_tonumber(L, index)));
	}
	else if (lua_isstring(L, index)) {
		const string dir = StringToLower(lua_tostring(L, index));
		if (dir == "s") { return 0; }
		if (dir == "e") { return 1; }
		if (dir == "n") { return 2; }
		if (dir == "w") { return 3; }
		luaL_error(L, "%s(): bad facing string", caller);
	}
	luaL_error(L, "%s(): bad facing parameter", caller);
	return 0;
}


int LuaSyncedRead::TestBuildOrder(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 5) || !lua_isnumber(L, 1) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L,
			"Incorrect arguments to TestBuildOrder(unitDefID, x, y, z, facing");
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	if ((unitDefID < 0) || (unitDefID > unitDefHandler->numUnitDefs)) {
		lua_pushboolean(L, 0);
		return 1;
	}
	const UnitDef* unitDef = unitDefHandler->GetUnitByID(unitDefID);
	if (unitDef == NULL) {
		lua_pushboolean(L, 0);
		return 1;
	}

	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	const int facing = ParseFacing(L, __FUNCTION__, 5);

	BuildInfo bi;
	bi.buildFacing = facing;
	bi.def = unitDef;
	bi.pos = pos;
	bi.pos = helper->Pos2BuildPos(bi);
	CFeature* feature;
	// negative allyTeam values have full visibility in TestUnitBuildSquare()
	int retval = uh->TestUnitBuildSquare(bi, feature, readAllyTeam);
	// 0 - blocked
	// 1 - mobile unit in the way
	// 2 - free  (or if feature is != 0 then with a
	//            blocking feature that can be reclaimed)
	if ((feature == NULL) ||
	    (!fullRead &&
	     (feature->allyteam >= 0) &&
	     (feature->allyteam != readAllyTeam) &&
	     !loshandler->InLos(feature->pos, readAllyTeam))) {
		lua_pushnumber(L, retval);
		return 1;
	}
	lua_pushnumber(L, retval);
	lua_pushnumber(L, feature->id);
	return 2;
}


int LuaSyncedRead::Pos2BuildPos(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) || !lua_isnumber(L, 1) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L,
			"Incorrect arguments to Pos2BuildPos(unitDefID, x, y, z)");
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));

	const float3 buildPos = helper->Pos2BuildPos(pos, ud);

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
		return readAllyTeam;
	}
	const bool isGaia = (readAllyTeam == gs->gaiaAllyTeamID);
	if (fullRead || isGaia) {
		const int at = luaL_checkint(L, arg);
		if (at >= gs->activeAllyTeams) {
			luaL_error(L, "Invalid allyTeam");
		}
		if (isGaia && (at >= 0) && (at != gs->gaiaAllyTeamID)) {
			luaL_error(L, "Invalid gaia access");
		}
		return at;
	}
	else if (readAllyTeam < 0) {
		luaL_error(L, "Invalid access");
	}
	return readAllyTeam;
}


int LuaSyncedRead::GetPositionLosState(lua_State* L)
{
	const float3 pos((float)luaL_checknumber(L, 1),
	                 (float)luaL_checknumber(L, 2),
	                 (float)luaL_checknumber(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);

	bool inLos    = false;
	bool inRadar  = false;

	if (allyTeamID >= 0) {
		inLos   = loshandler->InLos(pos, allyTeamID);
		inRadar = radarhandler->InRadar(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < gs->activeAllyTeams; at++) {
			if (loshandler->InLos(pos, at)) {
				inLos = true;
				break;
			}
		}
		for (int at = 0; at < gs->activeAllyTeams; at++) {
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
	const float3 pos((float)luaL_checknumber(L, 1),
	                 (float)luaL_checknumber(L, 2),
	                 (float)luaL_checknumber(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);

	bool state = false;
	if (allyTeamID >= 0) {
		state = loshandler->InLos(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < gs->activeAllyTeams; at++) {
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
	const float3 pos((float)luaL_checknumber(L, 1),
	                 (float)luaL_checknumber(L, 2),
	                 (float)luaL_checknumber(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);

	bool state = false;
	if (allyTeamID >= 0) {
		state = radarhandler->InRadar(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < gs->activeAllyTeams; at++) {
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
	const float3 pos((float)luaL_checknumber(L, 1),
	                 (float)luaL_checknumber(L, 2),
	                 (float)luaL_checknumber(L, 3));

	const int allyTeamID = GetEffectiveLosAllyTeam(L, 4);

	bool state = false;
	if (allyTeamID >= 0) {
		state = loshandler->InAirLos(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < gs->activeAllyTeams; at++) {
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
	const int unitDefID = (int)luaL_checknumber(L, 1);
	const float x     = (float)luaL_checknumber(L, 2);
	const float z     = (float)luaL_checknumber(L, 3);
	const float r     = (float)luaL_checknumber(L, 4);
	const int mx = (int)max(0 , min(gs->mapx - 1, (int)(x / SQUARE_SIZE)));
	const int mz = (int)max(0 , min(gs->mapy - 1, (int)(z / SQUARE_SIZE)));
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::GetUnitPieceMap(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;
	lua_newtable(L);
	for (int i = 0; i < localModel->numpieces; i++) {
		const LocalS3DO& lp = localModel->pieces[i];
		lua_pushstring(L, lp.name.c_str());
		lua_pushnumber(L, i + 1);
		lua_rawset(L, -3);
	}
	return 1;
}


int LuaSyncedRead::GetUnitPieceList(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;
	lua_newtable(L);
	for (int i = 0; i < localModel->numpieces; i++) {
		const LocalS3DO& lp = localModel->pieces[i];
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, lp.name.c_str());
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", localModel->numpieces);
	return 1;
}


template<class ModelType>
static int GetUnitPieceInfo(lua_State* L, const ModelType& op)
{
	lua_newtable(L);
	HSTR_PUSH_STRING(L, "name", op.name.c_str());

	HSTR_PUSH(L, "children");
	lua_newtable(L);
	for (int c = 0; c < (int)op.childs.size(); c++) {
		lua_pushnumber(L, c + 1);
		lua_pushstring(L, op.childs[c]->name.c_str());
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", op.childs.size());
	lua_rawset(L, -3);

	HSTR_PUSH(L, "isEmpty");
	lua_pushboolean(L, op.isEmpty);
	lua_rawset(L, -3);

	HSTR_PUSH(L, "min");
	lua_newtable(L); {
		lua_pushnumber(L, 1); lua_pushnumber(L, op.minx); lua_rawset(L, -3);
		lua_pushnumber(L, 2); lua_pushnumber(L, op.miny); lua_rawset(L, -3);
		lua_pushnumber(L, 3); lua_pushnumber(L, op.minz); lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	HSTR_PUSH(L, "max");
	lua_newtable(L); {
		lua_pushnumber(L, 1); lua_pushnumber(L, op.maxx); lua_rawset(L, -3);
		lua_pushnumber(L, 2); lua_pushnumber(L, op.maxy); lua_rawset(L, -3);
		lua_pushnumber(L, 3); lua_pushnumber(L, op.maxz); lua_rawset(L, -3);
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
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;

	const int piece = (int)luaL_checknumber(L, 2) - 1;
	if ((piece < 0) || (piece >= localModel->numpieces)) {
		return 0;
	}
	LocalS3DO& lp = localModel->pieces[piece];

	if (lp.originals3o) {
		const SS3O& op = *lp.originals3o;
		return ::GetUnitPieceInfo(L, op);
	}
	else if (lp.original3do) {
		const S3DO& op = *lp.original3do;
		return ::GetUnitPieceInfo(L, op);
	}

	return 0;
}


int LuaSyncedRead::GetUnitPiecePosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = (int)luaL_checknumber(L, 2) - 1;
	if ((piece < 0) || (piece >= localModel->numpieces)) {
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
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = (int)luaL_checknumber(L, 2) - 1;
	if ((piece < 0) || (piece >= localModel->numpieces)) {
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
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = (int)luaL_checknumber(L, 2) - 1;
	if ((piece < 0) || (piece >= localModel->numpieces)) {
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
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;
	if (localModel == NULL) {
		return 0;
	}
	const int piece = (int)luaL_checknumber(L, 2) - 1;
	if ((piece < 0) || (piece >= localModel->numpieces)) {
		return 0;
	}
	const CMatrix44f mat = unit->localmodel->GetRawPieceMatrix(piece);
	for (int m = 0; m < 16; m++) {
		lua_pushnumber(L, mat.m[m]);
	}
	return 16;
}


int LuaSyncedRead::GetUnitScriptPiece(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;
	if (localModel == NULL) {
		return 0;
	}

	if (!lua_isnumber(L, 2)) {
		// return the whole script->piece map
		lua_newtable(L);
		for (int sp = 0; sp < localModel->numpieces; sp++) {
			const int piece = localModel->scritoa[sp];
			if (piece != -1) {
				lua_pushnumber(L, sp);
				lua_pushnumber(L, piece + 1);
				lua_rawset(L, -3);
			}
		}
		return 1;
	}

	const int scriptPiece = (int)lua_tonumber(L, 2);
	const int piece = localModel->ScriptToArray(scriptPiece);
	if (piece < 0) {
		return 0;
	}

	lua_pushnumber(L, piece + 1);
	return 1;
}


int LuaSyncedRead::GetUnitScriptNames(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (unit->cob == NULL) {
		return 0;
	}
	const vector<struct PieceInfo>& pieces = unit->cob->pieces;

	lua_newtable(L);
	for (int sp = 0; sp < pieces.size(); sp++) {
		lua_pushstring(L, pieces[sp].name.c_str());
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
	if (unit->cob == NULL) {
		return 0;
	}
	const int varID = luaL_checkint(L, 2);
	if ((varID < 0) || (varID >= CCobInstance::UNIT_VAR_COUNT)) {
		return 0;
	}
	const int value = unit->cob->GetUnitVars()[varID];
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
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}
	if (!IsAlliedTeam(teamID)) {
		return 0;
	}
	const int varID = luaL_checkint(L, 2);
	if ((varID < 0) || (varID >= CCobInstance::TEAM_VAR_COUNT)) {
		return 0;
	}
	const int value = CCobInstance::GetTeamVars(teamID)[varID];
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
	if ((allyTeamID < 0) || (allyTeamID >= MAX_TEAMS)) {
		return 0;
	}
	if (!IsAlliedAllyTeam(allyTeamID)) {
		return 0;
	}
	const int varID = luaL_checkint(L, 2);
	if ((varID < 0) || (varID >= CCobInstance::ALLY_VAR_COUNT)) {
		return 0;
	}
	const int value = CCobInstance::GetAllyVars(allyTeamID)[varID];
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
	if ((varID < 0) || (varID >= CCobInstance::GLOBAL_VAR_COUNT)) {
		return 0;
	}
	const int value = CCobInstance::GetGlobalVars()[varID];
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
