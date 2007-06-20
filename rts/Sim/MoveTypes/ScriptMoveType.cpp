
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
	CR_MEMBER(vel),
	CR_MEMBER(relVel),
	CR_MEMBER(useRelVel),
	CR_MEMBER(rot),
	CR_MEMBER(rotVel),
	CR_MEMBER(useRotVel),
	CR_MEMBER(trackGround),
	CR_MEMBER(hasDecal),
	CR_MEMBER(isBuilding),
	CR_MEMBER(isBlocking),
	CR_MEMBER(gravityFactor),
	CR_MEMBER(windFactor),
	CR_MEMBER(trackSlope),
	CR_MEMBER(trackGround),
	CR_MEMBER(groundOffset),
	CR_MEMBER(mins),
	CR_MEMBER(maxs),
	CR_MEMBER(noBlocking),
	CR_MEMBER(shotStop),
	CR_MEMBER(slopeStop),
	CR_MEMBER(collideStop),
	CR_MEMBER(rotOffset),
	CR_MEMBER(lastTrackUpdate),
	CR_MEMBER(oldPos),
	CR_MEMBER(oldSlowUpdatePos)
));


CScriptMoveType::CScriptMoveType(CUnit* owner)
: CMoveType(owner),
	tag(0),
  extrapolate(true),
  vel(0.0f, 0.0f, 0.0f),
  relVel(0.0f, 0.0f, 0.0f),
  useRelVel(false),
  rot(0.0f, 0.0f, 0.0f),
  rotVel(0.0f, 0.0f, 0.0f),
  rotOffset(0.0f, 0.0f, 0.0f),
  useRotVel(false),
  hasDecal(false),
  isBuilding(false),
  gravityFactor(0.0f),
  windFactor(0.0f),
  trackSlope(false),
  trackGround(false),
  groundOffset(0.0f),
  noBlocking(false),
  shotStop(false),
  slopeStop(false),
  collideStop(false),
  mins(-1.0e9f, -1.0e9f, -1.0e9f),
  maxs(+1.0e9f, +1.0e9f, +1.0e9f),
  lastTrackUpdate(0),
	oldPos(owner?owner->pos:float3(0,0,0)),
	oldSlowUpdatePos(oldPos)
{
	useHeading = false; // use the transformation matrix instead of heading

	if (owner) {
		const UnitDef* unitDef = owner->unitDef;
		isBuilding = (unitDef->type == "Building");
		isBlocking = (!unitDef->canKamikaze || !isBuilding);
	} else {
		isBuilding = false;
		isBlocking = false;
	}
}


CScriptMoveType::~CScriptMoveType(void)
{
	if (isBlocking && noBlocking) {
		owner->Block();
	}
}


inline void CScriptMoveType::CalcMidPos()
{
	const float3& pos = owner->pos;
	const float3& pr = owner->relMidPos;
	const float3& df = owner->frontdir;
	const float3& du = owner->updir;
	const float3& dr = owner->rightdir;
	owner->midPos = pos + (df * pr.z) + (du * pr.y) + (dr * pr.x);
}


inline void CScriptMoveType::CalcDirections()
{
	CMatrix44f matrix;
	matrix.Translate(-rotOffset); // this doesn't work, Rotate is not a full rotate
	matrix.RotateY(-rot.y);
	matrix.RotateX(-rot.x);
	matrix.RotateZ(-rot.z);
	matrix.Translate(rotOffset);
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


void CScriptMoveType::SlowUpdate()
{
	const float3& pos = owner->pos;

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
	if (useRotVel) {
		rot += rotVel;
		CalcDirections();
	}

	owner->speed = vel;
	if (extrapolate) {
		if (useRelVel) {
			const float3 rVel = (owner->frontdir *  relVel.z) +
			                    (owner->updir    *  relVel.y) +
			                    (owner->rightdir * -relVel.x); // x is left
			owner->speed += rVel;
		}
		vel.y        += gs->gravity * gravityFactor;
		owner->speed += (wind.curWind * windFactor);
		owner->pos   += owner->speed;
	}

	if (trackGround) {
		const float gndMin =
			ground->GetHeight2(owner->pos.x, owner->pos.z) + groundOffset;
		if (owner->pos.y <= gndMin) {
			owner->pos.y = gndMin;
			owner->speed.y = 0.0f;
		}
	}

	// positional clamps
	CheckLimits();
//	owner->pos.CheckInBounds();

	if (trackSlope) {
		TrackSlope();
	}

	CalcMidPos();

	// don't need the rest if the pos hasn't changed
	if (oldPos == owner->pos) {
		return;
	}
	oldPos = owner->pos;

	if (isBlocking && !noBlocking) {
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


void CScriptMoveType::CheckLimits()
{
	if (owner->pos.x < mins.x) {
		owner->pos.x = mins.x;
		owner->speed.x = 0.0f;
	}
	if (owner->pos.x > maxs.x) {
		owner->pos.x = maxs.x;
		owner->speed.x = 0.0f;
	}
	if (owner->pos.y < mins.y) {
		owner->pos.y = mins.y;
		owner->speed.y = 0.0f;
	}
	if (owner->pos.y > maxs.y) {
		owner->pos.y = maxs.y;
		owner->speed.y = 0.0f;
	}
	if (owner->pos.z < mins.z) {
		owner->pos.z = mins.z;
		owner->speed.z = 0.0f;
	}
	if (owner->pos.z > maxs.z) {
		owner->pos.z = maxs.z;
		owner->speed.z = 0.0f;
	}
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
	noBlocking = state;
	if (noBlocking) {
		owner->UnBlock();
	} else {
		if (isBlocking) {
			owner->Block();
		}
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


