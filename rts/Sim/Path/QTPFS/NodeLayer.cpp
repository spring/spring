/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NodeLayer.hpp"
#include "PathManager.hpp"
#include "Node.hpp"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/myMath.h"

#define NL QTPFS::NodeLayer

static unsigned char GetSpeedModBin(float absSpeedMod, float relSpeedMod) {
	// NOTE:
	//     bins N and N+1 are reserved for modifiers <= min and >= max
	//     respectively; blocked squares MUST be in their own category
	const unsigned char defBin = NL::NUM_SPEEDMOD_BINS * relSpeedMod;
	const unsigned char maxBin = NL::NUM_SPEEDMOD_BINS - 1;

	unsigned char speedModBin = Clamp(defBin, static_cast<unsigned char>(0), maxBin);

	if (absSpeedMod <= NL::MIN_SPEEDMOD_VALUE) { speedModBin = NL::NUM_SPEEDMOD_BINS + 0; }
	if (absSpeedMod >= NL::MAX_SPEEDMOD_VALUE) { speedModBin = NL::NUM_SPEEDMOD_BINS + 1; }

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
void QTPFS::NodeLayer::QueueUpdate(const PathRectangle& r, const MoveDef* md) {
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
	for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
		for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			#ifdef QTPFS_IGNORE_MAP_EDGES
			const unsigned int chmx = Clamp(hmx, xsizehMD, r.x2 - xsizehMD - 1);
			const unsigned int chmz = Clamp(hmz, zsizehMD, r.z2 - zsizehMD - 1);
			#else
			const unsigned int chmx = hmx;
			const unsigned int chmz = hmz;
			#endif

			layerUpdate->speedMods[recIdx] = CMoveMath::GetPosSpeedMod(*md, chmx, chmz);
			layerUpdate->blockBits[recIdx] = CMoveMath::IsBlockedNoSpeedModCheck(*md, chmx, chmz, NULL);
		}
	}
}

bool QTPFS::NodeLayer::ExecQueuedUpdate() {
	const LayerUpdate& layerUpdate = layerUpdates.front();
	const PathRectangle& rectangle = layerUpdate.rectangle;
	const std::vector<float>* speedMods = &layerUpdate.speedMods;
	const std::vector<  int>* blockBits = &layerUpdate.blockBits;
	const bool ret = Update(rectangle, NULL, speedMods, blockBits);

	return ret;
}
#endif



bool QTPFS::NodeLayer::Update(
	const PathRectangle& r,
	const MoveDef* md,
	const std::vector<float>* luSpeedMods,
	const std::vector<  int>* luBlockBits
) {
	unsigned int numNewBinSquares = 0;

	#ifdef QTPFS_IGNORE_MAP_EDGES
	const unsigned int xsizehMD = (md == NULL)? 0: md->xsizeh;
	const unsigned int zsizehMD = (md == NULL)? 0: md->zsizeh;
	#endif

	// divide speed-modifiers into bins
	for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
		for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
			const unsigned int sqrIdx = hmz * xsize + hmx;
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			#ifdef QTPFS_IGNORE_MAP_EDGES
			const unsigned int chmx = Clamp(hmx, xsizehMD, r.x2 - xsizehMD - 1);
			const unsigned int chmz = Clamp(hmz, zsizehMD, r.z2 - zsizehMD - 1);
			#else
			const unsigned int chmx = hmx;
			const unsigned int chmz = hmz;
			#endif

			// NOTE:
			//     GetPosSpeedMod only checks the terrain (height/slope/type), not the blocking-map
			//     IsBlockedNoSpeedModCheck scans entire footprint, GetPosSpeedMod just one square
			const unsigned int blockBits = (luBlockBits == NULL)? CMoveMath::IsBlockedNoSpeedModCheck(*md, chmx, chmz, NULL): (*luBlockBits)[recIdx];

			const float rawAbsSpeedMod = (luSpeedMods == NULL)? CMoveMath::GetPosSpeedMod(*md, chmx, chmz): (*luSpeedMods)[recIdx];
			const float tmpAbsSpeedMod = Clamp(rawAbsSpeedMod, NL::MIN_SPEEDMOD_VALUE, NL::MAX_SPEEDMOD_VALUE);
			const float newAbsSpeedMod = ((blockBits & CMoveMath::BLOCK_STRUCTURE) == 0)? tmpAbsSpeedMod: 0.0f;
			const float newRelSpeedMod = (newAbsSpeedMod - NL::MIN_SPEEDMOD_VALUE) / (NL::MAX_SPEEDMOD_VALUE - NL::MIN_SPEEDMOD_VALUE);
			const float curRelSpeedMod = curSpeedMods[sqrIdx] / QTPFS_FLOAT_MAX_USSHORT;

			const unsigned char newSpeedModBin = GetSpeedModBin(newAbsSpeedMod, newRelSpeedMod);
			const unsigned char curSpeedModBin = curSpeedBins[sqrIdx];

			numNewBinSquares += int(newSpeedModBin != curSpeedModBin);

			// need to keep track of these for Tesselate
			oldSpeedMods[sqrIdx] = curRelSpeedMod * QTPFS_FLOAT_MAX_USSHORT;
			curSpeedMods[sqrIdx] = newRelSpeedMod * QTPFS_FLOAT_MAX_USSHORT;

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

		for (int x = xmin; x < xmax; x++) {
			for (int z = zmin; z < zmax; ) {
				n = nodeGrid[z * xsize + x];
				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
				z = n->zmax();
				// z += n->zsize();
			}
		}
	}
	{
		// top-right quadrant: [gs->mapx >> 1, gs->mapx) x [0, gs->mapy >> 1)
		const int xmin =         (xoff +              (gs->mapx >> 1)), zmin =         (zoff +           0               );
		const int xmax = std::min(xmin + SQUARE_SIZE,  gs->mapx      ), zmax = std::min(zmin + SQUARE_SIZE, gs->mapy >> 1);

		for (int x = xmin; x < xmax; x++) {
			for (int z = zmin; z < zmax; ) {
				n = nodeGrid[z * xsize + x];
				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
				z = n->zmax();
				// z += n->zsize();
			}
		}
	}
	{
		// bottom-right quadrant: [gs->mapx >> 1, gs->mapx) x [gs->mapy >> 1, gs->mapy)
		const int xmin =         (xoff +              (gs->mapx >> 1)), zmin =         (zoff +              (gs->mapy >> 1));
		const int xmax = std::min(xmin + SQUARE_SIZE,  gs->mapx      ), zmax = std::min(zmin + SQUARE_SIZE,  gs->mapy      );

		for (int x = xmin; x < xmax; x++) {
			for (int z = zmin; z < zmax; ) {
				n = nodeGrid[z * xsize + x];
				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
				z = n->zmax();
				// z += n->zsize();
			}
		}
	}
	{
		// bottom-left quadrant: [0, gs->mapx >> 1) x [gs->mapy >> 1, gs->mapy)
		const int xmin =         (xoff +           0               ), zmin =         (zoff +              (gs->mapy >> 1));
		const int xmax = std::min(xmin + SQUARE_SIZE, gs->mapx >> 1), zmax = std::min(zmin + SQUARE_SIZE,  gs->mapy      );

		for (int x = xmin; x < xmax; x++) {
			for (int z = zmin; z < zmax; ) {
				n = nodeGrid[z * xsize + x];
				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
				z = n->zmax();
				// z += n->zsize();
			}
		}
	}
}
#endif
#endif

void QTPFS::NodeLayer::ExecNodeNeighborCacheUpdates(const PathRectangle& ur, unsigned int currMagicNum) {
	// account for the rim of nodes around the bounding box
	// (whose neighbors also changed during re-tesselation)
	const int xmin = std::max(ur.x1 - 1, 0), xmax = std::min(ur.x2 + 1, gs->mapx);
	const int zmin = std::max(ur.z1 - 1, 0), zmax = std::min(ur.z2 + 1, gs->mapy);

	INode* n = NULL;

	for (int x = xmin; x < xmax; x++) {
		for (int z = zmin; z < zmax; ) {
			n = nodeGrid[z * xsize + x];

			// NOTE:
			//   during initialization, currMagicNum == 0 which nodes start with already 
			//   (does not matter because prevMagicNum == -1, so updates are not no-ops)
			n->SetMagicNumber(currMagicNum);
			n->UpdateNeighborCache(nodeGrid);

			z = n->zmax();
			// z += n->zsize();
		}
	}
}

