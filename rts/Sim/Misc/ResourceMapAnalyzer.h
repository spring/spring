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

		float3 GetNearestSpot(float3 fromPos, int team, const UnitDef* extractor = NULL) const;
		float3 GetNearestSpot(int builderUnitId, const UnitDef* extractor = NULL) const;
		float GetAverageIncome() const;
		/**
		 * Returns positions indicating where to place resource extractors on the map.
		 * Only the x and z values give the location of the spots, while the y values
		 * represents the actual amount of resource an extractor placed there can make.
		 * You should only compare the y values to each other, and not try to estimate
		 * effective output from spots.
		 */
		const std::vector<float3>& GetSpots() const;

	private:
		void Init();
		void GetResourcePoints();
		void SaveResourceMap();
		bool LoadResourceMap();

		std::string GetCacheFileName() const;

		int resourceId;
		float extractorRadius;
		int numSpotsFound;
		std::vector<float3> vectoredSpots;
		float averageIncome;
		float3 bufferSpot;
		bool stopMe;
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
		int minIncomeForSpot;
		int xtractorRadius;
		int doubleRadius;

		std::vector<unsigned char> rexArrayA;
		std::vector<unsigned char> rexArrayB;
		std::vector<unsigned char> rexArrayC;
		std::vector<int> tempAverage;
};

#endif // _RESOURCE_MAP_ANALYZER_H
