#include "StdAfx.h"
#include "MoveMath.h"
#include "Map/ReadMap.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "LogOutput.h"
#include "mmgr.h"

CR_BIND_INTERFACE(CMoveMath);

CMoveMath::~CMoveMath() {
}

/* Converts a point-request into a square-positional request. */
float CMoveMath::SpeedMod(const MoveData& moveData, float3 pos) {
	int x = int(pos.x / SQUARE_SIZE);
	int z = int(pos.z / SQUARE_SIZE);
	return SpeedMod(moveData, x, z);
}


/* calculate the local speed-modifier for this movedata */
float CMoveMath::SpeedMod(const MoveData& moveData, int xSquare, int zSquare) {
	// Error-check
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 0.0f;
	}

	// Extract data.
	int square = xSquare / 2 + zSquare / 2 * gs->hmapx;
	float height = readmap->mipHeightmap[1][square];
	float slope = readmap->slopemap[square];
	float typemod = moveinfo->terrainType2MoveFamilySpeed[readmap->typemap[square]][moveData.moveFamily];

	return (SpeedMod(moveData, height, slope) * typemod);
}

float CMoveMath::SpeedMod(const MoveData& moveData, float3 pos, const float3& moveDir) {
	int x = int(pos.x / SQUARE_SIZE);
	int z = int(pos.z / SQUARE_SIZE);
	return SpeedMod(moveData, x, z,moveDir);
}


float CMoveMath::SpeedMod(const MoveData& moveData, int xSquare, int zSquare, const float3& moveDir) {
	// Error-check
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 0.0f;
	}

	// Extract data.
	int square = xSquare / 2 + zSquare / 2 * gs->hmapx;
	float height = readmap->mipHeightmap[1][square];
	float slope = readmap->slopemap[square];

	float3 flatNorm = readmap->facenormals[(xSquare + zSquare * gs->mapx) * 2];
	flatNorm.y = 0;
	flatNorm.Normalize();
	float moveSlope = -moveDir.dot(flatNorm);
	float typemod = moveinfo->terrainType2MoveFamilySpeed[readmap->typemap[square]][moveData.moveFamily];

	return (SpeedMod(moveData, height, slope, moveSlope) * typemod);
}


/* Converts a point-request into a square-positional request. */
int CMoveMath::IsBlocked(const MoveData& moveData, float3 pos, bool fromEst) {
	int x = int(pos.x / SQUARE_SIZE);
	int z = int(pos.z / SQUARE_SIZE);
	return IsBlocked(moveData, x, z, fromEst);
}

/* Check if a given square-position is accessable by the movedata footprint. */
int CMoveMath::IsBlocked(const MoveData& moveData, int xSquare, int zSquare, bool fromEst) {
	if (CMoveMath::SpeedMod(moveData, xSquare, zSquare) == 0.0f) {
		return 1;
	}

	int ret = 0;

	ret |= SquareIsBlocked(moveData, xSquare                        , zSquare                        , fromEst);
	ret |= SquareIsBlocked(moveData, xSquare - moveData.size / 2    , zSquare - moveData.size / 2    , fromEst);
	ret |= SquareIsBlocked(moveData, xSquare + moveData.size / 2 - 1, zSquare - moveData.size / 2    , fromEst);
	ret |= SquareIsBlocked(moveData, xSquare - moveData.size / 2    , zSquare + moveData.size / 2 - 1, fromEst);
	ret |= SquareIsBlocked(moveData, xSquare + moveData.size / 2 - 1, zSquare + moveData.size / 2 - 1, fromEst);

	return ret;
}

/*
 * Check if a given square-position is accessable given the movedata footprint.
 * Doesn't check terrain, but takes size into account so it does not run over
 * something small if footprint is big.
 */
int CMoveMath::IsBlocked2(const MoveData& moveData, int xSquare, int zSquare, bool fromEst) {
	int ret = 0;

	switch (moveData.size) {
		case 12:
		case 11:
			ret |= SquareIsBlocked(moveData, xSquare + 4, zSquare + 4, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 6, zSquare + 4, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 6, zSquare - 6, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare + 4, zSquare - 6, fromEst);
		case 8:
		case 7:
			ret |= SquareIsBlocked(moveData, xSquare + 2, zSquare + 2, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 4, zSquare + 2, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 4, zSquare - 4, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare + 2, zSquare - 4, fromEst);
		case 4:
		case 3:
			ret |= SquareIsBlocked(moveData, xSquare    , zSquare    , fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 2, zSquare    , fromEst);
			ret |= SquareIsBlocked(moveData, xSquare    , zSquare - 2, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 2, zSquare - 2, fromEst);
			break;

		case 14:
		case 13:
			ret |= SquareIsBlocked(moveData, xSquare + 6, zSquare + 6, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 6, zSquare + 6, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 6, zSquare - 6, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare + 6, zSquare - 6, fromEst);
		case 10:
		case 9:
			ret |= SquareIsBlocked(moveData, xSquare + 4, zSquare + 4, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 4, zSquare + 4, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 4, zSquare - 4, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare + 4, zSquare - 4, fromEst);
		case 6:
		case 5:
			ret |= SquareIsBlocked(moveData, xSquare + 2, zSquare + 2, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 2, zSquare + 2, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare - 2, zSquare - 2, fromEst);
			ret |= SquareIsBlocked(moveData, xSquare + 2, zSquare - 2, fromEst);
		case 2:
		case 1:
			ret |= SquareIsBlocked(moveData, xSquare, zSquare, fromEst);
			break;

		default:
			logOutput.Print("Unknown footprint size in IsBlocked2() (%i)", moveData.size);
			break;
	};

	return ret;
}

/*
 * check if an object is blocking or not for a given MoveData (feature
 * objects block iif their mass exceeds the movedata's crush-strength).
 * NOTE: modify for selective blocking
 */
bool CMoveMath::IsBlocking(const MoveData& moveData, const CSolidObject* object) {
	return
		(object->blocking && (!dynamic_cast<const CFeature*>(object) ||
		object->mass > moveData.crushStrength));
}

/*
 * check if an object is NON-blocking for a given MoveData
 * (ex. a submarine's moveDef vs. a surface ship object)
 */
bool CMoveMath::IsNonBlocking(const MoveData& moveData, const CSolidObject* obstacle) {
	if (moveData.terrainClass == MoveData::Land) {
		return (obstacle->isUnderWater);
	} else {
		const int x = int(obstacle->pos.x / SQUARE_SIZE);
		const int z = int(obstacle->pos.z / SQUARE_SIZE);
		const int i = (x >> 1) + (z >> 1) * gs->hmapx;
		const bool iss = moveData.subMarine;
		const bool oss = (obstacle->mobility && obstacle->mobility->subMarine);

		const float oy = obstacle->pos.y;
		const float oh = obstacle->height;
		const float gy = readmap->mipHeightmap[1][i];

		// note that these conditions can lead to
		// a certain degree of clipping, for full
		// 3D accuracy the height of the movedata
		// owner would need to be accessible (but
		// the path-estimator defs aren't tied to
		// any)
		if (moveData.followGround || gy > 0.0f) {
			return ((oy - oh) > gy);
		} else {
			if (iss) {
				return ((oy + oh) > 0.0f);
			} else {
				return (((oy + oh) <= 0.0f) || oss);
			}
		}
	}

	return false;
}


/* Converts a point-request into a square-positional request. */
float CMoveMath::yLevel(const float3 pos) {
	int x = int(pos.x / SQUARE_SIZE);
	int z = int(pos.z / SQUARE_SIZE);
	return yLevel(x, z);
}

/* Check if a single square is accessable (for any object which uses the given movedata). */
int CMoveMath::SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare, bool fromEst) {
	// Error-check
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 1;
	}

	CSolidObject* obstacle = groundBlockingObjectMap->GroundBlockedUnsafe(xSquare + zSquare * gs->mapx);

	if (obstacle) {
		if (IsNonBlocking(moveData, obstacle)) {
			return 0;
		}

		if (obstacle->mobility) {
			// mobile obstacle
			if (obstacle->isMoving) {
				return BLOCK_MOVING;
			} else {
				if (!((CUnit*) obstacle)->beingBuilt && ((CUnit*) obstacle)->commandAI->commandQue.empty()) {
					// idling mobile unit
					return BLOCK_MOBILE;
				} else {
					// busy mobile unit (but not following path)
					return BLOCK_MOBILE_BUSY;
				}
			}
		} else {
			if (IsBlocking(moveData, obstacle)) {
				return BLOCK_STRUCTURE;
			}
		}
	}

	return 0;
}
