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
	CR_MEMBER(crushResistance),

	CR_MEMBER(blocking),
	CR_MEMBER(crushable),
	CR_MEMBER(immobile),
	CR_MEMBER(blockHeightChanges),
	CR_MEMBER(crushKilled),

	CR_MEMBER(xsize),
	CR_MEMBER(zsize),

	CR_MEMBER(heading),
	CR_ENUM_MEMBER(physicalState),

	CR_MEMBER(frontdir),
	CR_MEMBER(rightdir),
	CR_MEMBER(updir),

	CR_MEMBER(relMidPos),
	CR_MEMBER(midPos),
	// can not get creg work on templates
	CR_MEMBER(mapPos.x),
	CR_MEMBER(mapPos.y),

//	CR_MEMBER(drawPos),
//	CR_MEMBER(drawMidPos),

	CR_MEMBER(isMoving),
	CR_MEMBER(isUnderWater),
	CR_MEMBER(isMarkedOnBlockingMap),

	CR_MEMBER(speed),
	CR_MEMBER(residualImpulse),

	CR_MEMBER(team),
	CR_MEMBER(allyteam),

	CR_MEMBER(mobility),
	CR_MEMBER(collisionVolume),

	CR_MEMBER(buildFacing),
	CR_RESERVED(16))
);


CSolidObject::CSolidObject():
	mass(DEFAULT_MASS),
	crushResistance(0.0f),
	blocking(false),
	crushable(false),
	immobile(false),
	blockHeightChanges(false),
	crushKilled(false),
	xsize(1),
	zsize(1),
	heading(0),
	physicalState(OnGround),
	isMoving(false),
	isUnderWater(false),
	isMarkedOnBlockingMap(false),
	speed(ZeroVector),
	residualImpulse(ZeroVector),
	team(0),
	allyteam(0),
	mobility(NULL),
	collisionVolume(NULL),
	frontdir(0.0f, 0.0f, 1.0f),
	rightdir(-1.0f, 0.0f, 0.0f),
	updir(0.0f, 1.0f, 0.0f),
	relMidPos(ZeroVector),
	midPos(pos),
	mapPos(GetMapPos()),
	curYardMap(NULL),
	buildFacing(0)
{
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



// FIXME move somewhere else?
int2 CSolidObject::GetMapPos(const float3& position) const
{
	int2 mp;
	mp.x = (int(position.x + SQUARE_SIZE / 2) / SQUARE_SIZE) - (xsize / 2);
	mp.y = (int(position.z + SQUARE_SIZE / 2) / SQUARE_SIZE) - (zsize / 2);

	if (mp.x <                0) { mp.x =                0; }
	if (mp.y <                0) { mp.y =                0; }
	if (mp.x > gs->mapx - xsize) { mp.x = gs->mapx - xsize; }
	if (mp.y > gs->mapy - zsize) { mp.y = gs->mapy - zsize; }

	return mp;
}
