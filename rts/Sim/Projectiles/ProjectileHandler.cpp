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
#include "System/Cpp11Compat.hpp"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"


// reserve 5% of maxNanoParticles for important stuff such as capture and reclaim other teams' units
#define NORMAL_NANO_PRIO 0.95f
#define HIGH_NANO_PRIO 1.0f


CONFIG(int, MaxParticles).defaultValue(10000).headlessValue(0).minimumValue(0);
CONFIG(int, MaxNanoParticles).defaultValue(2000).headlessValue(0).minimumValue(0);


CR_BIND(CProjectileHandler, )
CR_REG_METADATA(CProjectileHandler, (
	CR_MEMBER(projectiles),
	CR_MEMBER_UN(flyingPieces),
	CR_MEMBER_UN(groundFlashes),
	CR_MEMBER_UN(resortFlyingPieces),

	CR_MEMBER(maxParticles),
	CR_MEMBER(maxNanoParticles),
	CR_MEMBER(currentNanoParticles),
	CR_MEMBER_UN(frameCurrentParticles),
	CR_MEMBER_UN(frameProjectileCounts)
))



// note: stores all ExpGenSpawnable types, not just projectiles
ProjMemPool projMemPool;

CProjectileHandler projectileHandler;



void CProjectileHandler::Init()
{
	currentNanoParticles = 0;
	frameCurrentParticles = 0;
	frameProjectileCounts[false] = 0;
	frameProjectileCounts[ true] = 0;

	resortFlyingPieces.fill(false);

	maxParticles     = configHandler->GetInt("MaxParticles");
	maxNanoParticles = configHandler->GetInt("MaxNanoParticles");

	projMemPool.clear();
	projMemPool.reserve(1024);

	for (int modelType = 0; modelType < MODELTYPE_CNT; ++modelType) {
		flyingPieces[modelType].clear();
		flyingPieces[modelType].reserve(1000);
	}

	projectiles[true ].SeedFreeKeys(0, maxParticles * 2, true); //seed only synced free ids.

	projectiles[false].reserve(static_cast<size_t>(maxParticles) * 2);

	// register ConfigNotify()
	configHandler->NotifyOnChange(this, {"MaxParticles", "MaxNanoParticles"});
}

void CProjectileHandler::Kill()
{
	configHandler->RemoveObserver(this);

	{
		// synced first, to avoid callback crashes
		for (CProjectile* p: projectiles[true])
			projMemPool.free(p);

		projectiles[true].clear();
	}

	{
		for (CProjectile* p: projectiles[false])
			projMemPool.free(p);

		projectiles[false].clear();
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

	CCollisionHandler::PrintStats();
}


void CProjectileHandler::ConfigNotify(const std::string& key, const std::string& value)
{
	maxParticles     = configHandler->GetInt("MaxParticles");
	maxNanoParticles = configHandler->GetInt("MaxNanoParticles");

	projectiles[false].reserve(static_cast<size_t>(maxParticles) * 2);
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

template<bool synced>
void CProjectileHandler::UpdateProjectilesImpl()
{
	auto& pc = projectiles[synced];

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

		// (delayed) creation for projectiles added after CheckCollisions()
		if (p->createMe)
			CreateProjectile(p);

		// deletion (FIXME: move outside of loop)
		if (p->deleteMe) {
			DestroyProjectile(p);
			continue;
		}

		// neither
		++i;
	}

	SCOPED_TIMER("Sim::Projectiles::Update");

	// WARNING: same as above but for p->Update()
	if constexpr (synced) {
		for (size_t i = 0; i < pc.size(); ++i) {
			CProjectile* p = pc[i];
			assert(p != nullptr);

			MAPPOS_SANITY_CHECK(p->pos);

			p->Update();
			quadField.MovedProjectile(p);

			MAPPOS_SANITY_CHECK(p->pos);
		}
	}
	else {
		for_mt_chunk(0, pc.size(), [&pc](int i) {
			CProjectile* p = pc[i];
			assert(p != nullptr);

			MAPPOS_SANITY_CHECK(p->pos);
			p->Update();
			MAPPOS_SANITY_CHECK(p->pos);
		});
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

	for (size_t i = 0; i < size; /*no-op*/) {
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

	for (size_t i = 0; i < size; /*no-op*/) {
		auto& p = cont[i];

		if (!p.Update()) {
			p = std::move(cont[size -= 1]);
			continue;
		}

		++i;
	}

	// WARNING: see UPDATE_PTR_CONTAINER
	assert(cont.size() == origSize);

	cont.erase(cont.begin() + size, cont.end());
}



void CProjectileHandler::CreateProjectile(CProjectile* p)
{
	p->createMe = false;

	if (p->synced || PH_UNSYNCED_PROJECTILE_EVENTS == 1)
		eventHandler.ProjectileCreated(p, p->GetAllyteamID());

	eventHandler.RenderProjectileCreated(p);
}

void CProjectileHandler::DestroyProjectile(CProjectile* p)
{
	assert(!p->createMe);

	eventHandler.RenderProjectileDestroyed(p);

	if (p->synced) {
		eventHandler.ProjectileDestroyed(p, p->GetAllyteamID());

		projectiles[true].Del(p->id);

		ASSERT_SYNCED(p->pos);
		ASSERT_SYNCED(p->id);
	} else {
	#if (PH_UNSYNCED_PROJECTILE_EVENTS == 1)
		eventHandler.ProjectileDestroyed(p, p->GetAllyteamID());
	#endif
		projectiles[false].Del(p->id);
	}

	projMemPool.free(p);
}

uint32_t CProjectileHandler::UnsyncedRandInt(uint32_t N) { return guRNG.NextInt(N); }
uint32_t CProjectileHandler::SyncedRandInt  (uint32_t N) { return gsRNG.NextInt(N); }

void CProjectileHandler::Update()
{
	{
		SCOPED_TIMER("Sim::Projectiles");

		// check if any projectiles have collided since the previous update
		CheckCollisions();
		UpdateProjectiles();

		UPDATE_PTR_CONTAINER(groundFlashes);

		// flying pieces; sort these every now and then
		for (int modelType = 0; modelType < MODELTYPE_CNT; ++modelType) {
			auto& fpc = flyingPieces[modelType];

			UPDATE_REF_CONTAINER(fpc);

			if (resortFlyingPieces[modelType]) {
				std::stable_sort(fpc.begin(), fpc.end());
			}
		}
	}

	// precache part of particles count calculation that else becomes very heavy
	frameCurrentParticles = 0;

	for (const CProjectile* p: projectiles[ true]) {
		frameCurrentParticles += p->GetProjectilesCount();
	}
	for (const CProjectile* p: projectiles[false]) {
		frameCurrentParticles += p->GetProjectilesCount();
	}

	frameProjectileCounts[ true] = projectiles[ true].size();
	frameProjectileCounts[false] = projectiles[false].size();
}

void CProjectileHandler::AddProjectile(CProjectile* p)
{
	// already initialized?
	assert(p->id < 0);
	assert(p->createMe);

	if (p->synced)
		p->id = static_cast<int>(projectiles[true ].Add(p, rngFuncs[true]));
	else
		p->id = static_cast<int>(projectiles[false].Add(p)); //don't bother with shuffling unsynced ids 

	if (p->synced) {
		ASSERT_SYNCED(freeIDs.size());
		ASSERT_SYNCED(p->id);
	}

	CreateProjectile(p);
}




static bool CheckProjectileCollisionFlags(const CProjectile* p, const CUnit* u)
{
	const unsigned int collFlags = p->GetCollisionFlags() * p->weapon;

	// only weapon-projectiles can have non-zero flags
	if (collFlags == 0)
		return false;

	// disregard everything else when this bit is set
	// (ground and feature flags are tested elsewhere)
	if ((collFlags & Collision::NONONTARGETS) != 0)
		return (static_cast<const CWeaponProjectile*>(p)->GetTargetObject() == u);

	if ((collFlags & Collision::NOCLOAKED) != 0 && u->IsCloaked())
		return false;
	if ((collFlags & Collision::NONEUTRALS) != 0 && u->IsNeutral())
		return false;

	if ((collFlags & Collision::NOFIREBASES) != 0) {
		const CUnit* owner = p->owner();
		const CUnit* trans = (owner != nullptr)? owner->GetTransporter(): nullptr;

		// check if the unit being collided with is occupied by p's owner
		if (u == trans && trans->unitDef->isFirePlatform)
			return false;
	}

	if (teamHandler.IsValidAllyTeam(p->GetAllyteamID())) {
		const bool noFriendsBit = ((collFlags & Collision::NOFRIENDLIES) != 0);
		const bool noEnemiesBit = ((collFlags & Collision::NOENEMIES   ) != 0);
		const bool friendlyFire = teamHandler.AlliedAllyTeams(p->GetAllyteamID(), u->allyteam);

		if (noFriendsBit && friendlyFire)
			return false;
		if (noEnemiesBit && !friendlyFire)
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
				unit->SetLastHitPiece(cq.GetHitPiece(), gs->frameNum, p->synced);

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
				feature->SetLastHitPiece(cq.GetHitPiece(), gs->frameNum, p->synced);

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
	// skip unsynced and non-weapon projectiles
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

		// we sometimes get false inside hits due to the movement of the shield
		// a very hacky solution is to nudge the start of the intersecting ray
		// back (proportional to how far the shield moved last frame) so as to
		// increase its length.
		// it's not 100% accurate so there's a bit of a FIXME here to do a real
		// solution (keep track in the projectile which shields it's in)
		const float3 rpvec  = ppos0 - ppos1;
		const float3 rppos0 = ppos0 + rpvec * repulser->GetDeltaDist();
		const float3 cvpos  = repulser->weaponMuzzlePos - repulser->owner->relMidPos;

		// shield volumes are always spherical, transform directly
		// (CollisionHandler will cancel out the relmidpos offset)
		if (!CCollisionHandler::DetectHit(repulser->owner, &repulser->collisionVolume, CMatrix44f{cvpos}, rppos0, ppos1, &cq))
			continue;

		if (cq.InsideHit() && repulser->IgnoreInteriorHit(wpro))
			continue;

		if (repulser->IncomingProjectile(wpro, cq.GetHitPos()))
			return;
	}
}

void CProjectileHandler::CheckUnitFeatureCollisions(bool synced)
{
	static std::vector<CUnit*> tempUnits;
	static std::vector<CFeature*> tempFeatures;
	static std::vector<CPlasmaRepulser*> tempRepulsers;

	//can't use iterators here, because instructions inside the loop modify projectiles[synced]
	for (size_t i = 0; i < projectiles[synced].size(); ++i) {
		CProjectile* p = projectiles[synced][i];

		if (!p->checkCol) continue;
		if ( p->deleteMe) continue;

		const float3 ppos0 = p->pos;
		const float3 ppos1 = p->pos + p->speed;
		// const float3 ppos1 = p->pos + p->dir * (p->speed.w + p->radius);

		quadField.GetUnitsAndFeaturesColVol(p->pos, p->speed.w + p->radius, tempUnits, tempFeatures, &tempRepulsers);

		CheckShieldCollisions (p, tempRepulsers, ppos0, ppos1); tempRepulsers.clear();
		CheckUnitCollisions   (p, tempUnits    , ppos0, ppos1); tempUnits.clear();
		CheckFeatureCollisions(p, tempFeatures , ppos0, ppos1); tempFeatures.clear();
	}
}

void CProjectileHandler::CheckGroundCollisions(bool synced)
{
	//can't use iterators here, because instructions inside the loop modify projectiles[synced]
	for (size_t i = 0; i < projectiles[synced].size(); ++i) {
		CProjectile* p = projectiles[synced][i];

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

	CheckUnitFeatureCollisions(true ); // changes simulation state
	CheckUnitFeatureCollisions(false); // does not change simulation state

	CheckGroundCollisions(true ); // changes simulation state
	CheckGroundCollisions(false); // does not change simulation state
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
	bool highPriority
) {
	const float priority = mix(NORMAL_NANO_PRIO, HIGH_NANO_PRIO, highPriority);
	const float emitProb = 1.0f - GetNanoParticleSaturation(priority);

	if (emitProb < guRNG.NextFloat())
		return;
	if (!unitDef->showNanoSpray)
		return;

	float3 dif = endPos - startPos;
	const float l = fastmath::apxsqrt2(dif.SqLength());

	dif /= l;
	dif += (guRNG.NextVector() * 0.15f);

	const     float3 udColor = unitDef->nanoColor;
	constexpr float  udAlpha = 20 / 256.0f; // denom=255 is not constexpr-able

	const     uint8_t* tColor = (teamHandler.Team(teamNum))->color;
	constexpr uint8_t  tAlpha = udAlpha * 256;

	const SColor colors[2] = {
		{udColor.r, udColor.g, udColor.b, udAlpha},
		{tColor[0], tColor[1], tColor[2],  tAlpha},
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
	bool highPriority
) {
	const float priority = mix(NORMAL_NANO_PRIO, HIGH_NANO_PRIO, highPriority);
	const float emitProb = 1.0f - GetNanoParticleSaturation(priority);

	if (emitProb < guRNG.NextFloat())
		return;
	if (!unitDef->showNanoSpray)
		return;

	float3 dif = endPos - startPos;
	const float len = fastmath::apxsqrt2(dif.SqLength());

	dif /= len;
	dif += (guRNG.NextVector() * (radius / len));

	const     float3 udColor = unitDef->nanoColor;
	constexpr float  udAlpha = 20 / 256.0f;

	const     uint8_t* tColor = (teamHandler.Team(teamNum))->color;
	constexpr uint8_t  tAlpha = udAlpha * 256;

	const SColor colors[2] = {
		{udColor.r, udColor.g, udColor.b, udAlpha},
		{tColor[0], tColor[1], tColor[2],  tAlpha},
	};

	if (!inverse) {
		projMemPool.alloc<CNanoProjectile>(startPos, dif * 3.0f, int(len / 3.0f), colors[globalRendering->teamNanospray]);
	} else {
		projMemPool.alloc<CNanoProjectile>(startPos + dif * len, -dif * 3.0f, int(len / 3.0f), colors[globalRendering->teamNanospray]);
	}
}

float CProjectileHandler::GetParticleSaturation(bool randomized) const
{
	const int curParticles = GetCurrentParticles();

	// use the random mult to weaken the max limit a little
	// so the chance is better spread when being close to the limit
	// i.e. when there are rockets that spam CEGs this gives smaller CEGs still a chance
	const float total = std::max(1.0f, maxParticles * 1.0f);
	const float fract = curParticles / total;
	const float rmult = 1.0f + (int(randomized) * 0.3f * guRNG.NextFloat());

	return (fract * rmult);
}

int CProjectileHandler::GetCurrentParticles() const
{
	// use precached part of particles count calculation that else becomes very heavy
	// example where it matters: (in ZK) /cheat /give 20 armraven -> shoot ground
	for (size_t i = frameProjectileCounts[true], e = projectiles[true].size(); i < e; ++i) {
		frameCurrentParticles += projectiles[true][i]->GetProjectilesCount();
	}
	frameProjectileCounts[true ] = projectiles[true ].size();

	for (size_t i = frameProjectileCounts[false], e = projectiles[false].size(); i < e; ++i) {
		frameCurrentParticles += projectiles[false][i]->GetProjectilesCount();
	}
	frameProjectileCounts[false] = projectiles[false].size();

	int partCount = frameCurrentParticles;
	for (const auto& c: flyingPieces) {
		for (const auto& fp: c) {
			partCount += fp.GetDrawCallCount();
		}
	}
	partCount += groundFlashes.size();
	return partCount;
}

