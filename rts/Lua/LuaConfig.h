/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONFIG_H
#define LUA_CONFIG_H

#include "lib/gml/gmlcnf.h"

// set to 0 to switch to 0-based weapon
// indices in LuaSynced** API functions
#define LUA_WEAPON_BASE_INDEX 1

#define LUA_MUTEX 1
#define LUA_BATCH 2
#define LUA_STATE 4

#if (defined(USE_GML) && GML_ENABLE_SIM)
#define LUA_MT_OPT ( LUA_MUTEX | LUA_BATCH | LUA_STATE ) // optimizations for faster parallel execution of separate lua_States
#elif defined(USE_LUA_MT)
#define LUA_MT_OPT ( LUA_BATCH | LUA_STATE )
#else
#define LUA_MT_OPT 0
#endif

#endif // LUA_CONFIG_H
