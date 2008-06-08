#ifndef METALMAP_H
#define METALMAP_H

#include "GlobalStuff.h"


// Each square on metalmap is a 2x2 square on normal map.
const float METAL_MAP_SQUARE_SIZE = SQUARE_SIZE * 2;


class CMetalMap
{
public:
	CR_DECLARE(CMetalMap);
	CMetalMap(unsigned char* map, int sizeX, int sizeZ, float metalScale);
	virtual ~CMetalMap(void);

	float getMetalAmount(int x1, int z1, int x2, int z2);
	float getMetalAmount(int x, int z);
	float requestExtraction(int x, int z, float toDepth);
	void  removeExtraction(int x, int z, float depth);

	unsigned char* metalMap;
	unsigned char  metalPal[768];
	std::vector<float> extractionMap;

	int GetSizeX() const { return sizeX; }
	int GetSizeZ() const { return sizeZ; }

protected:
	float metalScale;
	int sizeX;
	int sizeZ;
};


#endif /* METALMAP_H */
