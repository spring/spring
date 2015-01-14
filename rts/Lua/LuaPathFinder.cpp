/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaPathFinder.h"
#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

#include <stdlib.h>
#include <algorithm>
#include <vector>


struct NodeCostOverlay {
	NodeCostOverlay(): sizex(0), sizez(0) {}

	void Init(unsigned int sx, unsigned int sz) {
		costs.resize(sx * sz, 0.0f);
		sizex = sx;
		sizez = sz;
	}
	void Clear() { costs.clear(); }
	bool Empty() const { return costs.empty(); }
	unsigned int Size() const { return costs.size(); }

	std::vector<float> costs;

	unsigned int sizex;
	unsigned int sizez;
};

static std::map<unsigned int, NodeCostOverlay> costArrayMapSynced;
static std::map<unsigned int, NodeCostOverlay> costArrayMapUnsynced;

static void CreatePathMetatable(lua_State* L);


/******************************************************************************/
/******************************************************************************/

bool LuaPathFinder::PushEntries(lua_State* L)
{
	// safety in case of reload
	costArrayMapSynced.clear();
	costArrayMapUnsynced.clear();

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

int LuaPathFinder::PushPathNodes(lua_State* L, const int pathID)
{
	if (pathID == 0)
		return 0;

	vector<float3> points;
	vector<int>    starts;

	pathManager->GetPathWayPoints(pathID, points, starts);

	const int pointCount = points.size();
	const int startCount = starts.size();

	{
		lua_newtable(L);

		for (int i = 0; i < pointCount; i++) {
			lua_newtable(L); {
				const float3& p = points[i];
				lua_pushnumber(L, p.x); lua_rawseti(L, -2, 1);
				lua_pushnumber(L, p.y); lua_rawseti(L, -2, 2);
				lua_pushnumber(L, p.z); lua_rawseti(L, -2, 3);
			}
			lua_rawseti(L, -2, i + 1);
		}
	}

	{
		lua_newtable(L);

		for (int i = 0; i < startCount; i++) {
			lua_pushnumber(L, starts[i] + 1);
			lua_rawseti(L, -2, i + 1);
		}
	}

	return 2;
}


/******************************************************************************/

static int path_next(lua_State* L)
{
	const int* idPtr = (int*)luaL_checkudata(L, 1, "Path");
	const int pathID = *idPtr;

	if (pathID == 0)
		return 0;

	const int args = lua_gettop(L);

	float3 callerPos = ZeroVector;
	if (args >= 4) {
		callerPos.x = luaL_checkfloat(L, 2);
		callerPos.y = luaL_checkfloat(L, 3);
		callerPos.z = luaL_checkfloat(L, 4);
	}

	const float minDist = luaL_optfloat(L, 5, 0.0f);

	const bool synced = CLuaHandle::GetHandleSynced(L);
	const float3 point = pathManager->NextWayPoint(NULL, pathID, 0, callerPos, minDist, synced);

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


static int path_nodes(lua_State* L)
{
	const int* idPtr = (int*)luaL_checkudata(L, 1, "Path");
	const int pathID = *idPtr;

	return (LuaPathFinder::PushPathNodes(L, pathID));
}

static int path_index(lua_State* L)
{
	const int* idPtr = (int*)luaL_checkudata(L, 1, "Path");
	const int pathID = *idPtr;

	if (pathID == 0)
		return 0;

	const string key = luaL_checkstring(L, 2);
	if (key == "Next") {
		lua_pushcfunction(L, path_next);
		return 1;
	}
	else if (key == "GetPathWayPoints") {
		lua_pushcfunction(L, path_nodes);
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

	if (pathID == 0)
		return 0;

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
	const MoveDef* moveDef = NULL;

	if (lua_israwstring(L, 1)) {
		moveDef = moveDefHandler->GetMoveDefByName(lua_tostring(L, 1));
	} else {
		const unsigned int pathType = luaL_checkint(L, 1);

		if (pathType >= moveDefHandler->GetNumMoveDefs()) {
			luaL_error(L, "Invalid moveID passed to RequestPath");
		}

		moveDef = moveDefHandler->GetMoveDefByPathType(pathType);
	}

	if (moveDef == NULL)
		return 0;

	const float3 start(luaL_checkfloat(L, 2),
	                   luaL_checkfloat(L, 3),
	                   luaL_checkfloat(L, 4));

	const float3   end(luaL_checkfloat(L, 5),
	                   luaL_checkfloat(L, 6),
	                   luaL_checkfloat(L, 7));

	const float radius = luaL_optfloat(L, 8, 8.0f);

	const bool synced = CLuaHandle::GetHandleSynced(L);
	const int pathID = pathManager->RequestPath(NULL, moveDef, start, end, radius, synced);

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
	const bool synced = CLuaHandle::GetHandleSynced(L);

	std::map<unsigned int, NodeCostOverlay>& map = synced?
		costArrayMapSynced:
		costArrayMapUnsynced;
	NodeCostOverlay& overlay = map[array];

	if (sizex == 0 || sizez == 0) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (!overlay.Empty()) {
		// no resizing of existing overlays
		lua_pushboolean(L, false);
		return 1;
	}

	overlay.Init(sizex, sizez);
	lua_pushboolean(L, true);
	return 1;
}

int LuaPathFinder::FreePathNodeCostsArray(lua_State* L)
{
	const unsigned int array = luaL_checkint(L, 1);
	const bool synced = CLuaHandle::GetHandleSynced(L);

	std::map<unsigned int, NodeCostOverlay>& map = synced?
		costArrayMapSynced:
		costArrayMapUnsynced;
	NodeCostOverlay& overlay = map[array];

	if (overlay.Empty()) {
		// not an existing overlay
		lua_pushboolean(L, false);
		return 1;
	}

	// nullify the active cost-overlay if we are freeing it
	if (pathManager->GetNodeExtraCosts(synced) == &overlay.costs[0]) {
		pathManager->SetNodeExtraCosts(NULL, 1, 1, synced);
	}

	overlay.Clear();
	map.erase(array);

	lua_pushboolean(L, true);
	return 1;
}


int LuaPathFinder::SetPathNodeCosts(lua_State* L)
{
	const unsigned int array = luaL_checkint(L, 1);
	const bool synced = CLuaHandle::GetHandleSynced(L);

	std::map<unsigned int, NodeCostOverlay>& map = synced?
		costArrayMapSynced:
		costArrayMapUnsynced;
	NodeCostOverlay& overlay = map[array];

	if (overlay.Empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	// set the active cost-overlay to <overlay>
	lua_pushboolean(L, pathManager->SetNodeExtraCosts(&overlay.costs[0], overlay.sizex, overlay.sizez, synced));
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
	const float cost = luaL_checkfloat(L, 3);
	const bool synced = CLuaHandle::GetHandleSynced(L);

	std::map<unsigned int, NodeCostOverlay>& map = synced?
		costArrayMapSynced:
		costArrayMapUnsynced;
	NodeCostOverlay& overlay = map[array];

	if (overlay.Empty()) {
		// invalid array ID (possibly created through map::operator[]), fallback
		// lua_pushboolean(L, pathManager->SetNodeExtraCost(hmx, hmz, cost, synced));
		lua_pushboolean(L, false);
	} else {
		// modify only the cost-overlay (whether it is active or not)
		if (index < overlay.Size())
			overlay.costs[index] = cost;

		lua_pushboolean(L, (index < overlay.Size()));
	}

	return 1;
}

int LuaPathFinder::GetPathNodeCost(lua_State* L)
{
	const unsigned int hmx = luaL_checkint(L, 1);
	const unsigned int hmz = luaL_checkint(L, 2);
	const bool synced = CLuaHandle::GetHandleSynced(L);

	// reads from overlay if PathNodeStateBuffer::extraCosts != NULL
	const float cost = pathManager->GetNodeExtraCost(hmx, hmz, synced);

	lua_pushnumber(L, cost);
	return 1;
}

/******************************************************************************/
/******************************************************************************/
