/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_LUA_INCLUDE
#define SPRING_LUA_INCLUDE

#include <string>
#include <cstring> // strlen
#include <cassert>

#include "LuaUser.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lib/lua/src/lstate.h"
#include "lib/streflop/streflop_cond.h"
#include "System/Log/ILog.h"



///////////////////////////////////////////////////////////////////////////
// A few missing lua_to..., lua_check..., lua_opt...
//

static inline void lua_pushsstring(lua_State* L, const std::string& str)
{
	lua_pushlstring(L, str.data(), str.size());
}


static inline std::string luaL_tosstring(lua_State* L, int index)
{
	size_t len = 0;
	const char* s = lua_tolstring(L, index, &len);
	return std::string(s, len);
}


static inline std::string luaL_checksstring(lua_State* L, int index)
{
	size_t len = 0;
	const char* s = luaL_checklstring(L, index, &len);
	return std::string(s, len);
}


static inline std::string luaL_optsstring(lua_State* L, int index, const std::string& def)
{
	return luaL_opt(L, luaL_checksstring, index, def);
}


static inline bool lua_israwnumber(lua_State* L, int index)
{
	return (lua_type(L, index) == LUA_TNUMBER);
}


static inline bool lua_israwstring(lua_State* L, int index)
{
	return (lua_type(L, index) == LUA_TSTRING);
}


static inline int lua_checkgeti(lua_State* L, int idx, int n)
{
	lua_rawgeti(L, idx, n);
	if (lua_isnoneornil(L, -1)) {
		lua_pop(L, 1);
		return 0;
	}
	return 1;
}


static inline int lua_toint(lua_State* L, int idx)
{
	return (int)lua_tointeger(L, idx);
}


static inline float lua_tofloat(lua_State* L, int idx)
{
	const float n = lua_tonumber(L, idx);
#if defined(DEBUG_LUANAN)
	// Note:
	// luaL_argerror must be called from inside of lua, else it calls exit()
	// so it can't be used in LuaParser::Get...() and similar
	if (math::isinf(n) || math::isnan(n)) luaL_argerror(L, idx, "number expected, got NAN (check your code for div0)");
#endif
	return n;
}


static inline float luaL_checkfloat(lua_State* L, int idx)
{
	return (float)luaL_checknumber(L, idx);
}


static inline float luaL_optfloat(lua_State* L, int idx, float def)
{
	return (float)luaL_optnumber(L, idx, def);
}


static inline bool luaL_checkboolean(lua_State* L, int idx)
{
	luaL_checktype(L, idx, LUA_TBOOLEAN);
	return lua_toboolean(L, idx);
}


static inline bool luaL_optboolean(lua_State* L, int idx, bool def)
{
	return luaL_opt(L, luaL_checkboolean, idx, def);
}



///////////////////////////////////////////////////////////////////////////
// A few custom safety wrappers
//

#undef lua_pop
static inline void lua_pop(lua_State* L, const int args)
{
	assert(args > 0); // prevent lua_pop(L, -1) which is wrong, args is _count_ and not index (use lua_remove for that)
	lua_settop(L, -(args)-1); // from lua.h!
}


static inline int luaS_absIndex(lua_State* L, const int i)
{
	if (i <= 0 && i > LUA_REGISTRYINDEX)
		return lua_gettop(L) + (i) + 1;

	return i;
}


template<typename T>
static inline T luaL_SpringOpt(lua_State* L, int idx, const T def, T(*lua_optFoo)(lua_State*, int, const T), T(*lua_toFoo)(lua_State*, int), int typeFoo, const char* caller)
{
	if (L->errorJmp)
		return (*lua_optFoo)(L, idx, def);

	T ret = (*lua_toFoo)(L, idx);
	if (ret || (lua_type(L, idx) == typeFoo))
		return ret;

	if (!lua_isnoneornil(L, idx))
		LOG_L(L_WARNING, "[%s] wrong type for return argument %d in \"%s::%s\" (%s expected, got %s)", __func__, luaS_absIndex(L, idx), spring_lua_get_handle_name(L), caller, lua_typename(L, typeFoo), luaL_typename(L, idx));

	return def;
}


static inline std::string luaL_SpringOptString(lua_State* L, int idx, const std::string& def, std::string(*lua_optFoo)(lua_State*, int, const std::string&), std::string(*lua_toFoo)(lua_State*, int), int typeFoo, const char* caller)
{
	if (L->errorJmp)
		return (*lua_optFoo)(L, idx, def);

	std::string ret = (*lua_toFoo)(L, idx);
	if (!ret.empty() || (lua_type(L, idx) == typeFoo))
		return ret;

	if (!lua_isnoneornil(L, idx))
		LOG_L(L_WARNING, "[%s(def=%s)] wrong type for return argument %d in \"%s::%s\" (%s expected, got %s)", __func__, def.c_str(), luaS_absIndex(L, idx), spring_lua_get_handle_name(L), caller, lua_typename(L, typeFoo), luaL_typename(L, idx));

	return def;
}


static inline const char* luaL_SpringOptCString(lua_State* L, int idx, const char* def, size_t* len, const char*(*lua_optFoo)(lua_State*, int, const char*, size_t*), const char*(*lua_toFoo)(lua_State*, int, size_t*), int typeFoo, const char* caller)
{
	if (L->errorJmp)
		return (*lua_optFoo)(L, idx, def, len);

	const char* ret = (*lua_toFoo)(L, idx, len);
	if (ret || (lua_type(L, idx) == typeFoo))
		return ret;

	if (!lua_isnoneornil(L, idx))
		LOG_L(L_WARNING, "[%s(def=%s)] wrong type for return argument %d in \"%s::%s\" (%s expected, got %s)", __func__, def, luaS_absIndex(L, idx), spring_lua_get_handle_name(L), caller, lua_typename(L, typeFoo), luaL_typename(L, idx));

	if (len != nullptr)
		*len = strlen(def);

	return def;
}


#define luaL_optboolean(L,idx,def)     (luaL_SpringOpt<bool>(L,idx,def,::luaL_optboolean,lua_toboolean,LUA_TBOOLEAN,__FUNCTION__))
#define luaL_optfloat(L,idx,def)       ((float)luaL_SpringOpt<lua_Number>(L,idx,def,::luaL_optfloat,lua_tofloat,LUA_TNUMBER,__FUNCTION__))
#define luaL_optinteger(L,idx,def)     (luaL_SpringOpt<lua_Integer>(L,idx,def,::luaL_optinteger,lua_tointeger,LUA_TNUMBER,__FUNCTION__))
#define luaL_optlstring(L,idx,def,len) (luaL_SpringOptCString(L,idx,def,len,::luaL_optlstring,lua_tolstring,LUA_TSTRING,__FUNCTION__))
#define luaL_optnumber(L,idx,def)      (luaL_SpringOpt<lua_Number>(L,idx,def,::luaL_optnumber,lua_tonumber,LUA_TNUMBER,__FUNCTION__))

#define luaL_optsstring(L,idx,def)     (luaL_SpringOptString(L,idx,def,::luaL_optsstring,luaL_tosstring,LUA_TSTRING,__FUNCTION__))

#ifdef luaL_optint
	#undef luaL_optint
	#define luaL_optint(L,idx,def) ((int)luaL_SpringOpt<lua_Integer>(L,idx,def,::luaL_optinteger,lua_tointeger,LUA_TNUMBER,__FUNCTION__))
#endif
#ifdef luaL_optstring
	#undef luaL_optstring
	#define luaL_optstring(L,idx,def) (luaL_SpringOptCString(L,idx,def,NULL,::luaL_optlstring,lua_tolstring,LUA_TSTRING,__FUNCTION__))
#endif



///////////////////////////////////////////////////////////////////////////
// State creation & destruction
//

struct luaContextData;

static inline luaContextData* GetLuaContextData(const lua_State* L)
{
	return reinterpret_cast<luaContextData*>(G(L)->ud);
}

static inline lua_State* LUA_OPEN(luaContextData* lcd) {
	return lua_newstate(spring_lua_alloc, lcd); // we want to use our own memory allocator
}

static inline void LUA_CLOSE(lua_State** L) {
	assert((*L) != NULL); lua_close(*L); *L = NULL;
}


static inline void LUA_UNLOAD_LIB(lua_State* L, std::string libname) {
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

#ifdef USING_CREG
#  define SPRING_LUA_OPEN_LIB(L, lib) \
	LUA_OPEN_LIB(L, lib); \
	creg::RegisterCFunction(#lib, lib);
#else
#  define SPRING_LUA_OPEN_LIB(L, lib) LUA_OPEN_LIB(L, lib)
#endif


#endif // SPRING_LUA_INCLUDE
