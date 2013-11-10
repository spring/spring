/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureHandler.h"

#include "Lua/LuaParser.h"
#include "Lua/LuaRules.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "System/creg/STL_List.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/creg/STL_Set.h"

CFeatureHandler* featureHandler = NULL;

/******************************************************************************/

CR_BIND(CFeatureHandler, (NULL));
CR_REG_METADATA(CFeatureHandler, (
	CR_MEMBER(idPool),
	CR_MEMBER(featureDefs),
	CR_MEMBER(featureDefsVector),
	CR_MEMBER(toBeFreedFeatureIDs),
	CR_MEMBER(activeFeatures),
	CR_MEMBER(features),
	CR_MEMBER(toBeRemoved),
	CR_MEMBER(updateFeatures)
));

/******************************************************************************/

CFeatureHandler::CFeatureHandler(LuaParser* defsParser)
{
	const LuaTable rootTable = defsParser->GetRoot().SubTable("FeatureDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading FeatureDefs");
	}

	// featureDefIDs start with 1
	featureDefsVector.push_back(NULL);

	// get most of the feature defs (missing trees and geovent from the map)
	std::vector<std::string> keys;
	rootTable.GetKeys(keys);

	for (unsigned int i = 0; i < keys.size(); i++) {
		const std::string& nameMixedCase = keys[i];
		const std::string& nameLowerCase = StringToLower(nameMixedCase);
		const LuaTable& fdTable = rootTable.SubTable(nameMixedCase);

		AddFeatureDef(nameLowerCase, CreateFeatureDef(fdTable, nameLowerCase));
	}
	for (unsigned int i = 0; i < keys.size(); i++) {
		const std::string& nameMixedCase = keys[i];
		const std::string& nameLowerCase = StringToLower(nameMixedCase);
		const LuaTable& fdTable = rootTable.SubTable(nameMixedCase);

		const FeatureDef* fd = GetFeatureDef(nameLowerCase);
		const FeatureDef* dfd = GetFeatureDef(fdTable.GetString("featureDead", ""));

		if (fd == NULL) continue;
		if (dfd == NULL) continue;

		const_cast<FeatureDef*>(fd)->deathFeatureDefID = dfd->id;
	}
}

CFeatureHandler::~CFeatureHandler()
{
	for (CFeatureSet::iterator fi = activeFeatures.begin(); fi != activeFeatures.end(); ++fi) {
		delete *fi;
	}

	for (std::map<std::string, const FeatureDef*>::iterator it = featureDefs.begin(); it != featureDefs.end(); ++it) {
		delete it->second;
	}

	activeFeatures.clear();
	features.clear();
	featureDefs.clear();
}



void CFeatureHandler::AddFeatureDef(const std::string& name, FeatureDef* fd)
{
	if (fd == NULL)
		return;

	std::map<std::string, const FeatureDef*>::const_iterator it = featureDefs.find(name);

	if (it != featureDefs.end()) {
		featureDefsVector[it->second->id] = fd;
	} else {
		fd->id = featureDefsVector.size();
		featureDefsVector.push_back(fd);
	}

	// MUST be false for CollisionHandler (features have no LocalModel)
	fd->collisionVolume->SetDefaultToPieceTree(false);
	fd->collisionVolume->SetIgnoreHits(fd->geoThermal);

	featureDefs[name] = fd;
}


FeatureDef* CFeatureHandler::CreateFeatureDef(const LuaTable& fdTable, const std::string& mixedCase) const
{
	const std::string& name = StringToLower(mixedCase);

	if (featureDefs.find(name) != featureDefs.end())
		return NULL;

	FeatureDef* fd = new FeatureDef();

	fd->name = name;
	fd->description = fdTable.GetString("description", "");

	fd->collidable    =  fdTable.GetBool("blocking",        true);
	fd->selectable    = !fdTable.GetBool("noselect",        false);
	fd->burnable      =  fdTable.GetBool("flammable",       false);
	fd->destructable  = !fdTable.GetBool("indestructible",  false);
	fd->reclaimable   =  fdTable.GetBool("reclaimable",     fd->destructable);
	fd->autoreclaim   =  fdTable.GetBool("autoreclaimable", fd->reclaimable);
	fd->resurrectable =  fdTable.GetInt("resurrectable",    -1);
	fd->geoThermal    =  fdTable.GetBool("geoThermal",      false);
	fd->floating      =  fdTable.GetBool("floating",        false);

	fd->metal       = fdTable.GetFloat("metal",  0.0f);
	fd->energy      = fdTable.GetFloat("energy", 0.0f);
	fd->health      = fdTable.GetFloat("damage", 0.0f);
	fd->reclaimTime = std::max(1.0f, fdTable.GetFloat("reclaimTime", (fd->metal + fd->energy) * 6.0f));

	fd->smokeTime = fdTable.GetInt("smokeTime", 300);

	fd->modelName = fdTable.GetString("object", "");
	fd->drawType = fdTable.GetInt("drawType", DRAWTYPE_NONE);

	if (!fd->modelName.empty()) {
		fd->drawType = DRAWTYPE_MODEL;
	}


	// initialize the (per-featuredef) collision-volume,
	// all CFeature instances hold a copy of this object
	//
	// takes precedence over the old sphere tags as well
	// as feature->radius (for feature <---> projectile
	// interactions)
	fd->ParseCollisionVolume(fdTable);

	fd->upright = fdTable.GetBool("upright", false);

	fd->xsize = std::max(1 * SPRING_FOOTPRINT_SCALE, fdTable.GetInt("footprintX", 1) * SPRING_FOOTPRINT_SCALE);
	fd->zsize = std::max(1 * SPRING_FOOTPRINT_SCALE, fdTable.GetInt("footprintZ", 1) * SPRING_FOOTPRINT_SCALE);

	const float minMass = CSolidObject::MINIMUM_MASS;
	const float maxMass = CSolidObject::MAXIMUM_MASS;
	const float defMass = (fd->metal * 0.4f) + (fd->health * 0.1f);

	fd->mass = Clamp(fdTable.GetFloat("mass", defMass), minMass, maxMass);
	fd->crushResistance = fdTable.GetFloat("crushResistance", fd->mass);

	fd->decalDef.Parse(fdTable);

	// custom parameters table
	fdTable.SubTable("customParams").GetMap(fd->customParams);

	return fd;
}


FeatureDef* CFeatureHandler::CreateDefaultTreeFeatureDef(const std::string& name) const {
	FeatureDef* fd = new FeatureDef();
	fd->collidable = true;
	fd->burnable = true;
	fd->destructable = true;
	fd->reclaimable = true;
	fd->drawType = DRAWTYPE_TREE + atoi(name.substr(8).c_str());
	fd->energy = 250;
	fd->metal = 0;
	fd->reclaimTime = 1500;
	fd->health = 5.0f;
	fd->xsize = 2;
	fd->zsize = 2;
	fd->name = name;
	fd->description = "Tree";
	fd->mass = 20;
	fd->collisionVolume = new CollisionVolume("", ZeroVector, ZeroVector);
	return fd;
}

FeatureDef* CFeatureHandler::CreateDefaultGeoFeatureDef(const std::string& name) const {
	FeatureDef* fd = new FeatureDef();
	fd->collidable = false;
	fd->burnable = false;
	fd->destructable = false;
	fd->reclaimable = false;
	fd->geoThermal = true;
	// geos are (usually) rendered only as vents baked into
	// the map's ground texture and emit smoke to be visible
	fd->drawType = DRAWTYPE_NONE;
	fd->energy = 0;
	fd->metal = 0;
	fd->reclaimTime = 0;
	fd->health = 0.0f;
	fd->xsize = 0;
	fd->zsize = 0;
	fd->name = name;
	fd->mass = CSolidObject::DEFAULT_MASS;
	// geothermal features have no physical map presence
	fd->collisionVolume = new CollisionVolume("", ZeroVector, ZeroVector);
	return fd;
}




const FeatureDef* CFeatureHandler::GetFeatureDef(string name, const bool showError)
{
	if (name.empty())
		return NULL;

	StringToLowerInPlace(name);
	map<string, const FeatureDef*>::iterator fi = featureDefs.find(name);

	if (fi != featureDefs.end()) {
		return fi->second;
	}

	if (showError) {
		LOG_L(L_ERROR, "[%s] could not find FeatureDef \"%s\"",
				__FUNCTION__, name.c_str());
	}

	return NULL;
}


const FeatureDef* CFeatureHandler::GetFeatureDefByID(int id)
{
	if ((id < 1) || (static_cast<size_t>(id) >= featureDefsVector.size())) {
		return NULL;
	}
	return featureDefsVector[id];
}



void CFeatureHandler::LoadFeaturesFromMap(bool onlyCreateDefs)
{
	// add default tree and geo FeatureDefs defined by the map
	const int numFeatureTypes = readMap->GetNumFeatureTypes();

	for (int a = 0; a < numFeatureTypes; ++a) {
		const string& name = StringToLower(readMap->GetFeatureTypeName(a));

		if (GetFeatureDef(name, false) == NULL) {
			if (name.find("treetype") != string::npos) {
				AddFeatureDef(name, CreateDefaultTreeFeatureDef(name));
			}
			else if (name.find("geovent") != string::npos) {
				AddFeatureDef(name, CreateDefaultGeoFeatureDef(name));
			}
			else {
				LOG_L(L_ERROR, "[%s] unknown map feature type \"%s\"",
						__FUNCTION__, name.c_str());
			}
		}
	}

	// add a default geovent FeatureDef if the map did not
	if (GetFeatureDef("geovent", false) == NULL) {
		AddFeatureDef("geovent", CreateDefaultGeoFeatureDef("geovent"));
	}

	if (!onlyCreateDefs) {
		// create map-specified feature instances
		const int numFeatures = readMap->GetNumFeatures();
		MapFeatureInfo* mfi = new MapFeatureInfo[numFeatures];
		readMap->GetFeatureInfo(mfi);

		for (int a = 0; a < numFeatures; ++a) {
			const string& name = StringToLower(readMap->GetFeatureTypeName(mfi[a].featureType));
			map<string, const FeatureDef*>::iterator def = featureDefs.find(name);

			if (def == featureDefs.end()) {
				LOG_L(L_ERROR, "Unknown feature named '%s'", name.c_str());
				continue;
			}

			FeatureLoadParams params = {
				def->second,
				NULL,

				float3(mfi[a].pos.x, ground->GetHeightReal(mfi[a].pos.x, mfi[a].pos.z), mfi[a].pos.z),
				ZeroVector,

				-1, // featureID
				-1, // teamID
				-1, // allyTeamID

				static_cast<short int>(mfi[a].rotation),
				FACING_SOUTH,

				0, // smokeTime
			};

			LoadFeature(params);
		}

		delete[] mfi;
	}
}


CFeature* CFeatureHandler::LoadFeature(const FeatureLoadParams& params) {
	// need to check this BEFORE creating the instance
	if (!CanAddFeature(params.featureID))
		return NULL;

	// Initialize() calls AddFeature -> no memory-leak
	CFeature* feature = new CFeature();
	feature->Initialize(params);
	return feature;
}


bool CFeatureHandler::NeedAllocateNewFeatureIDs(const CFeature* feature) const
{
	if (feature->id < 0 && idPool.IsEmpty())
		return true;
	if (feature->id >= 0 && feature->id >= features.size())
		return true;

	return false;
}

void CFeatureHandler::AllocateNewFeatureIDs(const CFeature* feature)
{
	// if feature->id is non-negative, then allocate enough to
	// make it a valid index (we have no hard MAX_FEATURES cap)
	// and always make sure to at least double the pool
	// note: WorldObject::id is signed, so block RHS underflow
	const unsigned int numNewIDs = std::max(int(features.size()) + (128 * idPool.IsEmpty()), (feature->id + 1) - int(features.size()));

	idPool.Expand(features.size(), numNewIDs);
	features.resize(features.size() + numNewIDs, NULL);
}

void CFeatureHandler::InsertActiveFeature(CFeature* feature)
{
	idPool.AssignID(feature);

	assert(feature->id < features.size());
	assert(features[feature->id] == NULL);

	activeFeatures.insert(feature);
	features[feature->id] = feature;
}



bool CFeatureHandler::AddFeature(CFeature* feature)
{
	// LoadFeature should make sure this is true
	assert(CanAddFeature(feature->id));

	if (NeedAllocateNewFeatureIDs(feature)) {
		AllocateNewFeatureIDs(feature);
	}

	InsertActiveFeature(feature);
	SetFeatureUpdateable(feature, true);
	return true;
}


void CFeatureHandler::DeleteFeature(CFeature* feature)
{
	eventHandler.FeatureDestroyed(feature);

	toBeRemoved.push_back(feature->id);
}

CFeature* CFeatureHandler::GetFeature(int id)
{
	if (id >= 0 && id < features.size())
		return features[id];

	return NULL;
}


CFeature* CFeatureHandler::CreateWreckage(
	const FeatureLoadParams& cparams,
	const int numWreckLevels,
	bool emitSmoke)
{
	const FeatureDef* fd = cparams.featureDef;

	if (fd == NULL)
		return NULL;

	// move down the wreck-chain by <numWreckLevels> steps beyond <fd>
	for (int i = 0; i < numWreckLevels; i++) {
		if ((fd = GetFeatureDefByID(fd->deathFeatureDefID)) == NULL) {
			return NULL;
		}
	}

	if (luaRules && !luaRules->AllowFeatureCreation(fd, cparams.teamID, cparams.pos))
		return NULL;

	if (!fd->modelName.empty()) {
		FeatureLoadParams params = cparams;

		params.unitDef = ((fd->resurrectable == 0) || (numWreckLevels > 1 && fd->resurrectable < 0))? NULL: cparams.unitDef;
		params.smokeTime = fd->smokeTime * emitSmoke;
		params.featureDef = fd;

		return (LoadFeature(params));
	}

	return NULL;
}



void CFeatureHandler::Update()
{
	SCOPED_TIMER("FeatureHandler::Update");

	if ((gs->frameNum & 31) == 0) {
		for (std::list<int>::iterator it = toBeFreedFeatureIDs.begin(); it != toBeFreedFeatureIDs.end(); ) {
			if (CBuilderCAI::IsFeatureBeingReclaimed(*it)) {
				// postpone putting this ID back into the free pool
				// (this gives area-reclaimers time to choose a new
				// target with a different ID)
				++it;
			} else {
				assert(features[*it] == NULL);
				idPool.FreeID(*it, true);
				it = toBeFreedFeatureIDs.erase(it);
			}
		}
	}

	{
		GML_STDMUTEX_LOCK(rfeat); // Update

		if (!toBeRemoved.empty()) {

			GML_RECMUTEX_LOCK(obj); // Update
			eventHandler.DeleteSyncedObjects();

			GML_RECMUTEX_LOCK(feat); // Update
			eventHandler.DeleteSyncedFeatures();

			GML_RECMUTEX_LOCK(quad); // Update

			while (!toBeRemoved.empty()) {
				CFeature* feature = GetFeature(toBeRemoved.back());
				toBeRemoved.pop_back();

				if (feature) {
					toBeFreedFeatureIDs.push_back(feature->id);
					activeFeatures.erase(feature);
					features[feature->id] = NULL;

					CSolidObject::SetDeletingRefID(feature->id + unitHandler->MaxUnits());
					// destructor removes feature from update-queue
					delete feature;
					CSolidObject::SetDeletingRefID(-1);
				}
			}
		}

		eventHandler.UpdateFeatures();
	}

	CFeatureSet::iterator fi = updateFeatures.begin();

	while (fi != updateFeatures.end()) {
		CFeature* feature = *fi;
		++fi;

		assert(feature->inUpdateQue);

		if (!feature->Update()) {
			// feature is done updating itself, remove from queue
			SetFeatureUpdateable(feature, false);
		}
	}
}


void CFeatureHandler::SetFeatureUpdateable(CFeature* feature, bool updateable)
{
	if (updateable) {
		if (feature->inUpdateQue) {
			assert(updateFeatures.find(feature) != updateFeatures.end());
			return;
		}

		updateFeatures.insert(feature);
	} else {
		if (!feature->inUpdateQue) {
			assert(updateFeatures.find(feature) == updateFeatures.end());
			return;
		}

		updateFeatures.erase(feature);
	}

	feature->inUpdateQue = updateable;
}


void CFeatureHandler::TerrainChanged(int x1, int y1, int x2, int y2)
{
	const std::vector<int>& quads = quadField->GetQuadsRectangle(
		float3(x1 * SQUARE_SIZE, 0, y1 * SQUARE_SIZE),
		float3(x2 * SQUARE_SIZE, 0, y2 * SQUARE_SIZE)
	);

	for (std::vector<int>::const_iterator qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CFeature*>::const_iterator fi;
		const std::list<CFeature*>& features = quadField->GetQuad(*qi).features;

		for (fi = features.begin(); fi != features.end(); ++fi) {
			CFeature* feature = *fi;
			feature->UpdateFinalHeight(true);

			// put this feature back in the update-queue
			SetFeatureUpdateable(feature, true);
		}
	}
}

