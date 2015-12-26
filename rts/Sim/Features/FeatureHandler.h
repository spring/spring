/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATURE_HANDLER_H
#define _FEATURE_HANDLER_H

#include <string>
#include <vector>
#include <unordered_map>

#include <boost/noncopyable.hpp>
#include "System/creg/creg_cond.h"

#include "FeatureDef.h"
#include "FeatureSet.h"
#include "Sim/Misc/SimObjectIDPool.h"


struct UnitDef;
class LuaTable;

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
	CFeatureHandler(LuaParser* defsParser);
	~CFeatureHandler();

	CFeature* LoadFeature(const FeatureLoadParams& params);
	CFeature* CreateWreckage(const FeatureLoadParams& params, const int numWreckLevels, bool emitSmoke);

	void Update();

	bool AddFeature(CFeature* feature);
	void DeleteFeature(CFeature* feature);
	CFeature* GetFeature(int id);

	void LoadFeaturesFromMap(bool onlyCreateDefs);
	const FeatureDef* GetFeatureDef(std::string name, const bool showError = true) const;
	const FeatureDef* GetFeatureDefByID(int id) const {
		if ((id < 1) || (static_cast<size_t>(id) >= featureDefsVector.size())) {
			return NULL;
		}
		return &featureDefsVector[id];
	}


	void SetFeatureUpdateable(CFeature* feature);
	void TerrainChanged(int x1, int y1, int x2, int y2);

	const std::unordered_map<std::string, int>& GetFeatureDefs() const { return featureDefs; }
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

	FeatureDef* CreateDefaultTreeFeatureDef(const std::string& name);
	FeatureDef* CreateDefaultGeoFeatureDef(const std::string& name);
	FeatureDef* CreateFeatureDef(const LuaTable& luaTable, const std::string& name);

	FeatureDef& GetNewFeatureDef();

	void AddFeatureDef(const std::string& name, FeatureDef* feature, bool isDefaultFeature);

private:
	SimObjectIDPool idPool;

	std::unordered_map<std::string, int> featureDefs;
	std::vector<FeatureDef> featureDefsVector;

	std::vector<int> toBeFreedFeatureIDs;
	CFeatureSet activeFeatures;
	std::vector<CFeature*> features;

	std::vector<CFeature*> updateFeatures;
};

extern CFeatureHandler* featureHandler;


#endif // _FEATURE_HANDLER_H
