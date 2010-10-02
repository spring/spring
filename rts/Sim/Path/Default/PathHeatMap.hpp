/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_HEATMAP_HDR
#define PATH_HEATMAP_HDR

#include <vector>

struct MoveData;

/**
 * Heat mapping makes the pathfinder favor unused paths more. 
 * Less path overlap should make units behave more intelligently.
 */
class PathHeatMap {
public:
	static PathHeatMap* GetInstance();
	static void FreeInstance(PathHeatMap*);

	PathHeatMap(unsigned int sizex, unsigned int sizez);
	~PathHeatMap();

	void Update() { ++heatMapOffset; }


	unsigned int GetHeatMapIndex(unsigned int x, unsigned int y) const;

	void UpdateHeatValue(unsigned int x, unsigned int y, unsigned int value, unsigned int ownerID);

	const int GetHeatValue(unsigned int x, unsigned int y) const {
		const unsigned int idx = GetHeatMapIndex(x, y);
		const unsigned int val = heatMap[idx].value;

		return (heatMapOffset >= val)? 0: (val - heatMapOffset);
	}

	float GetHeatCost(unsigned int x, unsigned int z, const MoveData&, unsigned int ownerID) const;

private:
	struct HeatCell {
		HeatCell(): value(0), ownerID(0) {}

		unsigned int value;
		unsigned int ownerID;
	};


	bool enabled;
	unsigned int heatMapOffset;          //! heatmap values are relative to this

	std::vector<HeatCell> heatMap;   //! resolution is hmapx*hmapy

	unsigned int xscale, xsize;
	unsigned int zscale, zsize;

};

#endif
