#include "StdAfx.h"
#include "groundmovetype.h"
#include "Ground.h"
#include "QuadField.h"
#include "ReadMap.h"
#include "LosHandler.h"
#include "GameHelper.h"
#include "myMath.h"
#include "InfoConsole.h"
#include "UnitHandler.h"
#include "SyncTracer.h"
#include "3DOParser.h"
#include "CobInstance.h"
#include "CobFile.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "UnitDef.h"
#include "Sound.h"
#include "RadarHandler.h"
#include "PathManager.h"
#include "Player.h"
#include "Camera.h"
#include "CommandAI.h"
#include "Mobility.h"
//#include "mmgr.h"

const unsigned int MAX_REPATH_FREQUENCY = 30;		//The minimum of frames between two full path-requests.
const unsigned int FAILURE_RETRIALS = 5;			//The number of retrials a unit will make before failing it's action.

const float ETA_ESTIMATION = 1.5;					//How much time the unit are given to reach the waypoint.
const float MAX_WAYPOINT_DISTANCE_FACTOR = 2.0;		//Used to tune how often new waypoints are requested. Multiplied with MinDistanceToWaypoint().
const float MAX_OFF_PATH_FACTOR = 20;				//How far away from a waypoint a unit could be before a new path is requested.

const float MINIMUM_SPEED = 0.01;					//Minimum speed a unit may move in.

static const bool DEBUG_CONTROLLER=false;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroundMoveType::CGroundMoveType(CUnit* owner)
:	CMoveType(owner),
	accRate(0.01f),
	turnRate(0.1f),
	baseTurnRate(0.1f),
//	maxSpeed(0.2f),
	wantedSpeed(0),
	moveType(0),
	oldPos(owner->pos),
	oldSlowUpdatePos(oldPos),
	flatFrontDir(1,0,0),
	deltaSpeed(0),
	deltaHeading(0),
	skidding(false),
	flying(false),
	skidRotSpeed(0),
	floatOnWater(false),
	skidRotVector(UpVector),
	skidRotSpeed2(0),
	skidRotPos2(0),
	
	pathId(0),
	goal(0,0,0),
	goalRadius(0),
	waypoint(0,0,0),
	etaWaypoint(0),
	atWaypoint(false),
	waypointIsBlocked(false),
	haveFinalWaypoint(false),
	requestedSpeed(0),
	requestedTurnRate(0),
	currentDistanceToWaypoint(0),
	currentSpeed(0),
	avoidanceVec(0,0,0),
	restartDelay(0),
	lastGetPathPos(0,0,0),

	getPathFailures(0),
	pathFailures(0),
	speedFailures(0),
	etaFailures(0),
	nonMovingFailures(0),
	obstacleFailures(0),
	blockingFailures(0),

	nextDeltaSpeedUpdate(0),
	nextObstacleAvoidanceUpdate(0),
	oldPhysState(CSolidObject::OnGround)
{
	moveSquareX=(int)owner->pos.x/SQUARE_SIZE;
	moveSquareY=(int)owner->pos.z/SQUARE_SIZE;
}

CGroundMoveType::~CGroundMoveType()
{
	if(pathId)
		pathManager->DeletePath(pathId);
}

void CGroundMoveType::Update()
{
	//Update mobility.
	owner->mobility->maxSpeed = maxSpeed;

	
	if(owner->inTransport)
		return;

	if(skidding){
		UpdateSkid();
		return;
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if(owner->directControl){
		if(owner->directControl->forward){
			wantedSpeed=maxSpeed;
			SetDeltaSpeed();
			owner->isMoving = true;
			owner->cob->Call(COBFN_StartMoving);
		} else {
			wantedSpeed=0;
			SetDeltaSpeed();
			owner->isMoving = false;
			owner->cob->Call(COBFN_StopMoving);
		}
		short deltaHeading=0;
		if(owner->directControl->left){
			deltaHeading+=turnRate;
		}
		if(owner->directControl->right){
			deltaHeading-=turnRate;
		}
		
		ENTER_UNSYNCED;
		if(gu->directControl==owner)
			camera->rot.y+=deltaHeading*PI/32768;
		ENTER_SYNCED;

		ChangeHeading(owner->heading+deltaHeading);
	} else
#endif
	{
	//The system should only be runned if there is a need for it.
	if(pathId || currentSpeed > 0.0) {	//TODO: Stop the unit from moving as a reaction on collision/explosion physics.
		//Initial calculations.
		currentDistanceToWaypoint = owner->pos.distance2D(waypoint);

		//-- Waypoint --//
		//If close to waypoint, waypoint blocked or ETA has passed, then get next one in path.
		if(pathId && !atWaypoint &&
		(waypointIsBlocked || currentDistanceToWaypoint < MinDistanceToWaypoint() || gs->frameNum > etaWaypoint)) {
			if(gs->frameNum > etaWaypoint)
				etaFailures++;
			else
				etaFailures=0;
			if(waypointIsBlocked){
				blockingFailures++;
				if(DEBUG_CONTROLLER)
					info->AddLine("Blocking failure %i %i %i %i %i %i",owner->id,pathId, !atWaypoint,waypointIsBlocked, currentDistanceToWaypoint < MinDistanceToWaypoint(), gs->frameNum > etaWaypoint);
				if(blockingFailures>5+owner->pos.distance2D(goal)*0.5){
					Fail();
					blockingFailures=0;
				}
			} else {
				blockingFailures=0;
			}
			if(DEBUG_CONTROLLER)
				info->AddLine("new waypoint %i %i %i %i %i %i",owner->id,pathId, !atWaypoint,waypointIsBlocked, currentDistanceToWaypoint < MinDistanceToWaypoint(), gs->frameNum > etaWaypoint);
			GetNextWaypoint();
			currentDistanceToWaypoint = owner->pos.distance2D(waypoint);
		}

		//Set direction to waypoint.
		float3 waypointDir = waypoint - owner->pos;
		waypointDir.y = 0;
		waypointDir.Normalize();


		//Has reached the waypoint? (=> arrived at goal)
		if(pathId && !atWaypoint && haveFinalWaypoint && (owner->pos - waypoint).SqLength2D() < SQUARE_SIZE*SQUARE_SIZE)
			Arrived();


		//-- Steering --//
		//Apply obstacle avoidance.
		float3 desiredVelocity = ObstacleAvoidance(waypointDir);

		if(desiredVelocity != ZeroVector)
			ChangeHeading(GetHeadingFromVector(desiredVelocity.x, desiredVelocity.z));

		if(nextDeltaSpeedUpdate<=gs->frameNum){
			wantedSpeed = pathId ? requestedSpeed : 0;
			//If arriving at waypoint, then need to slow down, or may pass it.
			if(currentDistanceToWaypoint < BreakingDistance(currentSpeed) + SQUARE_SIZE) {
				wantedSpeed = min(wantedSpeed, (sqrt(currentDistanceToWaypoint * -owner->mobility->maxBreaking)));
			}
			wantedSpeed*=max(0.,min(1.,desiredVelocity.dot(owner->frontdir)+0.1));
			SetDeltaSpeed();
		}
	}
	}

	if(wantedSpeed>0 || currentSpeed>0) {

		currentSpeed += deltaSpeed;
		float3 tempSpeed = flatFrontDir * currentSpeed;

		owner->pos += tempSpeed;

		float wh;
		if(floatOnWater){
			wh = ground->GetHeight(owner->pos.x, owner->pos.z);
			if(wh==0)
				wh=-owner->unitDef->waterline;
		} else {
			wh = ground->GetHeight2(owner->pos.x, owner->pos.z);
		}
		owner->pos.y=wh;
	}

	if(owner->pos!=oldPos){
		CheckCollision();
		float wh;
		if(floatOnWater){
			wh = ground->GetHeight(owner->pos.x, owner->pos.z);
			if(wh==0)
				wh=-owner->unitDef->waterline;
		} else {
			wh = ground->GetHeight2(owner->pos.x, owner->pos.z);
		}
		owner->pos.y=wh;

		owner->speed=owner->pos-oldPos;
		owner->midPos = owner->pos + owner->frontdir * owner->relMidPos.z + owner->updir * owner->relMidPos.y + owner->rightdir * owner->relMidPos.x;
		oldPos=owner->pos;
	} else {
		owner->speed=ZeroVector;
	}
}

void CGroundMoveType::SlowUpdate()
{
	if(owner->inTransport){
		if(progressState == Active)
			StopEngine();
		return;
	}

	//If got too far away from path, then need to reconsider.
	if(progressState == Active && owner->midPos.distance2D(waypoint) > MaxDistanceToWaypoint() || etaFailures>2) {
		if(DEBUG_CONTROLLER)
			info->AddLine("Unit drifted %i",owner->id);
		StopEngine();
		StartEngine();
	}

	//If the action is active, but not the engine and the
	//re-try-delay has passed, then start the engine.
	if(progressState == Active && !pathId && gs->frameNum > restartDelay) {
		if(DEBUG_CONTROLLER)
			info->AddLine("Unit restart %i",owner->id);
		StartEngine();
	}

	owner->pos.CheckInBounds();		//just kindly move it into the map again instead of deleteing
/*	if(owner->pos.z<0 || owner->pos.z>(gs->mapy+1)*SQUARE_SIZE || owner->pos.x<0 || owner->pos.x>(gs->mapx+1)*SQUARE_SIZE){
		info->AddLine("Deleting unit due to bad coord %i %f %f %f %s",owner->id,owner->pos.x,owner->pos.y,owner->pos.z,owner->unitDef->humanName.c_str());
		uh->DeleteUnit(owner);
	}*/

	if(!(owner->pos==oldSlowUpdatePos)){
		oldSlowUpdatePos=owner->pos;

		int newmapSquare=ground->GetSquare(owner->pos);
		if(newmapSquare!=owner->mapSquare){
			owner->mapSquare=newmapSquare;

			loshandler->MoveUnit(owner,false);
			if(owner->radarRadius || owner->jammerRadius || owner->sonarRadius)
				radarhandler->MoveUnit(owner);

//			owner->UnBlock();
//			owner->Block();
		}
		qf->MovedUnit(owner);
		owner->isUnderWater=owner->pos.y+owner->height<0;
	}
}


/*
Sets unit to start moving against given position with max speed.
*/
void CGroundMoveType::StartMoving(float3 pos, float goalRadius) {
	StartMoving(pos, goalRadius, maxSpeed);
}


/*
Sets owner unit to start moving against given position with requested speed.
*/
void CGroundMoveType::StartMoving(float3 moveGoalPos, float goalRadius,  float speed) 
{
#ifdef TRACE_SYNC
	tracefile << "Start moving called: ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif
	if(progressState == Active) {
		StopEngine();
	}

	//Sets the new goal.
	goal = moveGoalPos;
	this->goalRadius = goalRadius;
	requestedSpeed = min(speed, owner->mobility->maxSpeed);
	requestedTurnRate = owner->mobility->maxTurnRate;
	atWaypoint = false;
	getPathFailures = 0;

	progressState = Active;

	//Starts the engine.
	StartEngine();

	ENTER_UNSYNCED;
	if(owner->team == gu->myTeam){
		//Play "activate" sound.
		if(owner->unitDef->sounds.activate.id)
			sound->PlaySound(owner->unitDef->sounds.activate.id, owner->midPos, owner->unitDef->sounds.activate.volume);
	}
	ENTER_SYNCED;
}

void CGroundMoveType::StopMoving() {
#ifdef TRACE_SYNC
	tracefile << "Stop moving called: ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif
	if(DEBUG_CONTROLLER)
		*info << "SMove: Action stopped.\n";

	StopEngine();

	if(progressState != Done)
		progressState = Failed;
}

void CGroundMoveType::SetDeltaSpeed(void)
{
	//Rounding of low speed.
	if(wantedSpeed == 0 && currentSpeed < 0.01){
		currentSpeed=0;
		deltaSpeed=0;
//		nextDeltaSpeedUpdate=gs->frameNum+8;
		return;
	}

	//Wanted speed and acceleration.
	float wSpeed = maxSpeed;

	//Limiting speed and acceleration.
	if(wSpeed > 0) {
		float groundMod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, owner->pos,flatFrontDir);
		if(groundMod < 0.01){
			if(DEBUG_CONTROLLER)
				info->AddLine("Speed failure %i",speedFailures);
			if(speedFailures>4){
				speedFailures=0;
	//			Fail();
				StopMoving();
			} else {
				speedFailures++;
			}
		}
		wSpeed *= groundMod;
	}
	if(wSpeed>wantedSpeed)
		wSpeed=wantedSpeed;

	//Limit change according to acceleration.
	float dif = wSpeed - currentSpeed;

	if(fabs(dif)<0.05){		//good speed
		deltaSpeed=dif/8;
		nextDeltaSpeedUpdate=gs->frameNum+8;

	} else if(dif>0){				//accelerate
		if(dif<accRate){
			deltaSpeed = dif;
			nextDeltaSpeedUpdate=gs->frameNum;
		} else {
			deltaSpeed=accRate;
			nextDeltaSpeedUpdate=(int)(gs->frameNum+min((float)8,dif/accRate));
		}
	}else {		//break, Breakrate = -3*accRate
		if(dif > -3*accRate){
			deltaSpeed = dif;
			nextDeltaSpeedUpdate=gs->frameNum+1;
		} else {
			deltaSpeed = -3*accRate;
			nextDeltaSpeedUpdate=(int)(gs->frameNum+min((float)8,dif/(-3*accRate)));
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
		heading += min(deltaHeading,(short)turnRate);
	} else {
		heading += max((short)-turnRate,deltaHeading);
	}

	owner->frontdir = GetVectorFromHeading(heading);
	owner->frontdir.Normalize();
	if(owner->upright){
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
//	info->AddLine("strength %f",strength);

	if(strength>3 || impulse.dot(groundNormal)>0.3){
		skidding=true;
		speed+=impulse;
		impulse=ZeroVector;

		skidRotSpeed+=(gs->randFloat()-0.5)*1500;
		skidRotPos2=0;
		skidRotSpeed2=0;
		float3 skidDir(speed);
		skidDir.y=0;
		skidDir.Normalize();
		skidRotVector=skidDir.cross(UpVector);
		oldPhysState=owner->physicalState;
		owner->physicalState= CSolidObject::Flying;
		owner->moveType->useHeading=false;

		if(speed.dot(groundNormal)>0.2){
			flying=true;
			skidRotSpeed2=(gs->randFloat()-0.5)*0.04;
		}
	}
}

void CGroundMoveType::UpdateSkid(void)
{
	float3& speed=owner->speed;
	float3& pos=owner->pos;
	float3& midPos=owner->midPos;

	if(flying){
		speed.y+=gs->gravity;
		if(midPos.y < 0)
			speed*=0.95;

		float wh;
		if(floatOnWater)
			wh = ground->GetHeight(midPos.x, midPos.z);
		else
			wh = ground->GetHeight2(midPos.x, midPos.z);

		if(wh>midPos.y-owner->relMidPos.y){
			flying=false;
			skidRotSpeed*=0.5+gs->randFloat();
			midPos.y=wh+owner->relMidPos.y-speed.y*0.8;
			float impactSpeed=-speed.dot(ground->GetNormal(midPos.x,midPos.z));
			if(impactSpeed>1){
				owner->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2,0,ZeroVector);
			}
		}
	} else {
		float speedf=speed.Length();
		float speedReduction=0.35;
//		if(owner->unitDef->movedata->moveType==MoveData::Hover_Move)
//			speedReduction=0.1;
		if(speedf<speedReduction){		//stop skidding
			currentSpeed=0;
			speed=ZeroVector;
			skidding=false;
			skidRotSpeed=0;
			owner->physicalState=oldPhysState;
			owner->moveType->useHeading=true;
			float rp=floor(skidRotPos2+skidRotSpeed2+0.5);
			skidRotSpeed2=(rp-skidRotPos2)*0.5;
			ChangeHeading(owner->heading);
		} else {
			speed*=(speedf-speedReduction)/speedf;

			float remTime=speedf/speedReduction-1;
			float rp=floor(skidRotPos2+skidRotSpeed2*remTime+0.5);
			skidRotSpeed2=(rp-skidRotPos2)/(remTime+1);

			if(floor(skidRotPos2)!=floor(skidRotPos2+skidRotSpeed2)){
				skidRotPos2=0;
				skidRotSpeed2=0;
			}
		}

		float wh;
		if(floatOnWater)
			wh = ground->GetHeight(midPos.x, midPos.z);
		else
			wh = ground->GetHeight2(midPos.x, midPos.z);

		if(wh-(midPos.y-owner->relMidPos.y) < speed.y+gs->gravity){
			speed.y+=gs->gravity;
			flying=true;
		} else {
			speed.y=wh-(midPos.y-owner->relMidPos.y);
			if(speed.Length()>speedf-(speedReduction*0.8))
				speed=speed.Normalize()*(speedf-(speedReduction*0.8));
		}
	}
	CalcSkidRot();

	midPos += speed;
	pos = owner->midPos - owner->frontdir * owner->relMidPos.z - owner->updir * owner->relMidPos.y - owner->rightdir * owner->relMidPos.x;
	CheckCollisionSkid();
}

void CGroundMoveType::CheckCollisionSkid(void)
{
	float3& pos=owner->pos;
	float3& midPos=owner->midPos;

	vector<CUnit*> nearUnits=qf->GetUnitsExact(midPos,owner->radius);
	for(vector<CUnit*>::iterator ui=nearUnits.begin();ui!=nearUnits.end();++ui){
		CUnit* u=(CUnit*)(*ui);
		float sqDist=(midPos-u->midPos).SqLength();
		float totRad=owner->radius+u->radius;
		if(sqDist<totRad*totRad && sqDist!=0){
			float dist=sqrt(sqDist);
			float3 dif=midPos-u->midPos;
			dif/=dist;
			if(u->mass==100000){
				midPos-=dif*(dist-totRad);
				pos=midPos-owner->frontdir*owner->relMidPos.z - owner->updir*owner->relMidPos.y - owner->rightdir*owner->relMidPos.x;
				float impactSpeed=-owner->speed.dot(dif);
				owner->speed=(owner->speed+dif*impactSpeed)*0.8;
				owner->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2,0,ZeroVector);
				u->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2,0,ZeroVector);
			} else {
				float part=owner->mass/(owner->mass+(*ui)->mass);
				float impactSpeed=(u->speed-owner->speed).dot(dif);

				midPos-=dif*(dist-totRad)*(1-part);
				pos=midPos-owner->frontdir*owner->relMidPos.z - owner->updir*owner->relMidPos.y - owner->rightdir*owner->relMidPos.x;
				owner->speed=(owner->speed+dif*impactSpeed)*(1-part);
				u->midPos+=dif*(dist-totRad)*(part);
				u->pos=u->midPos-u->frontdir*u->relMidPos.z - u->updir*u->relMidPos.y - u->rightdir*u->relMidPos.x;	

				owner->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2*(1-part),0,dif*impactSpeed*(owner->mass*(1-part)));
				owner->speed*=0.9;
				u->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2*part,0,dif*-impactSpeed*(u->mass*part));
			}
		}
	}
	vector<CFeature*> nearFeatures=qf->GetFeaturesExact(midPos,owner->radius);
	for(vector<CFeature*>::iterator fi=nearFeatures.begin();fi!=nearFeatures.end();++fi){
		CFeature* u=(*fi);
		if(!u->blocking)
			continue;
		float sqDist=(midPos-u->midPos).SqLength();
		float totRad=owner->radius+u->radius;
		if(sqDist<totRad*totRad && sqDist!=0){
			float dist=sqrt(sqDist);
			float3 dif=midPos-u->midPos;
			dif/=dist;
			midPos-=dif*(dist-totRad);
			pos=midPos-owner->frontdir*owner->relMidPos.z - owner->updir*owner->relMidPos.y - owner->rightdir*owner->relMidPos.x;
			float impactSpeed=-owner->speed.dot(dif);
			owner->speed=(owner->speed+dif*impactSpeed)*0.8;
			owner->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2,0,ZeroVector);
			u->DoDamage(DamageArray()*impactSpeed*owner->mass*0.2,0,-dif*impactSpeed);
		}
	}
}

float CGroundMoveType::GetFlyTime(float3 pos, float3 speed)
{
	return 0;
}

void CGroundMoveType::CalcSkidRot(void)
{
	owner->heading+=(short int)skidRotSpeed;

	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->frontdir.Normalize();
	if(owner->upright){
		owner->updir=UpVector;
		owner->rightdir=owner->frontdir.cross(owner->updir);
	} else {
		owner->updir=ground->GetSmoothNormal(owner->pos.x, owner->pos.z);
		owner->rightdir=owner->frontdir.cross(owner->updir);
		owner->rightdir.Normalize();
		owner->frontdir=owner->updir.cross(owner->rightdir);
	}

	skidRotPos2+=skidRotSpeed2;

	float cosp=cos(skidRotPos2*PI*2);
	float sinp=sin(skidRotPos2*PI*2);

	float3 f1=skidRotVector*skidRotVector.dot(owner->frontdir);
	float3 f2=owner->frontdir-f1;
	f2=f2*cosp+f2.cross(skidRotVector)*sinp;
	owner->frontdir=f1+f2;

	float3 r1=skidRotVector*skidRotVector.dot(owner->rightdir);
	float3 r2=owner->rightdir-r1;
	r2=r2*cosp+r2.cross(skidRotVector)*sinp;
	owner->rightdir=r1+r2;

	float3 u1=skidRotVector*skidRotVector.dot(owner->updir);
	float3 u2=owner->updir-u1;
	u2=u2*cosp+u2.cross(skidRotVector)*sinp;
	owner->updir=u1+u2;
}

const float AVOIDANCE_DISTANCE = 1.0;				//How far away a unit should start avoiding an obstacle. Multiplied with distance to waypoint.
const float AVOIDANCE_STRENGTH = 2.0;				//How strongly an object should be avoided. Raise this value to give some more marginal.
const float FORCE_FIELD_DISTANCE = 50;				//How faar away a unit may be affected by the force-field. Multiplied with speed of the unit.
const float FORCE_FIELD_STRENGTH = 0.4;				//Maximum strenght of the force-field.

/*
Dynamic obstacle avoidance.
Helps the unit to follow the path even when it's not perfect.
*/
float3 CGroundMoveType::ObstacleAvoidance(float3 desiredDir) {
	//NOTE: Based on the requirement that all objects has symetrical footprints.
	//		If this is false, then radius has to be calculated in a diffrent way!

	//Obstacle-avoidance-system only need to be runned if the unit want to move.
	if(pathId) {
//		START_TIME_PROFILE;
		float3 avoidanceDir = desiredDir;
		//Speed-optimizer. Reduces the times this system is runned.
		if(gs->frameNum>=nextObstacleAvoidanceUpdate) {
			nextObstacleAvoidanceUpdate=gs->frameNum+4;
			float currentDistanceToGoal = owner->midPos.distance2D(goal);
			float3 rightOfPath = desiredDir.cross(float3(0,1,0));
			float3 rightOfAvoid = rightOfPath;

			float avoidLeft = 0;
			float avoidRight = 0;
			
			float forceFieldDistance = currentSpeed * FORCE_FIELD_DISTANCE;
			float3 forceField;

			vector<CSolidObject*> nearbyObjects = qf->GetSolidsExact(owner->midPos, max(forceFieldDistance + owner->xsize * SQUARE_SIZE/2, currentDistanceToWaypoint));
			vector<CSolidObject*> objectsOnPath;

			vector<CSolidObject*>::iterator oi;
			for(oi = nearbyObjects.begin(); oi != nearbyObjects.end(); oi++) {
				CSolidObject* object = *oi;
				//Basic blocking-check.
				MoveData* moveData = owner->mobility->moveData;
				if(object != owner && moveData->moveMath->IsBlocking(*moveData, object)) {
					float3 objectToUnit = (owner->midPos - object->midPos);
					float distanceToObject = objectToUnit.Length();
					float radiusSum = (owner->xsize + object->xsize) * SQUARE_SIZE/2;
					//If object is close enought.
					if(distanceToObject < (AVOIDANCE_DISTANCE * currentDistanceToWaypoint) + object->xsize * SQUARE_SIZE/2
					&& distanceToObject < currentDistanceToGoal) {
						float objectDistToAvoidDirCenter = objectToUnit.dot(rightOfAvoid);	//Positive value means "to right".
						//If object and unit in relative motion are closing in to each other (or not yet fully apart),
						//the object are in path of the unit and they are not collided.
						if(objectToUnit.dot(avoidanceDir) < radiusSum
						&& abs(objectDistToAvoidDirCenter) < radiusSum
						&& Distance2D(owner, object) >= 0) {
							//Avoid collision by turning the heading to left or right.
							//Using the one who need most adjustment.
							if(objectDistToAvoidDirCenter > 0) {
								avoidRight += (radiusSum - objectDistToAvoidDirCenter) * AVOIDANCE_STRENGTH / distanceToObject;
								avoidanceDir += (rightOfAvoid * avoidRight);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(float3(0,1,0));
							} else {
								avoidLeft += (radiusSum - abs(objectDistToAvoidDirCenter)) * AVOIDANCE_STRENGTH / distanceToObject;
								avoidanceDir -= (rightOfAvoid * avoidLeft);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(float3(0,1,0));
							}
							objectsOnPath.push_back(object);
						}
					}

					//Calculating force-field-effect.
					//strength = F * ((D - d)/D)^2
					float distanceBetweenObject = max(distanceToObject - radiusSum, (float)0.0);
					if(distanceBetweenObject < forceFieldDistance) {
						float relativeDistance = (forceFieldDistance - distanceBetweenObject) / forceFieldDistance;
						float strength = FORCE_FIELD_STRENGTH * relativeDistance * relativeDistance;
						objectToUnit.Normalize();	//Could destroy the length of it, as it's not used after this.
						forceField += (objectToUnit * strength);
					}

					//Standing on top of waypoint?
					if(object->midPos.distance2D(waypoint) < object->xsize * SQUARE_SIZE/2) {
						waypointIsBlocked = true;
					}
				}
			}

			//If there are conteracting objects standing on the path,
			//then the situation has to be solved in a diffrent way.
			//This is solved by creating a super-objects.
			if(abs(avoidRight) > 0.0 && abs(avoidLeft) > 0.0) {
				//Reset avoidance-factors.
				avoidanceDir = desiredDir;
				rightOfPath = desiredDir.cross(float3(0,1,0));
				avoidRight = 0.0;
				avoidLeft = 0.0;

				//Creates a super-mass-point.
				float3 superMidPos;
				for(oi = objectsOnPath.begin(); oi != objectsOnPath.end(); oi++) {
					superMidPos += (*oi)->midPos;
				}
				superMidPos /= (objectsOnPath.size());

				//Gets the smallest super-radius including all objects
				//in the super-object.
				float superRadius = 0;
				for(oi = objectsOnPath.begin(); oi != objectsOnPath.end(); oi++) {
					float distance = (superMidPos - (*oi)->midPos).Length2D();
					superRadius = max(superRadius, (distance + (*oi)->xsize * SQUARE_SIZE/2));
				}

				//If the unit are deeply collided with the super-object,
				//then the situation might be unsolveable and the unit
				//has to stop and wait for a new path.
				float3 superObjectToUnit = (owner->midPos - superMidPos);
				float distanceToSuperObject = superObjectToUnit.Length2D();
				float superRadiusSum = superRadius + owner->xsize * SQUARE_SIZE/2;
				if(distanceToSuperObject < owner->radius) {		//TODO: radius or xsize*SQUARE_SIZE/2 ?
					avoidanceDir = ZeroVector;
					obstacleFailures++;
					if(obstacleFailures>8){
						if(DEBUG_CONTROLLER)
							info->AddLine("Obstacle failure %i %i",owner->id,obstacleFailures);
						StopEngine();
						obstacleFailures=0;
					}
				}
				//Otherwise the super-object has to be avoided.
				else {
					obstacleFailures=0;
					float superObjectDistToPathCenter = superObjectToUnit.dot(rightOfPath);
					if(superObjectDistToPathCenter > 0) {
						avoidRight = (superRadiusSum - superObjectDistToPathCenter) * AVOIDANCE_STRENGTH / distanceToSuperObject;
					} else {
						avoidLeft = (superRadiusSum - abs(superObjectDistToPathCenter)) * AVOIDANCE_STRENGTH / distanceToSuperObject;
					}
				}
			}

			//Sum up avoidance.
			avoidanceVec = (desiredDir.cross(float3(0,1,0)) * (avoidRight - avoidLeft));
			avoidanceVec += forceField;
		}

		//Return the resulting recommended velocity.
		avoidanceDir = desiredDir + avoidanceVec;
		if(avoidanceDir.Length2D() > 1.0)
			avoidanceDir.Normalize();

//		END_TIME_PROFILE("AI:SMoveOA");

		return avoidanceDir;
	} else {
		return ZeroVector;
	}
}

/*
Calculates an aproximation of the physical 2D-distance
between given two objects.
*/
float CGroundMoveType::Distance2D(CSolidObject *object1, CSolidObject *object2, float marginal) 
{
	//Calculating the distance in (x,z) depening in the look of the footprint.
	float dist2D;
	if(object1->xsize == object1->ysize || object2->xsize == object2->ysize) {
		//Using xsize as a cynlindrical radius.
		float3 distVec = (object1->midPos - object2->midPos);
		dist2D = distVec.Length2D() - (object1->xsize + object2->xsize)*SQUARE_SIZE/2 + 2*marginal;
	} else {
		//Pytagorean sum of the x and z distance.
		float3 distVec;
		distVec.x = abs(object1->midPos.x - object2->midPos.x) - (object1->xsize + object2->xsize)*SQUARE_SIZE/2 + 2*marginal;
		distVec.z = abs(object1->midPos.z - object2->midPos.z) - (object1->ysize + object2->ysize)*SQUARE_SIZE/2 + 2*marginal;
		if(distVec.x > 0.0 && distVec.z > 0.0)
			dist2D = distVec.Length2D();
		else if(distVec.x < 0.0 && distVec.z < 0.0)
			dist2D = -distVec.Length2D();
		else if(distVec.x > 0.0)
			dist2D = distVec.x;
		else
			dist2D = distVec.z;
	}
	return dist2D;
}

/*
Creates a path to the goal.
*/
void CGroundMoveType::GetNewPath() 
{
	if(owner->pos.distance2D(lastGetPathPos)<20){
		if(DEBUG_CONTROLLER)
			info->AddLine("Non moving failure %i %i",owner->id,nonMovingFailures);
		nonMovingFailures++;
		if(nonMovingFailures>4){
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

	pathId = pathManager->RequestPath(owner->mobility->moveData, owner->pos, goal, goalRadius,owner);

	//With new path recived, can't be at waypoint.
	if(pathId){
		atWaypoint = false;
		haveFinalWaypoint=false;
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
		float3 lastWaypoint, newWaypoint(0,0,0);
		float wantedDistanceToWaypoint = MinDistanceToWaypoint() * MAX_WAYPOINT_DISTANCE_FACTOR;
		//Get new waypoints until get one distant enought, or they start repeating (=> last waypoint in path).
		do {
			lastWaypoint = newWaypoint;
			newWaypoint = pathManager->NextWaypoint(pathId, owner->midPos, wantedDistanceToWaypoint);
		} while(newWaypoint != float3(-1,-1,-1) && lastWaypoint != newWaypoint && owner->pos.distance2D(newWaypoint) < wantedDistanceToWaypoint);
		//If a correct waypoint was recived, then use it.
		if(newWaypoint != float3(-1,-1,-1)) {
//			info->AddLine("New waypoint %i %f %f",owner->id,owner->pos.distance2D(newWaypoint),wantedDistanceToWaypoint);
			waypoint = newWaypoint;
			etaWaypoint = int((owner->midPos.distance2D(waypoint) / requestedSpeed) * ETA_ESTIMATION + gs->frameNum+3);
			atWaypoint = false;
			waypointIsBlocked = false;
		} else {
			if(DEBUG_CONTROLLER)
				info->AddLine("Path failure %i %i",owner->id,pathFailures);
			pathFailures++;
			if(pathFailures>8){
				pathFailures=0;
				Fail();
			}
			etaWaypoint = INT_MAX;
		}
		//If the waypoint is very close to the goal, then correct it into the goal.
		if(waypoint.distance2D(goal) < CPathManager::PATH_RESOLUTION){
			waypoint = goal;
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
	return abs(speed * speed / owner->mobility->maxBreaking);
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
	//Will be runned only if engine is no path
	//and the unit is not already at the goal.
	if(!pathId && !atWaypoint) {
		GetNewPath();
		//Engine will be activated only if a path could be found.
		if(pathId) {
			getPathFailures = 0;
			pathFailures=0;
			etaFailures=0;
			owner->isMoving=true;
			GetNextWaypoint();
			owner->cob->Call(COBFN_StartMoving);
		
			if(DEBUG_CONTROLLER)
				*info << "Engine started.\n";
		} else {
			getPathFailures++;

			//If no path could be found, this action has failed.
			if(getPathFailures >= FAILURE_RETRIALS)
				Fail();

			if(DEBUG_CONTROLLER)
				*info << "Engine start failed: " << int(getPathFailures) << "\n";
		}
	}
	nextObstacleAvoidanceUpdate=gs->frameNum;
	SetDeltaSpeed();
}

/*
Stops motion.
*/
void CGroundMoveType::StopEngine() {
	//Will be runned only if engine is active.
	if(pathId) {
		//Deactivating engine.
		pathManager->DeletePath(pathId);
		pathId = 0;
		if(!atWaypoint)
			waypoint = Here();

		//Stop animation.
		owner->cob->Call(COBFN_StopMoving);

		if(DEBUG_CONTROLLER)
			*info << "Engine stopped.\n";
	}
	owner->isMoving=false;
	wantedSpeed=0;
	SetDeltaSpeed();
}


/*
Called when the unit arrives at it's waypoint.
*/
void CGroundMoveType::Arrived() 
{
	//Can only "arrive" if the engine is active.
	if(progressState == Active) {
		//Have reached waypoint.
		atWaypoint = true;

		StopEngine();

		//Play "arrived" sound.
		ENTER_UNSYNCED;
		if(owner->team == gu->myTeam){
			if(owner->unitDef->sounds.arrived.id)
				sound->PlaySound(owner->unitDef->sounds.arrived.id, owner->midPos, owner->unitDef->sounds.arrived.volume);
		}
		ENTER_SYNCED;

		//And the action is done.
		progressState = Done;

		if(DEBUG_CONTROLLER)
			*info << "Unit arrived!\n";
	}
}

/*
Makes the unit fail this action.
No more trials will be done before a new goal is given.
*/
void CGroundMoveType::Fail() 
{
	StopEngine();

	//Failure of finding a path means that this action
	//has failed to reach it's goal.
	progressState = Failed;

	//Sends a message to user.
	ENTER_UNSYNCED;
	if(owner->team == gu->myTeam){
		//Playing "can't" sound.
		if(owner->unitDef->sounds.cant.id)
			sound->PlaySound(owner->unitDef->sounds.cant.id, owner->midPos, owner->unitDef->sounds.cant.volume);

		if(owner->pos.distance(goal)>goalRadius+150){
			*info << owner->unitDef->humanName.c_str() << ": Can't reach destination!\n";
			info->SetLastMsgPos(owner->pos);
		}
	}
	ENTER_SYNCED;
}

void CGroundMoveType::CheckCollision(void)
{
	int newMoveSquareX=(int)owner->pos.x / SQUARE_SIZE;		//first make sure we dont go into any terrain we cant get out of
	int newMoveSquareY=(int)owner->pos.z / SQUARE_SIZE;
	if(newMoveSquareX!=moveSquareX || newMoveSquareY!=moveSquareY){
		float cmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, moveSquareX,moveSquareY);
		if(newMoveSquareX>moveSquareX){
			float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX,newMoveSquareY);
			if(cmod>0.01 && nmod<=0.01){
				owner->pos.x=moveSquareX*SQUARE_SIZE+(SQUARE_SIZE-0.01);
				newMoveSquareX=moveSquareX;
			}
		} else if(newMoveSquareX<moveSquareX){
			float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX,newMoveSquareY);
			if(cmod>0.01 && nmod<=0.01){
				owner->pos.x=moveSquareX*SQUARE_SIZE+0.01;
				newMoveSquareX=moveSquareX;
			}

		}
		if(newMoveSquareY>moveSquareY){
			float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX,newMoveSquareY);
			if(cmod>0.01 && nmod<=0.01){
				owner->pos.z=moveSquareY*SQUARE_SIZE+(SQUARE_SIZE-0.01);
				newMoveSquareY=moveSquareY;
			}
		} else if(newMoveSquareY<moveSquareY){
			float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX,newMoveSquareY);
			if(cmod>0.01 && nmod<=0.01){
				owner->pos.z=moveSquareY*SQUARE_SIZE+0.01;
				newMoveSquareY=moveSquareY;
			}
		}
		moveSquareX=newMoveSquareX;
		moveSquareY=newMoveSquareY;
	}
	int2 newmp=owner->GetMapPos();
	if(newmp.x!=owner->mapPos.x || newmp.y!=owner->mapPos.y){		//now make sure we dont overrun any other units
		bool haveCollided=true;
		int retest=0;
		while(haveCollided && retest<2){
			if(fabs(owner->frontdir.x)>fabs(owner->frontdir.z)){
				if(newmp.y<owner->mapPos.y){
					haveCollided|=CheckColV(newmp.y,newmp.x,newmp.x+owner->xsize-1,(owner->mapPos.y+owner->ysize/2)*SQUARE_SIZE-3.99,owner->mapPos.y);
					newmp=owner->GetMapPos();
				} else if(newmp.y>owner->mapPos.y){
					haveCollided|=CheckColV(newmp.y+owner->ysize-1,newmp.x,newmp.x+owner->xsize-1,(owner->mapPos.y+owner->ysize/2)*SQUARE_SIZE+3.99,owner->mapPos.y+owner->ysize-1);
					newmp=owner->GetMapPos();
				}
				if(newmp.x<owner->mapPos.x){
					haveCollided|=CheckColH(newmp.x,newmp.y,newmp.y+owner->ysize-1,(owner->mapPos.x+owner->xsize/2)*SQUARE_SIZE-3.99,owner->mapPos.x);
					newmp=owner->GetMapPos();
				} else if(newmp.x>owner->mapPos.x){
					haveCollided|=CheckColH(newmp.x+owner->xsize-1,newmp.y,newmp.y+owner->ysize-1,(owner->mapPos.x+owner->xsize/2)*SQUARE_SIZE+3.99,owner->mapPos.x+owner->xsize-1);
					newmp=owner->GetMapPos();
				}
			} else {
				if(newmp.x<owner->mapPos.x){
					haveCollided|=CheckColH(newmp.x,newmp.y,newmp.y+owner->ysize-1,(owner->mapPos.x+owner->xsize/2)*SQUARE_SIZE-3.99,owner->mapPos.x);
					newmp=owner->GetMapPos();
				} else if(newmp.x>owner->mapPos.x){
					haveCollided|=CheckColH(newmp.x+owner->xsize-1,newmp.y,newmp.y+owner->ysize-1,(owner->mapPos.x+owner->xsize/2)*SQUARE_SIZE+3.99,owner->mapPos.x+owner->xsize-1);
					newmp=owner->GetMapPos();
				}
				if(newmp.y<owner->mapPos.y){
					haveCollided|=CheckColV(newmp.y,newmp.x,newmp.x+owner->xsize-1,(owner->mapPos.y+owner->ysize/2)*SQUARE_SIZE-3.99,owner->mapPos.y);
					newmp=owner->GetMapPos();
				} else if(newmp.y>owner->mapPos.y){
					haveCollided|=CheckColV(newmp.y+owner->ysize-1,newmp.x,newmp.x+owner->xsize-1,(owner->mapPos.y+owner->ysize/2)*SQUARE_SIZE+3.99,owner->mapPos.y+owner->ysize-1);
					newmp=owner->GetMapPos();
				}
			}
			++retest;
		}
		owner->UnBlock();
		owner->Block();
	}
	return;
}

bool CGroundMoveType::CheckColH(int x, int y1, int y2, float xmove, int squareTestX)
{
	for(int y=y1;y<=y2;++y){
		CSolidObject* c=readmap->groundBlockingObjectMap[y*gs->mapx+x];
		if(c!=0){
			if(readmap->groundBlockingObjectMap[y*gs->mapx+squareTestX]!=0 && readmap->groundBlockingObjectMap[y*gs->mapx+squareTestX]!=owner){
				continue;
			}
			if(c->mobility){		//if other part is mobile start to skuff it out of the way
				float part=owner->mass/(owner->mass+c->mass*2);
				float3 dif=c->pos-owner->pos;
				float dl=dif.Length();
				float colDepth=fabs(owner->pos.x-xmove);
				dif*=colDepth/dl;

				owner->pos-=dif*(1-part);
				c->pos+=dif*(part);
				CUnit* u=(CUnit*)c;		//only units can be mobile
				u->midPos=u->pos+u->frontdir*u->relMidPos.z + u->updir*u->relMidPos.y + u->rightdir*u->relMidPos.x;

				if(!(gs->frameNum+owner->id & 31) && !owner->commandAI->unimportantMove){
					helper->BuggerOff(owner->pos+owner->frontdir*owner->radius,owner->radius,owner);
				}
			}
			MoveData  *m=owner->mobility->moveData;		//if other part can be overrun then overrun it
			if (!m->moveMath->IsBlocking(*m,c)) {
				float3 tmp=owner->frontdir*currentSpeed*200;
				c->Kill(tmp);
			}
			if(readmap->groundBlockingObjectMap[y1*gs->mapx+x]==0)		//hack to make them find openings easier till the pathfinder can do it itself
				owner->pos.z-=fabs(owner->pos.x-xmove)*0.5;
			if(readmap->groundBlockingObjectMap[y2*gs->mapx+x]==0)
				owner->pos.z+=fabs(owner->pos.x-xmove)*0.5;

			owner->pos.x=xmove;
			currentSpeed*=0.97;
			return true;
		}
	}

	return false;
}

bool CGroundMoveType::CheckColV(int y, int x1, int x2, float zmove, int squareTestY)
{
	for(int x=x1;x<=x2;++x){
		CSolidObject* c=readmap->groundBlockingObjectMap[y*gs->mapx+x];
		if(c!=0){
			if(readmap->groundBlockingObjectMap[squareTestY*gs->mapx+x]!=0 && readmap->groundBlockingObjectMap[squareTestY*gs->mapx+x]!=owner){
				continue;
			}
			if(c->mobility){		//if other part is mobile start to skuff it out of the way
				float part=owner->mass/(owner->mass+c->mass*2);
				float3 dif=c->pos-owner->pos;
				float dl=dif.Length();
				float colDepth=fabs(owner->pos.z-zmove);
				dif*=colDepth/dl;

				owner->pos-=dif*(1-part);
				c->pos+=dif*(part);
				CUnit* u=(CUnit*)c;		//only units can be mobile
				u->midPos=u->pos+u->frontdir*u->relMidPos.z + u->updir*u->relMidPos.y + u->rightdir*u->relMidPos.x;

				if(!(gs->frameNum+owner->id & 31) && !owner->commandAI->unimportantMove){
					helper->BuggerOff(owner->pos+owner->frontdir*owner->radius,owner->radius,owner);
				}
			}
			MoveData  *m=owner->mobility->moveData;		//if other part can be overrun then overrun it
			if (!m->moveMath->IsBlocking(*m,c)) {
				float3 tmp=owner->frontdir*currentSpeed*200;
				c->Kill(tmp);
			}
			if(readmap->groundBlockingObjectMap[y*gs->mapx+x1]==0)		//hack to make them find openings easier till the pathfinder can do it itself
				owner->pos.x-=fabs(owner->pos.z-zmove)*0.5;
			if(readmap->groundBlockingObjectMap[y*gs->mapx+x2]==0)
				owner->pos.x+=fabs(owner->pos.z-zmove)*0.5;

			owner->pos.z=zmove;
			currentSpeed*=0.97;
			return true;
		}
	}

	return false;
}
