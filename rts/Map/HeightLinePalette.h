/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HEIGHT_LINE_PALETTE_H
#define HEIGHT_LINE_PALETTE_H

/** @brief The palette used in height-map rendering mode (F1). */
class CHeightLinePalette
{
public:
	/**
	 * @brief Generates the height line palette.
	 * Based on the configuration variable "ColorElev" (default: 1), it either
	 * generates a colored palette or a grayscale palette.
	 */
	CHeightLinePalette();

	/**
	 * @brief Returns the palette.
	 * The palette data consists of 256 RGB triplets with range 0-255.
	 */
	const unsigned char* GetData() const { return heightLinePal; }

private:
	unsigned char heightLinePal[3*256];
};

#endif // HEIGHT_LINE_PALETTE
