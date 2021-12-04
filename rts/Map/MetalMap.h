/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef METAL_MAP_H
#define METAL_MAP_H

#include <array>
#include <vector>

#include "System/creg/creg_cond.h"
#include "Sim/Misc/GlobalConstants.h"

// each metalmap square covers 2x2 normal squares
static constexpr float METAL_MAP_SQUARE_SIZE = SQUARE_SIZE * 2;


class CMetalMap
{
	CR_DECLARE_STRUCT(CMetalMap)

public:
	/** Receiving a map over all metal, and creating a map over extraction. */
	void Init(const unsigned char* map, int sizeX, int sizeZ, float metalScale);
	void Kill() {
		distributionMap.clear();
		extractionMap.clear();
	}

	/** Returns the amount of metal over an area. */
	float GetMetalAmount(int x1, int z1, int x2, int z2) const;
	/** Returns the amount of metal on a single square. */
	float GetMetalAmount(int x, int z) const;
	/** Sets the amount of metal on a single square. */
	void SetMetalAmount(int x, int z, float m);
	/**
	 * Makes a request for extracting metal from a given square.
	 * If there is metal left to extract to the requested depth,
	 * the amount available will be returned and the requested
	 * depth will be set as new extraction-depth on the extraction-map.
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

	int GetMetalExtraction(int x, int z) const;

	int GetSizeX() const { return sizeX; }
	int GetSizeZ() const { return sizeZ; }

	const unsigned char* GetTexturePalette () const { return  texturePalette.data(); }
	const unsigned char* GetDistributionMap() const { return distributionMap.data(); }
	const         float* GetExtractionMap  () const { return   extractionMap.data(); }

private:
	std::array<unsigned char, 256 * 3> texturePalette;

	std::vector<unsigned char> distributionMap;
	std::vector<        float> extractionMap;

	float metalScale = 0.0f;

	int sizeX = 0;
	int sizeZ = 0;
};

extern CMetalMap metalMap;

#endif

