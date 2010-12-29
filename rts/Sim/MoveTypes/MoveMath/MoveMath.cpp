/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "MoveMath.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "LogOutput.h"
#include "mmgr.h"

CR_BIND_INTERFACE(CMoveMath);

/* Converts a point-request into a square-positional request. */
float CMoveMath::yLevel(const float3& pos) const
{
	return yLevel((pos.x / SQUARE_SIZE), (pos.z / SQUARE_SIZE));
}



/* Converts a point-request into a square-positional request. */
float CMoveMath::SpeedMod(const MoveData& moveData, const float3& pos) const
{
	return SpeedMod(moveData, (pos.x / SQUARE_SIZE), (pos.z / SQUARE_SIZE));
}

float CMoveMath::SpeedMod(const MoveData& moveData, const float3& pos, const float3& moveDir) const
{
	return SpeedMod(moveData, (pos.x / SQUARE_SIZE), (pos.z / SQUARE_SIZE), moveDir);
}


/* calculate the local speed-modifier for this movedata */
float CMoveMath::SpeedMod(const MoveData& moveData, int xSquare, int zSquare) const
{
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 0.0f;
	}

	const int square = (xSquare >> 1) + ((zSquare >> 1) * gs->hmapx);
	const int squareTerrType = readmap->typemap[square];

	const float height  = readmap->mipHeightmap[1][square];
	const float slope   = readmap->slopemap[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	switch (moveData.moveFamily) {
		case MoveData::Tank:  { return (SpeedMod(moveData, height, slope) * tt.tankSpeed ); } break;
		case MoveData::KBot:  { return (SpeedMod(moveData, height, slope) * tt.kbotSpeed ); } break;
		case MoveData::Hover: { return (SpeedMod(moveData, height, slope) * tt.hoverSpeed); } break;
		case MoveData::Ship:  { return (SpeedMod(moveData, height, slope) * tt.shipSpeed ); } break;
		default: {} break;
	}
	return 0.0f;
}

float CMoveMath::SpeedMod(const MoveData& moveData, int xSquare, int zSquare, const float3& moveDir) const
{
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 0.0f;
	}

	const int square         = (xSquare >> 1) + ((zSquare >> 1) * gs->hmapx);
	const int squareTerrType = readmap->typemap[square];

	const float height  = readmap->mipHeightmap[1][square];
	const float slope   = readmap->slopemap[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	float3 flatNorm = readmap->centernormals[xSquare + zSquare * gs->mapx];
		flatNorm.y = 0.0f;
		flatNorm.SafeNormalize();

	const float moveSlope = -moveDir.dot(flatNorm);

	switch (moveData.moveFamily) {
		case MoveData::Tank:  { return (SpeedMod(moveData, height, slope, moveSlope) * tt.tankSpeed ); } break;
		case MoveData::KBot:  { return (SpeedMod(moveData, height, slope, moveSlope) * tt.kbotSpeed ); } break;
		case MoveData::Hover: { return (SpeedMod(moveData, height, slope, moveSlope) * tt.hoverSpeed); } break;
		case MoveData::Ship:  { return (SpeedMod(moveData, height, slope, moveSlope) * tt.shipSpeed ); } break;
		default: {} break;
	}

	return 0.0f;
}



/* Converts a point-request into a square-positional request. */
int CMoveMath::IsBlocked(const MoveData& moveData, const float3& pos) const
{
	return IsBlocked(moveData, (pos.x / SQUARE_SIZE), (pos.z / SQUARE_SIZE));
}

/* Check if a given square-position is accessable by the movedata footprint. */
int CMoveMath::IsBlocked(const MoveData& moveData, int xSquare, int zSquare) const
{
	if (CMoveMath::SpeedMod(moveData, xSquare, zSquare) == 0.0f) {
		return 1;
	}

	int ret = 0;

	const int xmin = xSquare - (moveData.xsize >> 1), xmax = xSquare + (moveData.xsize >> 1), xstep = (moveData.xsize >> 2);
	const int zmin = zSquare - (moveData.zsize >> 1), zmax = zSquare + (moveData.zsize >> 1), zstep = (moveData.zsize >> 2);

	if (moveData.xsize <= (SQUARE_SIZE >> 1) && moveData.zsize <= (SQUARE_SIZE >> 1)) {
		// only check squares under the footprint-corners and center
		// (footprints are point-symmetric around <xSquare, zSquare>)
		ret |= SquareIsBlocked(moveData, xSquare, zSquare);
		ret |= SquareIsBlocked(moveData, xmin, zmin);
		ret |= SquareIsBlocked(moveData, xmax, zmin);
		ret |= SquareIsBlocked(moveData, xmax, zmax);
		ret |= SquareIsBlocked(moveData, xmin, zmax);
	} else {
		for (int x = xmin; x <= xmax; x += xstep) {
			for (int z = zmin; z <= zmax; z += zstep) {
				ret |= SquareIsBlocked(moveData, x, z);
			}
		}
	}

	return ret;
}

/*
 * check if an object is blocking or not for a given MoveData (feature
 * objects block iif their mass exceeds the movedata's crush-strength).
 * NOTE: modify for selective blocking
 */
bool CMoveMath::CrushResistant(const MoveData& moveData, const CSolidObject* object) const
{
	if (!object->blocking) { return false; }
	if (dynamic_cast<const CFeature*>(object) == NULL) { return true; }

	return (object->mass > moveData.crushStrength);
}

/*
 * check if an object is NON-blocking for a given MoveData
 * (ex. a submarine's moveDef vs. a surface ship object)
 */
bool CMoveMath::IsNonBlocking(const MoveData& moveData, const CSolidObject* obstacle) const
{
	if (!obstacle->blocking) {
		return true;
	}

	const CSolidObject* unit = moveData.tempOwner;

	const int hx = int(obstacle->pos.x / SQUARE_SIZE);
	const int hz = int(obstacle->pos.z / SQUARE_SIZE);
	const int hi = (hx >> 1) + (hz >> 1) * gs->hmapx;
	const int hj = (gs->mapx >> 1) * (gs->mapy >> 1); // sizeof(mipHeightmap[1])

	if (hi < 0 || hi >= hj) {
		// unit is out of map bounds, so cannot be blocked
		return true;
	}

	if (unit != NULL) {
		// simple case: if unit and obstacle have non-zero
		// vertical separation as measured by their (model)
		// heights, unit can always pass
		// note: in many cases separation is not sufficient
		// even when it logically should be (submarines vs.
		// floating DT in shallow water)
		const float elevDif = streflop::fabs(unit->pos.y - obstacle->pos.y);
		const float hghtSum = streflop::fabs(unit->height) + streflop::fabs(obstacle->height);

		if ((elevDif - hghtSum) >= 1.0f) { return true;  }
		if ( elevDif            <= 1.0f) { return false; }
	}

	if (moveData.terrainClass == MoveData::Land) {
		// if unit is restricted to land with > 0 height,
		// it can not be blocked by underwater obstacles
		return (obstacle->isUnderWater);
	}

	const bool unitSub = moveData.subMarine;
	const bool obstSub = (obstacle->mobility && obstacle->mobility->subMarine);

	// some objects appear to have negative model heights
	// (the S3DO parsers allow it for some reason), take
	// the absolute value to prevent them being regarded
	// as non-blocking
	const float oy = obstacle->pos.y;
	const float oh = std::max(obstacle->height, -obstacle->height);
	const float gy = readmap->mipHeightmap[1][hi];

	// remaining conditions under which obstacle does NOT block unit
	//   1.
	//      (unit is ground-following or not currently in water) and
	//      obstacle's altitude minus its model height leaves a gap
	//      between it and the ground
	//   2.
	//      unit is a submarine, obstacle sticks out above-water
	//      (and not itself flagged as a submarine) *OR* unit is
	//      not a submarine and obstacle is (fully under-water or
	//      flagged as a submarine)
	//      NOTE: causes stacking for submarines that are *not*
	//      explicitly flagged as such
	//
	// note that these conditions can lead to a certain degree of
	// clipping, for full 3D accuracy the height of the movedata
	// owner would need to be accessible (but the path-estimator
	// defs aren't tied to any)
	if (moveData.followGround || (gy > 0.0f)) {
		return ((oy - oh) > gy);
	} else {
		if (unitSub) {
			return (((oy + oh) >  0.0f) && !obstSub);
		} else {
			return (((oy + oh) <= 0.0f) ||  obstSub);
		}
	}

	return false;
}



/* Check if a single square is accessable (for any object which uses the given movedata). */
int CMoveMath::SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare) const
{
	// bounds-check
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 1;
	}

	int r = 0;
	const BlockingMapCell& c = groundBlockingObjectMap->GetCell(xSquare + zSquare * gs->mapx);

	for (BlockingMapCellIt it = c.begin(); it != c.end(); it++) {
		CSolidObject* obstacle = it->second;

		if (IsNonBlocking(moveData, obstacle)) {
			continue;
		}

		if (!obstacle->immobile) {
			// mobile obstacle
			if (obstacle->isMoving) {
				r |= BLOCK_MOVING;
			} else {
				CUnit& u = *static_cast<CUnit*>(obstacle);
				if (!u.beingBuilt && u.commandAI->commandQue.empty()) {
					// idling mobile unit
					r |= BLOCK_MOBILE;
				} else {
					// busy mobile unit (but not following path)
					r |= BLOCK_MOBILE_BUSY;
				}
			}
		} else {
			if (CrushResistant(moveData, obstacle)) {
				r |= BLOCK_STRUCTURE;
			}
		}
	}

	return r;
}
