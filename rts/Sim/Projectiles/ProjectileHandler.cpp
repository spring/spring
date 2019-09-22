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
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"


// reserve 5% of maxNanoParticles for important stuff such as capture and reclaim other teams' units
#define NORMAL_NANO_PRIO 0.95f
#define HIGH_NANO_PRIO 1.0f


CONFIG(int, MaxParticles).defaultValue(10000).headlessValue(0).minimumValue(0);
CONFIG(int, MaxNanoParticles).defaultValue(2000).headlessValue(0).minimumValue(0);


CR_BIND(CProjectileHandler, )
CR_REG_METADATA(CProjectileHandler, (
	CR_MEMBER(projectileContainers),
	CR_MEMBER_UN(flyingPieces),
	CR_MEMBER_UN(groundFlashes),
	CR_MEMBER_UN(resortFlyingPieces),

	CR_MEMBER(maxParticles),
	CR_MEMBER(maxNanoParticles),
	CR_MEMBER(currentNanoParticles),
	CR_MEMBER_UN(lastCurrentParticles),
	CR_MEMBER_UN(lastProjectileCounts),

	CR_MEMBER(freeProjectileIDs),
	CR_MEMBER(projectileMaps)
))



// note: stores all ExpGenSpawnable types, not just projectiles
ProjMemPool projMemPool;

CProjectileHandler projectileHandler;



void CProjectileHandler::Init()
{
	currentNanoParticles = 0;
	lastCurrentParticles = 0;
	lastProjectileCounts[false] = 0;
	lastProjectileCounts[ true] = 0;

	resortFlyingPieces.fill(false);

	projectileMaps[true].clear();
	projectileMaps[true].resize(1024, nullptr);

#if UNSYNCED_PROJ_NOEVENT
	projectileMaps[false].clear();
	projectileMaps[false].resize(0, nullptr);
#else
	projectileMaps[false].clear();
	projectileMaps[false].resize(8192, nullptr);
#endif

	maxParticles     = configHandler->GetInt("MaxParticles");
	maxNanoParticles = configHandler->GetInt("MaxNanoParticles");

	projMemPool.clear();
	projMemPool.reserve(1024);

	{
		freeProjectileIDs[ true].reserve(projectileMaps[true].size());
		freeProjectileIDs[false].reserve(projectileMaps[false].size());

		// preload some IDs
		for (int i = 0; i < projectileMaps[true].size(); i++) {
			freeProjectileIDs[true].push_back(i);
		}

		for (int i = 0; i < projectileMaps[false].size(); i++) {
			freeProjectileIDs[false].push_back(i);
		}

		std::random_shuffle(freeProjectileIDs[ true].begin(), freeProjectileIDs[ true].end(), gsRNG);
		std::random_shuffle(freeProjectileIDs[false].begin(), freeProjectileIDs[false].end(), guRNG);
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
		for (CProjectile* p: projectileContainers[true])
			projMemPool.free(p);

		projectileContainers[true].clear();
	}

	{
		for (CProjectile* p: projectileContainers[false])
			projMemPool.free(p);

		projectileContainers[false].clear();
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

	freeProjectileIDs[ true].clear();
	freeProjectileIDs[false].clear();

	projectileMaps[ true].clear();
	projectileMaps[false].clear();

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


void CProjectileHandler::UpdateProjectileContainer(bool synced)
{
	ProjectileContainer& pc = projectileContainers[synced];

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

				projectileMaps[true][p->id] = nullptr;
				freeProjectileIDs[true].push_back(p->id);

				ASSERT_SYNCED(p->pos);
				ASSERT_SYNCED(p->id);
			} else {
			#if !UNSYNCED_PROJ_NOEVENT
				eventHandler.ProjectileDestroyed(p, p->GetAllyteamID());

				projectileMaps[false][p->id] = nullptr;
				freeProjectileIDs[false].push_back(p->id);
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

	// WARNING: see UPDATE_PTR_CONTAINER
	assert(cont.size() == origSize);

	cont.erase(cont.begin() + size, cont.end());
}


void CProjectileHandler::Update()
{
	{
		SCOPED_TIMER("Sim::Projectiles");

		// particles
		CheckCollisions(); // before :Update() to check if the particles move into stuff
		UpdateProjectileContainer( true);
		UpdateProjectileContainer(false);

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

	for (const CProjectile* p: projectileContainers[true]) {
		lastCurrentParticles += p->GetProjectilesCount();
	}
	for (const CProjectile* p: projectileContainers[false]) {
		lastCurrentParticles += p->GetProjectilesCount();
	}

	lastProjectileCounts[ true] = projectileContainers[true].size();
	lastProjectileCounts[false] = projectileContainers[false].size();
}




static unsigned int UnsyncedRandInt(unsigned int N) { return (guRNG.NextInt(N)); }
static unsigned int   SyncedRandInt(unsigned int N) { return (gsRNG.NextInt(N)); }

void CProjectileHandler::AddProjectile(CProjectile* p)
{
	// already initialized?
	assert(p->id < 0);
	assert(p->callEvent);

	static constexpr decltype(&UnsyncedRandInt) rngFuncs[] = {&UnsyncedRandInt, &SyncedRandInt};

	auto& freeIDs = freeProjectileIDs[p->synced];
	auto& projMap =    projectileMaps[p->synced];
	auto& rngFunc =          rngFuncs[p->synced];

	projectileContainers[p->synced].push_back(p);

	#if UNSYNCED_PROJ_NOEVENT
	if (!p->synced)
		return;
	#endif

	if (freeIDs.empty()) {
		const size_t oldSize = projMap.size();
		const size_t newSize = oldSize + 256;

		projMap.resize(newSize, nullptr);

		for (int i = oldSize; i < newSize; i++) {
			freeIDs.push_back(i);
		}

		std::random_shuffle(freeIDs.begin(), freeIDs.end(), rngFunc);
	}


	#if 0
	// popping from the back means ID's are reused more often
	projMap[p->id = spring::VectorBackPop(freeIDs)] = p;
	#else
	{
		// randomly shuffled, randomly indexed
		const unsigned int idx = rngFunc(freeIDs.size());
		const          int pid = freeIDs[idx];

		projMap[p->id = pid] = p;
		freeIDs[idx] = freeIDs.back();
		freeIDs.pop_back();
	}
	#endif

	if ((p->id) > (1 << 24))
		LOG_L(L_WARNING, "[ProjectileHandler::%s] Lua %s projectile IDs are now out of range", __func__, (p->synced? "synced": "unsynced"));

	if (!p->synced)
		return;

	ASSERT_SYNCED(freeIDs.size());
	ASSERT_SYNCED(p->id);
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
		const float3 ppos1 = p->pos + p->speed;
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

	CheckUnitFeatureCollisions(projectileContainers[ true]); // changes simulation state
	CheckUnitFeatureCollisions(projectileContainers[false]); // does not change simulation state

	CheckGroundCollisions(projectileContainers[ true]); // changes simulation state
	CheckGroundCollisions(projectileContainers[false]); // does not change simulation state
}



void CProjectileHandler::AddGroundFlash(CGroundFlash* flash)
{
	groundFlashes.push_back(flash);
}


void CProjectileHandler::AddFlyingPiece(
	const S3DModel* model,
	const S3DModelPiece* piece,
	const CMatrix44f& m,
	const float3 pos,
	const float3 speed,
	const float2 pieceParams,
	const int2 renderParams
) {
	flyingPieces[model->type].emplace_back(model, piece, m, pos, speed, pieceParams, renderParams);
	resortFlyingPieces[model->type] = true;
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
	constexpr float  udAlpha = 64.0f / 256.0f; // denom=255 is not constexpr-able

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
	constexpr float  udAlpha = 64.0f / 256.0f;

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


CProjectile* CProjectileHandler::GetProjectileBySyncedID(int id)
{
	if ((size_t)id < projectileMaps[true].size())
		return projectileMaps[true][id];

	return nullptr;
}


CProjectile* CProjectileHandler::GetProjectileByUnsyncedID(int id)
{
	if (UNSYNCED_PROJ_NOEVENT)
		return nullptr; // unsynced projectiles have no IDs if UNSYNCED_PROJ_NOEVENT

	if ((size_t)id < projectileMaps[false].size())
		return projectileMaps[false][id];

	return nullptr;
}


float CProjectileHandler::GetParticleSaturation(bool randomized) const
{
	const int curParticles = GetCurrentParticles();

	// use the random mult to weaken the max limit a little
	// so the chance is better spread when being close to the limit
	// i.e. when there are rockets that spam CEGs this gives smaller CEGs still a chance
	const float total = std::max(1.0f, maxParticles * 1.0f);
	const float fract = std::max(int(curParticles >= maxParticles), curParticles) / total;
	const float rmult = 1.0f + (int(randomized) * 0.3f * guRNG.NextFloat());

	return (fract * rmult);
}

int CProjectileHandler::GetCurrentParticles() const
{
	// use precached part of particles count calculation that else becomes very heavy
	// example where it matters: (in ZK) /cheat /give 20 armraven -> shoot ground
	int partCount = lastCurrentParticles;
	for (size_t i = lastProjectileCounts[true], e = projectileContainers[true].size(); i < e; ++i) {
		partCount += projectileContainers[true][i]->GetProjectilesCount();
	}
	for (size_t i = lastProjectileCounts[false], e = projectileContainers[false].size(); i < e; ++i) {
		partCount += projectileContainers[false][i]->GetProjectilesCount();
	}
	for (const auto& c: flyingPieces) {
		for (const auto& fp: c) {
			partCount += fp.GetDrawCallCount();
		}
	}
	partCount += groundFlashes.size();
	return partCount;
}
