/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <algorithm>
#include "mmgr.h"

#include "Projectile.h"
#include "ProjectileHandler.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/GroundFlash.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Projectiles/Unsynced/FlyingPiece.hpp"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/ConfigHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/EventHandler.h"
#include "System/LogOutput.h"
#include "System/TimeProfiler.h"
#include "System/creg/STL_Map.h"
#include "System/creg/STL_List.h"

CProjectileHandler* ph;

using namespace std;


CR_BIND_TEMPLATE(ProjectileContainer, )
CR_REG_METADATA(ProjectileContainer, (
	CR_MEMBER(cont),
	CR_POSTLOAD(PostLoad)
));
CR_BIND_TEMPLATE(GroundFlashContainer, )
CR_REG_METADATA(GroundFlashContainer, (
	CR_MEMBER(cont),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(CProjectileHandler, );
CR_REG_METADATA(CProjectileHandler, (
	CR_MEMBER(syncedProjectiles),
	CR_MEMBER(unsyncedProjectiles),
	CR_MEMBER(syncedProjectileIDs),
	CR_MEMBER(freeSyncedIDs),
	CR_MEMBER(maxUsedSyncedID),
	CR_MEMBER(groundFlashes),
	CR_RESERVED(32),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));



// need this for AddFlyingPiece
bool piececmp::operator() (const FlyingPiece* fp1, const FlyingPiece* fp2) const {
	if (fp1->texture != fp2->texture)
		return (fp1->texture > fp2->texture);
	if (fp1->team != fp2->team)
		return (fp1->team > fp2->team);
	return (fp1 > fp2);
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProjectileHandler::CProjectileHandler()
{
	maxParticles     = configHandler->Get("MaxParticles",      1000);
	maxNanoParticles = configHandler->Get("MaxNanoParticles", 2500);

	currentParticles       = 0;
	currentNanoParticles   = 0;
	particleSaturation     = 0.0f;
	nanoParticleSaturation = 0.0f;
	numPerlinProjectiles   = 0;

	// preload some IDs
	for (int i = 0; i < 16384; i++) {
		freeSyncedIDs.push_back(i);
		freeUnsyncedIDs.push_back(i);
	}

	maxUsedSyncedID = freeSyncedIDs.size();
	maxUsedUnsyncedID = freeUnsyncedIDs.size();
}

CProjectileHandler::~CProjectileHandler()
{
	syncedProjectiles.clear(); // synced first, to avoid callback crashes
	unsyncedProjectiles.clear();

	freeSyncedIDs.clear();
	freeUnsyncedIDs.clear();

	syncedProjectileIDs.clear();
	unsyncedProjectileIDs.clear();

	ph = 0;
}

void CProjectileHandler::Serialize(creg::ISerializer* s)
{
	if (s->IsWriting()) {
		int ssize = int(syncedProjectiles.size());
		int usize = int(unsyncedProjectiles.size());

		s->Serialize(&ssize, sizeof(int));
		for (ProjectileContainer::iterator it = syncedProjectiles.begin(); it != syncedProjectiles.end(); ++it) {
			void** ptr = (void**) &*it;
			s->SerializeObjectPtr(ptr, (*it)->GetClass());
		}

		s->Serialize(&usize, sizeof(int));
		for (ProjectileContainer::iterator it = unsyncedProjectiles.begin(); it != unsyncedProjectiles.end(); ++it) {
			void** ptr = (void**) &*it;
			s->SerializeObjectPtr(ptr, (*it)->GetClass());
		}
	} else {
		int ssize, usize;

		s->Serialize(&ssize, sizeof(int));
		syncedProjectiles.resize(ssize);

		for (ProjectileContainer::iterator it = syncedProjectiles.begin(); it != syncedProjectiles.end(); ++it) {
			void** ptr = (void**) &*it;
			s->SerializeObjectPtr(ptr, 0/*FIXME*/);
		}


		s->Serialize(&usize, sizeof(int));
		unsyncedProjectiles.resize(usize);

		for (ProjectileContainer::iterator it = unsyncedProjectiles.begin(); it != unsyncedProjectiles.end(); ++it) {
			void** ptr = (void**) &*it;
			s->SerializeObjectPtr(ptr, 0/*FIXME*/);
		}
	}
}

void CProjectileHandler::PostLoad()
{
}




void CProjectileHandler::UpdateProjectileContainer(ProjectileContainer& pc, bool synced) {
	ProjectileContainer::iterator pci = pc.begin();

	while (pci != pc.end()) {
		CProjectile* p = *pci;

		if (p->deleteMe) {
			ProjectileMap::iterator pIt;

			if (p->synced) {
				//! iterator is always valid
				pIt = syncedProjectileIDs.find(p->id);

				eventHandler.ProjectileDestroyed((pIt->second).first, (pIt->second).second);
				syncedProjectileIDs.erase(pIt);

				freeSyncedIDs.push_back(p->id);

				//! push_back this projectile for deletion
				pci = pc.erase_delete_synced(pci);
			} else {
#if UNSYNCED_PROJ_NOEVENT
				eventHandler.UnsyncedProjectileDestroyed(p);
#else
				pIt = unsyncedProjectileIDs.find(p->id);

				eventHandler.ProjectileDestroyed((pIt->second).first, (pIt->second).second);
				unsyncedProjectileIDs.erase(pIt);

				freeUnsyncedIDs.push_back(p->id);
#endif
				pci = pc.erase_delete(pci);
			}
		} else {
			p->Update();
			GML_GET_TICKS(p->lastProjUpdate);
			++pci;
		}
	}
}



void CProjectileHandler::Update()
{
	{
		SCOPED_TIMER("Projectile Collisions");
		CheckCollisions();
	}

	{
		SCOPED_TIMER("Projectile Update");

		GML_UPDATE_TICKS();

		UpdateProjectileContainer(syncedProjectiles, true);
		UpdateProjectileContainer(unsyncedProjectiles, false);


		{
			GML_STDMUTEX_LOCK(rproj); // Update

			if (syncedProjectiles.can_delete_synced()) {
				GML_STDMUTEX_LOCK(proj); // Update

				eventHandler.DeleteSyncedProjectiles();
				//! delete all projectiles that were
				//! queued (push_back'ed) for deletion
				syncedProjectiles.delete_erased_synced();
			}

			eventHandler.UpdateProjectiles();
		}


		GroundFlashContainer::iterator gfi = groundFlashes.begin();
		while (gfi != groundFlashes.end()) {
			CGroundFlash* gf = *gfi;

			if (!gf->Update()) {
				gfi = groundFlashes.erase_delete(gfi);
			} else {
				++gfi;
			}
		}

		{
			GML_STDMUTEX_LOCK(rflash); // Update

			groundFlashes.delay_delete();
			groundFlashes.delay_add();
		}

		#define UPDATE_FLYING_PIECES(fpContainer)                                        \
			FlyingPieceContainer::iterator pti = fpContainer.begin();                    \
                                                                                         \
			while (pti != fpContainer.end()) {                                           \
				FlyingPiece* p = *pti;                                                   \
				p->pos     += p->speed;                                                  \
				p->speed   *= 0.996f;                                                    \
				p->speed.y += mapInfo->map.gravity; /* fp's are not projectiles */       \
				p->rot     += p->rotSpeed;                                               \
                                                                                         \
				if (p->pos.y < ground->GetApproximateHeight(p->pos.x, p->pos.z - 10)) {  \
					pti = fpContainer.erase_delete_set(pti);                             \
				} else {                                                                 \
					++pti;                                                               \
				}                                                                        \
			}

		{ UPDATE_FLYING_PIECES(flyingPieces3DO); }
		{ UPDATE_FLYING_PIECES(flyingPiecesS3O); }
		#undef UPDATE_FLYING_PIECES

		{
			GML_STDMUTEX_LOCK(rpiece); // Update

			flyingPieces3DO.delay_delete();
			flyingPieces3DO.delay_add();
			flyingPiecesS3O.delay_delete();
			flyingPiecesS3O.delay_add();
		}
	}
}



void CProjectileHandler::AddProjectile(CProjectile* p)
{
	std::list<int>* freeIDs = NULL;
	std::map<int, ProjectileMapPair>* proIDs = NULL;
	int* maxUsedID = NULL;
	int newID = 0;

	if (p->synced) {
		syncedProjectiles.push(p);
		freeIDs = &freeSyncedIDs;
		proIDs = &syncedProjectileIDs;
		maxUsedID = &maxUsedSyncedID;
	} else {
		unsyncedProjectiles.push(p);
#if UNSYNCED_PROJ_NOEVENT
		eventHandler.UnsyncedProjectileCreated(p);
		return;
#endif
		freeIDs = &freeUnsyncedIDs;
		proIDs = &unsyncedProjectileIDs;
		maxUsedID = &maxUsedUnsyncedID;
	}

	if (!freeIDs->empty()) {
		newID = freeIDs->front();
		freeIDs->pop_front();
	} else {
		(*maxUsedID)++;
		newID = *maxUsedID;
	}

	if ((*maxUsedID) > (1 << 24)) {
		logOutput.Print("LUA %s projectile IDs are now out of range", (p->synced? "synced": "unsynced"));
	}

	ProjectileMapPair pp(p, p->owner() ? p->owner()->allyteam : -1);

	p->id = newID;
	(*proIDs)[p->id] = pp;

	eventHandler.ProjectileCreated(pp.first, pp.second);
}




void CProjectileHandler::CheckUnitCollisions(
	CProjectile* p,
	std::vector<CUnit*>& tempUnits,
	CUnit** endUnit,
	const float3& ppos0,
	const float3& ppos1)
{
	CollisionQuery q;

	for (CUnit** ui = &tempUnits[0]; ui != endUnit; ++ui) {
		CUnit* unit = *ui;

		const bool friendlyShot = (p->owner() && (unit->allyteam == p->owner()->allyteam));
		const bool raytraced = (unit->collisionVolume->GetTestType() == CollisionVolume::COLVOL_HITTEST_CONT);

		// if this unit fired this projectile or (this unit is in the
		// same allyteam as the unit that shot this projectile and we
		// are ignoring friendly collisions)
		if (p->owner() == unit || ((p->collisionFlags & COLLISION_NOFRIENDLY) && friendlyShot)) {
			continue;
		}

		if (p->collisionFlags & COLLISION_NONEUTRAL) {
			if (unit->IsNeutral()) { continue; }
		}

		if (CCollisionHandler::DetectHit(unit, ppos0, ppos1, &q)) {
			if (q.lmp != NULL) {
				unit->SetLastAttackedPiece(q.lmp, gs->frameNum);
			}

			// The current projectile <p> won't reach the raytraced surface impact
			// position until ::Update() is called (same frame). This is a problem
			// when dealing with fast low-AOE projectiles since they would do almost
			// no damage if detonated outside the collision volume. Therefore, smuggle
			// a bit with its position now (rather than rolling it back in ::Update()
			// and waiting for the next-frame CheckUnitCol(), which is problematic
			// for noExplode projectiles).

			// const float3& pimpp = (q.b0)? q.p0: q.p1;
			const float3 pimpp =
				(q.b0 && q.b1)? ( q.p0 +  q.p1) * 0.5f:
				(q.b0        )? ( q.p0 + ppos1) * 0.5f:
								(ppos0 +  q.p1) * 0.5f;

			p->pos = (raytraced)? pimpp: ppos0;
			p->Collision(unit);
			p->pos = (raytraced)? ppos0: p->pos;
			break;
		}
	}
}

void CProjectileHandler::CheckFeatureCollisions(
	CProjectile* p,
	std::vector<CFeature*>& tempFeatures,
	CFeature** endFeature,
	const float3& ppos0,
	const float3& ppos1)
{
	CollisionQuery q;

	if (p->collisionFlags & COLLISION_NOFEATURE) {
		return;
	}

	for (CFeature** fi = &tempFeatures[0]; fi != endFeature; ++fi) {
		CFeature* feature = *fi;

		// geothermals do not have a collision volume, skip them
		if (!feature->blocking || feature->def->geoThermal) {
			continue;
		}

		const bool raytraced =
			(feature->collisionVolume &&
			feature->collisionVolume->GetTestType() == CollisionVolume::COLVOL_HITTEST_CONT);

		if (CCollisionHandler::DetectHit(feature, ppos0, ppos1, &q)) {
			const float3 pimpp =
				(q.b0 && q.b1)? (q.p0 + q.p1) * 0.5f:
				(q.b0        )? (q.p0 + ppos1) * 0.5f:
								(ppos0 + q.p1) * 0.5f;

			p->pos = (raytraced)? pimpp: ppos0;
			p->Collision(feature);
			p->pos = (raytraced)? ppos0: p->pos;
			break;
		}
	}
}

void CProjectileHandler::CheckUnitFeatureCollisions(ProjectileContainer& pc) {
	static std::vector<CUnit*> tempUnits(uh->MaxUnits(), NULL);
	static std::vector<CFeature*> tempFeatures(uh->MaxUnits(), NULL);

	for (ProjectileContainer::iterator pci = pc.begin(); pci != pc.end(); ++pci) {
		CProjectile* p = *pci;

		if (p->checkCol && !p->deleteMe) {
			const float3 ppos0 = p->pos;
			const float3 ppos1 = p->pos + p->speed;
			const float speedf = p->speed.Length();

			CUnit** endUnit = &tempUnits[0];
			CFeature** endFeature = &tempFeatures[0];

			qf->GetUnitsAndFeaturesExact(p->pos, p->radius + speedf, endUnit, endFeature);

			CheckUnitCollisions(p, tempUnits, endUnit, ppos0, ppos1);
			if (p->checkCol) // already collided with unit?
				CheckFeatureCollisions(p, tempFeatures, endFeature, ppos0, ppos1);
		}
	}
}

void CProjectileHandler::CheckGroundCollisions(ProjectileContainer& pc) {
	ProjectileContainer::iterator pci;

	for (pci = pc.begin(); pci != pc.end(); ++pci) {
		CProjectile* p = *pci;

		if (p->checkCol) {
			// too many projectiles seem to impact the ground before
			// actually hitting so don't subtract the projectile radius
			if (ground->GetHeight(p->pos.x, p->pos.z) > p->pos.y /* - p->radius*/) {
				p->Collision();
			}
		}
	}
}

void CProjectileHandler::CheckCollisions()
{
	CheckUnitFeatureCollisions(syncedProjectiles); //! changes simulation state
	CheckUnitFeatureCollisions(unsyncedProjectiles); //! does not change simulation state

	CheckGroundCollisions(syncedProjectiles); //! changes simulation state
	CheckGroundCollisions(unsyncedProjectiles); //! does not change simulation state
}



void CProjectileHandler::AddGroundFlash(CGroundFlash* flash)
{
	groundFlashes.push(flash);
}


void CProjectileHandler::AddFlyingPiece(int team, float3 pos, float3 speed, const S3DOPiece* object, const S3DOPrimitive* piece)
{
	FlyingPiece* fp = new FlyingPiece(team, pos, speed, object, piece);
	flyingPieces3DO.insert(fp);
}

void CProjectileHandler::AddFlyingPiece(int textureType, int team, float3 pos, float3 speed, SS3OVertex* verts)
{
	if (textureType <= 0)
		return; // texture 0 means 3do

	FlyingPiece* fp = new FlyingPiece(team, pos, speed, textureType, verts);
	flyingPiecesS3O.insert(fp);
}
