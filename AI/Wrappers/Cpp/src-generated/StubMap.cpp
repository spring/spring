/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubMap.h"

#include "IncludesSources.h"

	springai::StubMap::~StubMap() {}
	void springai::StubMap::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubMap::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubMap::SetChecksum(int checksum) {
		this->checksum = checksum;
	}
	int springai::StubMap::GetChecksum() {
		return checksum;
	}

	void springai::StubMap::SetStartPos(springai::AIFloat3 startPos) {
		this->startPos = startPos;
	}
	springai::AIFloat3 springai::StubMap::GetStartPos() {
		return startPos;
	}

	void springai::StubMap::SetMousePos(springai::AIFloat3 mousePos) {
		this->mousePos = mousePos;
	}
	springai::AIFloat3 springai::StubMap::GetMousePos() {
		return mousePos;
	}

	bool springai::StubMap::IsPosInCamera(const springai::AIFloat3& pos, float radius) {
		return false;
	}

	void springai::StubMap::SetWidth(int width) {
		this->width = width;
	}
	int springai::StubMap::GetWidth() {
		return width;
	}

	void springai::StubMap::SetHeight(int height) {
		this->height = height;
	}
	int springai::StubMap::GetHeight() {
		return height;
	}

	void springai::StubMap::SetHeightMap(std::vector<float> heightMap) {
		this->heightMap = heightMap;
	}
	std::vector<float> springai::StubMap::GetHeightMap() {
		return heightMap;
	}

	void springai::StubMap::SetCornersHeightMap(std::vector<float> cornersHeightMap) {
		this->cornersHeightMap = cornersHeightMap;
	}
	std::vector<float> springai::StubMap::GetCornersHeightMap() {
		return cornersHeightMap;
	}

	void springai::StubMap::SetMinHeight(float minHeight) {
		this->minHeight = minHeight;
	}
	float springai::StubMap::GetMinHeight() {
		return minHeight;
	}

	void springai::StubMap::SetMaxHeight(float maxHeight) {
		this->maxHeight = maxHeight;
	}
	float springai::StubMap::GetMaxHeight() {
		return maxHeight;
	}

	void springai::StubMap::SetSlopeMap(std::vector<float> slopeMap) {
		this->slopeMap = slopeMap;
	}
	std::vector<float> springai::StubMap::GetSlopeMap() {
		return slopeMap;
	}

	void springai::StubMap::SetLosMap(std::vector<int> losMap) {
		this->losMap = losMap;
	}
	std::vector<int> springai::StubMap::GetLosMap() {
		return losMap;
	}

	void springai::StubMap::SetRadarMap(std::vector<int> radarMap) {
		this->radarMap = radarMap;
	}
	std::vector<int> springai::StubMap::GetRadarMap() {
		return radarMap;
	}

	void springai::StubMap::SetJammerMap(std::vector<int> jammerMap) {
		this->jammerMap = jammerMap;
	}
	std::vector<int> springai::StubMap::GetJammerMap() {
		return jammerMap;
	}

	std::vector<short> springai::StubMap::GetResourceMapRaw(Resource* resource) {
		return std::vector<short>();
	}

	std::vector<springai::AIFloat3> springai::StubMap::GetResourceMapSpotsPositions(Resource* resource) {
		return std::vector<springai::AIFloat3>();
	}

	float springai::StubMap::GetResourceMapSpotsAverageIncome(Resource* resource) {
		return 0;
	}

	springai::AIFloat3 springai::StubMap::GetResourceMapSpotsNearest(Resource* resource, const springai::AIFloat3& pos) {
		return springai::AIFloat3::NULL_VALUE;
	}

	void springai::StubMap::SetHash(int hash) {
		this->hash = hash;
	}
	int springai::StubMap::GetHash() {
		return hash;
	}

	void springai::StubMap::SetName(const char* name) {
		this->name = name;
	}
	const char* springai::StubMap::GetName() {
		return name;
	}

	void springai::StubMap::SetHumanName(const char* humanName) {
		this->humanName = humanName;
	}
	const char* springai::StubMap::GetHumanName() {
		return humanName;
	}

	float springai::StubMap::GetElevationAt(float x, float z) {
		return 0;
	}

	float springai::StubMap::GetMaxResource(Resource* resource) {
		return 0;
	}

	float springai::StubMap::GetExtractorRadius(Resource* resource) {
		return 0;
	}

	void springai::StubMap::SetMinWind(float minWind) {
		this->minWind = minWind;
	}
	float springai::StubMap::GetMinWind() {
		return minWind;
	}

	void springai::StubMap::SetMaxWind(float maxWind) {
		this->maxWind = maxWind;
	}
	float springai::StubMap::GetMaxWind() {
		return maxWind;
	}

	void springai::StubMap::SetCurWind(float curWind) {
		this->curWind = curWind;
	}
	float springai::StubMap::GetCurWind() {
		return curWind;
	}

	void springai::StubMap::SetTidalStrength(float tidalStrength) {
		this->tidalStrength = tidalStrength;
	}
	float springai::StubMap::GetTidalStrength() {
		return tidalStrength;
	}

	void springai::StubMap::SetGravity(float gravity) {
		this->gravity = gravity;
	}
	float springai::StubMap::GetGravity() {
		return gravity;
	}

	void springai::StubMap::SetWaterDamage(float waterDamage) {
		this->waterDamage = waterDamage;
	}
	float springai::StubMap::GetWaterDamage() {
		return waterDamage;
	}

	std::vector<springai::Point*> springai::StubMap::GetPoints(bool includeAllies) {
		return std::vector<springai::Point*>();
	}

	std::vector<springai::Line*> springai::StubMap::GetLines(bool includeAllies) {
		return std::vector<springai::Line*>();
	}

	bool springai::StubMap::IsPossibleToBuildAt(UnitDef* unitDef, const springai::AIFloat3& pos, int facing) {
		return false;
	}

	springai::AIFloat3 springai::StubMap::FindClosestBuildSite(UnitDef* unitDef, const springai::AIFloat3& pos, float searchRadius, int minDist, int facing) {
		return springai::AIFloat3::NULL_VALUE;
	}

	void springai::StubMap::SetDrawer(springai::Drawer* drawer) {
		this->drawer = drawer;
	}
	springai::Drawer* springai::StubMap::GetDrawer() {
		return drawer;
	}

