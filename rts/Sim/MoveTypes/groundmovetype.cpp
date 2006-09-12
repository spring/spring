#include "StdAfx.h"
#include "groundmovetype.h"
#include "Map/Ground.h"
#include "Sim/Misc/QuadField.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Game/GameHelper.h"
#include "myMath.h"
#include "LogOutput.h"
#include "Sim/Units/UnitHandler.h"
#include "SyncTracer.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/CobFile.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sound.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Path/PathManager.h"
#include "Game/Player.h"
#include "Game/Camera.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Mobility.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Weapons/Weapon.h"
#include "Game/SelectedUnits.h"
#include "Rendering/GroundDecalHandler.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "mmgr.h"

const unsigned int MAX_REPATH_FREQUENCY = 30;		//The minimum of frames between two full path-requests.

const float ETA_ESTIMATION = 1.5;					//How much time the unit are given to reach the waypoint.
const float MAX_WAYPOINT_DISTANCE_FACTOR = 2.0;		//Used to tune how often new waypoints are requested. Multiplied with MinDistanceToWaypoint().
const float MAX_OFF_PATH_FACTOR = 20;				//How far away from a waypoint a unit could be before a new path is requested.

const float MINIMUM_SPEED = 0.01;					//Minimum speed a unit may move in.

static const bool DEBUG_CONTROLLER=false;
std::vector<int2> CGroundMoveType::lineTable[11][11];

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
	moveSquareX=(int)owner->pos.x/(SQUARE_SIZE*2);
	moveSquareY=(int)owner->pos.z/(SQUARE_SIZE*2);
}

CGroundMoveType::~CGroundMoveType()
{
	if(pathId)
		pathManager->DeletePath(pathId);

	if(owner->myTrack)
		groundDecals->RemoveUnit(owner);
}

void CGroundMoveType::Update()
{
	//Update mobility.
	owner->mobility->maxSpeed = maxSpeed;

	if(owner->transporter)
		return;

	if(skidding){
		UpdateSkid();
		return;
	}

	if(owner->stunned){
		owner->cob->Call(COBFN_StopMoving);
		owner->speed=ZeroVector;
		return;
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if(owner->directControl){
		waypoint=owner->pos+owner->frontdir*100;
		waypoint.CheckInBounds();
		if(owner->directControl->forward){
			wantedSpeed=maxSpeed*2;
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
			deltaHeading+=(short)turnRate;
		}
		if(owner->directControl->right){
			deltaHeading-=(short)turnRate;
		}
		
		ENTER_UNSYNCED;
		if(gu->directControl==owner)
			camera->rot.y+=deltaHeading*PI/32768;
		ENTER_SYNCED;

		ChangeHeading(owner->heading+deltaHeading);
	} else
#endif
	if(pathId || currentSpeed > 0.0) {	//TODO: Stop the unit from moving as a reaction on collision/explosion physics.
		//Initial calculations.
		currentDistanceToWaypoint = owner->pos.distance2D(waypoint);

		if(pathId && !atGoal && gs->frameNum > etaWaypoint) {
			etaFailures+=10;
			etaWaypoint= INT_MAX;
			if(DEBUG_CONTROLLER)
				logOutput.Print("eta failure %i %i %i %i %i",owner->id,pathId, !atGoal,currentDistanceToWaypoint < MinDistanceToWaypoint(), gs->frameNum > etaWaypoint);
		}
		if(pathId && !atGoal && gs->frameNum > etaWaypoint2) {
			if(owner->pos.distance2D(goal)>200 || CheckGoalFeasability()){
				etaWaypoint2+=100;
			} else {
				if(DEBUG_CONTROLLER)
					logOutput.Print("Goal clogged up2 %i",owner->id);
				Fail();
			}
		}

		//Set direction to waypoint.
		float3 waypointDir = waypoint - owner->pos;
		waypointDir.y = 0;
		waypointDir.Normalize();


		//Has reached the waypoint? (=> arrived at goal)
		if(pathId && !atGoal && haveFinalWaypoint && (owner->pos - waypoint).SqLength2D() < SQUARE_SIZE*SQUARE_SIZE*2)
			Arrived();


		//-- Steering --//
		//Apply obstacle avoidance.
		float3 desiredVelocity = /*waypointDir/*/ObstacleAvoidance(waypointDir)/**/;

		if(desiredVelocity != ZeroVector){
			ChangeHeading(GetHeadingFromVector(desiredVelocity.x, desiredVelocity.z));
		} else {
			this->SetMainHeading();
		}

		if(nextDeltaSpeedUpdate<=gs->frameNum){
			wantedSpeed = pathId ? requestedSpeed : 0;
			//If arriving at waypoint, then need to slow down, or may pass it.
			if(currentDistanceToWaypoint < BreakingDistance(currentSpeed) + SQUARE_SIZE) {
				wantedSpeed = std::min((float)wantedSpeed, (float)(sqrt(currentDistanceToWaypoint * -owner->mobility->maxBreaking)));
			}
			wantedSpeed*=max(0.,std::min(1.,desiredVelocity.dot(owner->frontdir)+0.1));
			SetDeltaSpeed();
		}
	} else {
		this->SetMainHeading();
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
		TestNewTerrainSquare();
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

		if(groundDecals && owner->unitDef->leaveTracks && lastTrackUpdate<gs->frameNum-7 && ((owner->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectating)){
			lastTrackUpdate=gs->frameNum;
			groundDecals->UnitMoved(owner);
		}
	} else {
		owner->speed=ZeroVector;
	}
}

void CGroundMoveType::SlowUpdate()
{
	if(owner->transporter){
		if(progressState == Active)
			StopEngine();
		return;
	}

	//If got too far away from path, then need to reconsider.
	if(progressState == Active && etaFailures>8) {
		if(owner->pos.distance2D(goal)>200 || CheckGoalFeasability()){
			if(DEBUG_CONTROLLER)
				logOutput.Print("Unit eta failure %i",owner->id);
			StopEngine();
			StartEngine();
		} else {
			if(DEBUG_CONTROLLER)
				logOutput.Print("Goal clogged up %i",owner->id);
			Fail();
		}
	}

	//If the action is active, but not the engine and the
	//re-try-delay has passed, then start the engine.
	if(progressState == Active && !pathId && gs->frameNum > restartDelay) {
		if(DEBUG_CONTROLLER)
			logOutput.Print("Unit restart %i",owner->id);
		StartEngine();
	}

	owner->pos.CheckInBounds();		//just kindly move it into the map again instead of deleteing
/*	if(owner->pos.z<0 || owner->pos.z>(gs->mapy+1)*SQUARE_SIZE || owner->pos.x<0 || owner->pos.x>(gs->mapx+1)*SQUARE_SIZE){
		logOutput.Print("Deleting unit due to bad coord %i %f %f %f %s",owner->id,owner->pos.x,owner->pos.y,owner->pos.z,owner->unitDef->humanName.c_str());
		uh->DeleteUnit(owner);
	}*/

	float wh;		//need the following if the ground change height when unit stand still
	if(floatOnWater){
		wh = ground->GetHeight(owner->pos.x, owner->pos.z);
		if(wh==0)
			wh=-owner->unitDef->waterline;
	} else {
		wh = ground->GetHeight2(owner->pos.x, owner->pos.z);
	}
	owner->pos.y=wh;

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
		owner->isUnderWater=owner->pos.y+owner->height<1;
	}
}


/*
Sets unit to start moving against given position with max speed.
*/
void CGroundMoveType::StartMoving(float3 pos, float goalRadius) {
	StartMoving(pos, goalRadius, maxSpeed*2);
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
	requestedSpeed = min(speed, maxSpeed*2);
	requestedTurnRate = owner->mobility->maxTurnRate;
	atGoal = false;
	useMainHeading = false;

	progressState = Active;

	//Starts the engine.
	StartEngine();

	ENTER_UNSYNCED;
	if(owner->team == gu->myTeam){
		//Play "activate" sound.
		if(owner->unitDef->sounds.activate.id)
			sound->PlayUnitActivate(owner->unitDef->sounds.activate.id, owner, owner->unitDef->sounds.activate.volume);
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

	this->useMainHeading = false;
	if(progressState != Done)
		progressState = Done;
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
	if(wantedSpeed > 0) {
		float groundMod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, owner->pos,flatFrontDir);
		wSpeed *= groundMod;

		float3 goalDif=waypoint-owner->pos;
		short turn=owner->heading-GetHeadingFromVector(goalDif.x,goalDif.z);
		if(turn!=0){
			float goalLength=goalDif.Length();

			float turnSpeed=(goalLength+8)/(abs(turn)/turnRate)*0.5;
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
		accRate=0.01;
	}

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
//	logOutput.Print("strength %f",strength);

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
	SyncedFloat3& midPos=owner->midPos;

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
			float rp=floor(skidRotPos2+skidRotSpeed2+0.5f);
			skidRotSpeed2=(rp-skidRotPos2)*0.5;
			ChangeHeading(owner->heading);
		} else {
			speed*=(speedf-speedReduction)/speedf;

			float remTime=speedf/speedReduction-1;
			float rp=floor(skidRotPos2+skidRotSpeed2*remTime+0.5f);
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
	SyncedFloat3& midPos=owner->midPos;

	vector<CUnit*> nearUnits=qf->GetUnitsExact(midPos,owner->radius);
	for(vector<CUnit*>::iterator ui=nearUnits.begin();ui!=nearUnits.end();++ui){
		CUnit* u=(CUnit*)(*ui);
		float sqDist=(midPos-u->midPos).SqLength();
		float totRad=owner->radius+u->radius;
		if(sqDist<totRad*totRad && sqDist!=0){
			float dist=sqrt(sqDist);
			float3 dif=midPos-u->midPos;
			dif/=dist;
			if(u->mass==100000 || !u->mobility){
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

			//first check if the current waypoint is reachable
			int wsx=(int)waypoint.x / (SQUARE_SIZE*2);
			int wsy=(int)waypoint.z / (SQUARE_SIZE*2);
			int ltx=wsx-moveSquareX+5;
			int lty=wsy-moveSquareY+5;
			if(ltx>=0 && ltx<11 && lty>=0 && lty<11){
				for(std::vector<int2>::iterator li=lineTable[lty][ltx].begin();li!=lineTable[lty][ltx].end();++li){
					int x=(moveSquareX+li->x)*2;
					int y=(moveSquareY+li->y)*2;
					if((owner->unitDef->movedata->moveMath->IsBlocked(*owner->unitDef->movedata, x, y) & (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN | CMoveMath::BLOCK_MOBILE_BUSY)) || owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, x,y)<=0.01){
						++etaFailures;		//not reachable, force a new path to be calculated next slowupdate
						if(DEBUG_CONTROLLER)
							logOutput.Print("Waypoint path blocked %i",owner->id);
						break;
					}
				}
			}
			//now we do the obstacle avoidance proper
			float currentDistanceToGoal = owner->pos.distance2D(goal);
			float3 rightOfPath = desiredDir.cross(float3(0,1,0));
			float3 rightOfAvoid = rightOfPath;
			float speedf=owner->speed.Length2D();

			float avoidLeft = 0;
			float avoidRight = 0;
			
			vector<CSolidObject*> nearbyObjects = qf->GetSolidsExact(owner->pos, speedf*35 + 30 + owner->xsize/2);
			vector<CSolidObject*> objectsOnPath;

			vector<CSolidObject*>::iterator oi;
			for(oi = nearbyObjects.begin(); oi != nearbyObjects.end(); oi++) {
				CSolidObject* object = *oi;
				//Basic blocking-check.
				MoveData* moveData = owner->mobility->moveData;
				if(object != owner && moveData->moveMath->IsBlocking(*moveData, object) && desiredDir.dot(object->pos-owner->pos)>0) {
					float3 objectToUnit = (owner->pos - object->pos-object->speed*30);
					float distanceToObject = objectToUnit.Length();
					float radiusSum = (owner->xsize + object->xsize) * SQUARE_SIZE/2;
					//If object is close enought.
					if(distanceToObject < speedf*35 + 10 + radiusSum
					&& distanceToObject < currentDistanceToGoal
					&& distanceToObject > 0.001f) { // Don't divide by zero. TODO figure out why this can actually happen.
						float objectDistToAvoidDirCenter = objectToUnit.dot(rightOfAvoid);	//Positive value means "to right".
						//If object and unit in relative motion are closing in to each other (or not yet fully apart),
						//the object are in path of the unit and they are not collided.
						if(objectToUnit.dot(avoidanceDir) < radiusSum
						&& fabs(objectDistToAvoidDirCenter) < radiusSum
						&& (object->mobility || Distance2D(owner, object) >= 0)) {
							//Avoid collision by turning the heading to left or right.
							//Using the one who need most adjustment.
							if(DEBUG_CONTROLLER && selectedUnits.selectedUnits.find(owner)!=selectedUnits.selectedUnits.end())
								geometricObjects->AddLine(owner->pos+UpVector*20,object->pos+UpVector*20,3,1,4);
							if(objectDistToAvoidDirCenter > 0) {
								avoidRight += (radiusSum - objectDistToAvoidDirCenter) * AVOIDANCE_STRENGTH / distanceToObject;
								avoidanceDir += (rightOfAvoid * avoidRight);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(float3(0,1,0));
							} else {
								avoidLeft += (radiusSum - fabs(objectDistToAvoidDirCenter)) * AVOIDANCE_STRENGTH / distanceToObject;
								avoidanceDir -= (rightOfAvoid * avoidLeft);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(float3(0,1,0));
							}
							objectsOnPath.push_back(object);
						}
					}

				}
			}

			//Sum up avoidance.
			avoidanceVec = (desiredDir.cross(float3(0,1,0)) * (avoidRight - avoidLeft));
			if(DEBUG_CONTROLLER && selectedUnits.selectedUnits.find(owner)!=selectedUnits.selectedUnits.end()){
				int a=geometricObjects->AddLine(owner->pos+UpVector*20,owner->pos+UpVector*20+avoidanceVec*40,7,1,4);
				geometricObjects->SetColor(a,1,0.3,0.3,0.6);

				a=geometricObjects->AddLine(owner->pos+UpVector*20,owner->pos+UpVector*20+desiredDir*40,7,1,4);
				geometricObjects->SetColor(a,0.3,0.3,1,0.6);
			}
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
		distVec.x = fabs(object1->midPos.x - object2->midPos.x) - (object1->xsize + object2->xsize)*SQUARE_SIZE/2 + 2*marginal;
		distVec.z = fabs(object1->midPos.z - object2->midPos.z) - (object1->ysize + object2->ysize)*SQUARE_SIZE/2 + 2*marginal;
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

	pathId = pathManager->RequestPath(owner->mobility->moveData, owner->pos, goal, goalRadius,owner);

	nextWaypoint=owner->pos;
	//With new path recived, can't be at waypoint.
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
			etaWaypoint = int(30.0 / (requestedSpeed*terrainSpeed+0.001)) + gs->frameNum+50;
			etaWaypoint2 = int(25.0 / (requestedSpeed*terrainSpeed+0.001)) + gs->frameNum+10;
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
	//Will be runned only if engine is no path
	//and the unit is not already at the goal.
	if(!pathId && !atGoal) {
		GetNewPath();
		//Engine will be activated only if a path could be found.
		if(pathId) {
			pathFailures=0;
			etaFailures=0;
			owner->isMoving=true;
			owner->cob->Call(COBFN_StartMoving);
		
			if(DEBUG_CONTROLLER)
				logOutput << "Engine started" << " " << owner->id << "\n";
		} else {
			if(DEBUG_CONTROLLER)
				logOutput << "Engine start failed: " << owner->id << "\n";

			Fail();
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
		if(!atGoal)
			waypoint = Here();

		//Stop animation.
		owner->cob->Call(COBFN_StopMoving);

		if(DEBUG_CONTROLLER)
			logOutput << "Engine stopped." << " " << owner->id << "\n";
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
		atGoal = true;

		StopEngine();

		//Play "arrived" sound.
		ENTER_UNSYNCED;
		if(owner->team == gu->myTeam){
			if(owner->unitDef->sounds.arrived.id)
				sound->PlayUnitReply(owner->unitDef->sounds.arrived.id, owner, owner->unitDef->sounds.arrived.volume);
		}
		ENTER_SYNCED;

		//And the action is done.
		progressState = Done;

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
	if(owner->team == gu->myTeam){
		//Playing "can't" sound.
		if(owner->unitDef->sounds.cant.id)
			sound->PlayUnitReply(owner->unitDef->sounds.cant.id, owner, owner->unitDef->sounds.cant.volume);

		if(owner->pos.distance(goal)>goalRadius+150){
			logOutput << owner->unitDef->humanName.c_str() << ": Can't reach destination!\n";
			logOutput.SetLastMsgPos(owner->pos);
		}
	}
	ENTER_SYNCED;
}

void CGroundMoveType::CheckCollision(void)
{
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
				float3 fix = owner->frontdir*currentSpeed*200;
				c->Kill(fix);
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
				float3 fix = owner->frontdir*currentSpeed*200;
				c->Kill(fix);
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

//creates the tables used to see if we should advance to next pathfinding waypoint
void CGroundMoveType::CreateLineTable(void)
{
	for(int yt=0;yt<11;++yt){
		for(int xt=0;xt<11;++xt){
			float3 start(0.5,0,0.5);
			float3 to((xt-5)+0.5,0,(yt-5)+0.5);

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
						xp+=(xn+0.0001)*dx;
						zp+=(xn+0.0001)*dz;
					} else {
						xp+=(zn+0.0001)*dx;
						zp+=(zn+0.0001)*dz;
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

void CGroundMoveType::TestNewTerrainSquare(void)
{
	int newMoveSquareX=(int)owner->pos.x / (SQUARE_SIZE*2);		//first make sure we dont go into any terrain we cant get out of
	int newMoveSquareY=(int)owner->pos.z / (SQUARE_SIZE*2);

	if(newMoveSquareX!=moveSquareX || newMoveSquareY!=moveSquareY){
		float cmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, moveSquareX*2,moveSquareY*2);
		if(fabs(owner->frontdir.x)<fabs(owner->frontdir.z)){
			if(newMoveSquareX>moveSquareX){
				float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if(cmod>0.01 && nmod<=0.01){
					owner->pos.x=moveSquareX*SQUARE_SIZE*2+(SQUARE_SIZE*2-0.01);
					newMoveSquareX=moveSquareX;
				}
			} else if(newMoveSquareX<moveSquareX){
				float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if(cmod>0.01 && nmod<=0.01){
					owner->pos.x=moveSquareX*SQUARE_SIZE*2+0.01;
					newMoveSquareX=moveSquareX;
				}
			}
			if(newMoveSquareY>moveSquareY){
				float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if(cmod>0.01 && nmod<=0.01){
					owner->pos.z=moveSquareY*SQUARE_SIZE*2+(SQUARE_SIZE*2-0.01);
					newMoveSquareY=moveSquareY;
				}
			} else if(newMoveSquareY<moveSquareY){
				float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if(cmod>0.01 && nmod<=0.01){
					owner->pos.z=moveSquareY*SQUARE_SIZE*2+0.01;
					newMoveSquareY=moveSquareY;
				}
			}
		} else {
			if(newMoveSquareY>moveSquareY){
				float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if(cmod>0.01 && nmod<=0.01){
					owner->pos.z=moveSquareY*SQUARE_SIZE*2+(SQUARE_SIZE*2-0.01);
					newMoveSquareY=moveSquareY;
				}
			} else if(newMoveSquareY<moveSquareY){
				float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if(cmod>0.01 && nmod<=0.01){
					owner->pos.z=moveSquareY*SQUARE_SIZE*2+0.01;
					newMoveSquareY=moveSquareY;
				}
			}
			if(newMoveSquareX>moveSquareX){
				float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if(cmod>0.01 && nmod<=0.01){
					owner->pos.x=moveSquareX*SQUARE_SIZE*2+(SQUARE_SIZE*2-0.01);
					newMoveSquareX=moveSquareX;
				}
			} else if(newMoveSquareX<moveSquareX){
				float nmod=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, newMoveSquareX*2,newMoveSquareY*2);
				if(cmod>0.01 && nmod<=0.01){
					owner->pos.x=moveSquareX*SQUARE_SIZE*2+0.01;
					newMoveSquareX=moveSquareX;
				}
			}
		}
		if(newMoveSquareX!=moveSquareX || newMoveSquareY!=moveSquareY){
			moveSquareX=newMoveSquareX;
			moveSquareY=newMoveSquareY;
			terrainSpeed=owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, (moveSquareX)*2,(moveSquareY)*2);
			etaWaypoint = int(30.0 / (requestedSpeed*terrainSpeed+0.001)) + gs->frameNum+50;
			etaWaypoint2 = int(25.0 / (requestedSpeed*terrainSpeed+0.001)) + gs->frameNum+10;

			int nwsx=(int)nextWaypoint.x / (SQUARE_SIZE*2);		//if we have moved check if we can get a new waypoint
			int nwsy=(int)nextWaypoint.z / (SQUARE_SIZE*2);

			int numIter=0;
			//lowered the original 6 absolute distance to slightly more than 4.5 euclidian distance
			//to fix units getting stuck in buildings --tvo
			//My first fix set it to 21, as the pathfinding was still considered broken by many I reduced it to 11 (arbitrarily)
			//Does anyone know whether lowering this constant has any adverse side effects? Like e.g. more CPU usage? --tvo
			while((nwsx-moveSquareX)*(nwsx-moveSquareX)+(nwsy-moveSquareY)*(nwsy-moveSquareY) < 11 && !haveFinalWaypoint && pathId){
				int ltx=nwsx-moveSquareX+5;
				int lty=nwsy-moveSquareY+5;
				bool wpOk=true;
				for(std::vector<int2>::iterator li=lineTable[lty][ltx].begin();li!=lineTable[lty][ltx].end();++li){
					int x=(moveSquareX+li->x)*2;
					int y=(moveSquareY+li->y)*2;
					if((owner->unitDef->movedata->moveMath->IsBlocked(*owner->unitDef->movedata, x, y) & (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN | CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOBILE_BUSY)) || owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, x,y)<=0.01){
						wpOk=false;
						break;
					}
				}
				if(!wpOk || numIter>6)
					break;
				GetNextWaypoint();
				nwsx=(int)nextWaypoint.x / (SQUARE_SIZE*2);
				nwsy=(int)nextWaypoint.z / (SQUARE_SIZE*2);
				++numIter;
			}
		}
	}
}

bool CGroundMoveType::CheckGoalFeasability(void)
{
	float goalDist=goal.distance2D(owner->pos);

	int minx=(int)max(0.f,(goal.x-goalDist)/(SQUARE_SIZE*2));
	int minz=(int)max(0.f,(goal.z-goalDist)/(SQUARE_SIZE*2));

	int maxx=(int)min(float(gs->hmapx-1),(goal.x+goalDist)/(SQUARE_SIZE*2));
	int maxz=(int)min(float(gs->hmapy-1),(goal.z+goalDist)/(SQUARE_SIZE*2));

	MoveData* md=owner->unitDef->movedata;
	CMoveMath* mm=md->moveMath;

	float numBlocked=0;
	float numSquares=0;

	for(int z=minz;z<=maxz;++z){
		for(int x=minx;x<=maxx;++x){
			float3 pos(x*SQUARE_SIZE*2,0,z*SQUARE_SIZE*2);
			if((pos-goal).SqLength2D()<goalDist*goalDist){
				int blockingType=mm->SquareIsBlocked(*md,x*2,z*2);
				if((blockingType & CMoveMath::BLOCK_STRUCTURE) || mm->SpeedMod(*md,x*2,z*2)<0.01f){
					numBlocked+=0.3f;
					numSquares+=0.3f;
				} else {
					numSquares+=1.0f;
					if(blockingType) 
						numBlocked+=1.0f;
				}
			}
		}
	}
	if(numSquares>0){
		float partBlocked=numBlocked/numSquares;
		if(DEBUG_CONTROLLER)
			logOutput.Print("Part blocked %i %.0f%% %.0f",owner->id,partBlocked*100,goalDist);
		if(partBlocked>0.4)
			return false;
	}
	return true;
}

void CGroundMoveType::LeaveTransport(void)
{
	oldPos=owner->pos+UpVector*0.001;
}

void CGroundMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive){
	this->mainHeadingPos = pos;
	this->useMainHeading = aggressive;
	if(this->useMainHeading && !this->owner->weapons.empty()){
		float3 dir1 = this->owner->weapons.front()->mainDir;
		dir1.y = 0;
		dir1.Normalize();
		float3 dir2 = this->mainHeadingPos-this->owner->pos;
		dir2.y = 0;
		dir2.Normalize();
		if(dir2 != ZeroVector){
			short heading = GetHeadingFromVector(dir2.x,dir2.z) - GetHeadingFromVector(dir1.x,dir1.z);
			if(this->owner->heading != heading
			  && !this->owner->weapons.front()->TryTarget(this->mainHeadingPos, true,0)){
				this->progressState = Active;
			}
		}
	}
}

/**
* @brief Orients owner so that weapon[0]'s arc includes mainHeadingPos
*/
void CGroundMoveType::SetMainHeading(){
	if(this->useMainHeading && !this->owner->weapons.empty()){
		float3 dir1 = this->owner->weapons.front()->mainDir;
		dir1.y = 0;
		dir1.Normalize();
		float3 dir2 = this->mainHeadingPos-this->owner->pos;
		dir2.y = 0;
		dir2.Normalize();
		if(dir2 != ZeroVector){
			short heading = GetHeadingFromVector(dir2.x,dir2.z) - GetHeadingFromVector(dir1.x,dir1.z);
			if(this->progressState == Active && this->owner->heading == heading){
				this->owner->cob->Call(COBFN_StopMoving);
				this->progressState = Done;
			} else if(this->progressState == Active){
				this->ChangeHeading(heading);
			} else if(this->progressState != Active
			  && this->owner->heading != heading
			  && !this->owner->weapons.front()->TryTarget(this->mainHeadingPos, true,0)){
				this->progressState = Active;
				this->owner->cob->Call(COBFN_StartMoving);
				this->ChangeHeading(heading);
			}
		}
	}
}

void CGroundMoveType::SetMaxSpeed(float speed)
{
	if(requestedSpeed == maxSpeed*2)
		requestedSpeed = speed*2;	//why the *2 everywhere?
	maxSpeed=speed;
}
