/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _NANO_PIECE_CACHE_H
#define _NANO_PIECE_CACHE_H

#include <vector>

#include "Sim/Misc/GlobalConstants.h"
#include "System/bitops.h"

class CUnitScript;
struct NanoPieceCache {
public:
	NanoPieceCache(): lastNanoPieceCnt(0), curBuildPowerMask(0) {
	}

	void Update() { curBuildPowerMask >>= 1; }

	float GetBuildPower() const { return (count_bits_set(curBuildPowerMask) / float(UNIT_SLOWUPDATE_RATE)); }
	int GetNanoPiece(CUnitScript* ownerScript);

	const std::vector<int>& GetNanoPieces() const { return nanoPieces; }
	      std::vector<int>& GetNanoPieces()       { return nanoPieces; }

private:
	std::vector<int> nanoPieces;

	int lastNanoPieceCnt;
	int curBuildPowerMask;
};

#endif

