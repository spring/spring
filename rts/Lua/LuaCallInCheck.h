/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H

#include "System/Platform/Synchro.h"

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

#if defined(USE_GML) && GML_ENABLE_SIM
#define DUAL_LUA_STATES 1
#else
#define DUAL_LUA_STATES 0
#endif

#if DUAL_LUA_STATES
#define BEGIN_ITERATE_LUA_STATES() lua_State *L_Cur = L_Sim; do { lua_State * const L = L_Cur
#define END_ITERATE_LUA_STATES() if(L_Cur == L_Draw) break; L_Cur = L_Draw; } while(true)
#ifndef LUA_SYNCED_ONLY
#define SELECT_LUA_STATE() lua_State * const L = Threading::IsSimThread() ? L_Sim : L_Draw
#else
#define SELECT_LUA_STATE()
#endif
#if defined(USE_GML) && GML_ENABLE_SIM
#define GML_DRCMUTEX_LOCK(name) Threading::RecursiveScopedLock name##lock(name##drawmutex, !Threading::IsSimThread())
#else
#define GML_DRCMUTEX_LOCK(name)
#endif

#else

#define BEGIN_ITERATE_LUA_STATES() lua_State * const L = L_Sim
#define END_ITERATE_LUA_STATES()
#ifndef LUA_SYNCED_ONLY
#define SELECT_LUA_STATE() lua_State * const L = L_Sim
#else
#define SELECT_LUA_STATE()
#endif
#define GML_DRCMUTEX_LOCK(name) GML_RECMUTEX_LOCK(name)
#endif

#if DEBUG_LUA
#  define LUA_CALL_IN_CHECK(L) SELECT_LUA_STATE(); LuaCallInCheck ciCheck((L), __FUNCTION__)
#else
#  define LUA_CALL_IN_CHECK(L) SELECT_LUA_STATE()
#endif

#ifdef USE_GML // hack to add some degree of thread safety to LUA
#	include "Rendering/GL/myGL.h"
#	include "lib/gml/gmlsrv.h"
#	if GML_ENABLE_SIM
#		undef LUA_CALL_IN_CHECK
#		if DEBUG_LUA
#			define LUA_CALL_IN_CHECK(L) GML_DRCMUTEX_LOCK(lua); SELECT_LUA_STATE(); GML_CALL_DEBUGGER(); LuaCallInCheck ciCheck((L), __FUNCTION__);
#		else
#			define LUA_CALL_IN_CHECK(L) GML_DRCMUTEX_LOCK(lua); SELECT_LUA_STATE(); GML_CALL_DEBUGGER();
#		endif
#	endif
#endif

#endif /* LUA_CALL_IN_CHECK_H */
