#ifndef LUA_CONST_SPRING_H
#define LUA_CONST_SPRING_H
// LuaConstSpring.h: interface for the LuaConstSpring class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

struct lua_State;

class LuaConstSpring {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_CONST_SPRING_H */
