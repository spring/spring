/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "Projectile.h"
#include "ProjectileHandler.h"
#include "ProjectileMemPool.h"
#include "Game/GlobalUnsynced.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GroundFlash.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Rendering/Env/Particles/Classes/FlyingPiece.h"
#include "Rendering/Env/Particles/Classes/NanoProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/creg/STL_Deque.h"


// reserve 5% of maxNanoParticles for important stuff such as capture and reclaim other teams' units
#define NORMAL_NANO_PRIO 0.95f
#define HIGH_NANO_PRIO 1.0f


using namespace std;

CONFIG(int, MaxParticles).defaultValue(10000).headlessValue(1).minimumValue(1);
CONFIG(int, MaxNanoParticles).defaultValue(2000).headlessValue(1).minimumValue(1);


CR_BIND(CProjectileHandler, )
CR_REG_METADATA(CProjectileHandler, (
	CR_MEMBER(syncedProjectiles),
	CR_MEMBER(unsyncedProjectiles),
	CR_MEMBER_UN(flyingPieces),
	CR_MEMBER_UN(groundFlashes),
	CR_MEMBER_UN(resortFlyingPieces),

	CR_MEMBER(maxParticles),
	CR_MEMBER(maxNanoParticles),
	CR_MEMBER(currentNanoParticles),
	CR_MEMBER_UN(lastCurrentParticles),
	CR_MEMBER_UN(lastSyncedProjectilesCount),
	CR_MEMBER_UN(lastUnsyncedProjectilesCount),

	CR_MEMBER(freeSyncedIDs),
	CR_MEMBER(freeUnsyncedIDs),
	CR_MEMBER(syncedProjectileIDs),
	CR_MEMBER(unsyncedProjectileIDs)
))



// note: stores all ExpGenSpawnable types, not just projectiles
ProjMemPool projMemPool;

CProjectileHandler projectileHandler;



void CProjectileHandler::Init()
{
	currentNanoParticles = 0;
	lastCurrentParticles = 0;
	lastSyncedProjectilesCount = 0;
	lastUnsyncedProjectilesCount = 0;

	resortFlyingPieces.fill(false);

	syncedProjectileIDs.clear();
	syncedProjectileIDs.resize(1024, nullptr);

#if UNSYNCED_PROJ_NOEVENT
	unsyncedProjectileIDs.clear();
	unsyncedProjectileIDs.resize(0, nullptr);
#else
	unsyncedProjectileIDs.clear();
	unsyncedProjectileIDs.resize(8192, nullptr);
#endif

	maxParticles     = configHandler->GetInt("MaxParticles");
	maxNanoParticles = configHandler->GetInt("MaxNanoParticles");

	projMemPool.clear();
	projMemPool.reserve(1024);

	{
		// preload some IDs
		for (int i = 0; i < syncedProjectileIDs.size(); i++) {
			freeSyncedIDs.push_back(i);
		}

		for (int i = 0; i < unsyncedProjectileIDs.size(); i++) {
			freeUnsyncedIDs.push_back(i);
		}

		std::random_shuffle(freeSyncedIDs.begin(), freeSyncedIDs.end(), gsRNG);
		std::random_shuffle(freeUnsyncedIDs.begin(), freeUnsyncedIDs.end(), guRNG);
	}

	for (int modelType = 0; modelType < MODELTYPE_OTHER; ++modelType) {
		flyingPieces[modelType].clear();
		flyingPieces[modelType].reserve(1000);
	}

	// register ConfigNotify()
	configHandler->NotifyOnChange(this, {"MaxParticles", "MaxNanoParticles"});
}

void CProjectileHandler::Kill()
{
	configHandler->RemoveObserver(this);

	{
		// synced first, to avoid callback crashes
		for (CProjectile* p: syncedProjectiles)
			projMemPool.free(p);

		syncedProjectiles.clear();
	}

	{
		for (CProjectile* p: unsyncedProjectiles)
			projMemPool.free(p);

		unsyncedProjectiles.clear();
	}

	{
		for (CGroundFlash* gf: groundFlashes)
			projMemPool.free(gf);

		groundFlashes.clear();
	}

	{
		for (auto& fpc: flyingPieces) {
			fpc.clear();
		}
	}

	freeSyncedIDs.clear();
	freeUnsyncedIDs.clear();

	syncedProjectileIDs.clear();
	unsyncedProjectileIDs.clear();

	CCollisionHandler::PrintStats();
}


void CProjectileHandler::ConfigNotify(const std::string& key, const std::string& value)
{
	maxParticles     = configHandler->GetInt("MaxParticles");
	maxNanoParticles = configHandler->GetInt("MaxNanoParticles");
}


static void MAPPOS_SANITY_CHECK(const float3 v)
{
	v.AssertNaNs();
	assert(v.x >= -(float3::maxxpos * 16.0f));
	assert(v.x <=  (float3::maxxpos * 16.0f));
	assert(v.z >= -(float3::maxzpos * 16.0f));
	assert(v.z <=  (float3::maxzpos * 16.0f));
	assert(v.y >= -MAX_PROJECTILE_HEIGHT);
	assert(v.y <=  MAX_PROJECTILE_HEIGHT);
}


void CProjectileHandler::UpdateProjectileContainer(ProjectileContainer& pc, bool synced)
{
	// WARNING:
	//   we can't use iterators here because ProjectileCreated
	//   and ProjectileDestroyed events may add new projectiles
	//   to the container!
	for (size_t i = 0; i < pc.size(); /*no-op*/) {
		CProjectile* p = pc[i];

		assert(p != nullptr);
		assert(p->synced == synced);
#ifdef USING_CREG
		assert(p->synced == !!(p->GetClass()->flags & creg::CF_Synced));
#endif

		// creation
		if (p->callEvent) {
			if (synced || !UNSYNCED_PROJ_NOEVENT)
				eventHandler.ProjectileCreated(p, p->GetAllyteamID());

			eventHandler.RenderProjectileCreated(p);
			p->callEvent = false;
		}

		// deletion (FIXME: move outside of loop)
		if (p->deleteMe) {
			pc[i] = pc.back();
			pc.pop_back();

			eventHandler.RenderProjectileDestroyed(p);

			if (synced) {
				eventHandler.ProjectileDestroyed(p, p->GetAllyteamID());
				syncedProjectileIDs[p->id] = nullptr;
				freeSyncedIDs.push_back(p->id);

				ASSERT_SYNCED(p->pos);
				ASSERT_SYNCED(p->id);
			} else {
			#if !UNSYNCED_PROJ_NOEVENT
				eventHandler.ProjectileDestroyed(p, p->GetAllyteamID());
				unsyncedProjectileIDs[p->id] = nullptr;
				freeUnsyncedIDs.push_back(p->id);
			#endif
			}

			projMemPool.free(p);
			continue;
		}

		// neither
		++i;
	}

	SCOPED_TIMER("Sim::Projectiles::Update");

	// WARNING: same as above but for p->Update()
	for (size_t i = 0; i < pc.size(); ++i) {
		CProjectile* p = pc[i];
		assert(p != nullptr);

		MAPPOS_SANITY_CHECK(p->pos);

		p->Update();
		quadField.MovedProjectile(p);

		MAPPOS_SANITY_CHECK(p->pos);
	}
}


template<class T>
static void UPDATE_PTR_CONTAINER(T& cont) {
	if (cont.empty())
		return;

#ifndef NDEBUG
	const size_t origSize = cont.size();
#endif
	size_t size = cont.size();

	for (unsigned int i = 0; i < size; /*no-op*/) {
		CGroundFlash*& gf = cont[i];

		if (!gf->Update()) {
			projMemPool.free(gf);
			gf = cont[size -= 1];
			continue;
		}

		++i;
	}

	// WARNING:
	//   check if the vector was enlarged while iterating, in
	//   which case we will have missed updating newest items
	assert(cont.size() == origSize);

	cont.erase(cont.begin() + size, cont.end());
}

template<class T>
static void UPDATE_REF_CONTAINER(T& cont) {
	if (cont.empty())
		return;

#ifndef NDEBUG
	const size_t origSize = cont.size();
#endif
	size_t size = cont.size();

	for (unsigned int i = 0; i < size; /*no-op*/) {
		auto& p = cont[i];

		if (!p.Update()) {
			p = std::move(cont[size -= 1]);
			continue;
		}

		++i;
	}

	//WARNING: check if the vector got enlarged while iterating, in that case
	// we didn't update the newest items
	assert(cont.size() == origSize);

	cont.erase(cont.begin() + size, cont.end());
}


void CProjectileHandler::Update()
{
	{
		SCOPED_TIMER("Sim::Projectiles");

		// particles
		CheckCollisions(); // before :Update() to check if the particles move into stuff
		UpdateProjectileContainer(syncedProjectiles, true);
		UpdateProjectileContainer(unsyncedProjectiles, false);

		// groundflashes
		UPDATE_PTR_CONTAINER(groundFlashes);

		// flying pieces; sort these every now and then
		for (int modelType = 0; modelType < MODELTYPE_OTHER; ++modelType) {
			auto& fpc = flyingPieces[modelType];

			UPDATE_REF_CONTAINER(fpc);

			if (resortFlyingPieces[modelType]) {
				std::stable_sort(fpc.begin(), fpc.end());
			}
		}
	}

	// precache part of particles count calculation that else becomes very heavy
	lastCurrentParticles = 0;
	for (const CProjectile* p: syncedProjectiles) {
		lastCurrentParticles += p->GetProjectilesCount();
	}
	for (const CProjectile* p: unsyncedProjectiles) {
		lastCurrentParticles += p->GetProjectilesCount();
	}
	lastSyncedProjectilesCount = syncedProjectiles.size();
	lastUnsyncedProjectilesCount = unsyncedProjectiles.size();
}



void CProjectileHandler::AddProjectile(CProjectile* p)
{
	// already initialized?
	assert(p->id < 0);
	assert(p->callEvent);

	std::deque<int>* freeIDs = nullptr;
	ProjectileMap* proIDs = nullptr;

	if (p->synced) {
		syncedProjectiles.push_back(p);

		freeIDs = &freeSyncedIDs;
		proIDs = &syncedProjectileIDs;
		ASSERT_SYNCED(freeIDs->size());
	} else {
		unsyncedProjectiles.push_back(p);
#if UNSYNCED_PROJ_NOEVENT
		return;
#endif

		freeIDs = &freeUnsyncedIDs;
		proIDs = &unsyncedProjectileIDs;
	}

	if (freeIDs->empty()) {
		const size_t oldSize = proIDs->size();
		const size_t newSize = oldSize + 256;
		for (int i = oldSize; i < newSize; i++) {
			freeIDs->push_back(i);
		}
		if (p->synced) {
			std::random_shuffle(freeIDs->begin(), freeIDs->end(), gsRNG);
		} else{
			std::random_shuffle(freeIDs->begin(), freeIDs->end(), guRNG);
		}
		proIDs->resize(newSize, nullptr);
	}

	p->id = freeIDs->front();
	freeIDs->pop_front();
	(*proIDs)[p->id] = p;

	if ((p->id) > (1 << 24)) {
		LOG_L(L_WARNING, "Lua %s projectile IDs are now out of range", (p->synced? "synced": "unsynced"));
	}

	if (p->synced) {
		ASSERT_SYNCED(p->id);
#ifdef TRACE_SYNC
		tracefile << "New projectile id: " << p->id << ", ownerID: " << p->GetOwnerID();
		tracefile << ", type: " << p->GetProjectileType() << " pos: <" << p->pos.x << ", " << p->pos.y << ", " << p->pos.z << ">\n";
#endif
	}
}




static bool CheckProjectileCollisionFlags(const CProjectile* p, const CUnit* u)
{
	if (teamHandler.IsValidAllyTeam(p->GetAllyteamID())) {
		const bool noFriendsBit = ((p->GetCollisionFlags() & Collision::NOFRIENDLIES) != 0);
		const bool noEnemiesBit = ((p->GetCollisionFlags() & Collision::NOENEMIES   ) != 0);
		const bool friendlyFire = teamHandler.AlliedAllyTeams(p->GetAllyteamID(), u->allyteam);

		if (noFriendsBit && friendlyFire)
			return false;

		if (noEnemiesBit && !friendlyFire)
			return false;
	}

	if ((p->GetCollisionFlags() & Collision::NONEUTRALS) != 0 && u->IsNeutral())
		return false;

	if ((p->GetCollisionFlags() & Collision::NOFIREBASES) != 0) {
		const CUnit* owner = p->owner();
		const CUnit* trans = (owner != nullptr)? owner->GetTransporter(): nullptr;

		// check if the unit being collided with is occupied by p's owner
		if (u == trans && trans->unitDef->isFirePlatform)
			return false;
	}

	return true;
}


void CProjectileHandler::CheckUnitCollisions(
	CProjectile* p,
	std::vector<CUnit*>& tempUnits,
	const float3 ppos0,
	const float3 ppos1
) {
	if (!p->checkCol)
		return;

	CollisionQuery cq;

	for (CUnit* unit: tempUnits) {
		assert(unit != nullptr);

		// if this unit fired this projectile, always ignore
		if (unit == p->owner())
			continue;
		if (!unit->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES))
			continue;

		if (!CheckProjectileCollisionFlags(p, unit))
			continue;

		if (CCollisionHandler::DetectHit(unit, unit->GetTransformMatrix(true), ppos0, ppos1, &cq)) {
			if (cq.GetHitPiece() != nullptr)
				unit->SetLastHitPiece(cq.GetHitPiece(), gs->frameNum);

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
	const float3 ppos0,
	const float3 ppos1
) {
	// already collided with unit?
	if (!p->checkCol)
		return;

	if ((p->GetCollisionFlags() & Collision::NOFEATURES) != 0)
		return;

	CollisionQuery cq;

	for (CFeature* feature: tempFeatures) {
		assert(feature != nullptr);

		if (!feature->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES))
			continue;

		if (CCollisionHandler::DetectHit(feature, feature->GetTransformMatrix(true), ppos0, ppos1, &cq)) {
			if (cq.GetHitPiece() != nullptr)
				feature->SetLastHitPiece(cq.GetHitPiece(), gs->frameNum);

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


void CProjectileHandler::CheckShieldCollisions(
	CProjectile* p,
	std::vector<CPlasmaRepulser*>& tempRepulsers,
	const float3 ppos0,
	const float3 ppos1
) {
	if (!p->checkCol)
		return;
	if (!p->weapon)
		return;

	CWeaponProjectile* wpro = static_cast<CWeaponProjectile*>(p);
	const WeaponDef* wdef = wpro->GetWeaponDef();

	const unsigned int interceptType = wdef->interceptedByShieldType;
	const unsigned int projAllyTeam = p->GetAllyteamID();

	// bail early
	if (interceptType == 0)
		return;

	CollisionQuery cq;

	for (CPlasmaRepulser* repulser: tempRepulsers) {
		assert(repulser != nullptr);

		if (!repulser->CanIntercept(interceptType, projAllyTeam))
			continue;

		CUnit* owner = repulser->owner;

		// we sometimes get false inside hits due to the movement of the shield
		// a very hacky solution is to nudge the start of the intersecting ray
		// back (proportional to how far the shield moved last frame) so as to
		// increase its length.
		// it's not 100% accurate so there's a bit of a FIXME here to do a real
		// solution (keep track in the projectile which shields it's in)
		const float3 rpvec  = ppos0 - ppos1;
		const float3 rppos0 = ppos0 + rpvec * repulser->GetDeltaDist();

		if (!CCollisionHandler::DetectHit(owner, &repulser->collisionVolume, owner->GetTransformMatrix(true), rppos0, ppos1, &cq))
			continue;

		if (cq.InsideHit() && repulser->weaponDef->exteriorShield && !repulser->IsRepulsing(wpro))
			continue;

		if (repulser->IncomingProjectile(wpro, cq.GetHitPos()))
			return;
	}
}

void CProjectileHandler::CheckUnitFeatureCollisions(ProjectileContainer& pc)
{
	static std::vector<CUnit*> tempUnits;
	static std::vector<CFeature*> tempFeatures;
	static std::vector<CPlasmaRepulser*> tempRepulsers;

	for (size_t i = 0; i < pc.size(); ++i) {
		CProjectile* p = pc[i];

		if (!p->checkCol) continue;
		if ( p->deleteMe) continue;

		const float3 ppos0 = p->pos;
		// const float3 ppos1 = p->pos + p->dir * (p->speed.w + p->radius);

		quadField.GetUnitsAndFeaturesColVol(p->pos, p->speed.w + p->radius, tempUnits, tempFeatures, &tempRepulsers);

		CheckShieldCollisions(p, tempRepulsers, ppos0, ppos1); tempRepulsers.clear();
		CheckUnitCollisions(p, tempUnits, ppos0, ppos1); tempUnits.clear();
		CheckFeatureCollisions(p, tempFeatures, ppos0, ppos1); tempFeatures.clear();
	}
}

void CProjectileHandler::CheckGroundCollisions(ProjectileContainer& pc)
{
	for (size_t i = 0; i < pc.size(); ++i) {
		CProjectile* p = pc[i];

		if (!p->checkCol)
			continue;

		// NOTE:
		//   if <p> is a MissileProjectile and does not have
		//   selfExplode set, tbis will cause it to never be
		//   removed (!)
		if (p->GetCollisionFlags() & Collision::NOGROUND)
			continue;

		// don't collide with ground yet if last update scheduled a bounce
		if (p->weapon && static_cast<const CWeaponProjectile*>(p)->HasScheduledBounce())
			continue;

		// NOTE:
		//   don't add p->radius to groundHeight, or most (esp. modelled)
		//   projectiles will collide with the ground one or more frames
		//   too early
		const float gy = CGround::GetHeightReal(p->pos.x, p->pos.z);
		const float py = p->pos.y;

		const bool belowGround = (py < gy);
		const bool insideWater = (py <= 0.0f);

		if (!belowGround && (!insideWater || p->ignoreWater))
			continue;

		// if position has dropped below terrain or into water
		// where we can not live, adjust it and explode us now
		// (if the projectile does not set deleteMe = true, it
		// will keep hugging the terrain)
		p->SetPosition((p->pos * XZVector) + (UpVector * mix(py, gy, belowGround)));
		p->Collision();
	}
}

void CProjectileHandler::CheckCollisions()
{
	SCOPED_TIMER("Sim::Projectiles::Collisions");

	CheckUnitFeatureCollisions(syncedProjectiles); //! changes simulation state
	CheckUnitFeatureCollisions(unsyncedProjectiles); //! does not change simulation state

	CheckGroundCollisions(syncedProjectiles); //! changes simulation state
	CheckGroundCollisions(unsyncedProjectiles); //! does not change simulation state
}



void CProjectileHandler::AddGroundFlash(CGroundFlash* flash)
{
	groundFlashes.push_back(flash);
}


void CProjectileHandler::AddFlyingPiece(
	int modelType,
	const S3DModelPiece* piece,
	const CMatrix44f& m,
	const float3 pos,
	const float3 speed,
	const float2 pieceParams,
	const int2 renderParams
) {
	flyingPieces[modelType].emplace_back(piece, m, pos, speed, pieceParams, renderParams);
	resortFlyingPieces[modelType] = true;
}


void CProjectileHandler::AddNanoParticle(
	const float3 startPos,
	const float3 endPos,
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
	dif += guRNG.NextVector() * 0.15f;

	const float3 udColor = unitDef->nanoColor;
	const uint8_t* tColor = (teamHandler.Team(teamNum))->color;

	const SColor colors[2] = {
		SColor(udColor.r, udColor.g, udColor.b, 20.0f / 255.0f),
		SColor(tColor[0], tColor[1], tColor[2], uint8_t(20)),
	};

	projMemPool.alloc<CNanoProjectile>(startPos, dif, int(l), colors[globalRendering->teamNanospray]);
}

void CProjectileHandler::AddNanoParticle(
	const float3 startPos,
	const float3 endPos,
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

	float3 error = guRNG.NextVector() * (radius / l);

	const float3 udColor = unitDef->nanoColor;
	const uint8_t* tColor = (teamHandler.Team(teamNum))->color;

	const SColor colors[2] = {
		SColor(udColor.r, udColor.g, udColor.b, 20.0f / 255.0f),
		SColor(tColor[0], tColor[1], tColor[2], uint8_t(20)),
	};

	if (!inverse) {
		projMemPool.alloc<CNanoProjectile>(startPos, (dif + error) * 3, int(l / 3), colors[globalRendering->teamNanospray]);
	} else {
		projMemPool.alloc<CNanoProjectile>(startPos + (dif + error) * l, -(dif + error) * 3, int(l / 3), colors[globalRendering->teamNanospray]);
	}
}


CProjectile* CProjectileHandler::GetProjectileBySyncedID(int id)
{
	if ((size_t)id < syncedProjectileIDs.size())
		return syncedProjectileIDs[id];
	return nullptr;
}


CProjectile* CProjectileHandler::GetProjectileByUnsyncedID(int id)
{
	if (UNSYNCED_PROJ_NOEVENT)
		return nullptr; // unsynced projectiles have no IDs if UNSYNCED_PROJ_NOEVENT

	if ((size_t)id < unsyncedProjectileIDs.size())
		return unsyncedProjectileIDs[id];
	return nullptr;
}


float CProjectileHandler::GetParticleSaturation(const bool withRandomization) const
{
	// use the random mult to weaken the max limit a little
	// so the chance is better spread when being close to the limit
	// i.e. when there are rockets that spam CEGs this gives smaller CEGs still a chance
	return (GetCurrentParticles() / float(maxParticles)) * (1.f + int(withRandomization) * 0.3f * guRNG.NextFloat());
}

int CProjectileHandler::GetCurrentParticles() const
{
	// use precached part of particles count calculation that else becomes very heavy
	// example where it matters: (in ZK) /cheat /give 20 armraven -> shoot ground
	int partCount = lastCurrentParticles;
	for (size_t i = lastSyncedProjectilesCount, e = syncedProjectiles.size(); i < e; ++i) {
		partCount += syncedProjectiles[i]->GetProjectilesCount();
	}
	for (size_t i = lastUnsyncedProjectilesCount, e = unsyncedProjectiles.size(); i < e; ++i) {
		partCount += unsyncedProjectiles[i]->GetProjectilesCount();
	}
	for (const auto& c: flyingPieces) {
		for (const auto& fp: c) {
			partCount += fp.GetDrawCallCount();
		}
	}
	partCount += groundFlashes.size();
	return partCount;
}
