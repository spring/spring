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

CR_BIND_DERIVED(CSolidObject, CWorldObject, );
CR_REG_METADATA(CSolidObject,
(
	CR_MEMBER(health),
	CR_MEMBER(mass),
	CR_MEMBER(crushResistance),

	CR_MEMBER(crushable),
	CR_MEMBER(immobile),
	CR_MEMBER(blockEnemyPushing),
	CR_MEMBER(blockHeightChanges),

	CR_MEMBER(luaDraw),
	CR_MEMBER(noSelect),

	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
 	CR_MEMBER(footprint),
	CR_MEMBER(heading),

	CR_ENUM_MEMBER(physicalState),
	CR_ENUM_MEMBER(collidableState),

	CR_MEMBER(team),
	CR_MEMBER(allyteam),

	CR_MEMBER(tempNum),

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
	CR_MEMBER(groundBlockPos),

	CR_MEMBER(dragScales),

	CR_MEMBER(drawPos),
	CR_MEMBER(drawMidPos),
	// CR_MEMBER(blockMap), //FIXME add bitwiseenum to creg

	CR_MEMBER(buildFacing)
));


CSolidObject::CSolidObject():
	health(0.0f),
	mass(DEFAULT_MASS),
	crushResistance(0.0f),

	crushable(false),
	immobile(false),
	blockEnemyPushing(true),
	blockHeightChanges(false),

	luaDraw(false),
	noSelect(false),

	xsize(1),
	zsize(1),
	footprint(1, 1),

	heading(0),

	// objects start out non-blocking but fully collidable
	// SolidObjectDef::collidable controls only the SO-bit
	physicalState(PhysicalState(PSTATE_BIT_ONGROUND)),
	collidableState(CollidableState(CSTATE_BIT_SOLIDOBJECTS | CSTATE_BIT_PROJECTILES | CSTATE_BIT_QUADMAPRAYS)),

	team(0),
	allyteam(0),

	tempNum(0),

	objectDef(NULL),
	moveDef(NULL),
	collisionVolume(NULL),
	groundDecal(NULL),

	frontdir( FwdVector),
	rightdir(-RgtVector),
	updir(UpVector),

	midPos(pos),
	mapPos(GetMapPos()),

	dragScales(OnesVector),

	blockMap(NULL),
	buildFacing(0)
{
}

CSolidObject::~CSolidObject() {
	ClearCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS | CSTATE_BIT_PROJECTILES | CSTATE_BIT_QUADMAPRAYS);

	delete collisionVolume;
	collisionVolume = NULL;
}

void CSolidObject::UpdatePhysicalState(float eps) {
	const float gh = ground->GetHeightReal(pos.x, pos.z);
	const float wh = std::max(gh, 0.0f);

	unsigned int ps = physicalState;

	// clear all non-void non-special bits
	ps &= (~PSTATE_BIT_ONGROUND   );
	ps &= (~PSTATE_BIT_INWATER    );
	ps &= (~PSTATE_BIT_UNDERWATER );
	ps &= (~PSTATE_BIT_UNDERGROUND);
	ps &= (~PSTATE_BIT_INAIR      );

	// NOTE:
	//   height is not in general equivalent to radius * 2.0
	//   the height property is used for much fewer purposes
	//   than radius, so less reliable for determining state
	#define MASK_NOAIR (PSTATE_BIT_ONGROUND | PSTATE_BIT_INWATER | PSTATE_BIT_UNDERWATER | PSTATE_BIT_UNDERGROUND)
	ps |= (PSTATE_BIT_ONGROUND    * ((   pos.y -         gh) <=  eps));
	ps |= (PSTATE_BIT_INWATER     * ((   pos.y             ) <= 0.0f));
//	ps |= (PSTATE_BIT_UNDERWATER  * ((   pos.y +     height) <  0.0f));
//	ps |= (PSTATE_BIT_UNDERGROUND * ((   pos.y +     height) <    gh));
	ps |= (PSTATE_BIT_UNDERWATER  * ((midPos.y +     radius) <  0.0f));
	ps |= (PSTATE_BIT_UNDERGROUND * ((midPos.y +     radius) <    gh));
	ps |= (PSTATE_BIT_INAIR       * ((   pos.y -         wh) >   eps));
	ps |= (PSTATE_BIT_INAIR       * ((    ps   & MASK_NOAIR) ==    0));
	#undef MASK_NOAIR

	physicalState = static_cast<PhysicalState>(ps);

	// verify mutex relations (A != B); if one
	// fails then A and B *must* both be false
	//
	// problem case: pos.y < eps (but > 0) &&
	// gh < -eps causes ONGROUND and INAIR to
	// both be false but INWATER will fail too
	#if 0
	assert((IsInAir() != IsOnGround()) || IsInWater());
	assert((IsInAir() != IsInWater()) || IsOnGround());
	assert((IsInAir() != IsUnderWater()) || (IsOnGround() || IsInWater()));
	#endif
}


bool CSolidObject::SetVoidState() {
	if (IsInVoid())
		return false;

	// make us transparent to raycasts, quadfield queries, etc.
	// need to push and pop state bits in case Lua changes them
	// (otherwise gadgets must listen to all Unit*Loaded events)
	PushCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS);
	PushCollidableStateBit(CSTATE_BIT_PROJECTILES);
	PushCollidableStateBit(CSTATE_BIT_QUADMAPRAYS);
	ClearCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS | CSTATE_BIT_PROJECTILES | CSTATE_BIT_QUADMAPRAYS);
	SetPhysicalStateBit(PSTATE_BIT_INVOID);

	UnBlock();
	collisionVolume->SetIgnoreHits(true);
	return true;
}

bool CSolidObject::ClearVoidState() {
	if (!IsInVoid())
		return false;

	PopCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS);
	PopCollidableStateBit(CSTATE_BIT_PROJECTILES);
	PopCollidableStateBit(CSTATE_BIT_QUADMAPRAYS);
	ClearPhysicalStateBit(PSTATE_BIT_INVOID);

	Block();
	collisionVolume->SetIgnoreHits(false);
	return true;
}

void CSolidObject::UpdateVoidState(bool set) {
	if (set) {
		SetVoidState();
	} else {
		ClearVoidState();
	}

	noSelect = (set || !objectDef->selectable);
}



void CSolidObject::UnBlock() {
	if (!IsBlocking())
		return;

	groundBlockingObjectMap->RemoveGroundBlockingObject(this);
	assert(!IsBlocking());
}

void CSolidObject::Block() {
	// no point calling this if object is not
	// collidable in principle, but simplifies
	// external code to allow it
	if (!HasCollidableStateBit(CSTATE_BIT_SOLIDOBJECTS))
		return;

	if (IsBlocking() && !BlockMapPosChanged())
		return;

	UnBlock();
	groundBlockingObjectMap->AddGroundBlockingObject(this);
	assert(IsBlocking());
}


YardMapStatus CSolidObject::GetGroundBlockingMaskAtPos(float3 gpos) const
{
	if (!blockMap)
		return YARDMAP_OPEN;

	const int hxsize = footprint.x >> 1;
	const int hzsize = footprint.y >> 1;

	float3 frontv;
	float3 rightv;

	#if 1
		// use continuous floating-point space
		gpos   -= pos;
		gpos.x += SQUARE_SIZE / 2; //??? needed to move to SQUARE-center? (possibly current input is wrong)
		gpos.z += SQUARE_SIZE / 2;

		frontv =  frontdir;
		rightv = -rightdir; //??? spring's unit-rightdir is in real the LEFT vector :x
	#else
		// use old fixed space (4 facing dirs & ints for unit positions)

		// form the rotated axis vectors
		static float3 fronts[] = {FwdVector,  RgtVector, -FwdVector, -RgtVector};
		static float3 rights[] = {RgtVector, -FwdVector, -RgtVector,  FwdVector};

		// get used axis vectors
		frontv = fronts[buildFacing];
		rightv = rights[buildFacing];

		gpos -= float3(mapPos.x * SQUARE_SIZE, 0.0f, mapPos.y * SQUARE_SIZE);

		// need to revert some of the transformations of CSolidObject::GetMapPos()
		gpos.x += SQUARE_SIZE / 2 - (this->xsize >> 1) * SQUARE_SIZE; 
		gpos.z += SQUARE_SIZE / 2 - (this->zsize >> 1) * SQUARE_SIZE;
	#endif

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

float3 CSolidObject::GetDragAccelerationVec(const float4& params) const {
	// KISS: use the cross-sectional area of a sphere, object shapes are complex
	// this is a massive over-estimation so pretend the radius is in centimeters
	// other units as normal: mass in kg, speed in elmos/frame, density in kg/m^3
	//
	// params.xyzw map: {{atmosphere, water}Density, {drag, friction}Coefficient}
	//
	const float3 speedSignVec = float3(Sign(speed.x), Sign(speed.y), Sign(speed.z));
	const float3 dragScaleVec = float3(
		IsInAir()    * dragScales.x * (0.5f * params.x * params.z * (M_PI * sqRadius * 0.01f * 0.01f)), // air
		IsInWater()  * dragScales.y * (0.5f * params.y * params.z * (M_PI * sqRadius * 0.01f * 0.01f)), // water
		IsOnGround() * dragScales.z * (                  params.w * (                           mass))  // ground
	);

	float3 dragAccelVec;

	dragAccelVec.x += (speed.x * speed.x * dragScaleVec.x * -speedSignVec.x);
	dragAccelVec.y += (speed.y * speed.y * dragScaleVec.x * -speedSignVec.y);
	dragAccelVec.z += (speed.z * speed.z * dragScaleVec.x * -speedSignVec.z);

	dragAccelVec.x += (speed.x * speed.x * dragScaleVec.y * -speedSignVec.x);
	dragAccelVec.y += (speed.y * speed.y * dragScaleVec.y * -speedSignVec.y);
	dragAccelVec.z += (speed.z * speed.z * dragScaleVec.y * -speedSignVec.z);

	// FIXME?
	//   magnitude of dynamic friction may or may not depend on speed
	//   coefficient must be multiplied by mass or it will be useless
	//   (due to division by mass since the coefficient is normalized)
	dragAccelVec.x += (math::fabs(speed.x) * dragScaleVec.z * -speedSignVec.x);
	dragAccelVec.y += (math::fabs(speed.y) * dragScaleVec.z * -speedSignVec.y);
	dragAccelVec.z += (math::fabs(speed.z) * dragScaleVec.z * -speedSignVec.z);

	// convert from force
	dragAccelVec /= mass;

	// limit the acceleration
	dragAccelVec.x = Clamp(dragAccelVec.x, -math::fabs(speed.x), math::fabs(speed.x));
	dragAccelVec.y = Clamp(dragAccelVec.y, -math::fabs(speed.y), math::fabs(speed.y));
	dragAccelVec.z = Clamp(dragAccelVec.z, -math::fabs(speed.z), math::fabs(speed.z));

	return dragAccelVec;
}

float3 CSolidObject::GetWantedUpDir(bool useGroundNormal) const {
	// NOTE:
	//   for aircraft IsOnGround is already factored into useGroundNormal
	//   for ground-units the situation is more complicated because 1) it
	//   depends on the 'upright' tag and 2) ships and hovercraft are not
	//   "on the ground" all the time ('ground' is the ocean floor, *not*
	//   the water surface) and neither are tanks / bots due to impulses,
	//   gravity, ...
	//
	const float3 gn = ground->GetSmoothNormal(pos.x, pos.z) * (    useGroundNormal);
	const float3 wn =                             UpVector  * (1 - useGroundNormal);

	if (moveDef == NULL) {
		// aircraft cannot use updir reliably or their
		// coordinate-system would degenerate too much
		// over time without periodic re-ortho'ing
		return (gn + UpVector * (1 - useGroundNormal));
	}

	// not an aircraft if we get here, prevent pitch changes
	// if(f) the object is neither on the ground nor in water
	// for whatever reason (GMT also prevents heading changes)
	if (!IsInAir()) {
		switch (moveDef->speedModClass) {
			case MoveDef::Tank:  { return ((gn + wn) * IsOnGround() + updir * (1 - IsOnGround())); } break;
			case MoveDef::KBot:  { return ((gn + wn) * IsOnGround() + updir * (1 - IsOnGround())); } break;

			case MoveDef::Hover: { return ((UpVector * IsInWater()) + (gn + wn) * (1 - IsInWater())); } break;
			case MoveDef::Ship:  { return ((UpVector * IsInWater()) + (gn + wn) * (1 - IsInWater())); } break;
		}
	}

	// prefer to keep local up-vector as long as possible
	return updir;
}

void CSolidObject::SetHeadingFromDirection() {
	heading = GetHeadingFromVector(frontdir.x, frontdir.z);
}



void CSolidObject::Kill(CUnit* killer, const float3& impulse, bool crushed) {
	UpdateVoidState(false);

	if (crushed) {
		DoDamage(DamageArray(health + 1.0f), impulse, killer, -DAMAGE_EXTSOURCE_CRUSHED, -1);
	} else {
		DoDamage(DamageArray(health + 1.0f), impulse, killer, -DAMAGE_EXTSOURCE_KILLED, -1);
	}
}

