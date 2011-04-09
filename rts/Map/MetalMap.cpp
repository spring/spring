/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "MetalMap.h"
#include "ReadMap.h"
#include "ConfigHandler.h"

CR_BIND(CMetalMap,(NULL, 0, 0, 0.0f));

CR_REG_METADATA(CMetalMap,(
				CR_MEMBER(extractionMap)
				));

CMetalMap::CMetalMap(unsigned char* map, int sizeX, int sizeZ, float metalScale)
	: metalMap(map)
	, metalScale(metalScale)
	, sizeX(sizeX)
	, sizeZ(sizeZ)
{
	// Creating an empty map over extraction.
//	extractionMap = new float[sizeX * sizeZ];
	extractionMap.resize(sizeX * sizeZ, 0.0f);
//	int i;
//	for(i = 0; i < (sizeX * sizeZ); i++) {
//		extractionMap[i] = 0.0f;
//	}

	int whichPalette = configHandler->Get("MetalMapPalette", 0);

	if (whichPalette == 1){
		/* Swap the green and blue channels. making metal go
		   black -> blue -> cyan,
		   rather than the usual black -> green -> cyan. */
		for (int a = 0; a < 256; ++a) {
			metalPal[a * 3 + 0] = a;
			metalPal[a * 3 + 1] = std::max(0, a * 2 - 255);
			metalPal[a * 3 + 2] = std::min(255, a * 2);
		}
	}
	else {
		for(int a = 0; a < 256; ++a) {
			metalPal[a * 3 + 0] = a;
			metalPal[a * 3 + 1] = std::min(255, a * 2);
			metalPal[a * 3 + 2] = std::max(0, a * 2 - 255);
		}
	}

}


CMetalMap::~CMetalMap()
{
	delete[] metalMap;
//	delete[] extractionMap;
}


static inline void ClampInt(int& var, int min, int maxPlusOne)
{
	if (var < min) {
		var = min;
	} else if (var >= maxPlusOne) {
		var = maxPlusOne - 1;
	}
}


float CMetalMap::GetMetalAmount(int x1, int z1, int x2, int z2)
{
	ClampInt(x1, 0, sizeX);
	ClampInt(x2, 0, sizeX);
	ClampInt(z1, 0, sizeZ);
	ClampInt(z2, 0, sizeZ);

	float metal = 0.0f;
	int x, z;
	for (x = x1; x < x2; x++) {
		for (z = z1; z < z2; z++) {
			metal += metalMap[(z * sizeX) + x];
		}
	}
	return metal * metalScale;
}


float CMetalMap::GetMetalAmount(int x, int z)
{
	ClampInt(x, 0, sizeX);
	ClampInt(z, 0, sizeZ);

	return metalMap[(z * sizeX) + x] * metalScale;
}


float CMetalMap::RequestExtraction(int x, int z, float toDepth)
{
	ClampInt(x, 0, sizeX);
	ClampInt(z, 0, sizeZ);

	const float current = extractionMap[(z * sizeX) + x];

	if (toDepth <= current) {
		return 0.0f;
	}

	const float available = toDepth - current;

	extractionMap[(z * sizeX) + x] = toDepth;

	return available;
}


void CMetalMap::RemoveExtraction(int x, int z, float depth)
{
	ClampInt(x, 0, sizeX);
	ClampInt(z, 0, sizeZ);

	extractionMap[(z * sizeX) + x] -= depth;
}

