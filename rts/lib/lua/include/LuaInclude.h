#ifndef SPRING_LUA_INCLUDE
#define SPRING_LUA_INCLUDE

#include <string>
#include <lua.hpp>
#include "lib/streflop/streflop_cond.h"
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

#define lua_toboolean(L,v) ((bool)lua_toboolean(L,v))


inline float luaL_optfloat(lua_State* L, int idx, float def)
{
	return (float)luaL_optnumber(L, idx, def);
}

inline bool luaL_optboolean(lua_State* L, int idx, bool def)
{
	return lua_isboolean(L, idx) ? lua_toboolean(L, idx) : def;
}


#ifndef SPRING_LUA
typedef int lua_Hash;

inline lua_Hash lua_calchash(const char *s, size_t l)
{
	return 0;
}

inline void lua_pushhstring(lua_State *L, lua_Hash h, const char *s, size_t l)
{
	lua_pushlstring(L, s, l);
}

typedef FILE* (*lua_Func_fopen)(lua_State* L, const char* path, const char* mode);
typedef FILE* (*lua_Func_popen)(lua_State* L, const char* command, const char* type);
typedef int   (*lua_Func_pclose)(lua_State* L, FILE* stream);
typedef int   (*lua_Func_system)(lua_State* L, const char* command);
typedef int   (*lua_Func_remove)(lua_State* L, const char* pathname);
typedef int   (*lua_Func_rename)(lua_State* L, const char* oldpath, const char* newpath);
inline void lua_set_fopen(lua_State* L, lua_Func_fopen) {}
inline void lua_set_popen(lua_State* L, lua_Func_popen, lua_Func_pclose) {}
inline void lua_set_system(lua_State* L, lua_Func_system) {}
inline void lua_set_remove(lua_State* L, lua_Func_remove) {}
inline void lua_set_rename(lua_State* L, lua_Func_rename) {}
#endif


/*struct luaContextData;
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
}*/

#endif // SPRING_LUA_INCLUDE
