/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _RESOURCE_MAP_ANALYZER_H
#define _RESOURCE_MAP_ANALYZER_H

#include "System/float3.h"
#include <vector>

class CResource;
struct UnitDef;

class CResourceMapAnalyzer {
public:
	CResourceMapAnalyzer(int resourceId);

	// deferred to ResourceHandler
	void Init();

	/**
	 * Returns positions indicating where to place resource extractors on the map.
	 * Only the x and z values give the location of the spots, while the y values
	 * represents the actual amount of resource an extractor placed there can make.
	 * You should only compare the y values to each other, and not try to estimate
	 * effective output from spots.
	 */
	const std::vector<float3>& GetSpots() const { return vectoredSpots; }

	float3 GetNearestSpot(float3 fromPos, int team, const UnitDef* extractor = NULL) const;
	float3 GetNearestSpot(int builderUnitId, const UnitDef* extractor = NULL) const;
	float GetAverageIncome() const;

	// equal to vectoredSpots.size() after Init, otherwise -1
	int GetNumSpots() const { return numSpotsFound; }

private:
	void GetResourcePoints();
	void SaveResourceMap();
	bool LoadResourceMap();

	std::string GetCacheFileName() const;

	int resourceId;
	int numSpotsFound;

	float extractorRadius;
	float averageIncome;

	bool stopMe;

	// if more spots than this are found the map is considered a resource-map (eg. speed-metal), tweak as needed
	int maxSpots;
	int mapHeight;
	int mapWidth;
	int totalCells;
	int squareRadius;
	int doubleSquareRadius;
	int totalResources;
	int maxResource;
	int tempResources;
	int coordX;
	int coordZ;
	// from 0-255, the minimum percentage of resources a spot needs to have from
	// the maximum to be saved, prevents crappier spots in between taken spaces
	// (they are still perfectly valid and will generate resources mind you!)
	int minIncomeForSpot;
	int xtractorRadius;
	int doubleRadius;

	float3 bufferSpot;

	std::vector<unsigned char> rexArrayA;
	std::vector<unsigned char> rexArrayB;
	std::vector<unsigned char> rexArrayC;
	std::vector<int> tempAverage;

	std::vector<float3> vectoredSpots;
};

#endif // _RESOURCE_MAP_ANALYZER_H
