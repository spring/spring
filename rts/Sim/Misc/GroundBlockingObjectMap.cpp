/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>

#include "GroundBlockingObjectMap.h"
#include "GlobalConstants.h"
#include "Map/ReadMap.h"
#include "Sim/Path/IPathManager.h"
#include "System/creg/STL_Map.h"
#include "System/Sync/HsiehHash.h"
#include "System/ContainerUtil.h"

CGroundBlockingObjectMap groundBlockingObjectMap;

CR_BIND(CGroundBlockingObjectMap, )
CR_REG_METADATA(CGroundBlockingObjectMap, (
	CR_MEMBER(groundBlockingMap)
))



void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object)
{
	if (object->blockMap != nullptr) {
		// if object has a yardmap, add it to map selectively
		// (checking the specific state of each yardmap cell)
		AddGroundBlockingObject(object, YARDMAP_BLOCKED);
		return;
	}

	object->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_BLOCKING);
	object->SetMapPos(object->GetMapPos());

	const int bx = object->mapPos.x, sx = object->xsize;
	const int bz = object->mapPos.y, sz = object->zsize;
	const int xminSqr = bx, xmaxSqr = bx + sx;
	const int zminSqr = bz, zmaxSqr = bz + sz;

	for (int zSqr = zminSqr; zSqr < zmaxSqr; zSqr++) {
		for (int xSqr = xminSqr; xSqr < xmaxSqr; xSqr++) {
			spring::VectorInsertUnique(GetCellUnsafe(xSqr + zSqr * mapDims.mapx), object, true);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef != nullptr || pathManager == nullptr)
		return;

	pathManager->TerrainChange(xminSqr, zminSqr, xmaxSqr, zmaxSqr, TERRAINCHANGE_OBJECT_INSERTED);
}

void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object, const YardMapStatus& mask)
{
	object->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_BLOCKING);
	object->SetMapPos(object->GetMapPos());

	const int bx = object->mapPos.x, sx = object->xsize;
	const int bz = object->mapPos.y, sz = object->zsize;
	const int xminSqr = bx, xmaxSqr = bx + sx;
	const int zminSqr = bz, zmaxSqr = bz + sz;

	for (int z = zminSqr; z < zmaxSqr; z++) {
		for (int x = xminSqr; x < xmaxSqr; x++) {
			// unit yardmaps always contain sx=UnitDef::xsize * sz=UnitDef::zsize
			// cells (the unit->moveDef footprint can have different dimensions)
			const float3 testPos = float3(x, 0.0f, z) * SQUARE_SIZE;

			if ((object->GetGroundBlockingMaskAtPos(testPos) & mask) == 0)
				continue;

			spring::VectorInsertUnique(GetCellUnsafe(x + (z) * mapDims.mapx), object, true);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef != nullptr || pathManager == nullptr)
		return;

	pathManager->TerrainChange(xminSqr, zminSqr, xmaxSqr, zmaxSqr, TERRAINCHANGE_OBJECT_INSERTED_YM);
}


void CGroundBlockingObjectMap::RemoveGroundBlockingObject(CSolidObject* object)
{
	const int bx = object->mapPos.x;
	const int bz = object->mapPos.y;
	const int sx = object->xsize;
	const int sz = object->zsize;

	object->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_BLOCKING);

	for (int z = bz; z < bz + sz; ++z) {
		for (int x = bx; x < bx + sx; ++x) {
			spring::VectorErase(GetCellUnsafe(z * mapDims.mapx + x), object);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef != nullptr || pathManager == nullptr)
		return;

	pathManager->TerrainChange(bx, bz, bx + sx, bz + sz, TERRAINCHANGE_OBJECT_DELETED);
}



CSolidObject* CGroundBlockingObjectMap::GroundBlocked(int x, int z) const {
	if (static_cast<unsigned int>(x) >= mapDims.mapx || static_cast<unsigned int>(z) >= mapDims.mapy)
		return nullptr;

	return (GroundBlockedUnsafe(x + z * mapDims.mapx));
}


CSolidObject* CGroundBlockingObjectMap::GroundBlocked(const float3& pos) const {
	const int xSqr = int(pos.x / SQUARE_SIZE);
	const int zSqr = int(pos.z / SQUARE_SIZE);
	return (GroundBlocked(xSqr, zSqr));
}


bool CGroundBlockingObjectMap::GroundBlocked(int x, int z, const CSolidObject* ignoreObj) const
{
	if (static_cast<unsigned int>(x) >= mapDims.mapx || static_cast<unsigned int>(z) >= mapDims.mapy)
		return false;

	const BlockingMapCell& cell = GetCellUnsafeConst(z * mapDims.mapx + x);

	if (cell.empty())
		return false;

	// check if the first object in <cell> is NOT the ignoree
	// if so the ground is definitely blocked at this location
	if (*(cell.cbegin()) != ignoreObj)
		return true;

	// otherwise the ground is considered blocked only if there
	// is at least one other object in <cell> together with the
	// ignoree
	return (cell.size() >= 2);
}


bool CGroundBlockingObjectMap::GroundBlocked(const float3& pos, const CSolidObject* ignoreObj) const
{
	const int xSqr = unsigned(pos.x) / SQUARE_SIZE;
	const int zSqr = unsigned(pos.z) / SQUARE_SIZE;
	return (GroundBlocked(xSqr, zSqr, ignoreObj));
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
			if ((yardUnit->GetGroundBlockingMaskAtPos(float3(x * SQUARE_SIZE, 0.0f, z * SQUARE_SIZE)) & mask) == 0)
				continue;

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


unsigned int CGroundBlockingObjectMap::CalcChecksum() const
{
	unsigned int checksum = 666;

	for (unsigned int i = 0; i < groundBlockingMap.size(); ++i) {
		if (!groundBlockingMap[i].empty()) {
			checksum = HsiehHash(&i, sizeof(i), checksum);
		}
	}

	return checksum;
}

