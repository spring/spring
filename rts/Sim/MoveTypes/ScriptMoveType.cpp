/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "ScriptMoveType.h"

#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "GlobalUnsynced.h"
#include "Matrix44f.h"
#include "myMath.h"

CR_BIND_DERIVED(CScriptMoveType, AMoveType, (NULL));
CR_REG_METADATA(CScriptMoveType, (
	CR_MEMBER(tag),
	CR_MEMBER(extrapolate),
	CR_MEMBER(drag),
	CR_MEMBER(vel),
	CR_MEMBER(relVel),
	CR_MEMBER(useRelVel),
	CR_MEMBER(rot),
	CR_MEMBER(rotVel),
	CR_MEMBER(useRotVel),
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
	CR_MEMBER(leaveTracks),
	CR_MEMBER(rotOffset),
	CR_MEMBER(lastTrackUpdate),
	CR_MEMBER(scriptNotify),
	CR_RESERVED(64)
));


CScriptMoveType::CScriptMoveType(CUnit* owner):
	AMoveType(owner),
	tag(0),
	extrapolate(true),
	drag(0.0f),
	vel(0.0f, 0.0f, 0.0f),
	relVel(0.0f, 0.0f, 0.0f),
	useRelVel(false),
	rot(0.0f, 0.0f, 0.0f),
	rotVel(0.0f, 0.0f, 0.0f),
	useRotVel(false),
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
	leaveTracks(true),
	rotOffset(0.0f, 0.0f, 0.0f),
	lastTrackUpdate(0),
	scriptNotify(0)
{
	useHeading = false; // use the transformation matrix instead of heading

	oldPos = owner? owner->pos: ZeroVector;
	oldSlowUpdatePos = oldPos;
}


CScriptMoveType::~CScriptMoveType(void)
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
	//matrix.Translate(-rotOffset);
	matrix.RotateY(-rot.y);
	matrix.RotateX(-rot.x);
	matrix.RotateZ(-rot.z);
	//matrix.Translate(rotOffset);
	owner->rightdir.x = -matrix[ 0];
	owner->rightdir.y = -matrix[ 1];
	owner->rightdir.z = -matrix[ 2];
	owner->updir.x    =  matrix[ 4];
	owner->updir.y    =  matrix[ 5];
	owner->updir.z    =  matrix[ 6];
	owner->frontdir.x =  matrix[ 8];
	owner->frontdir.y =  matrix[ 9];
	owner->frontdir.z =  matrix[10];

	const shortint2 HandP = GetHAndPFromVector(owner->frontdir);
	owner->heading = HandP.x;
}



void CScriptMoveType::CheckNotify()
{
	if (scriptNotify) {
		scriptNotify = 0;

		if (luaRules && luaRules->MoveCtrlNotify(owner, scriptNotify)) {
			//! deletes <this>
			owner->DisableScriptMoveType();
		}
	}
}


void CScriptMoveType::Update()
{
	if (useRotVel) {
		rot += rotVel;
		CalcDirections();
	}

	owner->speed = vel;
	if (extrapolate) {
		if (drag != 0.0f) {
			vel *= (1.0f - drag); // quadratic drag does not work well here
		}
		if (useRelVel) {
			const float3 rVel = (owner->frontdir *  relVel.z) +
			                    (owner->updir    *  relVel.y) +
			                    (owner->rightdir * -relVel.x); // x is left
			owner->speed += rVel;
		}
		vel.y        += mapInfo->map.gravity * gravityFactor;
		owner->speed += (wind.GetCurrentWind() * windFactor);
		owner->pos   += owner->speed;
	}

	if (trackGround) {
		const float gndMin = ground->GetHeightReal(owner->pos.x, owner->pos.z) + groundOffset;

		if (owner->pos.y <= gndMin) {
			owner->pos.y = gndMin;
			owner->speed.y = 0.0f;
			if (gndStop) {
				owner->speed.y = 0.0f;
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
		TrackSlope();
	}

	owner->UpdateMidPos();

	// don't need the rest if the pos hasn't changed
	if (oldPos == owner->pos) {
		CheckNotify();
		return;
	}

	oldPos = owner->pos;

	if (!noBlocking) {
		owner->Block();
	}

	if (owner->unitDef->leaveTracks && leaveTracks &&
	    (lastTrackUpdate < (gs->frameNum - 7)) &&
	    ((owner->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView)) {
		lastTrackUpdate = gs->frameNum;
		groundDecals->UnitMoved(owner);
	}

	CheckNotify();
}


void CScriptMoveType::CheckLimits()
{
	if (owner->pos.x < mins.x) { owner->pos.x = mins.x; owner->speed.x = 0.0f; }
	if (owner->pos.x > maxs.x) { owner->pos.x = maxs.x; owner->speed.x = 0.0f; }
	if (owner->pos.y < mins.y) { owner->pos.y = mins.y; owner->speed.y = 0.0f; }
	if (owner->pos.y > maxs.y) { owner->pos.y = maxs.y; owner->speed.y = 0.0f; }
	if (owner->pos.z < mins.z) { owner->pos.z = mins.z; owner->speed.z = 0.0f; }
	if (owner->pos.z > maxs.z) { owner->pos.z = maxs.z; owner->speed.z = 0.0f; }
}


void CScriptMoveType::SetPhysics(const float3& pos,
                                 const float3& vel,
                                 const float3& rot)
{
	owner->pos = pos;
	owner->speed = vel;
	SetRotation(rot);
	return;
}


void CScriptMoveType::SetPosition(const float3& pos)
{
	owner->pos = pos;
	return;
}


void CScriptMoveType::SetVelocity(const float3& _vel)
{
	vel = _vel;
	return;
}


void CScriptMoveType::SetRelativeVelocity(const float3& _relVel)
{
	relVel = _relVel;
	useRelVel = ((relVel.x != 0.0f) || (relVel.y != 0.0f) || (relVel.z != 0.0f));
	return;
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
	return;
}


void CScriptMoveType::SetRotationOffset(const float3& rotOff)
{
	rotOffset = rotOff;
}


void CScriptMoveType::SetHeading(short heading)
{
	owner->heading = heading;
	if (!trackSlope) {
		owner->frontdir = GetVectorFromHeading(heading);
		owner->updir = UpVector;
		owner->rightdir = owner->frontdir.cross(UpVector);
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

void CScriptMoveType::TrackSlope()
{
	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->updir = ground->GetSmoothNormal(owner->pos.x, owner->pos.z);
	owner->rightdir = owner->frontdir.cross(owner->updir);
	owner->rightdir.Normalize();
	owner->frontdir = owner->updir.cross(owner->rightdir);
}
