
#include "StdAfx.h"
#include "ScriptMoveType.h"
#include "Map/Ground.h"
#include "Rendering/GroundDecalHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "mmgr.h"

CR_BIND_DERIVED(CScriptMoveType, CMoveType, (NULL));

CR_REG_METADATA(CScriptMoveType, (
	CR_MEMBER(tag),
	CR_MEMBER(extrapolate),
	CR_MEMBER(trackGround),
	CR_MEMBER(hasDecal),
	CR_MEMBER(isBuilding),
	CR_MEMBER(isBlocking),
	CR_MEMBER(gravityFactor),
	CR_MEMBER(windFactor),
	CR_MEMBER(trackGround),
	CR_MEMBER(groundOffset),
	CR_MEMBER(mins),
	CR_MEMBER(maxs),
	CR_MEMBER(shotStop),
	CR_MEMBER(slopeStop),
	CR_MEMBER(collideStop),
	CR_MEMBER(rotationOffset),
	CR_MEMBER(lastTrackUpdate),
	CR_MEMBER(oldPos),
	CR_MEMBER(oldSlowUpdatePos)
));


CScriptMoveType::CScriptMoveType(CUnit* owner)
: CMoveType(owner),
	tag(0),
  extrapolate(true),
  hasDecal(false),
  isBuilding(false),
  gravityFactor(0.0f),
  windFactor(0.0f),
  trackGround(false),
  groundOffset(0.0f),
  shotStop(false),
  slopeStop(false),
  collideStop(false),
  mins(-1.0e9, -1.0e9, -1.0e9),
  maxs(+1.0e9, +1.0e9, +1.0e9),
  rotationOffset(0.0f, 0.0f, 0.0f),
  lastTrackUpdate(0),
	oldPos(owner->pos),
	oldSlowUpdatePos(oldPos)
{
	useHeading = false; // use the transformation matrix instead of heading

	const UnitDef* unitDef = owner->unitDef;
	isBuilding = unitDef->type == "Building";
	isBlocking = !unitDef->canKamikaze || isBuilding;
}


CScriptMoveType::~CScriptMoveType(void)
{
}


inline void CScriptMoveType::SetMidPos()
{
	const float3& pos = owner->pos;
	const float3& pr = owner->relMidPos;
	const float3& df = owner->frontdir;
	const float3& du = owner->updir;
	const float3& dr = owner->rightdir;
	owner->midPos = pos + (df * pr.z) + (du * pr.y) + (dr * pr.x);
}


void CScriptMoveType::SlowUpdate()
{
	float3& pos = owner->pos;
	float3& vel = owner->speed;

	// make sure the unit is in the map
//	pos.CheckInBounds();

	// don't need the rest if the pos hasn't changed
	if (pos == oldSlowUpdatePos) {
		return;
	}
	oldSlowUpdatePos = pos;

	const int newmapSquare = ground->GetSquare(pos);
	if (newmapSquare != owner->mapSquare){
		owner->mapSquare = newmapSquare;

		loshandler->MoveUnit(owner, false);
		if (owner->hasRadarCapacity) {
			radarhandler->MoveUnit(owner);
		}
	}
	qf->MovedUnit(owner);
};


void CScriptMoveType::Update()
{
	float3& pos = owner->pos;
	float3& vel = owner->speed;

	if (extrapolate) {
		vel.y += gs->gravity * gravityFactor;
		pos += (wind.curWind * windFactor);
		pos += vel;
	}

	if (trackGround) {
		pos.y = ground->GetHeight2(pos.x, pos.z) + groundOffset;
		vel.y = 0.0f;
	}

	// positional clamps
	pos.x = max(mins.x, min(maxs.x, pos.x));
	pos.y = max(mins.y, min(maxs.y, pos.y));
	pos.z = max(mins.z, min(maxs.z, pos.z));
//	pos.CheckInBounds();

	if (trackSlope) {
		TrackSlope();
	}

	SetMidPos();

	// don't need the rest if the pos hasn't changed
	if (oldPos == pos) {
		return;
	}
	oldPos = pos;

	if (isBlocking) {
		owner->UnBlock();
		owner->Block();
	}

	if (groundDecals && owner->unitDef->leaveTracks &&
	    lastTrackUpdate < (gs->frameNum - 7) &&
	    ((owner->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView)) {
		lastTrackUpdate = gs->frameNum;
		groundDecals->UnitMoved(owner);
	}
};


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


void CScriptMoveType::SetVelocity(const float3& vel)
{
	owner->speed = vel;
	return;
}


void CScriptMoveType::SetRotation(const float3& rot)
{
	CMatrix44f matrix;
	matrix.Translate(rotationOffset); // this doesn't work, Rotate is not a full rotate
	matrix.RotateY(rot.y);
	matrix.RotateX(rot.x);
	matrix.RotateZ(rot.z);
	matrix.Translate(-rotationOffset);
	owner->rightdir.x = matrix[ 0];
	owner->rightdir.y = matrix[ 1];
	owner->rightdir.z = matrix[ 2];
	owner->updir.x    = matrix[ 4];
	owner->updir.y    = matrix[ 5];
	owner->updir.z    = matrix[ 6];
	owner->frontdir.x = matrix[ 8];
	owner->frontdir.y = matrix[ 9];
	owner->frontdir.z = matrix[10];

	const shortint2 HandP = GetHAndPFromVector(owner->frontdir);
	owner->heading = HandP.x;
}


void CScriptMoveType::SetRotationOffset(const float3& rotOff)
{
	rotationOffset = rotOff;
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


void CScriptMoveType::TrackSlope()
{
	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->updir = ground->GetNormal(owner->pos.x, owner->pos.z);
	owner->rightdir = owner->frontdir.cross(owner->updir);
	owner->rightdir.Normalize();
	owner->frontdir = owner->updir.cross(owner->rightdir);
}

