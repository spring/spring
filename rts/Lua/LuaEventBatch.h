/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_EVENT_BATCH_H
#define LUA_EVENT_BATCH_H

#include "lib/gml/gmlcnf.h"
#include "System/Misc/springTime.h"
#include "LuaConfig.h"


//FIXME
#if defined(USE_GML) && GML_ENABLE_SIM && GML_CALL_DEBUG
	#define GML_MEASURE_LOCK_TIME(lock) spring_time luatime = spring_gettime(); lock; luatime = spring_gettime() - luatime; gmlLockTime += luatime.toMilliSecsf()
#else
	#define GML_MEASURE_LOCK_TIME(lock) lock
#endif

#define GML_DRCMUTEX_LOCK(name) GML_MEASURE_LOCK_TIME(GML_OBJMUTEX_LOCK(name, GML_DRAW|GML_SIM, *GetLuaContextData(L)->))

#endif // LUA_EVENT_BATCH_H

