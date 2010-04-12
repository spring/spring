/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sim/Projectiles/Projectile.h" // for operator delete
#include "Sim/Projectiles/ProjectileHandler.h" // for UNSYNCED_PROJ_NOEVENT

#include "EventBatchHandler.h"
#include "EventHandler.h"

#if UNSYNCED_PROJ_NOEVENT
#include "Rendering/ProjectileDrawer.hpp"
#endif


void EventBatchHandler::ProjectileCreatedDestroyedEvent::Add(const CProjectile* p) { eventHandler.RenderProjectileCreated(p); }
void EventBatchHandler::ProjectileCreatedDestroyedEvent::Remove(const CProjectile* p) { eventHandler.RenderProjectileDestroyed(p); }
void EventBatchHandler::ProjectileCreatedDestroyedEvent::Delete(const CProjectile* p) { delete p; }

#if UNSYNCED_PROJ_NOEVENT
void EventBatchHandler::UnsyncedProjectileCreatedDestroyedEvent::Add(const CProjectile* p) { projectileDrawer->RenderProjectileCreated(p); }
void EventBatchHandler::UnsyncedProjectileCreatedDestroyedEvent::Remove(const CProjectile* p) { projectileDrawer->RenderProjectileDestroyed(p); }
void EventBatchHandler::UnsyncedProjectileCreatedDestroyedEvent::Delete(const CProjectile* p) { delete p; }
#endif

void EventBatchHandler::UnitCreatedDestroyedEvent::Add(const CUnit* u) { eventHandler.RenderUnitCreated(u); }
void EventBatchHandler::UnitCreatedDestroyedEvent::Remove(const CUnit* u) { eventHandler.RenderUnitDestroyed(u); }
void EventBatchHandler::UnitCloakStateChangedEvent::Add(const UAD& u) { eventHandler.RenderUnitCloakChanged(u.unit, u.data); }
void EventBatchHandler::UnitLOSStateChangedEvent::Add(const UAD& u) { eventHandler.RenderUnitLOSChanged(u.unit, u.data); }

void EventBatchHandler::FeatureCreatedDestroyedEvent::Add(const CFeature* f) { eventHandler.RenderFeatureCreated(f); }
void EventBatchHandler::FeatureCreatedDestroyedEvent::Remove(const CFeature* f) { eventHandler.RenderFeatureDestroyed(f); }
void EventBatchHandler::FeatureMovedEvent::Add(const CFeature* f) { eventHandler.RenderFeatureMoved(f); }

EventBatchHandler* EventBatchHandler::GetInstance() {
	static EventBatchHandler ebh;
	return &ebh;
}
