/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_EVENT_BATCH_H
#define LUA_EVENT_BATCH_H

#include "lib/gml/gmlcnf.h"
#include "System/Platform/Synchro.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"
#include "LuaConfig.h"


//FIXME
#define LUA_UNIT_BATCH_PUSH(r, unitEvent)
#define LUA_FEAT_BATCH_PUSH(r, featEvent)
#define LUA_PROJ_BATCH_PUSH(r, projEvent)
#define LUA_FRAME_BATCH_PUSH(r, frameEvent)
#define LUA_LOG_BATCH_PUSH(r, logEvent)
#define LUA_UI_BATCH_PUSH(r, uiEvent)


#if defined(USE_GML) && GML_ENABLE_SIM && GML_CALL_DEBUG
	#define GML_MEASURE_LOCK_TIME(lock) spring_time luatime = spring_gettime(); lock; luatime = spring_gettime() - luatime; gmlLockTime += luatime.toMilliSecsf()
#else
	#define GML_MEASURE_LOCK_TIME(lock) lock
#endif


#if (LUA_MT_OPT & LUA_STATE)
	#if defined(USE_GML) && GML_ENABLE_SIM
		#define GML_DRCMUTEX_LOCK(name) GML_MEASURE_LOCK_TIME(GML_OBJMUTEX_LOCK(name, GML_DRAW|GML_SIM, *GetLuaContextData(L)->))
	#else
		#define GML_DRCMUTEX_LOCK(name)
	#endif
#else
	#define GML_DRCMUTEX_LOCK(name) GML_MEASURE_LOCK_TIME(GML_OBJMUTEX_LOCK(name, GML_DRAW|GML_SIM, *GetLuaContextData(L)->))
#endif

#endif // LUA_EVENT_BATCH_H

