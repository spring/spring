#ifndef DEFENSEMATRIX_H
#define DEFENSEMATRIX_H


#include "GlobalAI.h"

class CSpotFinder;

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

		vector<vector<float> > ChokeMapsByMovetype;
		vector<float> ChokePointArray;
		vector<int> BuildMaskArray;

	private:
		CSpotFinder* spotFinder;
		int ThreatMapXSize, ThreatMapYSize, TotalCells;
		AIClasses* ai;
};


#endif
