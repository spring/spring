/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "Projectile.h"
#include "ProjectileHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
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

// reserve 5% of maxNanoParticles for important stuff such as capture and reclaim other teams' units
#define NORMAL_NANO_PRIO 0.95f
#define HIGH_NANO_PRIO 1.0f


using namespace std;

CONFIG(int, MaxParticles).defaultValue(3000).headlessValue(0);
CONFIG(int, MaxNanoParticles).defaultValue(2000).headlessValue(0);

CProjectileHandler* projectileHandler = NULL;


CR_BIND_TEMPLATE(ProjectileContainer, )
CR_REG_METADATA(ProjectileContainer, (
	CR_MEMBER(cont),
	CR_MEMBER(del),
	CR_POSTLOAD(PostLoad)
))
CR_BIND_TEMPLATE(GroundFlashContainer, )
CR_REG_METADATA(GroundFlashContainer, (
	CR_MEMBER(cont),
	CR_MEMBER(delObj),
	CR_POSTLOAD(PostLoad)
))

CR_BIND(CProjectileHandler, )
CR_REG_METADATA(CProjectileHandler, (
	CR_MEMBER(syncedProjectiles),
	CR_MEMBER(unsyncedProjectiles),
	CR_MEMBER_UN(flyingPieces3DO),
	CR_MEMBER_UN(flyingPiecesS3O),
	CR_MEMBER_UN(groundFlashes),

	CR_MEMBER(maxParticles),
	CR_MEMBER(maxNanoParticles),
	CR_MEMBER(currentParticles),
	CR_MEMBER(currentNanoParticles),
	CR_MEMBER(particleSaturation),

	CR_MEMBER(maxUsedSyncedID),
	CR_MEMBER(maxUsedUnsyncedID),

	//CR_MEMBER(syncedRenderProjectileIDs),
	//CR_MEMBER(unsyncedRenderProjectileIDs),

	CR_MEMBER(freeSyncedIDs),
	CR_MEMBER(freeUnsyncedIDs),
	CR_MEMBER(syncedProjectileIDs),
	CR_MEMBER(unsyncedProjectileIDs),

	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
))



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

	#define MAPPOS_SANITY_CHECK(v)                 \
		assert(v.x >= -(float3::maxxpos * 16.0f)); \
		assert(v.x <=  (float3::maxxpos * 16.0f)); \
		assert(v.z >= -(float3::maxzpos * 16.0f)); \
		assert(v.z <=  (float3::maxzpos * 16.0f)); \
		assert(v.y >= -MAX_PROJECTILE_HEIGHT);     \
		assert(v.y <=  MAX_PROJECTILE_HEIGHT);
	#define PROJECTILE_SANITY_CHECK(p) \
		p->pos.AssertNaNs();   \
		MAPPOS_SANITY_CHECK(p->pos);

	while (pci != pc.end()) {
		CProjectile* p = *pci;
		assert(p->synced == synced);
		assert(p->synced == !!(p->GetClass()->binder->flags & creg::CF_Synced));

		if (p->deleteMe) {
			ProjectileMap::iterator pIt;

			if (synced) {
				//! iterator is always valid
				pIt = syncedProjectileIDs.find(p->id);

				eventHandler.ProjectileDestroyed((pIt->second).first, (pIt->second).second);
				syncedRenderProjectileIDs.erase_delete(p);
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
				unsyncedRenderProjectileIDs.erase_delete(p);
				unsyncedProjectileIDs.erase(pIt);

				freeUnsyncedIDs.push_back(p->id);
#endif
				pci = pc.erase_detach(pci);
			}
		} else {
			PROJECTILE_SANITY_CHECK(p);

			p->Update();
			quadField->MovedProjectile(p);

			PROJECTILE_SANITY_CHECK(p);

			++pci;
		}
	}
}



void CProjectileHandler::Update()
{
	CheckCollisions();

	{
		SCOPED_TIMER("ProjectileHandler::Update");

		UpdateProjectileContainer(syncedProjectiles, true);
		UpdateProjectileContainer(unsyncedProjectiles, false);


		{
			if (syncedProjectiles.can_delete_synced()) {
				eventHandler.DeleteSyncedProjectiles();
				//! delete all projectiles that were
				//! queued (push_back'ed) for deletion
				syncedProjectiles.detach_erased_synced();
			}
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

			groundFlashes.delay_delete();
			groundFlashes.delay_add();
		}

		#define UPDATE_FLYING_PIECES(fpContainer)                      \
			FlyingPieceContainer::iterator pti = fpContainer.begin();  \
                                                                       \
			while (pti != fpContainer.end()) {                         \
				FlyingPiece* p = *pti;                                 \
				if (!p->Update()) {                                    \
					pti = fpContainer.erase_delete_set(pti);           \
				} else {                                               \
					++pti;                                             \
				}                                                      \
			}

		{ UPDATE_FLYING_PIECES(flyingPieces3DO); }
		{ UPDATE_FLYING_PIECES(flyingPiecesS3O); }
		#undef UPDATE_FLYING_PIECES

		{

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
	ProjectileMap* proIDs = NULL;
	ProjectileRenderMap* newProIDs = NULL;

	int* maxUsedID = NULL;
	int newUsedID = 0;

	if (p->synced) {
		syncedProjectiles.push(p);
		freeIDs = &freeSyncedIDs;
		proIDs = &syncedProjectileIDs;
		newProIDs = &syncedRenderProjectileIDs;
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
		newProIDs = &unsyncedRenderProjectileIDs;
		maxUsedID = &maxUsedUnsyncedID;
	}

	if (!freeIDs->empty()) {
		newUsedID = freeIDs->front();
		freeIDs->pop_front();
	} else {
		(*maxUsedID)++;
		newUsedID = *maxUsedID;
	}

	if ((*maxUsedID) > (1 << 24)) {
		LOG_L(L_WARNING, "Lua %s projectile IDs are now out of range", (p->synced? "synced": "unsynced"));
	}

	if (p->synced) {
		ASSERT_SYNCED(newUsedID);
	}

	p->id = newUsedID;

	const ProjectileMapValPair vp(p, p->GetAllyteamID());
	const ProjectileMapKeyPair kp(p->id, vp);

	proIDs->insert(kp);
	newProIDs->push(p, vp);

	eventHandler.ProjectileCreated(vp.first, vp.second);
}




void CProjectileHandler::CheckUnitCollisions(
	CProjectile* p,
	std::vector<CUnit*>& tempUnits,
	const float3& ppos0,
	const float3& ppos1)
{
	CollisionQuery cq;

	for (unsigned int n = 0; n < tempUnits.size(); n++) {
		CUnit* unit = tempUnits[n];

		if (unit == NULL)
			break;

		const CUnit* attacker = p->owner();

		// if this unit fired this projectile, always ignore
		if (attacker == unit)
			continue;
		if (!unit->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES))
			continue;

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
			if (cq.GetHitPiece() != NULL) {
				unit->SetLastAttackedPiece(cq.GetHitPiece(), gs->frameNum);
			}

			if (!cq.InsideHit()) {
				p->SetPosition(cq.GetHitPos());
				p->Collision(unit);
				p->SetPosition(ppos0);
			} else {
				p->Collision(unit);
			}

			break;
		}
	}
}

void CProjectileHandler::CheckFeatureCollisions(
	CProjectile* p,
	std::vector<CFeature*>& tempFeatures,
	const float3& ppos0,
	const float3& ppos1)
{
	// already collided with unit?
	if (!p->checkCol)
		return;

	if ((p->GetCollisionFlags() & Collision::NOFEATURES) != 0)
		return;

	CollisionQuery cq;

	for (unsigned int n = 0; n < tempFeatures.size(); n++) {
		CFeature* feature = tempFeatures[n];

		if (feature == NULL)
			break;

		if (!feature->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES))
			continue;

		if (CCollisionHandler::DetectHit(feature, ppos0, ppos1, &cq)) {
			if (!cq.InsideHit()) {
				p->SetPosition(cq.GetHitPos());
				p->Collision(feature);
				p->SetPosition(ppos0);
			} else {
				p->Collision(feature);
			}

			break;
		}
	}
}

void CProjectileHandler::CheckUnitFeatureCollisions(ProjectileContainer& pc) {
	static std::vector<CUnit*> tempUnits(unitHandler->MaxUnits(), NULL);
	static std::vector<CFeature*> tempFeatures(unitHandler->MaxUnits(), NULL);

	for (ProjectileContainer::iterator pci = pc.begin(); pci != pc.end(); ++pci) {
		CProjectile* p = *pci;

		if (!p->checkCol) continue;
		if ( p->deleteMe) continue;

		const float3 ppos0 = p->pos;
		const float3 ppos1 = p->pos + p->speed;

		quadField->GetUnitsAndFeaturesColVol(p->pos, p->radius + p->speed.w, tempUnits, tempFeatures);

		CheckUnitCollisions(p, tempUnits, ppos0, ppos1);
		CheckFeatureCollisions(p, tempFeatures, ppos0, ppos1);
	}
}

void CProjectileHandler::CheckGroundCollisions(ProjectileContainer& pc) {
	ProjectileContainer::iterator pci;

	for (pci = pc.begin(); pci != pc.end(); ++pci) {
		CProjectile* p = *pci;

		if (!p->checkCol)
			continue;

		// NOTE: if <p> is a MissileProjectile and does not
		// have selfExplode set, it will never be removed (!)
		if (p->GetCollisionFlags() & Collision::NOGROUND)
			continue;

		// NOTE: don't add p->radius to groundHeight, or most
		// projectiles will collide with the ground too early
		const float groundHeight = CGround::GetHeightReal(p->pos.x, p->pos.z);
		const bool belowGround = (p->pos.y < groundHeight);
		const bool insideWater = (p->pos.y <= 0.0f && !belowGround);
		const bool ignoreWater = p->ignoreWater;

		if (belowGround || (insideWater && !ignoreWater)) {
			// if position has dropped below terrain or into water
			// where we cannot live, adjust it and explode us now
			// (if the projectile does not set deleteMe = true, it
			// will keep hugging the terrain)
			p->pos.y = groundHeight * belowGround;
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


void CProjectileHandler::AddFlyingPiece(
	const float3& pos,
	const float3& speed,
	int team,
	const S3DOPiece* piece,
	const S3DOPrimitive* chunk)
{
	FlyingPiece* fp = new S3DOFlyingPiece(pos, speed, team, piece, chunk);
	flyingPieces3DO.insert(fp);
}

void CProjectileHandler::AddFlyingPiece(
	const float3& pos,
	const float3& speed,
	int team,
	int textureType,
	const SS3OVertex* chunk)
{
	assert(textureType > 0);

	FlyingPiece* fp = new SS3OFlyingPiece(pos, speed, team, textureType, chunk);
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

bool CProjectileHandler::RenderAccess(const CProjectile* p) const {
	const ProjectileMap* pmap = NULL;

	if (p->synced) {
		pmap = &(syncedRenderProjectileIDs.get_render_map());
	} else {
		#ifndef UNSYNCED_PROJ_NOEVENT
		pmap = &(unsyncedRenderProjectileIDs.get_render_map());
		#endif
	}

	return (pmap != NULL && pmap->find(p->id) != pmap->end());
}

