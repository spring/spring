#include "lib/gml/gmlcnf.h"

#define LUA_MUTEX 1
#define LUA_BATCH 2
#define LUA_STATE 4
#if (defined(USE_GML) && GML_ENABLE_SIM) || defined(USE_LUA_MT)
#define LUA_MT_OPT ( LUA_MUTEX | LUA_BATCH | LUA_STATE ) // optimizations for faster parallel execution of separate lua_States
#else
#define LUA_MT_OPT 0
#endif

