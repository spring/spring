#ifndef GROUNDBLOCKINGOBJECTMAP_H
#define GROUNDBLOCKINGOBJECTMAP_H

#include <vector>
#include "creg/creg.h"
#include "float3.h"

class CSolidObject;

class CGroundBlockingObjectMap
{
	CR_DECLARE(CGroundBlockingObjectMap);

public:
	CGroundBlockingObjectMap(int numSquares);
	void AddGroundBlockingObject(CSolidObject *object);
	void AddGroundBlockingObject(CSolidObject *object, unsigned char *blockingMap, unsigned char mask = 255);
	void RemoveGroundBlockingObject(CSolidObject *object);
	void MoveGroundBlockingObject(CSolidObject *object, float3 oldPos);
	void OpenBlockingYard(CSolidObject *yard, unsigned char *blockingMap);
	void CloseBlockingYard(CSolidObject *yard, unsigned char *blockingMap);
	bool CGroundBlockingObjectMap::CanCloseYard(CSolidObject* object);
	CSolidObject* GroundBlocked(float3 pos);
	CSolidObject* GroundBlocked(int mapSquare);
	///simple version of GroundBlocked without error checking
	CSolidObject* GroundBlockedUnsafe(int mapSquare){return groundBlockingObjectMap[mapSquare];}

private:
	std::vector<CSolidObject*> groundBlockingObjectMap;
};

extern CGroundBlockingObjectMap* groundBlockingObjectMap;

#endif
