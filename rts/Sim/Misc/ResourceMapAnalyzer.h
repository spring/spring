/*
	Copyright (c) 2005 Krogoth

	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	@author Krogoth
	@author Kloot
	@author Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef _RESOURCEMAPANALYZER_H
#define _RESOURCEMAPANALYZER_H

#include "float3.h"
#include <vector>

class CResource;
class UnitDef;

class CResourceMapAnalyzer {
	public:
		CResourceMapAnalyzer(int ResourceId);
		~CResourceMapAnalyzer();

		float3 GetNearestSpot(float3 fromPos, int team, const UnitDef* extractor = NULL) const;
		float3 GetNearestSpot(int builderUnitId, const UnitDef* extractor = NULL) const;
		float GetAverageIncome() const;
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
