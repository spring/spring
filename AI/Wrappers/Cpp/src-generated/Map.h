/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_MAP_H
#define _CPPWRAPPER_MAP_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Map {

public:
	virtual ~Map(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetChecksum() = 0;

public:
	virtual springai::AIFloat3 GetStartPos() = 0;

public:
	virtual springai::AIFloat3 GetMousePos() = 0;

public:
	virtual bool IsPosInCamera(const springai::AIFloat3& pos, float radius) = 0;

	/**
	 * Returns the maps center heightmap width.
	 * @see getHeightMap()
	 */
public:
	virtual int GetWidth() = 0;

	/**
	 * Returns the maps center heightmap height.
	 * @see getHeightMap()
	 */
public:
	virtual int GetHeight() = 0;

	/**
	 * Returns the height for the center of the squares.
	 * This differs slightly from the drawn map, since
	 * that one uses the height at the corners.
	 * Note that the actual map is 8 times larger (in each dimension) and
	 * all other maps (slope, los, resources, etc.) are relative to the
	 * size of the heightmap.
	 * 
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 8*8 in size
	 * - the value for the full resolution position (x, z) is at index (z * width + x)
	 * - the last value, bottom right, is at index (width * height - 1)
	 * 
	 * @see getCornersHeightMap()
	 */
public:
	virtual std::vector<float> GetHeightMap() = 0;

	/**
	 * Returns the height for the corners of the squares.
	 * This is the same like the drawn map.
	 * It is one unit wider and one higher then the centers height map.
	 * 
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - 4 points mark the edges of an area of 8*8 in size
	 * - the value for upper left corner of the full resolution position (x, z) is at index (z * width + x)
	 * - the last value, bottom right, is at index ((width+1) * (height+1) - 1)
	 * 
	 * @see getHeightMap()
	 */
public:
	virtual std::vector<float> GetCornersHeightMap() = 0;

public:
	virtual float GetMinHeight() = 0;

public:
	virtual float GetMaxHeight() = 0;

	/**
	 * @brief the slope map
	 * The values are 1 minus the y-component of the (average) facenormal of the square.
	 * 
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 2*2 in size
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 2)
	 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
	 */
public:
	virtual std::vector<float> GetSlopeMap() = 0;

	/**
	 * @brief the level of sight map
	 * mapDims.mapx >> losMipLevel
	 * A square with value zero means you do not have LOS coverage on it.
	 * Mod_getLosMipLevel
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - resolution factor (res) is min(1, 1 << Mod_getLosMipLevel())
	 *   examples:
	 *   	+ losMipLevel(0) -> res(1)
	 *   	+ losMipLevel(1) -> res(2)
	 *   	+ losMipLevel(2) -> res(4)
	 *   	+ losMipLevel(3) -> res(8)
	 * - each data position is res*res in size
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / res)
	 * - the last value, bottom right, is at index (width/res * height/res - 1)
	 */
public:
	virtual std::vector<int> GetLosMap() = 0;

	/**
	 * @brief the radar map
	 * A square with value 0 means you do not have radar coverage on it.
	 * 
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 8*8 in size
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 8)
	 * - the last value, bottom right, is at index (width/8 * height/8 - 1)
	 */
public:
	virtual std::vector<int> GetRadarMap() = 0;

	/**
	 * @brief the radar jammer map
	 * A square with value 0 means you do not have radar jamming coverage.
	 * 
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 8*8 in size
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 8)
	 * - the last value, bottom right, is at index (width/8 * height/8 - 1)
	 */
public:
	virtual std::vector<int> GetJammerMap() = 0;

	/**
	 * @brief resource maps
	 * This map shows the resource density on the map.
	 * 
	 * - do NOT modify or delete the height-map (native code relevant only)
	 * - index 0 is top left
	 * - each data position is 2*2 in size
	 * - the value for the full resolution position (x, z) is at index ((z * width + x) / 2)
	 * - the last value, bottom right, is at index (width/2 * height/2 - 1)
	 */
public:
	virtual std::vector<short> GetResourceMapRaw(Resource* resource) = 0;

	/**
	 * Returns positions indicating where to place resource extractors on the map.
	 * Only the x and z values give the location of the spots, while the y values
	 * represents the actual amount of resource an extractor placed there can make.
	 * You should only compare the y values to each other, and not try to estimate
	 * effective output from spots.
	 */
public:
	virtual std::vector<springai::AIFloat3> GetResourceMapSpotsPositions(Resource* resource) = 0;

	/**
	 * Returns the average resource income for an extractor on one of the evaluated positions.
	 */
public:
	virtual float GetResourceMapSpotsAverageIncome(Resource* resource) = 0;

	/**
	 * Returns the nearest resource extractor spot to a specified position out of the evaluated list.
	 */
public:
	virtual springai::AIFloat3 GetResourceMapSpotsNearest(Resource* resource, const springai::AIFloat3& pos) = 0;

	/**
	 * Returns the archive hash of the map.
	 * Use this for reference to the map, eg. in a cache-file, wherever human
	 * readability does not matter.
	 * This value will never be the same for two maps not having equal content.
	 * Tip: convert to 64 Hex chars for use in file names.
	 * @see getName()
	 */
public:
	virtual int GetHash() = 0;

	/**
	 * Returns the name of the map.
	 * Use this for reference to the map, eg. in cache- or config-file names
	 * which are map related, wherever humans may come in contact with the reference.
	 * Be aware though, that this may contain special characters and spaces,
	 * and may not be used as a file name without checks and replaces.
	 * Tip: replace every char matching [^0-9a-zA-Z_-.] with '_'
	 * @see getHash()
	 * @see getHumanName()
	 */
public:
	virtual const char* GetName() = 0;

	/**
	 * Returns the human readbale name of the map.
	 * @see getName()
	 */
public:
	virtual const char* GetHumanName() = 0;

	/**
	 * Gets the elevation of the map at position (x, z)
	 */
public:
	virtual float GetElevationAt(float x, float z) = 0;

	/**
	 * Returns what value 255 in the resource map is worth
	 */
public:
	virtual float GetMaxResource(Resource* resource) = 0;

	/**
	 * Returns extraction radius for resource extractors
	 */
public:
	virtual float GetExtractorRadius(Resource* resource) = 0;

public:
	virtual float GetMinWind() = 0;

public:
	virtual float GetMaxWind() = 0;

public:
	virtual float GetCurWind() = 0;

public:
	virtual float GetTidalStrength() = 0;

public:
	virtual float GetGravity() = 0;

public:
	virtual float GetWaterDamage() = 0;

	/**
	 * Returns all points drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
public:
	virtual std::vector<springai::Point*> GetPoints(bool includeAllies) = 0;

	/**
	 * Returns all lines drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
public:
	virtual std::vector<springai::Line*> GetLines(bool includeAllies) = 0;

public:
	virtual bool IsPossibleToBuildAt(UnitDef* unitDef, const springai::AIFloat3& pos, int facing) = 0;

	/**
	 * Returns the closest position from a given position that a building can be
	 * built at.
	 * @param minDist the distance in 1/(SQUARE_SIZE * 2) = 1/16 of full map
	 *                resolution, that the building must keep to other
	 *                buildings; this makes it easier to keep free paths through
	 *                a base
	 * @return actual map position with x, y and z all beeing positive,
	 *         or float[3]{-1, 0, 0} if no suitable position is found.
	 */
public:
	virtual springai::AIFloat3 FindClosestBuildSite(UnitDef* unitDef, const springai::AIFloat3& pos, float searchRadius, int minDist, int facing) = 0;

public:
	virtual springai::Drawer* GetDrawer() = 0;

}; // class Map

}  // namespace springai

#endif // _CPPWRAPPER_MAP_H

