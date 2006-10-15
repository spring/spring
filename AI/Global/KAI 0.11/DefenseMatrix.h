#pragma once

#include "GlobalAI.h"
class CSpotFinder;

class CDefenseMatrix
{
public:
	CDefenseMatrix(AIClasses *ai);
	
	virtual ~CDefenseMatrix();
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
	CSpotFinder * spotFinder;
	int ThreatMapXSize,ThreatMapYSize,TotalCells;
	AIClasses *ai;
	
};
