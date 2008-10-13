#ifndef METALMAP_H
#define METALMAP_H

#include "GlobalAI.h"


class CMetalMap
{
public:

	CMetalMap(AIClasses* ai);
	virtual ~CMetalMap();
	void Init();
	int NumSpotsFound;
	float AverageMetal;
	float3 GetNearestMetalSpot(int builderid, const UnitDef* extractor);
	int FindMetalSpotUpgrade(int builderid, const UnitDef* extractor);
	float GetMetalAtThisPoint(float3 pos);
	vector<float3> VectoredSpots;
private:
	void GetMetalPoints();
	void SaveMetalMap();
	bool LoadMetalMap();

	float3 BufferSpot;
	bool Stopme;
	int MaxSpots;
	int MetalMapHeight;
	int MetalMapWidth;
	int TotalCells;
	int SquareRadius;
	int DoubleSquareRadius;
	int TotalMetal;
	int MaxMetal;
	int TempMetal;
	int coordx;
	int coordy;
	int Minradius;
	int MinMetalForSpot;
	int XtractorRadius;
	int DoubleRadius;
	unsigned char* MexArrayA;	
	unsigned char* MexArrayB;
	unsigned char* MexArrayC; 
	unsigned char* MexArrayD; 
	int* TempAverage;
	AIClasses *ai;
};

#endif /* METALMAP_H */
