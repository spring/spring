/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureDefHandler.h"

#include "FeatureDef.h"
#include "Lua/LuaParser.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Objects/SolidObject.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

CFeatureDefHandler* featureDefHandler = NULL;

CFeatureDefHandler::CFeatureDefHandler(LuaParser* defsParser)
{
	const LuaTable rootTable = defsParser->GetRoot().SubTable("FeatureDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading FeatureDefs");
	}

	// featureDefIDs start with 1
	featureDefsVector.emplace_back();

	// get most of the feature defs (missing trees and geovent from the map)
	std::vector<std::string> keys;
	rootTable.GetKeys(keys);

	for (unsigned int i = 0; i < keys.size(); i++) {
		const std::string& nameMixedCase = keys[i];
		const std::string& nameLowerCase = StringToLower(nameMixedCase);
		const LuaTable& fdTable = rootTable.SubTable(nameMixedCase);

		AddFeatureDef(nameLowerCase, CreateFeatureDef(fdTable, nameLowerCase), false);
	}
	for (unsigned int i = 0; i < keys.size(); i++) {
		const std::string& nameMixedCase = keys[i];
		const std::string& nameLowerCase = StringToLower(nameMixedCase);
		const LuaTable& fdTable = rootTable.SubTable(nameMixedCase);

		const FeatureDef* fd = GetFeatureDef(nameLowerCase);
		const FeatureDef* dfd = GetFeatureDef(fdTable.GetString("featureDead", ""));

		if (fd == nullptr)
			continue;

		if (dfd == nullptr)
			continue;

		const_cast<FeatureDef*>(fd)->deathFeatureDefID = dfd->id;
	}
}

CFeatureDefHandler::~CFeatureDefHandler()
{
	featureDefs.clear();
	featureDefsVector.clear();
}


void CFeatureDefHandler::AddFeatureDef(const std::string& name, FeatureDef* fd, bool isDefaultFeature)
{
	if (fd == nullptr)
		return;

	assert(featureDefs.find(name) == featureDefs.end());

	// generated trees, etc have no pieces
	fd->collisionVolume.SetDefaultToPieceTree(fd->collisionVolume.DefaultToPieceTree() && !isDefaultFeature);
	fd->collisionVolume.SetIgnoreHits(fd->geoThermal);

	featureDefs[name] = fd->id;
}

FeatureDef& CFeatureDefHandler::GetNewFeatureDef()
{
	featureDefsVector.emplace_back();
	FeatureDef& fd = featureDefsVector.back();
	fd.id = featureDefsVector.size() - 1;

	return fd;
}


FeatureDef* CFeatureDefHandler::CreateFeatureDef(const LuaTable& fdTable, const std::string& mixedCase)
{
	const std::string& name = StringToLower(mixedCase);

	if (featureDefs.find(name) != featureDefs.end())
		return nullptr;

	FeatureDef& fd = GetNewFeatureDef();

	fd.name = name;
	fd.description = fdTable.GetString("description", "");

	fd.collidable    =  fdTable.GetBool("blocking",        true);
	fd.selectable    = !fdTable.GetBool("noselect",        false);
	fd.burnable      =  fdTable.GetBool("flammable",       false);
	fd.destructable  = !fdTable.GetBool("indestructible",  false);
	fd.reclaimable   =  fdTable.GetBool("reclaimable",     fd.destructable);
	fd.autoreclaim   =  fdTable.GetBool("autoreclaimable", fd.reclaimable);
	fd.resurrectable =  fdTable.GetInt("resurrectable",    -1);
	fd.geoThermal    =  fdTable.GetBool("geoThermal",      false);
	fd.floating      =  fdTable.GetBool("floating",        false);

	fd.metal       = fdTable.GetFloat("metal",  0.0f);
	fd.energy      = fdTable.GetFloat("energy", 0.0f);
	fd.health      = fdTable.GetFloat("damage", 0.0f);
	fd.reclaimTime = std::max(1.0f, fdTable.GetFloat("reclaimTime", (fd.metal + fd.energy) * 6.0f));

	fd.smokeTime = fdTable.GetInt("smokeTime", 300);

	fd.modelName = fdTable.GetString("object", "");
	fd.drawType = fdTable.GetInt("drawType", DRAWTYPE_NONE);

	if (!fd.modelName.empty()) {
		fd.drawType = DRAWTYPE_MODEL;
	}


	// initialize the (per-featuredef) collision-volume,
	// all CFeature instances hold a copy of this object
	//
	// takes precedence over the old sphere tags as well
	// as feature->radius (for feature <---> projectile
	// interactions)
	fd.ParseCollisionVolume(fdTable);

	fd.upright = fdTable.GetBool("upright", false);

	fd.xsize = std::max(1 * SPRING_FOOTPRINT_SCALE, fdTable.GetInt("footprintX", 1) * SPRING_FOOTPRINT_SCALE);
	fd.zsize = std::max(1 * SPRING_FOOTPRINT_SCALE, fdTable.GetInt("footprintZ", 1) * SPRING_FOOTPRINT_SCALE);

	const float minMass = CSolidObject::MINIMUM_MASS;
	const float maxMass = CSolidObject::MAXIMUM_MASS;
	const float defMass = (fd.metal * 0.4f) + (fd.health * 0.1f);

	fd.mass = Clamp(fdTable.GetFloat("mass", defMass), minMass, maxMass);
	fd.crushResistance = fdTable.GetFloat("crushResistance", fd.mass);

	fd.decalDef.Parse(fdTable);

	// custom parameters table
	fdTable.SubTable("customParams").GetMap(fd.customParams);

	return &fd;
}


FeatureDef* CFeatureDefHandler::CreateDefaultTreeFeatureDef(const std::string& name)
{
	FeatureDef& fd = GetNewFeatureDef();

	fd.collidable = true;
	fd.burnable = true;
	fd.destructable = true;
	fd.reclaimable = true;
	fd.drawType = DRAWTYPE_TREE + atoi(name.substr(8).c_str());
	fd.energy = 250;
	fd.metal = 0;
	fd.reclaimTime = 1500;
	fd.health = 5.0f;
	fd.xsize = 2;
	fd.zsize = 2;
	fd.name = name;
	fd.description = "Tree";
	fd.mass = 20;
	fd.collisionVolume = CollisionVolume("", ZeroVector, ZeroVector);
	return &fd;
}

FeatureDef* CFeatureDefHandler::CreateDefaultGeoFeatureDef(const std::string& name)
{
	FeatureDef& fd = GetNewFeatureDef();

	fd.collidable = false;
	fd.burnable = false;
	fd.destructable = false;
	fd.reclaimable = false;
	fd.geoThermal = true;
	// geos are (usually) rendered only as vents baked into
	// the map's ground texture and emit smoke to be visible
	fd.drawType = DRAWTYPE_NONE;
	fd.energy = 0;
	fd.metal = 0;
	fd.reclaimTime = 0;
	fd.health = 0.0f;
	fd.xsize = 0;
	fd.zsize = 0;
	fd.name = name;
	fd.mass = CSolidObject::DEFAULT_MASS;
	// geothermal features have no physical map presence
	fd.collisionVolume = CollisionVolume("", ZeroVector, ZeroVector);
	return &fd;
}


const FeatureDef* CFeatureDefHandler::GetFeatureDef(string name, const bool showError) const
{
	if (name.empty())
		return nullptr;

	StringToLowerInPlace(name);
	const auto fi = featureDefs.find(name);

	if (fi != featureDefs.end())
		return &featureDefsVector[fi->second];

	if (showError) {
		LOG_L(L_ERROR, "[%s] could not find FeatureDef \"%s\"",
				__FUNCTION__, name.c_str());
	}

	return nullptr;
}


void CFeatureDefHandler::LoadFeatureDefsFromMap()
{
	// add default tree and geo FeatureDefs defined by the map
	const int numFeatureTypes = readMap->GetNumFeatureTypes();

	for (int a = 0; a < numFeatureTypes; ++a) {
		const string& name = StringToLower(readMap->GetFeatureTypeName(a));

		if (GetFeatureDef(name, false) == nullptr) {
			if (name.find("treetype") != string::npos) {
				AddFeatureDef(name, CreateDefaultTreeFeatureDef(name), true);
			}
			else if (name.find("geovent") != string::npos) {
				AddFeatureDef(name, CreateDefaultGeoFeatureDef(name), true);
			}
			else {
				LOG_L(L_ERROR, "[%s] unknown map feature type \"%s\"",
						__FUNCTION__, name.c_str());
			}
		}
	}

	// add a default geovent FeatureDef if the map did not
	if (GetFeatureDef("geovent", false) == nullptr) {
		AddFeatureDef("geovent", CreateDefaultGeoFeatureDef("geovent"), true);
	}

}
