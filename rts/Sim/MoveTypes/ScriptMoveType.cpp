/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "ScriptMoveType.h"

#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CScriptMoveType, AMoveType, (NULL));
CR_REG_METADATA(CScriptMoveType, (
	CR_MEMBER(tag),
	CR_MEMBER(extrapolate),
	CR_MEMBER(useRelVel),
	CR_MEMBER(useRotVel),
	CR_MEMBER(drag),
	CR_MEMBER(vel),
	CR_MEMBER(relVel),
	CR_MEMBER(rot),
	CR_MEMBER(rotVel),
	CR_MEMBER(trackSlope),
	CR_MEMBER(trackGround),
	CR_MEMBER(groundOffset),
	CR_MEMBER(gravityFactor),
	CR_MEMBER(windFactor),
	CR_MEMBER(mins),
	CR_MEMBER(maxs),
	CR_MEMBER(noBlocking), // copy of CSolidObject::isMarkedOnBlockingMap
	CR_MEMBER(gndStop),
	CR_MEMBER(shotStop),
	CR_MEMBER(slopeStop),
	CR_MEMBER(collideStop),
	CR_MEMBER(scriptNotify),
	CR_RESERVED(64)
));


CScriptMoveType::CScriptMoveType(CUnit* owner):
	AMoveType(owner),
	tag(0),
	extrapolate(true),
	useRelVel(false),
	useRotVel(false),
	drag(0.0f),
	vel(ZeroVector),
	relVel(ZeroVector),
	rot(ZeroVector),
	rotVel(ZeroVector),
	trackSlope(false),
	trackGround(false),
	groundOffset(0.0f),
	gravityFactor(0.0f),
	windFactor(0.0f),
	mins(-1.0e9f, -1.0e9f, -1.0e9f),
	maxs(+1.0e9f, +1.0e9f, +1.0e9f),
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
	owner->Block();
}

#if defined(USE_GML) && defined(__GNUC__) && (__GNUC__ == 4)
// This is supposed to fix some GCC crashbug related to threading
// The MOVAPS SSE instruction is otherwise getting misaligned data
__attribute__ ((force_align_arg_pointer))
#endif
inline void CScriptMoveType::CalcDirections()
{
	CMatrix44f matrix;
	matrix.RotateY(-rot.y);
	matrix.RotateX(-rot.x);
	matrix.RotateZ(-rot.z);

	owner->SetDirVectors(matrix);
	owner->UpdateMidPos();
	owner->SetHeadingFromDirection();
}



void CScriptMoveType::CheckNotify()
{
	if (scriptNotify) {
		if (luaRules && luaRules->MoveCtrlNotify(owner, scriptNotify)) {
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

		owner->Move3D(gravVec + vel, true);
		owner->Move3D(windVec,       true);
		owner->Move3D(unitVec,       true);

		// quadratic drag does not work well here
		vel += gravVec;
		vel *= (1.0f - drag);

		owner->speed = vel;
	}

	if (trackGround) {
		const float gndMin = ground->GetHeightReal(owner->pos.x, owner->pos.z) + groundOffset;

		if (owner->pos.y <= gndMin) {
			owner->Move1D(gndMin, 1, false);
			owner->speed.y = 0.0f;

			if (gndStop) {
				vel    = ZeroVector;
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
		owner->UpdateMidPos();
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

	owner->UpdateMidPos();
}


void CScriptMoveType::SetPhysics(const float3& pos,
                                 const float3& vel,
                                 const float3& rot)
{
	owner->pos = pos;
	owner->speed = vel;

	SetRotation(rot);
}


void CScriptMoveType::SetPosition(const float3& pos) { owner->pos = pos; }
void CScriptMoveType::SetVelocity(const float3& _vel) { owner->speed = (vel = _vel); }



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


void CScriptMoveType::SetRotationVelocity(const float3& rvel)
{
	rotVel = rvel;
	useRotVel = ((rotVel.x != 0.0f) || (rotVel.y != 0.0f) || (rotVel.z != 0.0f));
}


void CScriptMoveType::SetHeading(short heading)
{
	owner->heading = heading;

	if (!trackSlope) {
		owner->UpdateDirVectors(false);
		owner->UpdateMidPos();
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
