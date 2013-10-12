/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H

#include "LuaEventBatch.h"
#include "System/TimeProfiler.h"

struct lua_State;


//FIXME duplicate with LuaUtils
class LuaCallInCheck {
	public:
		LuaCallInCheck(lua_State* L, const char* funcName);
		~LuaCallInCheck();

	private:
		lua_State* L;
		int startTop;
		const char* funcName;
};


#if DEBUG_LUA
#  define LUA_CALL_IN_CHECK(L, ...) LuaCallInCheck ciCheck((L), __FUNCTION__); SCOPED_TIMER("Lua");
#else
#  define LUA_CALL_IN_CHECK(L, ...) SCOPED_TIMER("Lua");
#endif

#endif /* LUA_CALL_IN_CHECK_H */
