/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NodeLayer.hpp"
#include "PathManager.hpp"
#include "Node.hpp"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/myMath.h"
#include "System/Rectangle.h"

// NOTE:
//   we need a fixed range that does not become wider / narrower
//   during terrain deformations (otherwise the bins would change
//   across ALL nodes)
#define PM QTPFS::PathManager
#define PM_SPEEDMOD_RANGE (PM::MAX_SPEEDMOD_VALUE - PM::MIN_SPEEDMOD_VALUE)

static int GetSpeedModBin(float absSpeedMod, float relSpeedMod) {
	// NOTE:
	//     bins N and N+1 are reserved for modifiers <= min and >= max
	//     respectively; blocked squares MUST be in their own category
	const int defBin = int(PM::NUM_SPEEDMOD_BINS * relSpeedMod);
	const int maxBin = int(PM::NUM_SPEEDMOD_BINS - 1);

	int speedModBin = Clamp(defBin, 0, maxBin);

	if (absSpeedMod <= PM::MIN_SPEEDMOD_VALUE) { speedModBin = PM::NUM_SPEEDMOD_BINS + 0; }
	if (absSpeedMod >= PM::MAX_SPEEDMOD_VALUE) { speedModBin = PM::NUM_SPEEDMOD_BINS + 1; }

	return speedModBin;
}



void QTPFS::NodeLayer::RegisterNode(INode* n) {
	for (unsigned int hmx = n->xmin(); hmx < n->xmax(); hmx++) {
		for (unsigned int hmz = n->zmin(); hmz < n->zmax(); hmz++) {
			nodeGrid[hmz * xsize + hmx] = n;
		}
	}
}

void QTPFS::NodeLayer::Init(unsigned int layerNum) {
	// pre-count the root
	numLeafNodes = 1;
	layerNumber = layerNum;

	xsize = gs->mapx;
	zsize = gs->mapy;

	nodeGrid.resize(xsize * zsize, NULL);

	curSpeedMods.resize(xsize * zsize, 0.0f);
	oldSpeedMods.resize(xsize * zsize, 0.0f);
	oldSpeedBins.resize(xsize * zsize, -1);
	curSpeedBins.resize(xsize * zsize, -1);
}

void QTPFS::NodeLayer::Clear() {
	nodeGrid.clear();

	curSpeedMods.clear();
	oldSpeedMods.clear();
	oldSpeedBins.clear();
	curSpeedBins.clear();

	#ifdef QTPFS_STAGGERED_LAYER_UPDATES
	layerUpdates.clear();
	#endif
}



#ifdef QTPFS_STAGGERED_LAYER_UPDATES
void QTPFS::NodeLayer::QueueUpdate(const SRectangle& r, const MoveDef* md, const CMoveMath* mm) {
	layerUpdates.push_back(LayerUpdate());
	LayerUpdate* layerUpdate = &(layerUpdates.back());

	layerUpdate->rectangle = r;
	layerUpdate->speedMods.resize(r.GetArea());
	layerUpdate->blockBits.resize(r.GetArea());

	#ifdef QTPFS_IGNORE_MAP_EDGES
	const unsigned int md_xsizeh = md->xsizeh;
	const unsigned int md_zsizeh = md->zsizeh;
	#endif

	// make a snapshot of the terrain-state within <r>
	for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
		for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			#ifdef QTPFS_IGNORE_MAP_EDGES
			const unsigned int chmx = Clamp(hmx, md_xsizeh, r.x2 - md_xsizeh - 1);
			const unsigned int chmz = Clamp(hmz, md_zsizeh, r.z2 - md_zsizeh - 1);
			#else
			const unsigned int chmx = hmx;
			const unsigned int chmz = hmz;
			#endif

			layerUpdate->speedMods[recIdx] = mm->GetPosSpeedMod(*md, chmx, chmz, NULL);
			layerUpdate->blockBits[recIdx] = mm->IsBlockedNoSpeedModCheck(*md, chmx, chmz, NULL);
		}
	}
}

bool QTPFS::NodeLayer::ExecQueuedUpdate() {
	const LayerUpdate& layerUpdate = layerUpdates.front();
	const SRectangle& rectangle = layerUpdate.rectangle;
	const std::vector<float>* speedMods = &layerUpdate.speedMods;
	const std::vector<int>* blockBits = &layerUpdate.blockBits;
	const bool ret = Update(rectangle, NULL, NULL, speedMods, blockBits);

	return ret;
}
#endif



bool QTPFS::NodeLayer::Update(
	const SRectangle& r,
	const MoveDef* md,
	const CMoveMath* mm,
	const std::vector<float>* luSpeedMods,
	const std::vector<int>* luBlockBits
) {
	unsigned int numNewBinSquares = 0;

	#ifdef QTPFS_IGNORE_MAP_EDGES
	const unsigned int md_xsizeh = (md == NULL)? 0: md->xsizeh;
	const unsigned int md_zsizeh = (md == NULL)? 0: md->zsizeh;
	#endif

	// divide speed-modifiers into bins
	for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
		for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
			const unsigned int sqrIdx = hmz * xsize + hmx;
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			#ifdef QTPFS_IGNORE_MAP_EDGES
			const unsigned int chmx = Clamp(hmx, md_xsizeh, r.x2 - md_xsizeh - 1);
			const unsigned int chmz = Clamp(hmz, md_zsizeh, r.z2 - md_zsizeh - 1);
			#else
			const unsigned int chmx = hmx;
			const unsigned int chmz = hmz;
			#endif

			// NOTE:
			//     GetPosSpeedMod only checks the terrain (height/slope/type), not the blocking-map
			//     IsBlockedNoSpeedModCheck scans entire footprint, GetPosSpeedMod just one square
			const unsigned int blockBits = (luBlockBits == NULL)? mm->IsBlockedNoSpeedModCheck(*md, chmx, chmz, NULL): (*luBlockBits)[recIdx];

			const float rawAbsSpeedMod = (luSpeedMods == NULL)? mm->GetPosSpeedMod(*md, chmx, chmz): (*luSpeedMods)[recIdx];
			const float tmpAbsSpeedMod = Clamp(rawAbsSpeedMod, PM::MIN_SPEEDMOD_VALUE, PM::MAX_SPEEDMOD_VALUE);
			const float newAbsSpeedMod = ((blockBits & CMoveMath::BLOCK_STRUCTURE) == 0)? tmpAbsSpeedMod: 0.0f;
			const float newRelSpeedMod = (newAbsSpeedMod - PM::MIN_SPEEDMOD_VALUE) / PM_SPEEDMOD_RANGE;
			const float curRelSpeedMod = curSpeedMods[sqrIdx];

			const int newSpeedModBin = GetSpeedModBin(newAbsSpeedMod, newRelSpeedMod);
			const int curSpeedModBin = curSpeedBins[sqrIdx];

			numNewBinSquares += (newSpeedModBin != curSpeedModBin)? 1: 0;

			// need to keep track of these for Tesselate
			oldSpeedMods[sqrIdx] = curRelSpeedMod;
			curSpeedMods[sqrIdx] = newRelSpeedMod;

			oldSpeedBins[sqrIdx] = curSpeedModBin;
			curSpeedBins[sqrIdx] = newSpeedModBin;
		}
	}

	// if at least one square changed bin, we need to re-tesselate
	// all nodes in the subtree of the deepest-level node that fully
	// contains <r>
	//
	// during initialization of the root this is true for ALL squares,
	// but we might NOT need to split it (ex. if the map is 100% flat)
	// if each square happened to change to the SAME bin
	//
	return (numNewBinSquares > 0);
}

