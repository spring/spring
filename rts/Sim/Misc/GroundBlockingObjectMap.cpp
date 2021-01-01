/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "GroundBlockingObjectMap.h"
#include "GlobalConstants.h"
#include "Map/ReadMap.h"
#include "Sim/Path/IPathManager.h"
#include "System/ContainerUtil.h"
#include "System/Sync/HsiehHash.h"

CGroundBlockingObjectMap groundBlockingObjectMap;

CR_BIND_TEMPLATE(CGroundBlockingObjectMap::ArrCell, )
CR_REG_METADATA_TEMPLATE(CGroundBlockingObjectMap::ArrCell, (
	CR_MEMBER(arr),
	CR_MEMBER(numObjs),
	CR_MEMBER(vecIndx)
))

CR_BIND(CGroundBlockingObjectMap, )
CR_REG_METADATA(CGroundBlockingObjectMap, (
	CR_MEMBER(arrCells),
	CR_MEMBER(vecCells),
	CR_MEMBER(vecIndcs)
))



void CGroundBlockingObjectMap::AddGroundBlockingObject(CSolidObject* object)
{
	if (object->GetBlockMap() != nullptr) {
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
			CellInsertUnique(zSqr * mapDims.mapx + xSqr, object);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef != nullptr)
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
			if ((object->GetGroundBlockingMaskAtPos({x * SQUARE_SIZE * 1.0f, 0.0f, z * SQUARE_SIZE * 1.0f}) & mask) == 0)
				continue;

			CellInsertUnique(z * mapDims.mapx + x, object);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef != nullptr)
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
			CellErase(z * mapDims.mapx + x, object);
		}
	}

	// FIXME: needs dependency injection (observer pattern?)
	if (object->moveDef != nullptr)
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
	if (cell[0] != ignoreObj)
		return true;

	// otherwise the ground is considered blocked only if there
	// is at least one other object in <cell> together with the
	// ignoree
	return (cell.size() >= 2);
}


bool CGroundBlockingObjectMap::GroundBlocked(const float3& pos, const CSolidObject* ignoreObj) const
{
	const int xSqr = static_cast<unsigned>(pos.x / SQUARE_SIZE);
	const int zSqr = static_cast<unsigned>(pos.z / SQUARE_SIZE);
	return (GroundBlocked(xSqr, zSqr, ignoreObj));
}


CGroundBlockingObjectMap::BlockingMapCell CGroundBlockingObjectMap::GetCellUnsafeConst(const float3& pos) const
{
	const int xSqr = static_cast<unsigned>(pos.x / SQUARE_SIZE);
	const int zSqr = static_cast<unsigned>(pos.z / SQUARE_SIZE);
	return (GetCellUnsafeConst(zSqr * mapDims.mapx + xSqr));
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


bool CGroundBlockingObjectMap::CheckYard(const CSolidObject* yardUnit, const YardMapStatus& mask) const
{
	const int2 mins = yardUnit->mapPos;
	const int2 maxs = mins + int2(yardUnit->xsize, yardUnit->zsize);

	for (int z = mins.y; z < maxs.y; ++z) {
		for (int x = mins.x; x < maxs.x; ++x) {
			if ((yardUnit->GetGroundBlockingMaskAtPos(float3(x * SQUARE_SIZE, 0.0f, z * SQUARE_SIZE)) & mask) == 0)
				continue;

			if (GroundBlocked(x, z, yardUnit))
				return false;
		}
	}

	return true;
}


unsigned int CGroundBlockingObjectMap::CalcChecksum() const
{
	unsigned int checksum = 666;

	for (unsigned int i = 0; i < arrCells.size(); ++i) {
		if (!arrCells[i].Empty())
			checksum = HsiehHash(&i, sizeof(i), checksum);
	}

	return checksum;
}



bool CGroundBlockingObjectMap::CellInsertUnique(unsigned int sqr, CSolidObject* o) {
	ArrCell& ac = GetArrCell(sqr);
	VecCell* vc = nullptr;

	if (ac.Contains(o))
		return false;
	if (ac.Insert(o))
		return true;

	// array-cell is full, spill over
	if ((vc = &GetVecCell(sqr)) == &vecCells[0]) {
		if (vecIndcs.empty()) {
			assert(vecCells.size() > 0);
			ac.SetVecIndx(vecCells.size());
			vc = &spring::VectorEmplaceBack(vecCells);
		} else {
			ac.SetVecIndx(spring::VectorBackPop(vecIndcs));
			vc = &vecCells[ac.GetVecIndx()];
		}
	}

	return (spring::VectorInsertUnique(*vc, o, true));
}

bool CGroundBlockingObjectMap::CellErase(unsigned int sqr, CSolidObject* o) {
	ArrCell& ac = GetArrCell(sqr);
	VecCell* vc = nullptr;

	if (ac.Erase(o)) {
		if (ac.GetVecIndx() == 0)
			return true;

		// never allow a hole between array and vector parts
		assert(!vecCells[ac.GetVecIndx()].empty());
		ac.Insert(spring::VectorBackPop(*(vc = &GetVecCell(sqr))));

		goto CommonExit;
	}

	// failed to erase, but array-cell is not filled to capacity
	// this means vc must be empty and can not contain the object
	if (!ac.Full())
		return false;

	// otherwise object must be in vc if(f) this cell contains it
	// note that vc can still point to the dummy element if ac is
	// full but never overflowed, which is fine since VectorErase
	// will simply return false
	if (!spring::VectorErase(*(vc = &GetVecCell(sqr)), o))
		return false;

CommonExit:

	if (vc->empty()) {
		assert(ac.GetVecIndx() != 0);
		vecIndcs.push_back(ac.GetVecIndx());
		ac.SetVecIndx(0);
	}

	return true;
}

