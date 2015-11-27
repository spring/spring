/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOS_HANDLER_H
#define LOS_HANDLER_H

#include <vector>
#include <deque>
#include <boost/noncopyable.hpp>
#include "Map/Ground.h"
#include "Sim/Misc/LosMap.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"
#include "System/type2.h"
#include "System/Rectangle.h"
#include "System/EventClient.h"
#include <boost/unordered_map.hpp>


/**
 * LoS Instance
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
 * (basePos, baseSquare) on the LOS map, has the same radius, is in the
 * same ally-team and has the same height.
 */
struct SLosInstance
{
	SLosInstance(int id)
		: allyteam(-1)
		, radius(-1)
		, basePos()
		, baseHeight(-1)
		, refCount(0)
		, hashNum(-1)
		, status(NONE)
		, toBeDeleted(false)
		, id(id)
	{}
	void Init(int radius, int allyteam, int2 basePos, float baseHeight, int hashNum);

	// hash properties
	int allyteam;
	int radius;
	int2 basePos;
	float baseHeight;

	// working data
	int refCount;
	struct RLE { int start; unsigned length; };
	static constexpr RLE EMPTY_RLE = {0,0};
	std::vector<RLE> squares;

	// helpers
	int hashNum;
	enum TLosStatus {
		NONE = 0,
		NEW  = 1,
		REACTIVATE = 2,
		RECALC = 4,
		REMOVE = 8,
	};
	int status;
	bool toBeDeleted;
	int id;
};




/**
 * All different types of LOS are implemented using ILosType, which is a
 * 2d array essentially containing a reference count. That is to say, each
 * position on the map counts how many LosInstances "can see" that square.
 * Units may share their "presence" on the LOS map through sharing a single
 * LosInstance.
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
class ILosType
{
public:
	// the Interface
	int2 PosToSquare(const float3 pos) const { return int2(pos.x * invDiv, pos.z * invDiv); }

	inline bool InSight(const float3 pos, int allyTeam) const {
		assert(allyTeam < losMaps.size());
		return (losMaps[allyTeam].At(PosToSquare(pos)) != 0);
	}

public:
	enum LosAlgoType { LOS_ALGO_RAYCAST, LOS_ALGO_CIRCLE };
	enum LosType {
		LOS_TYPE_LOS,
		LOS_TYPE_AIRLOS,
		LOS_TYPE_RADAR,
		LOS_TYPE_SONAR,
		LOS_TYPE_JAMMER,
		LOS_TYPE_SEISMIC,
		LOS_TYPE_SONAR_JAMMER,
		LOS_TYPE_COUNT
	};

	ILosType(const int mipLevel, LosType type);
	~ILosType();

public:
	void Update();
	void UpdateHeightMapSynced(SRectangle rect);
	void RemoveUnit(CUnit* unit, bool delayed = false);
	void UpdateUnit(CUnit* unit);

private:
	//void PostLoad();

	void LosAdd(SLosInstance* instance);
	void LosRemove(SLosInstance* instance);

	void RefInstance(SLosInstance* instance);
	void UnrefInstance(SLosInstance* instance);
	void DelayedUnrefInstance(SLosInstance* instance);
	void AddInstanceToCache(SLosInstance* instance);

	void UpdateInstanceStatus(SLosInstance* instance, SLosInstance::TLosStatus status);
	static SLosInstance::TLosStatus OptimizeInstanceUpdate(SLosInstance* instance);

	SLosInstance* CreateInstance();
	void DeleteInstance(SLosInstance* instance);

private:
	int GetHashNum(const int allyteam, const int2 baseLos, const float radius) const;

	float GetRadius(const CUnit* unit) const;
	float GetHeight(const CUnit* unit) const;

public:
	const int   mipLevel;
	const int   divisor;
	const float invDiv;
	const int2  size;
	const LosType type;
	const LosAlgoType algoType;
	std::vector<CLosMap> losMaps;

	static size_t cacheFails;
	static size_t cacheHits;
	static size_t cacheReactivated;

	boost::unordered_multimap<int, SLosInstance*> instanceHash; // we intentionally use boost version here, gcc's one uses a linked listed and so is much slower
	std::deque<SLosInstance> instances;
	std::deque<int> freeIDs;

private:
	struct DelayedInstance {
		SLosInstance* instance;
		int timeoutTime;
	};

	std::deque<DelayedInstance> delayedDeleteQue;
	std::deque<DelayedInstance> delayedTerraQue;
	std::deque<SLosInstance*> losUpdate;
	std::deque<SLosInstance*> losCache;
	static constexpr int CACHE_SIZE = 4096;
};




/**
 * Handles line of sight (LOS) updates for all units and all ally-teams.
 *
 * Helper class to access the different types of LoS.
 */
class CLosHandler : public CEventClient
{
	CR_DECLARE_STRUCT(CLosHandler)
	CLosHandler();
	~CLosHandler();

public:
	// the Interface
	bool InLos(const CUnit* unit, int allyTeam) const;

	bool InLos(const CWorldObject* obj, int allyTeam) const {
		if (obj->alwaysVisible || globalLOS[allyTeam])
			return true;
		if (obj->useAirLos)
			return (InAirLos(obj->pos, allyTeam) || InAirLos(obj->pos + obj->speed, allyTeam));

		// test visibility at two positions, mostly for long beam-projectiles
		//   slow-moving objects will be visible no earlier or later than before on average
		//   fast-moving objects will be visible at most one SU before they otherwise would
		return (InLos(obj->pos, allyTeam) || InLos(obj->pos + obj->speed, allyTeam));
	}

	bool InLos(const float3 pos, int allyTeam) const {
		if (globalLOS[allyTeam])
			return true;
		return los.InSight(pos, allyTeam);
	}

	bool InAirLos(const float3 pos, int allyTeam) const {
		if (globalLOS[allyTeam])
			return true;
		return airLos.InSight(pos, allyTeam);
	}


	bool InRadar(const float3 pos, int allyTeam) const;
	bool InRadar(const CUnit* unit, int allyTeam) const;


	// returns whether a square is being radar- or sonar-jammed
	// (even when the square is not in radar- or sonar-coverage)
	bool InJammer(const float3 pos, int allyTeam) const;
	bool InJammer(const CUnit* unit, int allyTeam) const;


	bool InSeismicDistance(const CUnit* unit, int allyTeam) const {
		return seismic.InSight(unit->pos, allyTeam);
	}

public:
	// default operations for targeting-facilities
	void IncreaseAllyTeamRadarErrorSize(int allyTeam) { radarErrorSizes[allyTeam] *= baseRadarErrorMult; }
	void DecreaseAllyTeamRadarErrorSize(int allyTeam) { radarErrorSizes[allyTeam] /= baseRadarErrorMult; }

	// API functions
	void SetAllyTeamRadarErrorSize(int allyTeam, float size) { radarErrorSizes[allyTeam] = size; }
	float GetAllyTeamRadarErrorSize(int allyTeam) const { return radarErrorSizes[allyTeam]; }

	void SetBaseRadarErrorSize(float size) { baseRadarErrorSize = size; }
	void SetBaseRadarErrorMult(float mult) { baseRadarErrorMult = mult; }
	float GetBaseRadarErrorSize() const { return baseRadarErrorSize; }
	float GetBaseRadarErrorMult() const { return baseRadarErrorMult; }

public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "UnitDestroyed") || (eventName == "UnitNanoframed") || (eventName == "UnitTaken") || (eventName == "UnitLoaded");
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void UnitDestroyed(const CUnit* unit, const CUnit* attacker) override;
	void UnitTaken(const CUnit* unit, int oldTeam, int newTeam) override;
	void UnitLoaded(const CUnit* unit, const CUnit* transport) override;
	void UnitNanoframed(const CUnit* unit) override;

public:
	void Update();
	void UpdateHeightMapSynced(SRectangle rect);

public:
	/**
	* @brief global line-of-sight
	*
	* Whether everything on the map is visible at all times to a given ALLYteam
	* There can never be more allyteams than teams, hence the size is MAX_TEAMS
	*/
	bool globalLOS[MAX_TEAMS];

	ILosType los;
	ILosType airLos;
	ILosType radar;
	ILosType sonar;
	ILosType seismic;
	ILosType commonJammer;
	ILosType commonSonarJammer;

private:
	static constexpr float defBaseRadarErrorSize = 96.0f;
	static constexpr float defBaseRadarErrorMult = 20.f;

	float baseRadarErrorSize;
	float baseRadarErrorMult;
	std::vector<float> radarErrorSizes;
	std::vector<ILosType*> losTypes;
};


extern CLosHandler* losHandler;

#endif /* LOS_HANDLER_H */
