#ifndef LUA_CONST_CMD_H
#define LUA_CONST_CMD_H
// LuaConstCMD.h: interface for the LuaConstCMD class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

struct lua_State;

class LuaConstCMD {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_CONST_CMD_H */
