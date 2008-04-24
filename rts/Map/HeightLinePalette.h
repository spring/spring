#ifndef HEIGHTLINEPALETTE_H
#define HEIGHTLINEPALETTE_H

/** @brief The palette used in heightmap rendering mode (F1). */
class CHeightLinePalette
{
public:
	CHeightLinePalette();

	/** @brief Gets the palette.
	The palette data consists of 256 RGB triplets with range 0-255. */
	const unsigned char* GetData() const { return heightLinePal; }

private:
	unsigned char heightLinePal[3*256];
};

#endif
