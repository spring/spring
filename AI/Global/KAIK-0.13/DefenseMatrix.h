#ifndef DEFENSEMATRIX_H
#define DEFENSEMATRIX_H


#include "GlobalAI.h"

class CSpotFinder;

class CDefenseMatrix {
	public:
		#ifdef USE_CREG
		CR_DECLARE(CDefenseMatrix);
		#endif

		CDefenseMatrix(AIClasses* ai);
		~CDefenseMatrix();

		void Init();
		void AddDefense(float3 pos, const UnitDef* def);
		void RemoveDefense(float3 pos, const UnitDef* def);
		void UpdateChokePointArray();
		float3 GetDefensePos(const UnitDef* def, float3 builderpos);
		void MaskBadBuildSpot(float3 pos);

		vector<float*> ChokeMapsByMovetype;
		float* ChokePointArray;
		int* BuildMaskArray;

	private:
		CSpotFinder* spotFinder;
		int ThreatMapXSize, ThreatMapYSize, TotalCells;
		AIClasses* ai;
};


#endif
