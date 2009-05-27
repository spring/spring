#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H
// LuaCallInCheck.h: interface for the LuaCallInCheck class.
//
//////////////////////////////////////////////////////////////////////


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
#  define LUA_CALL_IN_CHECK(L) LuaCallInCheck ciCheck((L), __FUNCTION__);
#else
#  define LUA_CALL_IN_CHECK(L)
#endif

#ifdef USE_GML // hack to add some degree of thread safety to LUA
#	include "Rendering/GL/myGL.h"
#	include "lib/gml/gmlsrv.h"
#	if GML_ENABLE_SIM
#		undef LUA_CALL_IN_CHECK
#		if DEBUG_LUA
#			define LUA_CALL_IN_CHECK(L) GML_RECMUTEX_LOCK(lua); GML_CALL_DEBUGGER(); LuaCallInCheck ciCheck((L), __FUNCTION__);
#		else
#			define LUA_CALL_IN_CHECK(L) GML_RECMUTEX_LOCK(lua); GML_CALL_DEBUGGER();
#		endif
#	endif
#endif

#endif /* LUA_CALL_IN_CHECK_H */
