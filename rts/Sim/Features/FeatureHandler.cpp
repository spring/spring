/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"
#include "FeatureHandler.h"

#include "Game/Game.h"
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


using std::list;
using std::map;
using std::string;
using std::vector;


CFeatureHandler* featureHandler = NULL;


/******************************************************************************/

CR_BIND(CFeatureHandler, );

CR_REG_METADATA(CFeatureHandler, (

//	CR_MEMBER(featureDefs),
//	CR_MEMBER(featureDefsVector),

	CR_MEMBER(freeIDs),
	CR_MEMBER(toBeFreedIDs),
	CR_MEMBER(activeFeatures),
	CR_MEMBER(features),

	CR_MEMBER(toBeRemoved),
	CR_MEMBER(updateFeatures),

	CR_RESERVED(128)
));

/******************************************************************************/

CFeatureHandler::CFeatureHandler()
{
	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("FeatureDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading FeatureDefs");
	}

	// featureDefIDs start with 1
	featureDefsVector.push_back(NULL);

	// get most of the feature defs (missing trees and geovent from the map)
	vector<string> keys;
	rootTable.GetKeys(keys);
	for (int i = 0; i < (int)keys.size(); i++) {
		const string& nameMixedCase = keys[i];
		const string& nameLowerCase = StringToLower(nameMixedCase);
		const LuaTable& fdTable = rootTable.SubTable(nameMixedCase);

		AddFeatureDef(nameLowerCase, CreateFeatureDef(fdTable, nameLowerCase));
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



void CFeatureHandler::AddFeatureDef(const string& name, FeatureDef* fd)
{
	if (fd == NULL)
		return;

	map<string, const FeatureDef*>::const_iterator it = featureDefs.find(name);

	if (it != featureDefs.end()) {
		featureDefsVector[it->second->id] = fd;
	} else {
		fd->id = featureDefsVector.size();
		featureDefsVector.push_back(fd);
	}

	featureDefs[name] = fd;
}


FeatureDef* CFeatureHandler::CreateFeatureDef(const LuaTable& fdTable, const string& mixedCase) const
{
	const string& name = StringToLower(mixedCase);
	if (featureDefs.find(name) != featureDefs.end()) {
		return NULL;
	}

	FeatureDef* fd = new FeatureDef();

	fd->name = name;
	fd->description = fdTable.GetString("description", "");

	fd->blocking      =  fdTable.GetBool("blocking",        true);
	fd->burnable      =  fdTable.GetBool("flammable",       false);
	fd->destructable  = !fdTable.GetBool("indestructible",  false);
	fd->reclaimable   =  fdTable.GetBool("reclaimable",     fd->destructable);
	fd->autoreclaim   =  fdTable.GetBool("autoreclaimable", fd->reclaimable);
	fd->resurrectable =  fdTable.GetInt("resurrectable",    -1);
	fd->geoThermal    =  fdTable.GetBool("geoThermal",      false);

	// this seem to be the closest thing to floating that ta wreckage contains
	//FIXME move this to featuredef_post.lua
	fd->floating = fdTable.GetBool("nodrawundergray", false);
	fd->floating = fd->floating && !fd->blocking;
	fd->floating = fdTable.GetBool("floating", fd->floating);

	fd->noSelect = fdTable.GetBool("noselect", false);

	fd->deathFeature = fdTable.GetString("featureDead", "");

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

	fd->xsize = std::max(1 * 2, fdTable.GetInt("footprintX", 1) * 2);
	fd->zsize = std::max(1 * 2, fdTable.GetInt("footprintZ", 1) * 2);

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
	fd->blocking = true;
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
	fd->blocking = false;
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
	fd->collisionVolume->SetIgnoreHits(true);
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
	const int numFeatureTypes = readmap->GetNumFeatureTypes();

	for (int a = 0; a < numFeatureTypes; ++a) {
		const string& name = StringToLower(readmap->GetFeatureTypeName(a));

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
		const int numFeatures = readmap->GetNumFeatures();
		MapFeatureInfo* mfi = new MapFeatureInfo[numFeatures];
		readmap->GetFeatureInfo(mfi);

		for (int a = 0; a < numFeatures; ++a) {
			const string& name = StringToLower(readmap->GetFeatureTypeName(mfi[a].featureType));
			map<string, const FeatureDef*>::iterator def = featureDefs.find(name);

			if (def == featureDefs.end()) {
				LOG_L(L_ERROR, "Unknown feature named '%s'", name.c_str());
				continue;
			}

			const float ypos = ground->GetHeightReal(mfi[a].pos.x, mfi[a].pos.z);
			const float3 fpos = float3(mfi[a].pos.x, ypos, mfi[a].pos.z);
			const FeatureDef* fdef = def->second;

			CFeature* f = new CFeature();
			f->Initialize(fpos, fdef, (short int) mfi[a].rotation, 0, -1, -1, NULL);
		}

		delete[] mfi;
	}
}


int CFeatureHandler::AddFeature(CFeature* feature)
{
	if (freeIDs.empty()) {
		// alloc n new ids and randomly insert to freeIDs
		static const unsigned n = 100;

		std::vector<int> newIds(n);

		for (unsigned i = 0; i < n; ++i)
			newIds[i] = i + features.size();

		features.resize(features.size() + n, NULL);

		SyncedRNG rng;
		std::random_shuffle(newIds.begin(), newIds.end(), rng); // synced
		std::copy(newIds.begin(), newIds.end(), std::back_inserter(freeIDs));
	}
	
	feature->id = freeIDs.front();
	freeIDs.pop_front();
	activeFeatures.insert(feature);
	features[feature->id] = feature;
	SetFeatureUpdateable(feature);

	eventHandler.FeatureCreated(feature);
	return feature->id;
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
	else
		return 0;
}


CFeature* CFeatureHandler::CreateWreckage(const float3& pos, const string& name,
	float rot, int facing, int iter, int team, int allyteam, bool emitSmoke, const UnitDef* udef,
	const float3& speed)
{
	const FeatureDef* fd;
	const string* defname = &name;

	int i = iter;
	do {
		if (defname->empty()) return NULL;
		fd = GetFeatureDef(*defname);
		if (!fd) return NULL;
		defname = &(fd->deathFeature);
	} while (--i > 0);

	if (luaRules && !luaRules->AllowFeatureCreation(fd, team, pos))
		return NULL;

	if (!fd->modelName.empty()) {
		CFeature* f = new CFeature();

		if (fd->resurrectable == 0 || (iter > 1 && fd->resurrectable < 0)) {
			f->Initialize(pos, fd, (short int) rot, facing, team, allyteam, NULL, speed, emitSmoke ? fd->smokeTime : 0);
		} else {
			f->Initialize(pos, fd, (short int) rot, facing, team, allyteam, udef, speed, emitSmoke ? fd->smokeTime : 0);
		}

		return f;
	}
	return NULL;
}



void CFeatureHandler::Update()
{
	SCOPED_TIMER("FeatureHandler::Update");

	if ((gs->frameNum & 31) == 0) {
		// let all areareclaimers choose a target with a different id
		bool dontClear = false;
		for (list<int>::iterator it = toBeFreedIDs.begin(); it != toBeFreedIDs.end(); ++it) {
			if (CBuilderCAI::IsFeatureBeingReclaimed(*it)) {
				// postpone recycling
				dontClear = true;
				break;
			}
		}
		if (!dontClear)
			freeIDs.splice(freeIDs.end(), toBeFreedIDs, toBeFreedIDs.begin(), toBeFreedIDs.end());
	}

	{
		GML_STDMUTEX_LOCK(rfeat); // Update

		if(!toBeRemoved.empty()) {

			GML_RECMUTEX_LOCK(obj); // Update

			eventHandler.DeleteSyncedObjects();

			GML_RECMUTEX_LOCK(feat); // Update

			eventHandler.DeleteSyncedFeatures();

			GML_RECMUTEX_LOCK(quad); // Update

			while (!toBeRemoved.empty()) {
				CFeature* feature = GetFeature(toBeRemoved.back());
				toBeRemoved.pop_back();
				if (feature) {
					int delID = feature->id;
					toBeFreedIDs.push_back(delID);
					activeFeatures.erase(feature);
					features[delID] = 0;

					if (feature->inUpdateQue) {
						updateFeatures.erase(feature);
					}
					CSolidObject::SetDeletingRefID(delID + uh->MaxUnits());
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

		if (!feature->Update()) {
			// remove it
			feature->inUpdateQue = false;
			updateFeatures.erase(feature);
		}
	}
}


void CFeatureHandler::SetFeatureUpdateable(CFeature* feature)
{
	if (feature->inUpdateQue) {
		return;
	}
	updateFeatures.insert(feature);
	feature->inUpdateQue = true;
}


void CFeatureHandler::TerrainChanged(int x1, int y1, int x2, int y2)
{
	const vector<int> &quads = qf->GetQuadsRectangle(float3(x1 * SQUARE_SIZE, 0, y1 * SQUARE_SIZE),
		float3(x2 * SQUARE_SIZE, 0, y2 * SQUARE_SIZE));

	for (vector<int>::const_iterator qi = quads.begin(); qi != quads.end(); ++qi) {
		list<CFeature*>::const_iterator fi;
		const list<CFeature*>& features = qf->GetQuad(*qi).features;

		for (fi = features.begin(); fi != features.end(); ++fi) {
			CFeature* feature = *fi;
			float3& fpos = feature->pos;
			float gh = ground->GetHeightReal(fpos.x, fpos.z);
			float wh = gh;
			if(feature->def->floating)
				wh = ground->GetHeightAboveWater(fpos.x, fpos.z);
			if (fpos.y > wh || fpos.y < gh) {
				feature->finalHeight = wh;
				feature->reachedFinalPos = false;
				SetFeatureUpdateable(feature);
			}
		}
	}
}
