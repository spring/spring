#pragma once
//#include "GlobalAI.h" // Insert your AI file Here.

//class CGlobalAI;
//class UnitTable;

class MetalMap{
public:
	MetalMap(IAICallback* ai);
	virtual ~MetalMap();

	void init();

	int NumSpotsFound;
	int AverageMetal;
	vector<float3> VectoredSpots;
	
	
private:
	void MakeTGA(unsigned char* data);
	void GetMetalPoints();
	void SaveMetalMap();
	bool LoadMetalMap();

	vector<int> LastTryAtSpot;
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
	int MinMetalForSpot;
	unsigned char XtractorRadius;
	unsigned char DoubleRadius;
	unsigned char* MexArrayA;	
	unsigned char* MexArrayB;
	unsigned char* MexArrayC; 
	int* TempAverage;
	IAICallback* cb;
	Global *ai;
};
