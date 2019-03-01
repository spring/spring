/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MetalMap.h"
#include "System/SpringMath.h"
#include "System/EventHandler.h"

#include <cstring>


CR_BIND(CMetalMap, )

CR_REG_METADATA(CMetalMap,(
	CR_MEMBER(metalScale),
	CR_MEMBER(sizeX),
	CR_MEMBER(sizeZ),

	CR_IGNORED(texturePalette),
	CR_MEMBER(distributionMap),
	CR_MEMBER(extractionMap)
))


CMetalMap metalMap;


#ifndef NO_METALMAP
void CMetalMap::Init(const unsigned char* map, int _sizeX, int _sizeZ, float _metalScale)
{
	metalScale = _metalScale;
	sizeX = _sizeX;
	sizeZ = _sizeZ;

	extractionMap.clear();
	extractionMap.resize(sizeX * sizeZ, 0.0f);
	distributionMap.clear();
	distributionMap.resize(sizeX * sizeZ, 0);

	if (map != nullptr) {
		memcpy(&distributionMap[0], map, sizeX * sizeZ);
	} else {
		metalScale = 1.0f;
	}

	for (int a = 0; a < 256; ++a) {
		texturePalette[a * 3 + 0] = a;
		texturePalette[a * 3 + 1] = std::min(255, a * 2      );
		texturePalette[a * 3 + 2] = std::max(  0, a * 2 - 255);
	}
}



float CMetalMap::GetMetalAmount(int x1, int z1, int x2, int z2) const
{
	x1 = Clamp(x1, 0, sizeX - 1);
	x2 = Clamp(x2, 0, sizeX - 1);
	z1 = Clamp(z1, 0, sizeZ - 1);
	z2 = Clamp(z2, 0, sizeZ - 1);

	float metal = 0.0f;

	for (int z = z1; z < z2; z++) {
		for (int x = x1; x < x2; x++) {
			metal += GetMetalAmount(x, z);
		}
	}

	return metal;
}


float CMetalMap::GetMetalAmount(int x, int z) const
{
	x = Clamp(x, 0, sizeX - 1);
	z = Clamp(z, 0, sizeZ - 1);

	return distributionMap[(z * sizeX) + x] * metalScale;
}


void CMetalMap::SetMetalAmount(int x, int z, float m)
{
	x = Clamp(x, 0, sizeX - 1);
	z = Clamp(z, 0, sizeZ - 1);

	distributionMap[(z * sizeX) + x] = (metalScale == 0.0f) ? 0 : Clamp((int)(m / metalScale), 0, 255);

	eventHandler.MetalMapChanged(x, z);
}


float CMetalMap::RequestExtraction(int x, int z, float toDepth)
{
	x = Clamp(x, 0, sizeX - 1);
	z = Clamp(z, 0, sizeZ - 1);

	const float current = extractionMap[(z * sizeX) + x];

	if (toDepth <= current)
		return 0.0f;

	const float available = toDepth - current;

	extractionMap[(z * sizeX) + x] = toDepth;

	return available;
}


void CMetalMap::RemoveExtraction(int x, int z, float depth)
{
	x = Clamp(x, 0, sizeX - 1);
	z = Clamp(z, 0, sizeZ - 1);

	extractionMap[(z * sizeX) + x] -= depth;
}


int CMetalMap::GetMetalExtraction(int x, int z) const
{
	x = Clamp(x, 0, sizeX - 1);
	z = Clamp(z, 0, sizeZ - 1);

	return extractionMap[(z * sizeX) + x];
}


#else


void CMetalMap::Init(const unsigned char* map, int _sizeX, int _sizeZ, float _metalScale) {}

float CMetalMap::GetMetalAmount(int x1, int z1, int x2, int z2) const { return 0.0f; }
float CMetalMap::GetMetalAmount(int x, int z) const { return 0.0f; }

void CMetalMap::SetMetalAmount(int x, int z, float m) {}
float CMetalMap::RequestExtraction(int x, int z, float toDepth) { return 0.0f; }
void CMetalMap::RemoveExtraction(int x, int z, float depth) {}
int CMetalMap::GetMetalExtraction(int x, int z) const { return 0; }
#endif

