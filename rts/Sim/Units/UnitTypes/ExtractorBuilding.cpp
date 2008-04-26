//////////////////////////////////////
//       CExtractorBuilding         //
// Used for all metal-extractors.   //
// Handling the metal-make-process. //
//////////////////////////////////////


#include "StdAfx.h"
#include <typeinfo>
#include "ExtractorBuilding.h"
#include "Sim/Units/UnitHandler.h"
#include "Map/ReadMap.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/CobFile.h"
#include "LogOutput.h"
#include "Sim/Units/UnitDef.h"
#include "Map/MetalMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sync/SyncTracer.h"
#include "creg/STL_List.h"
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


/*
Constructor
*/
CExtractorBuilding::CExtractorBuilding() :
	extractionRange(0.0f),
	extractionDepth(0.0f)
{
}


/*
Destructor
*/
CExtractorBuilding::~CExtractorBuilding()
{
	ResetExtraction();
}

/*
CReg PostLoad
*/
void CExtractorBuilding::PostLoad()
{
	cob->Call(COBFN_SetSpeed, (int)(metalExtract *5 * 100.0f));
}

/*
Resets the metalMap database
Notifies the neighbours
*/
void CExtractorBuilding::ResetExtraction()
{
	// Leaving back the extraction-area.
	for(std::list<MetalSquareOfControl*>::iterator si = metalAreaOfControl.begin(); si != metalAreaOfControl.end(); ++si) {
		readmap->metalMap->removeExtraction((*si)->x, (*si)->z, (*si)->extractionDepth);
		delete *si;
	}
	metalAreaOfControl.clear();

	// Tells the neighbours to take it over.
	for(std::list<CExtractorBuilding*>::iterator ei = neighbours.begin(); ei != neighbours.end(); ++ei) {
		(*ei)->RemoveNeighboor(this);
		(*ei)->ReCalculateMetalExtraction();
	}
	neighbours.clear();
}


/*
Sets the range of extraction for this extractor.
With this knowlege also finding intersected neighbours.
*/
void CExtractorBuilding::SetExtractionRangeAndDepth(float range, float depth)
{
	extractionRange = range;
	extractionDepth = depth;

	//Finding neighbours
	std::vector<CUnit*> cu = qf->GetUnits(pos, extractionRange + maxExtractionRange);
	maxExtractionRange = std::max(extractionRange, maxExtractionRange);

	for (std::vector<CUnit*>::iterator ui = cu.begin(); ui != cu.end(); ++ui) {
		if(typeid(**ui) == typeid(CExtractorBuilding) && *ui != this) {
			CExtractorBuilding *eb = (CExtractorBuilding*)*ui;
			if(eb->pos.distance2D(this->pos) < (eb->extractionRange + this->extractionRange)) {
				neighbours.push_back(eb);
				eb->AddNeighboor(this);
			}
		}
	}
	int nbr = neighbours.size();
//	logOutput << "Neighboors found: " << nbr << "\n";		//Debug

	//Calculating area of control and metalExtract.
	//TODO: Improve this method.
	metalExtract = 0;
	int xBegin = std::max(0,           (int)((pos.x - extractionRange) / METAL_MAP_SQUARE_SIZE));
	int xEnd   = std::min(gs->mapx/2-1,(int)((pos.x + extractionRange) / METAL_MAP_SQUARE_SIZE));
	int zBegin = std::max(0,           (int)((pos.z - extractionRange) / METAL_MAP_SQUARE_SIZE));
	int zEnd   = std::min(gs->mapy/2-1,(int)((pos.z + extractionRange) / METAL_MAP_SQUARE_SIZE));

	for (int x = xBegin; x <= xEnd; x++) {
		for (int z = zBegin; z <= zEnd; z++) {
			// go through the whole (x, z)-square
			const float3 msqrPos((x + 0.5f) * METAL_MAP_SQUARE_SIZE, pos.y,
			                     (z + 0.5f) * METAL_MAP_SQUARE_SIZE);	//Center of metalsquare.
			const float sqrCenterDistance = msqrPos.distance2D(this->pos);
			if (sqrCenterDistance < extractionRange) {	//... and add whose within the circle.
				MetalSquareOfControl *msqr = SAFE_NEW MetalSquareOfControl;
				msqr->x = x;
				msqr->z = z;
				msqr->extractionDepth = readmap->metalMap->requestExtraction(x, z, depth);	//Extraction is done in a cylinder
				metalAreaOfControl.push_back(msqr);
				metalExtract += msqr->extractionDepth * readmap->metalMap->getMetalAmount(msqr->x, msqr->z);
			}
		}
	}
	nbr = metalAreaOfControl.size();

//	logOutput << "MetalSquares of control: " << nbr << "\n";	//Debug

	// Set the COB animation speed
	cob->Call(COBFN_SetSpeed, (int)(metalExtract *5 * 100.0f));
	if (activated) {
		cob->Call("Go");
	}
}


/*
Adds a neighboor.
Most likely called from a newly built neighboor.
*/
void CExtractorBuilding::AddNeighboor(CExtractorBuilding *neighboor)
{
	if(neighboor != this){
		for(std::list<CExtractorBuilding*>::iterator ei = neighbours.begin(); ei != neighbours.end(); ++ei) {
			if((*ei)==neighboor)
				return;
		}
		neighbours.push_back(neighboor);
	}
}


/*
Removes a neighboor.
Most likely called from a dying neighboor.
*/
void CExtractorBuilding::RemoveNeighboor(CExtractorBuilding* neighboor)
{
	neighbours.remove(neighboor);
}


/*
Recalculate metalExtract.
Most likely called by a dying neighboor.
*/
void CExtractorBuilding::ReCalculateMetalExtraction()
{
	metalExtract = 0;
	for(std::list<MetalSquareOfControl*>::iterator si = metalAreaOfControl.begin(); si != metalAreaOfControl.end(); ++si) {
		MetalSquareOfControl *msqr = *si;
		readmap->metalMap->removeExtraction(msqr->x, msqr->z, msqr->extractionDepth);

		msqr->extractionDepth = readmap->metalMap->requestExtraction(msqr->x, msqr->z, extractionDepth);	//Extraction is done in a cylinder
		metalExtract += msqr->extractionDepth * readmap->metalMap->getMetalAmount(msqr->x, msqr->z);
	}

	//Sets the new rotation-speed.
	cob->Call(COBFN_SetSpeed, (int)(metalExtract*5*100.0f));
	if (activated) {
		cob->Call("Go");
	}
}


/*
Finds the amount of metal to extract and sets the rotationspeed when the extractor are built.
*/
void CExtractorBuilding::FinishedBuilding()
{
	SetExtractionRangeAndDepth(unitDef->extractRange, unitDef->extractsMetal);

#ifdef TRACE_SYNC
	tracefile << "Metal extractor finished: ";
	tracefile << metalExtract << " ";
#endif

	CBuilding::FinishedBuilding();
}
