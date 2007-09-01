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


#endif // SPRING_LUA_INCLUDE
