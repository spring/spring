/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MATH_EXTRA_H
#define LUA_MATH_EXTRA_H

struct lua_State;

class LuaMathExtra {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int hypot(lua_State* L);
		static int diag(lua_State* L);
		static int clamp(lua_State* L);
		static int sgn(lua_State* L);
		static int mix(lua_State* L);
		static int round(lua_State* L);
		static int erf(lua_State* L);
		static int smoothstep(lua_State* L);
};

#endif /* LUA_MATH_EXTRA_H */

