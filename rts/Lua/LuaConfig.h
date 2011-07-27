/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONFIG_H
#define LUA_CONFIG_H

#include "lib/gml/gmlcnf.h"

#define LUA_MUTEX 1
#define LUA_BATCH 2
#define LUA_STATE 4
#if (defined(USE_GML) && GML_ENABLE_SIM) || defined(USE_LUA_MT)
#define LUA_MT_OPT ( LUA_MUTEX | LUA_BATCH | LUA_STATE ) // optimizations for faster parallel execution of separate lua_States
#else
#define LUA_MT_OPT 0
#endif

enum {
	MT_LUA_FIRST = 0,
	MT_LUA_DEFAULT = 0,
	MT_LUA_NONE = 0,
	MT_LUA_FIRSTACTIVE = 1,
	MT_LUA_SINGLE = 1,
	MT_LUA_SINGLE_BATCH,
	MT_LUA_DUAL_EXPORT,
	MT_LUA_DUAL,
	MT_LUA_DUAL_ALL,
	MT_LUA_SIZE,
	MT_LUA_LAST = MT_LUA_SIZE - 1
};
// MT_LUA_DEFAULT: Use 'luaThreadingModel' setting from modInfo (default)
// MT_LUA_NONE: N/A (single threaded)
// MT_LUA_SINGLE: Single state
// MT_LUA_SINGLE_BATCH: Single state, batching of unsynced events
// MT_LUA_DUAL_EXPORT: Dual states for synced, batching of unsynced events, synced/unsynced communication via EXPORT table and SendToUnsynced
// MT_LUA_DUAL: Dual states for synced, batching of unsynced events, synced/unsynced communication via SendToUnsynced only
// MT_LUA_DUAL_ALL: Dual states for all, all synced/unsynced communication (widgets included) via SendToUnsynced only

#endif // LUA_CONFIG_H
