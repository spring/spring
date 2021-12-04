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


const char* GetStr(const std::string& str);

#endif // _UNITSYNC_H

