#include "StdAfx.h"
#include <assert.h>
#include "mmgr.h"

#include "GroundBlockingObjectMap.h"

#include "GlobalSynced.h"
#include "GlobalConstants.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Path/PathManager.h"
#include "creg/STL_Map.h"

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
	const int objID = GetObjectID(object);

	object->isMarkedOnBlockingMap = true;
	object->mapPos = object->GetMapPos();

	if (object->immobile) {
		object->mapPos.x &= 0xfffffe;
		object->mapPos.y &= 0xfffffe;
	}

	const int bx = object->mapPos.x;
	const int bz = object->mapPos.y;
	const int minXSqr = bx;
	const int minZSqr = bz;
	const int maxXSqr = bx + object->xsize;
	const int maxZSqr = bz + object->zsize;

	for (int zSqr = minZSqr; zSqr < maxZSqr; zSqr++) {
		for (int xSqr = minXSqr; xSqr < maxXSqr; xSqr++) {
			const int idx = xSqr + zSqr * gs->mapx;
			BlockingMapCell& cell = groundBlockingMap[idx];
			BlockingMapCellIt it = cell.find(objID);

			if (it == cell.end()) {
				cell[objID] = object;
			}
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (!object->mobility && pathManager) {
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
	}
}


void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object, unsigned char* yardMap, unsigned char mask)
{
	const int objID = GetObjectID(object);

	object->isMarkedOnBlockingMap = true;
	object->mapPos = object->GetMapPos();

	if (object->immobile) {
		object->mapPos.x &= 0xfffffe;
		object->mapPos.y &= 0xfffffe;
	}

	const int bx = object->mapPos.x;
	const int bz = object->mapPos.y;
	const int minXSqr = bx;
	const int minZSqr = bz;
	const int maxXSqr = bx + object->xsize;
	const int maxZSqr = bz + object->zsize;

	for (int z = 0; minZSqr + z < maxZSqr; z++) {
		for (int x = 0; minXSqr + x < maxXSqr; x++) {
			const int idx = minXSqr + x + (minZSqr + z) * gs->mapx;
			const int off = x + z * object->xsize;
			BlockingMapCell& cell = groundBlockingMap[idx];
			BlockingMapCellIt it = cell.find(objID);

			if ((it == cell.end()) && (yardMap[off] & mask)) {
				cell[objID] = object;
			}
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (!object->mobility && pathManager) {
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
	}
}


void CGroundBlockingObjectMap::RemoveGroundBlockingObject(CSolidObject* object)
{
	const int objID = GetObjectID(object);

	object->isMarkedOnBlockingMap = false;
	const int bx = object->mapPos.x;
	const int bz = object->mapPos.y;
	const int sx = object->xsize;
	const int sz = object->zsize;

	for (int z = bz; z < bz + sz; ++z) {
		for (int x = bx; x < bx + sx; ++x) {
			int idx = x + z * gs->mapx;
			BlockingMapCell& cell = groundBlockingMap[idx];
			BlockingMapCellIt it = cell.find(objID);

			if (it != cell.end()) {
				cell.erase(objID);
			}
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (!object->mobility) {
		pathManager->TerrainChange(bx, bz, bx + sx, bz + sz);
	}
}


/**
  * Moves a ground blocking object from old position to the current on map.
  * No longer used, functionality is handled by CSolidObject::{Un}Block()
  */
/*
void CGroundBlockingObjectMap::MoveGroundBlockingObject(CSolidObject* object, float3 oldPos) {
	RemoveGroundBlockingObject(object);
	AddGroundBlockingObject(object);
}
*/



/**
  * Checks if a ground-square is blocked.
  * If it's not blocked (empty), then NULL is returned. Otherwise, a
  * pointer to the top-most / bottom-most blocking object is returned.
  */
CSolidObject* CGroundBlockingObjectMap::GroundBlockedUnsafe(int mapSquare, bool topMost) {
	if (groundBlockingMap[mapSquare].empty()) {
		return NULL;
	}

	const BlockingMapCell& cell = groundBlockingMap[mapSquare];
	BlockingMapCellIt it = cell.begin();
	CSolidObject* p = it->second;
	CSolidObject* q = it->second;
	it++;

	for (; it != cell.end(); it++) {
		CSolidObject* obj = it->second;
		if (obj->pos.y > p->pos.y) { p = obj; }
		if (obj->pos.y < q->pos.y) { q = obj; }
	}

	return ((topMost)? p: q);
}

CSolidObject* CGroundBlockingObjectMap::GroundBlocked(int mapSquare, bool topMost) {
	if (mapSquare < 0 || mapSquare >= gs->mapSquares) {
		return NULL;
	}

	return GroundBlockedUnsafe(mapSquare);
}

CSolidObject* CGroundBlockingObjectMap::GroundBlocked(const float3& pos, bool topMost) {
	if (!pos.IsInBounds()) {
		return NULL;
	}

	const int xSqr = int(pos.x / SQUARE_SIZE);
	const int zSqr = int(pos.z / SQUARE_SIZE);
	return GroundBlocked(xSqr + zSqr * gs->mapx, topMost);
}


/**
  * Opens up a yard in a blocked area.
  * When a factory opens up, for example.
  */
void CGroundBlockingObjectMap::OpenBlockingYard(CSolidObject* yard, unsigned char* yardMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, yardMap, 2);
}

/**
  * Closes a yard, blocking the area.
  * When a factory closes, for example.
  */
void CGroundBlockingObjectMap::CloseBlockingYard(CSolidObject* yard, unsigned char* yardMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, yardMap, 1);
}

bool CGroundBlockingObjectMap::CanCloseYard(CSolidObject* yard)
{
	const int objID = GetObjectID(yard);

	for (int z = yard->mapPos.y; z < yard->mapPos.y + yard->zsize; ++z) {
		for (int x = yard->mapPos.x; x < yard->mapPos.x + yard->xsize; ++x) {
			const int idx = z * gs->mapx + x;
			BlockingMapCell& cell = groundBlockingMap[idx];
			BlockingMapCellIt it = cell.find(objID);

			if (it == cell.end()) {
				// we are non-blocking in this part of
				// our yardmap footprint, but something
				// might be inside us
				if (cell.size() >= 1) {
					return false;
				}
			} else {
				// this part of our yardmap is blocking, we
				// can't close if something else present on
				// it besides us
				if (cell.size() >= 2) {
					return false;
				}
			}
		}
	}

	return true;
}
