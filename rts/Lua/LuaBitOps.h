/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_BOOL_OPS_H
#define LUA_BOOL_OPS_H

struct lua_State;

class LuaBitOps {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int bit_or(lua_State* L);
		static int bit_and(lua_State* L);
		static int bit_xor(lua_State* L);
		static int bit_inv(lua_State* L);
		static int bit_bits(lua_State* L);
};

#endif /* LUA_BOOL_OPS_H */
