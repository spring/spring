/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATUREDEF_HANDLER_H
#define _FEATUREDEF_HANDLER_H

#include <string>
#include <vector>
#include <unordered_map>

#include <boost/noncopyable.hpp>


class LuaTable;
class LuaParser;
struct FeatureDef;

class CFeatureDefHandler : public boost::noncopyable
{

public:
	CFeatureDefHandler(LuaParser* defsParser);
	~CFeatureDefHandler();

	void LoadFeatureDefsFromMap();
	const FeatureDef* GetFeatureDef(std::string name, const bool showError = true) const;
	const FeatureDef* GetFeatureDefByID(int id) const {
		if ((id < 1) || (static_cast<size_t>(id) >= featureDefsVector.size())) {
			return NULL;
		}
		return &featureDefsVector[id];
	}

	const std::unordered_map<std::string, int>& GetFeatureDefs() const { return featureDefs; }

private:

	FeatureDef* CreateDefaultTreeFeatureDef(const std::string& name);
	FeatureDef* CreateDefaultGeoFeatureDef(const std::string& name);
	FeatureDef* CreateFeatureDef(const LuaTable& luaTable, const std::string& name);

	FeatureDef& GetNewFeatureDef();

	void AddFeatureDef(const std::string& name, FeatureDef* feature, bool isDefaultFeature);

private:
	std::unordered_map<std::string, int> featureDefs;
	std::vector<FeatureDef> featureDefsVector;
};

extern CFeatureDefHandler* featureDefHandler;


#endif // _FEATUREDEF_HANDLER_H
