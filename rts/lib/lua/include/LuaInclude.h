#ifndef SPRING_LUA_INCLUDE
#define SPRING_LUA_INCLUDE


#include "lua.h"
#include "lib/lua/src/lstate.h"
#include "lualib.h"
#include "lauxlib.h"
#include <boost/thread/recursive_mutex.hpp>


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


inline int lua_toint(lua_State* L, int idx)
{
  return (int)lua_tointeger(L, idx);
}


inline float lua_tofloat(lua_State* L, int idx)
{
  return (float)lua_tonumber(L, idx);
}


inline float luaL_checkfloat(lua_State* L, int idx)
{
  return (float)luaL_checknumber(L, idx);
}


inline float luaL_optfloat(lua_State* L, int idx, float def)
{
  return (float)luaL_optnumber(L, idx, def);
}

extern boost::recursive_mutex luaprimmutex, luasecmutex;
struct luaContextData;

inline lua_State *LUA_OPEN(luaContextData* lcd = NULL, bool userMode = true, bool primary = true) {
	lua_State *L_New = lua_open();
	L_New->lcd = lcd;
	if(userMode)
		L_New->luamutex = new boost::recursive_mutex();
	else // LuaGaia & LuaRules will share mutexes to avoid deadlocks during XCalls etc.
		L_New->luamutex = primary ? &luaprimmutex : &luasecmutex;
	return L_New;
}

inline void LUA_CLOSE(lua_State *L_Old) {
	if(L_Old->luamutex != &luaprimmutex && L_Old->luamutex != &luasecmutex)
		delete L_Old->luamutex;
	lua_close(L_Old);
}

#endif // SPRING_LUA_INCLUDE
