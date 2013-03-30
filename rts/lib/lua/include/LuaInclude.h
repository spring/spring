#ifndef SPRING_LUA_INCLUDE
#define SPRING_LUA_INCLUDE

#include <string>
#include "lua.h"
#include "lib/lua/src/lstate.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lib/streflop/streflop_cond.h"
#include "LuaUser.h"
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
  const float n = lua_tonumber(L, idx);
#ifdef DEBUG
  if (math::isinf(n) || math::isnan(n)) luaL_argerror(L, idx, "number expected, got NAN (check your code for div0)");
  //assert(!math::isinf(d));
  //assert(!math::isnan(d));
#endif
  return n;
}


inline float luaL_checkfloat(lua_State* L, int idx)
{
  return (float)luaL_checknumber(L, idx);
}


inline float luaL_optfloat(lua_State* L, int idx, float def)
{
  return (float)luaL_optnumber(L, idx, def);
}

inline bool luaL_optboolean(lua_State* L, int idx, bool def)
{
  return lua_isboolean(L, idx) ? lua_toboolean(L, idx) : def;
}

struct luaContextData;
extern boost::recursive_mutex* getLuaMutex(bool userMode, bool primary);

inline lua_State* LUA_OPEN(luaContextData* lcd = NULL, bool userMode = true, bool primary = true) {
	//lua_State* L = lua_open();
	lua_State* L = lua_newstate(spring_lua_alloc, NULL);
	L->lcd = lcd;
	L->luamutex = getLuaMutex(userMode, primary);
	return L;
}

inline void LUA_CLOSE(lua_State *L_Old) {
	if(L_Old->luamutex != getLuaMutex(false, false) && L_Old->luamutex != getLuaMutex(false, true))
		delete L_Old->luamutex;
	lua_close(L_Old);
}


inline void LUA_UNLOAD_LIB(lua_State* L, std::string libname) {
	luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 1);
	lua_pushsstring(L, libname);
	lua_pushnil(L);
	lua_rawset(L, -3);

	lua_pushnil(L); lua_setglobal(L, libname.c_str());
}


#if (LUA_VERSION_NUM < 500)
#  define LUA_OPEN_LIB(L, lib) lib(L)
#else
#  define LUA_OPEN_LIB(L, lib) \
     lua_pushcfunction((L), lib); \
     lua_pcall((L), 0, 0, 0);
#endif


#endif // SPRING_LUA_INCLUDE
