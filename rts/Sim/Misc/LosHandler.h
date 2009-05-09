#ifndef LOSHANDLER_H
#define LOSHANDLER_H
// LosHandler.h: interface for the CLosHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>
#include <list>
#include <deque>
#include <boost/noncopyable.hpp>
#include "MemPool.h"
#include "Map/Ground.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/RadarHandler.h"
#include <assert.h>


struct LosInstance : public boost::noncopyable
{
	CR_DECLARE_STRUCT(LosInstance);
 	std::vector<int> losSquares;
	LosInstance() {} // default constructor for creg
	LosInstance(int lossize, int airLosSize, int allyteam, int2 basePos,
	            int baseSquare, int2 baseAirPos, int hashNum, float baseHeight)
		: losSize(lossize),
			airLosSize(airLosSize),
			refCount(1),
			allyteam(allyteam),
			basePos(basePos),
			baseSquare(baseSquare),
			baseAirPos(baseAirPos),
			hashNum(hashNum),
			baseHeight(baseHeight),
			toBeDeleted(false) {}
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


class CLosHandler : public boost::noncopyable
{
	CR_DECLARE(CLosHandler);
//	CR_DECLARE_SUB(CPoint);
	CR_DECLARE_SUB(DelayedInstance);

public:
	void MoveUnit(CUnit* unit, bool redoCurrent);
	void FreeInstance(LosInstance* instance);

	bool InLos(const CWorldObject* object, int allyTeam) {
		if (object->alwaysVisible || gs->globalLOS) {
			return true;
		}
		else if (object->useAirLos) {
			const int gx = (int)(object->pos.x * invAirDiv);
			const int gz = (int)(object->pos.z * invAirDiv);
			return !!airLosMap[allyTeam].At(gx, gz);
		}
		else {
			const int gx = (int)(object->pos.x * invLosDiv);
			const int gz = (int)(object->pos.z * invLosDiv);
			return !!losMap[allyTeam].At(gx, gz);
		}
	}

	bool InLos(const CUnit* unit, int allyTeam) {
		// NOTE: units are treated differently than world objects in 2 ways:
		//       1. they can be cloaked
		//       2. when underwater, they only get LOS if they also have sonar
		//          (when the requireSonarUnderWater variable is enabled)
		if (unit->alwaysVisible || gs->globalLOS) {
			return true;
		}
		else if (unit->isCloaked) {
			return false;
		}
		else if (unit->useAirLos) {
			const int gx = (int)(unit->pos.x * invAirDiv);
			const int gz = (int)(unit->pos.z * invAirDiv);
			return !!airLosMap[allyTeam].At(gx, gz);
		}
		else {
			if (unit->isUnderWater && requireSonarUnderWater &&
			    !radarhandler->InRadar(unit, allyTeam)) {
				return false;
			}
			const int gx = (int)(unit->pos.x * invLosDiv);
			const int gz = (int)(unit->pos.z * invLosDiv);
			return !!losMap[allyTeam].At(gx, gz);
		}
	}

	bool InLos(float3 pos, int allyTeam) {
		if (gs->globalLOS) {
			return true;
		}
		const int gx = (int)(pos.x * invLosDiv);
		const int gz = (int)(pos.z * invLosDiv);
		return !!losMap[allyTeam].At(gx, gz);
	}

	bool InAirLos(float3 pos, int allyTeam) {
		if (gs->globalLOS) {
			return true;
		}
		const int gx = (int)(pos.x * invAirDiv);
		const int gz = (int)(pos.z * invAirDiv);
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
	void PostLoad();
	void LosAdd(LosInstance* instance);
	int GetHashNum(CUnit* unit);
	void AllocInstance(LosInstance* instance);
	void CleanupInstance(LosInstance* instance);

	CLosAlgorithm losAlgo;

	std::list<LosInstance*> instanceHash[2309+1];

	std::deque<LosInstance*> toBeDeleted;

	struct DelayedInstance {
		CR_DECLARE_STRUCT(DelayedInstance);
		LosInstance* instance;
		int timeoutTime;
	};

	std::deque<DelayedInstance> delayQue;

public:
	void Update(void);
	void DelayedFreeInstance(LosInstance* instance);
};

extern CLosHandler* loshandler;

#endif /* LOSHANDLER_H */
