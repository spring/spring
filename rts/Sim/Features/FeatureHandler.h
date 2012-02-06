/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATURE_HANDLER_H
#define _FEATURE_HANDLER_H

#include <string>
#include <list>
#include <vector>
#include <boost/noncopyable.hpp>
#include "System/creg/creg_cond.h"
#include "FeatureDef.h"
#include "FeatureSet.h"


struct S3DModel;
struct UnitDef;
class LuaTable;


class CFeatureHandler : public boost::noncopyable
{
	CR_DECLARE(CFeatureHandler);

public:
	CFeatureHandler();
	~CFeatureHandler();

	CFeature* CreateWreckage(const float3& pos, const std::string& name,
		float rot, int facing, int iter, int team, int allyteam, bool emitSmoke,
		const UnitDef* udef, const float3& speed = ZeroVector);

	void Update();

	int AddFeature(CFeature* feature);
	void DeleteFeature(CFeature* feature);
	CFeature* GetFeature(int id);

	void LoadFeaturesFromMap(bool onlyCreateDefs);
	const FeatureDef* GetFeatureDef(std::string name, const bool showError = true);
	const FeatureDef* GetFeatureDefByID(int id);

	void SetFeatureUpdateable(CFeature* feature);
	void TerrainChanged(int x1, int y1, int x2, int y2);

	const std::map<std::string, const FeatureDef*>& GetFeatureDefs() const { return featureDefs; }
	const CFeatureSet& GetActiveFeatures() const { return activeFeatures; }

private:
	FeatureDef* CreateDefaultTreeFeatureDef(const std::string& name) const;
	FeatureDef* CreateDefaultGeoFeatureDef(const std::string& name) const;
	FeatureDef* CreateFeatureDef(const LuaTable& luaTable, const std::string& name) const;

	void AddFeatureDef(const std::string& name, FeatureDef* feature);

private:
	std::map<std::string, const FeatureDef*> featureDefs;
	std::vector<const FeatureDef*> featureDefsVector;

	std::list<int> freeIDs;
	std::list<int> toBeFreedIDs;
	CFeatureSet activeFeatures;
	std::vector<CFeature*> features;

	std::list<int> toBeRemoved;
	CFeatureSet updateFeatures;
};

extern CFeatureHandler* featureHandler;


#endif // _FEATURE_HANDLER_H
