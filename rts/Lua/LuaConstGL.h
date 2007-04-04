#ifndef LUA_CONST_GL_H
#define LUA_CONST_GL_H
// LuaConstGL.h: interface for the LuaConstGL class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

struct lua_State;

class LuaConstGL {
	public:
		static bool PushEntries(lua_State* L);
};


#endif /* LUA_CONST_GL_H */
