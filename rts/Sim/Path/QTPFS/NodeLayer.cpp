/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <limits>

#include "NodeLayer.hpp"
#include "PathManager.hpp"
#include "Node.hpp"

#include "Map/MapInfo.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/SpringMath.h"

unsigned int QTPFS::NodeLayer::NUM_SPEEDMOD_BINS;
float        QTPFS::NodeLayer::MIN_SPEEDMOD_VALUE;
float        QTPFS::NodeLayer::MAX_SPEEDMOD_VALUE;



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

	xsize = mapDims.mapx;
	zsize = mapDims.mapy;

	nodeGrid.resize(xsize * zsize, nullptr);

	{
		// chunks are reserved OTF
		nodeIndcs.clear();
		nodeIndcs.resize(POOL_TOTAL_SIZE);

		std::for_each(nodeIndcs.begin(), nodeIndcs.end(), [&](const unsigned int& i) { nodeIndcs[&i - &nodeIndcs[0]] = &i - &nodeIndcs[0]; });
		std::reverse(nodeIndcs.begin(), nodeIndcs.end());
	}

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
	layerUpdates.emplace_back();
	LayerUpdate& layerUpdate = layerUpdates.back();

	// the first update MUST have a non-zero counter
	// since all nodes are at 0 after initialization
	layerUpdate.rectangle = r;
	layerUpdate.speedMods.resize(r.GetArea());
	layerUpdate.blockBits.resize(r.GetArea());
	layerUpdate.counter = ++updateCounter;

	// make a snapshot of the terrain-state within <r>
	for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
		for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			const unsigned int chmx = Clamp(int(hmx), md->xsizeh, r.x2 - md->xsizeh - 1);
			const unsigned int chmz = Clamp(int(hmz), md->zsizeh, r.z2 - md->zsizeh - 1);

			layerUpdate.speedMods[recIdx] = CMoveMath::GetPosSpeedMod(*md, hmx, hmz);
			layerUpdate.blockBits[recIdx] = CMoveMath::IsBlockedNoSpeedModCheck(*md, chmx, chmz, nullptr);
			// layerUpdate.blockBits[recIdx] = CMoveMath::SquareIsBlocked(*md, hmx, hmz, nullptr);
		}
	}
}

bool QTPFS::NodeLayer::ExecQueuedUpdate() {
	const LayerUpdate& layerUpdate = layerUpdates.front();
	const SRectangle& rectangle = layerUpdate.rectangle;

	const std::vector<float>* speedMods = &layerUpdate.speedMods;
	const std::vector<  int>* blockBits = &layerUpdate.blockBits;

	return (Update(rectangle, moveDefHandler.GetMoveDefByPathType(layerNumber), speedMods, blockBits));
}
#endif



bool QTPFS::NodeLayer::Update(
	const SRectangle& r,
	const MoveDef* md,
	const std::vector<float>* luSpeedMods,
	const std::vector<  int>* luBlockBits
) {
	assert((luSpeedMods == nullptr && luBlockBits == nullptr) || (luSpeedMods != nullptr && luBlockBits != nullptr));

	unsigned int numNewBinSquares = 0;
	unsigned int numClosedSquares = 0;

	const bool globalUpdate =
		((r.x1 == 0 && r.x2 == mapDims.mapx) &&
		 (r.z1 == 0 && r.z2 == mapDims.mapy));

	if (globalUpdate) {
		maxRelSpeedMod = 0.0f;
		avgRelSpeedMod = 0.0f;
	}

	// divide speed-modifiers into bins
	for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
		for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
			const unsigned int sqrIdx = hmz * xsize + hmx;
			const unsigned int recIdx = (hmz - r.z1) * r.GetWidth() + (hmx - r.x1);

			// don't tesselate map edges when footprint extends across them in IsBlocked*
			const unsigned int chmx = Clamp(int(hmx), md->xsizeh, r.x2 - md->xsizeh - 1);
			const unsigned int chmz = Clamp(int(hmz), md->zsizeh, r.z2 - md->zsizeh - 1);

			const float minSpeedMod = (luSpeedMods == nullptr)? CMoveMath::GetPosSpeedMod(*md, hmx, hmz): (*luSpeedMods)[recIdx];
			const   int maxBlockBit = (luBlockBits == nullptr)? CMoveMath::IsBlockedNoSpeedModCheck(*md, chmx, chmz, nullptr): (*luBlockBits)[recIdx];
			// NOTE:
			//   movetype code checks ONLY the *CENTER* square of a unit's footprint
			//   to get the current speedmod affecting it, and the default pathfinder
			//   only takes the entire footprint into account for STRUCTURE-blocking
			//   tests --> do the same here because full-footprint checking for both
			//   structures AND terrain is much slower (and if not handled correctly
			//   units will get stuck everywhere)
			// NOTE:
			//   IsBlockedNoSpeedModCheck works at HALF-heightmap resolution (as does
			//   the default pathfinder for DETAILED_DISTANCE searches!), so this can
			//   generate false negatives!
			//   
			// const int maxBlockBit = (luBlockBits == NULL)? CMoveMath::SquareIsBlocked(*md, hmx, hmz, NULL): (*luBlockBits)[recIdx];

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

	// if at least one open square, set the new average
	if (globalUpdate && maxRelSpeedMod > 0.0f)
		avgRelSpeedMod /= ((xsize * zsize) - numClosedSquares);

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

	const int xoff = (currFrameNum % ((mapDims.mapx >> 1) / SQUARE_SIZE)) * SQUARE_SIZE;
	const int zoff = (currFrameNum / ((mapDims.mapy >> 1) / SQUARE_SIZE)) * SQUARE_SIZE;

	INode* n = nullptr;

	{
		// top-left quadrant: [0, mapDims.mapx >> 1) x [0, mapDims.mapy >> 1)
		//
		// update an 8x8 block of squares per quadrant per frame
		// in row-major order; every GetNeighbors() call invokes
		// UpdateNeighborCache if the magic numbers do not match
		// (nodes can be visited multiple times per block update)
		const int xmin =         (xoff +           0                   ), zmin =         (zoff +           0                   );
		const int xmax = std::min(xmin + SQUARE_SIZE, mapDims.mapx >> 1), zmax = std::min(zmin + SQUARE_SIZE, mapDims.mapy >> 1);

		for (int z = zmin; z < zmax; ) {
			unsigned int zspan = zsize;

			for (int x = xmin; x < xmax; ) {
				n = nodeGrid[z * xsize + x];
				x = n->xmax();

				zspan = std::min(zspan, n->zmax() - z);
				zspan = std::max(zspan, 1u);

				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
			}

			z += zspan;
		}
	}
	{
		// top-right quadrant: [mapDims.mapx >> 1, mapDims.mapx) x [0, mapDims.mapy >> 1)
		const int xmin =         (xoff +              (mapDims.mapx >> 1)), zmin =         (zoff +           0                   );
		const int xmax = std::min(xmin + SQUARE_SIZE,  mapDims.mapx      ), zmax = std::min(zmin + SQUARE_SIZE, mapDims.mapy >> 1);

		for (int z = zmin; z < zmax; ) {
			unsigned int zspan = zsize;

			for (int x = xmin; x < xmax; ) {
				n = nodeGrid[z * xsize + x];
				x = n->xmax();

				zspan = std::min(zspan, n->zmax() - z);
				zspan = std::max(zspan, 1u);

				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
			}

			z += zspan;
		}
	}
	{
		// bottom-right quadrant: [mapDims.mapx >> 1, mapDims.mapx) x [mapDims.mapy >> 1, mapDims.mapy)
		const int xmin =         (xoff +              (mapDims.mapx >> 1)), zmin =         (zoff +              (mapDims.mapy >> 1));
		const int xmax = std::min(xmin + SQUARE_SIZE,  mapDims.mapx      ), zmax = std::min(zmin + SQUARE_SIZE,  mapDims.mapy      );

		for (int z = zmin; z < zmax; ) {
			unsigned int zspan = zsize;

			for (int x = xmin; x < xmax; ) {
				n = nodeGrid[z * xsize + x];
				x = n->xmax();

				zspan = std::min(zspan, n->zmax() - z);
				zspan = std::max(zspan, 1u);

				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
			}

			z += zspan;
		}
	}
	{
		// bottom-left quadrant: [0, mapDims.mapx >> 1) x [mapDims.mapy >> 1, mapDims.mapy)
		const int xmin =         (xoff +           0                   ), zmin =         (zoff +              (mapDims.mapy >> 1));
		const int xmax = std::min(xmin + SQUARE_SIZE, mapDims.mapx >> 1), zmax = std::min(zmin + SQUARE_SIZE,  mapDims.mapy      );

		for (int z = zmin; z < zmax; ) {
			unsigned int zspan = zsize;

			for (int x = xmin; x < xmax; ) {
				n = nodeGrid[z * xsize + x];
				x = n->xmax();

				zspan = std::min(zspan, n->zmax() - z);
				zspan = std::max(zspan, 1u);

				n->SetMagicNumber(currMagicNum);
				n->GetNeighbors(nodeGrid);
			}

			z += zspan;
		}
	}
}
#endif
#endif

void QTPFS::NodeLayer::ExecNodeNeighborCacheUpdates(const SRectangle& ur, unsigned int currMagicNum) {
	assert(!nodeGrid.empty());

	// account for the rim of nodes around the bounding box
	// (whose neighbors also changed during re-tesselation)
	const int xmin = std::max(ur.x1 - 1, 0), xmax = std::min(ur.x2 + 1, mapDims.mapx);
	const int zmin = std::max(ur.z1 - 1, 0), zmax = std::min(ur.z2 + 1, mapDims.mapy);

	INode* n = nullptr;

	for (int z = zmin; z < zmax; ) {
		unsigned int zspan = zsize;

		for (int x = xmin; x < xmax; ) {
			n = nodeGrid[z * xsize + x];
			x = n->xmax();

			// calculate largest safe z-increment along this row
			zspan = std::min(zspan, n->zmax() - z);
			zspan = std::max(zspan, 1u);

			// NOTE:
			//   during initialization, currMagicNum == 0 which nodes start with already 
			//   (does not matter because prevMagicNum == -1, so updates are not no-ops)
			n->SetMagicNumber(currMagicNum);
			n->UpdateNeighborCache(nodeGrid);
		}

		z += zspan;
	}
}

