#ifndef LUA_PATH_FINDER_H
#define LUA_PATH_FINDER_H
// LuaPathFinder.h: interface for the LuaPathFinder class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <vector>


struct lua_State;

class LuaPathFinder {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int RequestPath(lua_State* L);
};


#endif /* LUA_PATH_FINDER_H */
