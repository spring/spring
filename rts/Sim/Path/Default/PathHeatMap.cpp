/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathHeatMap.hpp"
#include "PathConstants.h"
#include "PathManager.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Objects/SolidObject.h"

// not extern'ed, so static
static PathHeatMap gPathHeatMap;


PathHeatMap* PathHeatMap::GetInstance() {
	gPathHeatMap.Init(PATH_HEATMAP_XSCALE, PATH_HEATMAP_ZSCALE);
	return &gPathHeatMap;
}

void PathHeatMap::FreeInstance(PathHeatMap* phm) {
	assert(phm == &gPathHeatMap);
	phm->Kill();
}



void PathHeatMap::Init(unsigned int scalex, unsigned int scalez) {
	xscale = std::max(1, std::min(mapDims.hmapx, int(scalex)));
	zscale = std::max(1, std::min(mapDims.hmapy, int(scalez)));
	xsize  = mapDims.hmapx / xscale;
	zsize  = mapDims.hmapy / zscale;

	heatMapOffset = 0;

	heatMap.resize(xsize * zsize);
}

unsigned int PathHeatMap::GetHeatMapIndex(unsigned int hmx, unsigned int hmz) const {
	assert(!heatMap.empty());

	// x & y are given in mapDims.mapi coords (:= mapDims.hmapi * 2)
	hmx >>= xscale;
	hmz >>= zscale;

	return (hmz * xsize + hmx);
}

void PathHeatMap::AddHeat(const CSolidObject* owner, const CPathManager* pm, unsigned int pathID) {
	if (pathID == 0)
		return;
	if (!owner->moveDef->heatMapping)
		return;

	// internally clears and reserves squares
	pm->GetDetailedPathSquares(pathID, pathSquares);

	if (pathSquares.empty())
		return;

	// called every frame by PathManager::UpdatePath
	//
	// the i-th waypoint receives an amount of heat equal to
	// ((N - i) / N) * heatProduced so the entire _remaining_
	// path is constantly reserved (the default PFS consumes
	// waypoints as they are passed by units) but far-future
	// waypoints do not contribute much cost for other units
	//
	//   i=0   --> value=((N-0)/N)*heatProd
	//   i=1   --> value=((N-1)/N)*heatProd
	//   ...
	//   i=N-1 --> value=((  1)/N)*heatProd
	//
	// NOTE:
	//   only the max-resolution pathfinder reacts to heat!
	//
	//   waypoints are spaced SQUARE_SIZE*2 elmos apart so
	//   the heatmapped paths look like "breadcrumb" trails
	//   this does not matter only because the default PFS
	//   uses the same spacing-factor between waypoints
	unsigned int i = pathSquares.size();

	const float scale = 1.0f / i;
	const float value = scale * owner->moveDef->heatProduced;

	for (const int2 sqr: pathSquares) {
		UpdateHeatValue(sqr.x, sqr.y, (i--) * value, owner->id);
	}
}

void PathHeatMap::UpdateHeatValue(unsigned int x, unsigned int y, unsigned int value, unsigned int ownerID) {
	const unsigned int idx = GetHeatMapIndex(x, y);

	if (heatMap[idx].value < (value + heatMapOffset)) {
		heatMap[idx].value = value + heatMapOffset;
		heatMap[idx].ownerID = ownerID;
	}
}

float PathHeatMap::GetHeatCost(unsigned int x, unsigned int z, const MoveDef& md, unsigned int ownerID) const {
	float c = 0.0f;

	if (!md.heatMapping)
		return c;

	const unsigned int idx = GetHeatMapIndex(x, z);
	const unsigned int val = (heatMapOffset >= heatMap[idx].value)? 0: (heatMap[idx].value - heatMapOffset);

	if (heatMap[idx].ownerID != ownerID)
		c = (md.heatMod * val);

	return c;
}
