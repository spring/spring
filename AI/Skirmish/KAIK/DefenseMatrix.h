#ifndef KAIK_DEFENSEMATRIX_HDR
#define KAIK_DEFENSEMATRIX_HDR

#include <vector>
#include "IncCREG.h"

class CSpotFinder;
struct AIClasses;

class CDefenseMatrix {
	public:
		CR_DECLARE(CDefenseMatrix);

		CDefenseMatrix(AIClasses* ai);
		~CDefenseMatrix();

		void PostLoad();
		void Init();
		void AddDefense(float3 pos, const UnitDef* def);
		void RemoveDefense(float3 pos, const UnitDef* def);
		void UpdateChokePointArray();
		float3 GetDefensePos(const UnitDef* def, float3 builderpos);
		void MaskBadBuildSpot(float3 pos);

		std::vector<std::vector<float> > ChokeMapsByMovetype;
		std::vector<float> ChokePointArray;
		std::vector<int> BuildMaskArray;

	private:
		CSpotFinder* spotFinder;
		int ThreatMapXSize, ThreatMapYSize, TotalCells;
		AIClasses* ai;
};

#endif
