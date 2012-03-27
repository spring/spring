/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include "System/mmgr.h"

#include "GroundBlockingObjectMap.h"

#include "GlobalSynced.h"
#include "GlobalConstants.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Path/IPathManager.h"
#include "System/creg/STL_Map.h"
#include "lib/gml/gmlmut.h"

CGroundBlockingObjectMap* groundBlockingObjectMap;

CR_BIND(CGroundBlockingObjectMap, (1))
CR_REG_METADATA(CGroundBlockingObjectMap, (
	CR_MEMBER(groundBlockingMap)
));



inline static const int GetObjectID(CSolidObject* obj)
{
	const int id = obj->GetBlockingMapID();
	// object should always be a derived type
	assert(id != -1);
	return id;
}


void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object)
{
	GML_STDMUTEX_LOCK(block); // AddGroundBlockingObject

	const int objID = GetObjectID(object);

	object->isMarkedOnBlockingMap = true;
	object->mapPos = object->GetMapPos();

	if (object->immobile) {
		object->mapPos.x &= 0xfffffe;
		object->mapPos.y &= 0xfffffe;
	}

	const int bx = object->mapPos.x, sx = object->xsize;
	const int bz = object->mapPos.y, sz = object->zsize;
	const int minXSqr = bx, maxXSqr = bx + sx;
	const int minZSqr = bz, maxZSqr = bz + sz;

	for (int zSqr = minZSqr; zSqr < maxZSqr; zSqr++) {
		for (int xSqr = minXSqr; xSqr < maxXSqr; xSqr++) {
			const int idx = xSqr + zSqr * gs->mapx;

			BlockingMapCell& cell = groundBlockingMap[idx];
			cell[objID] = object;
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->mobility == NULL && pathManager) {
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
	}
}


void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object, const unsigned char* yardMap, unsigned char mask)
{
	GML_STDMUTEX_LOCK(block); // AddGroundBlockingObject

	const int objID = GetObjectID(object);

	object->isMarkedOnBlockingMap = true;
	object->mapPos = object->GetMapPos();

	if (object->immobile) {
		object->mapPos.x &= 0xfffffe;
		object->mapPos.y &= 0xfffffe;
	}

	const int bx = object->mapPos.x, sx = object->xsize;
	const int bz = object->mapPos.y, sz = object->zsize;
	const int minXSqr = bx, maxXSqr = bx + sx;
	const int minZSqr = bz, maxZSqr = bz + sz;

	for (int z = 0; minZSqr + z < maxZSqr; z++) {
		for (int x = 0; minXSqr + x < maxXSqr; x++) {
			// unit yardmaps always contain sx=UnitDef::xsize * sz=UnitDef::zsize
			// cells (the unit->mobility footprint can have different dimensions)
			const int off = x + z * sx;
			if (yardMap[off] & mask) {
				const int idx = minXSqr + x + (minZSqr + z) * gs->mapx;

				BlockingMapCell& cell = groundBlockingMap[idx];
				cell[objID] = object;
			}
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->mobility == NULL && pathManager) {
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
	}
}


void CGroundBlockingObjectMap::RemoveGroundBlockingObject(CSolidObject* object)
{
	GML_STDMUTEX_LOCK(block); // RemoveGroundBlockingObject

	const int objID = GetObjectID(object);

	const int bx = object->mapPos.x;
	const int bz = object->mapPos.y;
	const int sx = object->xsize;
	const int sz = object->zsize;

	object->isMarkedOnBlockingMap = false;

	for (int z = bz; z < bz + sz; ++z) {
		for (int x = bx; x < bx + sx; ++x) {
			const int idx = x + z * gs->mapx;

			BlockingMapCell& cell = groundBlockingMap[idx];
			cell.erase(objID);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->mobility == NULL && pathManager) {
		pathManager->TerrainChange(bx, bz, bx + sx, bz + sz);
	}
}


/**
  * Checks if a ground-square is blocked.
  * If it's not blocked (empty), then NULL is returned. Otherwise, a
  * pointer to the top-most / bottom-most blocking object is returned.
  */
CSolidObject* CGroundBlockingObjectMap::GroundBlockedUnsafe(int mapSquare) const {
	GML_STDMUTEX_LOCK(block); // GroundBlockedUnsafe

	const BlockingMapCell& cell = groundBlockingMap[mapSquare];

	if (cell.empty()) {
		return NULL;
	}

	return cell.begin()->second;
}


CSolidObject* CGroundBlockingObjectMap::GroundBlocked(int x, int z) const {
	if (x < 0 || x >= gs->mapx || z < 0 || z >= gs->mapy)
		return NULL;

	return GroundBlockedUnsafe(x + z * gs->mapx);
}


CSolidObject* CGroundBlockingObjectMap::GroundBlocked(const float3& pos) const {
	const int xSqr = int(pos.x / SQUARE_SIZE);
	const int zSqr = int(pos.z / SQUARE_SIZE);
	return GroundBlocked(xSqr, zSqr);
}


bool CGroundBlockingObjectMap::GroundBlocked(int x, int z, CSolidObject* ignoreObj) const
{
	if (x < 0 || x >= gs->mapx || z < 0 || z >= gs->mapy)
		return NULL;

	const int mapSquare = x + z * gs->mapx;

	GML_STDMUTEX_LOCK(block); // GroundBlockedUnsafe

	if (groundBlockingMap[mapSquare].empty()) {
		return false;
	}

	const int objID = GetObjectID(ignoreObj);

	const BlockingMapCell& cell = groundBlockingMap[mapSquare];
	BlockingMapCellIt it = cell.begin();
	if (it != cell.end()) {
		if (it->first != objID) {
			// there are other objects blocking the square
			return true;
		} else {
			// ignoreObj is in the square. Check if there are other objects, too
			return (cell.size() >= 2);
		}
	}

	return false;
}


bool CGroundBlockingObjectMap::GroundBlocked(const float3& pos, CSolidObject* ignoreObj) const
{
	const int xSqr = int(pos.x / SQUARE_SIZE);
	const int zSqr = int(pos.z / SQUARE_SIZE);
	return GroundBlocked(xSqr, zSqr, ignoreObj);
}



/**
  * Opens up a yard in a blocked area.
  * When a factory opens up, for example.
  */
void CGroundBlockingObjectMap::OpenBlockingYard(CSolidObject* yard, const unsigned char* yardMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, yardMap, 2);
}

/**
  * Closes a yard, blocking the area.
  * When a factory closes, for example.
  */
void CGroundBlockingObjectMap::CloseBlockingYard(CSolidObject* yard, const unsigned char* yardMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, yardMap, 1);
}

bool CGroundBlockingObjectMap::CanCloseYard(CSolidObject* yard)
{
	//GML_STDMUTEX_LOCK(block); // CanCloseYard

	for (int z = yard->mapPos.y; z < yard->mapPos.y + yard->zsize; ++z) {
		for (int x = yard->mapPos.x; x < yard->mapPos.x + yard->xsize; ++x) {
			if (GroundBlocked(x, z, yard))
				return false;
		}
	}

	return true;
}
