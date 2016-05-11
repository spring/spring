/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LosHandler.h"

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Map/ReadMap.h"
#include "System/Log/ILog.h"
#include "System/Sync/HsiehHash.h"
#include "System/creg/STL_Deque.h"
#include "System/EventHandler.h"
#include "System/ThreadPool.h"
#include "System/TimeProfiler.h"


CR_BIND(CLosHandler, )

// ILosTypes aren't creg'ed cause they repopulate themselves in case of loading a saved game
CR_REG_METADATA(CLosHandler,(
	CR_IGNORED(autoLinkEvents),
	CR_IGNORED(autoLinkedEvents),
	CR_IGNORED(name),
	CR_IGNORED(order),
	CR_IGNORED(synced_),

	CR_MEMBER(globalLOS),
	CR_IGNORED(los),
	CR_IGNORED(airLos),
	CR_IGNORED(radar),
	CR_IGNORED(sonar),
	CR_IGNORED(seismic),
	CR_IGNORED(jammer),
	CR_IGNORED(sonarJammer),
	CR_MEMBER(baseRadarErrorSize),
	CR_MEMBER(baseRadarErrorMult),
	CR_MEMBER(radarErrorSizes),
	CR_IGNORED(losTypes)
))



//////////////////////////////////////////////////////////////////////
// SLosInstance
//////////////////////////////////////////////////////////////////////

inline void SLosInstance::Init(int radius, int allyteam, int2 basePos, float baseHeight, int hashNum)
{
	this->allyteam = allyteam;
	this->radius = radius;
	this->basePos = basePos;
	this->baseHeight = baseHeight;
	this->refCount = 0;
	this->hashNum = hashNum;
	this->status = NONE;
	this->isCache = false;
	this->isQueuedForUpdate = false;
	this->isQueuedForTerraform = false;
}


//////////////////////////////////////////////////////////////////////
// ILosType
//////////////////////////////////////////////////////////////////////

size_t ILosType::cacheFails = 1;
size_t ILosType::cacheHits  = 1;
size_t ILosType::cacheReactivated = 1;
constexpr float CLosHandler::defBaseRadarErrorSize;
constexpr float CLosHandler::defBaseRadarErrorMult;
constexpr SLosInstance::RLE SLosInstance::EMPTY_RLE;


ILosType::ILosType(const int mipLevel_, LosType type_)
	: mipLevel(mipLevel_)
	, divisor(SQUARE_SIZE * (1 << mipLevel))
	, invDiv(1.0f / divisor)
	, size(std::max(1, mapDims.mapx >> mipLevel), std::max(1, mapDims.mapy >> mipLevel))
	, type(type_)
	, algoType((type == LOS_TYPE_LOS || type == LOS_TYPE_RADAR) ? LOS_ALGO_RAYCAST : LOS_ALGO_CIRCLE)
	, losMaps(teamHandler->ActiveAllyTeams(),
		CLosMap(size, type == LOS_TYPE_LOS, readMap->GetMIPHeightMapSynced(mipLevel_), int2(mapDims.mapx, mapDims.mapy)))
{
}


float ILosType::GetRadius(const CUnit* unit) const
{
	switch (type) {
		case LOS_TYPE_LOS:          return (unit->losRadius      / SQUARE_SIZE) >> mipLevel;
		case LOS_TYPE_AIRLOS:       return (unit->airLosRadius   / SQUARE_SIZE) >> mipLevel;
		case LOS_TYPE_RADAR:        return (unit->radarRadius    / SQUARE_SIZE) >> mipLevel;
		case LOS_TYPE_SONAR:        return (unit->sonarRadius    / SQUARE_SIZE) >> mipLevel;
		case LOS_TYPE_JAMMER:       return (unit->jammerRadius   / SQUARE_SIZE) >> mipLevel;
		case LOS_TYPE_SEISMIC:      return (unit->seismicRadius  / SQUARE_SIZE) >> mipLevel;
		case LOS_TYPE_SONAR_JAMMER: return (unit->sonarJamRadius / SQUARE_SIZE) >> mipLevel;
		case LOS_TYPE_COUNT:        break; //make the compiler happy
	}
	assert(false);
	return 0.f;
}


float ILosType::GetHeight(const CUnit* unit) const
{
	if (algoType == LOS_ALGO_CIRCLE) {
		return 0.f;
	}
	const float emitHeight = (type == LOS_TYPE_LOS || type == LOS_TYPE_AIRLOS) ? unit->unitDef->losHeight : unit->unitDef->radarHeight;
	const float losHeight  = unit->midPos.y + emitHeight;
	const int bucketSize   = 1 << (mipLevel + 2);
	const float iLosHeight = (int(losHeight) / bucketSize + 0.5f) * bucketSize; // save losHeight in buckets
	return iLosHeight;
}


inline void ILosType::UpdateUnit(CUnit* unit)
{
	// NOTE: under normal circumstances, this only gets called if a unit
	// has moved to a new map square since its last SlowUpdate cycle, so
	// any units that changed position between enabling and disabling of
	// globalLOS and *stopped moving* will still provide LOS at their old
	// square *after* it is disabled (until they start moving again)
	if (losHandler->globalLOS[unit->allyteam])
		return;
	if (unit->isDead || unit->beingBuilt || unit->transporter != nullptr)
		return;

	// NOTE:
	//   when stunned, we are not called during Unit::SlowUpdate's
	//   but units can in principle still be given on/off commands
	//   this creates an exploit via Unit::Activate if the unit is
	//   a transported radar/jammer and leaves a detached coverage
	//   zone behind
	const bool isPureSight = (type == LOS_TYPE_LOS) || (type == LOS_TYPE_AIRLOS);
	if (!isPureSight && (!unit->activated || unit->IsStunned())) {
		// deactivate any type of radar/jam when deactivated
		RemoveUnit(unit);
		return;
	}

	SLosInstance* uli = unit->los[type];
	const float3 losPos = unit->midPos;
	const float radius = GetRadius(unit);
	const float height = GetHeight(unit);
	const int2 baseLos = PosToSquare(losPos);
	      int allyteam = unit->allyteam;

	// jammers share all the same map independent of the allyTeam
	if (type == LOS_TYPE_JAMMER || type == LOS_TYPE_SONAR_JAMMER)
		if (!modInfo.separateJammers)
			allyteam = 0;

	if (radius <= 0) {
		if (uli) {
			unit->los[type] = nullptr;
			UnrefInstance(uli);
		}
		return;
	}

	auto IS_FITTING_INSTANCE = [&](SLosInstance* li) -> bool {
		return (li
		    && (li->basePos    == baseLos)
		    && (li->baseHeight == height)
		    && (li->radius     == radius)
		    && (li->allyteam   == allyteam)
		);
	};

	// unchanged?
	if (IS_FITTING_INSTANCE(uli)) {
		return;
	}
	if (uli) {
		unit->los[type] = nullptr;
		UnrefInstance(uli);
	}
	const int hash = GetHashNum(unit->allyteam, baseLos, radius);

	// Cache - search if there is already an instance with same properties
	auto pair = instanceHash.equal_range(hash);
	for (auto it = pair.first; it != pair.second; ++it) {
		SLosInstance* li = it->second;
		if (IS_FITTING_INSTANCE(li)) {
			if (algoType == LOS_ALGO_RAYCAST) ++cacheHits;
			unit->los[type] = li;
			RefInstance(li);
			return;
		}
	}

	// New - create a new one
	if (algoType == LOS_ALGO_RAYCAST) ++cacheFails;
	SLosInstance* li = CreateInstance();
	li->Init(radius, allyteam, baseLos, height, hash);
	li->refCount++;
	unit->los[type] = li;
	instanceHash.emplace(hash, li);
	UpdateInstanceStatus(li, SLosInstance::TLosStatus::NEW);
}


inline void ILosType::RemoveUnit(CUnit* unit, bool delayed)
{
	if (unit->los[type] == nullptr)
		return;

	if (delayed) {
		DelayedUnrefInstance(unit->los[type]);
	} else {
		UnrefInstance(unit->los[type]);
	}
	unit->los[type] = nullptr;
}


inline void ILosType::LosAdd(SLosInstance* li)
{
	assert(li);
	assert(teamHandler->IsValidAllyTeam(li->allyteam));

	if (algoType == LOS_ALGO_RAYCAST) {
		losMaps[li->allyteam].AddRaycast(li, 1);
	} else {
		losMaps[li->allyteam].AddCircle(li, 1);
	}
}


inline void ILosType::LosRemove(SLosInstance* li)
{
	if (algoType == LOS_ALGO_RAYCAST) {
		losMaps[li->allyteam].AddRaycast(li, -1);
	} else {
		losMaps[li->allyteam].AddCircle(li, -1);
	}
}


inline void ILosType::RefInstance(SLosInstance* li)
{
	li->refCount++;
	if (li->refCount == 1) {
		if (li->isCache) {
			if (algoType == LOS_ALGO_RAYCAST) ++cacheReactivated;
			auto it = std::find(losCache.begin(), losCache.end(), li);
			li->isCache = false;
			losCache.erase(it);
		}

		UpdateInstanceStatus(li, SLosInstance::TLosStatus::REACTIVATE);
	}
}


void ILosType::UnrefInstance(SLosInstance* li)
{
	assert(li->refCount > 0);
	li->refCount--;
	if (li->refCount > 0) {
		return;
	}

	UpdateInstanceStatus(li, SLosInstance::TLosStatus::REMOVE);
}


inline void ILosType::DelayedUnrefInstance(SLosInstance* li)
{
	DelayedInstance di;
	di.instance = li;
	di.timeoutTime = (gs->frameNum + (GAME_SPEED + (GAME_SPEED >> 1)));
	delayedDeleteQue.push_back(di);
}


inline void ILosType::AddInstanceToCache(SLosInstance* li)
{
	if (li->status & SLosInstance::TLosStatus::RECALC) {
		assert(!li->isCache);
		DeleteInstance(li);
		return;
	}

	li->isCache = true;
	losCache.push_back(li);
}


inline SLosInstance* ILosType::CreateInstance()
{
	if (!freeIDs.empty()) {
		int id = freeIDs.back();
		freeIDs.pop_back();
		return &instances[id];
	}

	instances.emplace_back(instances.size());
	return &instances.back();
}


inline void ILosType::DeleteInstance(SLosInstance* li)
{
	assert(li->refCount == 0);
	auto pair = instanceHash.equal_range(li->hashNum);
	auto it = std::find_if(pair.first, pair.second, [&](decltype(*pair.first)& p){ return (p.second == li); });
	instanceHash.erase(it);

	// caller has to do that
	assert(!li->isCache);
	/*if (li->isCache) {
		auto it = std::find(losCache.begin(), losCache.end(), li);
		losCache.erase(it);
	}*/

	if (li->isQueuedForTerraform) {
		auto it = std::find_if(delayedTerraQue.begin(), delayedTerraQue.end(), [&](const DelayedInstance& inst) {
			return inst.instance == li;
		});
		if (it != delayedTerraQue.end())
			delayedTerraQue.erase(it);
		li->isQueuedForTerraform = false;
	}

	li->squares.clear();
	freeIDs.push_back(li->id);
}


inline void ILosType::UpdateInstanceStatus(SLosInstance* li, SLosInstance::TLosStatus status)
{
	// queue for update
	if (status == SLosInstance::TLosStatus::RECALC) {
		if (!li->isQueuedForTerraform && !li->isQueuedForUpdate) {
			li->isQueuedForTerraform = true;

			DelayedInstance di;
			di.instance = li;
			di.timeoutTime = (gs->frameNum + 2 * GAME_SPEED);
			delayedTerraQue.push_back(di);
		}
	} else {
		if (!li->isQueuedForUpdate) {
			li->isQueuedForUpdate = true;
			losUpdate.push_back(li);
		}
	}


	// mark the type of update needed
	assert((li->status & status) == 0 || status == SLosInstance::TLosStatus::RECALC);
	li->status |= status;


	// sanity checks (debug only)
	constexpr auto b = SLosInstance::TLosStatus::REACTIVATE | SLosInstance::TLosStatus::RECALC;
	assert((li->status & b) != b || (li->status & SLosInstance::TLosStatus::REMOVE));

	constexpr auto c = SLosInstance::TLosStatus::NEW | SLosInstance::TLosStatus::RECALC;
	assert((li->status & c) != c);

	constexpr auto d = SLosInstance::TLosStatus::NEW | SLosInstance::TLosStatus::REACTIVATE;
	assert((li->status & d) != d);

	constexpr auto e = SLosInstance::TLosStatus::NEW | SLosInstance::TLosStatus::REMOVE;
	assert((li->status & e) != e);

	if (li->status & SLosInstance::TLosStatus::RECALC)
		assert(li->refCount > 0 || (li->status & SLosInstance::TLosStatus::REMOVE));

	if (li->refCount == 0)
		assert(li->isCache || (li->status & SLosInstance::TLosStatus::REMOVE));

	if (status == SLosInstance::TLosStatus::REMOVE)
		assert(li->refCount == 0);
}


inline SLosInstance::TLosStatus ILosType::OptimizeInstanceUpdate(SLosInstance* li)
{
	constexpr auto a = SLosInstance::TLosStatus::REACTIVATE | SLosInstance::TLosStatus::REMOVE;
	if ((li->status & a) == a) {
		assert(li->refCount > 0);
		li->status &= ~a;
	}


	if (li->status & SLosInstance::TLosStatus::NEW) {
		assert(li->refCount > 0);
		return SLosInstance::TLosStatus::NEW;
	} else
	if (li->status & SLosInstance::TLosStatus::REMOVE) {
		assert(li->refCount == 0);
		return SLosInstance::TLosStatus::REMOVE;
	} else
	if (li->status & SLosInstance::TLosStatus::REACTIVATE) {
		assert(li->refCount > 0);
		return SLosInstance::TLosStatus::REACTIVATE;
	} else
	if (li->status & SLosInstance::TLosStatus::RECALC) {
		assert(li->refCount > 0);
		return SLosInstance::TLosStatus::RECALC;
	}
	return SLosInstance::TLosStatus::NONE;
}


inline int ILosType::GetHashNum(const int allyteam, const int2 baseLos, const float radius) const
{
	boost::uint32_t hash = 0;
	hash = HsiehHash(&allyteam, sizeof(allyteam), hash);
	hash = HsiehHash(&baseLos,  sizeof(baseLos),  hash);
	hash = HsiehHash(&radius,   sizeof(radius),   hash);
	return hash;
}


void ILosType::Update()
{
	// delayed delete
	while (!delayedDeleteQue.empty() && delayedDeleteQue.front().timeoutTime < gs->frameNum) {
		UnrefInstance(delayedDeleteQue.front().instance);
		delayedDeleteQue.pop_front();
	}

	// relos after terraform is delayed
	while (!delayedTerraQue.empty() && delayedTerraQue.front().timeoutTime < gs->frameNum) {
		SLosInstance* li = delayedTerraQue.front().instance;
		li->isQueuedForTerraform = false;
		if (!li->isQueuedForUpdate) {
			losUpdate.push_back(li);
		}
		delayedTerraQue.pop_front();
	}

	// no updates? -> early exit
	if (losUpdate.empty())
		return;

	std::vector<SLosInstance*> losRemove;
	std::vector<SLosInstance*> losRecalc;
	std::vector<SLosInstance*> losAdd;
	std::vector<SLosInstance*> losDeleted;
	losRemove.reserve(losUpdate.size());
	if (algoType == LOS_ALGO_RAYCAST) losRecalc.reserve(losUpdate.size());
	losAdd.reserve(losUpdate.size());
	losDeleted.reserve(losUpdate.size());

	// filter the updates into their subparts
	for (SLosInstance* li: losUpdate) {
		const auto status = OptimizeInstanceUpdate(li);
		li->isQueuedForUpdate = false;

		switch (status) {
			case SLosInstance::TLosStatus::NEW: {
				if (algoType == LOS_ALGO_RAYCAST) losRecalc.push_back(li);
				losAdd.push_back(li);
			} break;
			case SLosInstance::TLosStatus::REACTIVATE: {
				losAdd.push_back(li);
			} break;
			case SLosInstance::TLosStatus::RECALC: {
				losRemove.push_back(li);
				if (algoType == LOS_ALGO_RAYCAST) losRecalc.push_back(li);
				losAdd.push_back(li);
			} break;
			case SLosInstance::TLosStatus::REMOVE: {
				losRemove.push_back(li);
				losDeleted.push_back(li);
			} break;
			case SLosInstance::TLosStatus::NONE: {
			} break;
			default: assert(false);
		}

		if (status == SLosInstance::TLosStatus::REMOVE) {
			// clear all bits except recalc
			// so the instance gets deleted right away in AddInstanceToCache()
			li->status &= SLosInstance::TLosStatus::RECALC;
		} else {
			li->status = SLosInstance::TLosStatus::NONE;
		}
	}

	// remove sight
	//FIXME multithread?
	for (SLosInstance* li: losRemove) {
		LosRemove(li);
	}

	// raycast terrain
	if (algoType == LOS_ALGO_RAYCAST)  {
		for_mt(0, losRecalc.size(), [&](const int idx) {
			auto li = losRecalc[idx];
			assert(li->refCount > 0);
			li->squares.clear();
			losMaps[li->allyteam].PrepareRaycast(li);
		});
	}

	// add sight
	for (SLosInstance* li: losAdd) {
		assert(li->refCount > 0);
		LosAdd(li);
	}

	// delete / move to cache unused instances
	if (algoType == LOS_ALGO_RAYCAST) {
		while (!losCache.empty() && ((losCache.size() + losDeleted.size()) > CACHE_SIZE)) {
			SLosInstance* li = losCache.front();
			losCache.pop_front();
			li->isCache = false;
			DeleteInstance(li);
		}

		for (SLosInstance* li: losDeleted) {
			assert(li->refCount == 0);
			AddInstanceToCache(li);
		}
	} else {
		assert(losCache.empty());
		for (SLosInstance* li: losDeleted) {
			DeleteInstance(li);
		}
	}

	losUpdate.clear();
}


void ILosType::UpdateHeightMapSynced(SRectangle rect)
{
	if (algoType == LOS_ALGO_CIRCLE)
		return;

	auto CheckOverlap = [&](SLosInstance* li, SRectangle rect) -> bool {
		int2 pos = li->basePos * divisor;
		const int radius = li->radius * divisor;

		const int hw = rect.GetWidth() * (SQUARE_SIZE / 2);
		const int hh = rect.GetHeight() * (SQUARE_SIZE / 2);

		int2 circleDistance;
		circleDistance.x = std::abs(pos.x - rect.x1 * SQUARE_SIZE) - hw;
		circleDistance.y = std::abs(pos.y - rect.y1 * SQUARE_SIZE) - hh;

		if (circleDistance.x > radius) { return false; }
		if (circleDistance.y > radius) { return false; }
		if (circleDistance.x <= 0) { return true; }
		if (circleDistance.y <= 0) { return true; }

		return (Square(circleDistance.x) + Square(circleDistance.y)) <= Square(radius);
	};

	// delete unused instances that overlap with the changed rectangle
	for (auto it = losCache.begin(); it != losCache.end();) {
		SLosInstance* li = *it;
		if (li->refCount > 0 || !CheckOverlap(li, rect)) {
			++it;
			continue;
		}

		it = losCache.erase(it);
		li->isCache = false;
		DeleteInstance(li);
	}

	// relos used instances
	for (auto& p: instanceHash) {
		SLosInstance* li = p.second;

		if (li->status & SLosInstance::TLosStatus::RECALC)
			continue;
		if (!CheckOverlap(li, rect))
			continue;

		UpdateInstanceStatus(li, SLosInstance::TLosStatus::RECALC);
	}
}




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLosHandler* losHandler = nullptr;

CLosHandler::CLosHandler()
	: CEventClient("[CLosHandler]", 271993, true)

	, globalLOS{false}
	, los(modInfo.losMipLevel, ILosType::LOS_TYPE_LOS)
	, airLos(modInfo.airMipLevel, ILosType::LOS_TYPE_AIRLOS)
	, radar(modInfo.radarMipLevel, ILosType::LOS_TYPE_RADAR)
	, sonar(modInfo.radarMipLevel, ILosType::LOS_TYPE_SONAR)
	, seismic(modInfo.radarMipLevel, ILosType::LOS_TYPE_SEISMIC)
	, jammer(modInfo.radarMipLevel, ILosType::LOS_TYPE_JAMMER)
	, sonarJammer(modInfo.radarMipLevel, ILosType::LOS_TYPE_SONAR_JAMMER)

	, baseRadarErrorSize(defBaseRadarErrorSize)
	, baseRadarErrorMult(defBaseRadarErrorMult)
	, radarErrorSizes(teamHandler->ActiveAllyTeams(), defBaseRadarErrorSize)
{
	losTypes.reserve(ILosType::LOS_TYPE_COUNT);
	losTypes.push_back(&los);
	losTypes.push_back(&airLos);
	losTypes.push_back(&radar);
	losTypes.push_back(&sonar);
	losTypes.push_back(&seismic);
	losTypes.push_back(&jammer);
	losTypes.push_back(&sonarJammer);

	eventHandler.AddClient(this);
}


CLosHandler::~CLosHandler()
{
	/*size_t memUsage = 0;
	for (ILosType* lt: losTypes) {
		memUsage += lt->instances.size() * sizeof(SLosInstance);
		for (SLosInstance& li: lt->instances) {
			memUsage += li.squares.capacity() * sizeof(SLosInstance::RLE);
		}
		memUsage += lt->losMaps.size() * sizeof(CLosMap);
		for (CLosMap& lm: lt->losMaps) {
			memUsage += lm.losmap.capacity() * sizeof(unsigned short);
		}
	}
	LOG_L(L_WARNING, "LosHandler MemUsage: ~%.1fMB", memUsage / (1024.f * 1024.f));*/

	LOG("LosHandler stats: total instances=%u; shared=%.0f%%; from cache=%.0f%%",
		unsigned(ILosType::cacheHits + ILosType::cacheFails),
		100.f * float(ILosType::cacheHits - ILosType::cacheReactivated) / (ILosType::cacheHits + ILosType::cacheFails),
		100.f * float(ILosType::cacheReactivated) / (ILosType::cacheHits + ILosType::cacheFails));
}


void CLosHandler::UnitDestroyed(const CUnit* unit, const CUnit* attacker)
{
	for (ILosType* lt: losTypes) {
		lt->RemoveUnit(const_cast<CUnit*>(unit), true);
	}
}


void CLosHandler::UnitTaken(const CUnit* unit, int oldTeam, int newTeam)
{
	for (ILosType* lt: losTypes) {
		lt->RemoveUnit(const_cast<CUnit*>(unit));
	}
}


void CLosHandler::UnitReverseBuilt(const CUnit* unit)
{
	for (ILosType* lt: losTypes) {
		lt->RemoveUnit(const_cast<CUnit*>(unit));
	}
}


void CLosHandler::UnitLoaded(const CUnit* unit, const CUnit* transport)
{
	for (ILosType* lt: losTypes) {
		lt->RemoveUnit(const_cast<CUnit*>(unit));
	}
}


void CLosHandler::Update()
{
	SCOPED_TIMER("LosHandler::Update");

	for_mt(0, losTypes.size(), [&](const int idx){
		ILosType* lt = losTypes[idx];

		for (CUnit* u: unitHandler->activeUnits) {
			lt->UpdateUnit(u);
		}

		lt->Update();
	});
}


void CLosHandler::UpdateHeightMapSynced(SRectangle rect)
{
	SCOPED_TIMER("LosHandler::UpdateHeightMapSynced");
	for (ILosType* lt: losTypes) {
		lt->UpdateHeightMapSynced(rect);
	}
}


bool CLosHandler::InLos(const CUnit* unit, int allyTeam) const
{
	// NOTE: units are treated differently than world objects in two ways:
	//   1. they can be cloaked (has to be checked BEFORE all other cases)
	//   2. when underwater, they are only considered to be in LOS if they
	//      are also in radar ("sonar") coverage if requireSonarUnderWater
	//      is enabled --> underwater units can NOT BE SEEN AT ALL without
	//      active radar!
	if (modInfo.alwaysVisibleOverridesCloaked) {
		if (unit->alwaysVisible)
			return true;
		if (unit->isCloaked && unit->allyteam != allyTeam)
			return false;
	} else {
		if (unit->isCloaked && unit->allyteam != allyTeam)
			return false;
		if (unit->alwaysVisible)
			return true;
	}

	// isCloaked always overrides globalLOS
	if (globalLOS[allyTeam])
		return true;
	if (unit->useAirLos)
		return (InAirLos(unit->pos, allyTeam) || InAirLos(unit->pos + unit->speed, allyTeam));

	if (modInfo.requireSonarUnderWater) {
		if (unit->IsUnderWater() && !InRadar(unit, allyTeam)) {
			return false;
		}
	}

	return (InLos(unit->pos, allyTeam) || InLos(unit->pos + unit->speed, allyTeam));
}


bool CLosHandler::InAirLos(const CUnit* unit, int allyTeam) const
{
	// NOTE: units are treated differently than world objects in two ways:
	//   1. they can be cloaked (has to be checked BEFORE all other cases)
	//   2. when underwater, they are only considered to be in LOS if they
	//      are also in radar ("sonar") coverage if requireSonarUnderWater
	//      is enabled --> underwater units can NOT BE SEEN AT ALL without
	//      active radar!
	if (modInfo.alwaysVisibleOverridesCloaked) {
		if (unit->alwaysVisible)
			return true;
		if (unit->isCloaked && unit->allyteam != allyTeam)
			return false;
	} else {
		if (unit->isCloaked && unit->allyteam != allyTeam)
			return false;
		if (unit->alwaysVisible)
			return true;
	}

	// isCloaked always overrides globalLOS
	if (globalLOS[allyTeam])
		return true;

	if (modInfo.requireSonarUnderWater) {
		if (unit->IsUnderWater() && !InRadar(unit, allyTeam)) {
			return false;
		}
	}

	return airLos.InSight(unit->pos, allyTeam);
}


bool CLosHandler::InRadar(const float3 pos, int allyTeam) const
{
	if (pos.y < 0.0f) {
		// position is underwater, only sonar can see it
		// note: only check jammers when we have a common jammer map, else jammers only apply to objects!
		return (sonar.InSight(pos, allyTeam) && !(!modInfo.separateJammers && sonarJammer.InSight(pos, 0)));
	}

	return (radar.InSight(pos, allyTeam) && !(!modInfo.separateJammers && jammer.InSight(pos, 0)));
}


bool CLosHandler::InRadar(const CUnit* unit, int allyTeam) const
{
	if (unit->IsUnderWater()) {
		// unit is completely submerged, only sonar can see it
		if (unit->sonarStealth && !unit->beingBuilt)
			return false;

		return (sonar.InSight(unit->pos, allyTeam) && !InJammer(unit, allyTeam));
	}

	// radar stealth
	if (unit->stealth && !unit->beingBuilt)
		return false;

	return (radar.InSight(unit->pos, allyTeam) && !InJammer(unit, allyTeam));
}


bool CLosHandler::InJammer(const float3 pos, int allyTeam) const
{
	const int jammerAlly = modInfo.separateJammers ? allyTeam : 0;

	if (pos.y < 0.0f) {
		return sonarJammer.InSight(pos, jammerAlly);
	}
	return jammer.InSight(pos, jammerAlly);
}


bool CLosHandler::InJammer(const CUnit* unit, int allyTeam) const
{
	if (allyTeam == unit->allyteam) {
		return false;
	}

	//TODO handle ingame alliances

	const int jammerAlly = modInfo.separateJammers ? unit->allyteam : 0;

	if (unit->IsUnderWater()) {
		return sonarJammer.InSight(unit->pos, jammerAlly);
	}
	return jammer.InSight(unit->pos, jammerAlly);
}
