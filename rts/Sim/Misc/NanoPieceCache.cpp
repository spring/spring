/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NanoPieceCache.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Scripts/UnitScript.h"

CR_BIND(NanoPieceCache, )

CR_REG_METADATA(NanoPieceCache, (
	CR_MEMBER(nanoPieces),
	CR_MEMBER(lastNanoPieceCnt),
	CR_MEMBER(curBuildPowerMask)
))

int NanoPieceCache::GetNanoPiece(CUnitScript* ownerScript) {
	curBuildPowerMask |= (1 << (UNIT_SLOWUPDATE_RATE - 1));

	int nanoPiece = -1;

	if (!nanoPieces.empty()) {
		const unsigned cnt = nanoPieces.size();
		const unsigned rnd = gsRNG.NextInt(cnt);
		nanoPiece = nanoPieces[rnd];
	}

	if (lastNanoPieceCnt <= MAX_QUERYNANOPIECE_CALLS) {
		// only do so 30 times and then use the cache
		const int scriptPiece = ownerScript->QueryNanoPiece();
		const int modelPiece  = ownerScript->ScriptToModel(scriptPiece);

		if (ownerScript->PieceExists(scriptPiece)) {
			nanoPiece = modelPiece;

			if (std::find(nanoPieces.begin(), nanoPieces.end(), nanoPiece) != nanoPieces.end()) {
				// already in cache
				lastNanoPieceCnt++;
			} else {
				nanoPieces.push_back(nanoPiece);
				lastNanoPieceCnt = 0;
			}
		} else {
			lastNanoPieceCnt++;
		}
	}

	return nanoPiece;
}

