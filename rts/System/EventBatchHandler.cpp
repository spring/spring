/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Util.h"
#include "Sim/Projectiles/Projectile.h" // for operator delete

#include "System/EventBatchHandler.h"
#include "System/EventHandler.h"

#if UNSYNCED_PROJ_NOEVENT
#include "Rendering/ProjectileDrawer.h"
#endif

#include "lib/gml/gmlcnf.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/Platform/Threading.h"

boost::int64_t EventBatchHandler::eventSequenceNumber = 0;

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

EventBatchHandler* EventBatchHandler::GetInstance() {
	static EventBatchHandler ebh;
	return &ebh;
}

void EventBatchHandler::UpdateUnits() {
	unitCreatedDestroyedEventBatch.delay();
	unitCloakStateChangedEventBatch.delay();
	unitLOSStateChangedEventBatch.delay();
	unitMovedEventBatch.delay();
}
void EventBatchHandler::UpdateDrawUnits() {
	GML_STDMUTEX_LOCK(runit); // UpdateDrawUnits

	unitCreatedDestroyedEventBatch.execute();
	unitCloakStateChangedEventBatch.execute();
	unitLOSStateChangedEventBatch.execute();
	unitMovedEventBatch.execute();
}
void EventBatchHandler::DeleteSyncedUnits() {
	unitCloakStateChangedEventBatch.clean(unitCreatedDestroyedEventBatch.to_destroy());

	unitLOSStateChangedEventBatch.clean(unitCreatedDestroyedEventBatch.to_destroy());

	unitMovedEventBatch.clean(unitCreatedDestroyedEventBatch.to_destroy());

	unitCreatedDestroyedEventBatch.clean();
	unitCreatedDestroyedEventBatch.destroy_synced();
}

void EventBatchHandler::UpdateFeatures() {
	featureCreatedDestroyedEventBatch.delay();
	featureMovedEventBatch.delay();
}
void EventBatchHandler::UpdateDrawFeatures() {
	GML_STDMUTEX_LOCK(rfeat); // UpdateDrawFeatures

	featureCreatedDestroyedEventBatch.execute();
	featureMovedEventBatch.execute();
}
void EventBatchHandler::DeleteSyncedFeatures() {
	featureMovedEventBatch.clean(featureCreatedDestroyedEventBatch.to_destroy());

	featureCreatedDestroyedEventBatch.clean();
	featureCreatedDestroyedEventBatch.destroy();
}

void EventBatchHandler::UpdateProjectiles() {
	syncedProjectileCreatedDestroyedEventBatch.delay_delete();
	syncedProjectileCreatedDestroyedEventBatch.delay_add();
	unsyncedProjectileCreatedDestroyedEventBatch.delay_delete();
	unsyncedProjectileCreatedDestroyedEventBatch.delay_add();
}
void EventBatchHandler::UpdateDrawProjectiles() {
	GML_STDMUTEX_LOCK(rproj); // UpdateDrawProjectiles

	projectileHandler->syncedRenderProjectileIDs.delete_delayed();
	syncedProjectileCreatedDestroyedEventBatch.delete_delayed();

	syncedProjectileCreatedDestroyedEventBatch.add_delayed();
	projectileHandler->syncedRenderProjectileIDs.add_delayed();

#if !UNSYNCED_PROJ_NOEVENT
	projectileHandler->unsyncedRenderProjectileIDs.delete_delayed();
#endif
	unsyncedProjectileCreatedDestroyedEventBatch.delete_delayed();
	unsyncedProjectileCreatedDestroyedEventBatch.add_delayed();
#if !UNSYNCED_PROJ_NOEVENT
	projectileHandler->unsyncedRenderProjectileIDs.add_delayed();
#endif
}
void EventBatchHandler::DeleteSyncedProjectiles() {
}

void EventBatchHandler::UpdateObjects() {
	{ 
		GML_STDMUTEX_LOCK(runit); // UpdateObjects

		UpdateUnits();
	}
	{
		GML_STDMUTEX_LOCK(rfeat); // UpdateObjects

		UpdateFeatures();
	}
	{
		GML_STDMUTEX_LOCK(rproj); // UpdateObjects

		UpdateProjectiles();
	}
}

void EventBatchHandler::LoadedModelRequested() {
	// Make sure the requested model is available to the calling thread
	if (GML::SimEnabled() && GML::ShareLists() && !GML::IsSimThread()) 
		texturehandlerS3O->UpdateDraw();
}
