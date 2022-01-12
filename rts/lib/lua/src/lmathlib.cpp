/*
** $Id: lmathlib.c,v 1.67.1.1 2007/12/27 13:02:25 roberto Exp $
** Standard mathematical library
** See Copyright Notice in lua.h
*/


#include <stdlib.h>
//SPRING#include <math.h>
#include "streflop_cond.h"
#include "System/FastMath.h"

#define lmathlib_c
#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


#undef PI
//SPRING #define PI 3.14159265358979323846
#define RADIANS_PER_DEGREE math::DEG_TO_RAD //SPRING(PI/180.0)


static int math_abs (lua_State *L) {
  lua_pushnumber(L, math::fabs(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_sin (lua_State *L) {
  lua_pushnumber(L, math::sin(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_sinh (lua_State *L) {
  lua_pushnumber(L, math::sinh(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_cos (lua_State *L) {
  lua_pushnumber(L, math::cos(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_cosh (lua_State *L) {
  lua_pushnumber(L, math::cosh(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_tan (lua_State *L) {
  lua_pushnumber(L, math::tan(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_tanh (lua_State *L) {
  lua_pushnumber(L, math::tanh(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_asin (lua_State *L) {
  lua_pushnumber(L, math::asin(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_acos (lua_State *L) {
  lua_pushnumber(L, math::acos(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_atan (lua_State *L) {
  lua_pushnumber(L, math::atan(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_atan2 (lua_State *L) {
  lua_pushnumber(L, math::atan2(luaL_checknumber_noassert(L, 1), luaL_checknumber_noassert(L, 2)));
  return 1;
}

static int math_ceil (lua_State *L) {
  lua_pushnumber(L, math::ceil(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_floor (lua_State *L) {
  lua_pushnumber(L, math::floor(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_fmod (lua_State *L) {
  lua_pushnumber(L, math::fmod(luaL_checknumber_noassert(L, 1), luaL_checknumber_noassert(L, 2)));
  return 1;
}

static int math_modf (lua_State *L) {
  // FIXME -- streflop does not have modf()
  // double fp = math::modf(luaL_checknumber_noassert(L, 1), &ip);
  const float in = (float)luaL_checknumber_noassert(L, 1);

  if (math::isnan(in)) {
    lua_pushnumber(L, in);
    lua_pushnumber(L, in);
  }
  else if (math::isinf(in)) {
    lua_pushnumber(L, in);
    lua_pushnumber(L, 0.0f);
  }
  else {
    const float fp = math::fmod(in, 1.0f);
    const float ip = (in - fp);
    lua_pushnumber(L, ip);
    lua_pushnumber(L, fp);
  }
  return 2;
}

static int math_sqrt (lua_State *L) {
  lua_pushnumber(L, math::sqrt(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_pow (lua_State *L) {
  lua_pushnumber(L, math::pow(luaL_checknumber_noassert(L, 1), luaL_checknumber_noassert(L, 2)));
  return 1;
}

static int math_log (lua_State *L) {
  lua_pushnumber(L, math::log(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_log10 (lua_State *L) {
  lua_pushnumber(L, math::log10(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_exp (lua_State *L) {
  lua_pushnumber(L, math::exp(luaL_checknumber_noassert(L, 1)));
  return 1;
}

static int math_deg (lua_State *L) {
  lua_pushnumber(L, luaL_checknumber_noassert(L, 1)/RADIANS_PER_DEGREE);
  return 1;
}

static int math_rad (lua_State *L) {
  lua_pushnumber(L, luaL_checknumber_noassert(L, 1)*RADIANS_PER_DEGREE);
  return 1;
}

static int math_frexp (lua_State *L) {
  int e;
  lua_pushnumber(L, math::frexp(luaL_checknumber_noassert(L, 1), &e));
  lua_pushinteger(L, e);
  return 2;
}

static int math_ldexp (lua_State *L) {
  lua_pushnumber(L, math::ldexp(luaL_checknumber_noassert(L, 1), luaL_checkint(L, 2)));
  return 1;
}



static int math_min (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  lua_Number dmin = luaL_checknumber_noassert(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    lua_Number d = luaL_checknumber_noassert(L, i);
    if (d < dmin)
      dmin = d;
  }
  lua_pushnumber(L, dmin);
  return 1;
}


static int math_max (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  lua_Number dmax = luaL_checknumber_noassert(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    lua_Number d = luaL_checknumber_noassert(L, i);
    if (d > dmax)
      dmax = d;
  }
  lua_pushnumber(L, dmax);
  return 1;
}



static int math_random (lua_State *L) {
  #ifndef LUA_USER_H
  /* the `%' avoids the (rare) case of r==1, and is needed also because on
     some systems (SunOS!) `rand()' may return a value larger than RAND_MAX */
  lua_Number r = (lua_Number)(rand()%RAND_MAX) / (lua_Number)RAND_MAX;
  switch (lua_gettop(L)) {  /* check number of arguments */
    case 0: {  /* no arguments */
      lua_pushnumber(L, r);  /* Number between 0 and 1 */
      break;
    }
    case 1: {  /* only upper limit */
      int u = luaL_checkint(L, 1);
      luaL_argcheck(L, 1<=u, 1, "interval is empty");
      lua_pushnumber(L, math::floor(r*u)+1);  /* int between 1 and `u' */
      break;
    }
    case 2: {  /* lower and upper limits */
      int l = luaL_checkint(L, 1);
      int u = luaL_checkint(L, 2);
      luaL_argcheck(L, l<=u, 2, "interval is empty");
      lua_pushnumber(L, math::floor(r*(u-l+1))+l);  /* int between `l' and `u' */
      break;
    }
    default: return luaL_error(L, "wrong number of arguments");
  }
  return 1;
  #else
  return (spring_lua_unsynced_rand(L)); // SPRING
  #endif
}

static int math_randomseed (lua_State *L) {
  #ifndef LUA_USER_H
  srand(luaL_checkint(L, 1));
  return 0;
  #else
  return (spring_lua_unsynced_srand(L)); // SPRING
  #endif
}


static const luaL_Reg mathlib[] = {
  {"abs",   math_abs},
  {"acos",  math_acos},
  {"asin",  math_asin},
  {"atan2", math_atan2},
  {"atan",  math_atan},
  {"ceil",  math_ceil},
  {"cosh",   math_cosh},
  {"cos",   math_cos},
  {"deg",   math_deg},
  {"exp",   math_exp},
  {"floor", math_floor},
  {"fmod",   math_fmod},
  {"frexp", math_frexp},
  {"ldexp", math_ldexp},
  {"log10", math_log10},
  {"log",   math_log},
  {"max",   math_max},
  {"min",   math_min},
  {"modf",   math_modf},
  {"pow",   math_pow},
  {"rad",   math_rad},
  {"random",     math_random},
  {"randomseed", math_randomseed},
  {"sinh",   math_sinh},
  {"sin",   math_sin},
  {"sqrt",  math_sqrt},
  {"tanh",   math_tanh},
  {"tan",   math_tan},
  {NULL, NULL}
};


/*
** Open math library
*/
LUALIB_API int luaopen_math (lua_State *L) {
  luaL_register(L, LUA_MATHLIBNAME, mathlib);
  lua_pushnumber(L, math::PI);
  lua_setfield(L, -2, "pi");
  lua_pushnumber(L, math::TWOPI);
  lua_setfield(L, -2, "tau");
#if STREFLOP_ENABLED
  lua_pushnumber(L, math::SimplePositiveInfinity); // streflop
#else
  lua_pushnumber(L, HUGE_VAL);
#endif
  lua_setfield(L, -2, "huge");
#if defined(LUA_COMPAT_MOD)
  lua_getfield(L, -1, "fmod");
  lua_setfield(L, -2, "mod");
#endif
  return 1;
}

