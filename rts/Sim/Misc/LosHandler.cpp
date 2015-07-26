/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LosHandler.h"
#include "ModInfo.h"

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Map/ReadMap.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Sync/HsiehHash.h"
#include "System/creg/STL_Deque.h"
#include "System/EventHandler.h"


CR_BIND(LosInstance, )
CR_BIND(CLosHandler, )
CR_BIND(CLosHandler::DelayedInstance, )

CR_REG_METADATA(LosInstance,(
	CR_IGNORED(losSquares),
	CR_MEMBER(losRadius),
	CR_MEMBER(airLosRadius),
	CR_MEMBER(refCount),
	CR_MEMBER(allyteam),
	CR_MEMBER(basePos),
	CR_MEMBER(baseAirPos),
	CR_MEMBER(hashNum),
	CR_MEMBER(baseHeight),
	CR_MEMBER(toBeDeleted)
))

void CLosHandler::PostLoad()
{
	for (auto& bucket: instanceHash) {
		for (LosInstance* li: bucket) {
			if (li->refCount) {
				LosAdd(li);
			}
		}
	}
}

CR_REG_METADATA(CLosHandler,(
	CR_IGNORED(losMipLevel),
	CR_IGNORED(airMipLevel),
	CR_IGNORED(losDiv),
	CR_IGNORED(airDiv),
	CR_IGNORED(invLosDiv),
	CR_IGNORED(invAirDiv),
	CR_IGNORED(airSize),
	CR_IGNORED(losSize),
	CR_IGNORED(requireSonarUnderWater),

	CR_MEMBER(losAlgo),
	CR_MEMBER(losMaps),
	CR_MEMBER(airLosMaps),
	CR_MEMBER(instanceHash),
	CR_MEMBER(toBeDeleted),
	CR_MEMBER(delayQue),
	CR_POSTLOAD(PostLoad)
))

CR_REG_METADATA_SUB(CLosHandler,DelayedInstance, (
	CR_MEMBER(instance),
	CR_MEMBER(timeoutTime)))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CLosHandler* losHandler;


CLosHandler::CLosHandler()
	: CEventClient("[CLosHandler]", 271993, true)
	, losMaps(teamHandler->ActiveAllyTeams())
	, airLosMaps(teamHandler->ActiveAllyTeams())
	, losMipLevel(modInfo.losMipLevel)
	, airMipLevel(modInfo.airMipLevel)
	, losDiv(SQUARE_SIZE * (1 << losMipLevel))
	, airDiv(SQUARE_SIZE * (1 << airMipLevel))
	, invLosDiv(1.0f / losDiv)
	, invAirDiv(1.0f / airDiv)
	, airSize(std::max(1, mapDims.mapx >> airMipLevel), std::max(1, mapDims.mapy >> airMipLevel))
	, losSize(std::max(1, mapDims.mapx >> losMipLevel), std::max(1, mapDims.mapy >> losMipLevel))
	, requireSonarUnderWater(modInfo.requireSonarUnderWater)
	, losAlgo(losSize, -1e6f, 15, readMap->GetMIPHeightMapSynced(losMipLevel))
{
	for (int a = 0; a < teamHandler->ActiveAllyTeams(); ++a) {
		losMaps[a].SetSize(losSize.x, losSize.y, true);
		airLosMaps[a].SetSize(airSize.x, airSize.y, false);
	}
	eventHandler.AddClient(this);
}


CLosHandler::~CLosHandler()
{
	for (auto& bucket: instanceHash) {
		for (LosInstance* li: bucket) {
			delete li;
		}
		bucket.clear();
	}

}


void CLosHandler::UnitDestroyed(const CUnit* unit, const CUnit* attacker)
{
	RemoveUnit(const_cast<CUnit*>(unit), true);
}


void CLosHandler::UnitTaken(const CUnit* unit, int oldTeam, int newTeam)
{
	RemoveUnit(const_cast<CUnit*>(unit));
}


void CLosHandler::UnitLoaded(const CUnit* unit, const CUnit* transport)
{
	RemoveUnit(const_cast<CUnit*>(unit));
}


void CLosHandler::MoveUnit(CUnit* unit)
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
	if (unit->losRadius <= 0 && unit->airLosRadius <= 0)
		return;

	unit->lastLosUpdate = gs->frameNum;

	const float3& losPos = unit->midPos;
	const int allyteam = unit->allyteam;
	const float losHeight = losPos.y + unit->unitDef->losHeight;
	const float iLosHeight = ((int(losHeight) >> (losMipLevel + 1)) << (losMipLevel + 1)) + ((1 << (losMipLevel + 1)) * 0.5f); // save losHeight in buckets //FIXME Round
	const int2 baseLos = GetLosSquare(losPos);
	const int2 baseAir = GetAirSquare(losPos);

	// unchanged?
	LosInstance* uli = unit->los;
	if (uli
	    && (uli->basePos == baseLos)
	    && (uli->baseHeight == iLosHeight)
	    && (uli->losRadius == unit->losRadius)
	    && (uli->airLosRadius == unit->airLosRadius)
	) {
		return;
	}
	FreeInstance(unit->los);
	const int hash = GetHashNum(unit, baseLos, baseAir);

	// Cache - search if there is already an instance with same properties
	for (LosInstance* li: instanceHash[hash]) {
		if (li->basePos      == baseLos            &&
		    li->losRadius    == unit->losRadius    &&
		    li->airLosRadius == unit->airLosRadius &&
		    li->baseHeight   == iLosHeight         &&
		    li->allyteam     == allyteam
		) {
			AllocInstance(li);
			unit->los = li;
			return;
		}
	}

	// New - create a new one
	LosInstance* instance = new LosInstance(
		unit->losRadius,
		unit->airLosRadius,
		allyteam,
		baseLos,
		baseAir,
		hash,
		iLosHeight
	);
	instanceHash[hash].push_back(instance);
	unit->los = instance;
	LosAdd(instance);
}


void CLosHandler::RemoveUnit(CUnit* unit, bool delayed)
{
	if (delayed) {
		DelayedFreeInstance(unit->los);
	} else {
		FreeInstance(unit->los);
	}
	unit->los = nullptr;
}


void CLosHandler::LosAdd(LosInstance* li)
{
	assert(li);
	assert(teamHandler->IsValidAllyTeam(li->allyteam));

	if (li->losRadius > 0) {
		if (li->losSquares.empty())
			losAlgo.LosAdd(li->basePos, li->losRadius, li->baseHeight, li->losSquares);
		losMaps[li->allyteam].AddMapSquares(li->losSquares, li->allyteam, 1);
	}
	if (li->airLosRadius > 0) { airLosMaps[li->allyteam].AddMapArea(li->baseAirPos, li->allyteam, li->airLosRadius, 1); }
}


void CLosHandler::LosRemove(LosInstance* li)
{
	if (li->losRadius > 0) { losMaps[li->allyteam].AddMapSquares(li->losSquares, li->allyteam, -1); }
	if (li->airLosRadius > 0) { airLosMaps[li->allyteam].AddMapArea(li->baseAirPos, li->allyteam, li->airLosRadius, -1); }
}


void CLosHandler::FreeInstance(LosInstance* instance)
{
	if (instance == nullptr)
		return;

	instance->refCount--;
	if (instance->refCount > 0) {
		return;
	}

	LosRemove(instance);

	if (!instance->toBeDeleted) {
		instance->toBeDeleted = true;
		toBeDeleted.push_back(instance);
	}

	// reached max cache size, free one instance
	if (toBeDeleted.size() > 500) {
		LosInstance* li = toBeDeleted.front();
		toBeDeleted.pop_front();

		if (li->hashNum >= LOSHANDLER_MAGIC_PRIME || li->hashNum < 0) {
			LOG_L(L_WARNING,
					"[LosHandler::FreeInstance] bad LOS-instance hash (%d)",
					li->hashNum);
			return;
		}

		li->toBeDeleted = false;

		if (li->refCount == 0) {
			auto& cont = instanceHash[li->hashNum];
			auto it = std::find(cont.begin(), cont.end(), li);
			cont.erase(it);
			delete li;
		}
	}
}


void CLosHandler::AllocInstance(LosInstance* instance)
{
	if (instance->refCount == 0) {
		LosAdd(instance);
	}
	instance->refCount++;
}


int CLosHandler::GetHashNum(const CUnit* unit, const int2 baseLos, const int2 baseAirLos)
{
	boost::uint32_t hash = 127;
	hash = HsiehHash(&unit->allyteam,  sizeof(unit->allyteam), hash);
	hash = HsiehHash(&baseLos,         sizeof(baseLos), hash);
	hash = HsiehHash(&baseAirLos,      sizeof(baseAirLos), hash); //FIXME

	// hash-value range is [0, LOSHANDLER_MAGIC_PRIME - 1]
	return (hash % LOSHANDLER_MAGIC_PRIME);
}


void CLosHandler::Update()
{
	SCOPED_TIMER("LosHandler::Update");

	while (!delayQue.empty() && delayQue.front().timeoutTime < gs->frameNum) {
		FreeInstance(delayQue.front().instance);
		delayQue.pop_front();
	}

	for (CUnit* u: unitHandler->activeUnits) {
		MoveUnit(u);
	}
}


void CLosHandler::DelayedFreeInstance(LosInstance* instance)
{
	DelayedInstance di;
	di.instance = instance;
	di.timeoutTime = (gs->frameNum + (GAME_SPEED + (GAME_SPEED >> 1)));

	delayQue.push_back(di);
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

	if (requireSonarUnderWater) {
		if (unit->IsUnderWater() && !radarHandler->InRadar(unit, allyTeam)) {
			return false;
		}
	}

	return (InLos(unit->pos, allyTeam) || InLos(unit->pos + unit->speed, allyTeam));
}
