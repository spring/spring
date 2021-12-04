/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H

#include "System/TimeProfiler.h"
#include "LuaUtils.h"

#if DEBUG_LUA
#  define LUA_CALL_IN_CHECK(L, ...) LuaUtils::ScopedStackChecker ciCheck((L)); SCOPED_SPECIAL_TIMER("Lua");
#else
#  define LUA_CALL_IN_CHECK(L, ...) SCOPED_SPECIAL_TIMER("Lua");
#endif

#endif /* LUA_CALL_IN_CHECK_H */
