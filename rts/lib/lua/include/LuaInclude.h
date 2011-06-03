#ifndef SPRING_LUA_INCLUDE
#define SPRING_LUA_INCLUDE

#include <string>
#include "lua.h"
#include "lib/lua/src/lstate.h"
#include "lualib.h"
#include "lauxlib.h"
#include <boost/thread/recursive_mutex.hpp>


inline void lua_pushsstring(lua_State* L, const std::string& str)
{
	lua_pushlstring(L, str.data(), str.size());
}


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

struct luaContextData;
extern boost::recursive_mutex* getLuaMutex(bool userMode, bool primary);

inline lua_State *LUA_OPEN(luaContextData* lcd = NULL, bool userMode = true, bool primary = true) {
	lua_State *L_New = lua_open();
	L_New->lcd = lcd;
	L_New->luamutex = getLuaMutex(userMode, primary);
	return L_New;
}

inline void LUA_CLOSE(lua_State *L_Old) {
	if(L_Old->luamutex != getLuaMutex(false, false) && L_Old->luamutex != getLuaMutex(false, true))
		delete L_Old->luamutex;
	lua_close(L_Old);
}

#endif // SPRING_LUA_INCLUDE
