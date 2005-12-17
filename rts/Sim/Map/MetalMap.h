#ifndef METALMAP_H
#define METALMAP_H

#include "System/GlobalStuff.h"

const float METAL_MAP_SQUARE_SIZE = SQUARE_SIZE * 2;	//Each square on metalmap is a 2x2 square on normal map.

class CMetalMap
{
public:
	CMetalMap(unsigned char *map, int sizex, int sizez, float metalscale);
	virtual ~CMetalMap(void);

	float getMetalAmount(int x1, int z1, int x2, int z2);
	float getMetalAmount(int x, int z);
	float requestExtraction(int x, int z, float toDepth);
	void removeExtraction(int x, int z, float depth);

	unsigned char *metalMap;
	unsigned char metalPal[768];
	float *extractionMap;
	float metalscale;
protected:
	int sizex;
	int sizez;
};

#endif /* METALMAP_H */
