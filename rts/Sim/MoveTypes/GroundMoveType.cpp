/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GroundMoveType.h"
#include "MoveDefHandler.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FastMath.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/type2.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/HsiehHash.h"

#if 1
#include "Rendering/IPathDrawer.h"
#define DEBUG_DRAWING_ENABLED ((gs->cheatEnabled || gu->spectatingFullView) && pathDrawer->IsEnabled())
#else
#define DEBUG_DRAWING_ENABLED false
#endif

#define LOG_SECTION_GMT "GroundMoveType"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_GMT)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_GMT


// speeds near (MAX_UNIT_SPEED * 1e1) elmos / frame can be caused by explosion impulses
// CUnitHandler removes units with speeds > MAX_UNIT_SPEED as soon as they exit the map,
// so the assertion can be less strict
#define ASSERT_SANE_OWNER_SPEED(v) assert(v.SqLength() < (MAX_UNIT_SPEED * MAX_UNIT_SPEED * 1e2));

// magic number to reduce damage taken from collisions
// between a very heavy and a very light CSolidObject
#define COLLISION_DAMAGE_MULT    0.02f

#define MAX_IDLING_SLOWUPDATES     16
#define IGNORE_OBSTACLES            0
#define WAIT_FOR_PATH               1
#define MODEL_TURN_INERTIA          1

#define UNIT_CMD_QUE_SIZE(u) (u->commandAI->commandQue.size())
// Not using IsMoveCommand on purpose, as the following is changing the effective goalRadius
#define UNIT_HAS_MOVE_CMD(u) (u->commandAI->commandQue.empty() || u->commandAI->commandQue[0].GetID() == CMD_MOVE || u->commandAI->commandQue[0].GetID() == CMD_FIGHT)

#define WAYPOINT_RADIUS (1.25f * SQUARE_SIZE)

#define MAXREVERSESPEED_MEMBER_IDX 7

#define MEMBER_CHARPTR_HASH(memberName) HsiehHash(memberName, strlen(memberName),     0)
#define MEMBER_LITERAL_HASH(memberName) HsiehHash(memberName, sizeof(memberName) - 1, 0)



CR_BIND_DERIVED(CGroundMoveType, AMoveType, (nullptr))
CR_REG_METADATA(CGroundMoveType, (
	CR_IGNORED(pathController),

	CR_MEMBER(currWayPoint),
	CR_MEMBER(nextWayPoint),

	CR_MEMBER(waypointDir),
	CR_MEMBER(flatFrontDir),
	CR_MEMBER(lastAvoidanceDir),
	CR_MEMBER(mainHeadingPos),
	CR_MEMBER(skidRotVector),

	CR_MEMBER(turnRate),
	CR_MEMBER(turnSpeed),
	CR_MEMBER(turnAccel),
	CR_MEMBER(accRate),
	CR_MEMBER(decRate),
	CR_MEMBER(myGravity),

	CR_MEMBER(maxReverseDist),
	CR_MEMBER(minReverseAngle),
	CR_MEMBER(maxReverseSpeed),
	CR_MEMBER(sqSkidSpeedMult),

	CR_MEMBER(wantedSpeed),
	CR_MEMBER(currentSpeed),
	CR_MEMBER(deltaSpeed),

	CR_MEMBER(currWayPointDist),
	CR_MEMBER(prevWayPointDist),
	CR_MEMBER(goalRadius),
	CR_MEMBER(ownerRadius),
	CR_MEMBER(extraRadius),

	CR_MEMBER(skidRotSpeed),
	CR_MEMBER(skidRotAccel),

	CR_MEMBER(pathID),
	CR_MEMBER(nextObstacleAvoidanceFrame),

	CR_MEMBER(numIdlingUpdates),
	CR_MEMBER(numIdlingSlowUpdates),

	CR_MEMBER(wantedHeading),
	CR_MEMBER(minScriptChangeHeading),

	CR_MEMBER(atGoal),
	CR_MEMBER(atEndOfPath),
	CR_MEMBER(wantRepath),

	CR_MEMBER(reversing),
	CR_MEMBER(idling),
	CR_MEMBER(pushResistant),
	CR_MEMBER(canReverse),
	CR_MEMBER(useMainHeading),
	CR_MEMBER(useRawMovement),

	CR_POSTLOAD(PostLoad)
))



static CGroundMoveType::MemberData gmtMemberData = {
	{{
		std::pair<unsigned int,  bool*>{MEMBER_LITERAL_HASH(       "atGoal"), nullptr},
		std::pair<unsigned int,  bool*>{MEMBER_LITERAL_HASH(  "atEndOfPath"), nullptr},
		std::pair<unsigned int,  bool*>{MEMBER_LITERAL_HASH("pushResistant"), nullptr},
	}},
	{{
		std::pair<unsigned int, short*>{MEMBER_LITERAL_HASH("minScriptChangeHeading"), nullptr},
	}},
	{{
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(       "turnRate"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(      "turnAccel"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(        "accRate"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(        "decRate"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH(      "myGravity"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH( "maxReverseDist"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH("minReverseAngle"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH("maxReverseSpeed"), nullptr},
		std::pair<unsigned int, float*>{MEMBER_LITERAL_HASH("sqSkidSpeedMult"), nullptr},
	}},
};




namespace SAT {
	static float CalcSeparatingDist(
		const float3& axis,
		const float3& zdir,
		const float3& xdir,
		const float3& sepv,
		const float2& size
	) {
		const float axisDist = math::fabs(axis.dot(sepv)         );
		const float xdirDist = math::fabs(axis.dot(xdir) * size.x);
		const float zdirDist = math::fabs(axis.dot(zdir) * size.y);

		return (axisDist - zdirDist - xdirDist);
	}

	static bool HaveSeparatingAxis(
		const CSolidObject* collider,
		const CSolidObject* collidee,
		const MoveDef* colliderMD,
		const MoveDef* collideeMD,
		const float3& separationVec
	) {
		// const float2 colliderSize = (colliderMD != nullptr)? colliderMD->GetFootPrint(0.5f * SQUARE_SIZE): collider->GetFootPrint(0.5f * SQUARE_SIZE);
		const float2 colliderSize =                          colliderMD->GetFootPrint(0.5f * SQUARE_SIZE)                                            ;
		const float2 collideeSize = (collideeMD != nullptr)? collideeMD->GetFootPrint(0.5f * SQUARE_SIZE): collidee->GetFootPrint(0.5f * SQUARE_SIZE);

		// true if no overlap on at least one axis
		bool haveAxis = false;

		haveAxis = haveAxis || (CalcSeparatingDist(collider->frontdir,  collidee->frontdir, collidee->rightdir,  separationVec, collideeSize) > colliderSize.y);
		haveAxis = haveAxis || (CalcSeparatingDist(collider->rightdir,  collidee->frontdir, collidee->rightdir,  separationVec, collideeSize) > colliderSize.x);
		haveAxis = haveAxis || (CalcSeparatingDist(collidee->frontdir,  collider->frontdir, collider->rightdir,  separationVec, colliderSize) > collideeSize.y);
		haveAxis = haveAxis || (CalcSeparatingDist(collidee->rightdir,  collider->frontdir, collider->rightdir,  separationVec, colliderSize) > collideeSize.x);
		return haveAxis;
	}
};


static bool CheckCollisionExclSAT(
	const float4& separationVec,
	const CSolidObject* collider = nullptr,
	const CSolidObject* collidee = nullptr,
	const MoveDef* colliderMD = nullptr,
	const MoveDef* collideeMD = nullptr
) {
	return ((separationVec.SqLength() - separationVec.w) <= 0.01f);
}

static bool CheckCollisionInclSAT(
	const float4& separationVec,
	const CSolidObject* collider = nullptr,
	const CSolidObject* collidee = nullptr,
	const MoveDef* colliderMD = nullptr,
	const MoveDef* collideeMD = nullptr
) {
	assert(collider   != nullptr);
	assert(collidee   != nullptr);
	assert(colliderMD != nullptr);

	return (CheckCollisionExclSAT(separationVec) && !SAT::HaveSeparatingAxis(collider, collidee, colliderMD, collideeMD, separationVec));
}



static void HandleUnitCollisionsAux(
	const CUnit* collider,
	const CUnit* collidee,
	CGroundMoveType* gmtCollider,
	CGroundMoveType* gmtCollidee
) {
	if (!collider->IsMoving() || gmtCollider->progressState != AMoveType::Active)
		return;

	// if collidee shares our goal position and is no longer
	// moving along its path, trigger Arrived() to kill long
	// pushing contests
	//
	// check the progress-states so collisions with units which
	// failed to reach goalPos for whatever reason do not count
	// (or those that still have orders)
	//
	// CFactory applies random jitter to otherwise equal goal
	// positions of at most TWOPI elmos, use half as threshold
	// (simply bail if distance between collider and collidee
	// goal-positions exceeds PI)
	if (!gmtCollider->IsAtGoalPos(gmtCollidee->goalPos, math::PI))
		return;

	switch (gmtCollidee->progressState) {
		case AMoveType::Done: {
			if (collidee->IsMoving() || UNIT_CMD_QUE_SIZE(collidee) != 0)
				return;

			gmtCollider->TriggerCallArrived();
		} break;

		case AMoveType::Active: {
			// collider and collidee are both actively moving and share the same goal position
			// (i.e. a traffic jam) so ignore current waypoint and go directly to the next one
			// or just make collider give up if already within footprint radius
			if (gmtCollidee->GetCurrWayPoint() == gmtCollider->GetNextWayPoint()) {
				gmtCollider->TriggerSkipWayPoint();
				return;
			}

			if (!gmtCollider->IsAtGoalPos(collider->pos, gmtCollider->GetOwnerRadius()))
				return;

			gmtCollider->TriggerCallArrived();
		} break;

		default: {
		} break;
	}
}



static float3 CalcSpeedVectorInclGravity(const CUnit* owner, const CGroundMoveType* mt, float hAcc, float vAcc) {
	float3 newSpeedVector;

	// NOTE:
	//   the drag terms ensure speed-vector always decays if
	//   wantedSpeed and deltaSpeed are 0 (needed because we
	//   do not call GetDragAccelerationVect while a unit is
	//   moving under its own power)
	const float dragCoeff = mix(0.99f, 0.9999f, owner->IsInAir());
	const float slipCoeff = mix(0.95f, 0.9999f, owner->IsInAir());

	const float3& ownerPos = owner->pos;
	const float3& ownerSpd = owner->speed;

	// use terrain-tangent vector because it does not
	// depend on UnitDef::upright (unlike o->frontdir)
	const float3& gndNormVec = mt->GetGroundNormal(ownerPos);
	const float3  gndTangVec = gndNormVec.cross(owner->rightdir);
	const float3& flatFrontDir = mt->GetFlatFrontDir();

	const int dirSign = Sign(flatFrontDir.dot(ownerSpd));
	const int revSign = Sign(int(!mt->IsReversing()));

	const float3 horSpeed = ownerSpd * XZVector * dirSign * revSign;
	const float3 verSpeed = UpVector * ownerSpd.y;

	if (!modInfo.allowHoverUnitStrafing || owner->moveDef->speedModClass != MoveDef::Hover) {
		const float3 accelVec = (gndTangVec * hAcc) + (UpVector * vAcc);
		const float3 speedVec = (horSpeed + verSpeed) + accelVec;

		newSpeedVector += (flatFrontDir * speedVec.dot(flatFrontDir)) * dragCoeff;
		newSpeedVector += (    UpVector * speedVec.dot(    UpVector));
	} else {
		// TODO: also apply to non-hovercraft on low-gravity maps?
		newSpeedVector += (              gndTangVec * (  std::max(0.0f,   ownerSpd.dot(gndTangVec) + hAcc * 1.0f))) * dragCoeff;
		newSpeedVector += (   horSpeed - gndTangVec * (/*std::max(0.0f,*/ ownerSpd.dot(gndTangVec) - hAcc * 0.0f )) * slipCoeff;
		newSpeedVector += (UpVector * UpVector.dot(ownerSpd + UpVector * vAcc));
	}

	// never drop below terrain while following tangent
	// (SPEED must be adjusted so that it does not keep
	// building up when the unit is on the ground or is
	// within one frame of hitting it)
	const float oldGroundHeight = mt->GetGroundHeight(ownerPos                 );
	const float newGroundHeight = mt->GetGroundHeight(ownerPos + newSpeedVector);

	if ((ownerPos.y + newSpeedVector.y) <= newGroundHeight)
		newSpeedVector.y = std::min(newGroundHeight - ownerPos.y, math::fabs(newGroundHeight - oldGroundHeight));

	return newSpeedVector;
}

static float3 CalcSpeedVectorExclGravity(const CUnit* owner, const CGroundMoveType* mt, float hAcc, float vAcc) {
	// LuaSyncedCtrl::SetUnitVelocity directly assigns
	// to owner->speed which gets overridden below, so
	// need to calculate hSpeedScale from it (not from
	// currentSpeed) directly
	return (owner->frontdir * (owner->speed.w * Sign(int(!mt->IsReversing())) + hAcc));
}



static constexpr decltype(&CheckCollisionInclSAT) checkCollisionFuncs[] = {
	CheckCollisionExclSAT,
	CheckCollisionInclSAT,
};

static constexpr decltype(&CalcSpeedVectorInclGravity) calcSpeedVectorFuncs[] = {
	CalcSpeedVectorExclGravity,
	CalcSpeedVectorInclGravity,
};




CGroundMoveType::CGroundMoveType(CUnit* owner):
	AMoveType(owner),
	pathController(owner),

	currWayPoint(ZeroVector),
	nextWayPoint(ZeroVector),

	flatFrontDir(FwdVector),
	lastAvoidanceDir(ZeroVector),
	mainHeadingPos(ZeroVector),
	skidRotVector(UpVector),

	wantedHeading(0),
	minScriptChangeHeading((SPRING_CIRCLE_DIVS - 1) >> 1),

	pushResistant((owner != nullptr) && owner->unitDef->pushResistant),
	canReverse((owner != nullptr) && (owner->unitDef->rSpeed > 0.0f))
{
	// creg
	if (owner == nullptr)
		return;

	const UnitDef* ud = owner->unitDef;
	const MoveDef* md = owner->moveDef;

	assert(ud != nullptr);
	assert(md != nullptr);

	// maxSpeed is set in AMoveType's ctor
	maxReverseSpeed = ud->rSpeed / GAME_SPEED;

	// SPRING_CIRCLE_DIVS is 65536, but turnRate can be at most
	// 32767 since it is converted to (signed) shorts in places
	turnRate = Clamp(ud->turnRate, 1.0f, SPRING_CIRCLE_DIVS * 0.5f - 1.0f);
	turnAccel = turnRate * mix(0.333f, 0.033f, md->speedModClass == MoveDef::Ship);

	accRate = std::max(0.01f, ud->maxAcc);
	decRate = std::max(0.01f, ud->maxDec);

	// unit-gravity must always be negative
	myGravity = mix(-math::fabs(ud->myGravity), mapInfo->map.gravity, ud->myGravity == 0.0f);

	ownerRadius = md->CalcFootPrintMinExteriorRadius();
}

CGroundMoveType::~CGroundMoveType()
{
	if (pathID == 0)
		return;

	pathManager->DeletePath(pathID);
}

void CGroundMoveType::PostLoad()
{
	pathController = GMTDefaultPathController(owner);

	// HACK: re-initialize path after load
	if (pathID == 0)
		return;

	pathID = pathManager->RequestPath(owner, owner->moveDef, owner->pos, goalPos, goalRadius + extraRadius, true);
}

bool CGroundMoveType::OwnerMoved(const short oldHeading, const float3& posDif, const float3& cmpEps) {
	if (posDif.equals(ZeroVector, cmpEps)) {
		// note: the float3::== test is not exact, so even if this
		// evaluates to true the unit might still have an epsilon
		// speed vector --> nullify it to prevent apparent visual
		// micro-stuttering (speed is used to extrapolate drawPos)
		owner->SetVelocityAndSpeed(ZeroVector);

		// negative y-coordinates indicate temporary waypoints that
		// only exist while we are still waiting for the pathfinder
		// (so we want to avoid being considered "idle", since that
		// will cause our path to be re-requested and again give us
		// a temporary waypoint, etc.)
		// NOTE: this is only relevant for QTPFS (at present)
		// if the unit is just turning in-place over several frames
		// (eg. to maneuver around an obstacle), do not consider it
		// as "idling"
		idling = true;
		idling &= (currWayPoint.y != -1.0f && nextWayPoint.y != -1.0f);
		idling &= (std::abs(owner->heading - oldHeading) < turnRate);

		return false;
	}

	// note: HandleObjectCollisions() may have negated the position set
	// by UpdateOwnerPos() (so that owner->pos is again equal to oldPos)
	// note: the idling-check can only succeed if we are oriented in the
	// direction of our waypoint, which compensates for the fact distance
	// decreases much less quickly when moving orthogonal to <waypointDir>
	oldPos = owner->pos;

	const float3 ffd = flatFrontDir * posDif.SqLength() * 0.5f;
	const float3 wpd = waypointDir * ((int(!reversing) * 2) - 1);

	// too many false negatives: speed is unreliable if stuck behind an obstacle
	//   idling = (Square(owner->speed.w) < (accRate * accRate));
	//   idling &= (Square(currWayPointDist - prevWayPointDist) <= (accRate * accRate));
	// too many false positives: waypoint-distance delta and speed vary too much
	//   idling = (Square(currWayPointDist - prevWayPointDist) < Square(owner->speed.w));
	// too many false positives: many slow units cannot even manage 1 elmo/frame
	//   idling = (Square(currWayPointDist - prevWayPointDist) < 1.0f);
	idling = true;
	idling &= (math::fabs(posDif.y) < math::fabs(cmpEps.y * owner->pos.y));
	idling &= (Square(currWayPointDist - prevWayPointDist) < ffd.dot(wpd));
	idling &= (posDif.SqLength() < Square(owner->speed.w * 0.5f));

	return true;
}

bool CGroundMoveType::Update()
{
	ASSERT_SYNCED(owner->pos);

	// do nothing at all if we are inside a transport
	if (owner->GetTransporter() != nullptr)
		return false;

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING, owner->IsSkidding() || OnSlope(1.0f));

	if (owner->IsSkidding()) {
		UpdateSkid();
		return false;
	}

	ASSERT_SYNCED(owner->pos);

	// set drop height when we start to drop
	if (owner->IsFalling()) {
		UpdateControlledDrop();
		return false;
	}

	ASSERT_SYNCED(owner->pos);

	const short heading = owner->heading;

	// these must be executed even when stunned (so
	// units do not get buried by restoring terrain)
	UpdateOwnerAccelAndHeading();
	UpdateOwnerPos(owner->speed, calcSpeedVectorFuncs[modInfo.allowGroundUnitGravity](owner, this, deltaSpeed, myGravity));
	HandleObjectCollisions();
	AdjustPosToWaterLine();

	ASSERT_SANE_OWNER_SPEED(owner->speed);

	// <dif> is normally equal to owner->speed (if no collisions)
	// we need more precision (less tolerance) in the y-dimension
	// for all-terrain units that are slowed down a lot on cliffs
	return (OwnerMoved(heading, owner->pos - oldPos, float3(float3::cmp_eps(), float3::cmp_eps() * 1e-2f, float3::cmp_eps())));
}

void CGroundMoveType::UpdateOwnerAccelAndHeading()
{
	if (owner->IsStunned() || owner->beingBuilt) {
		ChangeSpeed(0.0f, false);
		return;
	}

	// either follow user control input or pathfinder
	// waypoints; change speed and heading as required
	if (owner->UnderFirstPersonControl()) {
		UpdateDirectControl();
	} else {
		FollowPath();
	}
}

void CGroundMoveType::SlowUpdate()
{
	if (owner->GetTransporter() != nullptr) {
		if (progressState == Active)
			StopEngine(false);

	} else {
		if (progressState == Active) {
			if (pathID != 0) {
				if (idling) {
					numIdlingSlowUpdates = std::min(MAX_IDLING_SLOWUPDATES, int(numIdlingSlowUpdates + 1));
				} else {
					numIdlingSlowUpdates = std::max(0, int(numIdlingSlowUpdates - 1));
				}

				if (numIdlingUpdates > (SPRING_MAX_HEADING / turnRate)) {
					// case A: we have a path but are not moving
					LOG_L(L_DEBUG, "[%s] unit %i has pathID %i but %i ETA failures", __func__, owner->id, pathID, numIdlingUpdates);

					if (numIdlingSlowUpdates < MAX_IDLING_SLOWUPDATES) {
						ReRequestPath(true);
					} else {
						// unit probably ended up on a non-traversable
						// square, or got stuck in a non-moving crowd
						Fail(false);
					}
				}
			} else {
				// case B: we want to be moving but don't have a path
				LOG_L(L_DEBUG, "[%s] unit %i has no path", __func__, owner->id);
				ReRequestPath(true);
			}

			if (wantRepath)
				ReRequestPath(true);
		}

		// move us into the map, and update <oldPos>
		// to prevent any extreme changes in <speed>
		if (!owner->IsFlying() && !owner->pos.IsInBounds())
			owner->Move(oldPos = owner->pos.cClampInBounds(), false);
	}

	AMoveType::SlowUpdate();
}


void CGroundMoveType::StartMovingRaw(const float3 moveGoalPos, float moveGoalRadius) {
	const float deltaRadius = std::max(0.0f, ownerRadius - moveGoalRadius);

	goalPos = moveGoalPos * XZVector;
	goalRadius = moveGoalRadius;
	extraRadius = deltaRadius * (1 - owner->moveDef->TestMoveSquare(nullptr, moveGoalPos, ZeroVector, true, true));

	currWayPoint = goalPos;
	nextWayPoint = goalPos;

	atGoal = (moveGoalPos.SqDistance2D(owner->pos) < Square(goalRadius + extraRadius));
	atEndOfPath = false;

	useMainHeading = false;
	useRawMovement = true;

	progressState = Active;

	numIdlingUpdates = 0;
	numIdlingSlowUpdates = 0;

	currWayPointDist = 0.0f;
	prevWayPointDist = 0.0f;
}

void CGroundMoveType::StartMoving(float3 moveGoalPos, float moveGoalRadius) {
	// add the footprint radius if moving onto goalPos would cause it to overlap impassable squares
	// (otherwise repeated coldet push-jittering can ensue if allowTerrainCollision is not disabled)
	// not needed if goalRadius actually exceeds ownerRadius, e.g. for builders
	const float deltaRadius = std::max(0.0f, ownerRadius - moveGoalRadius);

	// set the new goal
	goalPos = moveGoalPos * XZVector;
	goalRadius = moveGoalRadius;
	extraRadius = deltaRadius * (1 - owner->moveDef->TestMoveSquare(nullptr, moveGoalPos, ZeroVector, true, true));

	atGoal = (moveGoalPos.SqDistance2D(owner->pos) < Square(goalRadius + extraRadius));
	atEndOfPath = false;

	useMainHeading = false;
	useRawMovement = false;

	progressState = Active;

	numIdlingUpdates = 0;
	numIdlingSlowUpdates = 0;

	currWayPointDist = 0.0f;
	prevWayPointDist = 0.0f;

	LOG_L(L_DEBUG, "[%s] starting engine for unit %i", __func__, owner->id);

	if (atGoal)
		return;

	// silently free previous path if unit already had one
	//
	// units passing intermediate waypoints will TYPICALLY not cause any
	// script->{Start,Stop}Moving calls now (even when turnInPlace=true)
	// unless they come to a full stop first
	ReRequestPath(true);

	if (owner->team == gu->myTeam)
		Channels::General->PlayRandomSample(owner->unitDef->sounds.activate, owner);
}

void CGroundMoveType::StopMoving(bool callScript, bool hardStop, bool cancelRaw) {
	LOG_L(L_DEBUG, "[%s] stopping engine for unit %i", __func__, owner->id);

	if (!atGoal)
		goalPos = (currWayPoint = Here());

	// this gets called under a variety of conditions (see MobileCAI)
	// the most common case is a CMD_STOP being issued which means no
	// StartMoving-->StartEngine will follow
	StopEngine(callScript, hardStop);

	// force WantToStop to return true when useRawMovement is enabled
	atEndOfPath |= useRawMovement;
	// only a new StartMoving call can normally reset this
	useRawMovement &= (!cancelRaw);
	useMainHeading = false;

	progressState = Done;
}


bool CGroundMoveType::FollowPath()
{
	bool wantReverse = false;

	if (WantToStop()) {
		currWayPoint.y = -1.0f;
		nextWayPoint.y = -1.0f;

		SetMainHeading();
		ChangeSpeed(0.0f, false);
	} else {
		ASSERT_SYNCED(currWayPoint);
		ASSERT_SYNCED(nextWayPoint);
		ASSERT_SYNCED(owner->pos);

		const float3& opos = owner->pos;
		const float3& ovel = owner->speed;
		const float3&  ffd = flatFrontDir;
		const float3&  cwp = currWayPoint;

		prevWayPointDist = currWayPointDist;
		currWayPointDist = currWayPoint.distance2D(opos);

		{
			// NOTE:
			//   uses owner->pos instead of currWayPoint (ie. not the same as atEndOfPath)
			//
			//   if our first command is a build-order, then goal-radius is set to our build-range
			//   and we cannot increase tolerance safely (otherwise the unit might stop when still
			//   outside its range and fail to start construction)
			//
			//   units moving faster than <minGoalDist> elmos per frame might overshoot their goal
			//   the last two atGoal conditions will just cause flatFrontDir to be selected as the
			//   "wanted" direction when this happens
			const float curGoalDistSq = (opos - goalPos).SqLength2D();
			const float minGoalDistSq = (UNIT_HAS_MOVE_CMD(owner))?
				Square((goalRadius + extraRadius) * (numIdlingSlowUpdates + 1)):
				Square((goalRadius + extraRadius)                             );
			const float spdGoalDistSq = Square(currentSpeed * 1.05f);

			atGoal |= (curGoalDistSq <= minGoalDistSq);
			atGoal |= ((curGoalDistSq <= spdGoalDistSq) && !reversing && (ffd.dot(goalPos - opos) > 0.0f && ffd.dot(goalPos - (opos + ovel)) <= 0.0f));
			atGoal |= ((curGoalDistSq <= spdGoalDistSq) &&  reversing && (ffd.dot(goalPos - opos) < 0.0f && ffd.dot(goalPos - (opos + ovel)) >= 0.0f));
		}

		if (!atGoal) {
			numIdlingUpdates -= ((numIdlingUpdates >                  0) * (1 - idling));
			numIdlingUpdates += ((numIdlingUpdates < SPRING_MAX_HEADING) *      idling );
		}

		// atEndOfPath never becomes true when useRawMovement, except via StopMoving
		if (!atEndOfPath && !useRawMovement) {
			SetNextWayPoint();
		} else {
			if (atGoal)
				Arrived(false);
			else
				ReRequestPath(false);
		}


		// set direction to waypoint AFTER requesting it; should not be a null-vector
		// do not compare y-components since these usually differ and only x&z matter
		float3 waypointVec;
		// float3 wpProjDists;

		if (!epscmp(cwp.x, opos.x, float3::cmp_eps()) || !epscmp(cwp.z, opos.z, float3::cmp_eps())) {
			waypointVec = (cwp - opos) * XZVector;
			waypointDir = waypointVec / waypointVec.Length();
			// wpProjDists = {math::fabs(waypointVec.dot(ffd)), 1.0f, math::fabs(waypointDir.dot(ffd))};
		}

		ASSERT_SYNCED(waypointVec);
		ASSERT_SYNCED(waypointDir);

		wantReverse = WantReverse(waypointDir, ffd);

		// apply obstacle avoidance (steering), prevent unit from chasing its own tail if already at goal
		const float3  rawWantedDir = waypointDir * Sign(int(!wantReverse));
		const float3& modWantedDir = GetObstacleAvoidanceDir(mix(ffd, rawWantedDir, !atGoal));
		// const float3& modWantedDir = GetObstacleAvoidanceDir(mix(ffd, rawWantedDir, (!atGoal) && (wpProjDists.x > wpProjDists.y || wpProjDists.z < 0.995f)));

		ChangeHeading(GetHeadingFromVector(modWantedDir.x, modWantedDir.z));
		ChangeSpeed(maxWantedSpeed, wantReverse);
	}

	pathManager->UpdatePath(owner, pathID);
	return wantReverse;
}

void CGroundMoveType::ChangeSpeed(float newWantedSpeed, bool wantReverse, bool fpsMode)
{
	// round low speeds to zero
	if ((wantedSpeed = newWantedSpeed) <= 0.0f && currentSpeed < 0.01f) {
		currentSpeed = 0.0f;
		deltaSpeed = 0.0f;
		return;
	}

	// first calculate the "unrestricted" speed and acceleration
	float targetSpeed = mix(maxSpeed, maxReverseSpeed, wantReverse);

	#if (WAIT_FOR_PATH == 1)
	// don't move until we have an actual path, trying to hide queuing
	// lag is too dangerous since units can blindly drive into objects,
	// cliffs, etc. (requires the QTPFS idle-check in Update)
	if (currWayPoint.y == -1.0f && nextWayPoint.y == -1.0f) {
		targetSpeed = 0.0f;
	} else
	#endif
	{
		if (wantedSpeed > 0.0f) {
			const UnitDef* ud = owner->unitDef;
			const MoveDef* md = owner->moveDef;

			// the pathfinders do NOT check the entire footprint to determine
			// passability wrt. terrain (only wrt. structures), so we look at
			// the center square ONLY for our current speedmod
			float groundSpeedMod = CMoveMath::GetPosSpeedMod(*md, owner->pos, flatFrontDir);

			// the pathfinders don't check the speedmod of the square our unit is currently on
			// so if we got stuck on a nonpassable square and can't move try to see if we're
			// trying to release ourselves towards a passable square
			if (groundSpeedMod == 0.0f)
				groundSpeedMod = CMoveMath::GetPosSpeedMod(*md, owner->pos + flatFrontDir * SQUARE_SIZE, flatFrontDir);

			const float curGoalDistSq = (owner->pos - goalPos).SqLength2D();
			const float minGoalDistSq = Square(BrakingDistance(currentSpeed, decRate));

			const float3& waypointDifFwd = waypointDir;
			const float3  waypointDifRev = -waypointDifFwd;

			const float3& waypointDif = mix(waypointDifFwd, waypointDifRev, reversing);
			const short turnDeltaHeading = owner->heading - GetHeadingFromVector(waypointDif.x, waypointDif.z);

			const bool startBraking = (UNIT_CMD_QUE_SIZE(owner) <= 1 && curGoalDistSq <= minGoalDistSq && !fpsMode);

			if (!fpsMode && turnDeltaHeading != 0) {
				// only auto-adjust speed for turns when not in FPS mode
				const float reqTurnAngle = math::fabs(180.0f * short(owner->heading - wantedHeading) / SPRING_MAX_HEADING);
				const float maxTurnAngle = (turnRate / SPRING_CIRCLE_DIVS) * 360.0f;

				const float turnMaxSpeed = mix(maxSpeed, maxReverseSpeed, reversing);
				      float turnModSpeed = turnMaxSpeed;

				if (reqTurnAngle != 0.0f)
					turnModSpeed *= Clamp(maxTurnAngle / reqTurnAngle, 0.1f, 1.0f);

				if (waypointDir.SqLength() > 0.1f) {
					if (!ud->turnInPlace) {
						// never let speed drop below TIPSL, but limit TIPSL itself to turnMaxSpeed
						targetSpeed = Clamp(turnModSpeed, std::min(ud->turnInPlaceSpeedLimit, turnMaxSpeed), turnMaxSpeed);
					} else {
						targetSpeed = mix(targetSpeed, turnModSpeed, reqTurnAngle > ud->turnInPlaceAngleLimit);
					}
				}

				if (atEndOfPath) {
					// at this point, Update() will no longer call SetNextWayPoint()
					// and we must slow down to prevent entering an infinite circle
					// base ftt on maximum turning speed
					const float absTurnSpeed = turnRate;
					const float framesToTurn = SPRING_CIRCLE_DIVS / absTurnSpeed;

					targetSpeed = std::min(targetSpeed, (currWayPointDist * math::PI) / framesToTurn);
				}
			}

			// now apply the terrain and command restrictions
			// NOTE:
			//   if wantedSpeed > targetSpeed, the unit will
			//   not accelerate to speed > targetSpeed unless
			//   its actual max{Reverse}Speed is also changed
			//
			//   raise wantedSpeed iff the terrain-modifier is
			//   larger than 1 (so units still get their speed
			//   bonus correctly), otherwise leave it untouched
			//
			//   disallow changing speed (except to zero) without
			//   a path if not in FPS mode (FIXME: legacy PFS can
			//   return path when none should exist, mantis3720)
			wantedSpeed *= std::max(groundSpeedMod, 1.0f);
			targetSpeed *= groundSpeedMod;
			targetSpeed *= (1 - startBraking);
			targetSpeed *= ((1 - WantToStop()) || fpsMode);
			targetSpeed = std::min(targetSpeed, wantedSpeed);
		} else {
			targetSpeed = 0.0f;
		}
	}

	deltaSpeed = pathController.GetDeltaSpeed(
		pathID,
		targetSpeed,
		currentSpeed,
		accRate,
		decRate,
		wantReverse,
		reversing
	);
}

/*
 * Changes the heading of the owner.
 * FIXME near-duplicate of HoverAirMoveType::UpdateHeading
 */
void CGroundMoveType::ChangeHeading(short newHeading) {
	if (owner->IsFlying())
		return;
	if (owner->GetTransporter() != nullptr)
		return;

	#if (MODEL_TURN_INERTIA == 0)
	const short rawDeltaHeading = pathController.GetDeltaHeading(pathID, (wantedHeading = newHeading), owner->heading, turnRate);
	#else
	// model rotational inertia (more realistic for ships)
	const short rawDeltaHeading = pathController.GetDeltaHeading(pathID, (wantedHeading = newHeading), owner->heading, turnRate, turnAccel, BrakingDistance(turnSpeed, turnAccel), &turnSpeed);
	#endif
	const short absDeltaHeading = rawDeltaHeading * Sign(rawDeltaHeading);

	if (absDeltaHeading >= minScriptChangeHeading)
		owner->script->ChangeHeading(rawDeltaHeading);

	owner->AddHeading(rawDeltaHeading, !owner->upright && owner->IsOnGround(), owner->IsInAir());

	flatFrontDir = (owner->frontdir * XZVector).Normalize();
}




bool CGroundMoveType::CanApplyImpulse(const float3& impulse)
{
	// NOTE: ships must be able to receive impulse too (for collision handling)
	if (owner->beingBuilt)
		return false;
	// will be applied to transporter instead
	if (owner->GetTransporter() != nullptr)
		return false;
	if (impulse.SqLength() <= 0.01f)
		return false;

	UseHeading(false);

	skidRotSpeed = 0.0f;
	skidRotAccel = 0.0f;

	float3 newSpeed = owner->speed + impulse;
	float3 skidDir = owner->frontdir;

	// NOTE:
	//   we no longer delay the skidding-state until owner has "accumulated" an
	//   arbitrary hardcoded amount of impulse (possibly across several frames),
	//   but enter it on any vector that causes speed to become misaligned with
	//   frontdir
	// TODO
	//   there should probably be a configurable minimum-impulse below which the
	//   unit does not react at all but also does NOT "store" the impulse like a
	//   small-charge capacitor, or a more physically-based approach (|N*m*g*cf|
	//   > |impulse/dt|) could be used
	//
	const bool startSkidding = StartSkidding(newSpeed, skidDir);
	const bool startFlying = StartFlying(newSpeed, CGround::GetNormal(owner->pos.x, owner->pos.z));

	if (startSkidding)
		owner->script->StartSkidding(newSpeed);

	if (newSpeed.SqLength2D() >= 0.01f)
		skidDir = newSpeed.Normalize2D();

	skidRotVector = skidDir.cross(UpVector) * startSkidding;
	skidRotAccel = ((gsRNG.NextFloat() - 0.5f) * 0.04f) * startFlying;

	owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING * (startSkidding | startFlying));
	owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING * startFlying);

	// indicate we want to react to the impulse
	return true;
}

void CGroundMoveType::UpdateSkid()
{
	ASSERT_SYNCED(owner->midPos);

	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	const float minCollSpeed = owner->unitDef->minCollisionSpeed;
	const float groundHeight = GetGroundHeight(pos);
	const float negAltitude = groundHeight - pos.y;

	owner->SetVelocity(spd + owner->GetDragAccelerationVec(float4(mapInfo->atmosphere.fluidDensity, mapInfo->water.fluidDensity, 1.0f, 0.01f)));

	if (owner->IsFlying()) {
		const float collImpactSpeed = pos.IsInBounds()?
			-spd.dot(CGround::GetNormal(pos.x, pos.z)):
			-spd.dot(UpVector);
		const float impactDamageMul = collImpactSpeed * owner->mass * COLLISION_DAMAGE_MULT;

		if (negAltitude > 0.0f) {
			// ground impact, stop flying
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			owner->Move(UpVector * negAltitude, true);

			// deal ground impact damage
			// TODO:
			//   bouncing behaves too much like a rubber-ball,
			//   most impact energy needs to go into the ground
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > minCollSpeed && minCollSpeed >= 0.0f)
				owner->DoDamage(DamageArray(impactDamageMul), ZeroVector, nullptr, -CSolidObject::DAMAGE_COLLISION_GROUND, -1);

			skidRotSpeed = 0.0f;
			// skidRotAccel = 0.0f;
		} else {
			owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity));
		}
	} else {
		// *assume* this means the unit is still on the ground
		// (Lua gadgetry can interfere with our "physics" logic)
		float skidRotSpd = 0.0f;

		const bool onSlope = OnSlope(0.0f);
		const bool stopSkid = StopSkidding(spd, owner->frontdir);

		if (!onSlope && stopSkid) {
			skidRotSpd = math::floor(skidRotSpeed + skidRotAccel + 0.5f);
			skidRotAccel = (skidRotSpd - skidRotSpeed) * 0.5f;
			skidRotAccel *= math::DEG_TO_RAD;

			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
			owner->script->StopSkidding();

			UseHeading(true);
			// update wanted-heading after coming to a stop
			ChangeHeading(owner->heading);
		} else {
			constexpr float speedReduction = 0.35f;

			// number of frames until rotational speed would drop to 0
			const float speedScale = owner->SetSpeed(spd);
			const float rotRemTime = std::max(1.0f, speedScale / speedReduction);

			if (onSlope) {
				const float3& normalVector = CGround::GetNormal(pos.x, pos.z);
				const float3 normalForce = normalVector * normalVector.dot(UpVector * mapInfo->map.gravity);
				const float3 newForce = UpVector * mapInfo->map.gravity - normalForce;

				owner->SetVelocity(spd + newForce);
				owner->SetVelocity(spd * (1.0f - (0.1f * normalVector.y)));
			} else {
				// RHS is clamped to 0..1
				owner->SetVelocity(spd * (1.0f - std::min(1.0f, speedReduction / speedScale)));
			}

			skidRotSpd = math::floor(skidRotSpeed + skidRotAccel * (rotRemTime - 1.0f) + 0.5f);
			skidRotAccel = (skidRotSpd - skidRotSpeed) / rotRemTime;
			skidRotAccel *= math::DEG_TO_RAD;

			if (math::floor(skidRotSpeed) != math::floor(skidRotSpeed + skidRotAccel)) {
				skidRotSpeed = 0.0f;
				skidRotAccel = 0.0f;
			}
		}

		if (negAltitude < (spd.y + mapInfo->map.gravity)) {
			owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity));

			// flying requires skidding and relies on CalcSkidRot
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);

			UseHeading(false);
		} else if (negAltitude > spd.y) {
			// LHS is always negative, so this becomes true when the
			// unit is falling back down and will impact the ground
			// in one frame
			const float3& gndNormal = (pos.IsInBounds())? CGround::GetNormal(pos.x, pos.z): UpVector;
			const float projSpeed = spd.dot(gndNormal);

			if (projSpeed > 0.0f) {
				// not possible without lateral movement
				owner->SetVelocity(spd * 0.95f);
			} else {
				owner->SetVelocity(spd + (gndNormal * (math::fabs(projSpeed) + 0.1f)));
				owner->SetVelocity(spd * 0.8f);
			}
		}
	}

	// finally update speed.w
	owner->SetSpeed(spd);
	// translate before rotate, match terrain normal if not in air
	owner->Move(spd, true);
	owner->UpdateDirVectors(!owner->upright && owner->IsOnGround(), owner->IsInAir());

	if (owner->IsSkidding()) {
		CalcSkidRot();
		CheckCollisionSkid();
	} else {
		// do this here since ::Update returns early if it calls us
		HandleObjectCollisions();
	}

	AdjustPosToWaterLine();

	// always update <oldPos> here so that <speed> does not make
	// extreme jumps when the unit transitions from skidding back
	// to non-skidding
	oldPos = owner->pos;

	ASSERT_SANE_OWNER_SPEED(spd);
	ASSERT_SYNCED(owner->midPos);
}

void CGroundMoveType::UpdateControlledDrop()
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;
	const float3  acc = UpVector * std::min(mapInfo->map.gravity * owner->fallSpeed, 0.0f);
	const float   alt = GetGroundHeight(pos) - pos.y;

	owner->SetVelocity(spd + acc);
	owner->SetVelocity(spd + owner->GetDragAccelerationVec(float4(mapInfo->atmosphere.fluidDensity, mapInfo->water.fluidDensity, 1.0f, 0.1f)));
	owner->SetSpeed(spd);
	owner->Move(spd, true);

	if (alt <= 0.0f)
		return;

	// ground impact, stop parachute animation
	owner->Move(UpVector * alt, true);
	owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FALLING);
	owner->script->Landed();
}

void CGroundMoveType::CheckCollisionSkid()
{
	CUnit* collider = owner;

	// NOTE:
	//   the QuadField::Get* functions check o->midPos,
	//   but the quad(s) that objects are stored in are
	//   derived from o->pos (!)
	const float3& pos = collider->pos;

	const float colliderMinCollSpeed = collider->unitDef->minCollisionSpeed;
	      float collideeMinCollSpeed = 0.0f;

	// copy on purpose, since the below can call Lua
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, collider->radius);
	quadField.GetFeaturesExact(qfQuery, pos, collider->radius);

	for (CUnit* collidee: *qfQuery.units) {
		if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;

		const UnitDef* collideeUD = collidee->unitDef;

		const float sqDist = (pos - collidee->pos).SqLength();
		const float totRad = collider->radius + collidee->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f))
			continue;

		const float3 collSeparationDir = (pos - collidee->pos).SafeNormalize();

		if (collideeUD->IsImmobileUnit()) {
			const float collImpactSpeed = -collider->speed.dot(collSeparationDir);
			const float impactDamageMul = std::min(collImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);

			if (collImpactSpeed <= 0.0f)
				continue;

			// damage the collider, no added impulse
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > colliderMinCollSpeed && colliderMinCollSpeed >= 0.0f)
				collider->DoDamage(DamageArray(impactDamageMul), ZeroVector, collidee, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

			// damage the (static) collidee based on collider's mass, no added impulse
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > (collideeMinCollSpeed = collideeUD->minCollisionSpeed) && collideeMinCollSpeed >= 0.0f)
				collidee->DoDamage(DamageArray(impactDamageMul), ZeroVector, collider, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

			collider->Move(collSeparationDir * collImpactSpeed, true);
			collider->SetVelocity(collider->speed + ((collSeparationDir * collImpactSpeed) * 1.8f));
		} else {
			assert(collider->mass > 0.0f && collidee->mass > 0.0f);

			// don't conserve momentum (impact speed is halved, so impulses are too)
			// --> collisions are neither truly elastic nor truly inelastic to prevent
			// the simulation from blowing up from impulses applied to tight groups of
			// units
			const float collImpactSpeed = (collidee->speed - collider->speed).dot(collSeparationDir) * 0.5f;
			const float colliderRelMass = (collider->mass / (collider->mass + collidee->mass));
			const float colliderRelImpactSpeed = collImpactSpeed * (1.0f - colliderRelMass);
			const float collideeRelImpactSpeed = collImpactSpeed * (       colliderRelMass);

			const float  colliderImpactDmgMult = std::min(colliderRelImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
			const float  collideeImpactDmgMult = std::min(collideeRelImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
			const float3 colliderImpactImpulse = collSeparationDir * colliderRelImpactSpeed;
			const float3 collideeImpactImpulse = collSeparationDir * collideeRelImpactSpeed;

			if (collImpactSpeed <= 0.0f)
				continue;

			// damage the collider
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > colliderMinCollSpeed && colliderMinCollSpeed >= 0.0f)
				collider->DoDamage(DamageArray(colliderImpactDmgMult), collSeparationDir * colliderImpactDmgMult, collidee, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

			// damage the collidee
			if (modInfo.allowUnitCollisionDamage && collImpactSpeed > (collideeMinCollSpeed = collideeUD->minCollisionSpeed) && collideeMinCollSpeed >= 0.0f)
				collidee->DoDamage(DamageArray(collideeImpactDmgMult), collSeparationDir * -collideeImpactDmgMult, collider, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

			collider->Move( colliderImpactImpulse, true);
			collidee->Move(-collideeImpactImpulse, true);
			collider->SetVelocity        (collider->speed + colliderImpactImpulse);
			collidee->SetVelocityAndSpeed(collidee->speed - collideeImpactImpulse);
		}
	}

	for (CFeature* collidee: *qfQuery.features) {
		if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;

		const float sqDist = (pos - collidee->pos).SqLength();
		const float totRad = collider->radius + collidee->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f))
			continue;

		const float3 collSeparationDir = (pos - collidee->pos).SafeNormalize();
		const float collImpactSpeed = -collider->speed.dot(collSeparationDir);
		const float impactDamageMul = std::min(collImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
		const float3 impactImpulse = collSeparationDir * collImpactSpeed;

		if (collImpactSpeed <= 0.0f)
			continue;

		// damage the collider, no added impulse (!)
		// the collidee feature can not be passed along to the collider as attacker
		// yet, keep symmetry and do not pass collider along to the collidee either
		if (modInfo.allowUnitCollisionDamage && collImpactSpeed > colliderMinCollSpeed && colliderMinCollSpeed >= 0.0f)
			collider->DoDamage(DamageArray(impactDamageMul), ZeroVector, nullptr, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

		// damage the collidee feature based on collider's mass
		collidee->DoDamage(DamageArray(impactDamageMul), -impactImpulse, nullptr, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

		collider->Move(impactImpulse, true);
		collider->SetVelocity(collider->speed + (impactImpulse * 1.8f));
	}

	// finally update speed.w
	collider->SetSpeed(collider->speed);

	ASSERT_SANE_OWNER_SPEED(collider->speed);
}

void CGroundMoveType::CalcSkidRot()
{
	skidRotSpeed += skidRotAccel;
	skidRotSpeed *= 0.999f;
	skidRotAccel *= 0.95f;

	const float angle = (skidRotSpeed / GAME_SPEED) * math::TWOPI;
	const float cosp = math::cos(angle);
	const float sinp = math::sin(angle);

	float3 f1 = skidRotVector * skidRotVector.dot(owner->frontdir);
	float3 f2 = owner->frontdir - f1;

	float3 r1 = skidRotVector * skidRotVector.dot(owner->rightdir);
	float3 r2 = owner->rightdir - r1;

	float3 u1 = skidRotVector * skidRotVector.dot(owner->updir);
	float3 u2 = owner->updir - u1;

	f2 = f2 * cosp + f2.cross(skidRotVector) * sinp;
	r2 = r2 * cosp + r2.cross(skidRotVector) * sinp;
	u2 = u2 * cosp + u2.cross(skidRotVector) * sinp;

	owner->frontdir = f1 + f2;
	owner->rightdir = r1 + r2;
	owner->updir    = u1 + u2;

	owner->UpdateMidAndAimPos();
}




/*
 * Dynamic obstacle avoidance, helps the unit to
 * follow the path even when it's not perfect.
 */
float3 CGroundMoveType::GetObstacleAvoidanceDir(const float3& desiredDir) {
	#if (IGNORE_OBSTACLES == 1)
	return desiredDir;
	#endif

	// obstacle-avoidance only needs to run if the unit wants to move
	if (WantToStop())
		return flatFrontDir;

	// Speed-optimizer. Reduces the times this system is run.
	if (gs->frameNum < nextObstacleAvoidanceFrame)
		return lastAvoidanceDir;

	float3 avoidanceVec = ZeroVector;
	float3 avoidanceDir = desiredDir;

	lastAvoidanceDir = desiredDir;
	nextObstacleAvoidanceFrame = gs->frameNum + 1;

	CUnit* avoider = owner;

	// const UnitDef* avoiderUD = avoider->unitDef;
	const MoveDef* avoiderMD = avoider->moveDef;

	// degenerate case: if facing anti-parallel to desired direction,
	// do not actively avoid obstacles since that can interfere with
	// normal waypoint steering (if the final avoidanceDir demands a
	// turn in the opposite direction of desiredDir)
	if (avoider->frontdir.dot(desiredDir) < 0.0f)
		return lastAvoidanceDir;

	static constexpr float AVOIDER_DIR_WEIGHT = 1.0f;
	static constexpr float DESIRED_DIR_WEIGHT = 0.5f;
	static constexpr float LAST_DIR_MIX_ALPHA = 0.7f;
	static const     float MAX_AVOIDEE_COSINE = math::cosf(120.0f * math::DEG_TO_RAD);

	// now we do the obstacle avoidance proper
	// avoider always uses its never-rotated MoveDef footprint
	// note: should increase radius for smaller turnAccel values
	const float avoidanceRadius = std::max(currentSpeed, 1.0f) * (avoider->radius * 2.0f);
	const float avoiderRadius = avoiderMD->CalcFootPrintMinExteriorRadius();

	QuadFieldQuery qfQuery;
	quadField.GetSolidsExact(qfQuery, avoider->pos, avoidanceRadius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

	for (const CSolidObject* avoidee: *qfQuery.solids) {
		const MoveDef* avoideeMD = avoidee->moveDef;
		const UnitDef* avoideeUD = dynamic_cast<const UnitDef*>(avoidee->GetDef());

		// cases in which there is no need to avoid this obstacle
		if (avoidee == owner)
			continue;
		// do not avoid statics (it interferes too much with PFS)
		if (avoideeMD == nullptr)
			continue;
		// ignore aircraft (or flying ground units)
		if (avoidee->IsInAir() || avoidee->IsFlying())
			continue;
		if (CMoveMath::IsNonBlocking(*avoiderMD, avoidee, avoider))
			continue;
		if (!CMoveMath::CrushResistant(*avoiderMD, avoidee))
			continue;

		const bool avoideeMobile  = (avoideeMD != nullptr);
		const bool avoideeMovable = (avoideeUD != nullptr && !static_cast<const CUnit*>(avoidee)->moveType->IsPushResistant());

		const float3 avoideeVector = (avoider->pos + avoider->speed) - (avoidee->pos + avoidee->speed);

		// use the avoidee's MoveDef footprint as radius if it is mobile
		// use the avoidee's Unit (not UnitDef) footprint as radius otherwise
		const float avoideeRadius = avoideeMobile?
			avoideeMD->CalcFootPrintMinExteriorRadius():
			avoidee->CalcFootPrintMinExteriorRadius();
		const float avoidanceRadiusSum = avoiderRadius + avoideeRadius;
		const float avoidanceMassSum = avoider->mass + avoidee->mass;
		const float avoideeMassScale = avoideeMobile? (avoidee->mass / avoidanceMassSum): 1.0f;
		const float avoideeDistSq = avoideeVector.SqLength();
		const float avoideeDist   = math::sqrt(avoideeDistSq) + 0.01f;

		// do not bother steering around idling MOBILE objects
		// (since collision handling will just push them aside)
		if (avoideeMobile && avoideeMovable) {
			if (!avoiderMD->avoidMobilesOnPath || (!avoidee->IsMoving() && avoidee->allyteam == avoider->allyteam))
				continue;
		}

		// ignore objects that are more than this many degrees off-center from us
		// NOTE:
		//   if MAX_AVOIDEE_COSINE is too small, then this condition can be true
		//   one frame and false the next (after avoider has turned) causing the
		//   avoidance vector to oscillate --> units with turnInPlace = true will
		//   slow to a crawl as a result
		if (avoider->frontdir.dot(-(avoideeVector / avoideeDist)) < MAX_AVOIDEE_COSINE)
			continue;

		if (avoideeDistSq >= Square(std::max(currentSpeed, 1.0f) * GAME_SPEED + avoidanceRadiusSum))
			continue;
		if (avoideeDistSq >= avoider->pos.SqDistance2D(goalPos))
			continue;

		// if object and unit in relative motion are closing in on one another
		// (or not yet fully apart), then the object is on the path of the unit
		// and they are not collided
		if (DEBUG_DRAWING_ENABLED) {
			if (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end())
				geometricObjects->AddLine(avoider->pos + (UpVector * 20.0f), avoidee->pos + (UpVector * 20.0f), 3, 1, 4);
		}

		float avoiderTurnSign = -Sign(avoidee->pos.dot(avoider->rightdir) - avoider->pos.dot(avoider->rightdir));
		float avoideeTurnSign = -Sign(avoider->pos.dot(avoidee->rightdir) - avoidee->pos.dot(avoidee->rightdir));

		// for mobile units, avoidance-response is modulated by angle
		// between avoidee's and avoider's frontdir such that maximal
		// avoidance occurs when they are anti-parallel
		const float avoidanceCosAngle = Clamp(avoider->frontdir.dot(avoidee->frontdir), -1.0f, 1.0f);
		const float avoidanceResponse = (1.0f - avoidanceCosAngle * int(avoideeMobile)) + 0.1f;
		const float avoidanceFallOff  = (1.0f - std::min(1.0f, avoideeDist / (5.0f * avoidanceRadiusSum)));

		// if parties are anti-parallel, it is always more efficient for
		// both to turn in the same local-space direction (either R/R or
		// L/L depending on relative object positions) but there exists
		// a range of orientations for which the signs are not equal
		//
		// (this is also true for the parallel situation, but there the
		// degeneracy only occurs when one of the parties is behind the
		// other and can be ignored)
		if (avoidanceCosAngle < 0.0f)
			avoiderTurnSign = std::max(avoiderTurnSign, avoideeTurnSign);

		avoidanceDir = avoider->rightdir * AVOIDER_DIR_WEIGHT * avoiderTurnSign;
		avoidanceVec += (avoidanceDir * avoidanceResponse * avoidanceFallOff * avoideeMassScale);
	}


	// use a weighted combination of the desired- and the avoidance-directions
	// also linearly smooth it using the vector calculated the previous frame
	avoidanceDir = (mix(desiredDir, avoidanceVec, DESIRED_DIR_WEIGHT)).SafeNormalize();
	avoidanceDir = (mix(avoidanceDir, lastAvoidanceDir, LAST_DIR_MIX_ALPHA)).SafeNormalize();

	if (DEBUG_DRAWING_ENABLED) {
		if (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end()) {
			const float3 p0 = owner->pos + (    UpVector * 20.0f);
			const float3 p1 =         p0 + (avoidanceVec * 40.0f);
			const float3 p2 =         p0 + (avoidanceDir * 40.0f);

			const int avFigGroupID = geometricObjects->AddLine(p0, p1, 8.0f, 1, 4);
			const int adFigGroupID = geometricObjects->AddLine(p0, p2, 8.0f, 1, 4);

			geometricObjects->SetColor(avFigGroupID, 1, 0.3f, 0.3f, 0.6f);
			geometricObjects->SetColor(adFigGroupID, 1, 0.3f, 0.3f, 0.6f);
		}
	}

	return (lastAvoidanceDir = avoidanceDir);
}



#if 0
// Calculates an aproximation of the physical 2D-distance between given two objects.
// Old, no longer used since all separation tests are based on FOOTPRINT_RADIUS now.
float CGroundMoveType::Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal)
{
	// calculate the distance in (x,z) depending
	// on the shape of the object footprints
	float dist2D = 0.0f;

	const float xs = ((object1->xsize + object2->xsize) * (SQUARE_SIZE >> 1));
	const float zs = ((object1->zsize + object2->zsize) * (SQUARE_SIZE >> 1));

	if (object1->xsize == object1->zsize || object2->xsize == object2->zsize) {
		// use xsize as a cylindrical radius.
		const float3 distVec = object1->midPos - object2->midPos;

		dist2D = distVec.Length2D() - xs + 2.0f * marginal;
	} else {
		// Pytagorean sum of the x and z distance.
		float3 distVec;

		const float xdiff = math::fabs(object1->midPos.x - object2->midPos.x);
		const float zdiff = math::fabs(object1->midPos.z - object2->midPos.z);

		distVec.x = xdiff - xs + 2.0f * marginal;
		distVec.z = zdiff - zs + 2.0f * marginal;

		if (distVec.x > 0.0f && distVec.z > 0.0f) {
			dist2D = distVec.Length2D();
		} else if (distVec.x < 0.0f && distVec.z < 0.0f) {
			dist2D = -distVec.Length2D();
		} else if (distVec.x > 0.0f) {
			dist2D = distVec.x;
		} else {
			dist2D = distVec.z;
		}
	}

	return dist2D;
}
#endif

// Creates a path to the goal.
unsigned int CGroundMoveType::GetNewPath()
{
	unsigned int newPathID = 0;

	if (useRawMovement)
		return newPathID;
	// avoid frivolous requests if called from outside StartMoving*()
	if ((owner->pos - goalPos).SqLength2D() <= Square(goalRadius + extraRadius))
		return newPathID;

	if ((newPathID = pathManager->RequestPath(owner, owner->moveDef, owner->pos, goalPos, goalRadius + extraRadius, true)) != 0) {
		atGoal = false;
		atEndOfPath = false;

		currWayPoint = pathManager->NextWayPoint(owner, newPathID, 0,   owner->pos, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);
		nextWayPoint = pathManager->NextWayPoint(owner, newPathID, 0, currWayPoint, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);

		pathController.SetRealGoalPosition(newPathID, goalPos);
		pathController.SetTempGoalPosition(newPathID, currWayPoint);
	} else {
		Fail(false);
	}

	return newPathID;
}

void CGroundMoveType::ReRequestPath(bool forceRequest) {
	if (forceRequest) {
		StopEngine(false);
		StartEngine(false);
		wantRepath = false;
		return;
	}

	wantRepath = true;
}



bool CGroundMoveType::CanSetNextWayPoint() {
	assert(!useRawMovement);

	if (pathID == 0)
		return false;
	if (!pathController.AllowSetTempGoalPosition(pathID, nextWayPoint))
		return false;


	if (currWayPoint.y != -1.0f && nextWayPoint.y != -1.0f) {
		const float3& pos = owner->pos;
		      float3& cwp = (float3&) currWayPoint;
		      float3& nwp = (float3&) nextWayPoint;

		if (pathManager->PathUpdated(pathID)) {
			// path changed while we were following it (eg. due
			// to terrain deformation) in between two waypoints
			// but still has the same ID; in this case (which is
			// specific to QTPFS) we don't go through GetNewPath
			//
			cwp = pathManager->NextWayPoint(owner, pathID, 0, pos, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);
			nwp = pathManager->NextWayPoint(owner, pathID, 0, cwp, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);
		}

		if (DEBUG_DRAWING_ENABLED) {
			if (selectedUnitsHandler.selectedUnits.find(owner->id) != selectedUnitsHandler.selectedUnits.end()) {
				// plot the vectors to {curr, next}WayPoint
				const int cwpFigGroupID = geometricObjects->AddLine(pos + (UpVector * 20.0f), cwp + (UpVector * (pos.y + 20.0f)), 8.0f, 1, 4);
				const int nwpFigGroupID = geometricObjects->AddLine(pos + (UpVector * 20.0f), nwp + (UpVector * (pos.y + 20.0f)), 8.0f, 1, 4);

				geometricObjects->SetColor(cwpFigGroupID, 1, 0.3f, 0.3f, 0.6f);
				geometricObjects->SetColor(nwpFigGroupID, 1, 0.3f, 0.3f, 0.6f);
			}
		}

		// perform a turn-radius check: if the waypoint lies outside
		// our turning circle, do not skip since we can steer toward
		// this waypoint and pass it without slowing down
		// note that the DIAMETER of the turning circle is calculated
		// to prevent sine-like "snaking" trajectories; units capable
		// of instant turns *and* high speeds also need special care
		const int dirSign = Sign(int(!reversing));

		#if (MODEL_TURN_INERTIA == 0)
		const float absTurnSpeed = turnRate;
		#else
		// base ftt on current turning speed
		const float absTurnSpeed = std::max(0.0001f, math::fabs(turnSpeed));
		#endif
		const float framesToTurn = SPRING_CIRCLE_DIVS / absTurnSpeed;

		const float turnRadius = std::max((currentSpeed * framesToTurn) * math::INVPI2, currentSpeed * 1.05f);
		const float waypointDot = Clamp(waypointDir.dot(flatFrontDir * dirSign), -1.0f, 1.0f);

		#if 1
		// wp outside turning circle
		if (currWayPointDist > (turnRadius * 2.0f))
			return false;
		// wp inside but ~straight ahead and not reached within one tick
		if (currWayPointDist > std::max(SQUARE_SIZE * 1.0f, currentSpeed * 1.05f) && waypointDot >= 0.995f)
			return false;

		#else

		if ((currWayPointDist > std::max(turnRadius * 2.0f, 1.0f * SQUARE_SIZE)) && (waypointDot >= 0.0f))
			return false;

		if ((currWayPointDist > std::max(turnRadius * 1.0f, 1.0f * SQUARE_SIZE)) && (waypointDot <  0.0f))
			return false;

		if (math::acosf(waypointDot) < ((turnRate / SPRING_CIRCLE_DIVS) * math::TWOPI))
			return false;
		#endif

		{
			// check the rectangle between pos and cwp for obstacles
			// if still further than SS elmos from waypoint, disallow skipping
			// note: can somehow cause units to move in circles near obstacles
			// (mantis3718) if rectangle is too generous in size
			const bool rangeTest = owner->moveDef->TestMoveSquareRange(owner, float3::min(cwp, pos), float3::max(cwp, pos), owner->speed, true, true, true);
			const bool allowSkip = ((cwp - pos).SqLength() <= Square(SQUARE_SIZE));

			// CanSetNextWayPoint may return true if (allowSkip || rangeTest)
			if (!allowSkip && !rangeTest)
				return false;
		}

		{
			const float curGoalDistSq = (currWayPoint - goalPos).SqLength2D();
			const float minGoalDistSq = (UNIT_HAS_MOVE_CMD(owner))?
				Square((goalRadius + extraRadius) * (numIdlingSlowUpdates + 1)):
				Square((goalRadius + extraRadius)                             );

			// trigger Arrived on the next Update (only if we have non-temporary waypoints)
			// note:
			//   coldet can (very rarely) interfere with this, causing it to remain false
			//   a unit would then keep moving along its final waypoint-direction forever
			//   if atGoal, so we require waypointDir to always be updated in FollowPath
			//   (checking curr == next is not perfect, becomes true a waypoint too early)
			//
			// atEndOfPath |= (currWayPoint == nextWayPoint);
			atEndOfPath |= (curGoalDistSq <= minGoalDistSq);
		}

		if (atEndOfPath) {
			currWayPoint = goalPos;
			nextWayPoint = goalPos;
			return false;
		}
	}

	return true;
}

void CGroundMoveType::SetNextWayPoint()
{
	assert(!useRawMovement);

	if (CanSetNextWayPoint()) {
		pathController.SetTempGoalPosition(pathID, nextWayPoint);

		// NOTE:
		//   pathfinder implementation should ensure waypoints are not equal
		//   waypoint consumption radius has to at least equal current speed
		currWayPoint = nextWayPoint;
		nextWayPoint = pathManager->NextWayPoint(owner, pathID, 0, currWayPoint, std::max(WAYPOINT_RADIUS, currentSpeed * 1.05f), true);
	}

	if (nextWayPoint.x == -1.0f && nextWayPoint.z == -1.0f) {
		Fail(false);
		return;
	}

	const auto CWP_BLOCK_MASK = CMoveMath::SquareIsBlocked(*owner->moveDef, currWayPoint, owner);
	const auto NWP_BLOCK_MASK = CMoveMath::SquareIsBlocked(*owner->moveDef, nextWayPoint, owner);

	if ((CWP_BLOCK_MASK & CMoveMath::BLOCK_STRUCTURE) == 0 && (NWP_BLOCK_MASK & CMoveMath::BLOCK_STRUCTURE) == 0)
		return;

	// this can happen if we crushed a non-blocking feature
	// and it spawned another feature which we cannot crush
	// (eg.) --> repath
	ReRequestPath(false);
}




/*
Gives the position this unit will end up at with full braking
from current velocity.
*/
float3 CGroundMoveType::Here() const
{
	const float dist = BrakingDistance(currentSpeed, decRate);
	const int   sign = Sign(int(!reversing));

	const float3 pos2D = owner->pos * XZVector;
	const float3 dir2D = flatFrontDir * dist * sign;

	return (pos2D + dir2D);
}






void CGroundMoveType::StartEngine(bool callScript) {
	if (pathID == 0)
		pathID = GetNewPath();

	if (pathID != 0) {
		pathManager->UpdatePath(owner, pathID);

		if (callScript) {
			// makes no sense to call this unless we have a new path
			owner->script->StartMoving(reversing);
		}
	}

	nextObstacleAvoidanceFrame = gs->frameNum;
}

void CGroundMoveType::StopEngine(bool callScript, bool hardStop) {
	if (pathID != 0) {
		pathManager->DeletePath(pathID);
		pathID = 0;

		if (callScript)
			owner->script->StopMoving();
	}

	owner->SetVelocityAndSpeed(owner->speed * (1 - hardStop));

	currentSpeed *= (1 - hardStop);
	wantedSpeed = 0.0f;
}



/* Called when the unit arrives at its goal. */
void CGroundMoveType::Arrived(bool callScript)
{
	// can only "arrive" if the engine is active
	if (progressState == Active) {
		StopEngine(callScript);

		if (owner->team == gu->myTeam)
			Channels::General->PlayRandomSample(owner->unitDef->sounds.arrived, owner);

		// and the action is done
		progressState = Done;

		// update the position-parameter of our queue's front CMD_MOVE
		// this is needed in case we Arrive()'ed non-directly (through
		// colliding with another unit that happened to share our goal)
		if (!owner->commandAI->HasMoreMoveCommands())
			static_cast<CMobileCAI*>(owner->commandAI)->SetFrontMoveCommandPos(owner->pos);

		owner->commandAI->SlowUpdate();

		LOG_L(L_DEBUG, "[%s] unit %i arrived", __func__, owner->id);
	}
}

/*
Makes the unit fail this action.
No more trials will be done before a new goal is given.
*/
void CGroundMoveType::Fail(bool callScript)
{
	LOG_L(L_DEBUG, "[%s] unit %i failed", __func__, owner->id);

	StopEngine(callScript);

	// failure of finding a path means that
	// this action has failed to reach its goal.
	progressState = Failed;

	eventHandler.UnitMoveFailed(owner);
	eoh->UnitMoveFailed(*owner);
}




void CGroundMoveType::HandleObjectCollisions()
{
	SCOPED_TIMER("Sim::Unit::MoveType::Collisions");

	CUnit* collider = owner;

	// handle collisions for even-numbered objects on even-numbered frames and vv.
	// (temporal resolution is still high enough to not compromise accuracy much?)
	// if ((collider->id & 1) == (gs->frameNum & 1)) {
	if (!collider->beingBuilt) {
		const UnitDef* colliderUD = collider->unitDef;
		const MoveDef* colliderMD = collider->moveDef;

		// NOTE:
		//   use the collider's MoveDef footprint as radius since it is
		//   always mobile (its UnitDef footprint size may be different)
		const float colliderFootPrintRadius = colliderMD->CalcFootPrintMaxInteriorRadius(); // ~= CalcFootPrintMinExteriorRadius(0.75f)
		const float colliderAxisStretchFact = colliderMD->CalcFootPrintAxisStretchFactor();

		HandleUnitCollisions(collider, {collider->speed.w, colliderFootPrintRadius, colliderAxisStretchFact}, colliderUD, colliderMD);
		HandleFeatureCollisions(collider, {collider->speed.w, colliderFootPrintRadius, colliderAxisStretchFact}, colliderUD, colliderMD);

		// blocked square collision (very performance hungry, process only every 2nd game frame)
		// dangerous: reduces effective square-size from 8 to 4, but many ground units can move
		// at speeds greater than half the effective square-size per frame so this risks getting
		// stuck on impassable squares
		const bool squareChange = (CGround::GetSquare(owner->pos + owner->speed) != CGround::GetSquare(owner->pos));
		const bool checkAllowed = ((collider->id & 1) == (gs->frameNum & 1));

		if (!squareChange && !checkAllowed)
			return;

		if (!HandleStaticObjectCollision(owner, owner, owner->moveDef,  colliderFootPrintRadius, 0.0f,  ZeroVector, true, false, true))
			return;

		ReRequestPath(false);
	}
}

bool CGroundMoveType::HandleStaticObjectCollision(
	CUnit* collider,
	CSolidObject* collidee,
	const MoveDef* colliderMD,
	const float colliderRadius,
	const float collideeRadius,
	const float3& separationVector,
	bool canRequestPath,
	bool checkYardMap,
	bool checkTerrain
) {
	// while being built, units that overlap their factory yardmap should not be moved at all
	assert(!collider->beingBuilt);

	if (checkTerrain && (!collider->IsMoving() || collider->IsInAir()))
		return false;

	// for factories, check if collidee's position is behind us (which means we are likely exiting)
	//
	// NOTE:
	//   allow units to move _through_ idle open factories by extending the collidee's footprint such
	//   that insideYardMap is true in a larger area (otherwise pathfinder and coldet would disagree)
	//   the transition from radius- to footprint-based handling is discontinuous --> cannot mix them
	// TODO:
	//   increase cost of squares inside open factories so PFS is less likely to path through them
	//
	#if 0
	const int xext = ((collidee->xsize >> 1) + std::max(1, colliderMD->xsizeh));
	const int zext = ((collidee->zsize >> 1) + std::max(1, colliderMD->zsizeh));

	const bool insideYardMap =
		(collider->pos.x >= (collidee->pos.x - xext * SQUARE_SIZE)) &&
		(collider->pos.x <= (collidee->pos.x + xext * SQUARE_SIZE)) &&
		(collider->pos.z >= (collidee->pos.z - zext * SQUARE_SIZE)) &&
		(collider->pos.z <= (collidee->pos.z + zext * SQUARE_SIZE));
	const bool exitingYardMap =
		((collider->frontdir.dot(separationVector) > 0.0f) &&
		 (collider->   speed.dot(separationVector) > 0.0f));
	#endif

	const float3& pos = collider->pos;
	const float3& vel = collider->speed;
	const float3& rgt = collider->rightdir;

	float3 strafeVec;
	float3 bounceVec;
	float3 summedVec;

	if (checkYardMap || checkTerrain) {
		float3 sqrSumPosition; // .y is always 0
		float2 sqrPenDistance; // .x = sum, .y = count

		const float3 rightDir2D = (rgt * XZVector).SafeNormalize();
		const float3 speedDir2D = (vel * XZVector).SafeNormalize();


		const int xmid = (pos.x + vel.x) / SQUARE_SIZE;
		const int zmid = (pos.z + vel.z) / SQUARE_SIZE;

		// mantis{3614,4217}
		//   we cannot nicely bounce off terrain when checking only the center square
		//   however, testing more squares means CD can (sometimes) disagree with PFS
		//   in narrow passages --> still possible, but have to ensure we allow only
		//   lateral (non-obstructing) bounces
		const int xsh = colliderMD->xsizeh * (checkYardMap || (checkTerrain && colliderMD->allowTerrainCollisions));
		const int zsh = colliderMD->zsizeh * (checkYardMap || (checkTerrain && colliderMD->allowTerrainCollisions));

		const int xmin = std::min(-1, -xsh), xmax = std::max(1, xsh);
		const int zmin = std::min(-1, -zsh), zmax = std::max(1, zsh);

		if (DEBUG_DRAWING_ENABLED)
			geometricObjects->AddLine(pos + (UpVector * 25.0f), pos + (UpVector * 100.0f), 3, 1, 4);

		// check for blocked squares inside collider's MoveDef footprint zone
		// interpret each square as a "collidee" and sum up separation vectors
		//
		// NOTE:
		//   assumes the collider's footprint is still always axis-aligned
		// NOTE:
		//   the pathfinders only care about the CENTER square wrt terrain!
		//   this means paths can come closer to impassable terrain than is
		//   allowed by collision detection (more probable if edges between
		//   passable and impassable areas are hard instead of gradients or
		//   if a unit is not affected by slopes) --> can be solved through
		//   smoothing the cost-function, eg. blurring heightmap before PFS
		//   sees it
		//
		for (int z = zmin; z <= zmax; z++) {
			for (int x = xmin; x <= xmax; x++) {
				const int xabs = xmid + x;
				const int zabs = zmid + z;

				if ( checkTerrain &&  (CMoveMath::GetPosSpeedMod(*colliderMD, xabs, zabs, speedDir2D) > 0.01f))
					continue;
				if (!checkTerrain && ((CMoveMath::SquareIsBlocked(*colliderMD, xabs, zabs, collider) & CMoveMath::BLOCK_STRUCTURE) == 0))
					continue;

				const float3 squarePos = float3(xabs * SQUARE_SIZE + (SQUARE_SIZE >> 1), pos.y, zabs * SQUARE_SIZE + (SQUARE_SIZE >> 1));
				const float3 squareVec = pos - squarePos;

				// ignore squares behind us (relative to velocity vector)
				if (squareVec.dot(vel) > 0.0f)
					continue;

				// RHS magic constant is the radius of a square (sqrt(2*(SQUARE_SIZE>>1)*(SQUARE_SIZE>>1)))
				const float  squareColRadiusSum = colliderRadius + 5.656854249492381f;
				const float   squareSepDistance = squareVec.Length2D() + 0.1f;
				const float   squarePenDistance = std::min(squareSepDistance - squareColRadiusSum, 0.0f);
				// const float  squareColSlideSign = -Sign(squarePos.dot(rightDir2D) - pos.dot(rightDir2D));

				// this tends to cancel out too much on average
				// strafeVec += (rightDir2D * sqColSlideSign);
				bounceVec += (rightDir2D * (rightDir2D.dot(squareVec / squareSepDistance)));

				sqrPenDistance += float2(squarePenDistance, 1.0f);
				sqrSumPosition += (squarePos * XZVector);
			}
		}

		if (sqrPenDistance.y > 0.0f) {
			sqrSumPosition *= (1.0f / sqrPenDistance.y);
			sqrPenDistance *= (1.0f / sqrPenDistance.y);

			const float strafeSign = -Sign(sqrSumPosition.dot(rightDir2D) - pos.dot(rightDir2D));
			const float bounceSign =  Sign(rightDir2D.dot(bounceVec));
			const float strafeScale = std::min(std::max(currentSpeed*0.0f, maxSpeedDef), std::max(0.1f, -sqrPenDistance.x * 0.5f));
			const float bounceScale = std::min(std::max(currentSpeed*0.0f, maxSpeedDef), std::max(0.1f, -sqrPenDistance.x * 0.5f));

			// in FPS mode, normalize {strafe,bounce}Scale and multiply by maxSpeedDef
			// (otherwise it would be possible to slide along map edges at above-normal
			// speeds, etc.)
			const float fpsStrafeScale = (strafeScale / (strafeScale + bounceScale)) * maxSpeedDef;
			const float fpsBounceScale = (bounceScale / (strafeScale + bounceScale)) * maxSpeedDef;

			// bounceVec always points along rightDir by construction
			strafeVec = (rightDir2D * strafeSign) * mix(strafeScale, fpsStrafeScale, owner->UnderFirstPersonControl());
			bounceVec = (rightDir2D * bounceSign) * mix(bounceScale, fpsBounceScale, owner->UnderFirstPersonControl());
			summedVec = strafeVec + bounceVec;

			// if checkTerrain is true, test only the center square
			if (colliderMD->TestMoveSquare(collider, pos + summedVec, vel, checkTerrain, checkYardMap, checkTerrain)) {
				collider->Move(summedVec, true);

				// minimal hack to make FollowPath work at all turn-rates
				// since waypointDir will undergo a (large) discontinuity
				currWayPoint += summedVec;
				nextWayPoint += summedVec;
			} else {
				// never move fully back to oldPos when dealing with yardmaps
				collider->Move((oldPos - pos) + summedVec * 0.25f * checkYardMap, true);
			}
		}

		// note:
		//   in many cases this does not mean we should request a new path
		//   (and it can be counter-productive to do so since we might not
		//   even *get* one)
		return (canRequestPath && (summedVec != ZeroVector));
	}

	{
		const float  colRadiusSum = colliderRadius + collideeRadius;
		const float   sepDistance = separationVector.Length() + 0.1f;
		const float   penDistance = std::min(sepDistance - colRadiusSum, 0.0f);
		const float  colSlideSign = -Sign(collidee->pos.dot(rgt) - pos.dot(rgt));

		const float strafeScale = std::min(currentSpeed, std::max(0.0f, -penDistance * 0.5f)) * (1 - checkYardMap * false);
		const float bounceScale = std::min(currentSpeed, std::max(0.0f, -penDistance       )) * (1 - checkYardMap *  true);

		strafeVec = (             rgt * colSlideSign) * strafeScale;
		bounceVec = (separationVector /  sepDistance) * bounceScale;
		summedVec = strafeVec + bounceVec;

		if (colliderMD->TestMoveSquare(collider, pos + summedVec, vel, true, true, true)) {
			collider->Move(summedVec, true);

			currWayPoint += summedVec;
			nextWayPoint += summedVec;
		} else {
			// move back to previous-frame position
			// ChangeSpeed calculates speedMod without checking squares for *structure* blockage
			// (so that a unit can free itself if it ends up within the footprint of a structure)
			// this means deltaSpeed will be non-zero if stuck on an impassable square and hence
			// the new speedvector which is constructed from deltaSpeed --> we would simply keep
			// moving forward through obstacles if not counteracted by this
			collider->Move((oldPos - pos) + summedVec * 0.25f * (collider->frontdir.dot(separationVector) < 0.25f), true);
		}

		// same here
		return (canRequestPath && (penDistance < 0.0f));
	}
}

void CGroundMoveType::HandleUnitCollisions(
	CUnit* collider,
	const float3& colliderParams, // .x := speed, .y := radius, .z := fpstretch
	const UnitDef* colliderUD,
	const MoveDef* colliderMD
) {
	// NOTE: probably too large for most units (eg. causes tree falling animations to be skipped)
	const float3 crushImpulse = collider->speed * collider->mass * Sign(int(!reversing));

	const bool allowUCO = modInfo.allowUnitCollisionOverlap;
	const bool allowCAU = modInfo.allowCrushingAlliedUnits;
	const bool allowPEU = modInfo.allowPushingEnemyUnits;
	const bool allowSAT = modInfo.allowSepAxisCollisionTest;
	const bool forceSAT = (colliderParams.z > 0.1f);

	// copy on purpose, since the below can call Lua
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, collider->pos, colliderParams.x + (colliderParams.y * 2.0f));

	for (CUnit* collidee: *qfQuery.units) {
		if (collidee == collider) continue;
		if (collidee->IsSkidding()) continue;
		if (collidee->IsFlying()) continue;

		const UnitDef* collideeUD = collidee->unitDef;
		const MoveDef* collideeMD = collidee->moveDef;

		const bool colliderMobile = (colliderMD != nullptr); // always true
		const bool collideeMobile = (collideeMD != nullptr); // maybe true

		const bool unloadingCollidee = (collidee->unloadingTransportId == collider->id);
		const bool unloadingCollider = (collider->unloadingTransportId == collidee->id);

		if (unloadingCollidee)
			collidee->unloadingTransportId = -1;
		if (unloadingCollider)
			collider->unloadingTransportId = -1;


		// don't push/crush either party if the collidee does not block the collider (or vv.)
		if (colliderMobile && CMoveMath::IsNonBlocking(*colliderMD, collidee, collider))
			continue;
		if (collideeMobile && CMoveMath::IsNonBlocking(*collideeMD, collider, collidee))
			continue;

		// disable collisions between collider and collidee
		// if collidee is currently inside any transporter,
		// or if collider is being transported by collidee
		if (collider->GetTransporter() == collidee) continue;
		if (collidee->GetTransporter() != nullptr) continue;
		// also disable collisions if either party currently
		// has an order to load units (TODO: do we want this
		// for unloading as well?)
		if (collider->loadingTransportId == collidee->id) continue;
		if (collidee->loadingTransportId == collider->id) continue;

		// use the collidee's MoveDef footprint as radius if it is mobile
		// use the collidee's Unit (not UnitDef) footprint as radius otherwise
		const float2 collideeParams = {collidee->speed.w, collideeMobile? collideeMD->CalcFootPrintMaxInteriorRadius(): collidee->CalcFootPrintMaxInteriorRadius()};
		const float4 separationVect = {collider->pos - collidee->pos, Square(colliderParams.y + collideeParams.y)};

		if (!checkCollisionFuncs[allowSAT && (forceSAT || (collideeMD->CalcFootPrintAxisStretchFactor() > 0.1f))](separationVect, collider, collidee, colliderMD, collideeMD))
			continue;


		if (unloadingCollidee) {
			collidee->unloadingTransportId = collider->id;
			continue;
		}

		if (unloadingCollider) {
			collider->unloadingTransportId = collidee->id;
			continue;
		}


		// NOTE:
		//   we exclude aircraft (which have NULL moveDef's) landed
		//   on the ground, since they would just stack when pushed
		bool pushCollider = colliderMobile;
		bool pushCollidee = collideeMobile;
		bool crushCollidee = false;

		const bool alliedCollision =
			teamHandler.Ally(collider->allyteam, collidee->allyteam) &&
			teamHandler.Ally(collidee->allyteam, collider->allyteam);
		const bool collideeYields = (collider->IsMoving() && !collidee->IsMoving());
		const bool ignoreCollidee = (collideeYields && alliedCollision);

		crushCollidee |= (!alliedCollision || allowCAU);
		crushCollidee &= ((colliderParams.x * collider->mass) > (collideeParams.x * collidee->mass));

		if (crushCollidee && !CMoveMath::CrushResistant(*colliderMD, collidee))
			collidee->Kill(collider, crushImpulse, true);

		if (eventHandler.UnitUnitCollision(collider, collidee))
			continue;

		if (collideeMobile)
			HandleUnitCollisionsAux(collider, collidee, this, static_cast<CGroundMoveType*>(collidee->moveType));

		// NOTE:
		//   allowPushingEnemyUnits is (now) useless because alliances are bi-directional
		//   ie. if !alliedCollision, pushCollider and pushCollidee BOTH become false and
		//   the collision is treated normally --> not what we want here, but the desired
		//   behavior (making each party stop and block the other) has many corner-cases
		//   so instead have collider respond as though collidee is semi-static obstacle
		//   this also happens when both parties are pushResistant
		pushCollider = pushCollider && (alliedCollision || allowPEU || !collider->blockEnemyPushing);
		pushCollidee = pushCollidee && (alliedCollision || allowPEU || !collidee->blockEnemyPushing);
		pushCollider = pushCollider && (!collider->beingBuilt && !collider->UsingScriptMoveType() && !collider->moveType->IsPushResistant());
		pushCollidee = pushCollidee && (!collidee->beingBuilt && !collidee->UsingScriptMoveType() && !collidee->moveType->IsPushResistant());

		if ((!collideeMobile && !collideeUD->IsAirUnit()) || (!pushCollider && !pushCollidee)) {
			// building (always axis-aligned, possibly has a yardmap)
			// or semi-static collidee that should be handled as such
			//
			// since all push-resistant units use the BLOCK_STRUCTURE
			// mask when stopped, avoid the yardmap || terrain branch
			// of HSOC which is not well suited to both parties moving
			// and can leave them inside stuck each other's footprints
			const bool allowNewPath = (!atEndOfPath && !atGoal);
			const bool checkYardMap = ((pushCollider || pushCollidee) || collideeUD->IsFactoryUnit());

			if (HandleStaticObjectCollision(collider, collidee, colliderMD,  colliderParams.y, collideeParams.y,  separationVect, allowNewPath, checkYardMap, false))
				ReRequestPath(false);

			continue;
		}


		const float colliderRelRadius = colliderParams.y / (colliderParams.y + collideeParams.y);
		const float collideeRelRadius = collideeParams.y / (colliderParams.y + collideeParams.y);
		const float collisionRadiusSum = allowUCO?
			(colliderParams.y * colliderRelRadius + collideeParams.y * collideeRelRadius):
			(colliderParams.y                     + collideeParams.y                    );

		const float  sepDistance = separationVect.Length() + 0.1f;
		const float  penDistance = std::max(collisionRadiusSum - sepDistance, 1.0f);
		const float  sepResponse = std::min(SQUARE_SIZE * 2.0f, penDistance * 0.5f);

		const float3 sepDirection   = separationVect / sepDistance;
		const float3 colResponseVec = sepDirection * XZVector * sepResponse;

		const float
			m1 = collider->mass,
			m2 = collidee->mass,
			v1 = std::max(1.0f, colliderParams.x),
			v2 = std::max(1.0f, collideeParams.x),
			c1 = 1.0f + (1.0f - math::fabs(collider->frontdir.dot(-sepDirection))) * 5.0f,
			c2 = 1.0f + (1.0f - math::fabs(collidee->frontdir.dot( sepDirection))) * 5.0f,
			// weighted momenta
			s1 = m1 * v1 * c1,
			s2 = m2 * v2 * c2,
			// relative momenta
 			r1 = s1 / (s1 + s2 + 1.0f),
 			r2 = s2 / (s1 + s2 + 1.0f);

		// far from a realistic treatment, but works
		const float colliderMassScale = Clamp(1.0f - r1, 0.01f, 0.99f) * (allowUCO? (1.0f / colliderRelRadius): 1.0f);
		const float collideeMassScale = Clamp(1.0f - r2, 0.01f, 0.99f) * (allowUCO? (1.0f / collideeRelRadius): 1.0f);

		// try to prevent both parties from being pushed onto non-traversable
		// squares (without resetting their position which stops them dead in
		// their tracks and undoes previous legitimate pushes made this frame)
		//
		// if pushCollider and pushCollidee are both false (eg. if each party
		// is pushResistant), treat the collision as regular and push both to
		// avoid deadlocks
		const float colliderSlideSign = Sign( separationVect.dot(collider->rightdir));
		const float collideeSlideSign = Sign(-separationVect.dot(collidee->rightdir));

		const float3 colliderPushVec  =  colResponseVec * colliderMassScale * int(!ignoreCollidee);
		const float3 collideePushVec  = -colResponseVec * collideeMassScale;
		const float3 colliderSlideVec = collider->rightdir * colliderSlideSign * (1.0f / penDistance) * r2;
		const float3 collideeSlideVec = collidee->rightdir * collideeSlideSign * (1.0f / penDistance) * r1;
		const float3 colliderMoveVec  = colliderPushVec + colliderSlideVec;
		const float3 collideeMoveVec  = collideePushVec + collideeSlideVec;

		const bool moveCollider = ((pushCollider || !pushCollidee) && colliderMobile);
		const bool moveCollidee = ((pushCollidee || !pushCollider) && collideeMobile);

		if (moveCollider && colliderMD->TestMoveSquare(collider, collider->pos + colliderMoveVec, colliderMoveVec))
			collider->Move(colliderMoveVec, true);

		if (moveCollidee && collideeMD->TestMoveSquare(collidee, collidee->pos + collideeMoveVec, collideeMoveVec))
			collidee->Move(collideeMoveVec, true);
	}
}

void CGroundMoveType::HandleFeatureCollisions(
	CUnit* collider,
	const float3& colliderParams,
	const UnitDef* colliderUD,
	const MoveDef* colliderMD
) {
	const float3 crushImpulse = collider->speed * collider->mass * Sign(int(!reversing));

	const bool allowSAT = modInfo.allowSepAxisCollisionTest;
	const bool forceSAT = (colliderParams.z > 0.1f);

	// copy on purpose, since DoDamage below can call Lua
	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, collider->pos, colliderParams.x + (colliderParams.y * 2.0f));

	for (CFeature* collidee: *qfQuery.features) {
		// const FeatureDef* collideeFD = collidee->def;

		// use the collidee's Feature (not FeatureDef) footprint as radius
		const float2 collideeParams = {0.0f, collidee->CalcFootPrintMaxInteriorRadius()};
		const float4 separationVect = {collider->pos - collidee->pos, Square(colliderParams.y + collideeParams.y)};

		if (!checkCollisionFuncs[allowSAT && (forceSAT || (collidee->CalcFootPrintAxisStretchFactor() > 0.1f))](separationVect, collider, collidee, colliderMD, nullptr))
			continue;


		if (CMoveMath::IsNonBlocking(*colliderMD, collidee, collider))
			continue;
		if (!CMoveMath::CrushResistant(*colliderMD, collidee))
			collidee->Kill(collider, crushImpulse, true);

		#if 0
		if (pathController.IgnoreCollision(collider, collidee))
			continue;
		#endif

		if (eventHandler.UnitFeatureCollision(collider, collidee))
			continue;

		if (!collidee->IsMoving()) {
			if (HandleStaticObjectCollision(collider, collidee, colliderMD,  colliderParams.y, collideeParams.y,  separationVect, (!atEndOfPath && !atGoal), true, false))
				ReRequestPath(false);

			continue;
		}

		const float  sepDistance    = separationVect.Length() + 0.1f;
		const float  penDistance    = std::max((colliderParams.y + collideeParams.y) - sepDistance, 1.0f);
		const float  sepResponse    = std::min(SQUARE_SIZE * 2.0f, penDistance * 0.5f);

		const float3 sepDirection   = separationVect / sepDistance;
		const float3 colResponseVec = sepDirection * XZVector * sepResponse;

		// multiply the collidee's mass by a large constant (so that heavy
		// features do not bounce light units away like jittering pinballs;
		// collideeMassScale ~= 0.01 suppresses large responses)
		const float
			m1 = collider->mass,
			m2 = collidee->mass * 10000.0f,
			v1 = std::max(1.0f, colliderParams.x),
			v2 = 1.0f,
			c1 = (1.0f - math::fabs( collider->frontdir.dot(-sepDirection))) * 5.0f,
			c2 = (1.0f - math::fabs(-collider->frontdir.dot( sepDirection))) * 5.0f,
			s1 = m1 * v1 * c1,
			s2 = m2 * v2 * c2,
 			r1 = s1 / (s1 + s2 + 1.0f),
 			r2 = s2 / (s1 + s2 + 1.0f);

		const float colliderMassScale = Clamp(1.0f - r1, 0.01f, 0.99f);
		const float collideeMassScale = Clamp(1.0f - r2, 0.01f, 0.99f);

		quadField.RemoveFeature(collidee);
		collider->Move( colResponseVec * colliderMassScale, true);
		collidee->Move(-colResponseVec * collideeMassScale, true);
		quadField.AddFeature(collidee);
	}
}



void CGroundMoveType::LeaveTransport()
{
	oldPos = owner->pos + UpVector * 0.001f;
}



void CGroundMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive) { KeepPointingTo(unit->pos, distance, aggressive); }
void CGroundMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive) {
	mainHeadingPos = pos;
	useMainHeading = aggressive;

	if (!useMainHeading)
		return;
	if (owner->weapons.empty())
		return;

	const CWeapon* frontWeapon = owner->weapons.front();

	if (!frontWeapon->weaponDef->waterweapon)
		mainHeadingPos.y = std::max(mainHeadingPos.y, 0.0f);

	float3 dir1 = frontWeapon->mainDir;
	float3 dir2 = mainHeadingPos - owner->pos;

	// in this case aligning is impossible
	if (dir1 == UpVector)
		return;

	dir1 = (dir1 * XZVector).SafeNormalize();
	dir2 = (dir2 * XZVector).SafeNormalize();

	if (dir2 == ZeroVector)
		return;

	const short heading =
		GetHeadingFromVector(dir2.x, dir2.z) -
		GetHeadingFromVector(dir1.x, dir1.z);

	if (owner->heading == heading)
		return;

	// NOTE:
	//   by changing the progress-state here (which seems redundant),
	//   SlowUpdate can suddenly request a new path for us even after
	//   StopMoving (which clears pathID; CAI often calls StopMoving
	//   before unit is at goalPos!)
	//   for this reason StopMoving always updates goalPos so internal
	//   GetNewPath's are no-ops (while CAI does not call StartMoving)
	if (frontWeapon->TestRange(mainHeadingPos, SWeaponTarget(mainHeadingPos, true)))
		return;

	progressState = Active;
}


/**
* @brief Orients owner so that weapon[0]'s arc includes mainHeadingPos
*/
void CGroundMoveType::SetMainHeading() {
	if (!useMainHeading || owner->weapons.empty()) {
		ChangeHeading(owner->heading);
		return;
	}

	const CWeapon* frontWeapon = owner->weapons.front();

	const float3 dir1 = ((       frontWeapon->mainDir) * XZVector).SafeNormalize();
	const float3 dir2 = ((mainHeadingPos - owner->pos) * XZVector).SafeNormalize();

	// ASSERT_SYNCED(dir1);
	// ASSERT_SYNCED(dir2);

	if (dir2 == ZeroVector)
		return;

	short newHeading =
		GetHeadingFromVector(dir2.x, dir2.z) -
		GetHeadingFromVector(dir1.x, dir1.z);

	ASSERT_SYNCED(newHeading);

	if (progressState == Active) {
		if (owner->heading != newHeading) {
			// start or continue turning
			ChangeHeading(newHeading);
		} else {
			// stop turning
			progressState = Done;
		}

		return;
	}

	if (owner->heading == newHeading)
		return;

	if (frontWeapon->TestRange(mainHeadingPos, SWeaponTarget(mainHeadingPos, true)))
		return;

	progressState = Active;
}

bool CGroundMoveType::OnSlope(float minSlideTolerance) {
	const UnitDef* ud = owner->unitDef;
	const MoveDef* md = owner->moveDef;
	const float3& pos = owner->pos;

	if (ud->slideTolerance < minSlideTolerance)
		return false;
	if (owner->FloatOnWater() && owner->IsInWater())
		return false;
	if (!pos.IsInBounds())
		return false;

	// if minSlideTolerance is LEQ 0, do not multiply maxSlope by ud->slideTolerance
	// (otherwise the unit could stop on an invalid path location, and be teleported
	// back)
	const float slopeMul = mix(ud->slideTolerance, 1.0f, (minSlideTolerance <= 0.0f));
	const float curSlope = CGround::GetSlope(pos.x, pos.z);
	const float maxSlope = md->maxSlope * slopeMul;

	return (curSlope > maxSlope);
}



const float3& CGroundMoveType::GetGroundNormal(const float3& p) const
{
	// ship or hovercraft; return (CGround::GetNormalAboveWater(p));
	if (owner->IsInWater() && !owner->IsOnGround())
		return UpVector;

	return (CGround::GetNormal(p.x, p.z));
}

float CGroundMoveType::GetGroundHeight(const float3& p) const
{
	// in [minHeight, maxHeight]
	const float gh = CGround::GetHeightReal(p.x, p.z);
	const float wh = -waterline * (gh <= 0.0f);

	// in [-waterline, maxHeight], note that waterline
	// can be much deeper than ground in shallow water
	if (owner->FloatOnWater())
		return (std::max(gh, wh));

	return gh;
}

void CGroundMoveType::AdjustPosToWaterLine()
{
	if (owner->IsFalling())
		return;
	if (owner->IsFlying())
		return;

	if (modInfo.allowGroundUnitGravity) {
		if (owner->FloatOnWater()) {
			owner->Move(UpVector * (std::max(CGround::GetHeightReal(owner->pos.x, owner->pos.z),   -waterline) - owner->pos.y), true);
		} else {
			owner->Move(UpVector * (std::max(CGround::GetHeightReal(owner->pos.x, owner->pos.z), owner->pos.y) - owner->pos.y), true);
		}
	} else {
		owner->Move(UpVector * (GetGroundHeight(owner->pos) - owner->pos.y), true);
	}
}

bool CGroundMoveType::UpdateDirectControl()
{
	const CPlayer* myPlayer = gu->GetMyPlayer();
	const FPSUnitController& selfCon = myPlayer->fpsController;
	const FPSUnitController& unitCon = owner->fpsControlPlayer->fpsController;
	const bool wantReverse = (unitCon.back && !unitCon.forward);

	float turnSign = 0.0f;

	currWayPoint = owner->frontdir * XZVector * mix(100.0f, -100.0f, wantReverse);
	currWayPoint = (owner->pos + currWayPoint).cClampInBounds();

	if (unitCon.forward || unitCon.back) {
		ChangeSpeed((maxSpeed * unitCon.forward) + (maxReverseSpeed * unitCon.back), wantReverse, true);
	} else {
		// not moving forward or backward, stop
		ChangeSpeed(0.0f, false, true);
	}

	if (unitCon.left ) { ChangeHeading(owner->heading + turnRate); turnSign =  1.0f; }
	if (unitCon.right) { ChangeHeading(owner->heading - turnRate); turnSign = -1.0f; }

	// local client is controlling us
	if (selfCon.GetControllee() == owner)
		camera->SetRotY(camera->GetRot().y + turnRate * turnSign * TAANG2RAD);

	return wantReverse;
}


void CGroundMoveType::UpdateOwnerPos(const float3& oldSpeedVector, const float3& newSpeedVector) {
	const float oldSpeed = oldSpeedVector.dot(flatFrontDir);
	const float newSpeed = newSpeedVector.dot(flatFrontDir);

	// if being built, the nanoframe might not be exactly on
	// the ground and would jitter from gravity acting on it
	// --> nanoframes can not move anyway, just return early
	// (units that become reverse-built will continue moving)
	if (owner->beingBuilt)
		return;

	if (!newSpeedVector.same(ZeroVector)) {
		// use the simplest possible Euler integration
		owner->SetVelocityAndSpeed(newSpeedVector);
		owner->Move(owner->speed, true);

		// NOTE:
		//   does not check for structure blockage, coldet handles that
		//   entering of impassable terrain is *also* handled by coldet
		//
		//   the loop below tries to evade "corner" squares that would
		//   block us from initiating motion and is needed for when we
		//   are not *currently* moving but want to get underway to our
		//   first waypoint (HSOC coldet won't help then)
		//
 		//   allowing movement through blocked squares when pathID != 0
 		//   relies on assumption that PFS will not search if start-sqr
 		//   is blocked, so too fragile
		//
		if (!pathController.IgnoreTerrain(*owner->moveDef, owner->pos) && !owner->moveDef->TestMoveSquare(owner, owner->pos, owner->speed, true, false, true)) {
			bool updatePos = false;

			for (unsigned int n = 1; n <= SQUARE_SIZE; n++) {
				if (!updatePos && (updatePos = owner->moveDef->TestMoveSquare(owner, owner->pos + owner->rightdir * n, owner->speed, true, false, true))) {
					owner->Move(owner->pos + owner->rightdir * n, false);
					break;
				}
				if (!updatePos && (updatePos = owner->moveDef->TestMoveSquare(owner, owner->pos - owner->rightdir * n, owner->speed, true, false, true))) {
					owner->Move(owner->pos - owner->rightdir * n, false);
					break;
				}
			}

			if (!updatePos)
				owner->Move((owner->pos - newSpeedVector), false);
		}

		// NOTE:
		//   this can fail when gravity is allowed (a unit catching air
		//   can easily end up on an impassable square, especially when
		//   terrain contains micro-bumps) --> more likely at lower g's
		// assert(owner->moveDef->TestMoveSquare(owner, owner->pos, owner->speed, true, false, true));
	}

	reversing = UpdateOwnerSpeed(math::fabs(oldSpeed), math::fabs(newSpeed), newSpeed);
}

bool CGroundMoveType::UpdateOwnerSpeed(float oldSpeedAbs, float newSpeedAbs, float newSpeedRaw)
{
	const bool oldSpeedAbsGTZ = (oldSpeedAbs > 0.01f);
	const bool newSpeedAbsGTZ = (newSpeedAbs > 0.01f);
	const bool newSpeedRawLTZ = (newSpeedRaw < 0.0f );

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, newSpeedAbsGTZ);

	if (!oldSpeedAbsGTZ &&  newSpeedAbsGTZ)
		owner->script->StartMoving(newSpeedRawLTZ);
	if ( oldSpeedAbsGTZ && !newSpeedAbsGTZ)
		owner->script->StopMoving();

	currentSpeed = newSpeedAbs;
	deltaSpeed   = 0.0f;

	return newSpeedRawLTZ;
}


bool CGroundMoveType::WantReverse(const float3& wpDir, const float3& ffDir) const
{
	if (!canReverse)
		return false;

	// these values are normally non-0, but LuaMoveCtrl
	// can override them and we do not want any div0's
	if (maxReverseSpeed <= 0.0f)
		return false;
	if (maxSpeed <= 0.0f)
		return true;

	if (accRate <= 0.0f)
		return false;
	if (decRate <= 0.0f)
		return false;
	if (turnRate <= 0.0f)
		return false;

	if (wpDir.dot(ffDir) >= 0.0f)
		return false;

	const float goalDist   = (goalPos - owner->pos).Length2D();                  // use *final* WP for ETA calcs; in elmos
	const float goalFwdETA = (goalDist / maxSpeed);                              // in frames (simplistic)
	const float goalRevETA = (goalDist / maxReverseSpeed);                       // in frames (simplistic)

	const float waypointAngle = Clamp(wpDir.dot(owner->frontdir), -1.0f, 0.0f);  // clamp to prevent NaN's; [-1, 0]
	const float turnAngleDeg  = math::acosf(waypointAngle) * math::RAD_TO_DEG;   // in degrees; [90.0, 180.0]
	const float fwdTurnAngle  = (turnAngleDeg / 360.0f) * SPRING_CIRCLE_DIVS;    // in "headings"
	const float revTurnAngle  = SPRING_MAX_HEADING - fwdTurnAngle;               // 180 deg - angle

	// values <= 0 preserve default behavior
	if (maxReverseDist > 0.0f && minReverseAngle > 0.0f)
		return (currWayPointDist <= maxReverseDist && turnAngleDeg >= minReverseAngle);

	// units start accelerating before finishing the turn, so subtract something
	const float turnTimeMod      = 5.0f;
	const float fwdTurnAngleTime = std::max(0.0f, (fwdTurnAngle / turnRate) - turnTimeMod); // in frames
	const float revTurnAngleTime = std::max(0.0f, (revTurnAngle / turnRate) - turnTimeMod);

	const float apxFwdSpdAfterTurn = std::max(0.0f, currentSpeed - 0.125f * (fwdTurnAngleTime * decRate));
	const float apxRevSpdAfterTurn = std::max(0.0f, currentSpeed - 0.125f * (revTurnAngleTime * decRate));

	const float fwdDecTime = ( reversing * apxFwdSpdAfterTurn) / decRate;
	const float revDecTime = (!reversing * apxRevSpdAfterTurn) / decRate;
	const float fwdAccTime = (maxSpeed        - !reversing * apxFwdSpdAfterTurn) / accRate;
	const float revAccTime = (maxReverseSpeed -  reversing * apxRevSpdAfterTurn) / accRate;

	const float fwdETA = goalFwdETA + fwdTurnAngleTime + fwdAccTime + fwdDecTime;
	const float revETA = goalRevETA + revTurnAngleTime + revDecTime + revAccTime;

	return (fwdETA > revETA);
}



void CGroundMoveType::InitMemberPtrs(MemberData* memberData)
{
	memberData->bools[0].second = &atGoal;
	memberData->bools[1].second = &atEndOfPath;
	memberData->bools[2].second = &pushResistant;

	memberData->shorts[0].second = &minScriptChangeHeading;

	memberData->floats[0].second = &turnRate;
	memberData->floats[1].second = &turnAccel;
	memberData->floats[2].second = &accRate;
	memberData->floats[3].second = &decRate;
	memberData->floats[4].second = &myGravity;
	memberData->floats[5].second = &maxReverseDist,
	memberData->floats[6].second = &minReverseAngle;
	memberData->floats[7].second = &maxReverseSpeed;
	memberData->floats[8].second = &sqSkidSpeedMult;
}

bool CGroundMoveType::SetMemberValue(unsigned int memberHash, void* memberValue)
{
	// try the generic members first
	if (AMoveType::SetMemberValue(memberHash, memberValue))
		return true;

	// set pointers for this GMT instance
	InitMemberPtrs(&gmtMemberData);

	// special cases
	if (memberHash == gmtMemberData.floats[MAXREVERSESPEED_MEMBER_IDX].first) {
		*(gmtMemberData.floats[MAXREVERSESPEED_MEMBER_IDX].second) = *(reinterpret_cast<float*>(memberValue)) / GAME_SPEED;
		return true;
	}

	// note: <memberHash> should be calculated via HsiehHash
	// todo: use template lambdas in C++14
	{
		const auto pred = [memberHash](const std::pair<unsigned int, bool*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(gmtMemberData.bools.begin(), gmtMemberData.bools.end(), pred);
		if (iter != gmtMemberData.bools.end()) {
			*(iter->second) = *(reinterpret_cast<bool*>(memberValue));
			return true;
		}
	}
	{
		const auto pred = [memberHash](const std::pair<unsigned int, short*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(gmtMemberData.shorts.begin(), gmtMemberData.shorts.end(), pred);
		if (iter != gmtMemberData.shorts.end()) {
			*(iter->second) = short(*(reinterpret_cast<float*>(memberValue))); // sic (see SetMoveTypeValue)
			return true;
		}
	}
	{
		const auto pred = [memberHash](const std::pair<unsigned int, float*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(gmtMemberData.floats.begin(), gmtMemberData.floats.end(), pred);
		if (iter != gmtMemberData.floats.end()) {
			*(iter->second) = *(reinterpret_cast<float*>(memberValue));
			return true;
		}
	}

	return false;
}

