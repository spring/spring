#ifndef METALMAP_H
#define METALMAP_H

#pragma once
#include "ExternalAI/IAICallback.h"

class CMetalMap
{
public:

	CMetalMap(IAICallback* callback);
	virtual ~CMetalMap();
	void Init();
	int NumSpotsFound;
	float AverageMetal;
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
	int* TempAverage;
	IAICallback* callback;
};

#endif
