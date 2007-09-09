#ifndef SPOTFINDER_H
#define SPOTFINDER_H


#include "GlobalAI.h"

struct CachePoint {
	float maxValueInBox;
	int x;
	int y;
	unsigned isValid:1;
	unsigned isMasked:1;
};

class CSpotFinder {
	public:
		CSpotFinder(AIClasses* ai, int height, int width);
		virtual ~CSpotFinder();

		void SetBackingArray(float* map, int height, int width);
		float* GetSumMap();

		void InvalidateSumMap(int coordx, int coordy, int clearRadius);
		void SetRadius(int radius);
		void BackingArrayChanged();
		CachePoint* GetBestCachePoint(int x, int y);

	private:
		//void GetMetalPoints();
		float* MakeSumMap();
		void MakeCachePoints();
		void UpdateSumMapArea(int cacheX, int cacheY);
		void UpdateSumMap();

		// Temp
		void UpdateSumMap(int coordx, int coordy, int clearRadius);

		bool haveTheBestSpotReady;
		bool isValid;
		int bestSpotX;
		int bestSpotY;

		int MapHeight;
		int MapWidth;
		int TotalCells;
		int radius;
		float* MexArrayA;
		float* TempAverage;
		int* xend;

		AIClasses* ai;
		CachePoint* cachePoints;
};


#endif
