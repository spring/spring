/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NodeLayer.hpp"
#include "PathManager.hpp"
#include "Node.hpp"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/myMath.h"
#include "System/Rectangle.h"

#define PM QTPFS::PathManager

static int GetSpeedModBin(float absSpeedMod, float relSpeedMod) {
	// NOTE:
	//     bins N and N+1 are reserved for modifiers <= min and >= max
	//     respectively; blocked squares MUST be in their own category
	int speedModBin = Clamp(int(relSpeedMod * PM::NUM_SPEEDMOD_BINS), 0, int(PM::NUM_SPEEDMOD_BINS - 1));

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

void QTPFS::NodeLayer::Init() {
	// pre-count the root
	numLeafNodes = 1;

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
}

bool QTPFS::NodeLayer::Update(const SRectangle& r, const MoveData* md, const CMoveMath* mm) {
	// NOTE:
	//   we need a fixed range that does not become wider / narrower
	//   during terrain deformations (otherwise the bins would change
	//   across ALL nodes)
	static const float speedModRng = PM::MAX_SPEEDMOD_VALUE - PM::MIN_SPEEDMOD_VALUE;
	static const float minSpeedMod = PM::MIN_SPEEDMOD_VALUE;

	unsigned int numNewBinSquares = 0;

	// divide speed-modifiers into bins
	for (unsigned int hmx = r.x1; hmx < r.x2; hmx++) {
		for (unsigned int hmz = r.z1; hmz < r.z2; hmz++) {
			const unsigned int sqrIdx = hmz * xsize + hmx;

			// NOTE: GetPosSpeedMod only checks terrain (height/slope/type), not the blocking-map
			const CMoveMath::BlockType blockBits = mm->IsBlockedNoSpeedModCheck(*md, hmx, hmz);

			const float rawAbsSpeedMod = mm->GetPosSpeedMod(*md, hmx, hmz);
			const float tmpAbsSpeedMod = Clamp(rawAbsSpeedMod, PM::MIN_SPEEDMOD_VALUE, PM::MAX_SPEEDMOD_VALUE);
			const float newAbsSpeedMod = ((blockBits & CMoveMath::BLOCK_STRUCTURE) == 0)? tmpAbsSpeedMod: 0.0f;
			const float newRelSpeedMod = (newAbsSpeedMod - minSpeedMod) / speedModRng;
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

