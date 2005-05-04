#ifndef EXTRACTOR_BUILDING_H
#define EXTRACTOR_BUILDING_H

#include "Building.h"

class CExtractorBuilding : public CBuilding {
public:
	CExtractorBuilding(const float3 &pos, int team,UnitDef* unitDef);
	virtual ~CExtractorBuilding();

	void SetExtractionRangeAndDepth(float range, float depth);
	void ReCalculateMetalExtraction();
	void AddNeighboor(CExtractorBuilding* neighboor);
	void RemoveNeighboor(CExtractorBuilding* neighboor);

	virtual void FinishedBuilding();

protected:
	struct MetalSquareOfControl {
		int x;
		int z;
		float extractionDepth;
	};

	float extractionRange, extractionDepth;
	std::list<MetalSquareOfControl*> metalAreaOfControl;
	std::list<CExtractorBuilding*> neighboors;
};

#endif