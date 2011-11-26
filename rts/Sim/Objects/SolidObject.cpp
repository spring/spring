/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "SolidObject.h"
#include "Map/ReadMap.h"
#include "Map/Ground.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "System/myMath.h"

int CSolidObject::deletingRefID = -1;
const float CSolidObject::DEFAULT_MASS = 1e5f;
const float CSolidObject::MINIMUM_MASS = 1e0f; // 1.0f
const float CSolidObject::MAXIMUM_MASS = 1e6f;

CR_BIND_DERIVED(CSolidObject, CWorldObject, );
CR_REG_METADATA(CSolidObject,
(
	CR_MEMBER(mass),
	CR_MEMBER(blocking),
	CR_MEMBER(isUnderWater),
	CR_MEMBER(immobile),
	CR_MEMBER(blockHeightChanges),
	CR_MEMBER(crushKilled),
	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
	CR_MEMBER(height),
	CR_MEMBER(heading),
	CR_ENUM_MEMBER(physicalState),
	CR_MEMBER(relMidPos),
	CR_MEMBER(midPos),
//	CR_MEMBER(drawPos),
//	CR_MEMBER(drawMidPos),
	CR_MEMBER(isMoving),
	CR_MEMBER(residualImpulse),
	CR_MEMBER(allyteam),
	CR_MEMBER(team),
	CR_MEMBER(mobility),
	CR_MEMBER(collisionVolume),
	// can not get creg work on templates
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
	immobile(false),
	blockHeightChanges(false),
	crushKilled(false),
	xsize(1),
	zsize(1),
	height(1.0f),
	heading(0),
	physicalState(OnGround),
	isMoving(false),
	isUnderWater(false),
	isMarkedOnBlockingMap(false),
	speed(ZeroVector),
	residualImpulse(ZeroVector),
	allyteam(0),
	team(0),
	mobility(NULL),
	collisionVolume(NULL),
	relMidPos(0.0f, 0.0f, 0.0f),
	midPos(pos),
	curYardMap(NULL),
	buildFacing(0)
{
	mapPos = GetMapPos();
	// FIXME collisionVolume volume with CWorldObject.radius?
}

CSolidObject::~CSolidObject() {
	blocking = false;

	delete mobility;
	mobility = NULL;

	delete collisionVolume;
	collisionVolume = NULL;
}



void CSolidObject::UnBlock() {
	if (isMarkedOnBlockingMap) {
		groundBlockingObjectMap->RemoveGroundBlockingObject(this);
		// isMarkedOnBlockingMap is now false
	}
}

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

//FIXME move somewhere else?
int2 CSolidObject::GetMapPos(const float3& position)
{
	int2 p;
	p.x = (int(position.x + SQUARE_SIZE / 2) / SQUARE_SIZE) - (xsize / 2);
	p.y = (int(position.z + SQUARE_SIZE / 2) / SQUARE_SIZE) - (zsize / 2);

	if (p.x < 0) {
		p.x = 0;
	}
	if (p.x > gs->mapx - xsize) {
		p.x = gs->mapx - xsize;
	}

	if (p.y < 0) {
		p.y = 0;
	}
	if (p.y > gs->mapy - zsize) {
		p.y = gs->mapy - zsize;
	}

	return p;
}
