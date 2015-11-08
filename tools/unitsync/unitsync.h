/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _UNITSYNC_H
#define _UNITSYNC_H

#include <string>

#define STRBUF_SIZE 100000


/**
 * @addtogroup unitsync_api Unitsync API
 * @{
*/

#ifdef ENABLE_DEPRECATED_FUNCTIONS
/**
 * @brief 2d vector storing a map defined starting position
 * @sa MapInfo
 * @deprecated
 */
struct StartPos
{
	int x; ///< X component
	int z; ///< Z component
};


/**
 * @brief Metadata of a map
 * @sa GetMapInfo GetMapInfoEx
 * @deprecated
 */
struct MapInfo
{
	char* description;   ///< Description (max 255 chars)
	int tidalStrength;   ///< Tidal strength
	int gravity;         ///< Gravity
	float maxMetal;      ///< Metal scale factor
	int extractorRadius; ///< Extractor radius (of metal extractors)
	int minWind;         ///< Minimum wind speed
	int maxWind;         ///< Maximum wind speed

	// 0.61b1+
	int width;              ///< Width of the map
	int height;             ///< Height of the map
	int posCount;           ///< Number of defined start positions
	StartPos positions[16]; ///< Start positions defined by the map (max 16)

	// VERSION>=1
	char* author;   ///< Creator of the map (max 200 chars)
};

#endif //ENABLE_DEPRECATED_FUNCTIONS
/**
 * @brief Available bitmap typeHints
 * @sa GetInfoMap
 */
enum BitmapType {
	bm_grayscale_8  = 1, ///< 8 bits per pixel grayscale bitmap
	bm_grayscale_16 = 2  ///< 16 bits per pixel grayscale bitmap
};

/** @} */


const char* GetStr(std::string str);

#endif // _UNITSYNC_H

