/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H

#include "System/TimeProfiler.h"

//FIXME duplicate with LuaUtils

#if DEBUG_LUA
#  define LUA_CALL_IN_CHECK(L, ...) LuaUtils::ScopedStackChecker ciCheck((L)); SCOPED_TIMER("Lua");
#else
#  define LUA_CALL_IN_CHECK(L, ...) SCOPED_TIMER("Lua");
#endif

#endif /* LUA_CALL_IN_CHECK_H */
