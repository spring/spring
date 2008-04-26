#ifndef METAL_MAP_H
#define METAL_MAP_H
/*

MetalMap Class version 5

Based upon MetalMap Class version 4 by Krogothe

*/

#define M_CLASS_VERSION "5"

class CMetalMap{
public:
	CMetalMap(IAICallback* aicb, bool verbose);
	virtual ~CMetalMap();
	void Init();
	int NumSpotsFound;
	float AverageMetal;
	float AverageMetalPerSpot;
	std::vector<float3> VectoredSpots;
	bool IsMetalMap;
	void SaveMetalMap();
	bool LoadMetalMap();
	void GetMetalPoints();
	
private:
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
	int* TempAverage;
	IAICallback* aicb;
	bool verbose;
};
#endif
