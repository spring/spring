/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EVENT_BATCH_HANDLER_HDR
#define EVENT_BATCH_HANDLER_HDR

#include "lib/gml/ThreadSafeContainers.h"
#include "Sim/Projectiles/ProjectileHandler.h" // for UNSYNCED_PROJ_NOEVENT

class CUnit;
class CFeature;
class CProjectile;

struct EventBatchHandler {
private:
	static boost::int64_t eventSequenceNumber;

	struct ProjectileCreatedDestroyedEvent {
		static void Add(const CProjectile*);
		static void Remove(const CProjectile*);
		static void Delete(const CProjectile*);
	};

	typedef ThreadListRender<
		const CProjectile*,
		std::set<const CProjectile*>,
		const CProjectile*,
		ProjectileCreatedDestroyedEvent
	> ProjectileCreatedDestroyedEventBatch;

#if UNSYNCED_PROJ_NOEVENT
	struct UnsyncedProjectileCreatedDestroyedEvent {
		static void Add(const CProjectile*);
		static void Remove(const CProjectile*);
		static void Delete(const CProjectile*);
	};

	typedef ThreadListRender<
		const CProjectile*,
		std::set<const CProjectile*>,
		const CProjectile*,
		UnsyncedProjectileCreatedDestroyedEvent
	> UnsyncedProjectileCreatedDestroyedEventBatch;
#endif

	struct UAD {
		const CUnit* unit;
		int data;
		int status;
		boost::int64_t seqnum;

		UAD(const CUnit* u, int d): unit(u), data(d), status(0), seqnum(eventSequenceNumber++) {}
		UAD(const CUnit* u, int d, int s): unit(u), data(d), status(s), seqnum(eventSequenceNumber++) {}
		bool operator==(const CUnit* u) const { return unit == u; }
		bool operator==(const UAD& u) const { return unit == u.unit && seqnum == u.seqnum; }
		bool operator<(const UAD& u) const { return unit < u.unit || (unit == u.unit && seqnum < u.seqnum); }
	};
public:
	struct UD {
		const CUnit* unit;
		int data;

		UD(const CUnit* u): unit(u), data(0) {}
		UD(const CUnit* u, int d): unit(u), data(d) {}
		bool operator==(const UD& u) const { return unit == u.unit; }
		bool operator<(const UD& u) const { return unit < u.unit; }
	};
private:
	struct UnitCreatedDestroyedEvent {
		static void Add(const UD&);
		static void Remove(const UD&);
		static void Delete(const UD&) { }
	};

	struct UnitCloakStateChangedEvent {
		static void Add(const UAD&);
		static void Remove(const UAD&) { }
		static void Delete(const UAD&) { }
	};

	struct UnitLOSStateChangedEvent {
		static void Add(const UAD&);
		static void Remove(const UAD&) { }
		static void Delete(const UAD&) { }
	};

	typedef ThreadListRender<const CUnit *, std::set<UD>, UD, UnitCreatedDestroyedEvent> UnitCreatedDestroyedEventBatch;
	typedef ThreadListRender<const CUnit *, std::set<UAD>, UAD, UnitCloakStateChangedEvent> UnitCloakStateChangedEventBatch;
	typedef ThreadListRender<const CUnit *, std::set<UAD>, UAD, UnitLOSStateChangedEvent> UnitLOSStateChangedEventBatch;

	struct FAP {
		const CFeature* feat;
		boost::int64_t seqnum;
		float3 oldpos;
		float3 newpos;

		FAP(const CFeature* f, const float3 op, const float3 np): feat(f), seqnum(eventSequenceNumber++), oldpos(op), newpos(np) {}
		bool operator==(const CFeature* f) const { return feat == f; }
		bool operator==(const FAP& f) const { return feat == f.feat && seqnum == f.seqnum; }
		bool operator<(const FAP& f) const { return feat < f.feat || (feat == f.feat && seqnum < f.seqnum); }
	};

	struct FeatureCreatedDestroyedEvent {
		static void Add(const CFeature*);
		static void Remove(const CFeature*);
		static void Delete(const CFeature*) { }
	};

	struct FeatureMovedEvent {
		static void Add(const FAP&);
		static void Remove(const FAP&) { }
		static void Delete(const FAP&) { }
	};

	typedef ThreadListRender<
		const CFeature*,
		std::set<const CFeature*>,
		const CFeature*,
		FeatureCreatedDestroyedEvent
	> FeatureCreatedDestroyedEventBatch;
	typedef ThreadListRender<
		const CFeature*,
		std::set<FAP>,
		FAP,
		FeatureMovedEvent
	> FeatureMovedEventBatch;

	//! contains only projectiles that can change simulation state
	ProjectileCreatedDestroyedEventBatch syncedProjectileCreatedDestroyedEventBatch;

#if UNSYNCED_PROJ_NOEVENT
	static UnsyncedProjectileCreatedDestroyedEventBatch
#else
	ProjectileCreatedDestroyedEventBatch
#endif
	//! contains only projectiles that cannot change simulation state
	unsyncedProjectileCreatedDestroyedEventBatch;

	UnitCreatedDestroyedEventBatch unitCreatedDestroyedEventBatch;
	UnitCloakStateChangedEventBatch unitCloakStateChangedEventBatch;
	UnitLOSStateChangedEventBatch unitLOSStateChangedEventBatch;

	FeatureCreatedDestroyedEventBatch featureCreatedDestroyedEventBatch;
	FeatureMovedEventBatch featureMovedEventBatch;

public:
	static EventBatchHandler* GetInstance();

	void UpdateUnits();
	void UpdateDrawUnits();
	void DeleteSyncedUnits();

	void UpdateFeatures();
	void UpdateDrawFeatures();
	void DeleteSyncedFeatures();

	void UpdateProjectiles();
	void UpdateDrawProjectiles();
	void DeleteSyncedProjectiles();

	void UpdateObjects();

	void LoadedModelRequested();

	void EnqueueUnitLOSStateChangeEvent(const CUnit* unit, int allyteam, int newstatus) { unitLOSStateChangedEventBatch.enqueue(UAD(unit, allyteam, newstatus)); }
	void EnqueueUnitCloakStateChangeEvent(const CUnit* unit, int cloaked) { unitCloakStateChangedEventBatch.enqueue(UAD(unit, cloaked)); }

	void EnqueueFeatureMovedEvent(const CFeature* feature, const float3& oldpos, const float3& newpos) {
		featureMovedEventBatch.enqueue(FAP(feature, oldpos, newpos));
	}

	ProjectileCreatedDestroyedEventBatch& GetSyncedProjectileCreatedDestroyedBatch() { return syncedProjectileCreatedDestroyedEventBatch; }

#if UNSYNCED_PROJ_NOEVENT
	static inline UnsyncedProjectileCreatedDestroyedEventBatch&
#else
	ProjectileCreatedDestroyedEventBatch&
#endif
	GetUnsyncedProjectileCreatedDestroyedBatch() { return unsyncedProjectileCreatedDestroyedEventBatch; }


	UnitCreatedDestroyedEventBatch& GetUnitCreatedDestroyedBatch() { return unitCreatedDestroyedEventBatch; }
	UnitCloakStateChangedEventBatch& GetUnitCloakStateChangedBatch() { return unitCloakStateChangedEventBatch; }
	UnitLOSStateChangedEventBatch& GetUnitLOSStateChangedBatch() { return unitLOSStateChangedEventBatch; }

	FeatureCreatedDestroyedEventBatch& GetFeatureCreatedDestroyedEventBatch() { return featureCreatedDestroyedEventBatch; }
	FeatureMovedEventBatch& GetFeatureMovedEventBatch() { return featureMovedEventBatch; }
};

#define eventBatchHandler (EventBatchHandler::GetInstance())

#endif
