/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H

#include "System/TimeProfiler.h"
#include "LuaUtils.h"

#if DEBUG_LUA
#  define LUA_CALL_IN_CHECK_NAMED(L, name, ...) SCOPED_SPECIAL_TIMER_NOREG(name); LuaUtils::ScopedStackChecker ciCheck((L));
#else
#  define LUA_CALL_IN_CHECK_NAMED(L, name, ...) SCOPED_SPECIAL_TIMER_NOREG(name);
#endif

#define LUA_CALL_IN_CHECK(L, ...) LUA_CALL_IN_CHECK_NAMED(L, (GetLuaContextData(L)->synced)? "Lua::Callins::Synced": "Lua::Callins::Unsynced", __VA_ARGS__);
#endif /* LUA_CALL_IN_CHECK_H */
