/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveMath.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "System/mmgr.h"

CR_BIND_INTERFACE(CMoveMath);

/* Converts a point-request into a square-positional request. */
float CMoveMath::yLevel(const float3& pos) const
{
	return yLevel((pos.x / SQUARE_SIZE), (pos.z / SQUARE_SIZE));
}


/* calculate the local speed-modifier for this movedata */
float CMoveMath::GetPosSpeedMod(const MoveData& moveData, int xSquare, int zSquare) const
{
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 0.0f;
	}

	const int square = (xSquare >> 1) + ((zSquare >> 1) * gs->hmapx);
	const int squareTerrType = readmap->GetTypeMapSynced()[square];

	const float height  = readmap->GetMIPHeightMapSynced(1)[square];
	const float slope   = readmap->GetSlopeMapSynced()[square];

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

float CMoveMath::GetPosSpeedMod(const MoveData& moveData, int xSquare, int zSquare, const float3& moveDir) const
{
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 0.0f;
	}

	const int square = (xSquare >> 1) + ((zSquare >> 1) * gs->hmapx);
	const int squareTerrType = readmap->GetTypeMapSynced()[square];

	const float height  = readmap->GetMIPHeightMapSynced(1)[square];
	const float slope   = readmap->GetSlopeMapSynced()[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	float3 flatNorm = readmap->GetCenterNormalsSynced()[xSquare + zSquare * gs->mapx];
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
CMoveMath::BlockType CMoveMath::IsBlocked(const MoveData& moveData, const float3& pos) const
{
	return IsBlocked(moveData, (pos.x / SQUARE_SIZE), (pos.z / SQUARE_SIZE));
}

/* Check if a given square-position is accessable by the movedata footprint. */
CMoveMath::BlockType CMoveMath::IsBlocked(const MoveData& moveData, int xSquare, int zSquare) const
{
	if (GetPosSpeedMod(moveData, xSquare, zSquare) == 0.0f) {
		return BLOCK_IMPASSABLE;
	}

	BlockType ret = BLOCK_NONE;

	const int xSize = moveData.xsize;
	const int zSize = moveData.zsize;
	const bool xSizeOdd = (xSize & 1) != 0;
	const bool zSizeOdd = (zSize & 1) != 0;
	const int xstep = 2;
	const int zstep = 2;
	const int xmin = xSquare - (xSize >> 1), xmax = xSquare + (xSize >> 1);
	const int zmin = zSquare - (zSize >> 1), zmax = zSquare + (zSize >> 1);

	if (xSizeOdd && zSizeOdd) {
		if (xSize <= 3 && zSize <= 3) {
			// only check squares under the footprint-corners
			// (footprints are point-symmetric around <xSquare, zSquare>)
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
	}
	else {
		for (int x = xmin; x <= xmax; x += xstep) {
			for (int z = zmin; z <= zmax; z += zstep) {
				ret |= SquareIsBlocked(moveData, x, z);
			}
		}
		BlockType bt1 = BLOCK_NONE, bt2 = BLOCK_NONE, bt3 = BLOCK_NONE, bt4 = BLOCK_NONE;
		if (!xSizeOdd && xSize > 2) {
			for (int z = zmin; z <= zmax; z += zstep) {
				bt1 |= SquareIsBlocked(moveData, xmin - 1, z);
				bt3 |= SquareIsBlocked(moveData, xmax + 1, z);
			}
		}
		if (!zSizeOdd && zSize > 2) {
			for (int x = xmin; x <= xmax; x += xstep) {
				bt2 |= SquareIsBlocked(moveData, x, zmin - 1);
				bt4 |= SquareIsBlocked(moveData, x, zmax + 1);
			}
		}
		// Try extending the footprint towards each possible "even" side, and
		// only consider it blocked if all those sides have a matching BlockType.
		// This is an approximation, but better than truncating it to odd size.
		if (!xSizeOdd && !zSizeOdd) {
			BlockType bta = bt1 | bt2 | SquareIsBlocked(moveData, xmin - 1, zmin - 1);
			BlockType btb = bt1 | bt4 | SquareIsBlocked(moveData, xmin - 1, zmax + 1);
			BlockType btc = bt3 | bt2 | SquareIsBlocked(moveData, xmax + 1, zmin - 1);
			BlockType btd = bt3 | bt4 | SquareIsBlocked(moveData, xmax + 1, zmax + 1);
			ret |= (bta & btb & btc & btd);
		}
		else if (!xSizeOdd) {
			ret |= (bt1 & bt3);
		}
		else if (!zSizeOdd) {
			ret |= (bt2 & bt4);
		}
	}

	return ret;
}


/* Check if the Xmax edge of a given square-position is accessable by the movedata footprint. */
CMoveMath::BlockType CMoveMath::IsBlockedXmax(const MoveData& moveData, int xSquare, int zSquare) const
{
	if (GetPosSpeedMod(moveData, xSquare, zSquare) == 0.0f) {
		return BLOCK_IMPASSABLE;
	}

	BlockType ret = BLOCK_NONE;

	const int xSize = moveData.xsize;
	const int zSize = moveData.zsize;
	const bool xSizeOdd = (xSize & 1) != 0;
	const bool zSizeOdd = (zSize & 1) != 0;
	const int xstep = 2;
	const int zstep = 2;
	const int xmax = xSquare + (xSize >> 1);
	const int zmin = zSquare - (zSize >> 1), zmax = zSquare + (zSize >> 1);

	if (xSizeOdd && zSizeOdd) {
		if (xSize <= 3 && zSize <= 3) {
			// only check squares under the footprint-corners
			// (footprints are point-symmetric around <xSquare, zSquare>)
			ret |= SquareIsBlocked(moveData, xmax, zmin);
			ret |= SquareIsBlocked(moveData, xmax, zmax);
		} else {
			for (int z = zmin; z <= zmax; z += zstep) {
				ret |= SquareIsBlocked(moveData, xmax, z);
			}
		}
	}
	else {
		for (int z = zmin; z <= zmax; z += zstep) {
			ret |= SquareIsBlocked(moveData, xmax, z);
		}
		BlockType bt2 = BLOCK_NONE, bt3 = BLOCK_NONE, bt4 = BLOCK_NONE;
		if (!xSizeOdd && xSize > 2) {
			for (int z = zmin; z <= zmax; z += zstep) {
				bt3 |= SquareIsBlocked(moveData, xmax + 1, z);
			}
		}
		if (!zSizeOdd && zSize > 2) {
			bt2 |= SquareIsBlocked(moveData, xmax, zmin - 1);
			bt4 |= SquareIsBlocked(moveData, xmax, zmax + 1);
		}
		// Try extending the footprint towards each possible "even" side, and
		// only consider it blocked if all those sides have a matching BlockType.
		// This is an approximation, but better than truncating it to odd size.
		if (!xSizeOdd && !zSizeOdd) {
			BlockType btc = bt3 | bt2 | SquareIsBlocked(moveData, xmax + 1, zmin - 1);
			BlockType btd = bt3 | bt4 | SquareIsBlocked(moveData, xmax + 1, zmax + 1);
			ret |= (btc & btd);
		}
		else if (!xSizeOdd) {
			ret |= (bt3);
		}
		else if (!zSizeOdd) {
			ret |= (bt2 & bt4);
		}
	}

	return ret;
}


/* Check if the Zmax edge of a given square-position is accessable by the movedata footprint. */
CMoveMath::BlockType CMoveMath::IsBlockedZmax(const MoveData& moveData, int xSquare, int zSquare) const
{
	if (GetPosSpeedMod(moveData, xSquare, zSquare) == 0.0f) {
		return BLOCK_IMPASSABLE;
	}

	BlockType ret = BLOCK_NONE;

	const int xSize = moveData.xsize;
	const int zSize = moveData.zsize;
	const bool xSizeOdd = (xSize & 1) != 0;
	const bool zSizeOdd = (zSize & 1) != 0;
	const int xstep = 2;
	const int zstep = 2;
	const int xmin = xSquare - (xSize >> 1), xmax = xSquare + (xSize >> 1);
	const int zmax = zSquare + (zSize >> 1);

	if (xSizeOdd && zSizeOdd) {
		if (xSize <= 3 && zSize <= 3) {
			// only check squares under the footprint-corners
			// (footprints are point-symmetric around <xSquare, zSquare>)
			ret |= SquareIsBlocked(moveData, xmax, zmax);
			ret |= SquareIsBlocked(moveData, xmin, zmax);
		} else {
			for (int x = xmin; x <= xmax; x += xstep) {
				ret |= SquareIsBlocked(moveData, x, zmax);
			}
		}
	}
	else {
		for (int x = xmin; x <= xmax; x += xstep) {
			ret |= SquareIsBlocked(moveData, x, zmax);
		}
		BlockType bt1 = BLOCK_NONE, bt3 = BLOCK_NONE, bt4 = BLOCK_NONE;
		if (!xSizeOdd && xSize > 2) {
			bt1 |= SquareIsBlocked(moveData, xmin - 1, zmax);
			bt3 |= SquareIsBlocked(moveData, xmax + 1, zmax);
		}
		if (!zSizeOdd && zSize > 2) {
			for (int x = xmin; x <= xmax; x += xstep) {
				bt4 |= SquareIsBlocked(moveData, x, zmax + 1);
			}
		}
		// Try extending the footprint towards each possible "even" side, and
		// only consider it blocked if all those sides have a matching BlockType.
		// This is an approximation, but better than truncating it to odd size.
		if (!xSizeOdd && !zSizeOdd) {
			BlockType btb = bt1 | bt4 | SquareIsBlocked(moveData, xmin - 1, zmax + 1);
			BlockType btd = bt3 | bt4 | SquareIsBlocked(moveData, xmax + 1, zmax + 1);
			ret |= (btb & btd);
		}
		else if (!xSizeOdd) {
			ret |= (bt1 & bt3);
		}
		else if (!zSizeOdd) {
			ret |= (bt4);
		}
	}

	return ret;
}


/*
 * check if an object is blocking or not for a given MoveData (feature
 * objects block iif their mass exceeds the movedata's crush-strength).
 * NOTE: modify for selective blocking
 */
bool CMoveMath::CrushResistant(const MoveData& moveData, const CSolidObject* object)
{
	if (!object->blocking) { return false; }
	if (dynamic_cast<const CFeature*>(object) == NULL) { return true; }

	return (object->mass > moveData.crushStrength);
}

/*
 * check if an object is NON-blocking for a given MoveData
 * (ex. a submarine's moveDef vs. a surface ship object)
 */
bool CMoveMath::IsNonBlocking(const MoveData& moveData, const CSolidObject* obstacle)
{
	if (!obstacle->blocking) {
		return true;
	}

	const CSolidObject* unit = moveData.tempOwner;

	const int hx = int(obstacle->pos.x / SQUARE_SIZE);
	const int hz = int(obstacle->pos.z / SQUARE_SIZE);
	const int hi = (hx >> 1) + (hz >> 1) * gs->hmapx;
	const int hj = (gs->mapx >> 1) * (gs->mapy >> 1); // sizeof(GetMIPHeightMapSynced(1))

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
		const float elevDif = math::fabs(unit->pos.y - obstacle->pos.y);
		const float hghtSum = math::fabs(unit->height) + math::fabs(obstacle->height);

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
	const float gy = readmap->GetMIPHeightMapSynced(1)[hi];

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
CMoveMath::BlockType CMoveMath::SquareIsBlocked(const MoveData& moveData, int xSquare, int zSquare)
{
	// bounds-check
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return BLOCK_IMPASSABLE;
	}

	BlockType r = BLOCK_NONE;
	const BlockingMapCell& c = groundBlockingObjectMap->GetCell(xSquare + zSquare * gs->mapx);

	for (BlockingMapCellIt it = c.begin(); it != c.end(); ++it) {
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
