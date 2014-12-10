/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ScriptMoveType.h"

#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CScriptMoveType, AMoveType, (NULL))
CR_REG_METADATA(CScriptMoveType, (
	CR_MEMBER(tag),
	CR_MEMBER(extrapolate),
	CR_MEMBER(useRelVel),
	CR_MEMBER(useRotVel),
	CR_MEMBER(drag),
	CR_MEMBER(velVec),
	CR_MEMBER(relVel),
	CR_MEMBER(rot),
	CR_MEMBER(rotVel),
	CR_MEMBER(mins),
	CR_MEMBER(maxs),

	CR_MEMBER(trackSlope),
	CR_MEMBER(trackGround),
	CR_MEMBER(groundOffset),
	CR_MEMBER(gravityFactor),
	CR_MEMBER(windFactor),
	CR_MEMBER(noBlocking), // copy of CSolidObject::PSTATE_BIT_BLOCKING
	CR_MEMBER(gndStop),
	CR_MEMBER(shotStop),
	CR_MEMBER(slopeStop),
	CR_MEMBER(collideStop),
	CR_MEMBER(scriptNotify),
	CR_RESERVED(64)
))


CScriptMoveType::CScriptMoveType(CUnit* owner):
	AMoveType(owner),
	tag(0),
	extrapolate(true),
	useRelVel(false),
	useRotVel(false),
	drag(0.0f),
	velVec(ZeroVector),
	relVel(ZeroVector),
	rot(ZeroVector),
	rotVel(ZeroVector),
	mins(-1.0e9f, -1.0e9f, -1.0e9f),
	maxs(+1.0e9f, +1.0e9f, +1.0e9f),
	trackSlope(false),
	trackGround(false),
	groundOffset(0.0f),
	gravityFactor(0.0f),
	windFactor(0.0f),
	noBlocking(false),
	gndStop(false),
	shotStop(false),
	slopeStop(false),
	collideStop(false),
	scriptNotify(0)
{
	useHeading = false; // use the transformation matrix instead of heading

	oldPos = owner? owner->pos: ZeroVector;
	oldSlowUpdatePos = oldPos;
}


CScriptMoveType::~CScriptMoveType()
{
	// clean up if noBlocking was made true at
	// some point during this script's lifetime
	// and not reset
	owner->UnBlock();
}

inline void CScriptMoveType::CalcDirections()
{
	CMatrix44f matrix;
	matrix.RotateY(-rot.y);
	matrix.RotateX(-rot.x);
	matrix.RotateZ(-rot.z);

	owner->SetDirVectors(matrix);
	owner->UpdateMidAndAimPos();
	owner->SetHeadingFromDirection();
}



void CScriptMoveType::CheckNotify()
{
	if (scriptNotify) {
		if (eventHandler.MoveCtrlNotify(owner, scriptNotify)) {
			// NOTE: deletes \<this\>
			owner->DisableScriptMoveType();
		} else {
			scriptNotify = 0;
		}
	}
}


bool CScriptMoveType::Update()
{
	if (useRotVel) {
		rot += rotVel;
		CalcDirections();
	}

	if (extrapolate) {
		// NOTE: only gravitational acc. is allowed to build up velocity
		// NOTE: strong wind plus low gravity can cause substantial drift
		const float3 gravVec = UpVector * (mapInfo->map.gravity * gravityFactor);
		const float3 windVec =            (wind.GetCurrentWind() * windFactor);
		const float3 unitVec = useRelVel?
			(owner->frontdir *  relVel.z) +
			(owner->updir    *  relVel.y) +
			(owner->rightdir * -relVel.x):
			ZeroVector;

		owner->Move(gravVec + velVec, true);
		owner->Move(windVec,          true);
		owner->Move(unitVec,          true);

		// quadratic drag does not work well here
		velVec += gravVec;
		velVec *= (1.0f - drag);

		owner->SetVelocityAndSpeed(velVec);
	}

	if (trackGround) {
		const float gndMin = CGround::GetHeightReal(owner->pos.x, owner->pos.z) + groundOffset;

		if (owner->pos.y <= gndMin) {
			owner->Move(UpVector * (gndMin - owner->pos.y), true);
			owner->speed.y = 0.0f;

			if (gndStop) {
				velVec = ZeroVector;
				relVel = ZeroVector;
				rotVel = ZeroVector;
				scriptNotify = 1;
			}
		}
	}

	// positional clamps
	CheckLimits();

	if (trackSlope) {
		owner->UpdateDirVectors(true);
		owner->UpdateMidAndAimPos();
	}

	// don't need the rest if the pos hasn't changed
	if (oldPos == owner->pos) {
		CheckNotify();
		return false;
	}

	oldPos = owner->pos;

	if (!noBlocking) {
		owner->Block();
	}

	CheckNotify();
	return true;
}


void CScriptMoveType::CheckLimits()
{
	if (owner->pos.x < mins.x) { owner->pos.x = mins.x; owner->speed.x = 0.0f; }
	if (owner->pos.x > maxs.x) { owner->pos.x = maxs.x; owner->speed.x = 0.0f; }
	if (owner->pos.y < mins.y) { owner->pos.y = mins.y; owner->speed.y = 0.0f; }
	if (owner->pos.y > maxs.y) { owner->pos.y = maxs.y; owner->speed.y = 0.0f; }
	if (owner->pos.z < mins.z) { owner->pos.z = mins.z; owner->speed.z = 0.0f; }
	if (owner->pos.z > maxs.z) { owner->pos.z = maxs.z; owner->speed.z = 0.0f; }

	owner->UpdateMidAndAimPos();
}


void CScriptMoveType::SetPhysics(const float3& _pos, const float3& _vel, const float3& _rot)
{
	SetPosition(_pos);
	SetVelocity(_vel);
	SetRotation(_rot);
}


void CScriptMoveType::SetPosition(const float3& _pos) { owner->Move(_pos, false); }
void CScriptMoveType::SetVelocity(const float3& _vel) { owner->SetVelocityAndSpeed(velVec = _vel); }



void CScriptMoveType::SetRelativeVelocity(const float3& _relVel)
{
	relVel = _relVel;
	useRelVel = ((relVel.x != 0.0f) || (relVel.y != 0.0f) || (relVel.z != 0.0f));
}


void CScriptMoveType::SetRotation(const float3& _rot)
{
	rot = _rot;
	CalcDirections();
}


void CScriptMoveType::SetRotationVelocity(const float3& _rotVel)
{
	rotVel = _rotVel;
	useRotVel = ((rotVel.x != 0.0f) || (rotVel.y != 0.0f) || (rotVel.z != 0.0f));
}


void CScriptMoveType::SetHeading(short heading)
{
	owner->heading = heading;

	if (!trackSlope) {
		owner->UpdateDirVectors(false);
		owner->UpdateMidAndAimPos();
	}
}


void CScriptMoveType::SetNoBlocking(bool state)
{
	// if false, forces blocking-map updates
	noBlocking = state;

	if (noBlocking) {
		owner->UnBlock();
	} else {
		owner->Block();
	}
}
