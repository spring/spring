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
#include "System/TimeProfiler.h"


CR_BIND(CLosHandler, )

// ILosTypes aren't creg'ed cause they repopulate themselves in case of loading a saved game
CR_REG_METADATA(CLosHandler,(
	CR_IGNORED(autoLinkEvents),
	CR_IGNORED(autoLinkedEvents),
	CR_IGNORED(name),
	CR_IGNORED(order),
	CR_IGNORED(synced_),

	CR_IGNORED(los),
	CR_IGNORED(airLos),
	CR_IGNORED(radar),
	CR_IGNORED(sonar),
	CR_IGNORED(seismic),
	CR_IGNORED(commonJammer),
	CR_IGNORED(commonSonarJammer),
	CR_MEMBER(baseRadarErrorSize),
	CR_MEMBER(baseRadarErrorMult),
	CR_MEMBER(radarErrorSizes),
	CR_IGNORED(losTypes)
))




//////////////////////////////////////////////////////////////////////
// ILosType
//////////////////////////////////////////////////////////////////////

size_t ILosType::cacheFails = 0;
size_t ILosType::cacheHits  = 0;
size_t ILosType::cacheReactivated  = 0;
constexpr float CLosHandler::defBaseRadarErrorSize;
constexpr float CLosHandler::defBaseRadarErrorMult;



ILosType::ILosType(const int mipLevel_, LosType type_)
	: losMaps(
		(type != LOS_TYPE_JAMMER && type != LOS_TYPE_SONAR_JAMMER) ? teamHandler->ActiveAllyTeams() : 1,
		CLosMap(size, type == LOS_TYPE_LOS, readMap->GetMIPHeightMapSynced(mipLevel_)))
	, mipLevel(mipLevel_)
	, divisor(SQUARE_SIZE * (1 << mipLevel))
	, invDiv(1.0f / divisor)
	, size(std::max(1, mapDims.mapx >> mipLevel), std::max(1, mapDims.mapy >> mipLevel))
	, type(type_)
	, algoType((type == LOS_TYPE_LOS || type == LOS_TYPE_RADAR) ? LOS_ALGO_RAYCAST : LOS_ALGO_CIRCLE)
{
}


ILosType::~ILosType()
{
	if (algoType == LOS_ALGO_RAYCAST) {
		unsigned empties = 0;
		unsigned hashCount = 1; // 1 to prevent div0
		unsigned squaresCount = 1;
		for (auto& bucket: instanceHash) {
			if (bucket.empty()) {
				++empties;
				continue;
			}
			++squaresCount;
			hashCount += bucket.size();
		}

		LOG_L(L_DEBUG, "LosHandler: empties=%.0f%% avgHashCol=%0.1f", float(empties * 100) / MAGIC_PRIME, float(hashCount) / squaresCount);
	}
}


float ILosType::GetRadius(const CUnit* unit) const
{
	switch (type) {
		case LOS_TYPE_LOS:          return unit->losRadius;
		case LOS_TYPE_AIRLOS:       return unit->airLosRadius;
		case LOS_TYPE_RADAR:        return unit->radarRadius;
		case LOS_TYPE_SONAR:        return unit->sonarRadius;
		case LOS_TYPE_JAMMER:       return unit->jammerRadius;
		case LOS_TYPE_SEISMIC:      return unit->seismicRadius;
		case LOS_TYPE_SONAR_JAMMER: return unit->sonarJamRadius;
	}
	assert(false);
	return 0.f;
}


float ILosType::GetHeight(const CUnit* unit) const
{
	const float losHeight = unit->midPos.y + unit->unitDef->losHeight;
	const float iLosHeight = ((int(losHeight) >> (mipLevel + 1)) << (mipLevel + 1)) + ((1 << (mipLevel + 1)) * 0.5f); // save losHeight in buckets //FIXME Round
	return (algoType == LOS_ALGO_RAYCAST) ? iLosHeight : 0.f;
}


void ILosType::MoveUnit(CUnit* unit)
{
	// NOTE: under normal circumstances, this only gets called if a unit
	// has moved to a new map square since its last SlowUpdate cycle, so
	// any units that changed position between enabling and disabling of
	// globalLOS and *stopped moving* will still provide LOS at their old
	// square *after* it is disabled (until they start moving again)
	if (gs->globalLOS[unit->allyteam])
		return;
	if (unit->isDead || unit->transporter != nullptr)
		return;

	if (unit->beingBuilt) {
		if (unit->los[type] != nullptr) {
			RemoveUnit(unit);
		}
		return;
	}

	// NOTE:
	//   when stunned, we are not called during Unit::SlowUpdate's
	//   but units can in principle still be given on/off commands
	//   this creates an exploit via Unit::Activate if the unit is
	//   a transported radar/jammer and leaves a detached coverage
	//   zone behind
	const bool isPureSight = (type == LOS_TYPE_LOS) || (type == LOS_TYPE_AIRLOS);
	if (!isPureSight && (!unit->activated || unit->IsStunned())) {
		// deactivate any type of radar/jam when deactivated
		if (unit->los[type] != nullptr) {
			RemoveUnit(unit);
		}
		return;
	}


	const float3& losPos = unit->midPos;
	const float radius = GetRadius(unit);
	const float height = GetHeight(unit);
	const int2 baseLos = PosToSquare(losPos);
	const int allyteam = (type != LOS_TYPE_JAMMER && type != LOS_TYPE_SONAR_JAMMER) ? unit->allyteam : 0;

	if (radius <= 0)
		return;

	auto IS_FITTING_INSTANCE = [&](SLosInstance* li) -> bool {
		return (li
		    && (li->basePos    == baseLos)
		    && (li->baseHeight == height)
		    && (li->radius     == radius)
		    && (li->allyteam   == allyteam)
		);
	};

	// unchanged?
	SLosInstance* uli = unit->los[type];
	if (IS_FITTING_INSTANCE(uli)) {
		return;
	}
	UnrefInstance(uli);
	const int hash = GetHashNum(unit, baseLos);

	// Cache - search if there is already an instance with same properties
	for (SLosInstance* li: instanceHash[hash]) {
		if (IS_FITTING_INSTANCE(li)) {
			if (algoType == LOS_ALGO_RAYCAST) ++cacheHits;
			unit->los[type] = li;
			RefInstance(li);
			return;
		}
	}

	// New - create a new one
	if (algoType == LOS_ALGO_RAYCAST) ++cacheFails;
	SLosInstance* li = new SLosInstance(
		radius,
		allyteam,
		baseLos,
		height,
		hash
	);
	instanceHash[hash].push_back(li);
	unit->los[type] = li;
	LosAdd(li);
}


void ILosType::RemoveUnit(CUnit* unit, bool delayed)
{
	if (delayed) {
		DelayedFreeInstance(unit->los[type]);
	} else {
		UnrefInstance(unit->los[type]);
	}
	unit->los[type] = nullptr;
}


void ILosType::LosAdd(SLosInstance* li)
{
	assert(li);
	assert(teamHandler->IsValidAllyTeam(li->allyteam));

	if (li->radius > 0) {
		if (algoType == LOS_ALGO_RAYCAST) {
			losMaps[li->allyteam].AddRaycast(li, 1);
		} else {
			losMaps[li->allyteam].AddCircle(li, 1);
		}
	}
}


void ILosType::LosRemove(SLosInstance* li)
{
	if (li->radius > 0) {
		if (algoType == LOS_ALGO_RAYCAST) {
			losMaps[li->allyteam].AddRaycast(li, -1);
		} else {
			losMaps[li->allyteam].AddCircle(li, -1);
		}
	}
}


void ILosType::DeleteInstance(SLosInstance* li)
{
	auto& cont = instanceHash[li->hashNum];
	auto it = std::find(cont.begin(), cont.end(), li);
	cont.erase(it);
	delete li;
}


void ILosType::RefInstance(SLosInstance* instance)
{
	if (instance->refCount == 0) {
		if (algoType == LOS_ALGO_RAYCAST) ++cacheReactivated;
		LosAdd(instance);
	}
	instance->refCount++;
}


void ILosType::UnrefInstance(SLosInstance* instance)
{
	if (instance == nullptr)
		return;

	instance->refCount--;
	if (instance->refCount > 0) {
		return;
	}

	LosRemove(instance);

	if (algoType == LOS_ALGO_CIRCLE) {
		// in case of circle the instance doesn't keep any relevant buffer,
		// so there is no need to cache it
		DeleteInstance(instance);
		return;
	}

	if (!instance->toBeDeleted) {
		instance->toBeDeleted = true;
		toBeDeleted.push_back(instance);
	}

	// reached max cache size, free one instance
	if (toBeDeleted.size() > CACHE_SIZE) {
		SLosInstance* li = toBeDeleted.front();
		toBeDeleted.pop_front();

		if (li->hashNum >= MAGIC_PRIME || li->hashNum < 0) {
			LOG_L(L_WARNING,
				"[LosHandler::FreeInstance] bad LOS-instance hash (%d)",
				li->hashNum);
			return;
		}

		li->toBeDeleted = false;

		if (li->refCount == 0) {
			DeleteInstance(li);
		}
	}
}


int ILosType::GetHashNum(const CUnit* unit, const int2 baseLos)
{
	boost::uint32_t hash = 127;
	hash = HsiehHash(&unit->allyteam,  sizeof(unit->allyteam), hash);
	hash = HsiehHash(&baseLos,         sizeof(baseLos), hash);

	// hash-value range is [0, MAGIC_PRIME - 1]
	return (hash % MAGIC_PRIME);
}


void ILosType::DelayedFreeInstance(SLosInstance* instance)
{
	DelayedInstance di;
	di.instance = instance;
	di.timeoutTime = (gs->frameNum + (GAME_SPEED + (GAME_SPEED >> 1)));
	delayQue.push_back(di);
}


void ILosType::Update()
{
	// update LoS pos/radius
	for (CUnit* u: unitHandler->activeUnits) {
		MoveUnit(u);
	}

	// delayed delete
	while (!delayQue.empty() && delayQue.front().timeoutTime < gs->frameNum) {
		UnrefInstance(delayQue.front().instance);
		delayQue.pop_front();
	}

	// terraform
	if ((gs->frameNum-1) % LOS_TERRAFORM_SLOWUPDATE_RATE == 0) {
		for (auto& bucket: instanceHash) {
			for (SLosInstance* li: bucket) {
				if (!li->needsRecalc)
					continue;
				if (li->refCount == 0)
					continue;

				LosRemove(li);
				li->squares.clear();
				LosAdd(li);
				li->needsRecalc = false;
			}
		}
	}
}


void ILosType::UpdateHeightMapSynced(SRectangle rect)
{
	auto CheckOverlap = [&](SLosInstance* li, SRectangle rect) -> bool {
		int2 pos = li->basePos * divisor;
		const int radius = li->radius * divisor;

		const int hw = rect.GetWidth() * SQUARE_SIZE / 2;
		const int hh = rect.GetHeight() * SQUARE_SIZE / 2;

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
	for (auto it = toBeDeleted.begin(); it != toBeDeleted.end();) {
		SLosInstance* li = *it;
		if (li->refCount > 0 || !CheckOverlap(li, rect)) {
			++it;
			continue;
		}

		it = toBeDeleted.erase(it);
		DeleteInstance(li);
	}

	// relos used instances
	for (auto& bucket: instanceHash) {
		for (SLosInstance* li: bucket) {
			if (li->needsRecalc)
				continue;
			if (!CheckOverlap(li, rect))
				continue;

			li->needsRecalc = true;
		}
	}
}




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLosHandler* losHandler;

static const int RADAR_MIPLEVEL = 3;


CLosHandler::CLosHandler()
	: CEventClient("[CLosHandler]", 271993, true)

	, los(modInfo.losMipLevel, ILosType::LOS_TYPE_LOS)
	, airLos(modInfo.airMipLevel, ILosType::LOS_TYPE_AIRLOS)
	, radar(RADAR_MIPLEVEL, ILosType::LOS_TYPE_RADAR) //FIXME make mipLevel configurable!!!
	, sonar(RADAR_MIPLEVEL, ILosType::LOS_TYPE_SONAR)
	, seismic(RADAR_MIPLEVEL, ILosType::LOS_TYPE_SEISMIC)
	, commonJammer(RADAR_MIPLEVEL, ILosType::LOS_TYPE_JAMMER)
	, commonSonarJammer(RADAR_MIPLEVEL, ILosType::LOS_TYPE_SONAR_JAMMER)

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
	losTypes.push_back(&commonJammer);
	losTypes.push_back(&commonSonarJammer);

	eventHandler.AddClient(this);
}


CLosHandler::~CLosHandler()
{
	LOG("LosHandler stats: %.0f%% (%u of %u) instances shared with a %.0f%% cache hitrate",
		100.f * float(ILosType::cacheHits) / (ILosType::cacheHits + ILosType::cacheFails),
		unsigned(ILosType::cacheHits), unsigned(ILosType::cacheHits + ILosType::cacheFails),
		100.f * float(ILosType::cacheReactivated) / ILosType::cacheHits);
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


void CLosHandler::UnitLoaded(const CUnit* unit, const CUnit* transport)
{
	for (ILosType* lt: losTypes) {
		lt->RemoveUnit(const_cast<CUnit*>(unit));
	}
}


void CLosHandler::Update()
{
	SCOPED_TIMER("LosHandler::Update");
	for (ILosType* lt: losTypes) {
		lt->Update();
	}
}


void CLosHandler::UpdateHeightMapSynced(SRectangle rect)
{
	SCOPED_TIMER("LosHandler::Update");
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
		if (unit->isCloaked)
			return false;
	} else {
		if (unit->isCloaked)
			return false;
		if (unit->alwaysVisible)
			return true;
	}

	// isCloaked always overrides globalLOS
	if (gs->globalLOS[allyTeam])
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


bool CLosHandler::InRadar(const float3 pos, int allyTeam) const
{
	if (pos.y < 0.0f) {
		// position is underwater, only sonar can see it
		return (sonar.InSight(pos, allyTeam) && !commonSonarJammer.InSight(pos, 0));
	}

	return (radar.InSight(pos, allyTeam) && !commonJammer.InSight(pos, 0));
}


bool CLosHandler::InRadar(const CUnit* unit, int allyTeam) const
{
	if (unit->IsUnderWater()) {
		// unit is completely submerged, only sonar can see it
		if (unit->sonarStealth && !unit->beingBuilt) {
			return false;
		}

		return (sonar.InSight(unit->pos, allyTeam) && !commonSonarJammer.InSight(unit->pos, 0));
	}

	// (surface) units that are not completely submerged can potentially
	// be seen by both radar and sonar (by sonar iff the lowest point on
	// the model is still inside water)
	const bool radarVisible =
		(!unit->stealth || unit->beingBuilt) &&
		(radar.InSight(unit->pos, allyTeam)) &&
		(!commonJammer.InSight(unit->pos, 0));
	const bool sonarVisible =
		(unit->IsUnderWater()) &&
		(!unit->sonarStealth || unit->beingBuilt) &&
		(sonar.InSight(unit->pos, allyTeam)) &&
		(!commonSonarJammer.InSight(unit->pos, 0));

	return (radarVisible || sonarVisible);
}


bool CLosHandler::InJammer(const float3 pos, int allyTeam) const
{
	if (pos.y < 0.0f) {
		return commonSonarJammer.InSight(pos, 0);
	}
	return commonJammer.InSight(pos, 0);
}


bool CLosHandler::InJammer(const CUnit* unit, int allyTeam) const
{
	if (unit->IsUnderWater()) {
		return commonSonarJammer.InSight(unit->pos, 0);
	}
	return commonJammer.InSight(unit->pos, 0);
}
