#ifndef GROUNDBLOCKINGOBJECTMAP_H
#define GROUNDBLOCKINGOBJECTMAP_H

#include "creg/creg.h"
#include "creg/STL_Set.h"
#include "float3.h"

class CSolidObject;

class CGroundBlockingObjectMap
{
	CR_DECLARE(CGroundBlockingObjectMap);

public:
	CGroundBlockingObjectMap(int numSquares);

	void AddGroundBlockingObject(CSolidObject* object);
	void AddGroundBlockingObject(CSolidObject* object, unsigned char* yardMap, unsigned char mask = 255);
	void RemoveGroundBlockingObject(CSolidObject* object);
	void MoveGroundBlockingObject(CSolidObject* object, float3 oldPos);

	void OpenBlockingYard(CSolidObject* yard, unsigned char* yardMap);
	void CloseBlockingYard(CSolidObject* yard, unsigned char* yardMap);
	bool CanCloseYard(CSolidObject* object);

	CSolidObject* GroundBlocked(int mapSquare);
	CSolidObject* GroundBlocked(float3 pos);
	// same as GroundBlocked(), but does not bounds-check mapSquare
	CSolidObject* GroundBlockedUnsafe(int mapSquare);

private:
	typedef std::set<CSolidObject*> BlockingMapCell;
	typedef BlockingMapCell::iterator BlockingMapCellIt;
	typedef std::vector<BlockingMapCell> BlockingMap;
	BlockingMap groundBlockingMap;
};

extern CGroundBlockingObjectMap* groundBlockingObjectMap;

#endif
