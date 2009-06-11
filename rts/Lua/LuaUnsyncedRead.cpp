#include "StdAfx.h"
// LuaRules.cpp: implementation of the CLuaRules class.
//
//////////////////////////////////////////////////////////////////////

#include "Game/SelectedUnits.h"

#include <set>
#include <list>
#include <cctype>
using namespace std;

#include "SDL_timer.h"
#include "SDL_keysym.h"
#include "SDL_mouse.h"

#include "mmgr.h"

#include "LuaUnsyncedRead.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "Game/Camera.h"
#include "Game/Camera/CameraController.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/PlayerHandler.h"
#include "Game/PlayerRoster.h"
#include "Game/CameraHandler.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "NetProtocol.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "FileSystem/FileSystem.h"
#include "Sound/Music.h"


extern boost::uint8_t *keys;

const int CMD_INDEX_OFFSET = 1; // starting index for command descriptions


/******************************************************************************/
/******************************************************************************/

static const bool& fullRead     = CLuaHandle::GetActiveFullRead();
static const int&  readAllyTeam = CLuaHandle::GetActiveReadAllyTeam();


/******************************************************************************/
/******************************************************************************/

bool LuaUnsyncedRead::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(IsReplay);
	REGISTER_LUA_CFUNC(GetModUICtrl);

	REGISTER_LUA_CFUNC(GetDrawFrame);
	REGISTER_LUA_CFUNC(GetFrameTimeOffset);
	REGISTER_LUA_CFUNC(GetLastUpdateSeconds);
	REGISTER_LUA_CFUNC(GetHasLag);

	REGISTER_LUA_CFUNC(GetViewGeometry);
	REGISTER_LUA_CFUNC(GetWindowGeometry);
	REGISTER_LUA_CFUNC(GetScreenGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapDualScreen);
	REGISTER_LUA_CFUNC(IsAboveMiniMap);

	REGISTER_LUA_CFUNC(IsAABBInView);
	REGISTER_LUA_CFUNC(IsSphereInView);

	REGISTER_LUA_CFUNC(IsUnitAllied);
	REGISTER_LUA_CFUNC(IsUnitInView);
	REGISTER_LUA_CFUNC(IsUnitVisible);
	REGISTER_LUA_CFUNC(IsUnitIcon);
	REGISTER_LUA_CFUNC(IsUnitSelected);

	REGISTER_LUA_CFUNC(GetUnitLuaDraw);
	REGISTER_LUA_CFUNC(GetUnitNoDraw);
	REGISTER_LUA_CFUNC(GetUnitNoMinimap);
	REGISTER_LUA_CFUNC(GetUnitNoSelect);

	REGISTER_LUA_CFUNC(GetUnitTransformMatrix);
	REGISTER_LUA_CFUNC(GetUnitViewPosition);

	REGISTER_LUA_CFUNC(GetVisibleUnits);

	REGISTER_LUA_CFUNC(GetPlayerRoster);

	REGISTER_LUA_CFUNC(GetTeamColor);
	REGISTER_LUA_CFUNC(GetTeamOrigColor);

	REGISTER_LUA_CFUNC(GetLocalPlayerID);
	REGISTER_LUA_CFUNC(GetLocalTeamID);
	REGISTER_LUA_CFUNC(GetLocalAllyTeamID);
	REGISTER_LUA_CFUNC(GetSpectatingState);

	REGISTER_LUA_CFUNC(GetSelectedUnits);
	REGISTER_LUA_CFUNC(GetSelectedUnitsSorted);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCounts);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCount);

	REGISTER_LUA_CFUNC(IsGUIHidden);
	REGISTER_LUA_CFUNC(HaveShadows);
	REGISTER_LUA_CFUNC(HaveAdvShading);
	REGISTER_LUA_CFUNC(GetWaterMode);
	REGISTER_LUA_CFUNC(GetMapDrawMode);

	REGISTER_LUA_CFUNC(GetCameraNames);
	REGISTER_LUA_CFUNC(GetCameraState);
	REGISTER_LUA_CFUNC(GetCameraPosition);
	REGISTER_LUA_CFUNC(GetCameraDirection);
	REGISTER_LUA_CFUNC(GetCameraFOV);
	REGISTER_LUA_CFUNC(GetCameraVectors);
	REGISTER_LUA_CFUNC(WorldToScreenCoords);
	REGISTER_LUA_CFUNC(TraceScreenRay);

	REGISTER_LUA_CFUNC(GetTimer);
	REGISTER_LUA_CFUNC(DiffTimers);

	REGISTER_LUA_CFUNC(GetSoundStreamTime);

	// moved from LuaUI

	REGISTER_LUA_CFUNC(GetFPS);

	REGISTER_LUA_CFUNC(GetActiveCommand);
	REGISTER_LUA_CFUNC(GetDefaultCommand);
	REGISTER_LUA_CFUNC(GetActiveCmdDescs);
	REGISTER_LUA_CFUNC(GetActiveCmdDesc);
	REGISTER_LUA_CFUNC(GetCmdDescIndex);

	REGISTER_LUA_CFUNC(GetBuildFacing);
	REGISTER_LUA_CFUNC(GetBuildSpacing);

	REGISTER_LUA_CFUNC(GetGatherMode);

	REGISTER_LUA_CFUNC(GetActivePage);

	REGISTER_LUA_CFUNC(GetMouseState);
	REGISTER_LUA_CFUNC(GetMouseCursor);
	REGISTER_LUA_CFUNC(GetMouseStartPosition);

	REGISTER_LUA_CFUNC(GetKeyState);
	REGISTER_LUA_CFUNC(GetModKeyState);
	REGISTER_LUA_CFUNC(GetPressedKeys);
	REGISTER_LUA_CFUNC(GetInvertQueueKey);

	REGISTER_LUA_CFUNC(GetKeyCode);
	REGISTER_LUA_CFUNC(GetKeySymbol);
	REGISTER_LUA_CFUNC(GetKeyBindings);
	REGISTER_LUA_CFUNC(GetActionHotKeys);

	REGISTER_LUA_CFUNC(GetLastMessagePositions);

	REGISTER_LUA_CFUNC(GetConsoleBuffer);
	REGISTER_LUA_CFUNC(GetCurrentTooltip);

	REGISTER_LUA_CFUNC(GetMyAllyTeamID);
	REGISTER_LUA_CFUNC(GetMyTeamID);
	REGISTER_LUA_CFUNC(GetMyPlayerID);

	REGISTER_LUA_CFUNC(GetGroupList);
	REGISTER_LUA_CFUNC(GetSelectedGroup);
	REGISTER_LUA_CFUNC(GetGroupAIName);
	REGISTER_LUA_CFUNC(GetGroupAIList);

	REGISTER_LUA_CFUNC(GetUnitGroup);

	REGISTER_LUA_CFUNC(GetGroupUnits);
	REGISTER_LUA_CFUNC(GetGroupUnitsSorted);
	REGISTER_LUA_CFUNC(GetGroupUnitsCounts);
	REGISTER_LUA_CFUNC(GetGroupUnitsCount);

	REGISTER_LUA_CFUNC(GetPlayerTraffic);

	REGISTER_LUA_CFUNC(GetDrawSelectionInfo);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
		return NULL;
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	if (readAllyTeam < 0) {
		return fullRead ? unit : NULL;
	}
	if ((unit->losStatus[readAllyTeam] & (LOS_INLOS | LOS_INRADAR)) == 0) {
		return NULL;
	}
	return unit;
}


static inline void CheckNoArgs(lua_State* L, const char* funcName)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "%s() takes no arguments", funcName);
	}
}


/******************************************************************************/
/******************************************************************************/
//
//  The call-outs
//

int LuaUnsyncedRead::IsReplay(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (gameSetup && gameSetup->hostDemo) {
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}


int LuaUnsyncedRead::GetModUICtrl(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, CLuaHandle::GetModUICtrl());
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetViewGeometry(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->viewSizeX);
	lua_pushnumber(L, gu->viewSizeY);
	lua_pushnumber(L, gu->viewPosX);
	lua_pushnumber(L, gu->viewPosY);
	return 4;
}


int LuaUnsyncedRead::GetWindowGeometry(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->winSizeX);
	lua_pushnumber(L, gu->winSizeY);
	lua_pushnumber(L, gu->winPosX);
	lua_pushnumber(L, gu->winPosY);
	return 4;
}


int LuaUnsyncedRead::GetScreenGeometry(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->screenSizeX);
	lua_pushnumber(L, gu->screenSizeY);
	lua_pushnumber(L, 0.0f);
	lua_pushnumber(L, 0.0f);
	return 4;
}


int LuaUnsyncedRead::GetMiniMapGeometry(lua_State* L)
{
	if (minimap == NULL) {
		return 0;
	}
	lua_pushnumber(L, minimap->GetPosX());
	lua_pushnumber(L, minimap->GetPosY());
	lua_pushnumber(L, minimap->GetSizeX());
	lua_pushnumber(L, minimap->GetSizeY());
	lua_pushboolean(L, minimap->GetMinimized());
	lua_pushboolean(L, minimap->GetMaximized());

	return 6;
}


int LuaUnsyncedRead::GetMiniMapDualScreen(lua_State* L)
{
	if (minimap == NULL) {
		return 0;
	}
	if (!gu->dualScreenMode) {
		lua_pushboolean(L, false);
	} else {
		if (gu->dualScreenMiniMapOnLeft) {
			lua_pushliteral(L, "left");
		} else {
			lua_pushliteral(L, "right");
		}
	}
	return 1;
}


int LuaUnsyncedRead::IsAboveMiniMap(lua_State* L)
{
	if (minimap == NULL) {
		return 0;
	}

	if (minimap->GetMinimized() || game->hideInterface) {
		return false;
	}

	const int x = luaL_checkint(L, 1) + gu->viewPosX;
	const int y = luaL_checkint(L, 2);

	const int x0 = minimap->GetPosX();
	const int y0 = minimap->GetPosY();
	const int x1 = x0 + minimap->GetSizeX();
	const int y1 = y0 + minimap->GetSizeY();

	lua_pushboolean(L, (x >= x0) && (x < x1) &&
	                   (y >= y0) && (y < y1));

	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetDrawFrame(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->drawFrame & 0xFFFF);
	lua_pushnumber(L, (gu->drawFrame >> 16) & 0xFFFF);
	return 2;
}


int LuaUnsyncedRead::GetFrameTimeOffset(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->timeOffset);
	return 1;
}


int LuaUnsyncedRead::GetLastUpdateSeconds(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, game->updateDeltaSeconds);
	return 1;
}

int LuaUnsyncedRead::GetHasLag(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, game ? game->HasLag() : false);
	return 1;
}

int LuaUnsyncedRead::IsAABBInView(lua_State* L)
{
	float3 mins = float3(luaL_checkfloat(L, 1),
	                     luaL_checkfloat(L, 2),
	                     luaL_checkfloat(L, 3));
	float3 maxs = float3(luaL_checkfloat(L, 4),
	                     luaL_checkfloat(L, 5),
	                     luaL_checkfloat(L, 6));
	lua_pushboolean(L, camera->InView(mins, maxs));
	return 1;
}


int LuaUnsyncedRead::IsSphereInView(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));
	const float radius = lua_israwnumber(L, 4) ? lua_tofloat(L, 4) : 0.0f;

	lua_pushboolean(L, camera->InView(pos, radius));
	return 1;
}


int LuaUnsyncedRead::IsUnitAllied(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (readAllyTeam < 0) {
		lua_pushboolean(L, fullRead);
	} else {
		lua_pushboolean(L, (unit->allyteam == readAllyTeam));
	}
	return 1;
}


int LuaUnsyncedRead::IsUnitInView(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, camera->InView(unit->midPos, unit->radius));
	return 1;
}


static bool UnitIsIcon(const CUnit* unit)
{
	const float sqDist = (unit->pos - camera->pos).SqLength();
	const float iconLength = unitDrawer->iconLength;
	const float iconDistSqrMult = unit->unitDef->iconType->GetDistanceSqr();
	const float realIconLength = iconLength * iconDistSqrMult;
	return (sqDist > realIconLength);
}


int LuaUnsyncedRead::IsUnitVisible(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float radius = luaL_optnumber(L, 2, unit->radius);
	const bool checkIcon = lua_toboolean(L, 3);

	if (readAllyTeam < 0) {
		if (!fullRead) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L,
				(!checkIcon || !UnitIsIcon(unit)) &&
				camera->InView(unit->midPos, radius));
		}
	}
	else {
		if ((unit->losStatus[readAllyTeam] & LOS_INLOS) == 0) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L,
				(!checkIcon || !UnitIsIcon(unit)) &&
				camera->InView(unit->midPos, radius));
		}
	}
	return 1;
}


int LuaUnsyncedRead::IsUnitIcon(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, UnitIsIcon(unit));
	return 1;
}


int LuaUnsyncedRead::IsUnitSelected(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // IsUnitSelected

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const CUnitSet& selUnits = selectedUnits.selectedUnits;
	lua_pushboolean(L, selUnits.find(unit) != selUnits.end());
	return 1;
}


int LuaUnsyncedRead::GetUnitLuaDraw(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->luaDraw);
	return 1;
}


int LuaUnsyncedRead::GetUnitNoDraw(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->noDraw);
	return 1;
}


int LuaUnsyncedRead::GetUnitNoMinimap(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->noMinimap);
	return 1;
}


int LuaUnsyncedRead::GetUnitNoSelect(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->noSelect);
	return 1;
}


int LuaUnsyncedRead::GetUnitTransformMatrix(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	CMatrix44f m = unit->GetTransformMatrix(false, false);

	if ((lua_isboolean(L, 2) && lua_toboolean(L, 2))) {
		m = m.Invert();
	}

	for (int i = 0; i < 16; i += 4) {
		lua_pushnumber(L, m[i    ]);
		lua_pushnumber(L, m[i + 1]);
		lua_pushnumber(L, m[i + 2]);
		lua_pushnumber(L, m[i + 3]);
	}

	return 16;
}

int LuaUnsyncedRead::GetUnitViewPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const bool midPos = (lua_isboolean(L, 2) && lua_toboolean(L, 2));

	float3 pos = midPos ? (float3)unit->midPos : (float3)unit->pos;
	CTransportUnit *trans=unit->GetTransporter();
	if (trans == NULL) {
		pos += (unit->speed * gu->timeOffset);
	} else {
		pos += (trans->speed * gu->timeOffset);
	}

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

// FIXME -- copied from LuaSyncedRead.cpp, commonize
enum UnitAllegiance {
	AllUnits   = -1,
	MyUnits    = -2,
	AllyUnits  = -3,
	EnemyUnits = -4
};


int LuaUnsyncedRead::GetVisibleUnits(lua_State* L)
{
	// arg 1 - teamID
	int teamID = luaL_optint(L, 1, -1);
	if (teamID == MyUnits) {
		const int scriptTeamID = CLuaHandle::GetActiveHandle()->GetReadTeam();
		if (scriptTeamID >= 0) {
			teamID = scriptTeamID;
		} else {
			teamID = AllUnits;
		}
	}
	int allyTeamID = readAllyTeam;
	if (teamID >= 0) {
		allyTeamID = teamHandler->AllyTeam(teamID);
	}
	if (allyTeamID < 0) {
		if (!fullRead) {
			return 0;
		}
	}

	// arg 2 - unit radius
	bool fixedRadius = false;
	float radius = 30.0f; // value from UnitDrawer.cpp
	if (lua_israwnumber(L, 2)) {
		radius = lua_tofloat(L, 2);
		if (radius < 0.0f) {
			fixedRadius = true;
			radius = -radius;
		}
	}

	// arg 3 - noIcons
	const bool noIcons = lua_isboolean(L, 3) && !lua_toboolean(L, 3);

	// setup the list of unit sets
	vector<const CUnitSet*> unitSets;
	if (teamID >= 0) {
		unitSets.push_back(&teamHandler->Team(teamID)->units);
	}
	else {
		for (int t = 0; t < teamHandler->ActiveTeams(); t++) {
			if ((teamID == AllUnits) ||
			    ((teamID == AllyUnits)  && (allyTeamID == teamHandler->AllyTeam(t))) ||
			    ((teamID == EnemyUnits) && (allyTeamID != teamHandler->AllyTeam(t)))) {
				unitSets.push_back(&teamHandler->Team(t)->units);
			}
		}
	}

	const float iconLength = unitDrawer->iconLength;

	int count = 0;
	lua_newtable(L);

	vector<const CUnitSet*>::const_iterator setIt;
	for (setIt = unitSets.begin(); setIt != unitSets.end(); ++setIt) {
		const CUnitSet* unitSet = *setIt;

		CUnitSet::const_iterator unitIt;
		for (unitIt = unitSet->begin(); unitIt != unitSet->end(); ++unitIt) {
			const CUnit* unit = *unitIt;

			if (unit->noDraw) {
				continue;
			}

			if (allyTeamID >= 0) {
				if (!(unit->losStatus[allyTeamID] & LOS_INLOS)) {
					continue;
				}
			}

			if (noIcons) {
				const float sqDist = (unit->pos - camera->pos).SqLength();
				const float iconDistSqrMult = unit->unitDef->iconType->GetDistanceSqr();
				const float realIconLength = iconLength * iconDistSqrMult;
				if (sqDist > realIconLength) {
					continue;
				}
			}

			const float testRadius = fixedRadius ? radius : (unit->radius + radius);
			if (!camera->InView(unit->midPos, testRadius)) {
				continue;
			}

			// add the unit
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, unit->id);
			lua_rawset(L, -3);
		}
	}

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetLocalPlayerID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myPlayerNum);
	return 1;
}


int LuaUnsyncedRead::GetLocalTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myTeam);
	return 1;
}


int LuaUnsyncedRead::GetLocalAllyTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myAllyTeam);
	return 1;
}


int LuaUnsyncedRead::GetSpectatingState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, gu->spectating);
	lua_pushboolean(L, gu->spectatingFullView);
	lua_pushboolean(L, gu->spectatingFullSelect);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetSelectedUnits(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // GetSelectedUnits

	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	int count = 0;
	const CUnitSet& selUnits = selectedUnits.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, (*it)->id);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", count);
	return 1;
}


int LuaUnsyncedRead::GetSelectedUnitsSorted(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // GetSelectedUnitsSorted

	CheckNoArgs(L, __FUNCTION__);

	map<int, vector<CUnit*> > unitDefMap;
	const CUnitSet& selUnits = selectedUnits.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		CUnit* unit = *it;
		unitDefMap[unit->unitDef->id].push_back(unit);
	}

	lua_newtable(L);
	map<int, vector<CUnit*> >::const_iterator mit;
	for (mit = unitDefMap.begin(); mit != unitDefMap.end(); mit++) {
		lua_pushnumber(L, mit->first); // push the UnitDef index
		lua_newtable(L); {
			const vector<CUnit*>& v = mit->second;
			for (int i = 0; i < (int)v.size(); i++) {
				CUnit* unit = v[i];
				lua_pushnumber(L, i + 1);
				lua_pushnumber(L, unit->id);
				lua_rawset(L, -3);
			}
			HSTR_PUSH_NUMBER(L, "n", v.size());
		}
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", unitDefMap.size());
	return 1;
}


int LuaUnsyncedRead::GetSelectedUnitsCounts(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // GetSelectedUnitsCounts

	CheckNoArgs(L, __FUNCTION__);

	// tally the types
	map<int, int> countMap;
	const CUnitSet& selUnits = selectedUnits.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		CUnit* unit = *it;
		map<int, int>::iterator mit = countMap.find(unit->unitDef->id);
		if (mit == countMap.end()) {
			countMap[unit->unitDef->id] = 1;
		} else {
			mit->second++;
		}
	}

	lua_newtable(L);
	map<int, int>::const_iterator mit;
	for (mit = countMap.begin(); mit != countMap.end(); mit++) {
		lua_pushnumber(L, mit->first);  // push the UnitDef index
		lua_pushnumber(L, mit->second); // push the UnitDef unit count
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", countMap.size());
	return 1;
}


int LuaUnsyncedRead::GetSelectedUnitsCount(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, selectedUnits.selectedUnits.size());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::IsGUIHidden(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (game == NULL) {
		return 0;
	}
	lua_pushboolean(L, game->hideInterface);
	return 1;
}


int LuaUnsyncedRead::HaveShadows(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (shadowHandler == NULL) {
		return 0;
	}
	lua_pushboolean(L, shadowHandler->drawShadows);
	return 1;
}


int LuaUnsyncedRead::HaveAdvShading(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (unitDrawer == NULL) {
		return 0;
	}
	lua_pushboolean(L, unitDrawer->advShading);
	return 1;
}


int LuaUnsyncedRead::GetWaterMode(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (water == NULL) {
		return 0;
	}
	const int mode = water->GetID();
	const char* modeName;
	switch (mode) {
		case 0:  { modeName = "basic";      break; }
		case 1:  { modeName = "reflective"; break; }
		case 2:  { modeName = "dynamic";    break; }
		case 3:  { modeName = "refractive"; break; }
		default: { modeName = "unknown";    break; }
	}
	lua_pushnumber(L, mode);
	lua_pushstring(L, modeName);
	return 2;
}


int LuaUnsyncedRead::GetMapDrawMode(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	const CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	switch (gd->drawMode) {
		case CBaseGroundDrawer::drawNormal: { HSTR_PUSH(L, "normal"); break; }
		case CBaseGroundDrawer::drawHeight: { HSTR_PUSH(L, "height"); break; }
		case CBaseGroundDrawer::drawMetal:  { HSTR_PUSH(L, "metal");  break; }
		case CBaseGroundDrawer::drawPath:   { HSTR_PUSH(L, "path");   break; }
		case CBaseGroundDrawer::drawLos:    { HSTR_PUSH(L, "los");    break; }
	}
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetCameraNames(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);

	lua_newtable(L);
	const std::vector<CCameraController*>& cc = camHandler->GetAvailableControllers();
	for (size_t i = 0; i < cc.size(); ++i) {
		lua_pushstring(L, cc[i]->GetName().c_str());
		lua_pushnumber(L, i);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetCameraState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);

	lua_newtable(L);

	lua_pushstring(L, "name");
	lua_pushstring(L, camHandler->GetCurrentControllerName().c_str());
	lua_rawset(L, -3);

	CCameraController::StateMap camState;
	CCameraController::StateMap::const_iterator it;
	camHandler->GetState(camState);
	for (it = camState.begin(); it != camState.end(); ++it) {
		lua_pushstring(L, it->first.c_str());
		lua_pushnumber(L, it->second);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetCameraPosition(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, camera->pos.x);
	lua_pushnumber(L, camera->pos.y);
	lua_pushnumber(L, camera->pos.z);
	return 3;
}


int LuaUnsyncedRead::GetCameraDirection(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, camera->forward.x);
	lua_pushnumber(L, camera->forward.y);
	lua_pushnumber(L, camera->forward.z);
	return 3;
}


int LuaUnsyncedRead::GetCameraFOV(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, camera->GetFov());
	return 1;
}


int LuaUnsyncedRead::GetCameraVectors(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);

	const CCamera* cam = camera;

#define PACK_CAMERA_VECTOR(n) \
	HSTR_PUSH(L, #n);           \
	lua_newtable(L);            \
	lua_pushnumber(L, 1); lua_pushnumber(L, cam-> n .x); lua_rawset(L, -3); \
	lua_pushnumber(L, 2); lua_pushnumber(L, cam-> n .y); lua_rawset(L, -3); \
	lua_pushnumber(L, 3); lua_pushnumber(L, cam-> n .z); lua_rawset(L, -3); \
	lua_rawset(L, -3)

	lua_newtable(L);
	PACK_CAMERA_VECTOR(forward);
	PACK_CAMERA_VECTOR(up);
	PACK_CAMERA_VECTOR(right);
	PACK_CAMERA_VECTOR(top);
	PACK_CAMERA_VECTOR(bottom);
	PACK_CAMERA_VECTOR(leftside);
	PACK_CAMERA_VECTOR(rightside);

	return 1;
}


int LuaUnsyncedRead::WorldToScreenCoords(lua_State* L)
{
	const float3 worldPos(luaL_checkfloat(L, 1),
	                      luaL_checkfloat(L, 2),
	                      luaL_checkfloat(L, 3));
	const float3 winPos = camera->CalcWindowCoordinates(worldPos);
	lua_pushnumber(L, winPos.x);
	lua_pushnumber(L, winPos.y);
	lua_pushnumber(L, winPos.z);
	return 3;
}


int LuaUnsyncedRead::TraceScreenRay(lua_State* L)
{
	// window coordinates
	const int mx = luaL_checkint(L, 1);
	const int my = luaL_checkint(L, 2);
	const bool onlyCoords = (lua_isboolean(L, 3) && lua_toboolean(L, 3));
	const bool useMiniMap = (lua_isboolean(L, 4) && lua_toboolean(L, 4));

	const int wx = mx + gu->viewPosX;
	const int wy = gu->viewSizeY - 1 - my - gu->viewPosY;

	if (useMiniMap && (minimap != NULL) && !minimap->GetMinimized()) {
		const int px = minimap->GetPosX() - gu->viewPosX; // for left dualscreen
		const int py = minimap->GetPosY();
		const int sx = minimap->GetSizeX();
		const int sy = minimap->GetSizeY();
		if ((mx >= px) && (mx < (px + sx)) &&
		    (my >= py) && (my < (py + sy))) {
			const float3 pos = minimap->GetMapPosition(wx, wy);
			if (!onlyCoords) {
				const CUnit* unit = minimap->GetSelectUnit(pos);
				if (unit != NULL) {
					lua_pushstring(L, "unit");
					lua_pushnumber(L, unit->id);
					return 2;
				}
			}
			const float posY = ground->GetHeight2(pos.x, pos.z);
			lua_pushstring(L, "ground");
			lua_newtable(L);
			lua_pushnumber(L, 1); lua_pushnumber(L, pos.x); lua_rawset(L, -3);
			lua_pushnumber(L, 2); lua_pushnumber(L, posY);  lua_rawset(L, -3);
			lua_pushnumber(L, 3); lua_pushnumber(L, pos.z); lua_rawset(L, -3);
			return 2;
		}
	}

	if ((mx < 0) || (mx >= gu->viewSizeX) ||
	    (my < 0) || (my >= gu->viewSizeY)) {
		return 0;
	}

	CUnit* unit = NULL;
	CFeature* feature = NULL;
	const float range = gu->viewRange * 1.4f;
	const float3& pos = camera->pos;
	const float3 dir = camera->CalcPixelDir(wx, wy);


// FIXME	const int origAllyTeam = gu->myAllyTeam;
//	gu->myAllyTeam = readAllyTeam;
	const float udist = helper->GuiTraceRay(pos, dir, range, unit, true);
	const float fdist = helper->GuiTraceRayFeature(pos, dir, range, feature);
//	gu->myAllyTeam = origAllyTeam;

	const float badRange = (range - 300.0f);
	if ((udist > badRange) && (fdist > badRange) && (unit == NULL)) {
		return 0;
	}

	if (!onlyCoords) {
		if (udist > fdist) {
			unit = NULL;
		} else {
			feature = NULL;
		}

		if (unit != NULL) {
			lua_pushstring(L, "unit");
			lua_pushnumber(L, unit->id);
			return 2;
		}

		if (feature != NULL) {
			lua_pushstring(L, "feature");
			lua_pushnumber(L, feature->id);
			return 2;
		}
	}

	const float3 groundPos = pos + (dir * udist);
	lua_pushstring(L, "ground");
	lua_newtable(L);
	lua_pushnumber(L, 1); lua_pushnumber(L, groundPos.x); lua_rawset(L, -3);
	lua_pushnumber(L, 2); lua_pushnumber(L, groundPos.y); lua_rawset(L, -3);
	lua_pushnumber(L, 3); lua_pushnumber(L, groundPos.z); lua_rawset(L, -3);

	return 2;
}


/******************************************************************************/

static void AddPlayerToRoster(lua_State* L, int playerID)
{
#define PUSH_ROSTER_ENTRY(type, val) \
	lua_pushnumber(L, index); index++; \
	lua_push ## type(L, val); lua_rawset(L, -3);

	const CPlayer* p = playerHandler->Player(playerID);
	int index = 1;
	lua_newtable(L);
	PUSH_ROSTER_ENTRY(string, p->name.c_str());
	PUSH_ROSTER_ENTRY(number, playerID);
	PUSH_ROSTER_ENTRY(number, p->team);
	PUSH_ROSTER_ENTRY(number, teamHandler->AllyTeam(p->team));
	PUSH_ROSTER_ENTRY(number, p->spectator);
	PUSH_ROSTER_ENTRY(number, p->cpuUsage);
	const float pingScale = (GAME_SPEED * gs->speedFactor);
	const float pingSecs = float(p->ping - 1) / pingScale;
	PUSH_ROSTER_ENTRY(number, pingSecs);
}


int LuaUnsyncedRead::GetPlayerRoster(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		luaL_error(L, "Incorrect arguments to GetPlayerRoster([type])");
	}

	const PlayerRoster::SortType oldSort = playerRoster.GetSortType();

	if (args == 1) {
		const int type = lua_toint(L, 1);
		playerRoster.SetSortTypeByCode((PlayerRoster::SortType)type);
	}

	int count;
	const std::vector<int>& players = playerRoster.GetIndices(&count);

	playerRoster.SetSortTypeByCode(oldSort); // revert

	lua_newtable(L);
	for (int i = 0; i < count; i++) {
		lua_pushnumber(L, i + 1);
		AddPlayerToRoster(L, players[i]);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	return 1;
}


int LuaUnsyncedRead::GetTeamColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	lua_pushnumber(L, (float)team->color[0] / 255.0f);
	lua_pushnumber(L, (float)team->color[1] / 255.0f);
	lua_pushnumber(L, (float)team->color[2] / 255.0f);
	lua_pushnumber(L, (float)team->color[3] / 255.0f);

	return 4;
}


int LuaUnsyncedRead::GetTeamOrigColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	lua_pushnumber(L, (float)team->origColor[0] / 255.0f);
	lua_pushnumber(L, (float)team->origColor[1] / 255.0f);
	lua_pushnumber(L, (float)team->origColor[2] / 255.0f);
	lua_pushnumber(L, (float)team->origColor[3] / 255.0f);

	return 4;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetTimer(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushlightuserdata(L, (void*)SDL_GetTicks());
	return 1;
}


int LuaUnsyncedRead::DiffTimers(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isuserdata(L, 1) || !lua_isuserdata(L, 2)) {
		luaL_error(L, "Incorrect arguments to DiffTimers()");
	}
	const void* p1 = lua_touserdata(L, 1);
	const void* p2 = lua_touserdata(L, 2);
	const Uint32 t1 = *((const Uint32*)&p1);
	const Uint32 t2 = *((const Uint32*)&p2);
	const Uint32 diffTime = (t1 - t2);
	lua_pushnumber(L, (float)diffTime * 0.001f); // return seconds
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetSoundStreamTime(lua_State* L)
{
	lua_pushnumber(L, Channels::BGMusic.GetPlayTime());
	lua_pushnumber(L, Channels::BGMusic.GetTime());
	return 2;
}


/******************************************************************************/
/******************************************************************************/
//
// moved from LuaUI
//
/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetFPS(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (game) {
		lua_pushnumber(L, game->fps);
	} else {
		lua_pushnumber(L, 0);
	}
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetActiveCommand(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetActiveCommand

	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	const int inCommand = guihandler->inCommand;
	lua_pushnumber(L, inCommand + CMD_INDEX_OFFSET);
	if ((inCommand < 0) || (inCommand >= cmdDescCount)) {
		return 1;
	}
	lua_pushnumber(L, cmdDescs[inCommand].id);
	lua_pushnumber(L, cmdDescs[inCommand].type);
	lua_pushstring(L, cmdDescs[inCommand].name.c_str());
	return 4;
}


int LuaUnsyncedRead::GetDefaultCommand(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetDefaultCommand

	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	const int defCmd = guihandler->GetDefaultCommand(mouse->lastx, mouse->lasty);
	lua_pushnumber(L, defCmd + CMD_INDEX_OFFSET);
	if ((defCmd < 0) || (defCmd >= cmdDescCount)) {
		return 1;
	}
	lua_pushnumber(L, cmdDescs[defCmd].id);
	lua_pushnumber(L, cmdDescs[defCmd].type);
	lua_pushstring(L, cmdDescs[defCmd].name.c_str());
	return 4;
}


// FIXME: duplicated in LuaSyncedRead.cpp
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
	HSTR_PUSH_BOOL(L,   "hidden",      cd.hidden);
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


int LuaUnsyncedRead::GetActiveCmdDescs(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetActiveCmdDescs

	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();
	for (int i = 0; i < cmdDescCount; i++) {
		lua_pushnumber(L, i + CMD_INDEX_OFFSET);
		PushCommandDesc(L, cmdDescs[i]);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", cmdDescCount);
	return 1;
}


int LuaUnsyncedRead::GetActiveCmdDesc(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetActiveCmdDesc

	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetActiveCmdDesc()");
	}
	const int cmdIndex = lua_toint(L, 1) - CMD_INDEX_OFFSET;

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();
	if ((cmdIndex < 0) || (cmdIndex >= cmdDescCount)) {
		return 0;
	}
	PushCommandDesc(L, cmdDescs[cmdIndex]);
	return 1;
}


int LuaUnsyncedRead::GetCmdDescIndex(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetCmdDescIndex

	if (guihandler == NULL) {
		return 0;
	}
	const int cmdId = luaL_checkint(L, 1);

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();
	for (int i = 0; i < cmdDescCount; i++) {
		if (cmdId == cmdDescs[i].id) {
			lua_pushnumber(L, i + CMD_INDEX_OFFSET);
			return 1;
		}
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedRead::GetBuildFacing(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, guihandler->buildFacing);
	return 1;
}


int LuaUnsyncedRead::GetBuildSpacing(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, guihandler->buildSpacing);
	return 1;
}


int LuaUnsyncedRead::GetGatherMode(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, guihandler->GetGatherMode());
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetActivePage(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	lua_pushnumber(L, guihandler->GetActivePage());
	lua_pushnumber(L, guihandler->GetMaxPage());
	return 2;
}


/******************************************************************************/

int LuaUnsyncedRead::GetMouseState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, mouse->lastx - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - mouse->lasty - 1);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_LEFT].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_MIDDLE].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_RIGHT].pressed);
	return 5;
}


int LuaUnsyncedRead::GetMouseCursor(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushstring(L, mouse->cursorText.c_str());
	lua_pushnumber(L, mouse->cursorScale);
	return 2;
}


int LuaUnsyncedRead::GetMouseStartPosition(lua_State* L)
{
	if (mouse == NULL) {
		return 0;
	}
	const int button = luaL_checkint(L, 1);
	if ((button <= 0) || (button > NUM_BUTTONS)) {
		return 0;
	}
	const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];
	lua_pushnumber(L, bp.x);
	lua_pushnumber(L, bp.y);
	lua_pushnumber(L, bp.camPos.x);
	lua_pushnumber(L, bp.camPos.y);
	lua_pushnumber(L, bp.camPos.z);
	lua_pushnumber(L, bp.dir.x);
	lua_pushnumber(L, bp.dir.y);
	lua_pushnumber(L, bp.dir.z);
	return 8;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetKeyState(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeyState(keycode)");
	}
	const int key = lua_toint(L, 1);
	if ((key < 0) || (key >= SDLK_LAST)) {
		lua_pushboolean(L, 0);
	} else {
		lua_pushboolean(L, keys[key]);
	}
	return 1;
}


int LuaUnsyncedRead::GetModKeyState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, keys[SDLK_LALT]);
	lua_pushboolean(L, keys[SDLK_LCTRL]);
	lua_pushboolean(L, keys[SDLK_LMETA]);
	lua_pushboolean(L, keys[SDLK_LSHIFT]);
	return 4;
}


int LuaUnsyncedRead::GetPressedKeys(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	int count = 0;
	for (int i = 0; i < SDLK_LAST; i++) {
		if (keys[i]) {
			lua_pushnumber(L, i);
			lua_pushboolean(L, 1);
			lua_rawset(L, -3);
			count++;
		}
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
	return 1;
}


int LuaUnsyncedRead::GetInvertQueueKey(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (guihandler == NULL) {
		return 0;
	}
	lua_pushboolean(L, guihandler->GetInvertQueueKey());
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetLastMessagePositions(lua_State* L)
{
	CInfoConsole* ic = game->infoConsole;
	if (ic == NULL) {
		return 0;
	}

	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	for (int i=1; i<=ic->GetMsgPosCount(); i++) {
		lua_pushnumber(L, i);
		lua_newtable(L); {
			const float3 msgpos = ic->GetMsgPos();
			lua_pushnumber(L, 1);
			lua_pushnumber(L, msgpos.x);
			lua_rawset(L, -3);
			lua_pushnumber(L, 2);
			lua_pushnumber(L, msgpos.y);
			lua_rawset(L, -3);
			lua_pushnumber(L, 3);
			lua_pushnumber(L, msgpos.z);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetConsoleBuffer(lua_State* L)
{
	CInfoConsole* ic = game->infoConsole;
	if (ic == NULL) {
		return true;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		luaL_error(L, "Incorrect arguments to GetConsoleBuffer([count])");
	}

	deque<CInfoConsole::RawLine> lines;
	ic->GetRawLines(lines);
	const int lineCount = (int)lines.size();

	int start = 0;
	if (args >= 1) {
		const int maxLines = lua_toint(L, 1);
		if (maxLines < lineCount) {
			start = (lineCount - maxLines);
		}
	}

	// table = { [1] = { text = string, zone = number}, etc... }
	lua_newtable(L);
	int count = 0;
	for (int i = start; i < lineCount; i++) {
		count++;
		lua_pushnumber(L, count);
		lua_newtable(L); {
			lua_pushstring(L, "text");
			lua_pushstring(L, lines[i].text.c_str());
			lua_rawset(L, -3);
			// FIXME: migrate priority to subsystem...
			lua_pushstring(L, "priority");
			lua_pushnumber(L, 0 /*priority*/ );
			//lua_pushstring(L, lines[i].subsystem->name);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	return 1;
}


int LuaUnsyncedRead::GetCurrentTooltip(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	const string tooltip = mouse->GetCurrentTooltip();
	lua_pushstring(L, tooltip.c_str());
	return 1;
}


int LuaUnsyncedRead::GetKeyCode(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeyCode(\"keysym\")");
	}
	const string keysym = lua_tostring(L, 1);
	lua_pushnumber(L, keyCodes->GetCode(keysym));
	return 1;
}


int LuaUnsyncedRead::GetKeySymbol(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeySymbol(keycode)");
	}
	const int keycode = lua_toint(L, 1);
	lua_pushstring(L, keyCodes->GetName(keycode).c_str());
	lua_pushstring(L, keyCodes->GetDefaultName(keycode).c_str());
	return 2;
}


int LuaUnsyncedRead::GetKeyBindings(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeyBindings(\"keyset\")");
	}
	const string keysetStr = lua_tostring(L, 1);
	CKeySet ks;
	if (!ks.Parse(keysetStr)) {
		return 0;
	}
	const CKeyBindings::ActionList&	actions = keyBindings->GetActionList(ks);
	lua_newtable(L);
	for (int i = 0; i < (int)actions.size(); i++) {
		const Action& action = actions[i];
		lua_pushnumber(L, i + 1);
		lua_newtable(L);
		lua_pushstring(L, action.command.c_str());
		lua_pushstring(L, action.extra.c_str());
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, actions.size());
	lua_rawset(L, -3);
	return 1;
}


int LuaUnsyncedRead::GetActionHotKeys(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetActionHotKeys(\"command\")");
	}
	const string command = lua_tostring(L, 1);
	const CKeyBindings::HotkeyList&	hotkeys = keyBindings->GetHotkeys(command);
	lua_newtable(L);
	for (int i = 0; i < (int)hotkeys.size(); i++) {
		const string& hotkey = hotkeys[i];
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, hotkey.c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, hotkeys.size());
	lua_rawset(L, -3);
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetGroupList(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupList

	CheckNoArgs(L, __FUNCTION__);
	if (grouphandlers[gu->myTeam] == NULL) {
		return 0;
	}
	lua_newtable(L);
	int count = 0;
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	vector<CGroup*>::const_iterator git;
	for (git = groups.begin(); git != groups.end(); ++git) {
		const CGroup* group = *git;
		if ((group != NULL) && !group->units.empty()) {
			lua_pushnumber(L, group->id);
			lua_pushnumber(L, group->units.size());
			lua_rawset(L, -3);
			count++;
		}
	}
	lua_pushnumber(L, count);
	return 2;
}


int LuaUnsyncedRead::GetSelectedGroup(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, selectedUnits.selectedGroup);
	return 1;
}


int LuaUnsyncedRead::GetGroupAIList(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupAIList

	int groupAIsDoNotExistAnymore = false;
	assert(groupAIsDoNotExistAnymore);

	return 1;
}


int LuaUnsyncedRead::GetGroupAIName(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupAIName

	int groupAIsDoNotExistAnymore = false;
	assert(groupAIsDoNotExistAnymore);

	return 0; // failure
}


int LuaUnsyncedRead::GetUnitGroup(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if ((unit->team == gu->myTeam) && (unit->group)) {
		lua_pushnumber(L, unit->group->id);
		return 1;
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedRead::GetGroupUnits(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupUnits

	const int groupID = luaL_checkint(L, 1);
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || ((size_t)groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	lua_newtable(L);
	int count = 0;
	const CUnitSet& groupUnits = groups[groupID]->units;
	CUnitSet::const_iterator it;
	for (it = groupUnits.begin(); it != groupUnits.end(); ++it) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, (*it)->id);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", count);

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsSorted(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupUnitsSorted

	const int groupID = luaL_checkint(L, 1);
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || ((size_t)groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	map<int, vector<CUnit*> > unitDefMap;
	const CUnitSet& groupUnits = groups[groupID]->units;
	CUnitSet::const_iterator it;
	for (it = groupUnits.begin(); it != groupUnits.end(); ++it) {
		CUnit* unit = *it;
		unitDefMap[unit->unitDef->id].push_back(unit);
	}

	lua_newtable(L);
	map<int, vector<CUnit*> >::const_iterator mit;
	for (mit = unitDefMap.begin(); mit != unitDefMap.end(); mit++) {
		lua_pushnumber(L, mit->first); // push the UnitDef index
		lua_newtable(L); {
			const vector<CUnit*>& v = mit->second;
			for (int i = 0; i < (int)v.size(); i++) {
				CUnit* unit = v[i];
				lua_pushnumber(L, i + 1);
				lua_pushnumber(L, unit->id);
				lua_rawset(L, -3);
			}
			HSTR_PUSH_NUMBER(L, "n", v.size());
		}
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", unitDefMap.size());

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsCounts(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupUnitsCounts

	const int groupID = luaL_checkint(L, 1);
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || ((size_t)groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	map<int, int> countMap;
	const CUnitSet& groupUnits = groups[groupID]->units;
	CUnitSet::const_iterator it;
	for (it = groupUnits.begin(); it != groupUnits.end(); ++it) {
		CUnit* unit = *it;
		const int udID = unit->unitDef->id;
		map<int, int>::iterator mit = countMap.find(udID);
		if (mit == countMap.end()) {
			countMap[udID] = 1;
		} else {
			mit->second++;
		}
	}

	lua_newtable(L);
	map<int, int>::const_iterator mit;
	for (mit = countMap.begin(); mit != countMap.end(); mit++) {
		lua_pushnumber(L, mit->first);  // push the UnitDef index
		lua_pushnumber(L, mit->second); // push the UnitDef unit count
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", countMap.size());

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsCount(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupUnitsCount

	const int groupID = luaL_checkint(L, 1);
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || ((size_t)groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}
	lua_pushnumber(L, groups[groupID]->units.size());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetMyPlayerID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myPlayerNum);
	return 1;
}


int LuaUnsyncedRead::GetMyTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myTeam);
	return 1;
}


int LuaUnsyncedRead::GetMyAllyTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myAllyTeam);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetPlayerTraffic(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	const int packetID = (int)luaL_optnumber(L, 2, -1);

	const std::map<int, CGame::PlayerTrafficInfo>& traffic
		= game->GetPlayerTraffic();
	std::map<int, CGame::PlayerTrafficInfo>::const_iterator it;
	it = traffic.find(playerID);
	if (it == traffic.end()) {
		lua_pushnumber(L, -1);
		return 1;
	}

	// only allow viewing stats for specific packet types
	if ((playerID != -1) &&              // all system counts can be read
	    (playerID != gu->myPlayerNum) && // all  self  counts can be read
	    (packetID != -1) &&
	    (packetID != NETMSG_CHAT)     &&
	    (packetID != NETMSG_PAUSE)    &&
	    (packetID != NETMSG_LUAMSG)   &&
	    (packetID != NETMSG_STARTPOS) &&
	    (packetID != NETMSG_USER_SPEED)) {
    lua_pushnumber(L, -1);
    return 1;
  }

	const CGame::PlayerTrafficInfo& pti = it->second;
	if (packetID == -1) {
		lua_pushnumber(L, pti.total);
		return 1;
	}
	std::map<int, int>::const_iterator pit = pti.packets.find(packetID);
	if (pit == pti.packets.end()) {
		lua_pushnumber(L, -1);
		return 1;
	}
	lua_pushnumber(L, pit->second);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetDrawSelectionInfo(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "Incorrect arguments to GetDrawSelectionInfo()");
	}

	lua_pushboolean(L, guihandler ? guihandler->GetDrawSelectionInfo() : 0);
	return 1;
}


/******************************************************************************/
/******************************************************************************/




