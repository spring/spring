//////////////////////////////////////
//       CExtractorBuilding         //
// Used for all metal-extractors.   //
// Handling the metal-make-process. //
//////////////////////////////////////


#include "StdAfx.h"
#include "ExtractorBuilding.h"
#include "Sim/Units/UnitHandler.h"
#include "Map/ReadMap.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/CobFile.h"
#include "LogOutput.h"
#include "Sim/Units/UnitDef.h"
#include "Map/MetalMap.h"
#include "Sim/Misc/QuadField.h"
#include "SyncTracer.h"
#include "mmgr.h"

CR_BIND_DERIVED(CExtractorBuilding, CBuilding);

/*
Constructor
*/
CExtractorBuilding::CExtractorBuilding() :
	extractionRange(0),
	extractionDepth(0)
{
}


/*
Destructor
*/
CExtractorBuilding::~CExtractorBuilding() {
	//Leaving back the extraction-area.
	for(std::list<MetalSquareOfControl*>::iterator si = metalAreaOfControl.begin(); si != metalAreaOfControl.end(); ++si) {
		readmap->metalMap->removeExtraction((*si)->x, (*si)->z, (*si)->extractionDepth);
		delete *si;
	}
	//Tells the neighboors to take it over.
	for(std::list<CExtractorBuilding*>::iterator ei = neighboors.begin(); ei != neighboors.end(); ++ei) {
		(*ei)->RemoveNeighboor(this);
		(*ei)->ReCalculateMetalExtraction();
	}
}


/*
Sets the range of extraction for this extractor.
With this knowlege also finding intersected neighboors.
*/
void CExtractorBuilding::SetExtractionRangeAndDepth(float range, float depth) {
	extractionRange = range;
	extractionDepth = depth;

	//Finding neighboors
	std::vector<CUnit*> cu=qf->GetUnits(pos,extractionRange*2);

	for(std::vector<CUnit*>::iterator ui = cu.begin(); ui != cu.end(); ++ui) {
		if(typeid(**ui) == typeid(CExtractorBuilding) && *ui != this) {
			CExtractorBuilding *eb = (CExtractorBuilding*)*ui;
			if(eb->pos.distance2D(this->pos) < (eb->extractionRange + this->extractionRange)) {
				neighboors.push_back(eb);
				eb->AddNeighboor(this);
			}
		}
	}
	int nbr = neighboors.size();
//	logOutput << "Neighboors found: " << nbr << "\n";		//Debug

	//Calculating area of control and metalExtract.
	//TODO: Improve this method.
	metalExtract = 0;
	int xBegin = max(0,(int)((pos.x - extractionRange) / METAL_MAP_SQUARE_SIZE));
	int xEnd = min(gs->mapx/2-1,(int)((pos.x + extractionRange) / METAL_MAP_SQUARE_SIZE));
	int zBegin = max(0,(int)((pos.z - extractionRange) / METAL_MAP_SQUARE_SIZE));
	int zEnd = min(gs->mapy/2-1,(int)((pos.z + extractionRange) / METAL_MAP_SQUARE_SIZE));
	for(int x = xBegin; x <= xEnd; x++)
		for(int z = zBegin; z <= zEnd; z++) {	//Going thru the whole (x,z)-square...
			float3 msqrPos((x + 0.5) * METAL_MAP_SQUARE_SIZE, pos.y, (z + 0.5) * METAL_MAP_SQUARE_SIZE);	//Center of metalsquare.
			float sqrCenterDistance = msqrPos.distance2D(this->pos);
			if(sqrCenterDistance < extractionRange) {	//... and add whose within the circle.
				MetalSquareOfControl *msqr = new MetalSquareOfControl;
				msqr->x = x;
				msqr->z = z;
				msqr->extractionDepth = readmap->metalMap->requestExtraction(x, z, depth);	//Extraction is done in a cylinder
				metalAreaOfControl.push_back(msqr);
				metalExtract += msqr->extractionDepth * readmap->metalMap->getMetalAmount(msqr->x, msqr->z);
			}
		}
	nbr = metalAreaOfControl.size();
//	logOutput << "MetalSquares of control: " << nbr << "\n";	//Debug
}


/*
Adds a neighboor.
Most likely called from a newly built neighboor.
*/
void CExtractorBuilding::AddNeighboor(CExtractorBuilding *neighboor) {
	if(neighboor != this){
		for(std::list<CExtractorBuilding*>::iterator ei = neighboors.begin(); ei != neighboors.end(); ++ei) {
			if((*ei)==neighboor)
				return;
		}
		neighboors.push_back(neighboor);
	}
}


/*
Removes a neighboor.
Most likely called from a dying neighboor.
*/
void CExtractorBuilding::RemoveNeighboor(CExtractorBuilding* neighboor) {
	neighboors.remove(neighboor);
}


/*
Recalculate metalExtract.
Most likely called by a dying neighboor.
*/
void CExtractorBuilding::ReCalculateMetalExtraction() {
	metalExtract = 0;
	for(std::list<MetalSquareOfControl*>::iterator si = metalAreaOfControl.begin(); si != metalAreaOfControl.end(); ++si) {
		MetalSquareOfControl *msqr = *si;
		readmap->metalMap->removeExtraction(msqr->x, msqr->z, msqr->extractionDepth);

		msqr->extractionDepth = readmap->metalMap->requestExtraction(msqr->x, msqr->z, extractionDepth);	//Extraction is done in a cylinder
		metalExtract += msqr->extractionDepth * readmap->metalMap->getMetalAmount(msqr->x, msqr->z);
	}

	//Sets the new rotation-speed.
	cob->Call(COBFN_SetSpeed, (int)(metalExtract*100.0f));
	cob->Call("Go");

}


/*
Finds the amount of metal to extract and sets the rotationspeed when the extractor are built.
*/
void CExtractorBuilding::FinishedBuilding() {
	SetExtractionRangeAndDepth(unitDef->extractRange, unitDef->extractsMetal);

#ifdef TRACE_SYNC
		tracefile << "Metal extractor finished: ";
		tracefile << metalExtract << " ";
#endif

	cob->Call(COBFN_SetSpeed, (int)(metalExtract*100.0f));
	CBuilding::FinishedBuilding();
}
