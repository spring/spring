#ifndef LUA_CONST_GAME_H
#define LUA_CONST_GAME_H
// LuaConstGame.h: interface for the LuaConstGame class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

struct lua_State;

class LuaConstGame {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_CONST_GAME_H */
