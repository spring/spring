/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "lib/streflop/streflop_cond.h"
#include "System/SpringMath.h"
#include "LuaMathExtra.h"
#include "LuaInclude.h"
#include "LuaUtils.h"

static const lua_Number POWERS_OF_TEN[] = {1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f, 10000000.0f};

/******************************************************************************/
/******************************************************************************/

bool LuaMathExtra::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "hypot",  hypot);
	LuaPushNamedCFunc(L, "diag",   diag);
	LuaPushNamedCFunc(L, "clamp",  clamp);
	LuaPushNamedCFunc(L, "sgn",    sgn);
	LuaPushNamedCFunc(L, "mix",    mix);
	LuaPushNamedCFunc(L, "round",  round);
	LuaPushNamedCFunc(L, "erf",    erf);
	LuaPushNamedCFunc(L, "smoothstep", smoothstep);
	return true;
}


/******************************************************************************/
/******************************************************************************/

int LuaMathExtra::hypot(lua_State* L) {
	lua_pushnumber(L, math::hypot(luaL_checknumber_noassert(L, 1), luaL_checknumber_noassert(L, 2)));
	return 1;
}

int LuaMathExtra::diag(lua_State* L) {
	lua_Number res = 0.0f;

	for (int i = lua_gettop(L); i >= 1; i--) {
		res += Square(luaL_checknumber_noassert(L, i));
	}

	lua_pushnumber(L, math::sqrt(res));
	return 1;
}

int LuaMathExtra::clamp(lua_State* L) {
	const lua_Number lbound = luaL_checknumber_noassert(L, 2);
	const lua_Number ubound = luaL_checknumber_noassert(L, 3);

	lua_pushnumber(L, Clamp(luaL_checknumber_noassert(L, 1), lbound, ubound));
	return 1;
}

int LuaMathExtra::sgn(lua_State* L) {
	const lua_Number x = luaL_checknumber_noassert(L, 1);

	if (x != 0.0f) {
		// engine's version returns -1 for sgn(0)
		lua_pushnumber(L, Sign(x));
	} else {
		lua_pushnumber(L, 0.0f);
	}

	return 1;
}

int LuaMathExtra::mix(lua_State* L) {
	const lua_Number x = luaL_checknumber_noassert(L, 1);
	const lua_Number y = luaL_checknumber_noassert(L, 2);
	const lua_Number a = luaL_checknumber_noassert(L, 3);

	lua_pushnumber(L, ::mix(x, y, a));
	return 1;
}

int LuaMathExtra::round(lua_State* L) {
	const lua_Number x = luaL_checknumber_noassert(L, 1);

	if (lua_gettop(L) > 1) {
		// round number to <n> decimals
		// Spring's Lua interpreter uses 32-bit floats,
		// therefore max. accuracy is ~7 decimal digits
		const int i = std::min(7, int(sizeof(POWERS_OF_TEN) / sizeof(float)) - 1);
		const int n = Clamp(luaL_checkint(L, 2), 0, i);

		const lua_Number xinteg = math::floor(x);
		const lua_Number xfract = x - xinteg;

		lua_pushnumber(L, xinteg + math::floor((xfract * POWERS_OF_TEN[n]) + 0.5f) / POWERS_OF_TEN[n]);
	} else {
		lua_pushnumber(L, math::floor(x + 0.5f));
	}

	return 1;
}

int LuaMathExtra::erf(lua_State* L) {
	lua_pushnumber(L, math::erf(luaL_checknumber_noassert(L, 1)));
	return 1;
}

int LuaMathExtra::smoothstep(lua_State* L) {
	lua_pushnumber(L, ::smoothstep(luaL_checkfloat(L, 1), luaL_checkfloat(L, 2), luaL_checkfloat(L, 3)));
	return 1;
}

/******************************************************************************/
/******************************************************************************/

