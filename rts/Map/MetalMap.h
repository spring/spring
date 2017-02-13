/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef METAL_MAP_H
#define METAL_MAP_H

#include <vector>

#include "System/creg/creg_cond.h"
#include "Sim/Misc/GlobalConstants.h"

// Each square on metalmap is a 2x2 square on normal map.
static const float METAL_MAP_SQUARE_SIZE = SQUARE_SIZE * 2;


class CMetalMap
{
	CR_DECLARE_STRUCT(CMetalMap)

public:
	/** Receiving a map over all metal, and creating a map over extraction. */
	CMetalMap(const unsigned char* map, int sizeX, int sizeZ, float metalScale);

	/** Returns the amount of metal over an area. */
	float GetMetalAmount(int x1, int z1, int x2, int z2);
	/** Returns the amount of metal on a single square. */
	float GetMetalAmount(int x, int z);
	/** Sets the amount of metal on a single square. */
	void SetMetalAmount(int x, int z, float m);
	/**
	 * Makes a request for extracting metal from a given square.
	 * If there is metal left to extract to the requested depth,
	 * the amount available will be returned and the requested
	 * depth will be sat as new extraction-depth on the extraction-map.
	 * If the requested depth is greater than the current
	 * extraction-depth 0.0 will be returned and nothing changed.
	 */
	float RequestExtraction(int x, int z, float toDepth);
	/**
	 * When a extraction ends, the digged depth should be left
	 * back to the extraction-map. To be available for other
	 * extractors to use.
	 */
	void RemoveExtraction(int x, int z, float depth);

	int GetMetalExtraction(int x, int z);

	int GetSizeX() const { return sizeX; }
	int GetSizeZ() const { return sizeZ; }

	const unsigned char* GetTexturePalette () const { return        &metalPal[0]; }
	const unsigned char* GetDistributionMap() const { return &distributionMap[0]; }
	const         float* GetExtractionMap  () const { return   &extractionMap[0]; }

protected:
	float metalScale;
	int sizeX;
	int sizeZ;

private:
	unsigned char metalPal[768];

	std::vector<unsigned char> distributionMap;
	std::vector<        float> extractionMap;
};

#endif /* METAL_MAP_H */
