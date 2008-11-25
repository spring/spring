#ifndef UNITSYNC_H
#define UNITSYNC_H

#include <string>
#include "exportdefines.h"
#include "ExternalAI/Interface/ELevelOfSupport.h"
#include "ExternalAI/Interface/SOption.h"
#include "ExternalAI/Interface/SInfo.h"

#define STRBUF_SIZE 100000


/**
 * @addtogroup unitsync_api Unitsync API
 * @{
*/

/**
 * @brief 2d vector storing a map defined starting position
 * @sa MapInfo
 */
struct StartPos
{
	int x; ///< X component
	int z; ///< Z component
};


/**
 * @brief Metadata of a map
 * @sa GetMapInfo GetMapInfoEx
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


/**
 * @brief Available bitmap typeHints
 * @sa GetInfoMap
 */
enum BitmapType {
	bm_grayscale_8  = 1, ///< 8 bits per pixel grayscale bitmap
	bm_grayscale_16 = 2  ///< 16 bits per pixel grayscale bitmap
};

/** @} */


const char *GetStr(std::string str);


#ifdef WIN32
	#include <windows.h>
#else
	#include <iostream>
	#define MB_OK 0
	static inline void MessageBox(void*, const char* msg, const char* capt, unsigned int)
	{
		std::cerr << "unitsync: " << capt << ": " << msg << std::endl;
	}
#endif	/* WIN32 */

#endif // UNITSYNC_H
