#pragma once
 // Insert your AI file Here.

struct MetalSpot{
	int x;
	int y;
	int z;
	int Amount;
	bool Occupied;
};
class MetalMap{
public:
	MetalMap(IAICallback *cb);
	virtual ~MetalMap();
	void init();

	vector<MetalSpot> BestSpots;	// MetalSpot is a struct with int x,y,z and float Amount. 
							//Stuff like bool Occupied etc can be added at will.
	bool IsMetalMap; 
	int SpotsFound;
	float AverageMetal;
	
private:
	void MakeTGA(unsigned char* data);
	void GetMetalPoints();
	void SaveMetalMap();
	bool LoadMetalMap();

	bool Stopme;
	float CellsInRadius;
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
	vector<MetalSpot> TempBestSpots; 
	IAICallback* cb;
};