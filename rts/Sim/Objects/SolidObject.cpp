/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "SolidObject.h"
#include "Map/ReadMap.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "myMath.h"

const float CSolidObject::DEFAULT_MASS = 100000.0f;

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
//	CR_MEMBER(drawPos),
//	CR_MEMBER(drawMidPos),
	CR_MEMBER(isMoving),
	CR_MEMBER(residualImpulse),
	CR_MEMBER(allyteam),
	CR_MEMBER(team),
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
	mass(DEFAULT_MASS),
	blocking(false),
	floatOnWater(false),
	immobile(false),
	blockHeightChanges(false),
	xsize(1),
	zsize(1),
	height(1),
	heading(0),
	physicalState(OnGround),
	isMoving(false),
	isUnderWater(false),
	isMarkedOnBlockingMap(false),
	speed(0, 0, 0),
	residualImpulse(0, 0, 0),
	allyteam(0),
	team(0),
	mobility(NULL),
	midPos(pos),
	curYardMap(0),
	buildFacing(0)
{
	mapPos = GetMapPos();
	collisionVolume = NULL; //FIXME create collision volume with CWorldObject.radius?
}

CSolidObject::~CSolidObject() {
	blocking = false;

	delete mobility;
	mobility = NULL;

	delete collisionVolume;
	collisionVolume = NULL;
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
	if (physicalState == Flying) {
		return;
	}

	// use the object's current yardmap if available
	if (curYardMap != 0) {
		groundBlockingObjectMap->AddGroundBlockingObject(this, curYardMap, 255);
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
