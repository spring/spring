/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CALL_IN_CHECK_H
#define LUA_CALL_IN_CHECK_H

#include "Rendering/GL/myGL.h" // for GML_ENABLE_SIM
#include "System/Platform/Synchro.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/Unit.h"

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

#define UNSYNCED_SINGLE_LUA_STATE 1

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
	UNIT_EXPLOSION
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
};

#if DUAL_LUA_STATES && UNSYNCED_SINGLE_LUA_STATE
#define LUA_UNIT_BATCH_PUSH(...)\
	if(Threading::IsSimThread()) {\
		luaUnitEventBatch.push_back(LuaUnitEvent(__VA_ARGS__));\
		if(SingleState())\
			return;\
	}
#else
#define LUA_UNIT_BATCH_PUSH(...)
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

#if defined(LUA_SYNCED_ONLY) || !UNSYNCED_SINGLE_LUA_STATE
#define GML_DRCMUTEX_LOCK(name) Threading::RecursiveScopedLock name##drawlock(name##drawmutex, !Threading::IsSimThread())
#else
#define GML_DRCMUTEX_LOCK(name) Threading::RecursiveScopedLock name##drawlock(name##drawmutex, !SingleState() && !Threading::IsSimThread());\
								Threading::RecursiveScopedLock name##lock(name##mutex, SingleState())
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
