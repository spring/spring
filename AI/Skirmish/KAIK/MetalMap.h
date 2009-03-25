#ifndef KAIK_METALMAP_HDR
#define KAIK_METALMAP_HDR

#include <vector>

struct AIClasses;

class CMetalMap {
	public:
		CMetalMap(AIClasses* ai);
		~CMetalMap();

		void Init();
		int NumSpotsFound;
		float AverageMetal;
		float3 GetNearestMetalSpot(int builderid, const UnitDef* extractor);
		std::vector<float3> VectoredSpots;

	private:
		void GetMetalPoints();
		void SaveMetalMap();
		bool LoadMetalMap();

		std::string GetCacheName() const;

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

		AIClasses* ai;
};


#endif
