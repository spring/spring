/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_HEATMAP_HDR
#define PATH_HEATMAP_HDR

#include <vector>
#include "System/type2.h"

class CPathManager;
class CSolidObject;
struct MoveDef;

/**
 * Heat mapping makes the pathfinder favor unused paths more. 
 * Less path overlap should make units behave more intelligently.
 */
class PathHeatMap {
public:
	static PathHeatMap* GetInstance();
	static void FreeInstance(PathHeatMap*);

	void Init(unsigned int sizex, unsigned int sizez);
	void Kill() {
		heatMap.clear();
		pathSquares.clear();
	}

	void Update() { ++heatMapOffset; }


	unsigned int GetHeatMapIndex(unsigned int x, unsigned int y) const;

	void AddHeat(const CSolidObject* owner, const CPathManager* pm, unsigned int pathID);
	void UpdateHeatValue(unsigned int x, unsigned int y, unsigned int value, unsigned int ownerID);

	const int GetHeatValue(unsigned int x, unsigned int y) const {
		const unsigned int idx = GetHeatMapIndex(x, y);
		const unsigned int val = heatMap[idx].value;

		return ((val - heatMapOffset) * (heatMapOffset < val));
	}

	float GetHeatCost(unsigned int x, unsigned int z, const MoveDef&, unsigned int ownerID) const;

private:
	struct HeatCell {
		unsigned int value = 0;
		unsigned int ownerID = 0;
	};


	// resolution is hmapx*hmapy
	std::vector<HeatCell> heatMap;
	std::vector<int2> pathSquares;

	unsigned int xscale = 0, xsize = 0;
	unsigned int zscale = 0, zsize = 0;

	// heatmap values are relative to this
	unsigned int heatMapOffset = 0;
};

#endif
