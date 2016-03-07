/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// Used for all metal-extractors.
// Handles the metal-make-process.

#include <typeinfo>
#include "ExtractorBuilding.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/UnitHandler.h"
#include "Map/ReadMap.h"
#include "Sim/Units/UnitDef.h"
#include "Map/MetalMap.h"
#include "Sim/Misc/QuadField.h"
#include "System/Sync/SyncTracer.h"
#include "System/creg/STL_List.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CExtractorBuilding, CBuilding, )

CR_REG_METADATA(CExtractorBuilding, (
	CR_MEMBER(extractionRange),
	CR_MEMBER(extractionDepth),
	CR_MEMBER(metalAreaOfControl),
	CR_MEMBER(neighbours)
))

CR_BIND(CExtractorBuilding::MetalSquareOfControl, )

CR_REG_METADATA_SUB(CExtractorBuilding,MetalSquareOfControl, (
	CR_MEMBER(x),
	CR_MEMBER(z),
	CR_MEMBER(extractionDepth)
))

// TODO: How are class statics incorporated into creg?
float CExtractorBuilding::maxExtractionRange = 0.0f;

CExtractorBuilding::CExtractorBuilding():
	extractionRange(0.0f), extractionDepth(0.0f)
{
}

CExtractorBuilding::~CExtractorBuilding()
{
	// if uh == NULL then all pointers to units should be considered dangling pointers
	if (unitHandler != NULL) {
		ResetExtraction();
	}
}

/* resets the metalMap and notifies the neighbours */
void CExtractorBuilding::ResetExtraction()
{
	// undo the extraction-area
	for(std::vector<MetalSquareOfControl>::iterator si = metalAreaOfControl.begin(); si != metalAreaOfControl.end(); ++si) {
		readMap->metalMap->RemoveExtraction(si->x, si->z, si->extractionDepth);
	}

	metalAreaOfControl.clear();

	// tell the neighbours (if any) to take it over
	for (std::list<CExtractorBuilding*>::iterator ei = neighbours.begin(); ei != neighbours.end(); ++ei) {
		(*ei)->RemoveNeighbour(this);
		(*ei)->ReCalculateMetalExtraction();
	}
	neighbours.clear();
}



/* determine if two extraction areas overlap */
bool CExtractorBuilding::IsNeighbour(CExtractorBuilding* other)
{
	// circle vs. circle
	return (this->pos.SqDistance2D(other->pos) < Square(this->extractionRange + other->extractionRange));
}

/* sets the range of extraction for this extractor, also finds overlapping neighbours. */
void CExtractorBuilding::SetExtractionRangeAndDepth(float range, float depth)
{
	extractionRange = std::max(range, 0.001f);
	extractionDepth = std::max(depth, 0.0f);
	maxExtractionRange = std::max(extractionRange, maxExtractionRange);

	// find any neighbouring extractors
	const std::vector<CUnit*> &cu = quadField->GetUnits(pos, extractionRange + maxExtractionRange);

	for (std::vector<CUnit*>::const_iterator ui = cu.begin(); ui != cu.end(); ++ui) {
		if (typeid(**ui) == typeid(CExtractorBuilding) && *ui != this) {
			CExtractorBuilding* eb = (CExtractorBuilding*) *ui;

			if (IsNeighbour(eb)) {
				neighbours.push_back(eb);
				eb->AddNeighbour(this);
			}
		}
	}

	// calculate this extractor's area of control and metalExtract amount
	metalExtract = 0;
	int xBegin = std::max(0,                (int) ((pos.x - extractionRange) / METAL_MAP_SQUARE_SIZE));
	int xEnd   = std::min(mapDims.mapx / 2 - 1, (int) ((pos.x + extractionRange) / METAL_MAP_SQUARE_SIZE));
	int zBegin = std::max(0,                (int) ((pos.z - extractionRange) / METAL_MAP_SQUARE_SIZE));
	int zEnd   = std::min(mapDims.mapy / 2 - 1, (int) ((pos.z + extractionRange) / METAL_MAP_SQUARE_SIZE));

	metalAreaOfControl.reserve((xEnd - xBegin + 1) * (zEnd - zBegin + 1));

	// go through the whole (x, z)-square
	for (int x = xBegin; x <= xEnd; x++) {
		for (int z = zBegin; z <= zEnd; z++) {
			// center of metalsquare at (x, z)
			const float3 msqrPos((x + 0.5f) * METAL_MAP_SQUARE_SIZE, pos.y,
			                     (z + 0.5f) * METAL_MAP_SQUARE_SIZE);
			const float sqrCenterDistance = msqrPos.SqDistance2D(this->pos);

			if (sqrCenterDistance < Square(extractionRange)) {
				MetalSquareOfControl msqr;
				msqr.x = x;
				msqr.z = z;
				// extraction is done in a cylinder of height <depth>
				msqr.extractionDepth = readMap->metalMap->RequestExtraction(x, z, depth);
				metalAreaOfControl.push_back(msqr);
				metalExtract += msqr.extractionDepth * readMap->metalMap->GetMetalAmount(msqr.x, msqr.z);
			}
		}
	}

	// set the COB animation speed
	script->ExtractionRateChanged(metalExtract);
}


/* adds a neighbour for this extractor */
void CExtractorBuilding::AddNeighbour(CExtractorBuilding* neighboor)
{
	if (neighboor != this) {
		for (std::list<CExtractorBuilding*>::iterator ei = neighbours.begin(); ei != neighbours.end(); ++ei) {
			if ((*ei) == neighboor)
				return;
		}
		neighbours.push_back(neighboor);
	}
}


/* removes a neighboor for this extractor */
void CExtractorBuilding::RemoveNeighbour(CExtractorBuilding* neighboor)
{
	neighbours.remove(neighboor);
}


/* recalculate metalExtract for this extractor (eg. when a neighbour dies) */
void CExtractorBuilding::ReCalculateMetalExtraction()
{
	metalExtract = 0;
	for (std::vector<MetalSquareOfControl>::iterator si = metalAreaOfControl.begin(); si != metalAreaOfControl.end(); ++si) {
		MetalSquareOfControl& msqr = *si;
		readMap->metalMap->RemoveExtraction(msqr.x, msqr.z, msqr.extractionDepth);

		// extraction is done in a cylinder
		msqr.extractionDepth = readMap->metalMap->RequestExtraction(msqr.x, msqr.z, extractionDepth);
		metalExtract += msqr.extractionDepth * readMap->metalMap->GetMetalAmount(msqr.x, msqr.z);
	}

	// set the new rotation-speed
	script->ExtractionRateChanged(metalExtract);
}


/* Finds the amount of metal to extract and sets the rotationspeed when the extractor is built. */
void CExtractorBuilding::FinishedBuilding(bool postInit)
{
	SetExtractionRangeAndDepth(unitDef->extractRange, unitDef->extractsMetal);
	CUnit::FinishedBuilding(postInit);
}
