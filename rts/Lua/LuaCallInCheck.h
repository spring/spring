/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H

#include "LuaEventBatch.h"
#include "System/TimeProfiler.h"

struct lua_State;


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
#  define LUA_CALL_IN_CHECK(L, ...) SELECT_LUA_STATE(); LuaCallInCheck ciCheck((L), __FUNCTION__)
#else
#  define LUA_CALL_IN_CHECK(L, ...) SCOPED_TIMER("Lua"); SELECT_LUA_STATE()
#endif

#ifdef USE_GML // hack to add some degree of thread safety to LUA
#	include "lib/gml/gml.h"
#	include "lib/gml/gmlsrv.h"
#	if GML_ENABLE_SIM
#		undef LUA_CALL_IN_CHECK
#		if DEBUG_LUA
#			define LUA_CALL_IN_CHECK(L, ...) SELECT_LUA_STATE(); GML_CHECK_CALL_CHAIN(L, __VA_ARGS__); GML_DRCMUTEX_LOCK(lua); GML_CALL_DEBUGGER(); LuaCallInCheck ciCheck((L), __FUNCTION__);
#		else
#			define LUA_CALL_IN_CHECK(L, ...) SELECT_LUA_STATE(); GML_CHECK_CALL_CHAIN(L, __VA_ARGS__); GML_DRCMUTEX_LOCK(lua); GML_CALL_DEBUGGER();
#		endif
#	endif
#endif

#endif /* LUA_CALL_IN_CHECK_H */
