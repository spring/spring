/*
** $Id: ltablib.c,v 1.21 2003/04/03 13:35:34 roberto Exp $
** Library for Table Manipulation
** See Copyright Notice in lua.h
*/


#include <stddef.h>

#define ltablib_c

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"


#define aux_getn(L,n)	(luaL_checktype(L, n, LUA_TTABLE), luaL_getn(L, n))


static int luaB_foreachi (lua_State *L) {
  int i;
  int n = aux_getn(L, 1);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  for (i=1; i<=n; i++) {
    lua_pushvalue(L, 2);  /* function */
    lua_pushnumber(L, (lua_Number)i);  /* 1st argument */
    lua_rawgeti(L, 1, i);  /* 2nd argument */
    lua_call(L, 2, 1);
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 1);  /* remove nil result */
  }
  return 0;
}


static int luaB_foreach (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_pushnil(L);  /* first key */
  for (;;) {
    if (lua_next(L, 1) == 0)
      return 0;
    lua_pushvalue(L, 2);  /* function */
    lua_pushvalue(L, -3);  /* key */
    lua_pushvalue(L, -3);  /* value */
    lua_call(L, 2, 1);
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 2);  /* remove value and result */
  }
}


static int luaB_getn (lua_State *L) {
  lua_pushnumber(L, (lua_Number)aux_getn(L, 1));
  return 1;
}


static int luaB_setn (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_setn(L, 1, luaL_checkint(L, 2));
  return 0;
}


static int luaB_tinsert (lua_State *L) {
  int v = lua_gettop(L);  /* number of arguments */
  int n = aux_getn(L, 1) + 1;
  int pos;  /* where to insert new element */
  if (v == 2)  /* called with only 2 arguments */
    pos = n;  /* insert new element at the end */
  else {
    pos = luaL_checkint(L, 2);  /* 2nd argument is the position */
    if (pos > n) n = pos;  /* `grow' array if necessary */
    v = 3;  /* function may be called with more than 3 args */
  }
  luaL_setn(L, 1, n);  /* new size */
  while (--n >= pos) {  /* move up elements */
    lua_rawgeti(L, 1, n);
    lua_rawseti(L, 1, n+1);  /* t[n+1] = t[n] */
  }
  lua_pushvalue(L, v);
  lua_rawseti(L, 1, pos);  /* t[pos] = v */
  return 0;
}


static int luaB_tremove (lua_State *L) {
  int n = aux_getn(L, 1);
  int pos = luaL_optint(L, 2, n);
  if (n <= 0) return 0;  /* table is `empty' */
  luaL_setn(L, 1, n-1);  /* t.n = n-1 */
  lua_rawgeti(L, 1, pos);  /* result = t[pos] */
  for ( ;pos<n; pos++) {
    lua_rawgeti(L, 1, pos+1);
    lua_rawseti(L, 1, pos);  /* t[pos] = t[pos+1] */
  }
  lua_pushnil(L);
  lua_rawseti(L, 1, n);  /* t[n] = nil */
  return 1;
}


static int str_concat (lua_State *L) {
  luaL_Buffer b;
  size_t lsep;
  const char *sep = luaL_optlstring(L, 2, "", &lsep);
  int i = luaL_optint(L, 3, 1);
  int n = luaL_optint(L, 4, 0);
  luaL_checktype(L, 1, LUA_TTABLE);
  if (n == 0) n = luaL_getn(L, 1);
  luaL_buffinit(L, &b);
  for (; i <= n; i++) {
    lua_rawgeti(L, 1, i);
    luaL_argcheck(L, lua_isstring(L, -1), 1, "table contains non-strings");
    luaL_addvalue(&b);
    if (i != n)
      luaL_addlstring(&b, sep, lsep);
  }
  luaL_pushresult(&b);
  return 1;
}



/*
** {======================================================
** Quicksort
** (based on `Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
*/


static void set2 (lua_State *L, int i, int j) {
  lua_rawseti(L, 1, i);
  lua_rawseti(L, 1, j);
}

static int sort_comp (lua_State *L, int a, int b) {
  if (!lua_isnil(L, 2)) {  /* function? */
    int res;
    lua_pushvalue(L, 2);
    lua_pushvalue(L, a-1);  /* -1 to compensate function */
    lua_pushvalue(L, b-2);  /* -2 to compensate function and `a' */
    lua_call(L, 2, 1);
    res = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return res;
  }
  else  /* a < b? */
    return lua_lessthan(L, a, b);
}

static void auxsort (lua_State *L, int l, int u) {
  while (l < u) {  /* for tail recursion */
    int i, j;
    /* sort elements a[l], a[(l+u)/2] and a[u] */
    lua_rawgeti(L, 1, l);
    lua_rawgeti(L, 1, u);
    if (sort_comp(L, -1, -2))  /* a[u] < a[l]? */
      set2(L, l, u);  /* swap a[l] - a[u] */
    else
      lua_pop(L, 2);
    if (u-l == 1) break;  /* only 2 elements */
    i = (l+u)/2;
    lua_rawgeti(L, 1, i);
    lua_rawgeti(L, 1, l);
    if (sort_comp(L, -2, -1))  /* a[i]<a[l]? */
      set2(L, i, l);
    else {
      lua_pop(L, 1);  /* remove a[l] */
      lua_rawgeti(L, 1, u);
      if (sort_comp(L, -1, -2))  /* a[u]<a[i]? */
        set2(L, i, u);
      else
        lua_pop(L, 2);
    }
    if (u-l == 2) break;  /* only 3 elements */
    lua_rawgeti(L, 1, i);  /* Pivot */
    lua_pushvalue(L, -1);
    lua_rawgeti(L, 1, u-1);
    set2(L, i, u-1);
    /* a[l] <= P == a[u-1] <= a[u], only need to sort from l+1 to u-2 */
    i = l; j = u-1;
    for (;;) {  /* invariant: a[l..i] <= P <= a[j..u] */
      /* repeat ++i until a[i] >= P */
      while (lua_rawgeti(L, 1, ++i), sort_comp(L, -1, -2)) {
        if (i>u) luaL_error(L, "invalid order function for sorting");
        lua_pop(L, 1);  /* remove a[i] */
      }
      /* repeat --j until a[j] <= P */
      while (lua_rawgeti(L, 1, --j), sort_comp(L, -3, -1)) {
        if (j<l) luaL_error(L, "invalid order function for sorting");
        lua_pop(L, 1);  /* remove a[j] */
      }
      if (j<i) {
        lua_pop(L, 3);  /* pop pivot, a[i], a[j] */
        break;
      }
      set2(L, i, j);
    }
    lua_rawgeti(L, 1, u-1);
    lua_rawgeti(L, 1, i);
    set2(L, u-1, i);  /* swap pivot (a[u-1]) with a[i] */
    /* a[l..i-1] <= a[i] == P <= a[i+1..u] */
    /* adjust so that smaller half is in [j..i] and larger one in [l..u] */
    if (i-l < u-i) {
      j=l; i=i-1; l=i+2;
    }
    else {
      j=i+1; i=u; u=j-2;
    }
    auxsort(L, j, i);  /* call recursively the smaller one */
  }  /* repeat the routine for the larger one */
}

static int luaB_sort (lua_State *L) {
  int n = aux_getn(L, 1);
  luaL_checkstack(L, 40, "");  /* assume array is smaller than 2^40 */
  if (!lua_isnoneornil(L, 2))  /* is there a 2nd argument? */
    luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_settop(L, 2);  /* make sure there is two arguments */
  auxsort(L, 1, n);
  return 0;
}

/* }====================================================== */


static const luaL_reg tab_funcs[] = {
  {"concat", str_concat},
  {"foreach", luaB_foreach},
  {"foreachi", luaB_foreachi},
  {"getn", luaB_getn},
  {"setn", luaB_setn},
  {"sort", luaB_sort},
  {"insert", luaB_tinsert},
  {"remove", luaB_tremove},
  {NULL, NULL}
};


LUALIB_API int luaopen_table (lua_State *L) {
  luaL_openlib(L, LUA_TABLIBNAME, tab_funcs, 0);
  return 1;
}

