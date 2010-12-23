/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDBLOCKINGOBJECTMAP_H
#define GROUNDBLOCKINGOBJECTMAP_H

#include <map>

#include "creg/creg_cond.h"
#include "float3.h"

class CSolidObject;
typedef std::map<int, CSolidObject*> BlockingMapCell;
typedef BlockingMapCell::const_iterator BlockingMapCellIt;
typedef std::vector<BlockingMapCell> BlockingMap;

class CGroundBlockingObjectMap
{
	CR_DECLARE(CGroundBlockingObjectMap);

public:
	CGroundBlockingObjectMap(int numSquares) { groundBlockingMap.resize(numSquares); }

	void AddGroundBlockingObject(CSolidObject* object);
	void AddGroundBlockingObject(CSolidObject* object, const unsigned char* yardMap, unsigned char mask);
	void RemoveGroundBlockingObject(CSolidObject* object);

	void OpenBlockingYard(CSolidObject* yard, const unsigned char* yardMap);
	void CloseBlockingYard(CSolidObject* yard, const unsigned char* yardMap);
	bool CanCloseYard(CSolidObject* object);

	// these retrieve either the top-most or the bottom-most
	// object in a given cell, or NULL if the cell is empty
	CSolidObject* GroundBlocked(int mapSquare, bool topMost = true);
	CSolidObject* GroundBlocked(const float3& pos, bool topMost = true);
	// same as GroundBlocked(), but does not bounds-check mapSquare
	CSolidObject* GroundBlockedUnsafe(int mapSquare, bool topMost = true);

	const BlockingMapCell& GetCell(int mapSquare) const { return groundBlockingMap[mapSquare]; }

private:
	BlockingMap groundBlockingMap;
};

extern CGroundBlockingObjectMap* groundBlockingObjectMap;

#endif
