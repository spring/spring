#ifndef SPRING_LUA_INCLUDE
#define SPRING_LUA_INCLUDE


#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


inline bool lua_israwnumber(lua_State* L, int index)
{
  return (lua_type(L, index) == LUA_TNUMBER);
}


inline bool lua_israwstring(lua_State* L, int index)
{
  return (lua_type(L, index) == LUA_TSTRING);
}


inline int lua_checkgeti(lua_State* L, int idx, int n)
{
  lua_rawgeti(L, idx, n);
  if (lua_isnoneornil(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  return 1;
}


#endif // SPRING_LUA_INCLUDE
