/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BUILDINGMASKMAP_H
#define BUILDINGMASKMAP_H

#include <vector>

#include "System/creg/creg_cond.h"

#include "Map/ReadMap.h"

class BuildingMaskMap
{

	CR_DECLARE_STRUCT(BuildingMaskMap)

public:
	BuildingMaskMap() {
		maskMap.clear();

		// we are going to operate in 2*SQUARE_SIZE space as spring snaps buildings to 2*SQUARE_SIZE based grid
		maskMap.resize(mapDims.hmapx * mapDims.hmapy, 1); //1st bit set to 1 constitutes for "normal tile"
	};

	bool SetTileMask(int x, int z, boost::uint16_t value);
	bool TestTileMask(int x, int z, boost::uint16_t value);
	bool TestTileMaskUnsafe(int x, int z, boost::uint16_t value);

private:
	bool CheckBounds(int x, int z);
private:
	std::vector<boost::uint16_t> maskMap;
};

extern BuildingMaskMap* buildingMaskMap;

#endif
