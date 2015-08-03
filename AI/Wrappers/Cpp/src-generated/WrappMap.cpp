/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappMap.h"

#include "IncludesSources.h"

	springai::WrappMap::WrappMap(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappMap::~WrappMap() {

	}

	int springai::WrappMap::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappMap::Map* springai::WrappMap::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Map* internal_ret = NULL;
		internal_ret = new springai::WrappMap(skirmishAIId);
		return internal_ret;
	}


	int springai::WrappMap::GetChecksum() {

		int internal_ret_int;

		internal_ret_int = bridged_Map_getChecksum(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappMap::GetStartPos() {

		float return_posF3_out[3];

		bridged_Map_getStartPos(this->GetSkirmishAIId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	springai::AIFloat3 springai::WrappMap::GetMousePos() {

		float return_posF3_out[3];

		bridged_Map_getMousePos(this->GetSkirmishAIId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	bool springai::WrappMap::IsPosInCamera(const springai::AIFloat3& pos, float radius) {

		bool internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Map_isPosInCamera(this->GetSkirmishAIId(), pos_posF3, radius);
		return internal_ret_int;
	}

	int springai::WrappMap::GetWidth() {

		int internal_ret_int;

		internal_ret_int = bridged_Map_getWidth(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMap::GetHeight() {

		int internal_ret_int;

		internal_ret_int = bridged_Map_getHeight(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	std::vector<float> springai::WrappMap::GetHeightMap() {

		int heights_sizeMax;
		int heights_raw_size;
		float* heights;
		std::vector<float> heights_list;
		int heights_size;
		int internal_ret_int;

		heights_sizeMax = INT_MAX;
		heights = NULL;
		heights_size = bridged_Map_getHeightMap(this->GetSkirmishAIId(), heights, heights_sizeMax);
		heights_sizeMax = heights_size;
		heights_raw_size = heights_size;
		heights = new float[heights_raw_size];

		internal_ret_int = bridged_Map_getHeightMap(this->GetSkirmishAIId(), heights, heights_sizeMax);
		heights_list.reserve(heights_size);
		for (int i=0; i < heights_sizeMax; ++i) {
			heights_list.push_back(heights[i]);
		}
		delete[] heights;

		return heights_list;
	}

	std::vector<float> springai::WrappMap::GetCornersHeightMap() {

		int cornerHeights_sizeMax;
		int cornerHeights_raw_size;
		float* cornerHeights;
		std::vector<float> cornerHeights_list;
		int cornerHeights_size;
		int internal_ret_int;

		cornerHeights_sizeMax = INT_MAX;
		cornerHeights = NULL;
		cornerHeights_size = bridged_Map_getCornersHeightMap(this->GetSkirmishAIId(), cornerHeights, cornerHeights_sizeMax);
		cornerHeights_sizeMax = cornerHeights_size;
		cornerHeights_raw_size = cornerHeights_size;
		cornerHeights = new float[cornerHeights_raw_size];

		internal_ret_int = bridged_Map_getCornersHeightMap(this->GetSkirmishAIId(), cornerHeights, cornerHeights_sizeMax);
		cornerHeights_list.reserve(cornerHeights_size);
		for (int i=0; i < cornerHeights_sizeMax; ++i) {
			cornerHeights_list.push_back(cornerHeights[i]);
		}
		delete[] cornerHeights;

		return cornerHeights_list;
	}

	float springai::WrappMap::GetMinHeight() {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getMinHeight(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMap::GetMaxHeight() {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getMaxHeight(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	std::vector<float> springai::WrappMap::GetSlopeMap() {

		int slopes_sizeMax;
		int slopes_raw_size;
		float* slopes;
		std::vector<float> slopes_list;
		int slopes_size;
		int internal_ret_int;

		slopes_sizeMax = INT_MAX;
		slopes = NULL;
		slopes_size = bridged_Map_getSlopeMap(this->GetSkirmishAIId(), slopes, slopes_sizeMax);
		slopes_sizeMax = slopes_size;
		slopes_raw_size = slopes_size;
		slopes = new float[slopes_raw_size];

		internal_ret_int = bridged_Map_getSlopeMap(this->GetSkirmishAIId(), slopes, slopes_sizeMax);
		slopes_list.reserve(slopes_size);
		for (int i=0; i < slopes_sizeMax; ++i) {
			slopes_list.push_back(slopes[i]);
		}
		delete[] slopes;

		return slopes_list;
	}

	std::vector<int> springai::WrappMap::GetLosMap() {

		int losValues_sizeMax;
		int losValues_raw_size;
		int* losValues;
		std::vector<int> losValues_list;
		int losValues_size;
		int internal_ret_int;

		losValues_sizeMax = INT_MAX;
		losValues = NULL;
		losValues_size = bridged_Map_getLosMap(this->GetSkirmishAIId(), losValues, losValues_sizeMax);
		losValues_sizeMax = losValues_size;
		losValues_raw_size = losValues_size;
		losValues = new int[losValues_raw_size];

		internal_ret_int = bridged_Map_getLosMap(this->GetSkirmishAIId(), losValues, losValues_sizeMax);
		losValues_list.reserve(losValues_size);
		for (int i=0; i < losValues_sizeMax; ++i) {
			losValues_list.push_back(losValues[i]);
		}
		delete[] losValues;

		return losValues_list;
	}

	std::vector<int> springai::WrappMap::GetRadarMap() {

		int radarValues_sizeMax;
		int radarValues_raw_size;
		int* radarValues;
		std::vector<int> radarValues_list;
		int radarValues_size;
		int internal_ret_int;

		radarValues_sizeMax = INT_MAX;
		radarValues = NULL;
		radarValues_size = bridged_Map_getRadarMap(this->GetSkirmishAIId(), radarValues, radarValues_sizeMax);
		radarValues_sizeMax = radarValues_size;
		radarValues_raw_size = radarValues_size;
		radarValues = new int[radarValues_raw_size];

		internal_ret_int = bridged_Map_getRadarMap(this->GetSkirmishAIId(), radarValues, radarValues_sizeMax);
		radarValues_list.reserve(radarValues_size);
		for (int i=0; i < radarValues_sizeMax; ++i) {
			radarValues_list.push_back(radarValues[i]);
		}
		delete[] radarValues;

		return radarValues_list;
	}

	std::vector<int> springai::WrappMap::GetJammerMap() {

		int jammerValues_sizeMax;
		int jammerValues_raw_size;
		int* jammerValues;
		std::vector<int> jammerValues_list;
		int jammerValues_size;
		int internal_ret_int;

		jammerValues_sizeMax = INT_MAX;
		jammerValues = NULL;
		jammerValues_size = bridged_Map_getJammerMap(this->GetSkirmishAIId(), jammerValues, jammerValues_sizeMax);
		jammerValues_sizeMax = jammerValues_size;
		jammerValues_raw_size = jammerValues_size;
		jammerValues = new int[jammerValues_raw_size];

		internal_ret_int = bridged_Map_getJammerMap(this->GetSkirmishAIId(), jammerValues, jammerValues_sizeMax);
		jammerValues_list.reserve(jammerValues_size);
		for (int i=0; i < jammerValues_sizeMax; ++i) {
			jammerValues_list.push_back(jammerValues[i]);
		}
		delete[] jammerValues;

		return jammerValues_list;
	}

	std::vector<short> springai::WrappMap::GetResourceMapRaw(Resource* resource) {

		int resources_sizeMax;
		int resources_raw_size;
		short* resources;
		std::vector<short> resources_list;
		int resources_size;
		int internal_ret_int;

		int resourceId = resource->GetResourceId();
		resources_sizeMax = INT_MAX;
		resources = NULL;
		resources_size = bridged_Map_getResourceMapRaw(this->GetSkirmishAIId(), resourceId, resources, resources_sizeMax);
		resources_sizeMax = resources_size;
		resources_raw_size = resources_size;
		resources = new short[resources_raw_size];

		internal_ret_int = bridged_Map_getResourceMapRaw(this->GetSkirmishAIId(), resourceId, resources, resources_sizeMax);
		resources_list.reserve(resources_size);
		for (int i=0; i < resources_sizeMax; ++i) {
			resources_list.push_back(resources[i]);
		}
		delete[] resources;

		return resources_list;
	}

	std::vector<springai::AIFloat3> springai::WrappMap::GetResourceMapSpotsPositions(Resource* resource) {

		int spots_AposF3_sizeMax;
		int spots_AposF3_raw_size;
		float* spots_AposF3;
		std::vector<springai::AIFloat3> spots_AposF3_list;
		int spots_AposF3_size;
		int internal_ret_int;

		int resourceId = resource->GetResourceId();
		spots_AposF3_sizeMax = INT_MAX;
		spots_AposF3 = NULL;
		spots_AposF3_size = bridged_Map_getResourceMapSpotsPositions(this->GetSkirmishAIId(), resourceId, spots_AposF3, spots_AposF3_sizeMax);
		spots_AposF3_sizeMax = spots_AposF3_size;
		spots_AposF3_raw_size = spots_AposF3_size;
		if (spots_AposF3_size % 3 != 0) {
			throw std::runtime_error("returned AIFloat3 array has incorrect size, should be a multiple of 3.");
		}
		spots_AposF3_size /= 3;
		spots_AposF3 = new float[spots_AposF3_raw_size];

		internal_ret_int = bridged_Map_getResourceMapSpotsPositions(this->GetSkirmishAIId(), resourceId, spots_AposF3, spots_AposF3_sizeMax);
		spots_AposF3_list.reserve(spots_AposF3_size);
		for (int i=0; i < spots_AposF3_sizeMax; ++i) {
			spots_AposF3_list.push_back(AIFloat3(spots_AposF3[i], spots_AposF3[i+1], spots_AposF3[i+2]));
			i += 2;
		}
		delete[] spots_AposF3;

		return spots_AposF3_list;
	}

	float springai::WrappMap::GetResourceMapSpotsAverageIncome(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Map_getResourceMapSpotsAverageIncome(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappMap::GetResourceMapSpotsNearest(Resource* resource, const springai::AIFloat3& pos) {

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		float return_posF3_out[3];
		int resourceId = resource->GetResourceId();

		bridged_Map_getResourceMapSpotsNearest(this->GetSkirmishAIId(), resourceId, pos_posF3, return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	int springai::WrappMap::GetHash() {

		int internal_ret_int;

		internal_ret_int = bridged_Map_getHash(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappMap::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Map_getName(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappMap::GetHumanName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Map_getHumanName(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMap::GetElevationAt(float x, float z) {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getElevationAt(this->GetSkirmishAIId(), x, z);
		return internal_ret_int;
	}

	float springai::WrappMap::GetMaxResource(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Map_getMaxResource(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappMap::GetExtractorRadius(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Map_getExtractorRadius(this->GetSkirmishAIId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappMap::GetMinWind() {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getMinWind(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMap::GetMaxWind() {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getMaxWind(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMap::GetCurWind() {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getCurWind(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMap::GetTidalStrength() {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getTidalStrength(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMap::GetGravity() {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getGravity(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMap::GetWaterDamage() {

		float internal_ret_int;

		internal_ret_int = bridged_Map_getWaterDamage(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	std::vector<springai::Point*> springai::WrappMap::GetPoints(bool includeAllies) {

		std::vector<springai::Point*> RETURN_SIZE_list;
		int RETURN_SIZE_size;
		int internal_ret_int;

		internal_ret_int = bridged_Map_getPoints(this->GetSkirmishAIId(), includeAllies);
		RETURN_SIZE_size = internal_ret_int;
		RETURN_SIZE_list.reserve(RETURN_SIZE_size);
		for (int i=0; i < RETURN_SIZE_size; ++i) {
			RETURN_SIZE_list.push_back(springai::WrappPoint::GetInstance(skirmishAIId, i));
		}

		return RETURN_SIZE_list;
	}

	std::vector<springai::Line*> springai::WrappMap::GetLines(bool includeAllies) {

		std::vector<springai::Line*> RETURN_SIZE_list;
		int RETURN_SIZE_size;
		int internal_ret_int;

		internal_ret_int = bridged_Map_getLines(this->GetSkirmishAIId(), includeAllies);
		RETURN_SIZE_size = internal_ret_int;
		RETURN_SIZE_list.reserve(RETURN_SIZE_size);
		for (int i=0; i < RETURN_SIZE_size; ++i) {
			RETURN_SIZE_list.push_back(springai::WrappLine::GetInstance(skirmishAIId, i));
		}

		return RETURN_SIZE_list;
	}

	bool springai::WrappMap::IsPossibleToBuildAt(UnitDef* unitDef, const springai::AIFloat3& pos, int facing) {

		bool internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		int unitDefId = unitDef->GetUnitDefId();

		internal_ret_int = bridged_Map_isPossibleToBuildAt(this->GetSkirmishAIId(), unitDefId, pos_posF3, facing);
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappMap::FindClosestBuildSite(UnitDef* unitDef, const springai::AIFloat3& pos, float searchRadius, int minDist, int facing) {

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		float return_posF3_out[3];
		int unitDefId = unitDef->GetUnitDefId();

		bridged_Map_findClosestBuildSite(this->GetSkirmishAIId(), unitDefId, pos_posF3, searchRadius, minDist, facing, return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	springai::Drawer* springai::WrappMap::GetDrawer() {

		Drawer* internal_ret_int_out;

		internal_ret_int_out = springai::WrappDrawer::GetInstance(skirmishAIId);

		return internal_ret_int_out;
	}
