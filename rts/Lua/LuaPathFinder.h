/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_PATH_FINDER_H
#define LUA_PATH_FINDER_H

#include <vector>


struct lua_State;

class LuaPathFinder {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int RequestPath(lua_State* L);
};


#endif /* LUA_PATH_FINDER_H */
