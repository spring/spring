/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <limits>

#include "NodeLayer.hpp"
#include "PathManager.hpp"
#include "Node.hpp"

#include "Map/MapInfo.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/myMath.h"

unsigned int QTPFS::NodeLayer::NUM_SPEEDMOD_BINS;
float        QTPFS::NodeLayer::MIN_SPEEDMOD_VALUE;
float        QTPFS::NodeLayer::MAX_SPEEDMOD_VALUE;



QTPFS::NodeLayer::NodeLayer()
	: layerNumber(0)
	, numLeafNodes(0)
	, updateCounter(0)
	, xsize(0)
	, zsize(0)
	, maxRelSpeedMod(0.0f)
	, avgRelSpeedMod(0.0f)
{
}

void QTPFS::NodeLayer::InitStatic() {
	NUM_SPEEDMOD_BINS  = std::max(  1u, mapInfo->pfs.qtpfs_constants.numSpeedModBins);
	MIN_SPEEDMOD_VALUE = std::max(0.0f, mapInfo->pfs.qtpfs_constants.minSpeedModVal);
	MAX_SPEEDMOD_VALUE = std::min(8.0f, mapInfo->pfs.qtpfs_constants.maxSpeedModVal);
}

void QTPFS::NodeLayer::RegisterNode(INode* n) {
	for (unsigned int hmz = n->zmin(); hmz < n->zmax(); hmz++) {
		for (unsigned int hmx = n->xmin(); hmx < n->xmax(); hmx++) {
			nodeGrid[hmz * xsize + hmx] = n;
		}
	}
}

void QTPFS::NodeLayer::Init(unsigned int layerNum) {
	assert((QTPFS::NodeLayer::NUM_SPEEDMOD_BINS + 1) <= MaxSpeedBinTypeValue());

	// pre-count the root
	numLeafNodes = 1;
	layerNumber = layerNum;

	xsize = gs->mapx;
	zsize = gs->mapy;

	nodeGrid.resize(xsize * zsize, NULL);

	curSpeedMods.resize(xsize * zsize,  0);
	oldSpeedMods.resize(xsize * zsize,  0);
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
void QTPFS::NodeLayer::QueueUpdate(const SRectangle& r, const MoveDef* md) {
	layerUpdates.push_back(LayerUpdate());
	LayerUpdate* layerUpdate = &(layerUpdates.back());

	// the first update MUST have a non-zero counter
	// since all nodes are at 0 after initialization
	layerUpdate->rectangle = r;
	layerUpdate->speedMods.resize(r.GetArea());
	layerUpdate->blockBits.resize(r.GetArea());
	layerUpdate->counter = ++updateCounter;

	#ifdef QTPFS_IGNORE_MAP_EDGES
	const unsigned int xsizehMD = md->xsizeh;
	const unsigned int zsizehMD = md->zsizeh;
	#endif

	// make a snapshot of the terrain-state within <r>
	for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
		for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			#ifdef QTPFS_IGNORE_MAP_EDGES
			const unsigned int chmx = Clamp(hmx, xsizehMD, r.x2 - xsizehMD - 1);
			const unsigned int chmz = Clamp(hmz, zsizehMD, r.z2 - zsizehMD - 1);
			#else
			const unsigned int chmx = hmx;
			const unsigned int chmz = hmz;
			#endif

			layerUpdate->speedMods[recIdx] = NodeLayer::MAX_SPEEDMOD_VALUE;
			layerUpdate->blockBits[recIdx] = 0;

			md->TestMoveSquare(NULL, chmx, chmz, ZeroVector, true, true, false, &layerUpdate->speedMods[recIdx], &layerUpdate->blockBits[recIdx]);
		}
	}
}

bool QTPFS::NodeLayer::ExecQueuedUpdate() {
	const LayerUpdate& layerUpdate = layerUpdates.front();
	const SRectangle& rectangle = layerUpdate.rectangle;

	const std::vector<float>* speedMods = &layerUpdate.speedMods;
	const std::vector<  int>* blockBits = &layerUpdate.blockBits;

	return (Update(rectangle, NULL, speedMods, blockBits));
}
#endif



bool QTPFS::NodeLayer::Update(
	const SRectangle& r,
	const MoveDef* md,
	const std::vector<float>* luSpeedMods,
	const std::vector<  int>* luBlockBits
) {
	assert((luSpeedMods == NULL && luBlockBits == NULL) || (luSpeedMods != NULL && luBlockBits != NULL));

	unsigned int numNewBinSquares = 0;
	unsigned int numClosedSquares = 0;

	#ifdef QTPFS_IGNORE_MAP_EDGES
	const unsigned int xsizehMD = (md == NULL)? 0: md->xsizeh;
	const unsigned int zsizehMD = (md == NULL)? 0: md->zsizeh;
	#endif

	const bool globalUpdate =
		((r.x1 == 0 && r.x2 == gs->mapx) &&
		 (r.z1 == 0 && r.z2 == gs->mapy));

	if (globalUpdate) {
		maxRelSpeedMod = 0.0f;
		avgRelSpeedMod = 0.0f;
	}

	// divide speed-modifiers into bins
	for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
		for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
			const unsigned int sqrIdx = hmz * xsize + hmx;
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			#ifdef QTPFS_IGNORE_MAP_EDGES
			const unsigned int chmx = Clamp(hmx, xsizehMD, r.x2 - xsizehMD - 1);
			const unsigned int chmz = Clamp(hmz, zsizehMD, r.z2 - zsizehMD - 1);
			#else
			const unsigned int chmx = hmx;
			const unsigned int chmz = hmz;
			#endif

			float minSpeedMod = NodeLayer::MAX_SPEEDMOD_VALUE;
			int   maxBlockBit = 0;

			if (luBlockBits == NULL || luSpeedMods == NULL) {
				// FIXME: this needs to be faster (fewer duplicate checks per square)
				md->TestMoveSquare(NULL, chmx, chmz, ZeroVector, true, true, false, &minSpeedMod, &maxBlockBit);
			} else {
				minSpeedMod = (*luSpeedMods)[recIdx];
				maxBlockBit = (*luBlockBits)[recIdx];
			}

			#define NL QTPFS::NodeLayer
			const float tmpAbsSpeedMod = Clamp(minSpeedMod, NL::MIN_SPEEDMOD_VALUE, NL::MAX_SPEEDMOD_VALUE);
			const float newAbsSpeedMod = tmpAbsSpeedMod * ((maxBlockBit & CMoveMath::BLOCK_STRUCTURE) == 0);
			const float newRelSpeedMod = Clamp((newAbsSpeedMod - NL::MIN_SPEEDMOD_VALUE) / (NL::MAX_SPEEDMOD_VALUE - NL::MIN_SPEEDMOD_VALUE), 0.0f, 1.0f);
			const float curRelSpeedMod = Clamp(curSpeedMods[sqrIdx] / float(MaxSpeedModTypeValue()), 0.0f, 1.0f);
			#undef NL

			const SpeedBinType newSpeedModBin = GetSpeedModBin(newAbsSpeedMod, newRelSpeedMod);
			const SpeedBinType curSpeedModBin = curSpeedBins[sqrIdx];

			numNewBinSquares += int(newSpeedModBin != curSpeedModBin);
			numClosedSquares += int(newSpeedModBin == QTPFS::NodeLayer::NUM_SPEEDMOD_BINS);

			// need to keep track of these for Tesselate
			oldSpeedMods[sqrIdx] = curRelSpeedMod * float(MaxSpeedModTypeValue());
			curSpeedMods[sqrIdx] = newRelSpeedMod * float(MaxSpeedModTypeValue());

			oldSpeedBins[sqrIdx] = curSpeedModBin;
			curSpeedBins[sqrIdx] = newSpeedModBin;

			if (globalUpdate && newRelSpeedMod > 0.0f) {
				// only count open squares toward the maximum and average
				maxRelSpeedMod  = std::max(maxRelSpeedMod, newRelSpeedMod);
				avgRelSpeedMod += newRelSpeedMod;
			}
		}
	}

	if (globalUpdate && maxRelSpeedMod > 0.0f) {
		// if at least one open square, set the new average
		avgRelSpeedMod /= ((xsize * zsize) - numClosedSquares);
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



QTPFS::NodeLayer::SpeedBinType QTPFS::NodeLayer::GetSpeedModBin(float absSpeedMod, float relSpeedMod) const {
	// NOTE:
	//     bins N and N+1 are reserved for modifiers <= min and >= max
	//     respectively; blocked squares MUST be in their own category
	const SpeedBinType defBin = NUM_SPEEDMOD_BINS * relSpeedMod;
	const SpeedBinType maxBin = NUM_SPEEDMOD_BINS - 1;

	SpeedBinType speedModBin = Clamp(defBin, static_cast<SpeedBinType>(0), maxBin);

	if (absSpeedMod <= MIN_SPEEDMOD_VALUE) { speedModBin = NUM_SPEEDMOD_BINS + 0; }
	if (absSpeedMod >= MAX_SPEEDMOD_VALUE) { speedModBin = NUM_SPEEDMOD_BINS + 1; }

	return speedModBin;
}



// update the neighbor-cache for (a chunk of) the leaf
// nodes in this layer; this amortizes (in theory) the
// cost of doing it "on-demand" in PathSearch::Iterate
// when QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
//
// NOTE:
//   exclusive to the QTPFS_STAGGERED_LAYER_UPDATES path,
//   and makes no sense to use with the non-conservative
//   update scheme
//
#ifdef QTPFS_AMORTIZED_NODE_NEIGHBOR_CACHE_UPDATES
#ifdef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
void QTPFS::NodeLayer::ExecNodeNeighborCacheUpdate(unsigned int currFrameNum, unsigned int currMagicNum) {
	assert(!nodeGrid.empty());

	const int xoff = (currFrameNum % ((gs->mapx >> 1) / SQUARE_SIZE)) * SQUARE_SIZE;
	const int zoff = (currFrameNum / ((gs->mapy >> 1) / SQUARE_SIZE)) * SQUARE_SIZE;

	INode* n = NULL;

	{
		// top-left quadrant: [0, gs->mapx >> 1) x [0, gs->mapy >> 1)
		//
		// update an 8x8 block of squares per quadrant per frame
		// in row-major order; every GetNeighbors() call invokes
		// UpdateNeighborCache if the magic numbers do not match
		// (nodes can be visited multiple times per block update)
		const int xmin =         (xoff +           0               ), zmin =         (zoff +           0               );
		const int xmax = std::min(xmin + SQUARE_SIZE, gs->mapx >> 1), zmax = std::min(zmin + SQUARE_SIZE, gs->mapy >> 1);

		for (int z = zmin; z < zmax; z++) {
			for (int x = xmin; x < xmax; ) {
				n = nodeGrid[z * xsize + x];
				x = n->xmax();

				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
			}
		}
	}
	{
		// top-right quadrant: [gs->mapx >> 1, gs->mapx) x [0, gs->mapy >> 1)
		const int xmin =         (xoff +              (gs->mapx >> 1)), zmin =         (zoff +           0               );
		const int xmax = std::min(xmin + SQUARE_SIZE,  gs->mapx      ), zmax = std::min(zmin + SQUARE_SIZE, gs->mapy >> 1);

		for (int z = zmin; z < zmax; z++) {
			for (int x = xmin; x < xmax; ) {
				n = nodeGrid[z * xsize + x];
				x = n->xmax();

				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
			}
		}
	}
	{
		// bottom-right quadrant: [gs->mapx >> 1, gs->mapx) x [gs->mapy >> 1, gs->mapy)
		const int xmin =         (xoff +              (gs->mapx >> 1)), zmin =         (zoff +              (gs->mapy >> 1));
		const int xmax = std::min(xmin + SQUARE_SIZE,  gs->mapx      ), zmax = std::min(zmin + SQUARE_SIZE,  gs->mapy      );

		for (int z = zmin; z < zmax; z++) {
			for (int x = xmin; x < xmax; ) {
				n = nodeGrid[z * xsize + x];
				x = n->xmax();

				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
			}
		}
	}
	{
		// bottom-left quadrant: [0, gs->mapx >> 1) x [gs->mapy >> 1, gs->mapy)
		const int xmin =         (xoff +           0               ), zmin =         (zoff +              (gs->mapy >> 1));
		const int xmax = std::min(xmin + SQUARE_SIZE, gs->mapx >> 1), zmax = std::min(zmin + SQUARE_SIZE,  gs->mapy      );

		for (int z = zmin; z < zmax; z++) {
			for (int x = xmin; x < xmax; ) {
				n = nodeGrid[z * xsize + x];
				x = n->xmax();

				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
			}
		}
	}
}
#endif
#endif

void QTPFS::NodeLayer::ExecNodeNeighborCacheUpdates(const SRectangle& ur, unsigned int currMagicNum) {
	assert(!nodeGrid.empty());

	// account for the rim of nodes around the bounding box
	// (whose neighbors also changed during re-tesselation)
	const int xmin = std::max(ur.x1 - 1, 0), xmax = std::min(ur.x2 + 1, gs->mapx);
	const int zmin = std::max(ur.z1 - 1, 0), zmax = std::min(ur.z2 + 1, gs->mapy);

	INode* n = NULL;

	for (int z = zmin; z < zmax; z++) {
		for (int x = xmin; x < xmax; ) {
			n = nodeGrid[z * xsize + x];
			x = n->xmax();

			// NOTE:
			//   during initialization, currMagicNum == 0 which nodes start with already 
			//   (does not matter because prevMagicNum == -1, so updates are not no-ops)
			n->SetMagicNumber(currMagicNum);
			n->UpdateNeighborCache(nodeGrid);
		}
	}
}


