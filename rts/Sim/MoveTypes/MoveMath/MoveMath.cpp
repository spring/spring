/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveMath.h"

#include "Map/MapInfo.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"

bool CMoveMath::noHoverWaterMove = false;
float CMoveMath::waterDamageCost = 0.0f;



float CMoveMath::yLevel(const MoveDef& moveDef, int xSqr, int zSqr)
{
	switch (moveDef.moveFamily) {
		case MoveDef::Tank: // fall-through
		case MoveDef::KBot:  { return (readmap->GetCenterHeightMapSynced()[xSqr + zSqr * gs->mapx]);                 } break; // NOTE: why not just GetHeightReal too?
		case MoveDef::Hover: { return (ground->GetHeightAboveWater(xSqr * SQUARE_SIZE, zSqr * SQUARE_SIZE) + 10.0f); } break;
		case MoveDef::Ship:  { return (                                                                       0.0f); } break;
	}

	return 0.0f;
}

float CMoveMath::yLevel(const MoveDef& moveDef, const float3& pos)
{
	switch (moveDef.moveFamily) {
		case MoveDef::Tank: // fall-through
		case MoveDef::KBot:  { return (ground->GetHeightReal      (pos.x, pos.z) + 10.0f); } break;
		case MoveDef::Hover: { return (ground->GetHeightAboveWater(pos.x, pos.z) + 10.0f); } break;
		case MoveDef::Ship:  { return (                                             0.0f); } break;
	}

	return 0.0f;
}



/* calculate the local speed-modifier for this MoveDef */
float CMoveMath::GetPosSpeedMod(const MoveDef& moveDef, int xSquare, int zSquare)
{
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 0.0f;
	}

	const int square = (xSquare >> 1) + ((zSquare >> 1) * gs->hmapx);
	const int squareTerrType = readmap->GetTypeMapSynced()[square];

	const float height  = readmap->GetMIPHeightMapSynced(1)[square];
	const float slope   = readmap->GetSlopeMapSynced()[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	switch (moveDef.moveFamily) {
		case MoveDef::Tank:  { return (GroundSpeedMod(moveDef, height, slope) * tt.tankSpeed ); } break;
		case MoveDef::KBot:  { return (GroundSpeedMod(moveDef, height, slope) * tt.kbotSpeed ); } break;
		case MoveDef::Hover: { return ( HoverSpeedMod(moveDef, height, slope) * tt.hoverSpeed); } break;
		case MoveDef::Ship:  { return (  ShipSpeedMod(moveDef, height, slope) * tt.shipSpeed ); } break;
		default: {} break;
	}

	return 0.0f;
}

float CMoveMath::GetPosSpeedMod(const MoveDef& moveDef, int xSquare, int zSquare, const float3& moveDir)
{
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return 0.0f;
	}

	const int square = (xSquare >> 1) + ((zSquare >> 1) * gs->hmapx);
	const int squareTerrType = readmap->GetTypeMapSynced()[square];

	const float height = readmap->GetMIPHeightMapSynced(1)[square];
	const float slope  = readmap->GetSlopeMapSynced()[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	#if 1
	// with a flat normal, only consider the normalized xz-direction
	// (the actual steepness is represented by the "slope" variable)
	float3 sqrNormal = readmap->GetCenterNormalsSynced()[xSquare + zSquare * gs->mapx];
		sqrNormal.y = 0.0f;
		sqrNormal.SafeNormalize();
	#else
	const float3& sqrNormal = readmap->GetCenterNormalsSynced()[xSquare + zSquare * gs->mapx];
	#endif

	// note: moveDir is (or should be) a unit vector in the xz-plane, y=0
	// scale is negative for "downhill" slopes, positive for "uphill" ones
	const float dirSlopeMod = -moveDir.dot(sqrNormal);
	//
	// treat every move-direction as either fully uphill or fully downhill
	// (otherwise units are still able to move orthogonally across vertical
	// faces --> fixed)
	//   const float dirSlopeMod = (moveDir.dot(sqrNormal) < 0.0f) * 2.0f - 1.0f;

	switch (moveDef.moveFamily) {
		case MoveDef::Tank:  { return (GroundSpeedMod(moveDef, height, slope, dirSlopeMod) * tt.tankSpeed ); } break;
		case MoveDef::KBot:  { return (GroundSpeedMod(moveDef, height, slope, dirSlopeMod) * tt.kbotSpeed ); } break;
		case MoveDef::Hover: { return ( HoverSpeedMod(moveDef, height, slope, dirSlopeMod) * tt.hoverSpeed); } break;
		case MoveDef::Ship:  { return (  ShipSpeedMod(moveDef, height, slope, dirSlopeMod) * tt.shipSpeed ); } break;
		default: {} break;
	}

	return 0.0f;
}

/* Check if a given square-position is accessable by the MoveDef footprint. */
CMoveMath::BlockType CMoveMath::IsBlockedNoSpeedModCheck(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	BlockType ret = BLOCK_NONE;
	const int xmin = xSquare - moveDef.xsizeh, xmax = xSquare + moveDef.xsizeh;
	const int zmin = zSquare - moveDef.zsizeh, zmax = zSquare + moveDef.zsizeh;
	const int xstep = 2, zstep = 2;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int z = zmin; z <= zmax; z += zstep) {
		for (int x = xmin; x <= xmax; x += xstep) {
			ret |= SquareIsBlocked(moveDef, x, z, collider);
		}
	}
	return ret;
}

/* Optimized function to check if a given square-position has a structure block. */
bool CMoveMath::IsBlockedStructure(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	const int xmin = xSquare - moveDef.xsizeh, xmax = xSquare + moveDef.xsizeh;
	const int zmin = zSquare - moveDef.zsizeh, zmax = zSquare + moveDef.zsizeh;
	const int xstep = 2, zstep = 2;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int z = zmin; z <= zmax; z += zstep) {
		for (int x = xmin; x <= xmax; x += xstep) {
			if (SquareIsBlocked(moveDef, x, z, collider) & BLOCK_STRUCTURE)
				return true;
		}
	}

	return false;
}

/* Optimized function to check if the square at the given position has a structure block, 
   provided that the square at (xSquare - 1, zSquare) did not have a structure block */
bool CMoveMath::IsBlockedStructureXmax(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	const int                                  xmax = xSquare + moveDef.xsizeh;
	const int zmin = zSquare - moveDef.zsizeh, zmax = zSquare + moveDef.zsizeh;
	const int zstep = 2;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int z = zmin; z <= zmax; z += zstep) {
		if (SquareIsBlocked(moveDef, xmax, z, collider) & BLOCK_STRUCTURE)
			return true;
	}

	return false;
}

/* Optimized function to check if the square at the given position has a structure block, 
   provided that the square at (xSquare, zSquare - 1) did not have a structure block */
bool CMoveMath::IsBlockedStructureZmax(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	const int xmin = xSquare - moveDef.xsizeh, xmax = xSquare + moveDef.xsizeh;
	const int                                  zmax = zSquare + moveDef.zsizeh;
	const int xstep = 2;
	// (footprints are point-symmetric around <xSquare, zSquare>)
	for (int x = xmin; x <= xmax; x += xstep) {
		if (SquareIsBlocked(moveDef, x, zmax, collider) & BLOCK_STRUCTURE)
			return true;
	}

	return false;
}

/*
 * check if an object is resistant to being
 * crushed by a unit (with given MoveDef)
 * NOTE: modify for selective blocking
 */
bool CMoveMath::CrushResistant(const MoveDef& colliderMD, const CSolidObject* collidee)
{
	if (!collidee->blocking) { return false; }
	if (!collidee->crushable) { return true; }

	return (collidee->crushResistance > colliderMD.crushStrength);
}

/*
 * check if an object is NON-blocking for a given MoveDef
 * (ex. a submarine's moveDef vs. a surface ship object)
 */
bool CMoveMath::IsNonBlocking(const MoveDef& colliderMD, const CSolidObject* collidee, const CSolidObject* collider)
{
	if (!collidee->blocking)
		return true;

	if (collider == collidee)
		return true;

	// if obstacle is out of map bounds, it cannot block us
	if (!collidee->pos.IsInBounds())
		return true;


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
	// clipping, for full 3D accuracy the height of the MoveDef
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
		const bool collideeIsSub = (collidee->moveDef != NULL && collidee->moveDef->subMarine);

		if (colliderIsSub) {
			return (((collideeGndAlt + collideeMdlHgt) >  0.0f) && !collideeIsSub);
		} else {
			return (((collideeGndAlt + collideeMdlHgt) <= 0.0f) ||  collideeIsSub);
		}
	}

	return false;
}



/* Check if a single square is accessable (for any object which uses the given MoveDef). */
CMoveMath::BlockType CMoveMath::SquareIsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	// bounds-check
	if (xSquare < 0 || zSquare < 0 || xSquare >= gs->mapx || zSquare >= gs->mapy) {
		return BLOCK_IMPASSABLE;
	}

	BlockType r = BLOCK_NONE;
	const BlockingMapCell& c = groundBlockingObjectMap->GetCell(xSquare + zSquare * gs->mapx);

	for (BlockingMapCellIt it = c.begin(); it != c.end(); ++it) {
		const CSolidObject* obstacle = it->second;

		if (IsNonBlocking(moveDef, obstacle, collider)) {
			continue;
		}

		if (!obstacle->immobile) {
			// mobile obstacle
			if (obstacle->isMoving) {
				r |= BLOCK_MOVING;
			} else {
				const CUnit& u = *static_cast<const CUnit*>(obstacle);
				if (!u.beingBuilt && u.commandAI->commandQue.empty()) {
					// idling mobile unit
					r |= BLOCK_MOBILE;
				} else {
					// busy mobile unit (but not following path)
					r |= BLOCK_MOBILE_BUSY;
				}
			}
		} else {
			if (CrushResistant(moveDef, obstacle)) {
				r |= BLOCK_STRUCTURE;
			}
		}
	}

	return r;
}

