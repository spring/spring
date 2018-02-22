/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATURE_HANDLER_H
#define _FEATURE_HANDLER_H

#include <deque>
#include <vector>

#include "System/float3.h"
#include "System/Misc/NonCopyable.h"
#include "System/creg/creg_cond.h"
#include "System/UnorderedSet.hpp"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/SimObjectIDPool.h"

struct UnitDef;
class LuaTable;
struct FeatureDef;

struct FeatureLoadParams {
	const FeatureDef* featureDef;
	const UnitDef* unitDef;

	float3 pos;
	float3 speed;

	int featureID;
	int teamID;
	int allyTeamID;

	short int heading;
	short int facing;

	int wreckLevels;
	int smokeTime;
};


class CFeature;
class CFeatureHandler : public spring::noncopyable
{
	CR_DECLARE_STRUCT(CFeatureHandler)

public:
	CFeatureHandler(): idPool(MAX_FEATURES) {}

	void Init();
	void Kill();

	CFeature* LoadFeature(const FeatureLoadParams& params);
	CFeature* CreateWreckage(const FeatureLoadParams& params);
	CFeature* GetFeature(unsigned int id) { return ((id < features.size())? features[id]: nullptr); }

	void Update();

	bool UpdateFeature(CFeature* feature);
	bool TryFreeFeatureID(int id);
	bool AddFeature(CFeature* feature);
	void DeleteFeature(CFeature* feature);

	void LoadFeaturesFromMap();

	void SetFeatureUpdateable(CFeature* feature);
	void TerrainChanged(int x1, int y1, int x2, int y2);

	const spring::unordered_set<int>& GetActiveFeatureIDs() const { return activeFeatureIDs; }

private:
	bool CanAddFeature(int id) const {
		// do we want to be assigned a random ID and are any left in pool?
		if (id < 0)
			return true;
		// is this ID not already in use *and* has it been recycled by pool?
		if (id < features.size())
			return (features[id] == nullptr && idPool.HasID(id));
		// AddFeature will not make new room for us
		return false;
	}

	void InsertActiveFeature(CFeature* feature);

private:
	SimObjectIDPool idPool;

	spring::unordered_set<int> activeFeatureIDs;
	std::vector<int> deletedFeatureIDs;
	std::vector<CFeature*> features;
	std::vector<CFeature*> updateFeatures;
};

extern CFeatureHandler featureHandler;


#endif // _FEATURE_HANDLER_H
