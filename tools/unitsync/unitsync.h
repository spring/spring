#ifndef UNITSYNC_H
#define UNITSYNC_H

#include <string>

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
 * @brief Available mod/map option types
 * @sa GetOptionType
 */
enum OptionType {
	opt_error   = 0, ///< error
	opt_bool    = 1, ///< boolean option
	opt_list    = 2, ///< list option (e.g. combobox)
	opt_number  = 3, ///< numeric option (e.g. spinner / slider)
	opt_string  = 4, ///< string option (e.g. textbox)
	opt_section = 5  ///< option section (e.g. groupbox)
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

#endif

