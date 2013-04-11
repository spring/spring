/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SolidObject.h"
#include "SolidObjectDef.h"
#include "Map/ReadMap.h"
#include "Map/Ground.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "System/myMath.h"

int CSolidObject::deletingRefID = -1;

const float CSolidObject::DEFAULT_MASS = 1e5f;
const float CSolidObject::MINIMUM_MASS = 1e0f; // 1.0f
const float CSolidObject::MAXIMUM_MASS = 1e6f;
const float CSolidObject::IMPULSE_RATE = 0.968f;

CR_BIND_DERIVED(CSolidObject, CWorldObject, );
CR_REG_METADATA(CSolidObject,
(
	CR_MEMBER(health),
	CR_MEMBER(mass),
	CR_MEMBER(crushResistance),
	CR_MEMBER(impulseDecayRate),
	CR_MEMBER(blocking),
	CR_MEMBER(crushable),
	CR_MEMBER(immobile),
	CR_MEMBER(crushKilled),
	CR_MEMBER(blockEnemyPushing),
	CR_MEMBER(blockHeightChanges),
	CR_MEMBER(luaDraw),
	CR_MEMBER(noSelect),
	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
 	CR_MEMBER(footprint),
	CR_MEMBER(heading),
	CR_ENUM_MEMBER(physicalState),
	CR_MEMBER(isMoving),
	CR_MEMBER(isMarkedOnBlockingMap),
	CR_MEMBER(groundBlockPos),
	CR_MEMBER(speed),
	CR_MEMBER(residualImpulse),
	CR_MEMBER(team),
	CR_MEMBER(allyteam),
	CR_MEMBER(objectDef),
	CR_MEMBER(moveDef),
	CR_MEMBER(collisionVolume),
	CR_IGNORED(groundDecal),
	CR_MEMBER(frontdir),
	CR_MEMBER(rightdir),
	CR_MEMBER(updir),
	CR_MEMBER(relMidPos),
 	CR_MEMBER(relAimPos),
	CR_MEMBER(midPos),
	CR_MEMBER(aimPos),
	CR_MEMBER(mapPos),
	CR_MEMBER(drawPos),
	CR_MEMBER(drawMidPos),
	//CR_MEMBER(blockMap), //FIXME add bitwiseenum to creg
	CR_MEMBER(buildFacing)
));


CSolidObject::CSolidObject():
	health(0.0f),
	mass(DEFAULT_MASS),
	crushResistance(0.0f),
	impulseDecayRate(IMPULSE_RATE),

	blocking(false),
	crushable(false),
	immobile(false),
	crushKilled(false),
	blockEnemyPushing(true),
	blockHeightChanges(false),

	luaDraw(false),
	noSelect(false),

	xsize(1),
	zsize(1),
	footprint(1, 1),

	heading(0),
	physicalState(STATE_BIT_ONGROUND),

	isMoving(false),
	isMarkedOnBlockingMap(false),

	speed(ZeroVector),
	residualImpulse(ZeroVector),
	team(0),
	allyteam(0),

	objectDef(NULL),
	moveDef(NULL),
	collisionVolume(NULL),
	groundDecal(NULL),

	frontdir(0.0f, 0.0f, 1.0f),
	rightdir(-1.0f, 0.0f, 0.0f),
	updir(0.0f, 1.0f, 0.0f),
	relMidPos(ZeroVector),
	midPos(pos),
	mapPos(GetMapPos()),
	blockMap(NULL),
	buildFacing(0)
{
}

CSolidObject::~CSolidObject() {
	blocking = false;

	delete collisionVolume;
	collisionVolume = NULL;
}

void CSolidObject::UpdatePhysicalState() {
	const float mh = height * (moveDef == NULL || !moveDef->subMarine);
	const float gh = ground->GetHeightReal(pos.x, pos.z);

	unsigned int ps = physicalState;

	ps &= (~CSolidObject::STATE_BIT_ONGROUND);
	ps &= (~CSolidObject::STATE_BIT_INWATER);
	ps &= (~CSolidObject::STATE_BIT_UNDERWATER);
	ps &= (~CSolidObject::STATE_BIT_INAIR);

	ps |= (CSolidObject::STATE_BIT_ONGROUND   * ((pos.y -                 gh) <= 1.0f));
	ps |= (CSolidObject::STATE_BIT_INWATER    * ( pos.y                       <= 0.0f));
	ps |= (CSolidObject::STATE_BIT_UNDERWATER * ((pos.y +                 mh) <  0.0f));
	ps |= (CSolidObject::STATE_BIT_INAIR      * ((pos.y - std::max(gh, 0.0f)) >  1.0f));

	physicalState = static_cast<PhysicalState>(ps);
}



void CSolidObject::UnBlock() {
	if (isMarkedOnBlockingMap) {
		groundBlockingObjectMap->RemoveGroundBlockingObject(this);
	}

	assert(!isMarkedOnBlockingMap);
}

void CSolidObject::Block() {
	if (IsFlying()) {
		//FIXME why do airmovetypes really rely on Block() to UNblock!
		UnBlock();
		return;
	}

	if (!blocking) {
		UnBlock();
		return;
	}

	if (isMarkedOnBlockingMap && groundBlockPos == pos) {
		return;
	}

	UnBlock();
	groundBlockPos = pos;
	groundBlockingObjectMap->AddGroundBlockingObject(this);
	assert(isMarkedOnBlockingMap);
}


YardMapStatus CSolidObject::GetGroundBlockingMaskAtPos(float3 gpos) const
{
	if (!blockMap)
		return YARDMAP_OPEN;

	const int hxsize = footprint.x >> 1;
	const int hzsize = footprint.y >> 1;

	float3 frontv;
	float3 rightv;

	if (true) {
		// use continuous floating-point space
		gpos   -= pos;
		gpos.x += SQUARE_SIZE / 2; //??? needed to move to SQUARE-center? (possibly current input is wrong)
		gpos.z += SQUARE_SIZE / 2;

		frontv =  frontdir;
		rightv = -rightdir; //??? spring's unit-rightdir is in real the LEFT vector :x
	} else {
		// use old fixed space (4 facing dirs & ints for unit positions)

		// form the rotated axis vectors
		static float3 up   ( 0.0f, 0.0f, -1.0f);
		static float3 down ( 0.0f, 0.0f,  1.0f);
		static float3 left (-1.0f, 0.0f,  0.0f);
		static float3 right( 1.0f, 0.0f,  0.0f);
		static float3 fronts[] = {down, right, up, left};
		static float3 rights[] = {right, up, left, down};

		// get used axis vectors
		frontv = fronts[buildFacing];
		rightv = rights[buildFacing];

		gpos -= float3(mapPos.x * SQUARE_SIZE, 0.0f, mapPos.y * SQUARE_SIZE);

		// need to revert some of the transformations of CSolidObject::GetMapPos()
		gpos.x += SQUARE_SIZE / 2 - (this->xsize >> 1) * SQUARE_SIZE; 
		gpos.z += SQUARE_SIZE / 2 - (this->zsize >> 1) * SQUARE_SIZE;
	}

	// transform worldspace pos to unit rotation dependent `centered blockmap space` [-hxsize .. +hxsize] x [-hzsize .. +hzsize]
	float by = frontv.dot(gpos) / SQUARE_SIZE;
	float bx = rightv.dot(gpos) / SQUARE_SIZE;

	// outside of `blockmap space`?
	if ((math::fabsf(bx) >= hxsize) || (math::fabsf(by) >= hzsize))
		return YARDMAP_OPEN;

	// transform: [(-hxsize + eps) .. (+hxsize - eps)] x [(-hzsize + eps) .. (+hzsize - eps)] -> [0 .. (xsize - 1)] x [0 .. (zsize - 1)]
	bx += hxsize;
	by += hzsize;
	assert(int(bx) >= 0 && int(bx) < footprint.x);
	assert(int(by) >= 0 && int(by) < footprint.y);

	// read from blockmap
	return blockMap[int(bx) + int(by) * footprint.x];
}


// FIXME move somewhere else?
int2 CSolidObject::GetMapPos(const float3& position) const
{
	int2 mp;

	mp.x = (int(position.x + SQUARE_SIZE / 2) / SQUARE_SIZE) - (xsize / 2);
	mp.y = (int(position.z + SQUARE_SIZE / 2) / SQUARE_SIZE) - (zsize / 2);
	mp.x = Clamp(mp.x, 0, gs->mapx - xsize);
	mp.y = Clamp(mp.y, 0, gs->mapy - zsize);

	return mp;
}



void CSolidObject::Kill(const float3& impulse, bool crushKill) {
	crushKilled = crushKill;

	DamageArray damage(health + 1.0f);
	DoDamage(damage, impulse, NULL, -DAMAGE_EXTSOURCE_KILLED, -1);
}

