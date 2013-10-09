/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "lib/streflop/streflop_cond.h"
#include "System/myMath.h"
#include "LuaMathExtra.h"
#include "LuaInclude.h"
#include "LuaUtils.h"

static const lua_Number pow10[] = {1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f, 10000000.0f};

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
	return true;
}


/******************************************************************************/
/******************************************************************************/

int LuaMathExtra::hypot (lua_State *L) {
  lua_pushnumber(L, math::hypot(luaL_checknumber_noassert(L, 1), luaL_checknumber_noassert(L, 2)));
  return 1;
}

int LuaMathExtra::diag (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  lua_Number res = 0.0f;
  int i;
  for (i=1; i<=n; i++) {
    lua_Number d = luaL_checknumber_noassert(L, i);
    res += d*d;
  }
  lua_pushnumber(L, math::sqrt(res));
  return 1;
}

int LuaMathExtra::clamp (lua_State *L) {
  lua_Number clamp = luaL_checknumber_noassert(L, 1);
  lua_Number lbound = luaL_checknumber_noassert(L, 2);
  lua_Number ubound = luaL_checknumber_noassert(L, 3);
  if (clamp < lbound)
    clamp = lbound;
  else if (clamp > ubound)
    clamp = ubound;
  lua_pushnumber(L, clamp);
  return 1;
}

int LuaMathExtra::sgn (lua_State *L) {
  lua_Number x = luaL_checknumber_noassert(L, 1);
  if (x != 0.0f)
    lua_pushnumber(L, Sign(x));
  else
    lua_pushnumber(L, 0.0f);
  return 1;
}

int LuaMathExtra::mix (lua_State *L) {
  lua_Number x = luaL_checknumber_noassert(L, 1);
  lua_Number y = luaL_checknumber_noassert(L, 2);
  lua_Number a = luaL_checknumber_noassert(L, 3);
  lua_pushnumber(L, x+(y-x)*a);
  return 1;
}

int LuaMathExtra::round (lua_State *L) {
  lua_Number x = luaL_checknumber_noassert(L, 1);
  int ops = lua_gettop(L);  /* number of arguments */
  if (ops>1) {
    int n = luaL_checkint(L, 2);
    if (n>0){
      if (n<=7){ // LUA in Spring uses 32-bit floats, therefore max accuracy is 7 decimal digits
        lua_Number flx = math::floor(x);
        lua_Number t = x - flx;
        lua_Number rounding = pow10[n];
        lua_pushnumber(L, flx + math::floor(t*rounding + 0.5f)/rounding);
      }
      else
        lua_pushnumber(L, x);
      return 1;
    }
  }
  lua_pushnumber(L, math::floor(x+0.5f));
  return 1;
}

int LuaMathExtra::erf (lua_State *L) {
  lua_pushnumber(L, math::erf(luaL_checknumber_noassert(L, 1)));
  return 1;
}

/******************************************************************************/
/******************************************************************************/
