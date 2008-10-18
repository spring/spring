#ifndef __EXTRACTOR_BUILDING_H__
#define __EXTRACTOR_BUILDING_H__

#include "Building.h"

class CExtractorBuilding: public CBuilding {
public:
	CR_DECLARE(CExtractorBuilding);
	CR_DECLARE_SUB(MetalSquareOfControl);

	CExtractorBuilding();
	virtual ~CExtractorBuilding();
	void PostLoad();

	void ResetExtraction();
	void SetExtractionRangeAndDepth(float range, float depth);
	void ReCalculateMetalExtraction();
	bool IsNeighbour(CExtractorBuilding* neighbour);
	void AddNeighbour(CExtractorBuilding* neighbour);
	void RemoveNeighbour(CExtractorBuilding* neighbour);

	float GetExtractionRange() const { return extractionRange; }
	float GetExtractionDepth() const { return extractionDepth; }

	virtual void FinishedBuilding();

protected:
	struct MetalSquareOfControl {
		CR_DECLARE_STRUCT(MetalSquareOfControl);
		int x;
		int z;
		float extractionDepth;
	};

	float extractionRange, extractionDepth;
	std::vector<MetalSquareOfControl> metalAreaOfControl;
	std::list<CExtractorBuilding*> neighbours;

	static float maxExtractionRange;
};

#endif // __EXTRACTOR_BUILDING_H__
