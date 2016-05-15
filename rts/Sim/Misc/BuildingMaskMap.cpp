#include "BuildingMaskMap.h"
#include "GlobalConstants.h"

BuildingMaskMap* buildingMaskMap = nullptr;

CR_BIND(BuildingMaskMap, ())
CR_REG_METADATA(BuildingMaskMap, (
	CR_MEMBER(maskMap)
	))


bool BuildingMaskMap::CheckBounds(int x, int z)
{
	return
		(x >= 0 && x <= mapDims.mapx) &&
		(z >= 0 && z <= mapDims.mapy);
}

// sets mask value for tile[x,z] in SQUARE_SIZE coordinates
bool BuildingMaskMap::SetTileMask(int x, int z, boost::uint16_t value)
{	
	if (!CheckBounds(x, z))
		return false;

	maskMap[x + z * mapDims.mapx] = value;
	return true;
}

// tests previously set mask for tile[x,z] in SQUARE_SIZE coordinates against supplied value
// true - construction is allowed, false - it's not
bool BuildingMaskMap::TestTileMaskUnsafe(int x, int z, boost::uint16_t value)
{
	return value == maskMap[x + z * mapDims.mapx];
}