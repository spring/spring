/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureHandler.h"
#include "Feature.h"
#include "FeatureDef.h"
#include "FeatureDefHandler.h"
#include "FeatureMemPool.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "System/creg/STL_Set.h"
#include "System/EventHandler.h"
#include "System/TimeProfiler.h"

/******************************************************************************/

CR_BIND(CFeatureHandler, )
CR_REG_METADATA(CFeatureHandler, (
	CR_MEMBER(idPool),
	CR_MEMBER(deletedFeatureIDs),
	CR_MEMBER(activeFeatureIDs),
	CR_MEMBER(features),
	CR_MEMBER(updateFeatures)
))

/******************************************************************************/

FeatureMemPool featureMemPool;

CFeatureHandler featureHandler;


void CFeatureHandler::Init() {
	features.resize(MAX_FEATURES, nullptr);
	activeFeatureIDs.reserve(MAX_FEATURES); // internal table size must be constant
	featureMemPool.reserve(128);

	idPool.Clear();
	idPool.Expand(0, MAX_FEATURES);
}

void CFeatureHandler::Kill() {
	for (const int featureID: activeFeatureIDs) {
		featureMemPool.free(features[featureID]);
	}

	// do not clear in ctor because creg-loaded objects would be wiped out
	featureMemPool.clear();

	activeFeatureIDs.clear();
	deletedFeatureIDs.clear();
	features.clear();
	updateFeatures.clear();
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
			nullptr,

			float3(mfi[a].pos.x, CGround::GetHeightReal(mfi[a].pos.x, mfi[a].pos.z), mfi[a].pos.z),
			ZeroVector,

			-1, // featureID
			-1, // teamID
			-1, // allyTeamID

			static_cast<short int>(mfi[a].rotation),
			FACING_SOUTH,

			0, // wreckLevels
			0, // smokeTime
		};

		LoadFeature(params);
	}
}


CFeature* CFeatureHandler::LoadFeature(const FeatureLoadParams& params) {
	// need to check this BEFORE creating the instance
	if (!CanAddFeature(params.featureID))
		return nullptr;

	CFeature* feature = featureMemPool.alloc<CFeature>();

	// calls back into AddFeature
	feature->Initialize(params);
	return feature;
}


void CFeatureHandler::InsertActiveFeature(CFeature* feature)
{
	idPool.AssignID(feature);

	assert(feature->id < features.size());
	assert(features[feature->id] == nullptr);

	activeFeatureIDs.insert(feature->id);
	features[feature->id] = feature;
}



bool CFeatureHandler::AddFeature(CFeature* feature)
{
	// LoadFeature should make sure this is true
	assert(CanAddFeature(feature->id));

	InsertActiveFeature(feature);
	SetFeatureUpdateable(feature);
	return true;
}


void CFeatureHandler::DeleteFeature(CFeature* feature)
{
	SetFeatureUpdateable(feature);
	feature->deleteMe = true;
}


CFeature* CFeatureHandler::CreateWreckage(const FeatureLoadParams& cparams)
{
	const FeatureDef* fd = cparams.featureDef;

	if (fd == nullptr)
		return nullptr;

	// move down the wreck-chain by <numWreckLevels> steps beyond <fd>
	for (int i = 0; i < cparams.wreckLevels; i++) {
		if ((fd = featureDefHandler->GetFeatureDefByID(fd->deathFeatureDefID)) == nullptr)
			return nullptr;
	}

	if (!eventHandler.AllowFeatureCreation(fd, cparams.teamID, cparams.pos))
		return nullptr;

	if (!fd->modelName.empty()) {
		FeatureLoadParams params = cparams;

		params.unitDef = ((fd->resurrectable == 0) || (cparams.wreckLevels > 0 && fd->resurrectable < 0))? nullptr: cparams.unitDef;
		params.featureDef = fd;

		// for the CreateWreckage call, params.smokeTime acts as a multiplier
		params.smokeTime = fd->smokeTime * cparams.smokeTime;

		return (LoadFeature(params));
	}

	return nullptr;
}



void CFeatureHandler::Update()
{
	SCOPED_TIMER("Sim::Features");

	if ((gs->frameNum & 31) == 0) {
		const auto& pred = [this](int id) { return (this->TryFreeFeatureID(id)); };
		const auto& iter = std::remove_if(deletedFeatureIDs.begin(), deletedFeatureIDs.end(), pred);

		deletedFeatureIDs.erase(iter, deletedFeatureIDs.end());
	}
	{
		const auto& pred = [this](CFeature* feature) { return (this->UpdateFeature(feature)); };
		const auto& iter = std::remove_if(updateFeatures.begin(), updateFeatures.end(), pred);

		updateFeatures.erase(iter, updateFeatures.end());
	}
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

		deletedFeatureIDs.push_back(feature->id);
		activeFeatureIDs.erase(feature->id);

		features[feature->id] = nullptr;

		// ID must match parameter for object commands, just use this
		CSolidObject::SetDeletingRefID(feature->GetBlockingMapID());
		// destructor removes feature from update-queue
		featureMemPool.free(feature);
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
	feature->inUpdateQue = spring::VectorInsertUnique(updateFeatures, feature);
}


void CFeatureHandler::TerrainChanged(int x1, int y1, int x2, int y2)
{
	const float3 mins(x1 * SQUARE_SIZE, 0, y1 * SQUARE_SIZE);
	const float3 maxs(x2 * SQUARE_SIZE, 0, y2 * SQUARE_SIZE);

	QuadFieldQuery qfQuery;
	quadField.GetQuadsRectangle(qfQuery, mins, maxs);

	for (const int qi: *qfQuery.quads) {
		for (CFeature* f: quadField.GetQuad(qi).features) {
			// put this feature back in the update-queue
			SetFeatureUpdateable(f);
		}
	}
}

