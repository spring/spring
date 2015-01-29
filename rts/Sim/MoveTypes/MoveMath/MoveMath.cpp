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
	switch (moveDef.speedModClass) {
		case MoveDef::Tank: // fall-through
		case MoveDef::KBot:  { return (CGround::GetHeightReal      (xSqr * SQUARE_SIZE, zSqr * SQUARE_SIZE) + 10.0f); } break;
		case MoveDef::Hover: { return (CGround::GetHeightAboveWater(xSqr * SQUARE_SIZE, zSqr * SQUARE_SIZE) + 10.0f); } break;
		case MoveDef::Ship:  { return (                                                                       0.0f); } break;
	}

	return 0.0f;
}

float CMoveMath::yLevel(const MoveDef& moveDef, const float3& pos)
{
	switch (moveDef.speedModClass) {
		case MoveDef::Tank: // fall-through
		case MoveDef::KBot:  { return (CGround::GetHeightReal      (pos.x, pos.z) + 10.0f); } break;
		case MoveDef::Hover: { return (CGround::GetHeightAboveWater(pos.x, pos.z) + 10.0f); } break;
		case MoveDef::Ship:  { return (                                             0.0f); } break;
	}

	return 0.0f;
}



/* calculate the local speed-modifier for this MoveDef */
float CMoveMath::GetPosSpeedMod(const MoveDef& moveDef, int xSquare, int zSquare)
{
	if (xSquare < 0 || zSquare < 0 || xSquare >= mapDims.mapx || zSquare >= mapDims.mapy)
		return 0.0f;

	const int square = (xSquare >> 1) + ((zSquare >> 1) * mapDims.hmapx);
	const int squareTerrType = readMap->GetTypeMapSynced()[square];

	const float height  = readMap->GetMIPHeightMapSynced(1)[square];
	const float slope   = readMap->GetSlopeMapSynced()[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	switch (moveDef.speedModClass) {
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
	if (xSquare < 0 || zSquare < 0 || xSquare >= mapDims.mapx || zSquare >= mapDims.mapy)
		return 0.0f;

	const int square = (xSquare >> 1) + ((zSquare >> 1) * mapDims.hmapx);
	const int squareTerrType = readMap->GetTypeMapSynced()[square];

	const float height = readMap->GetMIPHeightMapSynced(1)[square];
	const float slope  = readMap->GetSlopeMapSynced()[square];

	const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[squareTerrType];

	float3 sqrNormal = readMap->GetCenterNormalsSynced()[xSquare + zSquare * mapDims.mapx];

	#if 1
	// with a flat normal, only consider the normalized xz-direction
	// (the actual steepness is represented by the "slope" variable)
	sqrNormal.SafeNormalize2D();
	#endif

	// note: moveDir is (or should be) a unit vector in the xz-plane, y=0
	// scale is negative for "downhill" slopes, positive for "uphill" ones
	const float dirSlopeMod = -moveDir.dot(sqrNormal);
	//
	// treat every move-direction as either fully uphill or fully downhill
	// (otherwise units are still able to move orthogonally across vertical
	// faces --> fixed)
	//   const float dirSlopeMod = -Sign(moveDir.dot(sqrNormal));

	switch (moveDef.speedModClass) {
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
			if (ret & BLOCK_STRUCTURE) return ret;
		}
	}
	return ret;
}

bool CMoveMath::CrushResistant(const MoveDef& colliderMD, const CSolidObject* collidee)
{
	if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
		return false;
	if (!collidee->crushable)
		return true;

	return (collidee->crushResistance > colliderMD.crushStrength);
}

bool CMoveMath::IsNonBlocking(const MoveDef& colliderMD, const CSolidObject* collidee, const CSolidObject* collider)
{
	if (collider == collidee)
		return true;
	if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
		return true;
	// if obstacle is out of map bounds, it cannot block us
	if (!collidee->pos.IsInBounds())
		return true;
	// same if obstacle is not currently marked on blocking-map
	if (!collidee->IsBlocking())
		return true;

	if (collider != NULL)
		return (IsNonBlocking(collidee, collider));

	// remaining conditions under which obstacle does NOT block unit
	// only reachable from stand-alone PE invocations or GameHelper
	//   1.
	//      unit is a submarine, obstacle sticks out above-water
	//      (and not itself flagged as a submarine) *OR* unit is
	//      not a submarine and obstacle is (fully under-water or
	//      flagged as a submarine)
	//
	//      NOTE:
	//        do we want to allow submarines to pass underneath
	//        any obstacle even if it is 99% submerged already?
	//
	//        will cause stacking for submarines that are *not*
	//        explicitly flagged as such in their MoveDefs
	//
	// note that these condition(s) can lead to a certain degree of
	// clipping: for full 3D accuracy the height of the MoveDef's
	// owner would need to be accessible (but the path-estimator
	// defs aren't tied to any collider instances) --> add extra
	// "clearance" parameter to MoveDef?
	//
	#define IS_SUBMARINE(md) ((md) != NULL && (md)->subMarine)

	if (IS_SUBMARINE(&colliderMD)) {
		return (!collidee->IsUnderWater() && !IS_SUBMARINE(collidee->moveDef));
	} else {
		return ( collidee->IsUnderWater() ||  IS_SUBMARINE(collidee->moveDef));
	}

	#undef IS_SUBMARINE

	return false;
}

bool CMoveMath::IsNonBlocking(const CSolidObject* collidee, const CSolidObject* collider)
{
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
	if ((collider->pos.y + math::fabs(collider->height)) < collidee->pos.y) return true;
	if ((collidee->pos.y + math::fabs(collidee->height)) < collider->pos.y) return true;

	return false;
}


CMoveMath::BlockType CMoveMath::SquareIsBlocked(const MoveDef& moveDef, int xSquare, int zSquare, const CSolidObject* collider)
{
	if ((unsigned)xSquare >= mapDims.mapx || (unsigned)zSquare >= mapDims.mapy)
		return BLOCK_IMPASSABLE;

	BlockType r = BLOCK_NONE;

	const BlockingMapCell& c = groundBlockingObjectMap->GetCell(zSquare * mapDims.mapx + xSquare);

	for (BlockingMapCellIt it = c.begin(); it != c.end(); ++it) {
		const CSolidObject* collidee = it->second;

		if (IsNonBlocking(moveDef, collidee, collider))
			continue;

		if (!collidee->immobile) {
			// mobile obstacle (must be a unit) --> if
			// moving, it is probably following a path
			if (collidee->IsMoving()) {
				r |= BLOCK_MOVING;
			} else {
				if ((static_cast<const CUnit*>(collidee))->IsIdle()) {
					// idling (no orders) mobile unit
					r |= BLOCK_MOBILE;
				} else {
					// busy mobile unit
					r |= BLOCK_MOBILE_BUSY;
				}
			}
		} else {
			if (CrushResistant(moveDef, collidee)) {
				r |= BLOCK_STRUCTURE;
			}
		}
	}

	return r;
}

