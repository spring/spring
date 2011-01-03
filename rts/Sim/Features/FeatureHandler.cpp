/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
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
#include "System/LogOutput.h"
#include "System/TimeProfiler.h"
#include "creg/STL_Set.h"


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

	//! featureDefIDs start with 1
	featureDefsVector.push_back(NULL);

	//! get most of the feature defs (missing trees and geovent from the map)
	vector<string> keys;
	rootTable.GetKeys(keys);
	for (int i = 0; i < (int)keys.size(); i++) {
		const string& name = keys[i];
		const LuaTable fdTable = rootTable.SubTable(name);
		CreateFeatureDef(fdTable, name);
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
	map<string, const FeatureDef*>::const_iterator it = featureDefs.find(name);

	if (it != featureDefs.end()) {
		featureDefsVector[it->second->id] = fd;
	} else {
		fd->id = featureDefsVector.size();
		featureDefsVector.push_back(fd);
	}
	featureDefs[name] = fd;
}


void CFeatureHandler::CreateFeatureDef(const LuaTable& fdTable, const string& mixedCase)
{
	const string name = StringToLower(mixedCase);
	if (featureDefs.find(name) != featureDefs.end()) {
		return;
	}

	FeatureDef* fd = new FeatureDef;

	fd->myName = name;

	fd->filename = fdTable.GetString("filename", "unknown");

	fd->description = fdTable.GetString("description", "");

	fd->blocking      =  fdTable.GetBool("blocking",       true);
	fd->burnable      =  fdTable.GetBool("flammable",      false);
	fd->destructable  = !fdTable.GetBool("indestructible", false);
	fd->reclaimable   =  fdTable.GetBool("reclaimable",    fd->destructable);
	fd->autoreclaim   =  fdTable.GetBool("autoreclaimable",    fd->autoreclaim);
	fd->resurrectable =  fdTable.GetInt("resurrectable",   -1);

	//this seem to be the closest thing to floating that ta wreckage contains
	fd->floating = fdTable.GetBool("nodrawundergray", false);
	if (fd->floating && !fd->blocking) {
		fd->floating = false;
	}

	fd->noSelect = fdTable.GetBool("noselect", false);

	fd->deathFeature = fdTable.GetString("featureDead", "");

	fd->metal       = fdTable.GetFloat("metal",  0.0f);
	fd->energy      = fdTable.GetFloat("energy", 0.0f);
	fd->maxHealth   = fdTable.GetFloat("damage", 0.0f);
	fd->reclaimTime = std::max(1.0f, fdTable.GetFloat("reclaimTime", (fd->metal + fd->energy) * 6.0f));

	fd->smokeTime = fdTable.GetInt("smokeTime", 300);

	fd->drawType = fdTable.GetInt("drawType", DRAWTYPE_MODEL);
	fd->modelname = fdTable.GetString("object", "");
	if (!fd->modelname.empty()) {
		if (fd->modelname.find(".") == string::npos) {
			fd->modelname += ".3do";
		}
		fd->modelname = string("objects3d/") + fd->modelname;
	}


	// initialize the (per-featuredef) collision-volume,
	// all CFeature instances hold a copy of this object
	//
	// takes precedence over the old sphere tags as well
	// as feature->radius (for feature <---> projectile
	// interactions)
	fd->collisionVolume = new CollisionVolume(
		fdTable.GetString("collisionVolumeType", ""),
		fdTable.GetFloat3("collisionVolumeScales", ZeroVector),
		fdTable.GetFloat3("collisionVolumeOffsets", ZeroVector),
		fdTable.GetInt("collisionVolumeTest", CollisionVolume::COLVOL_HITTEST_CONT)
	);


	fd->upright = fdTable.GetBool("upright", false);

	// our resolution is double TA's
	fd->xsize = fdTable.GetInt("footprintX", 1) * 2;
	fd->zsize = fdTable.GetInt("footprintZ", 1) * 2;

	const float defMass = (fd->metal * 0.4f) + (fd->maxHealth * 0.1f);
	fd->mass = fdTable.GetFloat("mass", defMass);
	fd->mass = std::max(0.001f, fd->mass);

	// custom parameters table
	fdTable.SubTable("customParams").GetMap(fd->customParams);

	AddFeatureDef(name, fd);
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

	if (showError)
		logOutput.Print("Couldnt find wreckage info %s", name.c_str());

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
	//! add map's featureDefs
	int numType = readmap->GetNumFeatureTypes ();
	for (int a = 0; a < numType; ++a) {
		const string name = StringToLower(readmap->GetFeatureTypeName(a));

		if (GetFeatureDef(name, false) == NULL) {
			if (name.find("treetype") != string::npos) {
				FeatureDef* fd = new FeatureDef;
				fd->blocking = 1;
				fd->burnable = true;
				fd->destructable = 1;
				fd->reclaimable = true;
				fd->drawType = DRAWTYPE_TREE + atoi(name.substr(8).c_str());
				fd->energy = 250;
				fd->metal = 0;
				fd->reclaimTime = 1500;
				fd->maxHealth = 5;
				fd->xsize = 2;
				fd->zsize = 2;
				fd->myName = name;
				fd->description = "Tree";
				fd->mass = 20;
				fd->collisionVolume = new CollisionVolume("", ZeroVector, ZeroVector, CollisionVolume::COLVOL_HITTEST_DISC);
				AddFeatureDef(name, fd);
			}
			else if (name.find("geovent") != string::npos) {
				FeatureDef* fd = new FeatureDef;
				fd->blocking = 0;
				fd->burnable = 0;
				fd->destructable = 0;
				fd->reclaimable = false;
				fd->geoThermal = true;
				// geos are drawn into the ground texture and emit smoke to be visible
				fd->drawType = DRAWTYPE_NONE;
				fd->energy = 0;
				fd->metal = 0;
				fd->reclaimTime = 0;
				fd->maxHealth = 0;
				fd->xsize = 0;
				fd->zsize = 0;
				fd->myName = name;
				fd->mass = CSolidObject::DEFAULT_MASS;
				// geothermals have no collision volume at all
				fd->collisionVolume = NULL;
				AddFeatureDef(name, fd);
			}
			else {
				logOutput.Print("Unknown map feature type %s", name.c_str());
			}
		}
	}

	if (!onlyCreateDefs) {
		//! create map features
		const int numFeatures = readmap->GetNumFeatures();
		MapFeatureInfo* mfi = new MapFeatureInfo[numFeatures];
		readmap->GetFeatureInfo(mfi);

		for(int a = 0; a < numFeatures; ++a) {
			const string name = StringToLower(readmap->GetFeatureTypeName(mfi[a].featureType));
			map<string, const FeatureDef*>::iterator def = featureDefs.find(name);

			if (def == featureDefs.end()) {
				logOutput.Print("Unknown feature named '%s'", name.c_str());
				continue;
			}

			const float ypos = ground->GetHeightReal(mfi[a].pos.x, mfi[a].pos.z);
			(new CFeature)->Initialize(
				float3(mfi[a].pos.x, ypos, mfi[a].pos.z),
				def->second, (short int) mfi[a].rotation,
				0, -1, -1, ""
			);
		}

		delete[] mfi;
	}
}


int CFeatureHandler::AddFeature(CFeature* feature)
{
	if (freeIDs.empty()) {
		// alloc n new ids and randomly insert to freeIDs
		const unsigned n = 100;
		std::vector<int> newIds(n);
		for (unsigned i = 0; i < n; ++i)
			newIds[i] = i + features.size();
		SyncedRNG rng;
		std::random_shuffle(newIds.begin(), newIds.end(), rng); // synced
		features.resize(features.size()+n, 0);
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
	float rot, int facing, int iter, int team, int allyteam, bool emitSmoke, string fromUnit,
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
	}while (--i > 0);

	if (luaRules && !luaRules->AllowFeatureCreation(fd, team, pos))
		return NULL;

	if (!fd->modelname.empty()) {
		if (fd->resurrectable==0 || (iter>1 && fd->resurrectable<0))
			fromUnit = "";

		CFeature* f = new CFeature;
		f->Initialize(pos, fd, (short int) rot, facing, team, allyteam, fromUnit, speed, emitSmoke ? fd->smokeTime : 0);

		return f;
	}
	return NULL;
}



void CFeatureHandler::Update()
{
	SCOPED_TIMER("Feature Update");

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

			GML_RECMUTEX_LOCK(feat); // Update
			GML_RECMUTEX_LOCK(quad); // Update

			eventHandler.DeleteSyncedFeatures();

			while (!toBeRemoved.empty()) {
				CFeature* feature = GetFeature(toBeRemoved.back());
				toBeRemoved.pop_back();
				if (feature) {
					toBeFreedIDs.push_back(feature->id);
					activeFeatures.erase(feature);
					features[feature->id] = 0;

					if (feature->inUpdateQue) {
						updateFeatures.erase(feature);
					}

					delete feature;
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
