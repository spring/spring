#include "StdAfx.h"
#include "TAAirMoveType.h"
#include "QuadField.h"
#include "Ground.h"
#include "LosHandler.h"
#include "RadarHandler.h"
#include "CobFile.h"
#include "CobInstance.h"
#include "InfoConsole.h"
#include "myMath.h"
#include "Matrix44f.h"
#include "3DOParser.h"
#include "Player.h"
#include "UnitDef.h"
#include "GeometricObjects.h"
#include "Mobility.h"
#include "TransportUnit.h"

CTAAirMoveType::CTAAirMoveType(CUnit* owner) :
	CMoveType(owner),
	aircraftState(AIRCRAFT_LANDED),
	wantedHeight(80),
	altitudeRate(3.0f),
	currentBank(0),
	wantedHeading(0),
	wantToStop(false),
	forceHeading(false),
	dontCheckCol(false),
	dontLand(false),
	lastMoveRate(0),
	waitCounter(0),
	deltaSpeed(ZeroVector),
	accRate(1),
	breakDistance(1),
	circlingPos(ZeroVector),
	decRate(1),
	flyState(FLY_CRUISING),
	forceHeadingTo(0),
	goalDistance(1),
	goalPos(owner->pos),
	oldGoalPos(owner->pos),
	turnRate(1),
	wantedSpeed(ZeroVector),
	lastColWarning(0),
	lastColWarningType(0),
	reservedLandingPos(-1,-1,-1),
	maxDrift(1),
	repairBelowHealth(0.30),
	padStatus(0),
	reservedPad(0),
	currentPitch(0)
{
	wantedHeight+=gs->randFloat()*5;
	orgWantedHeight=wantedHeight;
}

CTAAirMoveType::~CTAAirMoveType(void)
{
	if(reservedPad){
		airBaseHandler->LeaveLandingPad(reservedPad);
		reservedPad=0;
	}
}
/*
float globalForce=15;
extern float globalArf;
float3 marfmurf(1,2,1);
void arfurf()
{
	float3 murf=UpVector*globalForce;
	globalForce=globalForce+23*globalForce+murf.y;
	globalForce=globalArf;
}

void TestDenormal(float f)
{
	if(f>globalArf)
		globalForce=globalArf;
	arfurf();
}
*/
void CTAAirMoveType::SetGoal(float3 newPos, float distance)
{
	maxDrift=max(16.0f,distance);	//aircrafts need some marginals to avoid uber stacking when lots of them are ordered to one place

	goalPos = newPos;
	oldGoalPos=newPos;
	forceHeading=false;
	wantedHeight=orgWantedHeight;
//	goalPos = pos;
}

void CTAAirMoveType::SetState(AircraftState newState)
{
	if (newState == aircraftState)
		return;

	//info->AddLine("SetState %i",newState);

	//Perform cob animation
	if (aircraftState == AIRCRAFT_LANDED)
		owner->cob->Call(COBFN_StartMoving);	
	if (newState == AIRCRAFT_LANDED)
		owner->cob->Call(COBFN_StopMoving);	

	aircraftState = newState;
	
	//Do animations
	switch (aircraftState) {
		case AIRCRAFT_LANDED:
			if(padStatus==0){		//dont set us as on ground if we are on pad
				owner->physicalState = CSolidObject::OnGround;
				owner->Block();
			}
		case AIRCRAFT_LANDING:
//			if (ownerActivated) {
				owner->Deactivate();
//				ownerActivated = false;
//				owner->cob->Call(COBFN_Deactivate);	
//			}
			break;
		default:
			owner->physicalState = CSolidObject::Flying;
			owner->UnBlock();
			owner->Activate();
			reservedLandingPos.x=-1;
//			if (!ownerActivated) {
//				ownerActivated = true;
//				owner->cob->Call(COBFN_Activate);	
//			}
			break;
	}

	//Cruise as default
	if (aircraftState == AIRCRAFT_FLYING)
		flyState = FLY_CRUISING;

	owner->isMoving = (aircraftState != AIRCRAFT_LANDED);
	waitCounter = 0;
}

void CTAAirMoveType::StartMoving(float3 pos, float goalRadius)
{
	wantToStop = false;
	owner->isMoving=true;
	waitCounter=0;
	forceHeading=false;

	switch (aircraftState) {
		case AIRCRAFT_LANDED:
			SetState(AIRCRAFT_TAKEOFF);
			break;
		case AIRCRAFT_TAKEOFF:
			SetState(AIRCRAFT_TAKEOFF);
			break;
		case AIRCRAFT_FLYING:
			if (flyState != FLY_CRUISING)
				flyState = FLY_CRUISING;
			break;
		case AIRCRAFT_LANDING:
			SetState(AIRCRAFT_TAKEOFF);
			break;
	}

	//info->AddLine("Moving to %f %f %f", pos.x, pos.y, pos.z);
	SetGoal(pos, goalRadius);

	breakDistance = ((maxSpeed * maxSpeed) / decRate);
}

void CTAAirMoveType::StartMoving(float3 pos, float goalRadius, float speed)
{
	//info->AddLine("airmove: Ignoring startmoving speed");
	StartMoving(pos, goalRadius);
}

void CTAAirMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive)
{
	wantToStop = false;
	forceHeading=false;
	wantedHeight=orgWantedHeight;
	//close in a little to avoid the command ai to override the pos constantly
	distance -= 15;	

//	info->AddLine("point order");

	//Ignore the exact same order
	if ((aircraftState == AIRCRAFT_FLYING) && (flyState==FLY_CIRCLING || flyState==FLY_ATTACKING) && ((circlingPos-pos).SqLength2D()<64) && (goalDistance == distance))
		return;

	circlingPos = pos;
	goalDistance = distance;
	goalPos=owner->pos;
	//only reset sometimes
	//if (flyState == FLY_CRUISING)
	//	waitCounter = 0;

	SetState(AIRCRAFT_FLYING);

	if (aggressive)
		flyState = FLY_ATTACKING;
	else
		flyState = FLY_CIRCLING;

//	info->AddLine("Starting circling");	
}

void CTAAirMoveType::ExecuteStop()
{
	wantToStop = false;
//	info->AddLine("Executing stop");
	switch (aircraftState) {
		case AIRCRAFT_TAKEOFF:
			SetState(AIRCRAFT_LANDING);
			waitCounter = 30;		//trick to land directly..
			break;
		case AIRCRAFT_FLYING:
			SetState(AIRCRAFT_LANDING);
			break;
		case AIRCRAFT_LANDING:
			break;
		case AIRCRAFT_LANDED:
			break;
	}
}

void CTAAirMoveType::StopMoving()
{
//	info->AddLine("stop order");
	wantToStop = true;
	forceHeading=false;
	owner->isMoving=false;
	wantedHeight=orgWantedHeight;
}

void CTAAirMoveType::Idle()
{
	StopMoving();
}

void CTAAirMoveType::UpdateLanded()
{
	float3 &pos = owner->pos;

	if(padStatus==0)		//dont place on ground if we are on a repair pad
		pos.y = ground->GetHeight(pos.x, pos.z);

	owner->speed=ZeroVector;
}

void CTAAirMoveType::UpdateTakeoff()
{
	float3 &pos = owner->pos;
	wantedSpeed=ZeroVector;
	wantedHeight=orgWantedHeight;

	UpdateAirPhysics();

	float h = pos.y - ground->GetHeight(pos.x, pos.z);

	if (h > orgWantedHeight*0.8) {
		//info->AddLine("Houston, we have liftoff %f %f", h, wantedHeight);
		SetState(AIRCRAFT_FLYING);
	}
}

void CTAAirMoveType::UpdateFlying()
{
	float3 &pos = owner->pos;
	float3 &speed = owner->speed;

	//Direction to where we would like to be
	float3 dir = goalPos - pos;
	bool arrived = false;
	owner->restTime=0;

	//are we there yet?
//	info->AddLine("max drift %f %i %f %f",maxDrift,waitCounter,dir.Length2D(),fabs(ground->GetHeight(pos.x,pos.z)-pos.y+wantedHeight));
	bool closeToGoal=dir.SqLength2D() < maxDrift*maxDrift && fabs(ground->GetHeight(pos.x,pos.z)-pos.y+wantedHeight)<maxDrift;
	if(flyState==FLY_ATTACKING)
		closeToGoal=dir.SqLength2D() < 400;

	if (closeToGoal) {		//pretty close
		switch (flyState) {
			case FLY_CRUISING:
				if(dontLand || (++waitCounter<55 && dynamic_cast<CTransportUnit*>(owner))){		//transport aircrafts need some time to detect that they can pickup
					wantedSpeed=ZeroVector;
					if(waitCounter>60){
						wantedHeight=orgWantedHeight;
					}
				} else {
					wantedHeight=orgWantedHeight;
					SetState(AIRCRAFT_LANDING);
				}
				//info->AddLine("In position, landing");
				return;
			case FLY_CIRCLING:
				waitCounter++;
				if (waitCounter > 100) {
					//info->AddLine("moving circlepos");
					float3 relPos = pos - circlingPos;
					if(relPos.x<0.0001 && relPos.x>-0.0001)
						relPos.x=0.0001;
					relPos.y = 0;
					relPos.Normalize();
					CMatrix44f rot;
					rot.RotateY(1.0f);
					float3 newPos = rot.Mul(relPos);

					//Make sure the point is on the circle
					newPos = newPos.Normalize() * goalDistance;

					//Go there in a straight line
					goalPos = circlingPos + newPos;
					waitCounter = 0;
				}
				break;
			case FLY_ATTACKING:{
				//info->AddLine("wait is %d", waitCounter);
				float3 relPos = pos - circlingPos;
				if(relPos.x<0.0001 && relPos.x>-0.0001)
					relPos.x=0.0001;
				relPos.y = 0;
				relPos.Normalize();
				CMatrix44f rot;
				if (gs->randFloat()>0.5)
					rot.RotateY(0.6f+gs->randFloat()*0.6);
				else
					rot.RotateY(-(0.6f+gs->randFloat()*0.6));
				float3 newPos = rot.Mul(relPos);
				newPos = newPos.Normalize() * goalDistance;

				//Go there in a straight line
				goalPos = circlingPos + newPos;
//				info->AddLine("Changed circle pos");
				break;}
			case FLY_LANDING:{
/*				//First check if we can land around here somewhere
				if (!CanLandAt(pos)) {
					//Check the surrounding area for a suitable position
					float3 newPos;
					bool found = FindLandingSpot(pos, newPos);
					if (found) {
						SetState(AIRCRAFT_FLYING);		
						info->AddLine("Found a landingspot when cruising around");
						goalPos = newPos;			
						return;
					}
				}
				else {
					SetState(AIRCRAFT_LANDING);
					return;
				}

				float3 relPos = pos - circlingPos;
				relPos.y = 0;
				CMatrix44f rot;
				rot.RotateY(2.0f);
				float3 newPos = rot.Mul(relPos);

				//Make sure the point is on the circle
				newPos = newPos.Normalize() * goalDistance;

				//Go there in a straight line
				goalPos = circlingPos + newPos;
				waitCounter = 0;
	*/			break;}
		}
	}
	//no, so go there!
	
	dir.y = 0;
	float realMax = maxSpeed;
	float dist=dir.Length2D();

	//If we are close to our goal, we should go slow enough to be able to break in time
	//if in attack mode dont slow down
	if (flyState!=FLY_ATTACKING && dist < breakDistance) {
		realMax = dist/(speed.Length2D()+0.01) * decRate;
		//info->AddLine("Break! %f %f %f", maxSpeed, dir.Length2D(), realMax);
	}

	wantedSpeed = dir.Normalize() * realMax;

	UpdateAirPhysics();


	//Point toward goal or forward
	if ((flyState == FLY_ATTACKING) || (flyState == FLY_CIRCLING)) {
		dir = circlingPos - pos;
	} else {
		dir = goalPos - pos;
	}
	if(dir.SqLength2D()>1)
		wantedHeading = GetHeadingFromVector(dir.x, dir.z);
}

void CTAAirMoveType::UpdateLanding()
{
	float3 &pos = owner->pos;
	float3 &speed = owner->speed;

	//We want to land, and therefore cancel our speed first
	wantedSpeed = ZeroVector;

	waitCounter++;

	//Hang around for a while so queued commands have a chance to take effect
	if (waitCounter < 30) {
		//info->AddLine("want to land, but.. %d", waitCounter);
		UpdateAirPhysics();
		return;
	}

	if(reservedLandingPos.x<0){
//		info->AddLine("Searching for landing spot");
		if(CanLandAt(pos)){
//			info->AddLine("Found landing spot");
			reservedLandingPos=pos;
			goalPos=pos;
			owner->physicalState = CSolidObject::OnGround;
			owner->Block();
			owner->physicalState = CSolidObject::Flying;
			owner->Deactivate();
			owner->cob->Call(COBFN_StopMoving);
		} else {
			if(goalPos.distance2D(pos)<30){
				goalPos=goalPos+gs->randVector()*300;
				goalPos.CheckInBounds();
			}
			flyState = FLY_LANDING;
			UpdateFlying();
			return;
		}
	}
	//We should wait until we actually have stopped smoothly
	if (speed.SqLength2D() > 1) {
		UpdateFlying();
		return;
	}

	//We have stopped, time to land
	wantedHeight=-2;
	UpdateAirPhysics();
	float h = pos.y - ground->GetHeight(pos.x, pos.z);

	if (h <= 0) {
		//info->AddLine("Landed");
		SetState(AIRCRAFT_LANDED);
		pos.y = ground->GetHeight(pos.x, pos.z);
	}
}

void CTAAirMoveType::UpdateHeading()
{
	short& heading=owner->heading;

	short deltaHeading = wantedHeading - heading;

	if(forceHeading)
		deltaHeading= forceHeadingTo - heading;

	if(deltaHeading > 0){
		heading += deltaHeading > (short int)turnRate ? (short int)turnRate : deltaHeading;		//min(deltaHeading, turnRate);
	} else {
		heading += deltaHeading > (short int)(-turnRate) ? deltaHeading : (short int)(-turnRate);  //max(-turnRate, deltaHeading);
	}	
}

void CTAAirMoveType::UpdateBanking()
{
	float3 &frontdir = owner->frontdir;
	float3 &updir = owner->updir;

	float wantedPitch=0;
	if(aircraftState==AIRCRAFT_FLYING && flyState==FLY_ATTACKING && circlingPos.y<owner->pos.y){
		wantedPitch=(circlingPos.y-owner->pos.y)/circlingPos.distance(owner->pos);
	}
	currentPitch=currentPitch*0.95+wantedPitch*0.05;

	frontdir = GetVectorFromHeading(owner->heading);
	frontdir.y=currentPitch;
	frontdir.Normalize();

	float3 rightdir(frontdir.cross(UpVector));		//temp rightdir
	rightdir.Normalize();

	float wantedBank = rightdir.dot(deltaSpeed)/accRate*0.5;
	float limit=min(1.0f,goalPos.distance2D(owner->pos)*0.15f);
	if(wantedBank>limit)
		wantedBank=limit;
	else if(wantedBank<-limit)
		wantedBank=-limit;

	//Adjust our banking to the desired value
	if (currentBank > wantedBank)
		currentBank -= min(0.03f, currentBank - wantedBank);
	else 
		currentBank += min(0.03f, wantedBank - currentBank);

	//Calculate a suitable upvector
	updir= rightdir.cross(frontdir);
	updir = updir*cos(currentBank)+rightdir*sin(currentBank);
	owner->rightdir = frontdir.cross(updir);
}

void CTAAirMoveType::UpdateAirPhysics()
{
	float3& pos=owner->pos;
	float3& speed=owner->speed;

	if(!((gs->frameNum+owner->id)&3))
		CheckForCollision();
/*
	if(lastColWarningType==1){
		int g=geometricObjects->AddLine(owner->pos,lastColWarning->pos,10,1,1);
		geometricObjects->SetColor(g,0.2,1,0.2,0.6);
	} else if(lastColWarningType==2){
		int g=geometricObjects->AddLine(owner->pos,lastColWarning->pos,10,1,1);
		if(speed.dot(lastColWarning->midPos+lastColWarning->speed*20 - owner->midPos - owner->speed*20)<0)
			geometricObjects->SetColor(g,1,0.2,0.2,0.6);
		else
			geometricObjects->SetColor(g,1,1,0.2,0.6);
	}
*/
	float yspeed=speed.y;
	speed.y=0;

	float3 delta = wantedSpeed - speed;
	float dl=delta.Length();

	if(delta.dot(speed)>0){	//accelerate
		if(dl<accRate)
			speed=wantedSpeed;
		else
			speed+=delta/dl*accRate;
	} else {											//break
		if(dl<decRate)
			speed=wantedSpeed;
		else
			speed+=delta/dl*decRate;
	}

	speed.y=yspeed;
	float h = pos.y - max(ground->GetHeight(pos.x, pos.z),ground->GetHeight(pos.x+speed.x*40,pos.z+speed.z*40));

	if(h<4){
		speed.x*=0.95;
		speed.z*=0.95;
	}

	float wh=wantedHeight;
	if(lastColWarningType==2 && speed.dot(lastColWarning->midPos+lastColWarning->speed*20-owner->midPos-speed*20)<0){
		if(lastColWarning->midPos.y>owner->pos.y)
			wh-=30;
		else
			wh+=50;
	}
	float ws;
	if (h < wh){
		ws= altitudeRate;
		if(speed.y>0 && (wh-h)/speed.y*accRate*1.5 < speed.y)
			ws=0;
	} else {
		ws= -altitudeRate;
		if(speed.y<0 && (wh-h)/speed.y*accRate*0.7 < -speed.y)
			ws=0;
	}
	if(speed.y>ws)
		speed.y=max(ws,speed.y-accRate*1.5f);
	else
		speed.y=min(ws,speed.y+accRate*(h<20?2.0f:0.7f));	//let them accelerate upward faster if close to ground

	pos+=speed;
}

void CTAAirMoveType::UpdateMoveRate()
{
	int curRate;

	//currentspeed is not used correctly for vertical movement, so compensate with this hax
	if ((aircraftState == AIRCRAFT_LANDING) || (aircraftState == AIRCRAFT_TAKEOFF)) {
		curRate = 1;
	}
	else {
		float curSpeed = owner->speed.Length();
		curRate = (int)((curSpeed / maxSpeed) * 3);

		if (curRate < 0)
			curRate = 0;
		if (curRate > 2)
			curRate = 2;
	}

	if (curRate != lastMoveRate) {
		owner->cob->Call(COBFN_MoveRate0 + curRate);
		lastMoveRate = curRate;
		//info->AddLine("new moverate: %d", curRate);
	}
}

void CTAAirMoveType::Update()
{
	//Handy stuff. Wonder if there is a better way?
	float3 &pos=owner->pos;
	float3 &rightdir = owner->rightdir;
	float3 &frontdir = owner->frontdir;
	float3 &updir = owner->updir;
	float3 &speed = owner->speed;

	//This is only set to false after the plane has finished constructing
	if (useHeading){
		useHeading = false;
		SetState(AIRCRAFT_TAKEOFF);
	}

	//Allow us to stop if wanted
	if (wantToStop)
		ExecuteStop();

	float3 lastSpeed = speed;

	if(owner->stunned){
		wantedSpeed=ZeroVector;
		UpdateAirPhysics();
	} else {
#ifdef DIRECT_CONTROL_ALLOWED
		if(owner->directControl){
			DirectControlStruct* dc=owner->directControl;
			SetState(AIRCRAFT_FLYING);

			float3 forward=dc->viewDir;
			float3 flatForward=forward;
			flatForward.y=0;
			flatForward.Normalize();
			float3 right=forward.cross(UpVector);

			wantedSpeed=ZeroVector;
			if(dc->forward)
				wantedSpeed+=flatForward;
			if(dc->back)
				wantedSpeed-=flatForward;
			if(dc->right)
				wantedSpeed+=right;
			if(dc->left)
				wantedSpeed-=right;
			wantedSpeed.Normalize();
			wantedSpeed*=maxSpeed;
			UpdateAirPhysics();
			wantedHeading=GetHeadingFromVector(flatForward.x,flatForward.z);

		} else 
#endif
		{

			if(reservedPad){
				CUnit* unit=reservedPad->unit;
				float3 relPos=unit->localmodel->GetPiecePos(reservedPad->piece);
				float3 pos=unit->pos + unit->frontdir*relPos.z + unit->updir*relPos.y + unit->rightdir*relPos.x;
				if(padStatus==0){
					if(aircraftState!=AIRCRAFT_FLYING && aircraftState!=AIRCRAFT_TAKEOFF)
						SetState(AIRCRAFT_FLYING);

					goalPos=pos;

					if(pos.distance(owner->pos)<400){
						padStatus=1;
					}
					//geometricObjects->AddLine(owner->pos,pos,1,0,1);
				} else if(padStatus==1){
					if(aircraftState!=AIRCRAFT_FLYING)
						SetState(AIRCRAFT_FLYING);
					flyState=FLY_LANDING;

					goalPos=pos;
					reservedLandingPos=pos;
					wantedHeight=pos.y-ground->GetHeight(pos.x,pos.z);

					if(owner->pos.distance(pos)<3 || aircraftState==AIRCRAFT_LANDED){
						padStatus=2;
					}
					//geometricObjects->AddLine(owner->pos,pos,10,0,1);
				} else {
					if(aircraftState!=AIRCRAFT_LANDED)
						SetState(AIRCRAFT_LANDED);
					
					owner->pos=pos;

					owner->AddBuildPower(unit->unitDef->buildSpeed/30,unit);

					if(owner->health>=owner->maxHealth-1){
						airBaseHandler->LeaveLandingPad(reservedPad);
						reservedPad=0;
						padStatus=0;
						goalPos=oldGoalPos;
						SetState(AIRCRAFT_TAKEOFF);
					}
				}
			}

			//Main state handling
			switch (aircraftState) {
				case AIRCRAFT_LANDED:
					UpdateLanded();
					break;
				case AIRCRAFT_TAKEOFF:
					UpdateTakeoff();
					break;
				case AIRCRAFT_FLYING:
					UpdateFlying();
					break;
				case AIRCRAFT_LANDING:
					UpdateLanding();
					break;
			}
		}
	}
	deltaSpeed = speed - lastSpeed;
	deltaSpeed.y = 0;					//Banking requires this

	//Turn and bank and move
	UpdateHeading();
	UpdateBanking();			//updates dirs
	owner->midPos=pos+frontdir*owner->relMidPos.z + updir*owner->relMidPos.y + rightdir*owner->relMidPos.x;

	//Push other units out of the way
	if(pos!=oldpos && aircraftState!=AIRCRAFT_TAKEOFF && padStatus==0){
		oldpos=pos;
		if(!dontCheckCol){
			vector<CUnit*> nearUnits=qf->GetUnitsExact(pos,owner->radius+6);
			vector<CUnit*>::iterator ui;
			for(ui=nearUnits.begin();ui!=nearUnits.end();++ui){
				if((*ui)->inTransport)
					continue;
				float sqDist=(pos-(*ui)->pos).SqLength();
				float totRad=owner->radius+(*ui)->radius;
				if(sqDist<totRad*totRad && sqDist!=0){
					float dist=sqrt(sqDist);
					float3 dif=pos-(*ui)->pos;
					dif/=dist;
					if((*ui)->mass>=100000){
						pos-=dif*(dist-totRad);
						owner->midPos=pos+owner->frontdir*owner->relMidPos.z + owner->updir*owner->relMidPos.y + owner->rightdir*owner->relMidPos.x;	
						owner->speed*=0.99f;
//						float damage=((*ui)->speed-owner->speed).SqLength();		//dont think they should take damage when they dont try to avoid it
//						owner->DoDamage(DamageArray()*damage,0,ZeroVector);
//						(*ui)->DoDamage(DamageArray()*damage,0,ZeroVector);
					} else {
						float part=owner->mass/(owner->mass+(*ui)->mass);
						pos-=dif*(dist-totRad)*(1-part);
						owner->midPos=pos+owner->frontdir*owner->relMidPos.z + owner->updir*owner->relMidPos.y + owner->rightdir*owner->relMidPos.x;	
						CUnit* u=(CUnit*)(*ui);
						u->pos+=dif*(dist-totRad)*(part);
						u->midPos=u->pos+u->frontdir*u->relMidPos.z + u->updir*u->relMidPos.y + u->rightdir*u->relMidPos.x;
						float colSpeed=-owner->speed.dot(dif)+u->speed.dot(dif);
						owner->speed+=dif*colSpeed*(1-part);
						u->speed-=dif*colSpeed*(part);
//						float damage=(((*ui)->speed-owner->speed)*0.1).SqLength();
//						owner->DoDamage(DamageArray()*damage,0,ZeroVector);
//						(*ui)->DoDamage(DamageArray()*damage,0,ZeroVector);
//						owner->speed*=0.99f;
					}
				}
			}
		}
		if(pos.x<0){
			pos.x+=0.6;
			owner->midPos.x+=0.6;
		}	else if(pos.x>float3::maxxpos){
			pos.x-=0.6;
			owner->midPos.x-=0.6;
		}

		if(pos.z<0){
			pos.z+=0.6;
			owner->midPos.z+=0.6;
		}else if(pos.z>float3::maxzpos){
			pos.z-=0.6;
			owner->midPos.z-=0.6;
		}
	}
}

void CTAAirMoveType::SlowUpdate(void)
{
	if(!reservedPad && aircraftState==AIRCRAFT_FLYING && owner->health<owner->maxHealth*repairBelowHealth){
		CAirBaseHandler::LandingPad* lp=airBaseHandler->FindAirBase(owner,8000);
		if(lp){
			AddDeathDependence(lp);
			reservedPad=lp;
			padStatus=0;
			oldGoalPos=goalPos;
		}
	}

	UpdateMoveRate();

	//Update LOS stuff
	int newmapSquare=ground->GetSquare(owner->pos);
	if(newmapSquare!=owner->mapSquare){
		owner->mapSquare=newmapSquare;
		float oldlh=owner->losHeight;
		float h=owner->pos.y-ground->GetApproximateHeight(owner->pos.x,owner->pos.z);
		owner->losHeight=h+5;
		loshandler->MoveUnit(owner,false);
		if(owner->radarRadius || owner->jammerRadius || owner->sonarRadius)
			radarhandler->MoveUnit(owner);

		owner->losHeight=oldlh;
	}
	qf->MovedUnit(owner);
	owner->isUnderWater=owner->pos.y+owner->model->height<0;
}

//Returns true if indicated position is a suitable landing spot
bool CTAAirMoveType::CanLandAt(float3 pos)
{
	//Sometimes we can never land
	if (dontLand)
		return false;

	if(pos.x<0 || pos.z<0 || pos.x>float3::maxxpos || pos.z>float3::maxzpos)
		return false;

	float h = ground->GetApproximateHeight(pos.x, pos.z);
	if (h < 0)
		return false;

	float3 tpos=owner->pos;
	owner->pos=pos;
	int2 mp=owner->GetMapPos();
	owner->pos=tpos;

	for(int z=mp.y; z<mp.y+owner->ysize; z++){
		for(int x=mp.x; x<mp.x+owner->xsize; x++){
			if(readmap->groundBlockingObjectMap[z*gs->mapx+x] && readmap->groundBlockingObjectMap[z*gs->mapx+x]!=owner){
				return false;
			}
		}
	}	
	return true;
}

//Returns true and sets newPos to a suitable landing spot near curPos if possible, otherwise returns false
bool CTAAirMoveType::FindLandingSpot(float3 curPos, float3 &newPos)
{
	return false;
}

void CTAAirMoveType::ForceHeading(short h)
{
	forceHeading=true;
	forceHeadingTo=h;
}

void CTAAirMoveType::SetWantedAltitude(float altitude)
{
	if(altitude==0)
		wantedHeight=orgWantedHeight;
	else
		wantedHeight=altitude;
}

void CTAAirMoveType::CheckForCollision(void)
{
	float3& pos=owner->midPos;
	float3 forward=owner->speed;
	forward.Normalize();
	float3 midTestPos=pos+forward*121;

	std::vector<CUnit*> others=qf->GetUnitsExact(midTestPos,115);

	float dist=200;
	if(lastColWarning){
		DeleteDeathDependence(lastColWarning);
		lastColWarning=0;
		lastColWarningType=0;
	}

	for(std::vector<CUnit*>::iterator ui=others.begin();ui!=others.end();++ui){
		if(*ui==owner || !(*ui)->unitDef->canfly)
			continue;
		float3& op=(*ui)->midPos;
		float3 dif=op-pos;
		float3 forwardDif=forward*(forward.dot(dif));
		if(forwardDif.SqLength()<dist*dist){
			float frontLength=forwardDif.Length();
			float3 ortoDif=dif-forwardDif;
			float minOrtoDif=((*ui)->radius+owner->radius)*2+frontLength*0.05+5;		//note that the radiuses is multiplied by two since we rely on the aircrafts having to small radiuses (see unitloader)
			if(ortoDif.SqLength()<minOrtoDif*minOrtoDif){
				dist=frontLength;
				lastColWarning=(*ui);
			}
		}
	}
	if(lastColWarning){
		lastColWarningType=2;
		AddDeathDependence(lastColWarning);
		return;
	}
	for(std::vector<CUnit*>::iterator ui=others.begin();ui!=others.end();++ui){
		if(*ui==owner)
			continue;
		if(((*ui)->midPos-pos).SqLength()<dist*dist){
			lastColWarning=*ui;
		}
	}
	if(lastColWarning){
		lastColWarningType=1;
		AddDeathDependence(lastColWarning);
	}
	return;
}

void CTAAirMoveType::DependentDied(CObject* o)
{
	if(o==reservedPad){
		reservedPad=0;
		SetState(AIRCRAFT_FLYING);
		goalPos=oldGoalPos;
	}
	if(o==lastColWarning){
		lastColWarning=0;
		lastColWarningType=0;
	}
	CMoveType::DependentDied(o);
}
