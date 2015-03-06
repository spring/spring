/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HEIGHT_LINE_PALETTE_H
#define HEIGHT_LINE_PALETTE_H

#include "System/Color.h"
#include <array>


/** @brief The palette used in height-map rendering mode (F1). */
class CHeightLinePalette
{
public:
	/**
	 * @brief Returns the palette.
	 * Based on the configuration variable "ColorElev" (default: 1), it either
	 * generates a colored palette or a grayscale palette.
	 * The palette data consists of 256 RGB triplets with range 0-255.
	 */
	static const SColor* GetData();

	static std::array<SColor, 256> paletteColored;
	static std::array<SColor, 256> paletteBlackAndWhite;
};

#endif // HEIGHT_LINE_PALETTE
