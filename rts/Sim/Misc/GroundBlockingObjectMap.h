/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDBLOCKINGOBJECTMAP_H
#define GROUNDBLOCKINGOBJECTMAP_H

#include <vector>

#include "Sim/Objects/SolidObject.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"


typedef std::vector<CSolidObject*> BlockingMapCell;
typedef std::vector<BlockingMapCell> BlockingMap;


class CGroundBlockingObjectMap
{
	CR_DECLARE_STRUCT(CGroundBlockingObjectMap)

public:
	CGroundBlockingObjectMap(int numSquares) {
		groundBlockingMap.clear();
		groundBlockingMap.resize(numSquares);
	}

	unsigned int CalcChecksum() const;

	void AddGroundBlockingObject(CSolidObject* object);
	void AddGroundBlockingObject(CSolidObject* object, const YardMapStatus& mask);
	void RemoveGroundBlockingObject(CSolidObject* object);

	void OpenBlockingYard(CSolidObject* object);
	void CloseBlockingYard(CSolidObject* object);
	bool CanOpenYard(CSolidObject* object) const;
	bool CanCloseYard(CSolidObject* object) const;


	// these retrieve either the first object in
	// a given cell, or NULL if the cell is empty
	CSolidObject* GroundBlocked(int x, int z) const;
	CSolidObject* GroundBlocked(const float3& pos) const;

	// same as GroundBlocked(), but does not bounds-check mapSquare
	CSolidObject* GroundBlockedUnsafe(unsigned int mapSquare) const {
		const BlockingMapCell& cell = GetCellUnsafeConst(mapSquare);

		if (cell.empty())
			return nullptr;

		return (*cell.begin());
	}


	bool GroundBlocked(int x, int z, const CSolidObject* ignoreObj) const;
	bool GroundBlocked(const float3& pos, const CSolidObject* ignoreObj) const;

	bool ObjectInCell(unsigned int mapSquare, const CSolidObject* obj) const {
		if (mapSquare >= groundBlockingMap.size())
			return false;

		const BlockingMapCell& cell = GetCellUnsafeConst(mapSquare);
		const auto it = std::find(cell.cbegin(), cell.cend(), obj);
		return (it != cell.cend());
	}


	const BlockingMapCell& GetCellUnsafeConst(unsigned int mapSquare) const {
		assert(mapSquare < groundBlockingMap.size());
		return groundBlockingMap[mapSquare];
	}

	BlockingMapCell& GetCellUnsafe(unsigned int mapSquare) {
		return (const_cast<BlockingMapCell&>(GetCellUnsafeConst(mapSquare)));
	}

private:
	bool CheckYard(CSolidObject* yardUnit, const YardMapStatus& mask) const;

private:
	BlockingMap groundBlockingMap;
};

extern CGroundBlockingObjectMap* groundBlockingObjectMap;

#endif
