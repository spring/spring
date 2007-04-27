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

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "Game/Camera.h"
#include "Game/CameraController.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/Player.h"
#include "Game/PlayerRoster.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/MouseHandler.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Net.h"
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
	REGISTER_LUA_CFUNC(IsGUIHidden);

	REGISTER_LUA_CFUNC(GetFrameTimeOffset);
	REGISTER_LUA_CFUNC(IsSphereInView);
	REGISTER_LUA_CFUNC(IsUnitInView);
	REGISTER_LUA_CFUNC(IsUnitSelected);
	REGISTER_LUA_CFUNC(GetUnitViewPosition);

	REGISTER_LUA_CFUNC(GetPlayerRoster);

	REGISTER_LUA_CFUNC(GetLocalPlayerID);
	REGISTER_LUA_CFUNC(GetLocalTeamID);
	REGISTER_LUA_CFUNC(GetLocalAllyTeamID);
	REGISTER_LUA_CFUNC(GetSpectatingState);

	REGISTER_LUA_CFUNC(GetSelectedUnits);
	REGISTER_LUA_CFUNC(GetSelectedUnitsSorted);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCounts);

	REGISTER_LUA_CFUNC(GetCameraNames);
	REGISTER_LUA_CFUNC(GetCameraState);
	REGISTER_LUA_CFUNC(GetCameraPosition);
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


int LuaUnsyncedRead::IsGUIHidden(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (!game || game->hideInterface) {
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;	
}


int LuaUnsyncedRead::GetFrameTimeOffset(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->timeOffset);
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


int LuaUnsyncedRead::IsUnitInView(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, camera->InView(unit->midPos, unit->radius));
	return 1;	
}


int LuaUnsyncedRead::IsUnitSelected(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const set<CUnit*>& selUnits = selectedUnits.selectedUnits;
	lua_pushboolean(L, selUnits.find(unit) != selUnits.end());
	return 1;	
}


int LuaUnsyncedRead::GetUnitViewPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float3 interPos = unit->pos + (unit->speed * gu->timeOffset);
	lua_pushnumber(L, interPos.x);
	lua_pushnumber(L, interPos.y);
	lua_pushnumber(L, interPos.z);
	return 3;	
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
	const set<CUnit*>& selUnits = selectedUnits.selectedUnits;
	set<CUnit*>::const_iterator it;
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
	const set<CUnit*>& selUnits = selectedUnits.selectedUnits;
	set<CUnit*>::const_iterator it;
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
	const set<CUnit*>& selUnits = selectedUnits.selectedUnits;
	set<CUnit*>::const_iterator it;
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


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetCameraNames(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);

	lua_newtable(L);
	const int count = (int)mouse->camControllers.size();
	for (int i = 0; i < count; i++) {
		const CCameraController* camCtrl = mouse->camControllers[i];
		lua_pushstring(L, camCtrl->GetName().c_str());
		lua_pushnumber(L, camCtrl->num);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetCameraState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);

	lua_newtable(L);

	lua_pushstring(L, "mode");
	lua_pushnumber(L, mouse->currentCamControllerNum);
	lua_rawset(L, -3);
	lua_pushstring(L, "name");
	lua_pushstring(L, mouse->currentCamController->GetName().c_str());
	lua_rawset(L, -3);

	vector<float> camState = mouse->currentCamController->GetState();
	for (int i = 0; i < (int)camState.size(); i++) {
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
	const int wx = (int)lua_tonumber(L, 1) - gu->viewPosX;
	const int wy = gu->viewSizeY - 1 - (int)lua_tonumber(L, 2);
	if ((wx < gu->viewPosX) || (wx >= (gu->viewSizeX + gu->viewPosX)) ||
	    (wy < gu->viewPosY) || (wy >= (gu->viewSizeY + gu->viewPosY))) {
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
	PUSH_ROSTER_ENTRY(number, p->ping);
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
