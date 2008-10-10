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

#ifdef USE_GML
#include "Rendering/GL/myGL.h"
#include "lib/gml/gmlsrv.h"
#	if GML_MT_TEST
#include <boost/thread/recursive_mutex.hpp>
extern boost::recursive_mutex luamutex;
#undef LUA_CALL_IN_CHECK
#define LUA_CALL_IN_CHECK(L) boost::recursive_mutex::scoped_lock lualock(luamutex);
#	endif
#endif

#endif /* LUA_CALL_IN_CHECK_H */
