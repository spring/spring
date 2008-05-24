#include "StdAfx.h"
// LuaRules.cpp: implementation of the CLuaRules class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUnsyncedRead.h"
#include <set>
#include <list>
#include <cctype>
using namespace std;

#include "SDL_timer.h"
#include "SDL_types.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "Game/Camera.h"
#include "Game/Camera/CameraController.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/Player.h"
#include "Game/PlayerRoster.h"
#include "Game/SelectedUnits.h"
#include "Game/CameraHandler.h"
#include "Game/Team.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/ReadMap.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Platform/FileSystem.h"


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

	REGISTER_LUA_CFUNC(GetFrameTimeOffset);
	REGISTER_LUA_CFUNC(GetLastUpdateSeconds);
	REGISTER_LUA_CFUNC(GetHasLag);

	REGISTER_LUA_CFUNC(IsAABBInView);
	REGISTER_LUA_CFUNC(IsSphereInView);

	REGISTER_LUA_CFUNC(IsUnitAllied);
	REGISTER_LUA_CFUNC(IsUnitInView);
	REGISTER_LUA_CFUNC(IsUnitVisible);
	REGISTER_LUA_CFUNC(IsUnitSelected);

	REGISTER_LUA_CFUNC(GetUnitLuaDraw);
	REGISTER_LUA_CFUNC(GetUnitNoDraw);
	REGISTER_LUA_CFUNC(GetUnitNoMinimap);
	REGISTER_LUA_CFUNC(GetUnitNoSelect);

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
	const int unitID = (int)lua_tonumber(L, index);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
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


/******************************************************************************/

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
	float3 mins = float3((float)luaL_checknumber(L, 1),
	                     (float)luaL_checknumber(L, 2),
	                     (float)luaL_checknumber(L, 3));
	float3 maxs = float3((float)luaL_checknumber(L, 4),
	                     (float)luaL_checknumber(L, 5),
	                     (float)luaL_checknumber(L, 6));
	lua_pushboolean(L, camera->InView(mins, maxs));
	return 1;
}


int LuaUnsyncedRead::IsSphereInView(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to IsSphereInView()");
	}
	const float3 pos((float)lua_tonumber(L, 1),
	                 (float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3));
	float radius = 0.0f;
	if ((args >= 4) && lua_isnumber(L, 4)) {
		radius = (float)lua_tonumber(L, 4);
	}

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


int LuaUnsyncedRead::IsUnitVisible(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float radius = luaL_optnumber(L, 2, unit->radius);
	if (readAllyTeam < 0) {
		if (!fullRead) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L, camera->InView(unit->midPos, radius));
		}
	}
	else {
		if ((unit->losStatus[readAllyTeam] & LOS_INLOS) == 0) {
			lua_pushboolean(L, false);
		} else { // FIXME -- iconMode?
			lua_pushboolean(L, camera->InView(unit->midPos, radius));
		}
	}
	return 1;
}


int LuaUnsyncedRead::IsUnitSelected(lua_State* L)
{
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


int LuaUnsyncedRead::GetUnitViewPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const bool midPos = (lua_isboolean(L, 2) && lua_toboolean(L, 2));

	float3 pos = midPos ? (float3)unit->midPos : (float3)unit->pos;
	if (unit->transporter == NULL) {
		pos += (unit->speed * gu->timeOffset);
	} else {
		pos += (unit->transporter->speed * gu->timeOffset);
	}

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetVisibleUnits(lua_State* L)
{
	// FIXME: implement ALL_UNITS / ENEMY_UNITS / ALLY_UNITS / MY_UNITS
	const int teamID = luaL_optint(L, 1, -1);

	bool fixedRadius = false;
	float radius = 30.0f; // value from UnitDrawer.cpp
	if (lua_israwnumber(L, 2)) {
		radius = (float)lua_tonumber(L, 2);
		if (radius < 0.0f) {
			fixedRadius = true;
			radius = -radius;
		}
	}

	const bool noIcons = lua_isboolean(L, 3) && !lua_toboolean(L, 3);

	const float iconLength = unitDrawer->iconLength;

	int count = 0;
	lua_newtable(L);

	list<CUnit*>::iterator usi;
	for (usi = uh->activeUnits.begin(); usi != uh->activeUnits.end(); ++usi) {
		const CUnit* unit = *usi;

		if (unit->noDraw) {
			continue;
		}

		if ((teamID >= 0) && (unit->team != teamID)) {
			continue;
		}

		if (!gs->Ally(unit->allyteam, gu->myAllyTeam) &&
			  !(unit->losStatus[gu->myAllyTeam] & LOS_INLOS) &&
				!gu->spectatingFullView) {
			continue;
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
	for (int i = 0; i < cc.size(); ++i) {
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
	vector<float> camState;
	camHandler->GetState(camState);
	
	lua_pushstring(L, "mode");
	lua_pushnumber(L, camState[0]);
	lua_rawset(L, -3);
	lua_pushstring(L, "name");
	lua_pushstring(L, camHandler->GetCurrentControllerName().c_str());
	lua_rawset(L, -3);
	
	for (int i = 1; i < (int)camState.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushnumber(L, camState[i]);
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
	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to WorldToScreenCoords(x,y,z)");
	}
	const float3 worldPos((float)lua_tonumber(L, 1),
	                      (float)lua_tonumber(L, 2),
	                      (float)lua_tonumber(L, 3));
	const float3 winPos = camera->CalcWindowCoordinates(worldPos);
	lua_pushnumber(L, winPos.x);
	lua_pushnumber(L, winPos.y);
	lua_pushnumber(L, winPos.z);
	return 3;
}


int LuaUnsyncedRead::TraceScreenRay(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    ((args >= 3) && !lua_isboolean(L, 3))) {
		luaL_error(L, "Incorrect arguments to TraceScreenRay()");
	}

	// window coordinates
	const int mx = (int) lua_tonumber(L, 1);
	const int my = (int) lua_tonumber(L, 2);
	const int wx = mx + gu->viewPosX;
	const int wy = gu->viewSizeY - 1 - my - gu->viewPosY;

	if ((mx < 0) || (mx >= gu->viewSizeX) ||
		(my < 0) || (my >= gu->viewSizeY)) {
		return 0;
	}

	CUnit* unit = NULL;
	CFeature* feature = NULL;
	const float range = gu->viewRange * 1.4f;
	const float3& pos = camera->pos;
	const float3 dir = camera->CalcPixelDir(wx, wy);

	const bool onlyCoords = ((args >= 3) && lua_toboolean(L, 3));

// FIXME	const int origAllyTeam = gu->myAllyTeam;
//	gu->myAllyTeam = readAllyTeam;
	const float udist = helper->GuiTraceRay(pos, dir, range, unit, 20.0f, true);
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

	const CPlayer* p = gs->players[playerID];
	int index = 1;
	lua_newtable(L);
	PUSH_ROSTER_ENTRY(string, p->playerName.c_str());
	PUSH_ROSTER_ENTRY(number, playerID);
	PUSH_ROSTER_ENTRY(number, p->team);
	PUSH_ROSTER_ENTRY(number, gs->AllyTeam(p->team));
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
		const int type = (int)lua_tonumber(L, 1);
		playerRoster.SetSortTypeByCode((PlayerRoster::SortType)type);
	}

	int count;
	const int* players = playerRoster.GetIndices(&count);

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
	const int teamID = (int)luaL_checknumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
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
	const int teamID = (int)luaL_checknumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
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
