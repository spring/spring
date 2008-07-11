#include "StdAfx.h"
#include "GroundMoveType.h"
#include "Map/Ground.h"
#include "Sim/Misc/QuadField.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/LosHandler.h"
#include "Game/GameHelper.h"
#include "myMath.h"
#include "LogOutput.h"
#include "Sim/Units/UnitHandler.h"
#include "Sync/SyncTracer.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/CobFile.h"
#include "Sim/Units/UnitDef.h"
#include "Sound.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Path/PathManager.h"
#include "Game/Player.h"
#include "Game/Camera.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Game/SelectedUnits.h"
#include "Rendering/GroundDecalHandler.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "mmgr.h"

CR_BIND_DERIVED(CGroundMoveType, AMoveType, (NULL));

CR_REG_METADATA(CGroundMoveType, (
		CR_MEMBER(baseTurnRate),
		CR_MEMBER(turnRate),
		CR_MEMBER(accRate),
		CR_MEMBER(decRate),
		
		CR_MEMBER(wantedSpeed),
		CR_MEMBER(currentSpeed),
		CR_MEMBER(deltaSpeed),
		CR_MEMBER(deltaHeading),

		CR_MEMBER(oldPos),
		CR_MEMBER(oldSlowUpdatePos),
		CR_MEMBER(flatFrontDir),

		CR_MEMBER(pathId),
		CR_MEMBER(goalRadius),

		CR_MEMBER(waypoint),
		CR_MEMBER(nextWaypoint),
		CR_MEMBER(etaWaypoint),
		CR_MEMBER(etaWaypoint2),
		CR_MEMBER(atGoal),
		CR_MEMBER(haveFinalWaypoint),
		CR_MEMBER(terrainSpeed),

		CR_MEMBER(requestedSpeed),
		CR_MEMBER(requestedTurnRate),

		CR_MEMBER(currentDistanceToWaypoint),

		CR_MEMBER(avoidanceVec),

		CR_MEMBER(restartDelay),
		CR_MEMBER(lastGetPathPos),

		CR_MEMBER(pathFailures),
		CR_MEMBER(etaFailures),
		CR_MEMBER(nonMovingFailures),

		CR_MEMBER(moveType),

		CR_MEMBER(floatOnWater),

		CR_MEMBER(moveSquareX),
		CR_MEMBER(moveSquareY),

		CR_MEMBER(nextDeltaSpeedUpdate),
		CR_MEMBER(nextObstacleAvoidanceUpdate),

		CR_MEMBER(lastTrackUpdate),

		CR_MEMBER(skidding),
		CR_MEMBER(flying),
		CR_MEMBER(skidRotSpeed),
		CR_MEMBER(dropSpeed),
		CR_MEMBER(dropHeight),

		CR_MEMBER(skidRotVector),
		CR_MEMBER(skidRotSpeed2),
		CR_MEMBER(skidRotPos2),
		CR_ENUM_MEMBER(oldPhysState),

		CR_MEMBER(mainHeadingPos),
		CR_MEMBER(useMainHeading),
		CR_RESERVED(64),
		CR_POSTLOAD(PostLoad)
		));


const unsigned int MAX_REPATH_FREQUENCY = 30;		//The minimum of frames between two full path-requests.

const float ETA_ESTIMATION = 1.5f;					//How much time the unit are given to reach the waypoint.
const float MAX_WAYPOINT_DISTANCE_FACTOR = 2.0f;		//Used to tune how often new waypoints are requested. Multiplied with MinDistanceToWaypoint().
const float MAX_OFF_PATH_FACTOR = 20;				//How far away from a waypoint a unit could be before a new path is requested.

const float MINIMUM_SPEED = 0.01f;					//Minimum speed a unit may move in.

static const bool DEBUG_CONTROLLER=false;
std::vector<int2> (*CGroundMoveType::lineTable)[11] = 0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroundMoveType::CGroundMoveType(CUnit* owner)
:	AMoveType(owner),
	accRate(0.01f),
	decRate(0.01f),
	turnRate(0.1f),
	baseTurnRate(0.1f),
//	maxSpeed(0.2f),
	wantedSpeed(0),
	moveType(0),
	oldPos(owner?owner->pos:float3(0,0,0)),
	oldSlowUpdatePos(oldPos),
	flatFrontDir(1,0,0),
	deltaSpeed(0),
	deltaHeading(0),
	skidding(false),
	flying(false),
	dropHeight(0),
	skidRotSpeed(0),
	floatOnWater(false),
	skidRotVector(UpVector),
	skidRotSpeed2(0),
	skidRotPos2(0),

	pathId(0),
	goalRadius(0),
	waypoint(0,0,0),
	nextWaypoint(0,0,0),
	etaWaypoint(0),
	etaWaypoint2(0),
	terrainSpeed(1),
	atGoal(false),
	haveFinalWaypoint(false),
	requestedSpeed(0),
	requestedTurnRate(0),
	currentDistanceToWaypoint(0),
	currentSpeed(0),
	avoidanceVec(0,0,0),
	restartDelay(0),
	lastGetPathPos(0,0,0),

	pathFailures(0),
	etaFailures(0),
	nonMovingFailures(0),

	nextDeltaSpeedUpdate(0),
	nextObstacleAvoidanceUpdate(0),
	oldPhysState(CSolidObject::OnGround),
	lastTrackUpdate(0),
	mainHeadingPos(0,0,0),
	useMainHeading(false)
{
	if (owner){
		moveSquareX=(int)owner->pos.x/(SQUARE_SIZE*2);
		moveSquareY=(int)owner->pos.z/(SQUARE_SIZE*2);
	} else {
		moveSquareX = 0;
		moveSquareY = 0;
	}
}

CGroundMoveType::~CGroundMoveType()
{
	if (pathId) {
		pathManager->DeletePath(pathId);
	}
	if (owner->myTrack) {
		groundDecals->RemoveUnit(owner);
	}
}

void CGroundMoveType::PostLoad()
{
	//HACK: re-initialize path after load
	if (pathId) {
		pathId = pathManager->RequestPath(owner->mobility, owner->pos, goalPos, goalRadius, owner);
	}
}

void CGroundMoveType::Update()
{
	//Update mobility.
	owner->mobility->maxSpeed = maxSpeed;

	if(owner->transporter) {
		return;
	}
	if(OnSlope()
		&& (!floatOnWater || ground->GetHeight(owner->midPos.x, owner->midPos.z) > 0))
	{
		skidding = true;
	}

	if(skidding){
		UpdateSkid();
		return;
	}

	//set drop height when we start to drop
	if(owner->falling) {
		UpdateControlledDrop();
		return;
	}

	if (owner->stunned) {
		owner->cob->Call(COBFN_StopMoving);
		owner->speed = ZeroVector;
	} else {
#ifdef DIRECT_CONTROL_ALLOWED
		if (owner->directControl) {
			waypoint = owner->pos+owner->frontdir * 100;
			waypoint.CheckInBounds();

			if (owner->directControl->forward) {
				wantedSpeed = maxSpeed * 2;
				SetDeltaSpeed();
				owner->isMoving = true;
				owner->cob->Call(COBFN_StartMoving);
			} else {
				wantedSpeed = 0;
				SetDeltaSpeed();
				owner->isMoving = false;
				owner->cob->Call(COBFN_StopMoving);
			}
			short deltaHeading = 0;
			if (owner->directControl->left) {
				deltaHeading += (short) turnRate;
			}
			if (owner->directControl->right) {
				deltaHeading -= (short) turnRate;
			}

			ENTER_UNSYNCED;
			if (gu->directControl == owner)
				camera->rot.y += deltaHeading * PI / 32768;
			ENTER_SYNCED;

			ChangeHeading(owner->heading + deltaHeading);
		} else
#endif

		if (pathId || currentSpeed > 0.0f) {
			// TODO: Stop the unit from moving as a reaction on collision/explosion physics.
			// Initial calculations.
			currentDistanceToWaypoint = owner->pos.distance2D(waypoint);

			if (pathId && !atGoal && gs->frameNum > etaWaypoint) {
				etaFailures += 10;
				etaWaypoint = INT_MAX;
				if (DEBUG_CONTROLLER)
					logOutput.Print("eta failure %i %i %i %i %i", owner->id, pathId, !atGoal, currentDistanceToWaypoint < MinDistanceToWaypoint(), gs->frameNum > etaWaypoint);
			}
			if (pathId && !atGoal && gs->frameNum > etaWaypoint2) {
				if (owner->pos.distance2D(goalPos) > 200 || CheckGoalFeasability()) {
					etaWaypoint2 += 100;
				} else {
					if (DEBUG_CONTROLLER)
						logOutput.Print("Goal clogged up2 %i", owner->id);
					Fail();
				}
			}

			// Set direction to waypoint.
			float3 waypointDir = waypoint - owner->pos;
			waypointDir.y = 0;
			waypointDir.Normalize();


			// Has reached the waypoint? (=> arrived at goal)
			if (pathId && !atGoal && haveFinalWaypoint && (owner->pos - waypoint).SqLength2D() < SQUARE_SIZE * SQUARE_SIZE * 2) {
				Arrived();
			}

			//-- Steering --//
			// Apply obstacle avoidance.
			float3 desiredVelocity = /* waypointDir / */ ObstacleAvoidance(waypointDir);

			if(desiredVelocity != ZeroVector){
				ChangeHeading(GetHeadingFromVector(desiredVelocity.x, desiredVelocity.z));
			} else {
				SetMainHeading();
			}

			if (nextDeltaSpeedUpdate <= gs->frameNum) {
				wantedSpeed = pathId? requestedSpeed: 0;
				// If arriving at waypoint, then need to slow down, or may pass it.
				if (!owner->commandAI->HasMoreMoveCommands() &&
					currentDistanceToWaypoint < BreakingDistance(currentSpeed) + SQUARE_SIZE) {
					wantedSpeed = std::min((float) wantedSpeed, (float) (sqrt(currentDistanceToWaypoint * -owner->mobility->maxBreaking)));
				}
				wantedSpeed *= std::max(0.0f, std::min(1.0f, desiredVelocity.dot(owner->frontdir) + 0.1f));
				SetDeltaSpeed();
			}
		} else {
			SetMainHeading();
		}

		if (wantedSpeed > 0 || currentSpeed > 0) {
			currentSpeed += deltaSpeed;
			float3 tempSpeed = flatFrontDir * currentSpeed;
			owner->pos += tempSpeed;

			float wh = 0.0f;
			if (floatOnWater) {
				wh = ground->GetHeight(owner->pos.x, owner->pos.z);
				if (wh == 0.0f)
					wh =- owner->unitDef->waterline;
			} else {
				wh = ground->GetHeight2(owner->pos.x, owner->pos.z);
			}

			// need this to stop jitter when falling
			if (!(owner->falling || flying)) {
				owner->pos.y = wh;
			}
		}
	}


	if (owner->pos != oldPos) {
		// these checks must be executed even when we are stunned
		TestNewTerrainSquare();
		CheckCollision();

		float wh = 0.0f;
		if (floatOnWater) {
			wh = ground->GetHeight(owner->pos.x, owner->pos.z);
			if (wh == 0.0f)
				wh = -owner->unitDef->waterline;
		} else {
			wh = ground->GetHeight2(owner->pos.x, owner->pos.z);
		}

		if (!(owner->falling || flying)) {
			owner->pos.y = wh;
		}

		owner->speed = owner->pos - oldPos;
		owner->UpdateMidPos();
		oldPos = owner->pos;

		if (groundDecals && owner->unitDef->leaveTracks && (lastTrackUpdate < gs->frameNum - 7) &&
			((owner->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView)) {
			lastTrackUpdate = gs->frameNum;
			groundDecals->UnitMoved(owner);
		}
	} else {
		owner->speed = ZeroVector;
	}
}

void CGroundMoveType::SlowUpdate()
{
	if (owner->transporter) {
		if (progressState == Active)
			StopEngine();
		return;
	}

	// if we've strayed too far away from path, then need to reconsider
	if (progressState == Active && etaFailures > 8) {
		if (owner->pos.distance2D(goalPos) > 200 || CheckGoalFeasability()) {
			if (DEBUG_CONTROLLER)
				logOutput.Print("Unit eta failure %i", owner->id);
			StopEngine();
			StartEngine();
		} else {
			if (DEBUG_CONTROLLER)
				logOutput.Print("Goal clogged up %i", owner->id);
			Fail();
		}
	}

	// If the action is active, but not the engine and the
	// re-try-delay has passed, then start the engine.
	if (progressState == Active && !pathId && gs->frameNum > restartDelay) {
		if (DEBUG_CONTROLLER) {
			logOutput.Print("Unit restart %i", owner->id);
		}
		StartEngine();
	}

	if (!flying) {
		// just kindly move it into the map again instead of deleting
		owner->pos.CheckInBounds();
	}

	if (!(owner->falling || flying)) {
		float wh = 0.0f;

		// need the following if the ground changes
		// height while the unit is standing still
		if (floatOnWater) {
			wh = ground->GetHeight(owner->pos.x, owner->pos.z);
			if (wh == 0.0f) {
				wh = -owner->unitDef->waterline;
			}
		} else {
			wh = ground->GetHeight2(owner->pos.x, owner->pos.z);
		}
		owner->pos.y = wh;
	}

	if (!(owner->pos == oldSlowUpdatePos)) {
		oldSlowUpdatePos = owner->pos;

		int newmapSquare = ground->GetSquare(owner->pos);
		if (newmapSquare != owner->mapSquare) {
			owner->mapSquare = newmapSquare;

			loshandler->MoveUnit(owner, false);
			if (owner->hasRadarCapacity)
				radarhandler->MoveUnit(owner);

			// owner->UnBlock();
			// owner->Block();
		}
		qf->MovedUnit(owner);

		// NOTE: submarines aren't always deep enough to be fully
		// submerged (yet should always have the isUnderWater flag
		// set)
		const float s = (owner->mobility->moveType == MoveData::Ship_Move)? 0.5f: 1.0f;
		owner->isUnderWater = ((owner->pos.y + owner->height * s) < 0.0f);
	}
}


/*
Sets unit to start moving against given position with max speed.
*/
void CGroundMoveType::StartMoving(float3 pos, float goalRadius) {
	StartMoving(pos, goalRadius, maxSpeed * 2);
}


/*
Sets owner unit to start moving against given position with requested speed.
*/
void CGroundMoveType::StartMoving(float3 moveGoalPos, float goalRadius, float speed)
{
#ifdef TRACE_SYNC
	tracefile << "Start moving called: ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif

	if (progressState == Active) {
		StopEngine();
	}

	//Sets the new goal.
	goalPos = moveGoalPos;
	goalRadius = goalRadius;
	requestedSpeed = std::min(speed, maxSpeed * 2);
	requestedTurnRate = owner->mobility->maxTurnRate;
	atGoal = false;
	useMainHeading = false;

	progressState = Active;

	//Starts the engine.
	if (DEBUG_CONTROLLER) {
		logOutput << owner->id << ": StartMoving() starting engine.\n";
	}
	StartEngine();

	ENTER_UNSYNCED;
	if (owner->team == gu->myTeam) {
		// Play "activate" sound.
		int soundIdx = owner->unitDef->sounds.activate.getRandomIdx();
		if (soundIdx >= 0) {
			sound->PlayUnitActivate(
				owner->unitDef->sounds.activate.getID(soundIdx), owner,
				owner->unitDef->sounds.activate.getVolume(soundIdx));
		}
	}
	ENTER_SYNCED;
}

void CGroundMoveType::StopMoving() {
#ifdef TRACE_SYNC
	tracefile << "Stop moving called: ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif
	if(DEBUG_CONTROLLER)
		logOutput << "SMove: Action stopped." << " " << owner->id << "\n";

	StopEngine();

	useMainHeading = false;
	if(progressState != Done)
		progressState = Done;
}

void CGroundMoveType::SetDeltaSpeed(void)
{
	//Rounding of low speed.
	if(wantedSpeed == 0 && currentSpeed < 0.01f){
		currentSpeed=0;
		deltaSpeed=0;
//		nextDeltaSpeedUpdate=gs->frameNum+8;
		return;
	}

	//Wanted speed and acceleration.
	float wSpeed = maxSpeed;

	//Limiting speed and acceleration.
	if(wantedSpeed > 0) {
		float groundMod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, owner->pos,flatFrontDir);
		wSpeed *= groundMod;

		float3 goalDif=waypoint-owner->pos;
		short turn=owner->heading-GetHeadingFromVector(goalDif.x,goalDif.z);
		if(turn!=0){
			float goalLength=goalDif.Length();

			float turnSpeed=(goalLength+8)/(abs(turn)/turnRate)*0.5f;
			if(turnSpeed<wSpeed)		//make sure we can turn fast enought to get hit the goal
				wSpeed=turnSpeed;
	}
	if(wSpeed>wantedSpeed)
		wSpeed=wantedSpeed;
	} else {
		wSpeed=0;
	}
	//Limit change according to acceleration.
	float dif = wSpeed - currentSpeed;

	if (!accRate) {
		logOutput.Print("Acceleration is zero on unit %s\n",owner->unitDef->name.c_str());
		accRate=0.01f;
	}
	if(fabs(dif) < 0.05f) {		//good speed
		deltaSpeed = dif / 8;
		nextDeltaSpeedUpdate = gs->frameNum + 8;
	}
	else if(dif > 0) {				//accelerate
		if(dif < accRate) {
			deltaSpeed = dif;
			nextDeltaSpeedUpdate = gs->frameNum;
		} else {
			deltaSpeed = accRate;
			nextDeltaSpeedUpdate = (int)(gs->frameNum + std::min((float) 8, dif / accRate));
		}
	}
	else {	// decRate = 0.1*brakeRate, I guess this is for aircraft
		if(dif > -10*decRate){
			deltaSpeed = dif;
			nextDeltaSpeedUpdate = gs->frameNum + 1;
		}
		else {
			deltaSpeed = -10*decRate;
			nextDeltaSpeedUpdate = (int)(gs->frameNum + std::min((float) 8, dif / (-10*decRate)));
		}
	}
	//float3 temp=UpVector*wSpeed;
	//temp.CheckInBounds();

#ifdef TRACE_SYNC
	tracefile << "Unit delta speed: ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << deltaSpeed << " " /*<< wSpeed*/ << " " << owner->id << "\n";
#endif

}

/*
Changes the heading of the owner.
*/
void CGroundMoveType::ChangeHeading(short wantedHeading) {
	short heading = owner->heading;

	deltaHeading = wantedHeading - heading;

	if(deltaHeading>0){
		heading += std::min(deltaHeading,(short)turnRate);
	} else {
		heading += std::max((short)-turnRate,deltaHeading);
	}

	owner->frontdir = GetVectorFromHeading(heading);
	if (owner->upright) {
		owner->updir=UpVector;
		owner->rightdir=owner->frontdir.cross(owner->updir);
	} else {
		owner->updir=ground->GetNormal(owner->pos.x, owner->pos.z);
		owner->rightdir=owner->frontdir.cross(owner->updir);
		owner->rightdir.Normalize();
		owner->frontdir=owner->updir.cross(owner->rightdir);
	}
	owner->heading=heading;
	flatFrontDir=owner->frontdir;
	flatFrontDir.y=0;
	flatFrontDir.Normalize();
}

void CGroundMoveType::ImpulseAdded(void)
{
	if(owner->beingBuilt || owner->unitDef->movedata->moveType==MoveData::Ship_Move)
		return;

	float3& impulse=owner->residualImpulse;
	float3& speed=owner->speed;

	if(skidding){
		speed+=impulse;
		impulse=ZeroVector;
	}
	float3 groundNormal=ground->GetNormal(owner->pos.x,owner->pos.z);

	if(impulse.dot(groundNormal)<0)
		impulse-=groundNormal*impulse.dot(groundNormal);

	float strength=impulse.Length();
//	logOutput.Print("strength %f",strength);

	if(strength>3 || impulse.dot(groundNormal)>0.3f){
		skidding=true;
		speed+=impulse;
		impulse=ZeroVector;

		skidRotSpeed+=(gs->randFloat()-0.5f)*1500;
		skidRotPos2=0;
		skidRotSpeed2=0;
		float3 skidDir(speed);
		skidDir.y=0;
		skidDir.Normalize();
		skidRotVector=skidDir.cross(UpVector);
		oldPhysState=owner->physicalState;
		owner->physicalState= CSolidObject::Flying;
		owner->moveType->useHeading=false;

		if(speed.dot(groundNormal)>0.2f){
			flying=true;
			skidRotSpeed2=(gs->randFloat()-0.5f)*0.04f;
		}
	}
}

void CGroundMoveType::UpdateSkid(void)
{
	float3& speed=owner->speed;
	float3& pos=owner->pos;
	SyncedFloat3& midPos=owner->midPos;

	if(flying){
		speed.y+=mapInfo->map.gravity;
		if(midPos.y < 0)
			speed*=0.95f;

		float wh;
		if(floatOnWater)
			wh = ground->GetHeight(midPos.x, midPos.z);
		else
			wh = ground->GetHeight2(midPos.x, midPos.z);

		if(wh>midPos.y-owner->relMidPos.y){
			flying=false;
			skidRotSpeed+=(gs->randFloat()-0.5f)*1500;//*=0.5f+gs->randFloat();
			midPos.y=wh+owner->relMidPos.y-speed.y*0.5f;
			float impactSpeed=-speed.dot(ground->GetNormal(midPos.x,midPos.z));
			if(impactSpeed > owner->unitDef->minCollisionSpeed
				&& owner->unitDef->minCollisionSpeed >= 0)
			{
				owner->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2f,
					0, ZeroVector);
			}
		}
	} else {
		float speedf=speed.Length();
		float speedReduction=0.35f;
//		if(owner->unitDef->movedata->moveType==MoveData::Hover_Move)
//			speedReduction=0.1f;
		// does not use OnSlope() because then it could stop on an invalid path
		// location, and be teleported back.
		bool onSlope = (ground->GetSlope(owner->midPos.x, owner->midPos.z) >
			owner->unitDef->movedata->maxSlope)
			&& (!floatOnWater || ground->GetHeight(midPos.x, midPos.z) > 0);
		if (speedf < speedReduction && !onSlope) {
			//stop skidding
			currentSpeed=0;
			speed=ZeroVector;
			skidding=false;
			skidRotSpeed=0;
			owner->physicalState=oldPhysState;
			owner->moveType->useHeading=true;
			float rp=floor(skidRotPos2+skidRotSpeed2+0.5f);
			skidRotSpeed2=(rp-skidRotPos2)*0.5f;
			ChangeHeading(owner->heading);
		} else {
			if (onSlope) {
				float3 dir = ground->GetNormal(midPos.x, midPos.z);
				float3 normalForce = dir*dir.dot(UpVector*mapInfo->map.gravity);
				float3 newForce = UpVector*mapInfo->map.gravity - normalForce;
				speed+=newForce;
				speedf = speed.Length();
				speed *= 1 - (.1*dir.y);
			} else {
				speed*=(speedf-speedReduction)/speedf;
			}

			float remTime=speedf/speedReduction-1;
			float rp=floor(skidRotPos2+skidRotSpeed2*remTime+0.5f);
			skidRotSpeed2=(remTime+1 == 0 ) ? 0 : (rp-skidRotPos2)/(remTime+1);

			if(floor(skidRotPos2)!=floor(skidRotPos2+skidRotSpeed2)){
				skidRotPos2=0;
				skidRotSpeed2=0;
			}
		}

		float wh;
		if(floatOnWater)
			wh = ground->GetHeight(pos.x, pos.z);
		else
			wh = ground->GetHeight2(pos.x, pos.z);

		if(wh-pos.y < speed.y + mapInfo->map.gravity){
			speed.y += mapInfo->map.gravity;
			skidding = true; // flying requires skidding
			flying = true;
		} else if(wh-pos.y > speed.y){
			const float3& normal = ground->GetNormal(pos.x, pos.z);
			float dot = speed.dot(normal);
			if(dot > 0){
				speed*=0.95f;
			}
			else {
				speed += (normal*(fabs(speed.dot(normal)) + .1))*1.9f;
				speed*=.8;
			}
		}
	}
	CalcSkidRot();

	midPos += speed;
	pos = midPos - owner->frontdir * owner->relMidPos.z
			- owner->updir * owner->relMidPos.y
			- owner->rightdir * owner->relMidPos.x;
	CheckCollisionSkid();
}

void CGroundMoveType::UpdateControlledDrop(void)
{
	float3& speed=owner->speed;
	float3& pos=owner->pos;
	SyncedFloat3& midPos=owner->midPos;

	if(owner->falling){
		//set us upright
		owner->cob->Call("Falling"); //start/continue parachute animation

		speed.y += mapInfo->map.gravity*owner->fallSpeed;

		if(owner->speed.y > 0) //sometimes the dropped unit gets an upward force, still unsure where its coming from
			owner->speed.y = 0;

		midPos += speed;
		pos = midPos - owner->frontdir * owner->relMidPos.z
			- owner->updir * owner->relMidPos.y
			- owner->rightdir * owner->relMidPos.x;

		owner->midPos.y = owner->pos.y + owner->relMidPos.y;

		if(midPos.y < 0)
			speed*=0.90;

		float wh;

		if(floatOnWater)
			wh = ground->GetHeight(midPos.x, midPos.z);
		else
			wh = ground->GetHeight2(midPos.x, midPos.z);

		if(wh > midPos.y-owner->relMidPos.y){
			owner->falling = false;
			midPos.y = wh + owner->relMidPos.y - speed.y*0.8;
			owner->cob->Call("Landed"); //stop parachute animation
		}



	}
}

void CGroundMoveType::CheckCollisionSkid(void)
{
	float3& pos=owner->pos;
	SyncedFloat3& midPos=owner->midPos;

	vector<CUnit*> nearUnits=qf->GetUnitsExact(midPos,owner->radius);
	for(vector<CUnit*>::iterator ui=nearUnits.begin();ui!=nearUnits.end();++ui){
		CUnit* u=(*ui);
		float sqDist=(midPos-u->midPos).SqLength();
		float totRad=owner->radius+u->radius;
		if(sqDist<totRad*totRad && sqDist!=0){
			float dist=sqrt(sqDist);
			float3 dif=midPos-u->midPos;
			dif/=dist;
			if(u->mass==100000 || !u->mobility){
				float impactSpeed=-owner->speed.dot(dif);
				if(impactSpeed > 0) {
					midPos += dif*impactSpeed;
					pos=midPos-owner->frontdir*owner->relMidPos.z
						- owner->updir*owner->relMidPos.y
						- owner->rightdir*owner->relMidPos.x;
					owner->speed += dif*(impactSpeed*1.8f);
					if(impactSpeed > owner->unitDef->minCollisionSpeed
						&& owner->unitDef->minCollisionSpeed >= 0)
					{
						owner->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2f,
								0,ZeroVector);
					}
					if(impactSpeed > u->unitDef->minCollisionSpeed
						&& u->unitDef->minCollisionSpeed >= 0)
					{
						u->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2f,0,ZeroVector);
					}
				}
			} else {
				float part=owner->mass/(owner->mass+u->mass);
				float impactSpeed=(u->speed - owner->speed).dot(dif);
				if(impactSpeed > 0) {
					midPos += dif * (impactSpeed * (1 - part) * 2);
					pos = midPos - owner->frontdir*owner->relMidPos.z
						- owner->updir*owner->relMidPos.y
						- owner->rightdir*owner->relMidPos.x;
					owner->speed += dif*(impactSpeed*(1-part)*2);
					u->midPos-=dif*(impactSpeed*part * 2);
					u->pos = u->midPos - u->frontdir * u->relMidPos.z
						- u->updir*u->relMidPos.y - u->rightdir*u->relMidPos.x;
					u->speed -= dif*(impactSpeed*part*2);
					if(CGroundMoveType* mt
						= dynamic_cast<CGroundMoveType*>(u->moveType))
					{
						mt->skidding = true;
					}
					if(impactSpeed > owner->unitDef->minCollisionSpeed
						&& owner->unitDef->minCollisionSpeed >= 0)
					{
						owner->DoDamage(
							DamageArray()*impactSpeed*owner->mass*0.2f*(1-part),
							0,dif*impactSpeed*(owner->mass*(1-part)));
					}
					if(impactSpeed > u->unitDef->minCollisionSpeed
						&& u->unitDef->minCollisionSpeed >= 0)
					{
						u->DoDamage(
							DamageArray()*impactSpeed*owner->mass*0.2f*part,
							0, dif*-impactSpeed*(u->mass*part));
					}
					owner->speed*=0.9f;
				}
			}
		}
	}
	vector<CFeature*> nearFeatures=qf->GetFeaturesExact(midPos,owner->radius);
	for(vector<CFeature*>::iterator fi=nearFeatures.begin();
		fi!=nearFeatures.end();++fi)
	{
		CFeature* u=(*fi);
		if(!u->blocking)
			continue;
		float sqDist=(midPos-u->midPos).SqLength();
		float totRad=owner->radius+u->radius;
		if(sqDist<totRad*totRad && sqDist!=0){
			float dist=sqrt(sqDist);
			float3 dif=midPos-u->midPos;
			dif/=dist;
			float impactSpeed=-owner->speed.dot(dif);
			if(impactSpeed > 0){
				midPos+=dif*(impactSpeed);
				pos = midPos - owner->frontdir*owner->relMidPos.z
					- owner->updir*owner->relMidPos.y
					- owner->rightdir*owner->relMidPos.x;
				owner->speed+=dif*(impactSpeed*1.8f);
				if(impactSpeed > owner->unitDef->minCollisionSpeed
					&& owner->unitDef->minCollisionSpeed >= 0)
				{
					owner->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2f,
						0, ZeroVector);
				}
				u->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2f,
					0, -dif*impactSpeed);
			}
		}
	}
}

float CGroundMoveType::GetFlyTime(float3 pos, float3 speed)
{
	return 0;
}

void CGroundMoveType::CalcSkidRot(void)
{
	owner->heading += (short int) skidRotSpeed;

	owner->frontdir = GetVectorFromHeading(owner->heading);

	if (owner->upright) {
		owner->updir = UpVector;
		owner->rightdir = owner->frontdir.cross(owner->updir);
	} else {
		owner->updir = ground->GetSmoothNormal(owner->pos.x, owner->pos.z);
		owner->rightdir = owner->frontdir.cross(owner->updir);
		owner->rightdir.Normalize();
		owner->frontdir = owner->updir.cross(owner->rightdir);
	}

	skidRotPos2 += skidRotSpeed2;

	float cosp = cos(skidRotPos2 * PI * 2.0f);
	float sinp = sin(skidRotPos2 * PI * 2.0f);

	float3 f1 = skidRotVector * skidRotVector.dot(owner->frontdir);
	float3 f2 = owner->frontdir - f1;
	f2 = f2 * cosp + f2.cross(skidRotVector) * sinp;
	owner->frontdir = f1 + f2;

	float3 r1 = skidRotVector * skidRotVector.dot(owner->rightdir);
	float3 r2 = owner->rightdir - r1;
	r2 = r2 * cosp + r2.cross(skidRotVector) * sinp;
	owner->rightdir = r1 + r2;

	float3 u1 = skidRotVector * skidRotVector.dot(owner->updir);
	float3 u2 = owner->updir - u1;
	u2 = u2 * cosp + u2.cross(skidRotVector) * sinp;
	owner->updir = u1 + u2;
}


const float AVOIDANCE_DISTANCE = 1.0f;				// How far away a unit should start avoiding an obstacle. Multiplied with distance to waypoint.
const float AVOIDANCE_STRENGTH = 2.0f;				// How strongly an object should be avoided. Raise this value to give some more marginal.
const float FORCE_FIELD_DISTANCE = 50;				// How faar away a unit may be affected by the force-field. Multiplied with speed of the unit.
const float FORCE_FIELD_STRENGTH = 0.4f;			// Maximum strenght of the force-field.



/*
 * Dynamic obstacle avoidance, helps the unit to
 * follow the path even when it's not perfect.
 */
float3 CGroundMoveType::ObstacleAvoidance(float3 desiredDir) {
	// NOTE: based on the requirement that all objects has symetrical footprints.
	// If this is false, then radius has to be calculated in a different way!

	// Obstacle-avoidance-system only needs to be run if the unit wants to move
	if (pathId) {
		float3 avoidanceDir = desiredDir;
		// Speed-optimizer. Reduces the times this system is run.
		if (gs->frameNum >= nextObstacleAvoidanceUpdate) {
			nextObstacleAvoidanceUpdate = gs->frameNum + 4;

			// first check if the current waypoint is reachable
			int wsx = (int) waypoint.x / (SQUARE_SIZE * 2);
			int wsy = (int) waypoint.z / (SQUARE_SIZE * 2);
			int ltx = wsx - moveSquareX + 5;
			int lty = wsy - moveSquareY + 5;

			if (ltx >= 0 && ltx < 11 && lty >= 0 && lty < 11) {
				std::vector<int2>::iterator li;
				for (li = lineTable[lty][ltx].begin(); li != lineTable[lty][ltx].end(); ++li) {
					const int x = (moveSquareX + li->x) * 2;
					const int y = (moveSquareY + li->y) * 2;
					const int blockBits = CMoveMath::BLOCK_STRUCTURE |
					                      CMoveMath::BLOCK_TERRAIN   | 
					                      CMoveMath::BLOCK_MOBILE_BUSY;
					MoveData& moveData  = *owner->unitDef->movedata;
					if ((moveData.moveMath->IsBlocked(moveData, x, y) & blockBits) ||
					    (moveData.moveMath->SpeedMod(moveData, x, y) <= 0.01f)) {
						// not reachable, force a new path to be calculated next slowupdate
						++etaFailures;
						if (DEBUG_CONTROLLER) {
							logOutput.Print("Waypoint path blocked for unit %i", owner->id);
						}
						break;
					}
				}
			}

			// now we do the obstacle avoidance proper
			float currentDistanceToGoal = owner->pos.distance2D(goalPos);
			float3 rightOfPath = desiredDir.cross(float3(0.0f, 1.0f, 0.0f));
			float3 rightOfAvoid = rightOfPath;
			float speedf = owner->speed.Length2D();

			float avoidLeft = 0.0f;
			float avoidRight = 0.0f;


			vector<CSolidObject*> nearbyObjects = qf->GetSolidsExact(owner->pos, speedf * 35 + 30 + owner->xsize / 2);
			vector<CSolidObject*> objectsOnPath;
			vector<CSolidObject*>::iterator oi;

			for (oi = nearbyObjects.begin(); oi != nearbyObjects.end(); oi++) {
				CSolidObject* object = *oi;
				MoveData* moveData = owner->mobility;
				CMoveMath* moveMath = moveData->moveMath;

				// basic blocking-check
				if (object != owner && moveMath->IsBlocking(*moveData, object) && desiredDir.dot(object->pos - owner->pos) > 0) {
					float3 objectToUnit = (owner->pos - object->pos - object->speed * 30);
					float distanceToObject = objectToUnit.Length();
					float radiusSum = (owner->xsize + object->xsize) * SQUARE_SIZE / 2;

					// if object is close enough
					if (distanceToObject < speedf * 35 + 10 + radiusSum && distanceToObject < currentDistanceToGoal && distanceToObject > 0.001f) {
						// Don't divide by zero. TODO figure out why this can actually happen.
						float objectDistToAvoidDirCenter = objectToUnit.dot(rightOfAvoid);	//Positive value means "to right".
						// If object and unit in relative motion are closing in to each other (or not yet fully apart),
						// the object are in path of the unit and they are not collided.

						if (objectToUnit.dot(avoidanceDir) < radiusSum && fabs(objectDistToAvoidDirCenter) < radiusSum && (object->mobility || Distance2D(owner, object) >= 0)) {
							// Avoid collision by turning the heading to left or right.
							// Using the one who need most adjustment.
							if (DEBUG_CONTROLLER && selectedUnits.selectedUnits.find(owner) != selectedUnits.selectedUnits.end())
								geometricObjects->AddLine(owner->pos + UpVector * 20, object->pos + UpVector * 20, 3, 1, 4);

							if (objectDistToAvoidDirCenter > 0.0f) {
								avoidRight += (radiusSum - objectDistToAvoidDirCenter) * AVOIDANCE_STRENGTH / distanceToObject;
								avoidanceDir += (rightOfAvoid * avoidRight);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(float3(0.0f, 1.0f, 0.0f));
							} else {
								avoidLeft += (radiusSum - fabs(objectDistToAvoidDirCenter)) * AVOIDANCE_STRENGTH / distanceToObject;
								avoidanceDir -= (rightOfAvoid * avoidLeft);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(float3(0.0f, 1.0f, 0.0f));
							}
							objectsOnPath.push_back(object);
						}
					}

				}
			}



			// Sum up avoidance.
			avoidanceVec = (desiredDir.cross(float3(0.0f, 1.0f, 0.0f)) * (avoidRight - avoidLeft));
			if (DEBUG_CONTROLLER && selectedUnits.selectedUnits.find(owner) != selectedUnits.selectedUnits.end()) {
				int a = geometricObjects->AddLine(owner->pos + UpVector * 20, owner->pos + UpVector * 20 + avoidanceVec * 40, 7, 1, 4);
				geometricObjects->SetColor(a, 1, 0.3f, 0.3f, 0.6f);

				a = geometricObjects->AddLine(owner->pos + UpVector * 20, owner->pos + UpVector * 20 + desiredDir * 40, 7, 1, 4);
				geometricObjects->SetColor(a, 0.3f, 0.3f, 1, 0.6f);
			}
		}

		// Return the resulting recommended velocity.
		avoidanceDir = desiredDir + avoidanceVec;
		if (avoidanceDir.Length2D() > 1.0f)
			avoidanceDir.Normalize();

		return avoidanceDir;
	} else {
		return ZeroVector;
	}
}



// Calculates an aproximation of the physical 2D-distance between given two objects.
float CGroundMoveType::Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal)
{
	// calculate the distance in (x,z) depending
	// on the shape of the object footprints
	float dist2D;
	if (object1->xsize == object1->ysize || object2->xsize == object2->ysize) {
		// use xsize as a cylindrical radius.
		float3 distVec = (object1->midPos - object2->midPos);
		dist2D = distVec.Length2D() - (object1->xsize + object2->xsize) * SQUARE_SIZE / 2 + 2 * marginal;
	} else {
		// Pytagorean sum of the x and z distance.
		float3 distVec;
		float xdiff = streflop::fabs(object1->midPos.x - object2->midPos.x);
		float zdiff = streflop::fabs(object1->midPos.z - object2->midPos.z);

		distVec.x = xdiff - (object1->xsize + object2->xsize) * SQUARE_SIZE / 2 + 2 * marginal;
		distVec.z = zdiff - (object1->ysize + object2->ysize) * SQUARE_SIZE / 2 + 2 * marginal;

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

// Creates a path to the goal.
void CGroundMoveType::GetNewPath()
{
	if(owner->pos.distance2D(lastGetPathPos)<20){
		if(DEBUG_CONTROLLER)
			logOutput.Print("Non moving failure %i %i",owner->id,nonMovingFailures);
		nonMovingFailures++;
		if(nonMovingFailures>10){
			nonMovingFailures=0;
			Fail();
			pathId=0;
			return;
		}
	}else{
		lastGetPathPos=owner->pos;
		nonMovingFailures=0;
	}

	pathManager->DeletePath(pathId);
	pathId = pathManager->RequestPath(owner->mobility, owner->pos, goalPos, goalRadius, owner);
	nextWaypoint = owner->pos;
	// new path received, can't be at waypoint.
	if(pathId){
		atGoal = false;
		haveFinalWaypoint=false;
		GetNextWaypoint();
		GetNextWaypoint();
	} else {

	}

	//Sets the limit for when next path-request could be done.
	restartDelay = gs->frameNum + MAX_REPATH_FREQUENCY;
}


/*
Sets waypoint to next in path.
*/
void CGroundMoveType::GetNextWaypoint()
{
	if(pathId) {
		waypoint=nextWaypoint;
		nextWaypoint = pathManager->NextWaypoint(pathId, waypoint, 2);

		if(nextWaypoint.x != -1) {
//			logOutput.Print("New waypoint %i %f %f",owner->id,owner->pos.distance2D(newWaypoint),wantedDistanceToWaypoint);
			etaWaypoint = int(30.0f / (requestedSpeed*terrainSpeed+0.001f)) + gs->frameNum+50;
			etaWaypoint2 = int(25.0f / (requestedSpeed*terrainSpeed+0.001f)) + gs->frameNum+10;
			atGoal = false;
		} else {
			if(DEBUG_CONTROLLER)
				logOutput.Print("Path failure %i %i",owner->id,pathFailures);
			pathFailures++;
			if(pathFailures>0){
				pathFailures=0;
				Fail();
			}
			etaWaypoint = INT_MAX;
			etaWaypoint2 =INT_MAX;
			nextWaypoint=waypoint;
		}
		//If the waypoint is very close to the goal, then correct it into the goal.
		if(waypoint.distance2D(goalPos) < CPathManager::PATH_RESOLUTION){
			waypoint = goalPos;
			haveFinalWaypoint=true;
		}
	}
}


/*
The distance the unit will move at max breaking before stopping,
starting from given speed.
*/
float CGroundMoveType::BreakingDistance(float speed)
{
	if (!owner->mobility->maxBreaking) {
		logOutput << "maxBreaking is zero for unit " << owner->unitDef->name.c_str();
		return 0.0f;
	}
	return fabs(speed * speed / owner->mobility->maxBreaking);
}


/*
Gives the position this unit will end up at with full breaking
from current velocity.
*/
float3 CGroundMoveType::Here()
{
	float3 motionDir = owner->speed;
	if(motionDir.SqLength2D() == 0) {
		return owner->midPos;
	} else {
		motionDir.Normalize();
		return owner->midPos + (motionDir * BreakingDistance(owner->speed.Length2D()));
	}
}


/*
Gives the minimum distance to next waypoint that should be used,
based on current speed.
*/
float CGroundMoveType::MinDistanceToWaypoint()
{
	return BreakingDistance(owner->speed.Length2D()) + CPathManager::PATH_RESOLUTION;
}


/*
Gives the maximum distance from it's waypoint a unit could be
before a new path is requested.
*/
float CGroundMoveType::MaxDistanceToWaypoint()
{
	return MinDistanceToWaypoint() * MAX_OFF_PATH_FACTOR;
}

/*
Initializes motion.
*/
void CGroundMoveType::StartEngine() {
	// ran only if the unit has no path and is not already at goal
	if (!pathId && !atGoal) {
		GetNewPath();

		// activate "engine" only if a path was found
		if (pathId) {
			pathFailures = 0;
			etaFailures = 0;
			owner->isMoving = true;
			owner->cob->Call(COBFN_StartMoving);

			if (DEBUG_CONTROLLER) {
				logOutput << "Engine started" << " " << owner->id << "\n";
			}
		} else {
			if (DEBUG_CONTROLLER) {
				logOutput << "Engine start failed: " << owner->id << "\n";
			}

			Fail();
		}
	}

	nextObstacleAvoidanceUpdate = gs->frameNum;
	//SetDeltaSpeed();
}

/*
Stops motion.
*/
void CGroundMoveType::StopEngine() {
	// ran only if engine is active
	if (pathId) {
		// Deactivating engine.
		pathManager->DeletePath(pathId);
		pathId = 0;
		if (!atGoal) {
			waypoint = Here();
		}

		// Stop animation.
		owner->cob->Call(COBFN_StopMoving);

		if (DEBUG_CONTROLLER) {
			logOutput << "Engine stopped." << " " << owner->id << "\n";
		}
	}
	owner->isMoving = false;
	wantedSpeed = 0;
	// SetDeltaSpeed();
}


/*
Called when the unit arrives at it's waypoint.
*/
void CGroundMoveType::Arrived()
{
	//Can only "arrive" if the engine is active.
	if(progressState == Active) {
		//Have reached waypoint.
		atGoal = true;

		StopEngine();

		// Play "arrived" sound.
		ENTER_UNSYNCED;
		if (owner->team == gu->myTeam) {
			int soundIdx = owner->unitDef->sounds.arrived.getRandomIdx();
			if (soundIdx >= 0) {
				sound->PlayUnitReply(
					owner->unitDef->sounds.arrived.getID(soundIdx), owner,
					owner->unitDef->sounds.arrived.getVolume(soundIdx));
			}
		}
		ENTER_SYNCED;

		//And the action is done.
		progressState = Done;
		owner->commandAI->SlowUpdate();

		if(DEBUG_CONTROLLER)
			logOutput << "Unit arrived!\n";
	}
}

/*
Makes the unit fail this action.
No more trials will be done before a new goal is given.
*/
void CGroundMoveType::Fail()
{
	if(DEBUG_CONTROLLER)
		logOutput.Print("Unit failed! %i",owner->id);

	StopEngine();

	//Failure of finding a path means that this action
	//has failed to reach it's goal.
	progressState = Failed;

	globalAI->UnitMoveFailed(owner);

	//Sends a message to user.
	ENTER_UNSYNCED;
	if (owner->team == gu->myTeam) {
		// Playing "can't" sound.
		int soundIdx = owner->unitDef->sounds.cant.getRandomIdx();
		if (soundIdx >= 0) {
			sound->PlayUnitReply(
				owner->unitDef->sounds.cant.getID(soundIdx), owner,
				owner->unitDef->sounds.cant.getVolume(soundIdx));
		}

		if (!owner->commandAI->unimportantMove && owner->pos.distance(goalPos) > goalRadius + 150) {
			if (gs->frameNum % (GAME_SPEED * 3) == 0) {
				logOutput << owner->unitDef->humanName.c_str() << ": Can't reach destination!\n";
				logOutput.SetLastMsgPos(owner->pos);
			}
		}
	}
	ENTER_SYNCED;
}



void CGroundMoveType::CheckCollision(void)
{
	int2 newmp = owner->GetMapPos();

	if (newmp.x != owner->mapPos.x || newmp.y != owner->mapPos.y) {
		// now make sure we don't overrun any other units
		bool haveCollided = false;
		int retest = 0;

		do {
			const float zmove = (owner->mapPos.y + owner->ysize / 2) * SQUARE_SIZE;
			const float xmove = (owner->mapPos.x + owner->xsize / 2) * SQUARE_SIZE;

			if (fabs(owner->frontdir.x) > fabs(owner->frontdir.z)) {
				if (newmp.y < owner->mapPos.y) {
					haveCollided |= CheckColV(newmp.y, newmp.x, newmp.x + owner->xsize - 1,  zmove - 3.99f, owner->mapPos.y);
					newmp = owner->GetMapPos();
				} else if (newmp.y > owner->mapPos.y) {
					haveCollided |= CheckColV(newmp.y + owner->ysize - 1, newmp.x, newmp.x + owner->xsize - 1,  zmove + 3.99f, owner->mapPos.y + owner->ysize - 1);
					newmp = owner->GetMapPos();
				}
				if (newmp.x < owner->mapPos.x) {
					haveCollided |= CheckColH(newmp.x, newmp.y, newmp.y + owner->ysize - 1,  xmove - 3.99f, owner->mapPos.x);
					newmp = owner->GetMapPos();
				} else if (newmp.x > owner->mapPos.x) {
					haveCollided |= CheckColH(newmp.x + owner->xsize - 1, newmp.y, newmp.y + owner->ysize - 1,  xmove + 3.99f, owner->mapPos.x + owner->xsize - 1);
					newmp = owner->GetMapPos();
				}
			} else {
				if (newmp.x < owner->mapPos.x) {
					haveCollided |= CheckColH(newmp.x, newmp.y, newmp.y + owner->ysize - 1,  xmove - 3.99f, owner->mapPos.x);
					newmp = owner->GetMapPos();
				} else if (newmp.x > owner->mapPos.x) {
					haveCollided |= CheckColH(newmp.x + owner->xsize - 1, newmp.y, newmp.y + owner->ysize - 1,  xmove + 3.99f, owner->mapPos.x + owner->xsize - 1);
					newmp = owner->GetMapPos();
				}
				if (newmp.y < owner->mapPos.y) {
					haveCollided |= CheckColV(newmp.y, newmp.x, newmp.x + owner->xsize - 1,  zmove - 3.99f, owner->mapPos.y);
					newmp = owner->GetMapPos();
				} else if (newmp.y > owner->mapPos.y) {
					haveCollided |= CheckColV(newmp.y + owner->ysize - 1, newmp.x, newmp.x + owner->xsize - 1,  zmove + 3.99f, owner->mapPos.y + owner->ysize - 1);
					newmp = owner->GetMapPos();
				}
			}
			++retest;
		}
		while (haveCollided && retest < 2);

		// owner->UnBlock();
		owner->Block();
	}

	return;
}


bool CGroundMoveType::CheckColH(int x, int y1, int y2, float xmove, int squareTestX)
{
	for (int y = y1; y <= y2; ++y) {
		CSolidObject* c = groundBlockingObjectMap->GroundBlockedUnsafe(y * gs->mapx + x);

		if (c) {
			if (groundBlockingObjectMap->GroundBlockedUnsafe(y * gs->mapx + squareTestX) != NULL &&
				groundBlockingObjectMap->GroundBlockedUnsafe(y * gs->mapx + squareTestX) != owner) {
				continue;
			}
			if (c->mobility) {
				// if other party is mobile, start to skuff it out of the way
				float part = owner->mass / (owner->mass + c->mass * 2);
				float3 dif = c->pos - owner->pos;
				float dl = dif.Length();
				float colDepth = fabs(owner->pos.x - xmove);
				dif *= dl != 0 ? colDepth / dl : 0;

				// adjust our own position a bit so
				// we have to turn less (FIXME: can
				// place us in building)
				owner->pos -= dif * (1 - part);
				// safe cast (only units can be mobile)
				CUnit* u = (CUnit*) c;

				const int uAllyTeam = u->allyteam;
				const int oAllyTeam = owner->allyteam;
				const bool allied = (gs->Ally(uAllyTeam, oAllyTeam) || gs->Ally(oAllyTeam, uAllyTeam));

				if (!u->unitDef->pushResistant && !u->usingScriptMoveType && allied) {
					// push the blocking unit out of the way
					// FIXME CAN PLACE OTHER PARTY IN BUILDING
					u->pos += dif * (part);
					u->UpdateMidPos();
				}

				if (!(gs->frameNum + owner->id & 31) && !owner->commandAI->unimportantMove) {
					// if we (MT owner) are doing something important, tell units around us to bugger off
					helper->BuggerOff(owner->pos + owner->frontdir * owner->radius, owner->radius, owner);
				}
			}

			MoveData* m = owner->mobility;

			// if other party can be overrun then overrun it
			if (!m->moveMath->IsBlocking(*m, c)) {
				float3 fix = owner->frontdir * currentSpeed * 200;
				c->Kill(fix);
			}

			// hack to make units find openings easier until the pathfinder can do it itself
			// FIXME CAN PLACE US IN BUILDING
			if (groundBlockingObjectMap->GroundBlockedUnsafe(y1 * gs->mapx + x) == NULL)
				owner->pos.z -= fabs(owner->pos.x - xmove) * 0.5f;
			if (groundBlockingObjectMap->GroundBlockedUnsafe(y2 * gs->mapx + x) == NULL)
				owner->pos.z += fabs(owner->pos.x - xmove) * 0.5f;

			owner->pos.x = xmove;
			currentSpeed *= 0.97f;
			return true;
		}
	}

	return false;
}

bool CGroundMoveType::CheckColV(int y, int x1, int x2, float zmove, int squareTestY)
{
	for (int x = x1; x <= x2; ++x) {
		CSolidObject* c = groundBlockingObjectMap->GroundBlockedUnsafe(y * gs->mapx + x);

		if (c) {
			if (groundBlockingObjectMap->GroundBlockedUnsafe(squareTestY * gs->mapx + x) != NULL &&
				groundBlockingObjectMap->GroundBlockedUnsafe(squareTestY * gs->mapx + x) != owner) {
				continue;
			}
			if (c->mobility) {
				// if other party is mobile, start to skuff it out of the way
				float part = owner->mass / (owner->mass + c->mass * 2);
				float3 dif = c->pos - owner->pos;
				float dl = dif.Length();
				float colDepth = fabs(owner->pos.z - zmove);
				dif *= dl != 0 ? colDepth / dl : 0;

				// adjust our own position a bit so
				// we have to turn less (FIXME: can
				// place us in building)
				owner->pos -= dif * (1 - part);
				// safe cast (only units can be mobile)
				CUnit* u = (CUnit*) c;

				const int uAllyTeam = u->allyteam;
				const int oAllyTeam = owner->allyteam;
				const bool allied = (gs->Ally(uAllyTeam, oAllyTeam) || gs->Ally(oAllyTeam, uAllyTeam));

				if (!u->unitDef->pushResistant && !u->usingScriptMoveType && allied) {
					// push the blocking unit out of the way
					// FIXME CAN PLACE OTHER PARTY IN BUILDING
					c->pos += dif * (part);
					u->UpdateMidPos();
				}

				if (!(gs->frameNum + owner->id & 31) && !owner->commandAI->unimportantMove) {
					// if we (MT owner) are doing something important, tell units around us to bugger off
					helper->BuggerOff(owner->pos + owner->frontdir * owner->radius, owner->radius, owner);
				}
			}

			MoveData* m = owner->mobility;

			// if other party can be overrun then overrun it
			if (!m->moveMath->IsBlocking(*m, c)) {
				float3 fix = owner->frontdir * currentSpeed * 200;
				c->Kill(fix);
			}

			// hack to make units find openings easier until the pathfinder can do it itself
			// FIXME CAN PLACE US IN BUILDING
			if (groundBlockingObjectMap->GroundBlockedUnsafe(y * gs->mapx + x1) == NULL)
				owner->pos.x -= fabs(owner->pos.z - zmove) * 0.5f;
			if (groundBlockingObjectMap->GroundBlockedUnsafe(y * gs->mapx + x2) == NULL)
				owner->pos.x += fabs(owner->pos.z - zmove) * 0.5f;

			owner->pos.z = zmove;
			currentSpeed *= 0.97f;
			return true;
		}
	}

	return false;
}



//creates the tables used to see if we should advance to next pathfinding waypoint
void CGroundMoveType::CreateLineTable(void)
{
	lineTable = new std::vector<int2>[11][11];

	for(int yt=0;yt<11;++yt){
		for(int xt=0;xt<11;++xt){
			float3 start(0.5f,0,0.5f);
			float3 to((xt-5)+0.5f,0,(yt-5)+0.5f);

			float dx=to.x-start.x;
			float dz=to.z-start.z;
			float xp=start.x;
			float zp=start.z;
			float xn,zn;

			if(floor(start.x)==floor(to.x)){
				if(dz>0)
					for(int a=1;a<floor(to.z);++a)
						lineTable[yt][xt].push_back(int2(0,a));
				else
					for(int a=-1;a>floor(to.z);--a)
						lineTable[yt][xt].push_back(int2(0,a));
			} else if(floor(start.z)==floor(to.z)){
				if(dx>0)
					for(int a=1;a<floor(to.x);++a)
						lineTable[yt][xt].push_back(int2(a,0));
				else
					for(int a=-1;a>floor(to.x);--a)
						lineTable[yt][xt].push_back(int2(a,0));
			} else {
				bool keepgoing=true;
				while(keepgoing){
					if(dx>0){
						xn=(floor(xp)+1-xp)/dx;
					} else {
						xn=(floor(xp)-xp)/dx;
					}
					if(dz>0){
						zn=(floor(zp)+1-zp)/dz;
					} else {
						zn=(floor(zp)-zp)/dz;
					}

					if(xn<zn){
						xp+=(xn+0.0001f)*dx;
						zp+=(xn+0.0001f)*dz;
					} else {
						xp+=(zn+0.0001f)*dx;
						zp+=(zn+0.0001f)*dz;
					}
					keepgoing=fabs(xp-start.x)<fabs(to.x-start.x) && fabs(zp-start.z)<fabs(to.z-start.z);

					lineTable[yt][xt].push_back(int2((int)(floor(xp)),(int)(floor(zp))));
				}
				lineTable[yt][xt].pop_back();
				lineTable[yt][xt].pop_back();
			}
		}
	}
}

void CGroundMoveType::DeleteLineTable(void)
{
	delete [] lineTable;
	lineTable = 0;
}

void CGroundMoveType::TestNewTerrainSquare(void)
{
	// first make sure we dont go into any terrain we cant get out of
	int newMoveSquareX = (int) owner->pos.x / (SQUARE_SIZE * 2);
	int newMoveSquareY = (int) owner->pos.z / (SQUARE_SIZE * 2);

	if (newMoveSquareX != moveSquareX || newMoveSquareY != moveSquareY) {
		float cmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, moveSquareX * 2, moveSquareY * 2);

		if (fabs(owner->frontdir.x) < fabs(owner->frontdir.z)) {
			if (newMoveSquareX > moveSquareX) {
				float nmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					owner->pos.x = moveSquareX * SQUARE_SIZE * 2 + (SQUARE_SIZE * 2 - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				float nmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					owner->pos.x = moveSquareX * SQUARE_SIZE * 2 + 0.01f;
					newMoveSquareX = moveSquareX;
				}
			}
			if (newMoveSquareY > moveSquareY) {
				float nmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					owner->pos.z = moveSquareY * SQUARE_SIZE * 2 + (SQUARE_SIZE * 2 - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				float nmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					owner->pos.z = moveSquareY * SQUARE_SIZE * 2 + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}
		} else {
			if (newMoveSquareY > moveSquareY) {
				float nmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if (cmod>0.01f && nmod <= 0.01f) {
					owner->pos.z = moveSquareY * SQUARE_SIZE * 2 + (SQUARE_SIZE * 2 - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				float nmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					owner->pos.z = moveSquareY * SQUARE_SIZE * 2 + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}

			if (newMoveSquareX > moveSquareX) {
				float nmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					owner->pos.x = moveSquareX * SQUARE_SIZE * 2 + (SQUARE_SIZE * 2 - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				float nmod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					owner->pos.x = moveSquareX * SQUARE_SIZE * 2 + 0.01f;
					newMoveSquareX = moveSquareX;
				}
			}
		}

		if (newMoveSquareX != moveSquareX || newMoveSquareY != moveSquareY) {
			moveSquareX = newMoveSquareX;
			moveSquareY = newMoveSquareY;
			terrainSpeed = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, moveSquareX * 2, moveSquareY * 2);
			etaWaypoint = int(30.0f / (requestedSpeed * terrainSpeed + 0.001f)) + gs->frameNum + 50;
			etaWaypoint2 = int(25.0f / (requestedSpeed * terrainSpeed + 0.001f)) + gs->frameNum + 10;

			// if we have moved check if we can get a new waypoint
			int nwsx = (int) nextWaypoint.x / (SQUARE_SIZE * 2);
			int nwsy = (int) nextWaypoint.z / (SQUARE_SIZE * 2);
			int numIter = 0;

			// lowered the original 6 absolute distance to slightly more than 4.5f euclidian distance
			// to fix units getting stuck in buildings --tvo
			// My first fix set it to 21, as the pathfinding was still considered broken by many I reduced it to 11 (arbitrarily)
			// Does anyone know whether lowering this constant has any adverse side effects? Like e.g. more CPU usage? --tvo
			while ((nwsx - moveSquareX) * (nwsx - moveSquareX) + (nwsy - moveSquareY) * (nwsy - moveSquareY) < 11 && !haveFinalWaypoint && pathId) {
				int ltx = nwsx - moveSquareX + 5;
				int lty = nwsy - moveSquareY + 5;
				bool wpOk = true;

				for (std::vector<int2>::iterator li = lineTable[lty][ltx].begin(); li != lineTable[lty][ltx].end(); ++li) {
					int x = (moveSquareX + li->x) * 2;
					int y = (moveSquareY + li->y) * 2;
					int blockMask =
						(CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN |
						CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOBILE_BUSY);
					CMoveMath* mmath = owner->unitDef->movedata->moveMath;

					if ((mmath->IsBlocked(*owner->unitDef->movedata, x, y) & blockMask) ||
						mmath->SpeedMod(*owner->unitDef->movedata, x, y) <= 0.01f) {
						wpOk = false;
						break;
					}
				}
				if (!wpOk || numIter > 6) {
					break;
				}
				GetNextWaypoint();
				nwsx = (int) nextWaypoint.x / (SQUARE_SIZE * 2);
				nwsy = (int) nextWaypoint.z / (SQUARE_SIZE * 2);
				++numIter;
			}
		}
	}
}

bool CGroundMoveType::CheckGoalFeasability(void)
{
	float goalDist = goalPos.distance2D(owner->pos);

	int minx = (int) std::max(0.0f, (goalPos.x - goalDist) / (SQUARE_SIZE * 2));
	int minz = (int) std::max(0.0f, (goalPos.z - goalDist) / (SQUARE_SIZE * 2));
	int maxx = (int) std::min(float(gs->hmapx - 1), (goalPos.x + goalDist) / (SQUARE_SIZE * 2));
	int maxz = (int) std::min(float(gs->hmapy - 1), (goalPos.z + goalDist) / (SQUARE_SIZE * 2));

	MoveData* md = owner->unitDef->movedata;
	CMoveMath* mm = md->moveMath;

	float numBlocked = 0.0f;
	float numSquares = 0.0f;

	for (int z = minz; z <= maxz; ++z) {
		for (int x = minx; x <= maxx; ++x) {
			float3 pos(x * SQUARE_SIZE * 2, 0, z * SQUARE_SIZE * 2);
			if ((pos - goalPos).SqLength2D() < goalDist * goalDist) {
				int blockingType = mm->SquareIsBlocked(*md, x * 2, z * 2);

				if ((blockingType & CMoveMath::BLOCK_STRUCTURE) || mm->SpeedMod(*md, x * 2, z * 2) < 0.01f) {
					numBlocked += 0.3f;
					numSquares += 0.3f;
				} else {
					numSquares += 1.0f;
					if (blockingType) {
						numBlocked += 1.0f;
					}
				}
			}
		}
	}

	if (numSquares > 0.0f) {
		float partBlocked = numBlocked / numSquares;
		if (DEBUG_CONTROLLER) {
			logOutput.Print("Percentage of blocked squares for unit %i: %.0f%% (goal distance: %.0f)",
				owner->id, partBlocked * 100.0f, goalDist);
		}
		if (partBlocked > 0.4f) {
			return false;
		}
	}
	return true;
}

void CGroundMoveType::LeaveTransport(void)
{
	oldPos = owner->pos + UpVector * 0.001f;
}

void CGroundMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive){
	mainHeadingPos = pos;
	useMainHeading = aggressive;
	if(useMainHeading && !owner->weapons.empty()){
		if(!owner->weapons[0]->weaponDef->waterweapon && mainHeadingPos.y <= 1){
			mainHeadingPos.y = 1;
		}
		float3 dir1 = owner->weapons.front()->mainDir;
		dir1.y = 0;
		dir1.Normalize();
		float3 dir2 = mainHeadingPos-owner->pos;
		dir2.y = 0;
		dir2.Normalize();
		if(dir2 != ZeroVector){
			short heading = GetHeadingFromVector(dir2.x,dir2.z)
				- GetHeadingFromVector(dir1.x,dir1.z);
			if(owner->heading != heading
					&& !(owner->weapons.front()->TryTarget(
					mainHeadingPos, true, 0))){
				progressState = Active;
			}
		}
	}
}

void CGroundMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive){
	mainHeadingPos = unit->pos;
	useMainHeading = aggressive;
	if(useMainHeading
			&& !owner->weapons.empty()
			&& (this-owner->weapons[0]->weaponDef->waterweapon
			|| mainHeadingPos.y >= 0)){
		float3 dir1 = owner->weapons.front()->mainDir;
		dir1.y = 0;
		dir1.Normalize();
		float3 dir2 = mainHeadingPos-owner->pos;
		dir2.y = 0;
		dir2.Normalize();
		if(dir2 != ZeroVector){
			short heading = GetHeadingFromVector(dir2.x,dir2.z)
				- GetHeadingFromVector(dir1.x,dir1.z);
			if(owner->heading != heading
					&& !(owner->weapons.front()->TryTarget(
					mainHeadingPos, true, 0))){
				progressState = Active;
			}
		}
	}
}

/**
* @brief Orients owner so that weapon[0]'s arc includes mainHeadingPos
*/
void CGroundMoveType::SetMainHeading(){
	if(useMainHeading && !owner->weapons.empty()){
		float3 dir1 = owner->weapons.front()->mainDir;
		dir1.y = 0;
		dir1.Normalize();
		float3 dir2 = mainHeadingPos-owner->pos;
		dir2.y = 0;
		dir2.Normalize();
		if(dir2 != ZeroVector){
			short heading = GetHeadingFromVector(dir2.x,dir2.z)
				- GetHeadingFromVector(dir1.x,dir1.z);

			if(progressState == Active && owner->heading == heading){
				//logOutput.Print("Stop turning");
				owner->cob->Call(COBFN_StopMoving);
				progressState = Done;
			} else if(progressState == Active){
				ChangeHeading(heading);
				//logOutput.Print("Test headding: %d,  Real headding: %d", heading,
				//	owner->heading);
			} else if(progressState != Active
			  && owner->heading != heading
			  && !owner->weapons.front()->TryTarget(mainHeadingPos, true,0)){
				progressState = Active;
				owner->cob->Call(COBFN_StartMoving);
				ChangeHeading(heading);
				//logOutput.Print("Start Moving");
			} else {
				//logOutput.Print("No set main headding");
			}
		} else {
			//logOutput.Print("Zero Vector");
		}
	}
}

void CGroundMoveType::SetMaxSpeed(float speed)
{
	if (requestedSpeed == maxSpeed * 2)
		requestedSpeed = speed * 2;	//why the *2 everywhere?
	maxSpeed = speed;
}

bool CGroundMoveType::OnSlope(){
	return owner->unitDef->slideTolerance >= 1
		&& (ground->GetSlope(owner->midPos.x, owner->midPos.z) >
		owner->unitDef->movedata->maxSlope*owner->unitDef->slideTolerance);
}

void CGroundMoveType::StartSkidding(){
	skidding = true;
}

void CGroundMoveType::StartFlying() {
	skidding = true; // flying requires skidding
	flying = true;
}
