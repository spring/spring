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
	CR_MEMBER(ysize),
	CR_MEMBER(height),
	CR_MEMBER(heading),
	CR_ENUM_MEMBER(physicalState),
	CR_MEMBER(midPos),
	CR_MEMBER(isMoving),
	CR_MEMBER(residualImpulse),
	CR_MEMBER(mobility),
	CR_MEMBER(mapPos),
	CR_MEMBER(buildFacing),
	CR_MEMBER(isMarkedOnBlockingMap),
	CR_MEMBER(speed),
	CR_RESERVED(16))
);


CSolidObject::CSolidObject():
	mass(100000),
	blocking(false),
	blockHeightChanges(false),
	floatOnWater(false),
	xsize(1),
	ysize(1),
	height(1),

	physicalState(Ghost),
	midPos(pos),

	immobile(false),
	isMoving(false),
	heading(0),
	buildFacing(0),
	residualImpulse(0, 0, 0),

	mobility(0),
	yardMap(0),
	isMarkedOnBlockingMap(false),

	speed(0, 0, 0),
	isUnderWater(false)
{
	mapPos = GetMapPos();
}

CSolidObject::~CSolidObject() {
	if (mobility) {
		delete mobility;
		mobility = 0x0;
	}
}



/* Removes this object from the GroundBlockingMap. */
void CSolidObject::UnBlock() {
	if (isMarkedOnBlockingMap) {
		groundBlockingObjectMap->RemoveGroundBlockingObject(this);
	}
}

/* Adds this object to the GroundBlockingMap. */
void CSolidObject::Block() {
	UnBlock();

	if (blocking && (physicalState == OnGround ||
	                 physicalState == Floating ||
	                 physicalState == Hovering ||
	                 physicalState == Submarine)) {
		// use the object's yardmap if available
		if (yardMap) {
			groundBlockingObjectMap->AddGroundBlockingObject(this, yardMap);
		} else {
			groundBlockingObjectMap->AddGroundBlockingObject(this);
		}
	}
}



bool CSolidObject::AddBuildPower(float amount, CUnit* builder) {
	return false;
}

int2 CSolidObject::GetMapPos()
{
	int2 p;
	p.x = (int(pos.x + SQUARE_SIZE / 2) / SQUARE_SIZE) - xsize / 2;
	p.y = (int(pos.z + SQUARE_SIZE / 2) / SQUARE_SIZE) - ysize / 2;

	if (p.x < 0)
		p.x = 0;
	if (p.x > gs->mapx - xsize)
		p.x = gs->mapx - xsize;

	if (p.y < 0)
		p.y = 0;
	if (p.y > gs->mapy - ysize)
		p.y = gs->mapy - ysize;

	return p;
}
