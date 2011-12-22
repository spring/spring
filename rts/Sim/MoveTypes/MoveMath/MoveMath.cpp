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

/* Check if a given square-position is accessable by the movedata footprint. */
CMoveMath::BlockType CMoveMath::IsBlockedNoSpeedModCheck(const MoveData& moveData, int xSquare, int zSquare) const
{
	BlockType ret = BLOCK_NONE;
	const int xmin = xSquare - moveData.xsizeh, xmax = xSquare + moveData.xsizeh;
	const int zmin = zSquare - moveData.zsizeh, zmax = zSquare + moveData.zsizeh;
	const int xstep = 2, zstep = 2;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int x = xmin; x <= xmax; x += xstep) {
		for (int z = zmin; z <= zmax; z += zstep) {
			ret |= SquareIsBlocked(moveData, x, z);
		}
	}

	return ret;
}

/* Optimized function to check if a given square-position has a structure block. */
bool CMoveMath::IsBlockedStructure(const MoveData& moveData, int xSquare, int zSquare) const
{
	const int xmin = xSquare - moveData.xsizeh, xmax = xSquare + moveData.xsizeh;
	const int zmin = zSquare - moveData.zsizeh, zmax = zSquare + moveData.zsizeh;
	const int xstep = 2, zstep = 2;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int x = xmin; x <= xmax; x += xstep) {
		for (int z = zmin; z <= zmax; z += zstep) {
			if (SquareIsBlocked(moveData, x, z) & BLOCK_STRUCTURE)
				return true;
		}
	}

	return false;
}

/* Optimized function to check if the square at the given position has a structure block, 
   provided that the square at (xSquare - 1, zSquare) did not have a structure block */
bool CMoveMath::IsBlockedStructureXmax(const MoveData& moveData, int xSquare, int zSquare) const
{
	const int                                   xmax = xSquare + moveData.xsizeh;
	const int zmin = zSquare - moveData.zsizeh, zmax = zSquare + moveData.zsizeh;
	const int zstep = 2;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int z = zmin; z <= zmax; z += zstep) {
		if (SquareIsBlocked(moveData, xmax, z) & BLOCK_STRUCTURE)
			return true;
	}

	return false;
}

/* Optimized function to check if the square at the given position has a structure block, 
   provided that the square at (xSquare, zSquare - 1) did not have a structure block */
bool CMoveMath::IsBlockedStructureZmax(const MoveData& moveData, int xSquare, int zSquare) const
{
	const int xmin = xSquare - moveData.xsizeh, xmax = xSquare + moveData.xsizeh;
	const int                                   zmax = zSquare + moveData.zsizeh;
	const int xstep = 2;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int x = xmin; x <= xmax; x += xstep) {
		if (SquareIsBlocked(moveData, x, zmax) & BLOCK_STRUCTURE)
			return true;
	}

	return false;
}

/*
 * check if an object is resistant to being
 * crushed by a unit (with given MoveData)
 * NOTE: modify for selective blocking
 */
bool CMoveMath::CrushResistant(const MoveData& colliderMD, const CSolidObject* collidee)
{
	if (!collidee->blocking) { return false; }
	if (!collidee->crushable) { return true; }

	return (collidee->crushResistance > colliderMD.crushStrength);
}

/*
 * check if an object is NON-blocking for a given MoveData
 * (ex. a submarine's moveDef vs. a surface ship object)
 */
bool CMoveMath::IsNonBlocking(const MoveData& colliderMD, const CSolidObject* collidee)
{
	const CSolidObject* collider = colliderMD.tempOwner;

	if (!collidee->blocking)
		return true;

	if (collider == collidee)
		return true;

	// if obstacle is out of map bounds, it cannot block us
	if (!collidee->pos.IsInBounds())
		return true;

	// if unit is restricted to land with height > 0,
	// it can not be blocked by underwater obstacles
	if (colliderMD.terrainClass == MoveData::Land)
		return (collidee->isUnderWater);

	// some objects appear to have negative model heights
	// (the model parsers allow it for some reason), take
	// absolute values to prevent them from being regarded
	// as non-blocking
	const float colliderMdlHgt = (collider != NULL)? math::fabs(collider->height): 1e6;
	const float collideeMdlHgt =                     math::fabs(collidee->height);
	const float colliderGndAlt = (collider != NULL)? collider->pos.y: 1e6f;
	const float collideeGndAlt =                     collidee->pos.y;

	if (collider != NULL) {
		// simple case: if unit and obstacle have non-zero
		// vertical separation as measured by their (model)
		// heights, unit can always pass obstacle
		//
		// note: in many cases separation is not sufficient
		// even when it logically should be (submarines vs.
		// floating DT in shallow water eg.)
		// note: if unit and obstacle are on a steep slope,
		// this can return true even when their horizontal
		// separation points to a collision
		if (math::fabs(colliderGndAlt - collideeGndAlt) <= 1.0f) return false;
		if ((colliderGndAlt + colliderMdlHgt) < collideeGndAlt) return true;
		if ((collideeGndAlt + collideeMdlHgt) < colliderGndAlt) return true;

		return false;
	}

	// (code below is only reachable from stand-alone PE invocations)
	// remaining conditions under which obstacle does NOT block unit
	//   1.
	//      unit is ground-following and obstacle's altitude
	//      minus its model height leaves a gap between it and
	//      the ground large enough for unit to pass
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
	//
	if (colliderMD.followGround) {
		const float collideeMinHgt = collideeGndAlt - collideeMdlHgt;
		const float colliderMaxHgt = ground->GetHeightReal(collidee->pos.x, collidee->pos.z) + (SQUARE_SIZE >> 1);
		// FIXME: would be the correct way, but values are invalid here
		// const float colliderMaxHgt = colliderGndAlt + colliderMdlHgt;

		return (collideeMinHgt > colliderMaxHgt);
	} else {
		const bool colliderIsSub = colliderMD.subMarine;
		const bool collideeIsSub = (collidee->mobility != NULL && collidee->mobility->subMarine);

		if (colliderIsSub) {
			return (((collideeGndAlt + collideeMdlHgt) >  0.0f) && !collideeIsSub);
		} else {
			return (((collideeGndAlt + collideeMdlHgt) <= 0.0f) ||  collideeIsSub);
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
