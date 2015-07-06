/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Util.h"
#include "Sim/Projectiles/Projectile.h" // for operator delete

#include "System/EventBatchHandler.h"
#include "System/EventHandler.h"

#if UNSYNCED_PROJ_NOEVENT
#include "Rendering/ProjectileDrawer.h"
#endif

#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/Platform/Threading.h"

// managed by EventHandler
EventBatchHandler* EventBatchHandler::ebh = nullptr;

// global counter for certain batch-events
boost::int64_t EventBatchHandler::eventSequenceNumber = 0;

void EventBatchHandler::CreateInstance()
{
	if (ebh == nullptr) {
		ebh = new EventBatchHandler();
	}
}

void EventBatchHandler::DeleteInstance()
{
	if (ebh != nullptr)	{
		delete ebh;
		ebh = nullptr;
	}
}



EventBatchHandler::EventBatchHandler()
	: CEventClient("[EventBatchHandler]", 0, true)
{
	autoLinkEvents = true;
	RegisterLinkedEvents(this);
	eventHandler.AddClient(this);
}

void EventBatchHandler::UnitMoved(const CUnit* unit) { EnqueueUnitMovedEvent(unit, unit->pos); }
void EventBatchHandler::UnitEnteredRadar(const CUnit* unit, int at) { EnqueueUnitLOSStateChangeEvent(unit, at, unit->losStatus[at]); }
void EventBatchHandler::UnitEnteredLos(const CUnit* unit, int at) { EnqueueUnitLOSStateChangeEvent(unit, at, unit->losStatus[at]); }
void EventBatchHandler::UnitLeftRadar(const CUnit* unit, int at) { EnqueueUnitLOSStateChangeEvent(unit, at, unit->losStatus[at]); }
void EventBatchHandler::UnitLeftLos(const CUnit* unit, int at) { EnqueueUnitLOSStateChangeEvent(unit, at, unit->losStatus[at]); }
void EventBatchHandler::UnitCloaked(const CUnit* unit) { EnqueueUnitCloakStateChangeEvent(unit, 1); }
void EventBatchHandler::UnitDecloaked(const CUnit* unit) { EnqueueUnitCloakStateChangeEvent(unit, 0); }

void EventBatchHandler::FeatureCreated(const CFeature* feature) { GetFeatureCreatedDestroyedEventBatch().enqueue(feature); }
void EventBatchHandler::FeatureDestroyed(const CFeature* feature) { GetFeatureCreatedDestroyedEventBatch().dequeue(feature); }
void EventBatchHandler::FeatureMoved(const CFeature* feature, const float3& oldpos) { EnqueueFeatureMovedEvent(feature, oldpos, feature->pos); }
void EventBatchHandler::ProjectileCreated(const CProjectile* proj)
{
	if (proj->synced) {
		GetSyncedProjectileCreatedDestroyedBatch().insert(proj);
	} else {
		GetUnsyncedProjectileCreatedDestroyedBatch().insert(proj);
	}
}
void EventBatchHandler::ProjectileDestroyed(const CProjectile* proj)
{
	if (proj->synced) {
		GetSyncedProjectileCreatedDestroyedBatch().erase_delete(proj);
	} else {
		GetUnsyncedProjectileCreatedDestroyedBatch().erase_delete(proj);
	}
}





void EventBatchHandler::ProjectileCreatedDestroyedEvent::Add(const CProjectile* p) { eventHandler.RenderProjectileCreated(p); }
void EventBatchHandler::ProjectileCreatedDestroyedEvent::Remove(const CProjectile* p) { eventHandler.RenderProjectileDestroyed(p); }
void EventBatchHandler::ProjectileCreatedDestroyedEvent::Delete(const CProjectile* p) { delete p; }

#if UNSYNCED_PROJ_NOEVENT
EventBatchHandler::UnsyncedProjectileCreatedDestroyedEventBatch EventBatchHandler::unsyncedProjectileCreatedDestroyedEventBatch;
void EventBatchHandler::UnsyncedProjectileCreatedDestroyedEvent::Add(const CProjectile* p) { projectileDrawer->RenderProjectileCreated(p); }
void EventBatchHandler::UnsyncedProjectileCreatedDestroyedEvent::Remove(const CProjectile* p) { projectileDrawer->RenderProjectileDestroyed(p); }
void EventBatchHandler::UnsyncedProjectileCreatedDestroyedEvent::Delete(const CProjectile* p) { delete p; }
#endif

void EventBatchHandler::UnitCreatedDestroyedEvent::Add(const UD& u) { eventHandler.RenderUnitCreated(u.unit, u.data); }
void EventBatchHandler::UnitCreatedDestroyedEvent::Remove(const UD& u) { eventHandler.RenderUnitDestroyed(u.unit); }
void EventBatchHandler::UnitCloakStateChangedEvent::Add(const UAD& u) { if(!u.unit->isDead) eventHandler.RenderUnitCloakChanged(u.unit, u.data); }
void EventBatchHandler::UnitLOSStateChangedEvent::Add(const UAD& u) { if(!u.unit->isDead) eventHandler.RenderUnitLOSChanged(u.unit, u.data, u.status); }
void EventBatchHandler::UnitMovedEvent::Add(const UAP& u) { eventHandler.RenderUnitMoved(u.unit, u.newpos); }

void EventBatchHandler::FeatureCreatedDestroyedEvent::Add(const CFeature* f) { eventHandler.RenderFeatureCreated(f); }
void EventBatchHandler::FeatureCreatedDestroyedEvent::Remove(const CFeature* f) { eventHandler.RenderFeatureDestroyed(f); }
void EventBatchHandler::FeatureMovedEvent::Add(const FAP& f) { eventHandler.RenderFeatureMoved(f.feat, f.oldpos, f.newpos); }
