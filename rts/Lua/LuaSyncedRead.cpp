#include "StdAfx.h"
// LuaSyncedRead.cpp: implementation of the CLuaSyncedRead class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaSyncedRead.h"
#include <set>
#include <list>
#include <cctype>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaRules.h"
#include "LuaUtils.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Game/command.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/Team.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
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
#include "Sim/Weapons/Weapon.h"
#include "System/myMath.h"
#include "System/LogOutput.h"
#include "System/TdfParser.h"
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
	REGISTER_LUA_CFUNC(IsEditDefsEnabled);
	REGISTER_LUA_CFUNC(AreHelperAIsEnabled);

	REGISTER_LUA_CFUNC(IsGameOver);

	REGISTER_LUA_CFUNC(GetGaiaTeamID);
	REGISTER_LUA_CFUNC(GetRulesInfoMap);

	REGISTER_LUA_CFUNC(GetGameSpeed);
	REGISTER_LUA_CFUNC(GetGameFrame);
	REGISTER_LUA_CFUNC(GetGameSeconds);

	REGISTER_LUA_CFUNC(GetGameRulesParam);  
	REGISTER_LUA_CFUNC(GetGameRulesParams);  

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

	REGISTER_LUA_CFUNC(GetUnitDefID);
	REGISTER_LUA_CFUNC(GetUnitTeam);
	REGISTER_LUA_CFUNC(GetUnitAllyTeam);
	REGISTER_LUA_CFUNC(GetUnitLineage);
	REGISTER_LUA_CFUNC(GetUnitHealth);
	REGISTER_LUA_CFUNC(GetUnitResources);
	REGISTER_LUA_CFUNC(GetUnitExperience);
	REGISTER_LUA_CFUNC(GetUnitStates);
	REGISTER_LUA_CFUNC(GetUnitSelfDTime);
	REGISTER_LUA_CFUNC(GetUnitStockpile);
	REGISTER_LUA_CFUNC(GetUnitRadius);
	REGISTER_LUA_CFUNC(GetUnitPosition);
	REGISTER_LUA_CFUNC(GetUnitBasePosition);
	REGISTER_LUA_CFUNC(GetUnitDirection);
	REGISTER_LUA_CFUNC(GetUnitHeading);
	REGISTER_LUA_CFUNC(GetUnitBuildFacing);
	REGISTER_LUA_CFUNC(GetUnitIsBuilding);
	REGISTER_LUA_CFUNC(GetUnitTransporter);
	REGISTER_LUA_CFUNC(GetUnitIsTransporting);
	REGISTER_LUA_CFUNC(GetUnitWeaponState);
	REGISTER_LUA_CFUNC(GetUnitLosState);
	REGISTER_LUA_CFUNC(GetUnitDefDimensions);

	REGISTER_LUA_CFUNC(GetUnitCommands);
	REGISTER_LUA_CFUNC(GetFactoryCounts);
	REGISTER_LUA_CFUNC(GetFactoryCommands);

	REGISTER_LUA_CFUNC(GetCommandQueue);
	REGISTER_LUA_CFUNC(GetFullBuildQueue);
	REGISTER_LUA_CFUNC(GetRealBuildQueue);

	REGISTER_LUA_CFUNC(GetUnitCmdDescs);
	REGISTER_LUA_CFUNC(GetUnitRulesParam);  
	REGISTER_LUA_CFUNC(GetUnitRulesParams);  

	REGISTER_LUA_CFUNC(GetFeatureList);
	REGISTER_LUA_CFUNC(GetFeatureDefID);
	REGISTER_LUA_CFUNC(GetFeatureTeam);
	REGISTER_LUA_CFUNC(GetFeatureAllyTeam);
	REGISTER_LUA_CFUNC(GetFeatureHealth);
	REGISTER_LUA_CFUNC(GetFeaturePosition);
	REGISTER_LUA_CFUNC(GetFeatureDirection);
	REGISTER_LUA_CFUNC(GetFeatureHeading);
	REGISTER_LUA_CFUNC(GetFeatureResources);

	REGISTER_LUA_CFUNC(GetGroundHeight);
	REGISTER_LUA_CFUNC(GetGroundNormal);
	REGISTER_LUA_CFUNC(GetGroundInfo);
	REGISTER_LUA_CFUNC(GetGroundBlocked);
	REGISTER_LUA_CFUNC(GetGroundExtremes);

	REGISTER_LUA_CFUNC(TestBuildOrder);
	REGISTER_LUA_CFUNC(Pos2BuildPos);
	REGISTER_LUA_CFUNC(GetPositionLosState);

	REGISTER_LUA_CFUNC(LoadTextVFS);
	REGISTER_LUA_CFUNC(FileExistsVFS);
	REGISTER_LUA_CFUNC(GetDirListVFS);

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
	return (unit->losStatus[readAllyTeam] & (LOS_INLOS | LOS_INRADAR));
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
	const int losStatus = unit->losStatus[readAllyTeam];
	const int prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
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
	int index = 0;
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -1)) {
			logOutput.Print("LUA: error parsing numeric array\n");
			lua_pop(L, 2); // pop the value and the key
			return -1;
		}
		if (index < size) {
			array[index] = (float)lua_tonumber(L, -1);
			index++;
		}
	}
	return index;
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

	if (lua_isnumber(L, index)) {
		pIndex = (int)lua_tonumber(L, index) - 1;
	}
	else if (lua_isstring(L, index)) {
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
	lua_pushnumber(L, wind.curWind.x);
	lua_pushnumber(L, wind.curWind.y);
	lua_pushnumber(L, wind.curWind.z);
	lua_pushnumber(L, wind.curStrength);
	lua_pushnumber(L, wind.curDir.x);
	lua_pushnumber(L, wind.curDir.y);
	lua_pushnumber(L, wind.curDir.z);
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
	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		luaL_error(L, "Incorrect arguments to GetPlayerList([teamID])");
	}

	int teamID = -1;
	if (args == 1) {
		teamID = (int)lua_tonumber(L, 1);
		if ((teamID < 0) || (teamID >= gs->activeTeams) ||
		    (gs->Team(teamID) == NULL)) {
			return 0;
		}
	}

	lua_newtable(L);
	int count = 0;
	for (int p = 0; p < gs->activePlayers; p++) {
		const CPlayer* player = gs->players[p];
		if (player == NULL) {
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
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetTeamInfo(teamID)");
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	bool isAiTeam = false;
	if ((globalAI != NULL) && (globalAI->ais[teamID] != NULL)) {
		isAiTeam = true;
	}

	lua_pushnumber(L, team->teamNum);
	lua_pushnumber(L, team->leader);
	lua_pushboolean(L, team->active);
	lua_pushboolean(L, team->isDead);
	lua_pushboolean(L, isAiTeam);
	lua_pushstring(L, team->side.c_str());
	lua_pushnumber(L, (float)team->color[0] / 255.0f);
	lua_pushnumber(L, (float)team->color[1] / 255.0f);
	lua_pushnumber(L, (float)team->color[2] / 255.0f);
	lua_pushnumber(L, (float)team->color[3] / 255.0f);

	return 10;
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
	const int teamID = team->teamNum;

	if (!IsAlliedTeam(teamID)) {
		return 0;
	}

	const vector<float>& params       = team->modParams;
	const map<string, int>& paramsMap = team->modParamsMap;

	return GetRulesParam(L, __FUNCTION__, 2, params, paramsMap);
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
	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		lua_pushstring(L, player->playerName.c_str());
	} else {
		HSTR_PUSH(L, "SYNCED_NONAME");
	}
	lua_pushboolean(L, player->active);
	lua_pushboolean(L, player->spectator);
	lua_pushnumber(L, player->team);
	lua_pushnumber(L, gs->AllyTeam(player->team));
	lua_pushnumber(L, player->ping);
	lua_pushnumber(L, player->cpuUsage);
	lua_pushstring(L, player->countryCode.c_str());

	return 8;
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
	if (!fullRead) {
		return 0;
	}
	lua_newtable(L);
	int count = 0;
	std::list<CUnit*>::const_iterator uit;
	for (uit = uh->activeUnits.begin(); uit != uh->activeUnits.end(); ++uit) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, (*uit)->id);
		lua_rawset(L, -3);
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

	const set<CUnit*>& units = team->units;
	set<CUnit*>::const_iterator uit;

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
	const set<CUnit*>& units = team->units;
	set<CUnit*>::const_iterator uit;
	
	// tally for allies
	if (IsAlliedTeam(teamID)) {
		for (uit = units.begin(); uit != units.end(); ++uit) {
			CUnit* unit = *uit;
			unitDefMap[unit->unitDef->id].push_back(unit);
		}
	}
	// tally for enemies
	else {
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

	// send the raw unitsType counts for allies
	if (IsAlliedTeam(teamID)) {
		lua_newtable(L);
		int defCount = 0;
		for (int udID = 0; udID < (unitDefHandler->numUnits + 1); udID++) {
			const int unitCount = uh->unitsType[teamID][udID];
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
	map<const UnitDef*, int> unitDefCounts;
	const set<CUnit*>& unitSet = team->units;
	set<CUnit*>::const_iterator uit;
	int unknownCount = 0;
	for (uit = unitSet.begin(); uit != unitSet.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitVisible(unit)) {
			if (!IsUnitTyped(unit)) {
				unknownCount++;
			} else {
				const UnitDef* ud = EffectiveUnitDef(unit);	
				map<const UnitDef*, int>::iterator mit = unitDefCounts.find(ud);
				if (mit == unitDefCounts.end()) {
					unitDefCounts[ud] = 1;
				} else {
					unitDefCounts[ud] = mit->second + 1;
				}
			}
		}
	}

	// push the counts
	lua_newtable(L);
	int defCount = 0;
	map<const UnitDef*, int>::const_iterator mit;
	for (mit = unitDefCounts.begin(); mit != unitDefCounts.end(); ++mit) {
		lua_pushnumber(L, mit->first->id);
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

	// parse the unitDefs
	set<const UnitDef*> defs;
	if (lua_isnumber(L, 2)) {
		const int unitDefID = (int)lua_tonumber(L, 2);
		const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
		if (ud != NULL) {
			defs.insert(ud);
		}
	} else if (lua_istable(L, 2)) {
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_isnumber(L, -1)) {
				const int unitDefID = (int)lua_tonumber(L, -1);
				const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
				if (ud != NULL) {
					defs.insert(ud);
				}
			}
		}
	} else {
		luaL_error(L, "Incorrect arguments to GetTeamUnitsByDefs()");
	}

	lua_newtable(L);
	int count = 0;

	set<const UnitDef*>::const_iterator udit;
	for (udit = defs.begin(); udit != defs.end(); ++udit) {
		const UnitDef* unitDef = *udit;
		const int unitCount = uh->unitsType[teamID][unitDef->id];
		if (unitCount <= 0) {
			continue; // don't bother
		}
		const set<CUnit*>& units = team->units;
		set<CUnit*>::const_iterator uit;

		if (IsAlliedTeam(teamID)) {
			for (uit = units.begin(); uit != units.end(); ++uit) {
				const CUnit* unit = *uit;
				if (unit->unitDef == unitDef) {
					count++;
					lua_pushnumber(L, count);
					lua_pushnumber(L, unit->id);
					lua_rawset(L, -3);
				}
			}
		} else {
			for (uit = units.begin(); uit != units.end(); ++uit) {
				const CUnit* unit = *uit;
				if (IsUnitTyped(unit) && (EffectiveUnitDef(unit) == unitDef)) {
					count++;
					lua_pushnumber(L, count);
					lua_pushnumber(L, unit->id);
					lua_rawset(L, -3);
				}
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

	// use the unitsType count for allies
	if (IsAlliedTeam(teamID)) {
		lua_pushnumber(L, uh->unitsType[teamID][unitDefID]);
		return 1;
	}

	// loop through the units for enemies
	int count = 0;
	const set<CUnit*>& units = team->units;
	set<CUnit*>::const_iterator uit;
	for (uit = units.begin(); uit != units.end(); ++uit) {
		const CUnit* unit = *uit;
		if (IsUnitTyped(unit) && (EffectiveUnitDef(unit) == unitDef)) {
			count++;
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

	// use the unitsType count for allies
	if (IsAlliedTeam(teamID)) {
		lua_pushnumber(L, team->units.size());
		return 1;
	}

	// loop through the units for enemies
	int count = 0;
	const set<CUnit*>& units = team->units;
	set<CUnit*>::const_iterator uit;
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
		luaL_error(L, "Incorrect arguments to GetUnitsInCylinder()");
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
		const set<CUnit*>& units = gs->Team(team)->units;
		set<CUnit*>::const_iterator it;

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

	const CMoveType* mt = unit->moveType;
  if (mt) {
  	const CTAAirMoveType* taAirMove = dynamic_cast<const CTAAirMoveType*>(mt);
  	if (taAirMove) {
			HSTR_PUSH_NUMBER(L, "autorepairlevel", taAirMove->repairBelowHealth);
		}
		else {
			const CAirMoveType* airMove = dynamic_cast<const CAirMoveType*>(mt);
			if (airMove) {
				HSTR_PUSH_BOOL  (L, "loopbackattack",  airMove->loopbackAttack);
				HSTR_PUSH_NUMBER(L, "autorepairlevel", airMove->repairBelowHealth);
			}
		}
	}
	
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
	lua_pushnumber(L, weapon->reloadStatus);
	lua_pushnumber(L, weapon->salvoLeft);
	lua_pushnumber(L, weapon->numStockpiled);
	return 5;
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
		const int args = lua_gettop(L); // number of arguments
		if ((args < 2) || !lua_isnumber(L, 2)) {
			return 0;
		}
		allyTeam = (int)lua_tonumber(L, 2);
	}
	if ((allyTeam < 0) || (allyTeam >= gs->activeAllyTeams)) {
		return 0;
	}
	const int losStatus = unit->losStatus[allyTeam];
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
	const S3DOModel* model;
	model = modelParser->Load3DO(ud->model.modelpath, 1.0f, 0 /* team */);
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
		const Command& cmd = *it;

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
				const std::map<int, std::string>& bOpts = builder_ud->buildOptions;
				std::map<int, std::string>::const_iterator it;
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

	lua_newtable(L);
	const int cmdDescCount = (int)cmdDescs.size();
	for (int i = 0; i < cmdDescCount; i++) {
		lua_pushnumber(L, i + 1);
		PushCommandDesc(L, cmdDescs[i]);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", cmdDescCount);

	return 1;
}


/******************************************************************************/
/******************************************************************************/

static CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, index)) {
		char buf[256];
		luaL_error(L, "Incorrect arguments to %s(featureID)", caller);
	}
	const int featureID = (int)lua_tonumber(L, index);
	if ((featureID < 0) || (featureID >= MAX_FEATURES)) {
		return NULL;
	}
	CFeature* feature = featureHandler->features[featureID];
	if (feature == NULL) {
		return NULL;
	}
	if (feature->allyteam < 0) {
		return feature; // global feature has allyteam -1
	}
	if (readAllyTeam < 0) {
		return fullRead ? feature : NULL;
	}
	if ((readAllyTeam != feature->allyteam) &&
	    (!loshandler->InLos(feature->pos, readAllyTeam))) {
		return NULL;
	}
	return feature;
}


int LuaSyncedRead::GetFeatureList(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L); {
		// push external feature names (-1 for IDs)
		TdfParser& wreckParser = featureHandler->wreckParser;
		const vector<string> sections = wreckParser.GetSectionList("");
		for (int i = 0; i < (int)sections.size(); i++) {
			LuaPushNamedNumber(L, sections[i], -1);
		}
		// push loaded feature names with their IDs
		// (this may overwrite some external entries)
		const map<string, FeatureDef*>& defs = featureHandler->featureDefs;
		map<string, FeatureDef*>::const_iterator it;
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


/******************************************************************************/
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
	const CReadMap::TerrainType& tt = readmap->terrainTypes[typeIndex];

	lua_pushstring(L, tt.name.c_str());
	lua_pushnumber(L, metal);
	lua_pushnumber(L, tt.hardness * mapDamage->mapHardness);
	lua_pushnumber(L, tt.tankSpeed);
	lua_pushnumber(L, tt.kbotSpeed);
	lua_pushnumber(L, tt.hoverSpeed);
	lua_pushnumber(L, tt.shipSpeed);
	return 7;
}


// similar to ParseMapParams in LuaSyncedCtrl
static void ParseMapCoords(lua_State* L, const char* caller,
                           int& tx1, int& tz1, int& tx2, int& tz2)
{
	float fx1, fz1, fx2, fz2;

	const int args = lua_gettop(L); // number of arguments
	if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			luaL_error(L, "Incorrect arguments to %s()", caller);
		}
		fx1 = fx2 = (float)lua_tonumber(L, 1);
		fz1 = fz2 = (float)lua_tonumber(L, 2);
	}
	else if (args == 4) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
		    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
			luaL_error(L, "Incorrect arguments to %s()", caller);
		}
		fx1 = (float)lua_tonumber(L, 1);
		fz1 = (float)lua_tonumber(L, 2);
		fx2 = (float)lua_tonumber(L, 3);
		fz2 = (float)lua_tonumber(L, 4);
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
			const CSolidObject* s = readmap->GroundBlocked((z * gs->mapx) + x);

			const CFeature* feature = dynamic_cast<const CFeature*>(s);
			if (feature) {
				if (fullRead || (feature->allyteam < 0) ||
				    loshandler->InLos(feature, readAllyTeam)) {
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
	if ((unitDefID < 0) || (unitDefID > unitDefHandler->numUnits)) {
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
	UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
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

int LuaSyncedRead::GetPositionLosState(lua_State* L)
{
	if (!fullRead && (readAllyTeam < 0)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to GetPositionLosState()");
	}
	const float3 pos((float)lua_tonumber(L, 1),
	                 (float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3));

	int allyTeamID = readAllyTeam;
	if (args == 3) {
		if (fullRead) {
			allyTeamID = -1; // -1 => check all allyTeams
		}
	}
	else if (lua_isnumber(L, 4)) {
		if (fullRead || (readAllyTeam == gs->gaiaAllyTeamID)) {
			allyTeamID = (int)lua_tonumber(L, 4);
			if (allyTeamID >= gs->activeAllyTeams) {
				luaL_error(L, "Bad allyTeamID in GetPositionLosState()");
			}
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to GetPositionLosState()");
	}

	// send the values
	bool inLos = false;
	bool radar = false;
	if (allyTeamID >= 0) {
		inLos = loshandler->InLos(pos, allyTeamID);
		radar = radarhandler->InRadar(pos, allyTeamID);
	}
	else {
		for (int at = 0; at < gs->activeAllyTeams; at++) {
			inLos = inLos || loshandler->InLos(pos, at);
			radar = radar || radarhandler->InRadar(pos, at);
		}
	}
	lua_pushboolean(L, inLos || radar);
	lua_pushboolean(L, inLos);
	lua_pushboolean(L, radar);

	return 3;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedRead::LoadTextVFS(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to LoadTextVFS()");
	}

	const string& filename = lua_tostring(L, 1);
	CFileHandler fh(filename, CFileHandler::OnlyArchiveFS);
	if (!fh.FileExists()) {
		return 0;
	}

	string buf;
	while (fh.Peek() != EOF) {
		int c;
		fh.Read(&c, 1);
		buf += (char)c;
	}
	lua_pushstring(L, buf.c_str());

	return 1;
}


int LuaSyncedRead::FileExistsVFS(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to FileExistsVFS()");
	}

	const string& filename = lua_tostring(L, 1);
	CFileHandler fh(filename, CFileHandler::OnlyArchiveFS);
	lua_pushboolean(L, fh.FileExists());
	return 1;
}


int LuaSyncedRead::GetDirListVFS(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetDirListVFS(\"dir\")");
	}
	const string dir = lua_tostring(L, 1);
	vector<string> filenames = hpiHandler->GetFilesInDir(dir);

	lua_newtable(L);
	for (int i = 0; i < filenames.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, filenames[i].c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, filenames.size());
	lua_rawset(L, -3);

	return 1;
}


/******************************************************************************/
/******************************************************************************/
