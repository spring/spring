#ifndef LUA_WEAPONDEFS_H
#define LUA_WEAPONDEFS_H
// LuaWeaponDefs.h: interface for the LuaWeaponDefs class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

struct lua_State;

class LuaWeaponDefs {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_WEAPONDEFS_H */
