/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPMAP_H
#define _CPPWRAPPER_WRAPPMAP_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Map.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappMap : public Map {

private:
	int skirmishAIId;

	WrappMap(int skirmishAIId);
	virtual ~WrappMap();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Map* GetInstance(int skirmishAIId);

public:
	// @Override
	virtual int GetChecksum();

public:
	// @Override
	virtual springai::AIFloat3 GetStartPos();

public:
	// @Override
	virtual springai::AIFloat3 GetMousePos();

public:
	// @Override
	virtual bool IsPosInCamera(const springai::AIFloat3& pos, float radius);

	/**
	 * Returns the maps center heightmap width.
	 * @see getHeightMap()
	 */
public:
	// @Override
	virtual int GetWidth();

	/**
	 * Returns the maps center heightmap height.
	 * @see getHeightMap()
	 */
public:
	// @Override
	virtual int GetHeight();

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
	// @Override
	virtual std::vector<float> GetHeightMap();

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
	// @Override
	virtual std::vector<float> GetCornersHeightMap();

public:
	// @Override
	virtual float GetMinHeight();

public:
	// @Override
	virtual float GetMaxHeight();

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
	// @Override
	virtual std::vector<float> GetSlopeMap();

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
	// @Override
	virtual std::vector<int> GetLosMap();

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
	// @Override
	virtual std::vector<int> GetRadarMap();

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
	// @Override
	virtual std::vector<int> GetJammerMap();

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
	// @Override
	virtual std::vector<short> GetResourceMapRaw(Resource* resource);

	/**
	 * Returns positions indicating where to place resource extractors on the map.
	 * Only the x and z values give the location of the spots, while the y values
	 * represents the actual amount of resource an extractor placed there can make.
	 * You should only compare the y values to each other, and not try to estimate
	 * effective output from spots.
	 */
public:
	// @Override
	virtual std::vector<springai::AIFloat3> GetResourceMapSpotsPositions(Resource* resource);

	/**
	 * Returns the average resource income for an extractor on one of the evaluated positions.
	 */
public:
	// @Override
	virtual float GetResourceMapSpotsAverageIncome(Resource* resource);

	/**
	 * Returns the nearest resource extractor spot to a specified position out of the evaluated list.
	 */
public:
	// @Override
	virtual springai::AIFloat3 GetResourceMapSpotsNearest(Resource* resource, const springai::AIFloat3& pos);

	/**
	 * Returns the archive hash of the map.
	 * Use this for reference to the map, eg. in a cache-file, wherever human
	 * readability does not matter.
	 * This value will never be the same for two maps not having equal content.
	 * Tip: convert to 64 Hex chars for use in file names.
	 * @see getName()
	 */
public:
	// @Override
	virtual int GetHash();

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
	// @Override
	virtual const char* GetName();

	/**
	 * Returns the human readbale name of the map.
	 * @see getName()
	 */
public:
	// @Override
	virtual const char* GetHumanName();

	/**
	 * Gets the elevation of the map at position (x, z)
	 */
public:
	// @Override
	virtual float GetElevationAt(float x, float z);

	/**
	 * Returns what value 255 in the resource map is worth
	 */
public:
	// @Override
	virtual float GetMaxResource(Resource* resource);

	/**
	 * Returns extraction radius for resource extractors
	 */
public:
	// @Override
	virtual float GetExtractorRadius(Resource* resource);

public:
	// @Override
	virtual float GetMinWind();

public:
	// @Override
	virtual float GetMaxWind();

public:
	// @Override
	virtual float GetCurWind();

public:
	// @Override
	virtual float GetTidalStrength();

public:
	// @Override
	virtual float GetGravity();

public:
	// @Override
	virtual float GetWaterDamage();

	/**
	 * Returns all points drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
public:
	// @Override
	virtual std::vector<springai::Point*> GetPoints(bool includeAllies);

	/**
	 * Returns all lines drawn with this AIs team color,
	 * and additionally the ones drawn with allied team colors,
	 * if <code>includeAllies</code> is true.
	 */
public:
	// @Override
	virtual std::vector<springai::Line*> GetLines(bool includeAllies);

public:
	// @Override
	virtual bool IsPossibleToBuildAt(UnitDef* unitDef, const springai::AIFloat3& pos, int facing);

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
	// @Override
	virtual springai::AIFloat3 FindClosestBuildSite(UnitDef* unitDef, const springai::AIFloat3& pos, float searchRadius, int minDist, int facing);

public:
	// @Override
	virtual springai::Drawer* GetDrawer();
}; // class WrappMap

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPMAP_H

