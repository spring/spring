#include "StdAfx.h"
#include "mmgr.h"

#include "SolidObject.h"
#include "Map/ReadMap.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "myMath.h"

CR_BIND_DERIVED(CSolidObject, CWorldObject, );
CR_REG_METADATA(CSolidObject,
(
	CR_MEMBER(mass),
	CR_MEMBER(blocking),
	CR_MEMBER(floatOnWater),
	CR_MEMBER(isUnderWater),
	CR_MEMBER(immobile),
	CR_MEMBER(blockHeightChanges),
	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
	CR_MEMBER(height),
	CR_MEMBER(heading),
	CR_ENUM_MEMBER(physicalState),
	CR_MEMBER(midPos),
	CR_MEMBER(isMoving),
	CR_MEMBER(residualImpulse),
	CR_MEMBER(mobility),
	// can't get creg work on templates
	CR_MEMBER(mapPos.x),
	CR_MEMBER(mapPos.y),
	CR_MEMBER(buildFacing),
	CR_MEMBER(isMarkedOnBlockingMap),
	CR_MEMBER(speed),
	CR_RESERVED(16))
);


CSolidObject::CSolidObject():
	mass(100000),
	blocking(false),
	floatOnWater(false),
	isUnderWater(false),
	immobile(false),
	blockHeightChanges(false),
	xsize(1),
	zsize(1),
	height(1),
	heading(0),
	physicalState(Ghost),
	midPos(pos),
	isMoving(false),
	residualImpulse(0, 0, 0),
	mobility(0),
	yardMap(0),
	buildFacing(0),
	isMarkedOnBlockingMap(false),
	speed(0, 0, 0)
{
	mapPos = GetMapPos();
}

CSolidObject::~CSolidObject() {
	if (mobility) {
		delete mobility;
	}

	mobility = 0x0;
	blocking = false;
}



/*
 * removes this object from the GroundBlockingMap
 * if it is currently marked on it, does nothing
 * otherwise
 */
void CSolidObject::UnBlock() {
	if (isMarkedOnBlockingMap) {
		groundBlockingObjectMap->RemoveGroundBlockingObject(this);
		// isMarkedOnBlockingMap is now false
	}
}

/*
 * adds this object to the GroundBlockingMap
 * if and only if its collidable property is
 * set (blocking), else does nothing (except
 * call UnBlock())
 */
void CSolidObject::Block() {
	UnBlock();

	if (!blocking) {
		return;
	}
	if (physicalState == Ghost || physicalState == Flying) {
		return;
	}

	// use the object's yardmap if available
	if (yardMap) {
		groundBlockingObjectMap->AddGroundBlockingObject(this, yardMap);
	} else {
		groundBlockingObjectMap->AddGroundBlockingObject(this);
	}

	// isMarkedOnBlockingMap is now true
}



bool CSolidObject::AddBuildPower(float amount, CUnit* builder) {
	return false;
}

int2 CSolidObject::GetMapPos()
{
	return GetMapPos(pos);
}

int2 CSolidObject::GetMapPos(const float3 &position)
{
	int2 p;
	p.x = (int(position.x + SQUARE_SIZE / 2) / SQUARE_SIZE) - xsize / 2;
	p.y = (int(position.z + SQUARE_SIZE / 2) / SQUARE_SIZE) - zsize / 2;

	if (p.x < 0)
		p.x = 0;
	if (p.x > gs->mapx - xsize)
		p.x = gs->mapx - xsize;

	if (p.y < 0)
		p.y = 0;
	if (p.y > gs->mapy - zsize)
		p.y = gs->mapy - zsize;

	return p;
}