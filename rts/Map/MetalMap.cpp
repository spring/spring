/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "MetalMap.h"
#include "ReadMap.h"
#include "System/Config/ConfigHandler.h"
#include "System/myMath.h"

CONFIG(bool, MetalMapPalette).defaultValue(false);

CR_BIND(CMetalMap,(NULL, 0, 0, 0.0f));

CR_REG_METADATA(CMetalMap,(
	CR_MEMBER(metalScale),
	CR_MEMBER(sizeX),
	CR_MEMBER(sizeZ),
	CR_MEMBER(metalPal),
	CR_MEMBER(metalMap),
	CR_MEMBER(extractionMap)
));

CMetalMap::CMetalMap(const unsigned char* map, int _sizeX, int _sizeZ, float _metalScale)
	: metalScale(_metalScale)
	, sizeX(_sizeX)
	, sizeZ(_sizeZ)
{
	extractionMap.resize(sizeX * sizeZ, 0.0f);
	metalMap.resize(sizeX * sizeZ, 0);

	if (map != NULL) {
		memcpy(&metalMap[0], map, sizeX * sizeZ);
	} else {
		metalScale = 1.0f;
	}

	if (configHandler->GetBool("MetalMapPalette")) {
		/* Swap the green and blue channels. making metal go
		   black -> blue -> cyan,
		   rather than the usual black -> green -> cyan. */
		for (int a = 0; a < 256; ++a) {
			metalPal[a * 3 + 0] = a;
			metalPal[a * 3 + 1] = std::max(0, a * 2 - 255);
			metalPal[a * 3 + 2] = std::min(255, a * 2);
		}
	} else {
		for (int a = 0; a < 256; ++a) {
			metalPal[a * 3 + 0] = a;
			metalPal[a * 3 + 1] = std::min(255, a * 2);
			metalPal[a * 3 + 2] = std::max(0, a * 2 - 255);
		}
	}

}


CMetalMap::~CMetalMap()
{
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


void CMetalMap::SetMetalAmount(int x, int z, float m)
{
	ClampInt(x, 0, sizeX);
	ClampInt(z, 0, sizeZ);

	metalMap[(z * sizeX) + x] = (metalScale == 0.0f) ? 0 : Clamp((int)(m / metalScale), 0, 255);
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


int CMetalMap::GetMetalExtraction(int x, int z)
{
	ClampInt(x, 0, sizeX);
	ClampInt(z, 0, sizeZ);

	return extractionMap[(z * sizeX) + x];
}
