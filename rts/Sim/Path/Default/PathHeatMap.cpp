/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathHeatMap.hpp"
#include "PathConstants.h"
#include "PathManager.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Objects/SolidObject.h"

PathHeatMap* PathHeatMap::GetInstance() {
	static PathHeatMap* phm = NULL;

	if (phm == NULL) {
		phm = new PathHeatMap(PATH_HEATMAP_XSCALE, PATH_HEATMAP_ZSCALE);
	}

	return phm;
}

void PathHeatMap::FreeInstance(PathHeatMap* phm) {
	delete phm;
}



PathHeatMap::PathHeatMap(unsigned int scalex, unsigned int scalez): enabled(true) {
	xscale = std::max(1, std::min(gs->hmapx, int(scalex)));
	zscale = std::max(1, std::min(gs->hmapy, int(scalez)));
	xsize  = gs->hmapx / xscale;
	zsize  = gs->hmapy / zscale;

	heatMap.resize(xsize * zsize, HeatCell());
	heatMapOffset = 0;
}

PathHeatMap::~PathHeatMap() {
	heatMap.clear();
}

unsigned int PathHeatMap::GetHeatMapIndex(unsigned int hmx, unsigned int hmz) const {
	assert(!heatMap.empty());

	//! x & y are given in gs->mapi coords (:= gs->hmapi * 2)
	hmx >>= xscale;
	hmz >>= zscale;

	return (hmz * xsize + hmx);
}

void PathHeatMap::AddHeat(const CSolidObject* owner, const CPathManager* pm, unsigned int pathID) {
	if (pathID == 0)
		return;
	if (!owner->moveDef->heatMapping)
		return;

	#ifndef USE_GML
	static std::vector<int2> points;
	#else
	std::vector<int2> points;
	#endif

	pm->GetDetailedPathSquares(pathID, points);

	if (!points.empty()) {
		const float scale = 1.0f / points.size();
		const float heat = scale * owner->moveDef->heatProduced;

		unsigned int i = points.size();

		for (std::vector<int2>::const_iterator it = points.begin(); it != points.end(); ++it) {
			UpdateHeatValue(it->x, it->y, i * heat, owner->id);

			// decrease contribution from further-away points
			i--;
		}
	}
}

void PathHeatMap::UpdateHeatValue(unsigned int x, unsigned int y, unsigned int value, unsigned int ownerID) {
	const unsigned int idx = GetHeatMapIndex(x, y);

	if (heatMap[idx].value < value + heatMapOffset) {
		heatMap[idx].value = value + heatMapOffset;
		heatMap[idx].ownerID = ownerID;
	}
}

float PathHeatMap::GetHeatCost(unsigned int x, unsigned int z, const MoveDef& md, unsigned int ownerID) const {
	float c = 0.0f;

	if (!enabled) { return c; }
	if (!md.heatMapping) { return c; }

	const unsigned int idx = GetHeatMapIndex(x, z);
	const unsigned int val = (heatMapOffset >= heatMap[idx].value)? 0: (heatMap[idx].value - heatMapOffset);

	if (heatMap[idx].ownerID != ownerID) {
		c = (md.heatMod * val);
	}

	return c;
}
