#ifndef KAIK_SPOTFINDER_HDR
#define KAIK_SPOTFINDER_HDR

#include <vector>
struct AIClasses;

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
		~CSpotFinder();

		void SetBackingArray(float* map, int height, int width);
		float* GetSumMap();

		void InvalidateSumMap(int coordx, int coordy, int clearRadius);
		void SetRadius(int radius);
		void BackingArrayChanged();
		CachePoint* GetBestCachePoint(int x, int y);

	private:
		float* MakeSumMap();
		void MakeCachePoints();
		void UpdateSumMapArea(int cacheX, int cacheY);
		void UpdateSumMap();

		// temp
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
		std::vector<CachePoint> cachePoints;
};

#endif
