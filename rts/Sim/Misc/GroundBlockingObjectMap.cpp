/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>

#include "GroundBlockingObjectMap.h"
#include "GlobalConstants.h"
#include "Map/ReadMap.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Sim/Path/IPathManager.h"
#include "System/creg/STL_Map.h"

CGroundBlockingObjectMap* groundBlockingObjectMap = NULL;

CR_BIND(CGroundBlockingObjectMap, (1))
CR_REG_METADATA(CGroundBlockingObjectMap, (
	CR_MEMBER(groundBlockingMap)
))



inline static const int GetObjectID(CSolidObject* obj)
{
	const int id = obj->GetBlockingMapID();
	// object should always be a derived type
	assert(id != -1);
	return id;
}


void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object)
{
	if (object->blockMap != NULL) {
		// if object has a yardmap, add it to map selectively
		// (checking the specific state of each yardmap cell)
		AddGroundBlockingObject(object, YARDMAP_BLOCKED);
		return;
	}

	const int objID = GetObjectID(object);

	object->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_BLOCKING);
	object->mapPos = object->GetMapPos();
	object->groundBlockPos = object->pos;

	const int bx = object->mapPos.x, sx = object->xsize;
	const int bz = object->mapPos.y, sz = object->zsize;
	const int minXSqr = bx, maxXSqr = bx + sx;
	const int minZSqr = bz, maxZSqr = bz + sz;

	for (int zSqr = minZSqr; zSqr < maxZSqr; zSqr++) {
		for (int xSqr = minXSqr; xSqr < maxXSqr; xSqr++) {
			BlockingMapCell& cell = groundBlockingMap[xSqr + zSqr * mapDims.mapx];
			cell[objID] = object;
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef == NULL && pathManager != NULL) {
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr, TERRAINCHANGE_OBJECT_INSERTED);
	}
}

void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object, const YardMapStatus& mask)
{
	const int objID = GetObjectID(object);

	object->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_BLOCKING);
	object->mapPos = object->GetMapPos();
	object->groundBlockPos = object->pos;

	const int bx = object->mapPos.x, sx = object->xsize;
	const int bz = object->mapPos.y, sz = object->zsize;
	const int minXSqr = bx, maxXSqr = bx + sx;
	const int minZSqr = bz, maxZSqr = bz + sz;

	for (int z = minZSqr; z < maxZSqr; z++) {
		for (int x = minXSqr; x < maxXSqr; x++) {
			// unit yardmaps always contain sx=UnitDef::xsize * sz=UnitDef::zsize
			// cells (the unit->moveDef footprint can have different dimensions)
			const float3 testPos = float3(x, 0.0f, z) * SQUARE_SIZE;

			if (object->GetGroundBlockingMaskAtPos(testPos) & mask) {
				BlockingMapCell& cell = groundBlockingMap[x + (z) * mapDims.mapx];
				cell[objID] = object;
			}
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef == NULL && pathManager != NULL) {
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr, TERRAINCHANGE_OBJECT_INSERTED_YM);
	}
}


void CGroundBlockingObjectMap::RemoveGroundBlockingObject(CSolidObject* object)
{
	const int objID = GetObjectID(object);

	const int bx = object->mapPos.x;
	const int bz = object->mapPos.y;
	const int sx = object->xsize;
	const int sz = object->zsize;

	object->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_BLOCKING);

	for (int z = bz; z < bz + sz; ++z) {
		for (int x = bx; x < bx + sx; ++x) {
			const int idx = x + z * mapDims.mapx;

			BlockingMapCell& cell = groundBlockingMap[idx];
			cell.erase(objID);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef == NULL && pathManager != NULL) {
		pathManager->TerrainChange(bx, bz, bx + sx, bz + sz, TERRAINCHANGE_OBJECT_DELETED);
	}
}


/**
  * Checks if a ground-square is blocked.
  * If it's not blocked (empty), then NULL is returned. Otherwise, a
  * pointer to the top-most / bottom-most blocking object is returned.
  */
CSolidObject* CGroundBlockingObjectMap::GroundBlockedUnsafe(int mapSquare) const {
	const BlockingMapCell& cell = groundBlockingMap[mapSquare];

	if (cell.empty())
		return NULL;

	return ((cell.begin())->second);
}


CSolidObject* CGroundBlockingObjectMap::GroundBlocked(int x, int z) const {
	if (x < 0 || x >= mapDims.mapx || z < 0 || z >= mapDims.mapy)
		return NULL;

	return GroundBlockedUnsafe(x + z * mapDims.mapx);
}


CSolidObject* CGroundBlockingObjectMap::GroundBlocked(const float3& pos) const {
	const int xSqr = int(pos.x / SQUARE_SIZE);
	const int zSqr = int(pos.z / SQUARE_SIZE);
	return GroundBlocked(xSqr, zSqr);
}


bool CGroundBlockingObjectMap::GroundBlocked(int x, int z, CSolidObject* ignoreObj) const
{
	if ((unsigned)x >= mapDims.mapx || (unsigned)z >= mapDims.mapy)
		return false;

	const int mapSquare = z * mapDims.mapx + x;
	const BlockingMapCell& cell = groundBlockingMap[mapSquare];

	if (cell.empty())
		return false;

	const int objID = GetObjectID(ignoreObj);
	BlockingMapCellIt it = cell.begin();

	if (it->first != objID) {
		// there are other objects blocking the square
		return true;
	}

	// ignoreObj is in the square. Check if there are other objects, too
	return (cell.size() >= 2);
}


bool CGroundBlockingObjectMap::GroundBlocked(const float3& pos, CSolidObject* ignoreObj) const
{
	const int xSqr = unsigned(pos.x) / SQUARE_SIZE;
	const int zSqr = unsigned(pos.z) / SQUARE_SIZE;
	return GroundBlocked(xSqr, zSqr, ignoreObj);
}



/**
  * Opens up a yard in a blocked area.
  * When a factory opens up, for example.
  */
void CGroundBlockingObjectMap::OpenBlockingYard(CSolidObject* object)
{
	RemoveGroundBlockingObject(object);
	AddGroundBlockingObject(object, YARDMAP_YARDFREE);
	object->yardOpen = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
///

/**
  * Closes a yard, blocking the area.
  * When a factory closes, for example.
  */
void CGroundBlockingObjectMap::CloseBlockingYard(CSolidObject* object)
{
	RemoveGroundBlockingObject(object);
	AddGroundBlockingObject(object, YARDMAP_YARDBLOCKED);
	object->yardOpen = false;
}


inline bool CGroundBlockingObjectMap::CheckYard(CSolidObject* yardUnit, const YardMapStatus& mask) const
{
	for (int z = yardUnit->mapPos.y; z < yardUnit->mapPos.y + yardUnit->zsize; ++z) {
		for (int x = yardUnit->mapPos.x; x < yardUnit->mapPos.x + yardUnit->xsize; ++x) {
			if (yardUnit->GetGroundBlockingMaskAtPos(float3(x * SQUARE_SIZE, 0.0f, z * SQUARE_SIZE)) & mask)
				if (GroundBlocked(x, z, yardUnit))
					return false;
		}
	}

	return true;
}


bool CGroundBlockingObjectMap::CanOpenYard(CSolidObject* yardUnit) const
{
	return CheckYard(yardUnit, YARDMAP_YARDINV);
}

bool CGroundBlockingObjectMap::CanCloseYard(CSolidObject* yardUnit) const
{
	return CheckYard(yardUnit, YARDMAP_YARD);
}
