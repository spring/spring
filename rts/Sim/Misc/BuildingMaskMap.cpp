#include "BuildingMaskMap.h"
#include "Map/ReadMap.h"

BuildingMaskMap buildingMaskMap;

CR_BIND(BuildingMaskMap, ())
CR_REG_METADATA(BuildingMaskMap, (
	CR_MEMBER(maskMap)
))


bool BuildingMaskMap::CheckBounds(unsigned int x, unsigned int z) const
{
	return ((x < mapDims.hmapx) && (z < mapDims.hmapy));
}

// sets mask value for tile[x,z] in 2*SQUARE_SIZE coordinates
bool BuildingMaskMap::SetTileMask(unsigned int x, unsigned int z, std::uint16_t value)
{
	if (!CheckBounds(x, z))
		return false;

	maskMap[x + z * mapDims.hmapx] = value;
	return true;
}

// tests previously set mask for tile[x,z] in 2*SQUARE_SIZE coordinates against supplied value
// true - construction is allowed, false - it's not
bool BuildingMaskMap::TestTileMaskUnsafe(unsigned int x, unsigned int z, std::uint16_t value) const
{
	assert(CheckBounds(x, z));
	return (maskMap[x + z * mapDims.hmapx] & value) == value;
}
