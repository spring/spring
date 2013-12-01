/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_EVENT_BATCH_H
#define LUA_EVENT_BATCH_H

#include "lib/gml/gmlcnf.h"
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
	UNIT_FEAT_COLLISION,
	UNIT_STOCKPILE_CHANGED
};

enum FeatEvent {
	FEAT_CREATED,
	FEAT_DESTROYED,
	FEAT_DAMAGED
};

enum ProjEvent {
	PROJ_CREATED,
	PROJ_DESTROYED
};

enum LogEvent {
	LOG_CONSOLE_LINE
};

enum UIEvent {
	SHOCK_FRONT
};


struct LuaUnitEventBase {
public:
	LuaUnitEventBase(UnitEvent eventID): id(eventID) {}
	UnitEvent GetID() const { return id; }
protected:
	UnitEvent id;
};

// TODO: FINISH ME
struct LuaUnitCreatedEvent: public LuaUnitEventBase { LuaUnitCreatedEvent(): LuaUnitEventBase(UNIT_CREATED) {} };
struct LuaUnitFinishedEvent: public LuaUnitEventBase { LuaUnitFinishedEvent(): LuaUnitEventBase(UNIT_FINISHED) {} };
struct LuaUnitFromFactoryEvent: public LuaUnitEventBase { LuaUnitFromFactoryEvent(): LuaUnitEventBase(UNIT_FROM_FACTORY) {} };
struct LuaUnitDestroyedEvent: public LuaUnitEventBase { LuaUnitDestroyedEvent(): LuaUnitEventBase(UNIT_DESTROYED) {} };
struct LuaUnitTakenEvent: public LuaUnitEventBase { LuaUnitTakenEvent(): LuaUnitEventBase(UNIT_TAKEN) {} };
struct LuaUnitGivenEvent: public LuaUnitEventBase { LuaUnitGivenEvent(): LuaUnitEventBase(UNIT_GIVEN) {} };
struct LuaUnitIdleEvent: public LuaUnitEventBase { LuaUnitIdleEvent(): LuaUnitEventBase(UNIT_IDLE) {} };
struct LuaUnitCommandEvent: public LuaUnitEventBase { LuaUnitCommandEvent(): LuaUnitEventBase(UNIT_COMMAND) {} };
struct LuaUnitCommandDoneEvent: public LuaUnitEventBase { LuaUnitCommandDoneEvent(): LuaUnitEventBase(UNIT_CMD_DONE) {} };
struct LuaUnitDamagedEvent: public LuaUnitEventBase { LuaUnitDamagedEvent(): LuaUnitEventBase(UNIT_DAMAGED) {} };
struct LuaUnitExperienceEvent: public LuaUnitEventBase { LuaUnitExperienceEvent(): LuaUnitEventBase(UNIT_EXPERIENCE) {} };
struct LuaUnitSeismicPingEvent: public LuaUnitEventBase { LuaUnitSeismicPingEvent(): LuaUnitEventBase(UNIT_SEISMIC_PING) {} };
struct LuaUnitEnteredRadarEvent: public LuaUnitEventBase { LuaUnitEnteredRadarEvent(): LuaUnitEventBase(UNIT_ENTERED_RADAR) {} };
struct LuaUnitEnteredLosEvent: public LuaUnitEventBase { LuaUnitEnteredLosEvent(): LuaUnitEventBase(UNIT_ENTERED_LOS) {} };
struct LuaUnitLeftRadarEvent: public LuaUnitEventBase { LuaUnitLeftRadarEvent(): LuaUnitEventBase(UNIT_LEFT_RADAR) {} };
struct LuaUnitLeftLosEvent: public LuaUnitEventBase { LuaUnitLeftLosEvent(): LuaUnitEventBase(UNIT_LEFT_LOS) {} };
struct LuaUnitLoadedEvent: public LuaUnitEventBase { LuaUnitLoadedEvent(): LuaUnitEventBase(UNIT_LOADED) {} };
struct LuaUnitUnloadedEvent: public LuaUnitEventBase { LuaUnitUnloadedEvent(): LuaUnitEventBase(UNIT_UNLOADED) {} };
struct LuaUnitEnteredWaterEvent: public LuaUnitEventBase { LuaUnitEnteredWaterEvent(): LuaUnitEventBase(UNIT_ENTERED_WATER) {} };
struct LuaUnitEnteredAirEvent: public LuaUnitEventBase { LuaUnitEnteredAirEvent(): LuaUnitEventBase(UNIT_ENTERED_AIR) {} };
struct LuaUnitLeftWaterEvent: public LuaUnitEventBase { LuaUnitLeftWaterEvent(): LuaUnitEventBase(UNIT_LEFT_WATER) {} };
struct LuaUnitLeftAirEvent: public LuaUnitEventBase { LuaUnitLeftAirEvent(): LuaUnitEventBase(UNIT_LEFT_AIR) {} };
struct LuaUnitCloakedEvent: public LuaUnitEventBase { LuaUnitCloakedEvent(): LuaUnitEventBase(UNIT_CLOAKED) {} };
struct LuaUnitDecloakedEvent: public LuaUnitEventBase { LuaUnitDecloakedEvent(): LuaUnitEventBase(UNIT_DECLOAKED) {} };
struct LuaUnitUnitCollisionEvent: public LuaUnitEventBase { LuaUnitUnitCollisionEvent(): LuaUnitEventBase(UNIT_UNIT_COLLISION) {} };
struct LuaUnitFeatureCollisionEvent: public LuaUnitEventBase { LuaUnitFeatureCollisionEvent(): LuaUnitEventBase(UNIT_FEAT_COLLISION) {} };
struct LuaUnitMoveFailedEvent: public LuaUnitEventBase { LuaUnitMoveFailedEvent(): LuaUnitEventBase(UNIT_MOVE_FAILED) {} };
struct LuaUnitExplosionEvent: public LuaUnitEventBase { LuaUnitExplosionEvent(): LuaUnitEventBase(UNIT_EXPLOSION) {} }; // ?
struct LuaUnitStockpileChangedEvent: public LuaUnitEventBase { LuaUnitStockpileChangedEvent(): LuaUnitEventBase(UNIT_STOCKPILE_CHANGED) {} };



struct LuaFeatEventBase {
public:
	LuaFeatEventBase(FeatEvent eventID): id(eventID) {}
	FeatEvent GetID() const { return id; }
protected:
	FeatEvent id;
};

struct LuaFeatureCreatedEvent: public LuaFeatEventBase {
public:
	LuaFeatureCreatedEvent(const CFeature* f): LuaFeatEventBase(FEAT_CREATED) {
		feature = f;
	}
	const CFeature* GetFeature() const { return feature; }
private:
	const CFeature* feature;
};

struct LuaFeatureDestroyedEvent: public LuaFeatEventBase {
public:
	LuaFeatureDestroyedEvent(const CFeature* f): LuaFeatEventBase(FEAT_DESTROYED) {
		feature = f;
	}
	const CFeature* GetFeature() const { return feature; }
private:
	const CFeature* feature;
};

struct LuaFeatureDamagedEvent: public LuaFeatEventBase {
public:
	LuaFeatureDamagedEvent(const CFeature* f, const CUnit* a, float d, int wdID, int pID): LuaFeatEventBase(FEAT_DAMAGED) {
		feature = f;
		attacker = a;
		damage = d;
		weaponDefID = wdID;
		projectileID = pID;
	}

	const CFeature* GetFeature() const { return feature; }
	const CUnit* GetAttacker() const { return attacker; }
	float GetDamage() const { return damage; }
	int GetWeaponDefID() const { return weaponDefID; }
	int GetProjectileID() const { return projectileID; }
private:
	const CFeature* feature;
	const CUnit* attacker;
	float damage;
	int weaponDefID;
	int projectileID;
};



struct LuaProjEventBase {
public:
	LuaProjEventBase(ProjEvent eventID): id(eventID) {}
	ProjEvent GetID() const { return id; }
protected:
	ProjEvent id;
};

struct LuaProjCreatedEvent: public LuaProjEventBase {
public:
	LuaProjCreatedEvent(const CProjectile* p): LuaProjEventBase(PROJ_CREATED) {
		projectile = p;
	}
	const CProjectile* GetProjectile() const { return projectile; }
private:
	const CProjectile* projectile;
};

struct LuaProjDestroyedEvent: public LuaProjEventBase {
public:
	LuaProjDestroyedEvent(const CProjectile* p): LuaProjEventBase(PROJ_DESTROYED) {
		projectile = p;
	}
	const CProjectile* GetProjectile() const { return projectile; }
private:
	const CProjectile* projectile;
};



struct LuaLogEventBase {
public:
	LuaLogEventBase(LogEvent eventID): id(eventID) {}
	LogEvent GetID() const { return id; }
protected:
	LogEvent id;
};

struct LuaAddConsoleLineEvent: public LuaLogEventBase {
public:
	LuaAddConsoleLineEvent(const std::string& msg, const std::string& sec, int lvl):
		LuaLogEventBase(LOG_CONSOLE_LINE),
		message(msg),
		section(sec),
		level(lvl)
	{
	}
	const std::string& GetMessage() const { return message; }
	const std::string& GetSection() const { return section; }
	int GetLevel() const { return level; }
private:
	std::string message;
	std::string section;
	int level;
};



struct UIEventBase {
public:
	UIEventBase(UIEvent eventID): id(eventID) {}
	UIEvent GetID() const { return id; }
protected: UIEvent id;
};

struct UIShockFrontEvent: public UIEventBase {
public:
	UIShockFrontEvent(const float3& p, float pwr, float r, float d): UIEventBase(SHOCK_FRONT) {
		pos = p;
		power = pwr;
		aoe = r;
		dist = d;
	}
	const float3& GetPos() const { return pos; }
	float GetPower() const { return power; }
	float GetAreaOfEffect() const { return aoe; }
	const float* GetDistMod() const { return &dist; }
private:
	float3 pos;
	float power;
	float aoe;
	float dist;
};






// FIXME:
//   THIS A FUCKING RETARDED WAY OF DOING IT, UNMAINTAINABLE AND UNEXTENDIBLE
//   CREATE SIMPLE DEDICATED EVENT STRUCTURES OR THROW BATCHING THE FUCK OUT
/*
struct LuaUnitEvent {
	LuaUnitEvent(UnitEvent i, const CUnit* u1, const Command& c1)
		: id(i)
		, unit1(u1)
		, bool1(false)
		, int1(0)
		, float1(0.0f)
		, unit2(NULL)
	{ cmd1.reset(new Command(c1)); }
	LuaUnitEvent(UnitEvent i, const CUnit* u1, int i1, const float3& p1, float f1)
		: id(i)
		, unit1(u1)
		, bool1(false)
		, int1(i1)
		, float1(f1)
		, unit2(NULL)
	{ cmd1.reset(new Command(p1)); } // store the vector as a command to save some memory
	LuaUnitEvent(UnitEvent i, int i1, const float3& p1, const CUnit* u1)
		: id(i)
		, unit1(u1)
		, bool1(false)
		, int1(i1)
		, float1(0.0f)
		, unit2(NULL)
	{ cmd1.reset(new Command(p1)); } // store the vector as a command to save some memory

	UnitEvent id;
	const CUnit* unit1;
	bool bool1;
	int int1;
	float float1;
	union {
		const CUnit* unit2;
		int int2;
		const CWeapon* weapon2;
	};
	boost::shared_ptr<Command> cmd1; // using a shared pointer instead of an actual object saves some memory
};
*/

#define LUA_EVENT_CAST(type, event) const type& ee = static_cast<const type&>(event)

#if (LUA_MT_OPT & LUA_BATCH)
	#define LUA_UNIT_BATCH_PUSH(r, unitEvent)                   \
		if (UseEventBatch() && Threading::IsLuaBatchThread()) { \
			GML_STDMUTEX_LOCK(ulbatch);                         \
			luaUnitEventBatch.push_back(unitEvent);             \
			return r;                                           \
		}
	#define LUA_FEAT_BATCH_PUSH(r, featEvent)                   \
		if (UseEventBatch() && Threading::IsLuaBatchThread()) { \
			GML_STDMUTEX_LOCK(flbatch);                         \
			luaFeatEventBatch.push_back(featEvent);             \
			return r;                                           \
		}
	#define LUA_PROJ_BATCH_PUSH(r, projEvent)                   \
		if (UseEventBatch() && Threading::IsLuaBatchThread()) { \
			GML_STDMUTEX_LOCK(plbatch);                         \
			luaProjEventBatch.push_back(projEvent);             \
			return r;                                           \
		}
	#define LUA_FRAME_BATCH_PUSH(r, frameEvent)                 \
		if (UseEventBatch() && Threading::IsLuaBatchThread()) { \
			GML_STDMUTEX_LOCK(glbatch);                         \
			luaFrameEventBatch.push_back(frameEvent);           \
			return r;                                           \
		}
	#define LUA_LOG_BATCH_PUSH(r, logEvent)                     \
		if (UseEventBatch() && Threading::IsLuaBatchThread()) { \
			GML_STDMUTEX_LOCK(mlbatch);                         \
			luaLogEventBatch.push_back(logEvent);               \
			return r;                                           \
		}
	#define LUA_UI_BATCH_PUSH(r, uiEvent)                       \
		if (UseEventBatch() && Threading::IsLuaBatchThread()) { \
			GML_STDMUTEX_LOCK(llbatch);                         \
			luaUIEventBatch.push_back(uiEvent);                 \
			return r;                                           \
		}
#else
	#define LUA_UNIT_BATCH_PUSH(r, unitEvent)
	#define LUA_FEAT_BATCH_PUSH(r, featEvent)
	#define LUA_PROJ_BATCH_PUSH(r, projEvent)
	#define LUA_FRAME_BATCH_PUSH(r, frameEvent)
	#define LUA_LOG_BATCH_PUSH(r, logEvent)
	#define LUA_UI_BATCH_PUSH(r, uiEvent)
#endif

#if defined(USE_GML) && GML_ENABLE_SIM && GML_CALL_DEBUG
	#define GML_MEASURE_LOCK_TIME(lock) spring_time luatime = spring_gettime(); lock; luatime = spring_gettime() - luatime; gmlLockTime += luatime.toMilliSecsf()
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
		#define GML_DRCMUTEX_LOCK(name) GML_MEASURE_LOCK_TIME(GML_OBJMUTEX_LOCK(name, GML_DRAW|GML_SIM, *GetLuaContextData(L)->))
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
	#define GML_DRCMUTEX_LOCK(name) GML_MEASURE_LOCK_TIME(GML_OBJMUTEX_LOCK(name, GML_DRAW|GML_SIM, *GetLuaContextData(L)->))
#endif

#if defined(USE_GML) && GML_ENABLE_SIM
	#define GML_SELECT_LUA_STATE() SELECT_LUA_STATE()
#else
	#define GML_SELECT_LUA_STATE()
#endif

#endif // LUA_EVENT_BATCH_H

