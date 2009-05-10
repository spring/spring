//////////////////////////////////////
//       CExtractorBuilding         //
// Used for all metal-extractors.   //
// Handles the metal-make-process.  //
//////////////////////////////////////


#include "StdAfx.h"
#include <typeinfo>
#include "ExtractorBuilding.h"
#include "Sim/Units/COB/UnitScript.h"
#include "Sim/Units/UnitHandler.h"
#include "Map/ReadMap.h"
#include "LogOutput.h"
#include "Sim/Units/UnitDef.h"
#include "Map/MetalMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sync/SyncTracer.h"
#include "creg/STL_List.h"
#include "myMath.h"
#include "mmgr.h"

CR_BIND_DERIVED(CExtractorBuilding, CBuilding, );

CR_REG_METADATA(CExtractorBuilding, (
	CR_MEMBER(extractionRange),
	CR_MEMBER(extractionDepth),
	CR_MEMBER(metalAreaOfControl),
	CR_MEMBER(neighbours),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(CExtractorBuilding::MetalSquareOfControl, );

CR_REG_METADATA_SUB(CExtractorBuilding,MetalSquareOfControl, (
	CR_MEMBER(x),
	CR_MEMBER(z),
	CR_MEMBER(extractionDepth)
));

// TODO: How are class statics incorporated into creg?
float CExtractorBuilding::maxExtractionRange = 0.0f;

CExtractorBuilding::CExtractorBuilding():
	extractionRange(0.0f), extractionDepth(0.0f)
{
}

CExtractorBuilding::~CExtractorBuilding()
{
	// if uh == NULL then all pointers to units should be considered dangling pointers
	if (uh) {
		ResetExtraction();
	}
}

/* CReg PostLoad */
void CExtractorBuilding::PostLoad()
{
	script->SetSpeed(metalExtract, 500.0f);
}

/* resets the metalMap and notifies the neighbours */
void CExtractorBuilding::ResetExtraction()
{
	// undo the extraction-area
	for(std::vector<MetalSquareOfControl>::iterator si = metalAreaOfControl.begin(); si != metalAreaOfControl.end(); ++si) {
		readmap->metalMap->RemoveExtraction(si->x, si->z, si->extractionDepth);
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
	const int sum = int(this->unitDef->extractSquare) + int(other->unitDef->extractSquare);

	if (sum == 2) {
		// square vs. square
		const float dx = streflop::fabs(this->pos.x - other->pos.x);
		const float dz = streflop::fabs(this->pos.z - other->pos.z);
		const float r = this->extractionRange + other->extractionRange;
		return (dx < r && dz < r);
	}
	if (sum == 1) {
		// circle vs. square or square vs. circle
		const CExtractorBuilding* square = (this->unitDef->extractSquare)? this: other;
		const CExtractorBuilding* circle = (this->unitDef->extractSquare)? other: this;

		const float3 p0 = square->pos + float3(-square->extractionRange, 0.0f, -square->extractionRange); // top-left
		const float3 p1 = square->pos + float3( square->extractionRange, 0.0f, -square->extractionRange); // top-right
		const float3 p2 = square->pos + float3( square->extractionRange, 0.0f,  square->extractionRange); // bottom-right
		const float3 p3 = square->pos + float3(-square->extractionRange, 0.0f,  square->extractionRange); // bottom-left
		const float3 p4 = circle->pos + float3( circle->extractionRange, 0.0f,                     0.0f); //   0
		const float3 p5 = circle->pos + float3(                    0.0f, 0.0f, -circle->extractionRange); //  90
		const float3 p6 = circle->pos + float3(-circle->extractionRange, 0.0f,                     0.0f); // 180
		const float3 p7 = circle->pos + float3(                    0.0f, 0.0f,  circle->extractionRange); // 270

		// square corners must all lie outside circle
		const bool b0 = (p0.SqDistance2D(circle->pos) > Square(circle->extractionRange));
		const bool b1 = (p1.SqDistance2D(circle->pos) > Square(circle->extractionRange));
		const bool b2 = (p2.SqDistance2D(circle->pos) > Square(circle->extractionRange));
		const bool b3 = (p3.SqDistance2D(circle->pos) > Square(circle->extractionRange));
		// circle "corners" must all lie outside square
		const bool b4 = ((p4.x < p0.x || p4.x > p1.x) && (p4.z < p0.z || p4.z > p3.z));
		const bool b5 = ((p5.x < p0.x || p5.x > p1.x) && (p5.z < p0.z || p5.z > p3.z));
		const bool b6 = ((p6.x < p0.x || p6.x > p1.x) && (p6.z < p0.z || p6.z > p3.z));
		const bool b7 = ((p7.x < p0.x || p7.x > p1.x) && (p7.z < p0.z || p7.z > p3.z));

		return !(b0 && b1 && b2 && b3  &&  b4 && b5 && b6 && b7);
	}
	if (sum == 0) {
		// circle vs. circle
		return (this->pos.SqDistance2D(other->pos) < Square(this->extractionRange + other->extractionRange));
	}

	return false;
}

/* sets the range of extraction for this extractor, also finds overlapping neighbours. */
void CExtractorBuilding::SetExtractionRangeAndDepth(float range, float depth)
{
	extractionRange = range;
	extractionDepth = depth;

	// find any neighbouring extractors
	std::vector<CUnit*> cu = qf->GetUnits(pos, extractionRange + maxExtractionRange);
	maxExtractionRange = std::max(extractionRange, maxExtractionRange);

	for (std::vector<CUnit*>::iterator ui = cu.begin(); ui != cu.end(); ++ui) {
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
	int xEnd   = std::min(gs->mapx / 2 - 1, (int) ((pos.x + extractionRange) / METAL_MAP_SQUARE_SIZE));
	int zBegin = std::max(0,                (int) ((pos.z - extractionRange) / METAL_MAP_SQUARE_SIZE));
	int zEnd   = std::min(gs->mapy / 2 - 1, (int) ((pos.z + extractionRange) / METAL_MAP_SQUARE_SIZE));

	metalAreaOfControl.reserve((xEnd - xBegin + 1) * (zEnd - zBegin + 1));

	// go through the whole (x, z)-square
	for (int x = xBegin; x <= xEnd; x++) {
		for (int z = zBegin; z <= zEnd; z++) {
			// center of metalsquare at (x, z)
			const float3 msqrPos((x + 0.5f) * METAL_MAP_SQUARE_SIZE, pos.y,
			                     (z + 0.5f) * METAL_MAP_SQUARE_SIZE);
			const float sqrCenterDistance = msqrPos.SqDistance2D(this->pos);

			if (unitDef->extractSquare || sqrCenterDistance < Square(extractionRange)) {
				MetalSquareOfControl msqr;
				msqr.x = x;
				msqr.z = z;
				// extraction is done in a cylinder of height <depth>
				msqr.extractionDepth = readmap->metalMap->RequestExtraction(x, z, depth);
				metalAreaOfControl.push_back(msqr);
				metalExtract += msqr.extractionDepth * readmap->metalMap->GetMetalAmount(msqr.x, msqr.z);
			}
		}
	}

	// set the COB animation speed
	script->SetSpeed(metalExtract, 500.0f);
	if (activated) {
		script->Go();
	}
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
		readmap->metalMap->RemoveExtraction(msqr.x, msqr.z, msqr.extractionDepth);

		// extraction is done in a cylinder
		msqr.extractionDepth = readmap->metalMap->RequestExtraction(msqr.x, msqr.z, extractionDepth);
		metalExtract += msqr.extractionDepth * readmap->metalMap->GetMetalAmount(msqr.x, msqr.z);
	}

	// set the new rotation-speed
	script->SetSpeed(metalExtract, 500.0f);
	if (activated) {
		script->Go();
	}
}


/* Finds the amount of metal to extract and sets the rotationspeed when the extractor is built. */
void CExtractorBuilding::FinishedBuilding()
{
	SetExtractionRangeAndDepth(unitDef->extractRange, unitDef->extractsMetal);

#ifdef TRACE_SYNC
	tracefile << "Metal extractor finished: ";
	tracefile << metalExtract << " ";
#endif

	CBuilding::FinishedBuilding();
}
