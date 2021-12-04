/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATUREDEF_HANDLER_H
#define _FEATUREDEF_HANDLER_H

#include <string>
#include <vector>

#include "FeatureDef.h"

#include "System/Misc/NonCopyable.h"
#include "System/UnorderedMap.hpp"


class LuaTable;
class LuaParser;

class CFeatureDefHandler : public spring::noncopyable
{

public:
	void Init(LuaParser* defsParser);
	void Kill() {
		featureDefIDs.clear(); // never iterated in synced code
		featureDefsVector.clear();
	}

	void LoadFeatureDefsFromMap();
	const FeatureDef* GetFeatureDef(std::string name, const bool showError = true) const;
	const FeatureDef* GetFeatureDefByID(int id) const {
		if (!IsValidFeatureDefID(id))
			return nullptr;

		return &featureDefsVector[id];
	}

	bool IsValidFeatureDefID(const int id) const {
		return (id > 0) && (static_cast<size_t>(id) < featureDefsVector.size());
	}

	// id=0 is not a valid FeatureDef, hence the -1
	unsigned int NumFeatureDefs() const { return (featureDefsVector.size() - 1); }

	const std::vector<FeatureDef>& GetFeatureDefsVec() const { return featureDefsVector; }
	const spring::unordered_map<std::string, int>& GetFeatureDefIDs() const { return featureDefIDs; }

private:

	FeatureDef* CreateDefaultTreeFeatureDef(const std::string& name);
	FeatureDef* CreateDefaultGeoFeatureDef(const std::string& name);
	FeatureDef* CreateFeatureDef(const LuaTable& luaTable, const std::string& name);

	FeatureDef& GetNewFeatureDef();

	void AddFeatureDef(const std::string& name, FeatureDef* feature, bool isDefaultFeature);

private:
	spring::unordered_map<std::string, int> featureDefIDs;
	std::vector<FeatureDef> featureDefsVector;
};

extern CFeatureDefHandler* featureDefHandler;


#endif // _FEATUREDEF_HANDLER_H
