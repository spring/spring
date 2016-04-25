/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATURE_HANDLER_H
#define _FEATURE_HANDLER_H

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include "System/creg/creg_cond.h"

#include "FeatureSet.h"
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

	int smokeTime;
};

class LuaParser;
class CFeatureHandler : public boost::noncopyable
{
	CR_DECLARE_STRUCT(CFeatureHandler)

public:
	CFeatureHandler() { }
	~CFeatureHandler();

	CFeature* LoadFeature(const FeatureLoadParams& params);
	CFeature* CreateWreckage(const FeatureLoadParams& params, const int numWreckLevels, bool emitSmoke);

	void Update();

	bool UpdateFeature(CFeature* feature);
	bool TryFreeFeatureID(int id);
	bool AddFeature(CFeature* feature);
	void DeleteFeature(CFeature* feature);
	CFeature* GetFeature(int id);

	void LoadFeaturesFromMap();

	void SetFeatureUpdateable(CFeature* feature);
	void TerrainChanged(int x1, int y1, int x2, int y2);

	const CFeatureSet& GetActiveFeatures() const { return activeFeatures; }

private:
	bool CanAddFeature(int id) const {
		// do we want to be assigned a random ID? (in case
		// idPool is empty, AddFeature will always allocate
		// more)
		if (id < 0)
			return true;
		// is this ID not already in use?
		if (id < features.size())
			return (features[id] == NULL);
		// AddFeature will make new room for us
		return true;
	}
	bool NeedAllocateNewFeatureIDs(const CFeature* feature) const;
	void AllocateNewFeatureIDs(const CFeature* feature);
	void InsertActiveFeature(CFeature* feature);

private:
	SimObjectIDPool idPool;

	std::vector<int> toBeFreedFeatureIDs;
	CFeatureSet activeFeatures;
	std::vector<CFeature*> features;

	std::vector<CFeature*> updateFeatures;
};

extern CFeatureHandler* featureHandler;


#endif // _FEATURE_HANDLER_H
