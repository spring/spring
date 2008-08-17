#include "StdAfx.h"
#include "GroundBlockingObjectMap.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Path/PathManager.h"

CGroundBlockingObjectMap* groundBlockingObjectMap;

CR_BIND(CGroundBlockingObjectMap, (1))
CR_REG_METADATA(CGroundBlockingObjectMap, (
	CR_MEMBER(groundBlockingMap)
));



CGroundBlockingObjectMap::CGroundBlockingObjectMap(int numSquares)
{
	groundBlockingMap.resize(numSquares);
}

void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object)
{
	object->isMarkedOnBlockingMap = true;
	object->mapPos = object->GetMapPos();

	if (object->immobile) {
		object->mapPos.x &= 0xfffffe;
		object->mapPos.y &= 0xfffffe;
	}

	int bx = object->mapPos.x;
	int bz = object->mapPos.y;
	int minXSqr = bx;
	int minZSqr = bz;
	int maxXSqr = bx + object->xsize;
	int maxZSqr = bz + object->ysize;

	for (int zSqr = minZSqr; zSqr < maxZSqr; zSqr++) {
		for (int xSqr = minXSqr; xSqr < maxXSqr; xSqr++) {
			const int idx = xSqr + zSqr * gs->mapx;
			BlockingMapCell& cell = groundBlockingMap[idx];

			cell.insert(object);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (!object->mobility && pathManager) {
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
	}
}


void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object, unsigned char* yardMap, unsigned char mask)
{
	object->isMarkedOnBlockingMap = true;
	object->mapPos = object->GetMapPos();

	if (object->immobile) {
		object->mapPos.x &= 0xfffffe;
		object->mapPos.y &= 0xfffffe;
	}

	int bx = object->mapPos.x;
	int bz = object->mapPos.y;
	int minXSqr = bx;
	int minZSqr = bz;
	int maxXSqr = bx + object->xsize;
	int maxZSqr = bz + object->ysize;

	for (int z = 0; minZSqr + z < maxZSqr; z++) {
		for (int x = 0; minXSqr + x < maxXSqr; x++) {
			const int idx = minXSqr + x + (minZSqr + z) * gs->mapx;
			const int off = x + z * object->xsize;
			BlockingMapCell& cell = groundBlockingMap[idx];

			if ((yardMap[off] & mask)) {
				cell.insert(object);
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
	object->isMarkedOnBlockingMap = false;
	int bx = object->mapPos.x;
	int bz = object->mapPos.y;
	int sx = object->xsize;
	int sz = object->ysize;

	for (int z = bz; z < bz + sz; ++z) {
		for (int x = bx; x < bx + sx; ++x) {
			int idx = x + z * gs->mapx;
			BlockingMapCell& cell = groundBlockingMap[idx];
			BlockingMapCellIt it = cell.find(object);

			if (it != cell.end()) {
				cell.erase(it);
			}
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (!object->mobility) {
		pathManager->TerrainChange(bx, bz, bx + sx, bz + sz);
	}
}


/**
Moves a ground blocking object from old position to the current on map.
*/
void CGroundBlockingObjectMap::MoveGroundBlockingObject(CSolidObject* object, float3 oldPos) {
	RemoveGroundBlockingObject(object);
	AddGroundBlockingObject(object);
}



/**
Checks if a ground-square is blocked.
If it's not blocked, then NULL is returned.
If it's blocked, then a pointer to the blocking object is returned.
*/
CSolidObject* CGroundBlockingObjectMap::GroundBlockedUnsafe(int mapSquare, bool topMost) {
	if (groundBlockingMap[mapSquare].empty()) {
		return 0x0;
	}

	BlockingMapCellIt it = groundBlockingMap[mapSquare].begin();
	CSolidObject* p = *it;
	CSolidObject* q = *it;
	it++;

	// todo: accessors to get the block count, etc
	for (; it != groundBlockingMap[mapSquare].end(); it++) {
		if ((*it)->pos.y > p->pos.y) { p = *it; }
		if ((*it)->pos.y < q->pos.y) { q = *it; }
	}

	// return the top-most blocking object
	// (rather than objects.begin(), since
	// we cannot rely on pointer order)
	return ((topMost)? p: q);
}

CSolidObject* CGroundBlockingObjectMap::GroundBlocked(int mapSquare, bool topMost) {
	if (mapSquare < 0 || mapSquare >= gs->mapSquares) {
		return NULL;
	}

	return GroundBlockedUnsafe(mapSquare);
}

CSolidObject* CGroundBlockingObjectMap::GroundBlocked(float3 pos, bool topMost) {
	int xSqr = int(pos.x / SQUARE_SIZE) % gs->mapx;
	int zSqr = int(pos.z / SQUARE_SIZE) / gs->mapx;
	return GroundBlocked(xSqr + zSqr * gs->mapx, topMost);
}


/**
Opens up a yard in a blocked area.
When a factory opens up, for example.
*/
void CGroundBlockingObjectMap::OpenBlockingYard(CSolidObject* yard, unsigned char* yardMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, yardMap, 2);
}

/**
Closes a yard, blocking the area.
When a factory closes, for example.
*/
void CGroundBlockingObjectMap::CloseBlockingYard(CSolidObject* yard, unsigned char* yardMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, yardMap, 1);
}

bool CGroundBlockingObjectMap::CanCloseYard(CSolidObject* yard)
{
	for (int z = yard->mapPos.y; z < yard->mapPos.y + yard->ysize; ++z) {
		for (int x = yard->mapPos.x; x < yard->mapPos.x + yard->xsize; ++x) {
			const int idx = z * gs->mapx + x;
			BlockingMapCell& cell = groundBlockingMap[idx];
			BlockingMapCellIt it = cell.find(yard);

			if (it != cell.end() && cell.size() >= 2) {
				// something else besides us present
				// at this position, cannot close yet
				return false;
			}
		}
	}
	return true;
}
