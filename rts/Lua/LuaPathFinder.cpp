/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <stdlib.h>
#include <algorithm>
#include <vector>

#include "mmgr.h"

using namespace std;

#include "LuaPathFinder.h"
#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/MoveTypes/MoveInfo.h"

static std::map<unsigned int, std::vector<float> > costArrayMapSynced;
static std::map<unsigned int, std::vector<float> > costArrayMapUnsynced;

static void CreatePathMetatable(lua_State* L);


/******************************************************************************/
/******************************************************************************/

bool LuaPathFinder::PushEntries(lua_State* L)
{
	CreatePathMetatable(L);

#define REGISTER_LUA_CFUNC(x)   \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)
                        
	REGISTER_LUA_CFUNC(RequestPath);
	REGISTER_LUA_CFUNC(InitPathNodeCostsArray);
	REGISTER_LUA_CFUNC(FreePathNodeCostsArray);
	REGISTER_LUA_CFUNC(SetPathNodeCosts);
	REGISTER_LUA_CFUNC(GetPathNodeCosts);
	REGISTER_LUA_CFUNC(SetPathNodeCost);
	REGISTER_LUA_CFUNC(GetPathNodeCost);

	return true;	
}


/******************************************************************************/

static int path_next(lua_State* L)
{
	const int* idPtr = (int*)luaL_checkudata(L, 1, "Path");
	const int pathID = *idPtr;
	if (pathID == 0) {
		return 0;
	}

	const int args = lua_gettop(L);

	float3 callerPos = ZeroVector;
	if (args >= 4) {
		callerPos.x = luaL_checkfloat(L, 2);
		callerPos.y = luaL_checkfloat(L, 3);
		callerPos.z = luaL_checkfloat(L, 4);
	}

	const float minDist = luaL_optfloat(L, 5, 0.0f);

	const bool synced = CLuaHandle::GetActiveHandle()->GetSynced();
	const float3 point = pathManager->NextWaypoint(pathID, callerPos, minDist, 0, 0, synced);

	if ((point.x == -1.0f) &&
	    (point.y == -1.0f) &&
	    (point.z == -1.0f)) {
		return 0;
	}

	lua_pushnumber(L, point.x);
	lua_pushnumber(L, point.y);
	lua_pushnumber(L, point.z);

	return 3;
}


static int path_estimates(lua_State* L)
{
	const int* idPtr = (int*)luaL_checkudata(L, 1, "Path");
	const int pathID = *idPtr;
	if (pathID == 0) {
		return 0;
	}

	vector<float3> points;
	vector<int>    starts;
	pathManager->GetEstimatedPath(pathID, points, starts);

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



static int path_index(lua_State* L)
{
	const int* idPtr = (int*)luaL_checkudata(L, 1, "Path");
	const int pathID = *idPtr;
	if (pathID == 0) {
		return 0;
	}
	const string key = luaL_checkstring(L, 2);
	if (key == "Next") {
		lua_pushcfunction(L, path_next);
		return 1;
	}
	else if (key == "GetEstimatedPath") {
		lua_pushcfunction(L, path_estimates);
		return 1;
	}
	return 0;
}

static int path_newindex(lua_State* L)
{
	return 0;
}

static int path_gc(lua_State* L)
{
	int* idPtr = (int*)luaL_checkudata(L, 1, "Path");
	const int pathID = *idPtr;
	if (pathID == 0) {
		return 0;
	}
	pathManager->DeletePath(*idPtr);
	*idPtr = 0;
	return 0;
}


static void CreatePathMetatable(lua_State* L) 
{
	luaL_newmetatable(L, "Path");
	HSTR_PUSH_CFUNC(L, "__gc",       path_gc);
	HSTR_PUSH_CFUNC(L, "__index",    path_index);
	HSTR_PUSH_CFUNC(L, "__newindex", path_newindex);
	lua_pop(L, 1);
}


/******************************************************************************/
/******************************************************************************/

int LuaPathFinder::RequestPath(lua_State* L)
{
	const MoveData* moveData = NULL;
	
	if (lua_israwstring(L, 1)) {
		moveData = moveinfo->GetMoveDataFromName(lua_tostring(L, 1));
	} else {
		const int moveID = luaL_checkint(L, 1);
		if ((moveID < 0) || ((size_t)moveID >= moveinfo->moveData.size())) {
			luaL_error(L, "Invalid moveID passed to RequestPath");
		}
		moveData = moveinfo->moveData[moveID];
	}

	if (moveData == NULL) {
		return 0;
	}

	const float3 start(luaL_checkfloat(L, 2),
	                   luaL_checkfloat(L, 3),
	                   luaL_checkfloat(L, 4));

	const float3   end(luaL_checkfloat(L, 5),
	                   luaL_checkfloat(L, 6),
	                   luaL_checkfloat(L, 7));

	const float radius = luaL_optfloat(L, 8, 8.0f);

	const bool synced = CLuaHandle::GetActiveHandle()->GetSynced();
	const int pathID = pathManager->RequestPath(moveData, start, end, radius, NULL, synced);

	if (pathID == 0) {
		return 0;
	}
	
	int* idPtr = (int*)lua_newuserdata(L, sizeof(int));
	luaL_getmetatable(L, "Path");
	lua_setmetatable(L, -2);

	*idPtr = pathID;

	return 1;
}



int LuaPathFinder::InitPathNodeCostsArray(lua_State* L)
{
	const unsigned int array = luaL_checkint(L, 1);
	const unsigned int sizex = luaL_checkint(L, 2);
	const unsigned int sizez = luaL_checkint(L, 3);
	const bool synced = CLuaHandle::GetActiveHandle()->GetSynced();

	std::map<unsigned int, std::vector<float> >& map = synced?
		costArrayMapSynced:
		costArrayMapUnsynced;
	std::vector<float>& vec = map[array];

	if (sizex == 0 || sizez == 0) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (!vec.empty()) {
		// no resizing
		lua_pushboolean(L, false);
		return 1;
	}

	vec.resize(sizex * sizez, 0.0f);

	lua_pushboolean(L, true);
	return 1;
}

int LuaPathFinder::FreePathNodeCostsArray(lua_State* L)
{
	const unsigned int array = luaL_checkint(L, 1);
	const bool synced = CLuaHandle::GetActiveHandle()->GetSynced();

	std::map<unsigned int, std::vector<float> >& map = synced?
		costArrayMapSynced:
		costArrayMapUnsynced;
	std::vector<float>& vec = map[array];

	if (vec.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	// nullify the active cost-overlay
	if (pathManager->GetNodeExtraCosts(synced) == &vec[0]) {
		pathManager->SetNodeExtraCosts(NULL, 1, 1, synced);
	}

	vec.clear();
	map.erase(array);

	lua_pushboolean(L, true);
	return 1;
}


int LuaPathFinder::SetPathNodeCosts(lua_State* L)
{
	const unsigned int array = luaL_checkint(L, 1);
	const unsigned int sizex = luaL_checkint(L, 2);
	const unsigned int sizez = luaL_checkint(L, 3);
	const bool synced = CLuaHandle::GetActiveHandle()->GetSynced();

	std::map<unsigned int, std::vector<float> >& map = synced?
		costArrayMapSynced:
		costArrayMapUnsynced;
	std::vector<float>& vec = map[array];

	if (vec.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	// set the active cost-overlay to <vec>
	lua_pushboolean(L, pathManager->SetNodeExtraCosts(&vec[0], sizex, sizez, synced));
	return 1;
}

int LuaPathFinder::GetPathNodeCosts(lua_State* L)
{
	// not implemented
	return 0;
}


int LuaPathFinder::SetPathNodeCost(lua_State* L)
{
	const unsigned int array = luaL_checkint(L, 1);
	const unsigned int index = luaL_checkint(L, 2);
	const unsigned int hmx = luaL_checkint(L, 3);
	const unsigned int hmz = luaL_checkint(L, 4);
	const float cost = luaL_checkfloat(L, 5);
	const bool synced = CLuaHandle::GetActiveHandle()->GetSynced();

	std::map<unsigned int, std::vector<float> >& map = synced?
		costArrayMapSynced:
		costArrayMapUnsynced;
	std::vector<float>& vec = map[array];

	if (vec.empty()) {
		// invalid array ID (possibly created through operator[])
		lua_pushboolean(L, pathManager->SetNodeExtraCost(hmx, hmz, cost, synced));
	} else {
		// modify only the cost-overlay (whether it is active or not)
		if (index < vec.size())
			vec[index] = cost;

		lua_pushboolean(L, (index < vec.size()));
	}

	return 1;
}

int LuaPathFinder::GetPathNodeCost(lua_State* L)
{
	const unsigned int hmx = luaL_checkint(L, 1);
	const unsigned int hmz = luaL_checkint(L, 2);
	const bool synced = CLuaHandle::GetActiveHandle()->GetSynced();

	// reads from overlay if PathNodeStateBuffer::extraCosts != NULL
	const float cost = pathManager->GetNodeExtraCost(hmx, hmz, synced);

	lua_pushnumber(L, cost);
	return 1;
}

/******************************************************************************/
/******************************************************************************/
