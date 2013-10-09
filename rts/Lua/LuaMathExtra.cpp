/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "lib/streflop/streflop_cond.h"
#include "LuaMathExtra.h"
#include "LuaInclude.h"
#include "LuaUtils.h"


/******************************************************************************/
/******************************************************************************/

bool LuaMathExtra::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "hypot",  hypot);
	LuaPushNamedCFunc(L, "diag",   diag);
	LuaPushNamedCFunc(L, "clamp",  clamp);
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

/******************************************************************************/
/******************************************************************************/
