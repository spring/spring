#ifndef LUA_CONST_CMDTYPE_H
#define LUA_CONST_CMDTYPE_H
// LuaConstCMDTYPE.h: interface for the LuaConstCMDTYPE class.
//
//////////////////////////////////////////////////////////////////////

struct lua_State;

class LuaConstCMDTYPE {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_CONST_CMDTYPE_H */
