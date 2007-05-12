#ifndef LUA_BOOL_OPS_H
#define LUA_BOOL_OPS_H
// LuaBoolOps.h: interface for the LuaBoolOps class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

struct lua_State;

class LuaBoolOps {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int bool_or(lua_State* L);
		static int bool_and(lua_State* L);
		static int bool_xor(lua_State* L);
		static int bool_inv(lua_State* L);
		static int bool_bits(lua_State* L);
};


#endif /* LUA_BOOL_OPS_H */
