/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOS_HANDLER_H
#define LOS_HANDLER_H

#include <vector>
#include <list>
#include <deque>
#include <boost/noncopyable.hpp>
#include "Map/Ground.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/RadarHandler.h"
#include "System/MemPool.h"
#include "System/Vec2.h"
#include <assert.h>

/**
 * CLosHandler specific data attached to each unit.
 *
 * The main goal of this object is to store the squares on the LOS map that
 * have been incremented (CLosHandler::LosAdd) when the unit last moved.
 * (CLosHandler::MoveUnit)
 *
 * These squares must be remembered because 1) raycasting against the terrain
 * is not particularly fast and more imporantly 2) the terrain may have changed
 * between the LosAdd and the moment we want to undo the LosAdd.
 *
 * LosInstances may be shared between multiple units. Reference counting is
 * used to track how many units currently use one instance.
 *
 * An instance will be shared iff the other unit is in the same square
 * (basePos, baseSquare) on the LOS map, has the same LOS and air LOS
 * radius, is in the same allyteam and has the same baseheight.
 */
struct LosInstance : public boost::noncopyable
{
private:
	CR_DECLARE_STRUCT(LosInstance);

	/// default constructor for creg
	LosInstance()
		: losSize(0)
		, airLosSize(0)
		, refCount(0)
		, allyteam(-1)
		, baseSquare(0)
		, hashNum(-1)
		, baseHeight(0.0f)
		, toBeDeleted(false)
	{}

public:
	LosInstance(int lossize, int airLosSize, int allyteam, int2 basePos,
			int baseSquare, int2 baseAirPos, int hashNum, float baseHeight)
		: losSize(lossize)
		, airLosSize(airLosSize)
		, refCount(1)
		, allyteam(allyteam)
		, basePos(basePos)
		, baseSquare(baseSquare)
		, baseAirPos(baseAirPos)
		, hashNum(hashNum)
		, baseHeight(baseHeight)
		, toBeDeleted(false)
	{}

 	std::vector<int> losSquares;
	int losSize;
	int airLosSize;
	int refCount;
	int allyteam;
	int2 basePos;
	int baseSquare;
	int2 baseAirPos;
	int hashNum;
	float baseHeight;
	bool toBeDeleted;
};

/**
 * Handles line of sight (LOS) updates for all units and all allyteams.
 *
 * Units have both LOS and air LOS. The former is raycasted against the terrain
 * so hills obstruct view (see CLosAlgorithm). The second is circular: air LOS
 * is not influenced by terrain.
 *
 * LOS is implemented using CLosMap, which is a 2d array essentially containing
 * a reference count. That is to say, each position on the map counts how many
 * LosInstances "can see" that square. Units may share their "presence" on
 * the LOS map through sharing a single LosInstance.
 *
 * To quickly find LosInstances that can be shared CLosHandler implements a
 * hash table (instanceHash). Additionally, LosInstances that reach a refCount
 * of 0 aren't immediately deleted, but up to 500 of those are stored, in case
 * they can be reused for a future unit.
 *
 * LOS isn't removed immediately when a unit gets killed. Instead,
 * DelayedFreeInstance is called. This keeps the LosInstance (including the
 * actual sight) alive until 1.5 game seconds after the unit got killed.
 */
class CLosHandler : public boost::noncopyable
{
	CR_DECLARE(CLosHandler);
//	CR_DECLARE_SUB(CPoint);
	CR_DECLARE_SUB(DelayedInstance);

public:
	void MoveUnit(CUnit* unit, bool redoCurrent);
	void FreeInstance(LosInstance* instance);

	inline bool InLos(const CWorldObject* object, int allyTeam) {
		if (object->alwaysVisible || gs->globalLOS) {
			return true;
		} else if (object->useAirLos) {
			return (InAirLos(object->pos, allyTeam));
		} else {
			return (InLos(object->pos, allyTeam));
		}
	}

	inline bool InLos(const CUnit* unit, int allyTeam) {
		// NOTE: units are treated differently than world objects in 2 ways:
		//       1. they can be cloaked
		//       2. when underwater, they only get LOS if they also have sonar
		//          (when the requireSonarUnderWater variable is enabled)
		if (unit->alwaysVisible || gs->globalLOS) {
			return true;
		} else if (unit->isCloaked) {
			return false;
		} else if (unit->useAirLos) {
			return (InAirLos(unit->pos, allyTeam));
		} else {
			if (unit->isUnderWater && requireSonarUnderWater &&
			    !radarhandler->InRadar(unit, allyTeam)) {
				return false;
			}
			return (InLos(unit->pos, allyTeam));
		}
	}

	inline bool InLos(const float3& pos, int allyTeam) {
		if (gs->globalLOS) { return true; }
		const int gx = int(pos.x * invLosDiv);
		const int gz = int(pos.z * invLosDiv);
		return !!losMap[allyTeam].At(gx, gz);
	}

	inline bool InAirLos(const float3& pos, int allyTeam) {
		if (gs->globalLOS) { return true; }
		const int gx = int(pos.x * invAirDiv);
		const int gz = int(pos.z * invAirDiv);
		return !!airLosMap[allyTeam].At(gx, gz);
	}

	CLosHandler();
	~CLosHandler();

	std::vector<CLosMap> losMap;
	std::vector<CLosMap> airLosMap;

	const int losMipLevel;
	const int airMipLevel;
	const int losDiv;
	const int airDiv;
	const float invLosDiv;
	const float invAirDiv;
	const int airSizeX;
	const int airSizeY;
	const int losSizeX;
	const int losSizeY;

	const bool requireSonarUnderWater;

private:
	static const unsigned int LOSHANDLER_MAGIC_PRIME = 2309;

	void PostLoad();
	void LosAdd(LosInstance* instance);
	int GetHashNum(CUnit* unit);
	void AllocInstance(LosInstance* instance);
	void CleanupInstance(LosInstance* instance);

	CLosAlgorithm losAlgo;

	std::list<LosInstance*> instanceHash[LOSHANDLER_MAGIC_PRIME];

	std::deque<LosInstance*> toBeDeleted;

	struct DelayedInstance {
		CR_DECLARE_STRUCT(DelayedInstance);
		LosInstance* instance;
		int timeoutTime;
	};

	std::deque<DelayedInstance> delayQue;

public:
	void Update();
	void DelayedFreeInstance(LosInstance* instance);
};

extern CLosHandler* loshandler;

#endif /* LOS_HANDLER_H */
