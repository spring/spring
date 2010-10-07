/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H

#include "Rendering/GL/myGL.h" // for GML_ENABLE_SIM
#include "System/Platform/Synchro.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"

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

#if defined(USE_GML) && GML_ENABLE_SIM
#define DUAL_LUA_STATES 1
#else
#define DUAL_LUA_STATES 0
#endif

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
	UNIT_STOCKPILE_CHANGED
};

enum FeatEvent {
	FEAT_CREATED,
	FEAT_DESTROYED
};

enum ProjEvent {
	PROJ_CREATED,
	PROJ_DESTROYED
};

enum MiscEvent {
	ADD_CONSOLE_LINE
};

struct LuaUnitEvent {
	UnitEvent id;
	const CUnit *unit1, *unit2;
	bool bool1;
	int int1, int2;
	Command cmd1;
	float float1;
	float3 pos1;
	LuaUnitEvent(UnitEvent i, const CUnit *u1) : id(i), unit1(u1) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, const CUnit *u2) : id(i), unit1(u1), unit2(u2) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, const CUnit *u2, bool b1) : id(i), unit1(u1), unit2(u2), bool1(b1) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, int i1) : id(i), unit1(u1), int1(i1) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, const Command &c1) : id(i), unit1(u1), cmd1(c1) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, int i1, int i2) : id(i), unit1(u1), int1(i1), int2(i2) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, const CUnit *u2, float f1, int i1, bool b1) : id(i), unit1(u1), unit2(u2), float1(f1), int1(i1), bool1(b1) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, float f1) : id(i), unit1(u1), float1(f1) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, int i1, const float3& p1, float f1) : id(i), unit1(u1), int1(i1), pos1(p1), float1(f1)  {}
	LuaUnitEvent(UnitEvent i, int i1, const float3& p1, const CUnit *u1) : id(i), int1(i1), pos1(p1), unit1(u1) {}
	LuaUnitEvent(UnitEvent i, const CUnit *u1, const CWeapon *u2, int i1) : id(i), unit1(u1), unit2((CUnit *)u2), int1(i1) {}
};

struct LuaFeatEvent {
	FeatEvent id;
	const CFeature *feat1;
	LuaFeatEvent(FeatEvent i, const CFeature *f1) : id(i), feat1(f1) {}
};

struct LuaProjEvent {
	ProjEvent id;
	const CProjectile *proj1;
	LuaProjEvent(ProjEvent i, const CProjectile *p1) : id(i), proj1(p1) {}
};

struct LuaMiscEvent {
	MiscEvent id;
	std::string str1;
	LuaMiscEvent(MiscEvent i, const std::string &s1) : id(i), str1(s1) {}
};

#if DUAL_LUA_STATES
#define LUA_UNIT_BATCH_PUSH(r,...)\
	if(UseEventBatch() && !execUnitBatch && Threading::IsSimThread()) {\
		GML_STDMUTEX_LOCK(ulbatch);\
		luaUnitEventBatch.push_back(LuaUnitEvent(__VA_ARGS__));\
		return r;\
	}
#define LUA_FEAT_BATCH_PUSH(...)\
	if(UseEventBatch() && !execFeatBatch && Threading::IsSimThread()) {\
		GML_STDMUTEX_LOCK(flbatch);\
		luaFeatEventBatch.push_back(LuaFeatEvent(__VA_ARGS__));\
		return;\
	}
#define LUA_PROJ_BATCH_PUSH(...)\
	if(UseEventBatch() && !execProjBatch && Threading::IsSimThread()) {\
		GML_STDMUTEX_LOCK(plbatch);\
		luaProjEventBatch.push_back(LuaProjEvent(__VA_ARGS__));\
		return;\
	}
#define LUA_FRAME_BATCH_PUSH(...)\
	if(UseEventBatch() && !execFrameBatch && Threading::IsSimThread()) {\
		GML_STDMUTEX_LOCK(glbatch);\
		luaFrameEventBatch.push_back(__VA_ARGS__);\
		return;\
	}
#define LUA_MISC_BATCH_PUSH(r,...)\
	if(UseEventBatch() && !execMiscBatch && Threading::IsSimThread()) {\
		GML_STDMUTEX_LOCK(mlbatch);\
		luaMiscEventBatch.push_back(LuaMiscEvent(__VA_ARGS__));\
		return r;\
	}
#else
#define LUA_UNIT_BATCH_PUSH(r,...)
#define LUA_FEAT_BATCH_PUSH(...)
#define LUA_PROJ_BATCH_PUSH(...)
#define LUA_FRAME_BATCH_PUSH(...)
#define LUA_MISC_BATCH_PUSH(r,...)
#endif


#if DUAL_LUA_STATES
#define BEGIN_ITERATE_LUA_STATES() lua_State *L_Cur = L_Sim; do { lua_State * const L = L_Cur
#define END_ITERATE_LUA_STATES() if(SingleState() || L_Cur == L_Draw) break; L_Cur = L_Draw; } while(true)
#ifndef LUA_SYNCED_ONLY
#define SELECT_LUA_STATE() lua_State * const L = (SingleState() || Threading::IsSimThread()) ? L_Sim : L_Draw
#else
#define SELECT_LUA_STATE()
#endif
#if defined(USE_GML) && GML_ENABLE_SIM

#if defined(LUA_SYNCED_ONLY)
#define GML_DRCMUTEX_LOCK(name) Threading::RecursiveScopedLock name##lock(!CLuaHandle::UseDualStates() ? name##mutex : name##drawmutex, !CLuaHandle::UseDualStates() || !Threading::IsSimThread())
#else
#define GML_DRCMUTEX_LOCK(name) Threading::RecursiveScopedLock name##lock(SingleState() ? name##mutex : name##drawmutex, SingleState() || !SingleState() && !Threading::IsSimThread())
#endif

#else
#define GML_DRCMUTEX_LOCK(name)
#endif

#else

#define BEGIN_ITERATE_LUA_STATES() lua_State * const L = L_Sim
#define END_ITERATE_LUA_STATES()
#ifndef LUA_SYNCED_ONLY
#define SELECT_LUA_STATE() lua_State * const L = L_Sim
#else
#define SELECT_LUA_STATE()
#endif
#define GML_DRCMUTEX_LOCK(name) GML_RECMUTEX_LOCK(name)

#endif

#if DEBUG_LUA
#  define LUA_CALL_IN_CHECK(L) SELECT_LUA_STATE(); LuaCallInCheck ciCheck((L), __FUNCTION__)
#else
#  define LUA_CALL_IN_CHECK(L) SELECT_LUA_STATE()
#endif

#ifdef USE_GML // hack to add some degree of thread safety to LUA
#	include "Rendering/GL/myGL.h"
#	include "lib/gml/gmlsrv.h"
#	if GML_ENABLE_SIM
#		undef LUA_CALL_IN_CHECK
#		if DEBUG_LUA
#			define LUA_CALL_IN_CHECK(L) GML_DRCMUTEX_LOCK(lua); SELECT_LUA_STATE(); GML_CALL_DEBUGGER(); LuaCallInCheck ciCheck((L), __FUNCTION__);
#		else
#			define LUA_CALL_IN_CHECK(L) GML_DRCMUTEX_LOCK(lua); SELECT_LUA_STATE(); GML_CALL_DEBUGGER();
#		endif
#	endif
#endif

#endif /* LUA_CALL_IN_CHECK_H */
