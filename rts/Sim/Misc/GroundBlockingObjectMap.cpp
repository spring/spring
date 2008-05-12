#include "GroundBlockingObjectMap.h"

#include "Sim/Objects/SolidObject.h"
#include "Sim/Path/PathManager.h"


CR_BIND(CGroundBlockingObjectMap, (1))
CR_REG_METADATA(CGroundBlockingObjectMap, (
				CR_MEMBER(groundBlockingObjectMap)
				));


CGroundBlockingObjectMap* groundBlockingObjectMap;


CGroundBlockingObjectMap::CGroundBlockingObjectMap(int numSquares)
{
	groundBlockingObjectMap.resize(numSquares, NULL);
}


void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject *object)
{
	object->isMarkedOnBlockingMap=true;
	object->mapPos=object->GetMapPos();
	if(object->immobile){
		object->mapPos.x&=0xfffffe;
		object->mapPos.y&=0xfffffe;
	}
	int bx=object->mapPos.x;
	int bz=object->mapPos.y;

	int minXSqr = bx;
	int minZSqr = bz;
	int maxXSqr = bx + object->xsize;
	int maxZSqr = bz + object->ysize;

	for(int zSqr = minZSqr; zSqr < maxZSqr; zSqr++)
		for(int xSqr = minXSqr; xSqr < maxXSqr; xSqr++)
			if(!groundBlockingObjectMap[xSqr + zSqr*gs->mapx])
				groundBlockingObjectMap[xSqr + zSqr*gs->mapx] = object;

	// FIXME: needs dependency injection (observer pattern?)
	if(!object->mobility && pathManager)
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
}


void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject *object, unsigned char *yardMap, unsigned char mask)
{
	object->isMarkedOnBlockingMap=true;
	object->mapPos=object->GetMapPos();
	if(object->immobile){
		object->mapPos.x&=0xfffffe;
		object->mapPos.y&=0xfffffe;
	}
	int bx=object->mapPos.x;
	int bz=object->mapPos.y;

	int minXSqr = bx;
	int minZSqr = bz;
	int maxXSqr = bx + object->xsize;
	int maxZSqr = bz + object->ysize;

	for(int z = 0; minZSqr + z < maxZSqr; z++) {
		for(int x = 0; minXSqr + x < maxXSqr; x++) {
			if(!groundBlockingObjectMap[minXSqr + x + (minZSqr + z)*gs->mapx]){
				if(yardMap[x + z*object->xsize] & mask) {
					groundBlockingObjectMap[minXSqr + x + (minZSqr + z)*gs->mapx] = object;
				}
			}
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if(!object->mobility && pathManager)
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
}


void CGroundBlockingObjectMap::RemoveGroundBlockingObject(CSolidObject *object)
{
	object->isMarkedOnBlockingMap=false;
	int bx=object->mapPos.x;
	int bz=object->mapPos.y;
	int sx=object->xsize;
	int sz=object->ysize;
	for(int z = bz; z < bz+sz; ++z)
		for(int x = bx; x < bx+sx; ++x)
			if(groundBlockingObjectMap[x + z*gs->mapx]==object)
				groundBlockingObjectMap[x + z*gs->mapx] = 0;

	// FIXME: needs dependency injection (observer pattern?)
	if(!object->mobility)
		pathManager->TerrainChange(bx, bz, bx+sx, bz+sz);
}


/**
Moves a ground blocking object from old position to the current on map.
*/
void CGroundBlockingObjectMap::MoveGroundBlockingObject(CSolidObject *object, float3 oldPos) {
	RemoveGroundBlockingObject(object);
	AddGroundBlockingObject(object);
}


/**
Checks if a ground-square is blocked.
If it's not blocked, then NULL is returned.
If it's blocked, then a pointer to the blocking object is returned.
*/
CSolidObject* CGroundBlockingObjectMap::GroundBlocked(int mapSquare) {
	if(mapSquare < 0 || mapSquare >= gs->mapSquares)
		return NULL;
	return groundBlockingObjectMap[mapSquare];
}


/**
Checks if a ground-square is blocked.
If it's not blocked, then NULL is returned.
If it's blocked, then a pointer to the blocking object is returned.
*/
CSolidObject* CGroundBlockingObjectMap::GroundBlocked(float3 pos) {
	int xSqr = int(pos.x / SQUARE_SIZE) % gs->mapx;
	int zSqr = int(pos.z / SQUARE_SIZE) / gs->mapx;
	return GroundBlocked(xSqr + zSqr*gs->mapx);
}


/**
Opens up a yard in a blocked area.
When a factory opens up, for example.
*/
void CGroundBlockingObjectMap::OpenBlockingYard(CSolidObject *yard, unsigned char *blockingMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, blockingMap, 2);
}


/**
Closes a yard, blocking the area.
When a factory closes, for example.
*/
void CGroundBlockingObjectMap::CloseBlockingYard(CSolidObject *yard, unsigned char *blockingMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, blockingMap, 1);
}


bool CGroundBlockingObjectMap::CanCloseYard(CSolidObject* yard)
{
	for(int z = yard->mapPos.y; z < yard->mapPos.y + yard->ysize; ++z) {
		for(int x = yard->mapPos.x; x < yard->mapPos.x + yard->xsize; ++x) {
			CSolidObject* c = groundBlockingObjectMap[z * gs->mapx + x];
			if (c != NULL && c != yard)
				return false;
		}
	}
	return true;
}
