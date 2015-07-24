/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOS_HANDLER_H
#define LOS_HANDLER_H

#include <vector>
#include <deque>
#include <boost/noncopyable.hpp>
#include "Map/Ground.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/RadarHandler.h"
#include "System/type2.h"


/**
 * CLosHandler specific data attached to each unit.
 *
 * The main goal of this object is to store the squares on the LOS map that
 * have been incremented (CLosHandler::LosAdd) when the unit last moved.
 * (CLosHandler::MoveUnit)
 *
 * These squares must be remembered because 1) ray-casting against the terrain
 * is not particularly fast and more importantly 2) the terrain may have changed
 * between the LosAdd and the moment we want to undo the LosAdd.
 *
 * LosInstances may be shared between multiple units. Reference counting is
 * used to track how many units currently use one instance.
 *
 * An instance will be shared iff the other unit is in the same square
 * (basePos, baseSquare) on the LOS map, has the same LOS and air LOS
 * radius, is in the same ally-team and has the same base-height.
 */
struct LosInstance : public boost::noncopyable
{
private:
	CR_DECLARE_STRUCT(LosInstance)

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
 * Handles line of sight (LOS) updates for all units and all ally-teams.
 *
 * Units have both LOS and air LOS. The former is ray-casted against the terrain
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
 * of 0 are not immediately deleted, but up to 500 of those are stored, in case
 * they can be reused for a future unit.
 *
 * LOS is not removed immediately when a unit gets killed. Instead,
 * DelayedFreeInstance is called. This keeps the LosInstance (including the
 * actual sight) alive until 1.5 game seconds after the unit got killed.
 */
class CLosHandler : public boost::noncopyable
{
	CR_DECLARE_STRUCT(CLosHandler)
	CR_DECLARE_SUB(DelayedInstance)

public:
	void MoveUnit(CUnit* unit, bool redoCurrent);
	void FreeInstance(LosInstance* instance);
public:
	int2 GetLosSquare(const float3 pos) const { return int2( Round(pos.x * invLosDiv), Round(pos.z * invLosDiv) ); }
	int2 GetAirSquare(const float3 pos) const { return int2( Round(pos.x * invAirDiv), Round(pos.z * invAirDiv) ); }

	bool InLos(const CUnit* unit, int allyTeam) const;

	inline bool InLos(const CWorldObject* obj, int allyTeam) const {
		if (obj->alwaysVisible || gs->globalLOS[allyTeam])
			return true;
		if (obj->useAirLos)
			return (InAirLos(obj->pos, allyTeam) || InAirLos(obj->pos + obj->speed, allyTeam));

		// test visibility at two positions, mostly for long beam-projectiles
		//   slow-moving objects will be visible no earlier or later than before on average
		//   fast-moving objects will be visible at most one SU before they otherwise would
		return (InLos(obj->pos, allyTeam) || InLos(obj->pos + obj->speed, allyTeam));
	}

	inline bool InLos(const float3 pos, int allyTeam) const {
		if (gs->globalLOS[allyTeam])
			return true;
		return (losMaps[allyTeam].At(GetLosSquare(pos)) != 0);
	}

	inline bool InAirLos(const float3 pos, int allyTeam) const {
		if (gs->globalLOS[allyTeam])
			return true;
		return (airLosMaps[allyTeam].At(GetAirSquare(pos)) != 0);
	}


	CLosHandler();
	~CLosHandler();

	std::vector<CLosMap> losMaps;
	std::vector<CLosMap> airLosMaps;

	const int losMipLevel;
	const int airMipLevel;
	const int losDiv;
	const int airDiv;
	const float invLosDiv;
	const float invAirDiv;
	const int2 airSize;
	const int2 losSize;

	const bool requireSonarUnderWater;

private:
	static const unsigned int LOSHANDLER_MAGIC_PRIME = 2309;

	void PostLoad();
	void LosAdd(LosInstance* instance);
	int GetHashNum(CUnit* unit);
	void AllocInstance(LosInstance* instance);
	void CleanupInstance(LosInstance* instance);

	CLosAlgorithm losAlgo;

	std::deque<LosInstance*> instanceHash[LOSHANDLER_MAGIC_PRIME];

	std::deque<LosInstance*> toBeDeleted;

	struct DelayedInstance {
		CR_DECLARE_STRUCT(DelayedInstance)
		LosInstance* instance;
		int timeoutTime;
	};

	std::deque<DelayedInstance> delayQue;

public:
	void Update();
	void DelayedFreeInstance(LosInstance* instance);
};

extern CLosHandler* losHandler;

#endif /* LOS_HANDLER_H */
