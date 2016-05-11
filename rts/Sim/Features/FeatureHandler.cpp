/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureHandler.h"

#include "FeatureDef.h"
#include "FeatureDefHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "System/creg/STL_Set.h"
#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"

CFeatureHandler* featureHandler = NULL;

/******************************************************************************/

CR_BIND(CFeatureHandler, )
CR_REG_METADATA(CFeatureHandler, (
	CR_MEMBER(idPool),
	CR_MEMBER(toBeFreedFeatureIDs),
	CR_MEMBER(activeFeatures),
	CR_MEMBER(features),
	CR_MEMBER(updateFeatures)
))

/******************************************************************************/

CFeatureHandler::~CFeatureHandler()
{
	for (CFeatureSet::iterator fi = activeFeatures.begin(); fi != activeFeatures.end(); ++fi) {
		delete *fi;
	}

	activeFeatures.clear();
	features.clear();
}


void CFeatureHandler::LoadFeaturesFromMap()
{
	// create map-specified feature instances
	const int numFeatures = readMap->GetNumFeatures();
	if (numFeatures == 0)
		return;
	std::vector<MapFeatureInfo> mfi;
	mfi.resize(numFeatures);
	readMap->GetFeatureInfo(&mfi[0]);

	for (int a = 0; a < numFeatures; ++a) {
		const FeatureDef* def = featureDefHandler->GetFeatureDef(readMap->GetFeatureTypeName(mfi[a].featureType), true);
		if (def == nullptr)
			continue;

		FeatureLoadParams params = {
			def,
			NULL,

			float3(mfi[a].pos.x, CGround::GetHeightReal(mfi[a].pos.x, mfi[a].pos.z), mfi[a].pos.z),
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
}


CFeature* CFeatureHandler::LoadFeature(const FeatureLoadParams& params) {
	// need to check this BEFORE creating the instance
	if (!CanAddFeature(params.featureID))
		return nullptr;

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
	SetFeatureUpdateable(feature);
	return true;
}


void CFeatureHandler::DeleteFeature(CFeature* feature)
{
	SetFeatureUpdateable(feature);
	feature->deleteMe = true;
}

CFeature* CFeatureHandler::GetFeature(int id)
{
	if (id >= 0 && id < features.size())
		return features[id];

	return nullptr;
}


CFeature* CFeatureHandler::CreateWreckage(
	const FeatureLoadParams& cparams,
	const int numWreckLevels,
	bool emitSmoke)
{
	const FeatureDef* fd = cparams.featureDef;

	if (fd == nullptr)
		return nullptr;

	// move down the wreck-chain by <numWreckLevels> steps beyond <fd>
	for (int i = 0; i < numWreckLevels; i++) {
		if ((fd = featureDefHandler->GetFeatureDefByID(fd->deathFeatureDefID)) == nullptr) {
			return nullptr;
		}
	}

	if (!eventHandler.AllowFeatureCreation(fd, cparams.teamID, cparams.pos))
		return nullptr;

	if (!fd->modelName.empty()) {
		FeatureLoadParams params = cparams;

		params.unitDef = ((fd->resurrectable == 0) || (numWreckLevels > 0 && fd->resurrectable < 0)) ? nullptr: cparams.unitDef;
		params.smokeTime = fd->smokeTime * emitSmoke;
		params.featureDef = fd;

		return (LoadFeature(params));
	}

	return nullptr;
}



void CFeatureHandler::Update()
{
	SCOPED_TIMER("FeatureHandler::Update");

	if ((gs->frameNum & 31) == 0) {
		toBeFreedFeatureIDs.erase(std::remove_if(toBeFreedFeatureIDs.begin(), toBeFreedFeatureIDs.end(),
			[this](int id) { return this->TryFreeFeatureID(id); }
		), toBeFreedFeatureIDs.end());
	}

	updateFeatures.erase(std::remove_if(updateFeatures.begin(), updateFeatures.end(), 
		[this](CFeature* feature) { return this->UpdateFeature(feature); }
	), updateFeatures.end());
}


bool CFeatureHandler::TryFreeFeatureID(int id)
{
	if (CBuilderCAI::IsFeatureBeingReclaimed(id)) {
		// postpone putting this ID back into the free pool
		// (this gives area-reclaimers time to choose a new
		// target with a different ID)
		return false;
	}

	assert(features[id] == nullptr);
	idPool.FreeID(id, true);

	return true;
}


bool CFeatureHandler::UpdateFeature(CFeature* feature)
{
	assert(feature->inUpdateQue);

	if (feature->deleteMe) {
		eventHandler.RenderFeatureDestroyed(feature);
		eventHandler.FeatureDestroyed(feature);
		toBeFreedFeatureIDs.push_back(feature->id);
		activeFeatures.erase(feature);
		features[feature->id] = NULL;

		// ID must match parameter for object commands, just use this
		CSolidObject::SetDeletingRefID(feature->GetBlockingMapID());
		// destructor removes feature from update-queue
		delete feature;
		CSolidObject::SetDeletingRefID(-1);

		return true;
	}

	if (!feature->Update()) {
		// feature is done updating itself, remove from queue
		feature->inUpdateQue = false;

		return true;
	}

	return false;
}


void CFeatureHandler::SetFeatureUpdateable(CFeature* feature)
{
	if (feature->inUpdateQue) {
		assert(std::find(updateFeatures.begin(), updateFeatures.end(), feature) != updateFeatures.end());
		return;
	}

	// always true
	feature->inUpdateQue = VectorInsertUnique(updateFeatures, feature);
}


void CFeatureHandler::TerrainChanged(int x1, int y1, int x2, int y2)
{
	const float3 mins(x1 * SQUARE_SIZE, 0, y1 * SQUARE_SIZE);
	const float3 maxs(x2 * SQUARE_SIZE, 0, y2 * SQUARE_SIZE);

	const auto& quads = quadField->GetQuadsRectangle(mins, maxs);

	for (const int qi: quads) {
		for (CFeature* f: quadField->GetQuad(qi).features) {
			// put this feature back in the update-queue
			SetFeatureUpdateable(f);
		}
	}
}

