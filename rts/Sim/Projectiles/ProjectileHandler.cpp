/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "Projectile.h"
#include "ProjectileHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GroundFlash.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Unsynced/FlyingPiece.h"
#include "Sim/Projectiles/Unsynced/GfxProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/creg/STL_Map.h"
#include "System/creg/STL_List.h"
#include "lib/gml/gmlmut.h"

// reserve 5% of maxNanoParticles for important stuff such as capture and reclaim other teams' units
#define NORMAL_NANO_PRIO 0.95f
#define HIGH_NANO_PRIO 1.0f


using namespace std;

CONFIG(int, MaxParticles).defaultValue(1000);
CONFIG(int, MaxNanoParticles).defaultValue(2500);

CProjectileHandler* ph;


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

void projdetach::Detach(CProjectile *p) { p->Detach(); }


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProjectileHandler::CProjectileHandler()
{
	maxParticles     = configHandler->GetInt("MaxParticles");
	maxNanoParticles = configHandler->GetInt("MaxNanoParticles");

	currentParticles       = 0;
	currentNanoParticles   = 0;
	particleSaturation     = 0.0f;
	nanoParticleSaturation = 0.0f;

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
	syncedProjectiles.detach_all();
	syncedProjectiles.clear(); // synced first, to avoid callback crashes
	unsyncedProjectiles.clear();

	freeSyncedIDs.clear();
	freeUnsyncedIDs.clear();

	syncedProjectileIDs.clear();
	unsyncedProjectileIDs.clear();

	ph = NULL;

	CCollisionHandler::PrintStats();
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
	//TODO
}




void CProjectileHandler::UpdateProjectileContainer(ProjectileContainer& pc, bool synced) {
	ProjectileContainer::iterator pci = pc.begin();

	#define VECTOR_SANITY_CHECK(v)                              \
		assert(!math::isnan(v.x) && !math::isinf(v.x)); \
		assert(!math::isnan(v.y) && !math::isinf(v.y)); \
		assert(!math::isnan(v.z) && !math::isinf(v.z));
	#define MAPPOS_SANITY_CHECK(v)                 \
		assert(v.x >= -(float3::maxxpos * 16.0f)); \
		assert(v.x <=  (float3::maxxpos * 16.0f)); \
		assert(v.z >= -(float3::maxzpos * 16.0f)); \
		assert(v.z <=  (float3::maxzpos * 16.0f)); \
		assert(v.y >= -MAX_PROJECTILE_HEIGHT);     \
		assert(v.y <=  MAX_PROJECTILE_HEIGHT);
	#define PROJECTILE_SANITY_CHECK(p) \
		VECTOR_SANITY_CHECK(p->pos);   \
		MAPPOS_SANITY_CHECK(p->pos);

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
#if DETACH_SYNCED
				pci = pc.erase_detach(pci);
#else
				pci = pc.erase_delete(pci);
#endif
			}
		} else {
			PROJECTILE_SANITY_CHECK(p);

			p->Update();
			qf->MovedProjectile(p);

			PROJECTILE_SANITY_CHECK(p);
			GML::GetTicks(p->lastProjUpdate);

			++pci;
		}
	}
}



void CProjectileHandler::Update()
{
	CheckCollisions();

	{
		SCOPED_TIMER("ProjectileHandler::Update");
		GML::UpdateTicks();

		UpdateProjectileContainer(syncedProjectiles, true);
		UpdateProjectileContainer(unsyncedProjectiles, false);


		{
			GML_STDMUTEX_LOCK(rproj); // Update

			if (syncedProjectiles.can_delete_synced()) {
#if !DETACH_SYNCED
				GML_RECMUTEX_LOCK(proj); // Update
#endif

				eventHandler.DeleteSyncedProjectiles();
				//! delete all projectiles that were
				//! queued (push_back'ed) for deletion
#if DETACH_SYNCED
				syncedProjectiles.detach_erased_synced();
#else
				syncedProjectiles.delete_erased_synced();
#endif
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
	// already initialized?
	assert(p->id < 0);
	
	std::list<int>* freeIDs = NULL;
	std::map<int, ProjectileMapPair>* proIDs = NULL;
	int* maxUsedID = NULL;
	int newID = 0;

	if (p->synced) {
		syncedProjectiles.push(p);
		freeIDs = &freeSyncedIDs;
		proIDs = &syncedProjectileIDs;
		maxUsedID = &maxUsedSyncedID;

		ASSERT_SYNCED(*maxUsedID);
		ASSERT_SYNCED(freeIDs->size());
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
		LOG_L(L_WARNING, "Lua %s projectile IDs are now out of range", (p->synced? "synced": "unsynced"));
	}
	
	if (p->synced) {
		ASSERT_SYNCED(newID);
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
	CollisionQuery cq;

	for (CUnit** ui = &tempUnits[0]; ui != endUnit; ++ui) {
		CUnit* unit = *ui;

		const CUnit* attacker = p->owner();

		// if this unit fired this projectile, always ignore
		if (attacker == unit) {
			continue;
		}

		if (p->GetCollisionFlags() & Collision::NOFRIENDLIES) {
			if (attacker != NULL && (unit->allyteam == attacker->allyteam)) { continue; }
		}
		if (p->GetCollisionFlags() & Collision::NOENEMIES) {
			if (attacker != NULL && (unit->allyteam != attacker->allyteam)) { continue; }
		}
		if (p->GetCollisionFlags() & Collision::NONEUTRALS) {
			if (unit->IsNeutral()) { continue; }
		}

		if (CCollisionHandler::DetectHit(unit, ppos0, ppos1, &cq)) {
			if (cq.lmp != NULL) {
				unit->SetLastAttackedPiece(cq.lmp, gs->frameNum);
			}

			const float3 newpos = (cq.b0) ? cq.p0 : cq.p1;
			if ((newpos - p->pos).dot(p->speed) >= 0.0f) { // only move the projectile forward
				p->pos = newpos;
			}
			p->Collision(unit);
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
	if (!p->checkCol) // already collided with unit?
		return;

	if ((p->GetCollisionFlags() & Collision::NOFEATURES) != 0)
		return;

	CollisionQuery cq;

	for (CFeature** fi = &tempFeatures[0]; fi != endFeature; ++fi) {
		CFeature* feature = *fi;

		if (!feature->blocking) {
			continue;
		}

		if (CCollisionHandler::DetectHit(feature, ppos0, ppos1, &cq)) {
			const float3 newpos = (cq.b0) ? cq.p0 : cq.p1;
			if ((newpos - p->pos).dot(p->speed) >= 0.0f) { // only move the projectile forward
				p->pos = newpos;
			}
			p->Collision(feature);
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
			CheckFeatureCollisions(p, tempFeatures, endFeature, ppos0, ppos1);
		}
	}
}

void CProjectileHandler::CheckGroundCollisions(ProjectileContainer& pc) {
	ProjectileContainer::iterator pci;

	for (pci = pc.begin(); pci != pc.end(); ++pci) {
		CProjectile* p = *pci;

		if (!p->checkCol) {
			continue;
		}

		// NOTE: if <p> is a MissileProjectile and does not
		// have selfExplode set, it will never be removed (!)
		if (p->GetCollisionFlags() & Collision::NOGROUND) {
			continue;
		}

		// NOTE: don't add p->radius to groundHeight, or most
		// projectiles will collide with the ground too early
		const float groundHeight = ground->GetHeightReal(p->pos.x, p->pos.z);
		const bool belowGround = (p->pos.y < groundHeight);
		const bool insideWater = (p->pos.y <= 0.0f && !belowGround);
		const bool ignoreWater = p->ignoreWater;

		if (belowGround || (insideWater && !ignoreWater)) {
			// if position has dropped below terrain or into water
			// where we cannot live, adjust it and explode us now
			// (if the projectile does not set deleteMe = true, it
			// will keep hugging the terrain)
			p->pos.y = belowGround? groundHeight: 0.0f;
			p->Collision();
		}
	}
}

void CProjectileHandler::CheckCollisions()
{
	SCOPED_TIMER("ProjectileHandler::CheckCollisions");

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


void CProjectileHandler::AddNanoParticle(
	const float3& startPos,
	const float3& endPos,
	const UnitDef* unitDef,
	int teamNum,
	bool highPriority)
{
	const float priority = highPriority? HIGH_NANO_PRIO: NORMAL_NANO_PRIO;

	if (currentNanoParticles >= (maxNanoParticles * priority))
		return;
	if (!unitDef->showNanoSpray)
		return;

	float3 dif = (endPos - startPos);
	const float l = fastmath::apxsqrt2(dif.SqLength());

	dif /= l;
	dif += gu->RandVector() * 0.15f;

	float3 color = unitDef->nanoColor;

	if (globalRendering->teamNanospray) {
		const CTeam* team = teamHandler->Team(teamNum);
		const unsigned char* tcol = team->color;
		color = float3(tcol[0] / 255.0f, tcol[1] / 255.0f, tcol[2] / 255.0f);
	}

	new CGfxProjectile(startPos, dif, int(l), color);
}

void CProjectileHandler::AddNanoParticle(
	const float3& startPos,
	const float3& endPos,
	const UnitDef* unitDef,
	int teamNum,
	float radius,
	bool inverse,
	bool highPriority)
{
	const float priority = highPriority? HIGH_NANO_PRIO: NORMAL_NANO_PRIO;

	if (currentNanoParticles >= (maxNanoParticles * priority))
		return;
	if (!unitDef->showNanoSpray)
		return;

	float3 dif = (endPos - startPos);
	const float l = fastmath::apxsqrt2(dif.SqLength());
	dif /= l;

	float3 error = gu->RandVector() * (radius / l);
	float3 color = unitDef->nanoColor;

	if (globalRendering->teamNanospray) {
		const CTeam* team = teamHandler->Team(teamNum);
		const unsigned char* tcol = team->color;
		color = float3(tcol[0] / 255.0f, tcol[1] / 255.0f, tcol[2] / 255.0f);
	}

	if (inverse) {
		new CGfxProjectile(startPos + (dif + error) * l, -(dif + error) * 3, int(l / 3), color);
	} else {
		new CGfxProjectile(startPos, (dif + error) * 3, int(l / 3), color);
	}
}
