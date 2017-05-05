/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EXTRACTOR_BUILDING_H
#define _EXTRACTOR_BUILDING_H

#include <vector>

#include "Building.h"

class CExtractorBuilding : public CBuilding {
public:
	CR_DECLARE(CExtractorBuilding)
	CR_DECLARE_SUB(MetalSquareOfControl)

	CExtractorBuilding(): CBuilding() {
		extractionRange = 0.0f;
		extractionDepth = 0.0f;
	}
	~CExtractorBuilding();

	void ResetExtraction();
	void SetExtractionRangeAndDepth(float range, float depth);
	void ReCalculateMetalExtraction();
	bool IsNeighbour(CExtractorBuilding* neighbour);
	void AddNeighbour(CExtractorBuilding* neighbour);
	void RemoveNeighbour(CExtractorBuilding* neighbour);

	float GetExtractionRange() const { return extractionRange; }
	float GetExtractionDepth() const { return extractionDepth; }

	void FinishedBuilding(bool postInit);

protected:
	struct MetalSquareOfControl {
		CR_DECLARE_STRUCT(MetalSquareOfControl)
		int x;
		int z;
		float extractionDepth;
	};

	float extractionRange, extractionDepth;
	std::vector<MetalSquareOfControl> metalAreaOfControl;
	std::vector<CExtractorBuilding*> neighbours;

	static float maxExtractionRange;
};

#endif // _EXTRACTOR_BUILDING_H
