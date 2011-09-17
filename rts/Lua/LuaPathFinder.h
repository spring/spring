/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_PATH_FINDER_H
#define LUA_PATH_FINDER_H

struct lua_State;

class LuaPathFinder {
public:
	static bool PushEntries(lua_State* L);
	static int PushPathNodes(lua_State* L, const int pathID);

private:
	static int RequestPath(lua_State* L);
	static int InitPathNodeCostsArray(lua_State* L);
	static int FreePathNodeCostsArray(lua_State* L);
	static int SetPathNodeCosts(lua_State* L);
	static int GetPathNodeCosts(lua_State* L);
	static int SetPathNodeCost(lua_State* L);
	static int GetPathNodeCost(lua_State* L);
};


#endif /* LUA_PATH_FINDER_H */
