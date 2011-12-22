/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_EVENT_BATCH_H
#define LUA_EVENT_BATCH_H

#include "Rendering/GL/myGL.h"
#include "System/Platform/Synchro.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"
#include "LuaConfig.h"

enum UnitEvent {
	UNIT_FINISHED,
	UNIT_CREATED,
	UNIT_FROM_FACTORY,
	UNIT_DESTROYED,
	UNIT_TAKEN,
	UNIT_GIVEN,
	UNIT_IDLE,
	UNIT_COMMAND,
	UNIT_CMD_DONE,
	UNIT_DAMAGED,
	UNIT_EXPERIENCE,
	UNIT_SEISMIC_PING,
	UNIT_ENTERED_RADAR,
	UNIT_ENTERED_LOS,
	UNIT_LEFT_RADAR,
	UNIT_LEFT_LOS,
	UNIT_LOADED,
	UNIT_UNLOADED,
	UNIT_ENTERED_WATER,
	UNIT_ENTERED_AIR,
	UNIT_LEFT_WATER,
	UNIT_LEFT_AIR,
	UNIT_CLOAKED,
	UNIT_DECLOAKED,
	UNIT_MOVE_FAILED,
	UNIT_EXPLOSION,
	UNIT_UNIT_COLLISION,
	UNIT_STOCKPILE_CHANGED
};

enum FeatEvent {
	FEAT_CREATED,
	FEAT_DESTROYED
};

enum ObjEvent {
	UNIT_FEAT_COLLISION
};

enum ProjEvent {
	PROJ_CREATED,
	PROJ_DESTROYED
};

enum LogEvent {
	ADD_CONSOLE_LINE
};

enum UIEvent {
	SHOCK_FRONT
};

struct LuaUnitEvent {
	LuaUnitEvent(UnitEvent i, const CUnit* u1)
		: id(i)
		, unit1(u1)
		, unit2(NULL)
		, bool1(false)
		, int1(0)
		, int2(0)
		, float1(0.0f)
		, pos1(ZeroVector)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, const CUnit* u2)
		: id(i)
		, unit1(u1)
		, unit2(u2)
		, bool1(false)
		, int1(0)
		, int2(0)
		, float1(0.0f)
		, pos1(ZeroVector)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, const CUnit* u2, bool b1)
		: id(i)
		, unit1(u1)
		, unit2(u2)
		, bool1(b1)
		, int1(0)
		, int2(0)
		, float1(0.0f)
		, pos1(ZeroVector)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, int i1)
		: id(i)
		, unit1(u1)
		, unit2(NULL)
		, bool1(false)
		, int1(i1)
		, int2(0)
		, float1(0.0f)
		, pos1(ZeroVector)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, const Command& c1)
		: id(i)
		, unit1(u1)
		, unit2(NULL)
		, bool1(false)
		, int1(0)
		, int2(0)
		, cmd1(c1)
		, float1(0.0f)
		, pos1(ZeroVector)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, int i1, int i2)
		: id(i)
		, unit1(u1)
		, unit2(NULL)
		, bool1(false)
		, int1(i1)
		, int2(i2)
		, float1(0.0f)
		, pos1(ZeroVector)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, const CUnit* u2, float f1, int i1, bool b1)
		: id(i)
		, unit1(u1)
		, unit2(u2)
		, bool1(b1)
		, int1(i1)
		, int2(0)
		, float1(f1)
		, pos1(ZeroVector)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, float f1)
		: id(i)
		, unit1(u1)
		, unit2(NULL)
		, bool1(false)
		, int1(0)
		, int2(0)
		, float1(f1)
		, pos1(ZeroVector)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, int i1, const float3& p1, float f1)
		: id(i)
		, unit1(u1)
		, unit2(NULL)
		, bool1(false)
		, int1(i1)
		, int2(0)
		, float1(f1)
		, pos1(p1)
	{}
	LuaUnitEvent(UnitEvent i, int i1, const float3& p1, const CUnit* u1)
		: id(i)
		, unit1(u1)
		, unit2(NULL)
		, bool1(false)
		, int1(i1)
		, int2(0)
		, float1(0.0f)
		, pos1(p1)
	{}
	LuaUnitEvent(UnitEvent i, const CUnit* u1, const CWeapon* u2, int i1)
		: id(i)
		, unit1(u1)
		, unit2((CUnit*)u2)
		, bool1(false)
		, int1(i1)
		, int2(0)
		, float1(0.0f)
		, pos1(ZeroVector)
	{}

	UnitEvent id;
	const CUnit* unit1;
	const CUnit* unit2;
	bool bool1;
	int int1;
	int int2;
	Command cmd1;
	float float1;
	float3 pos1;
};

struct LuaFeatEvent {
	LuaFeatEvent(FeatEvent i, const CFeature* f1)
		: id(i)
		, feat1(f1)
	{}

	FeatEvent id;
	const CFeature* feat1;
};

struct LuaObjEvent {
	LuaObjEvent(ObjEvent i, const CUnit* u, const CFeature* f)
		: id(i)
		, unit(u)
		, feat(f)
	{}

	ObjEvent id;
	const CUnit* unit;
	const CFeature* feat;
};

struct LuaProjEvent {
	LuaProjEvent(ProjEvent i, const CProjectile* p1)
		: id(i)
		, proj1(p1)
	{}

	ProjEvent id;
	const CProjectile* proj1;
};

struct LuaLogEvent {
	LuaLogEvent(LogEvent id, const std::string& msg, int level, const std::string& section)
		: id(id)
		, msg(msg)
		, level(level)
		, section(section)
	{}

	LogEvent id;
	std::string msg;
	int level;
	std::string section;
};

struct LuaUIEvent {
	LuaUIEvent(UIEvent i, const float f1, const float3& p1, const float f2, const float f3)
		: id(i)
		, flt1(f1)
		, flt2(f2)
		, flt3(f3)
		, pos1(p1)
	{}

	UIEvent id;
	float flt1;
	float flt2;
	float flt3;
	float3 pos1;
};

#if (LUA_MT_OPT & LUA_BATCH)
	#define LUA_UNIT_BATCH_PUSH(r,...)\
		if(UseEventBatch() && Threading::IsBatchThread()) {\
			GML_STDMUTEX_LOCK(ulbatch);\
			luaUnitEventBatch.push_back(LuaUnitEvent(__VA_ARGS__));\
			return r;\
		}
	#define LUA_FEAT_BATCH_PUSH(...)\
		if(UseEventBatch() && Threading::IsBatchThread()) {\
			GML_STDMUTEX_LOCK(flbatch);\
			luaFeatEventBatch.push_back(LuaFeatEvent(__VA_ARGS__));\
			return;\
		}
	#define LUA_OBJ_BATCH_PUSH(...)\
		if(UseEventBatch() && Threading::IsBatchThread()) {\
			GML_STDMUTEX_LOCK(olbatch);\
			luaObjEventBatch.push_back(LuaObjEvent(__VA_ARGS__));\
			return;\
		}
	#define LUA_PROJ_BATCH_PUSH(...)\
		if(UseEventBatch() && Threading::IsBatchThread()) {\
			GML_STDMUTEX_LOCK(plbatch);\
			luaProjEventBatch.push_back(LuaProjEvent(__VA_ARGS__));\
			return;\
		}
	#define LUA_FRAME_BATCH_PUSH(...)\
		if(UseEventBatch() && Threading::IsBatchThread()) {\
			GML_STDMUTEX_LOCK(glbatch);\
			luaFrameEventBatch.push_back(__VA_ARGS__);\
			return;\
		}
	#define LUA_LOG_BATCH_PUSH(r,...)\
		if(UseEventBatch() && Threading::IsBatchThread()) {\
			GML_STDMUTEX_LOCK(mlbatch);\
			luaLogEventBatch.push_back(LuaLogEvent(__VA_ARGS__));\
			return r;\
		}
	#define LUA_UI_BATCH_PUSH(...)\
		if(UseEventBatch() && Threading::IsBatchThread()) {\
			GML_STDMUTEX_LOCK(llbatch);\
			luaUIEventBatch.push_back(LuaUIEvent(__VA_ARGS__));\
			return;\
		}
#else
	#define LUA_UNIT_BATCH_PUSH(r,...)
	#define LUA_FEAT_BATCH_PUSH(...)
	#define LUA_OBJ_BATCH_PUSH(...)
	#define LUA_PROJ_BATCH_PUSH(...)
	#define LUA_FRAME_BATCH_PUSH(...)
	#define LUA_LOG_BATCH_PUSH(r,...)
	#define LUA_UI_BATCH_PUSH(...)
#endif

#if defined(USE_GML) && GML_ENABLE_SIM && GML_CALL_DEBUG
	#define GML_MEASURE_LOCK_TIME(lock) unsigned luatime = SDL_GetTicks(); lock; luatime = SDL_GetTicks() - luatime; if(luatime >= 1) gmlLockTime += luatime
#else
	#define GML_MEASURE_LOCK_TIME(lock) lock
#endif

#if (LUA_MT_OPT & LUA_STATE)
	#define BEGIN_ITERATE_LUA_STATES() lua_State* L_Cur = L_Sim; do { lua_State * const L = L_Cur
	#define END_ITERATE_LUA_STATES() if(SingleState() || L_Cur == L_Draw) break; L_Cur = L_Draw; } while(true)
	#ifndef LUA_SYNCED_ONLY
		#define SELECT_LUA_STATE() lua_State * const L = (SingleState() || Threading::IsSimThread()) ? L_Sim : L_Draw
		#define SELECT_UNSYNCED_LUA_STATE() lua_State * const L = SingleState() ? L_Sim : L_Draw
	#else
		#define SELECT_LUA_STATE()
		#define SELECT_UNSYNCED_LUA_STATE()
	#endif
	#if defined(USE_GML) && GML_ENABLE_SIM
		#if GML_DEBUG_MUTEX
			#define GML_LOCK_TEST(nme) { std::map<std::string, int>::iterator locki = lockmaps[GML::ThreadNumber()].find(#nme); if((lockmi==lockmmaps[GML::ThreadNumber()].end() || (*lockmi).second == 0) && locki!=lockmaps[GML::ThreadNumber()].end() && (*locki).second>0) while(1); }
			#define GML_DEBUG_LOCK(name) { GML_STDMUTEX_LOCK(lm); std::map<boost::recursive_mutex*, int>::iterator lockmi = lockmmaps[GML::ThreadNumber()].find(L->##name##mutex); GML_LOCK_TEST(sel); GML_LOCK_TEST(group); GML_LOCK_TEST(grpsel); GML_LOCK_TEST(gui); GML_LOCK_TEST(quad); GML_LOCK_TEST(qnum); }
		#else
			#define GML_DEBUG_LOCK(name)
		#endif
		#define GML_DRCMUTEX_LOCK(name) GML_DEBUG_LOCK(name); GML_MEASURE_LOCK_TIME(GML_OBJMUTEX_LOCK(name, GML_DRAW|GML_SIM, *L->))
	#else
		#define GML_DRCMUTEX_LOCK(name)
	#endif
#else
	#define BEGIN_ITERATE_LUA_STATES() lua_State * const L = L_Sim
	#define END_ITERATE_LUA_STATES()
	#ifndef LUA_SYNCED_ONLY
		#define SELECT_LUA_STATE() lua_State * const L = L_Sim
		#define SELECT_UNSYNCED_LUA_STATE() lua_State * const L = L_Sim
	#else
		#define SELECT_LUA_STATE()
		#define SELECT_UNSYNCED_LUA_STATE()
	#endif
	#define GML_DRCMUTEX_LOCK(name) GML_MEASURE_LOCK_TIME(GML_OBJMUTEX_LOCK(name, GML_DRAW|GML_SIM, *L->))
#endif

#if defined(USE_GML) && GML_ENABLE_SIM
	#define GML_SELECT_LUA_STATE() SELECT_LUA_STATE()
#else
	#define GML_SELECT_LUA_STATE()
#endif

#endif // LUA_EVENT_BATCH_H

