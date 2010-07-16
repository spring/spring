/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _RESOURCEMAPANALYZER_H
#define _RESOURCEMAPANALYZER_H

#include "float3.h"
#include <vector>

class CResource;
struct UnitDef;

class CResourceMapAnalyzer {
	public:
		CResourceMapAnalyzer(int ResourceId);
		~CResourceMapAnalyzer();

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

		int ResourceId;
		float ExtractorRadius;
		int NumSpotsFound;
		std::vector<float3> VectoredSpots;
		float AverageIncome;
		float3 BufferSpot;
		bool StopMe;
		int MaxSpots;
		int MapHeight;
		int MapWidth;
		int TotalCells;
		int SquareRadius;
		int DoubleSquareRadius;
		int TotalResources;
		int MaxResource;
		int TempResources;
		int coordX;
		int coordZ;
		int MinRadius;
		int MinIncomeForSpot;
		int XtractorRadius;
		int DoubleRadius;
		unsigned char* RexArrayA;
		unsigned char* RexArrayB;
		unsigned char* RexArrayC;
		int* TempAverage;
};

#endif // _RESOURCEMAPANALYZER_H
