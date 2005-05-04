#include "stdafx.h"
#include ".\airmovetype.h"
#include "quadfield.h"
#include "ground.h"
#include "sound.h"
#include "loshandler.h"
#include "smokeprojectile.h"
#include "gamehelper.h"
//#include "weapon.h"
#include "3doparser.h"
#include "radarhandler.h"
#include "CobFile.h"
#include "CobInstance.h"
#include "infoconsole.h"
#include "unitdef.h"
#include "player.h"
#include "geometricobjects.h"
#include "Mobility.h"
#include "mymath.h"

CAirMoveType::CAirMoveType(CUnit* owner):
	CMoveType(owner),
	wingDrag(0.07),
	wingAngle(0.1),
	frontToSpeed(0.04),
	speedToFront(0.01),
	maxBank(0.55),
	maxPitch(0.35),
	maxAcc(0.006),
	maxAileron(0.04),
	maxElevator(0.02),
	maxRudder(0.01),
	goalPos(1000,0,1000),
	wantedHeight(80),
	invDrag(0.995),
	aircraftState(AIRCRAFT_LANDED),
	inSupply(0),
	subState(0),
	myGravity(0.8),
	mySide(1),
	isFighter(false),
	oldpos(0,0,0),
	oldGoalPos(0,1,0),
	maneuver(0),
	maneuverSubState(0),
	inefficientAttackTime(0),
	reservedLandingPos(-1,-1,-1),
	oldSlowUpdatePos(-1,-1,-1),
	lastColWarning(0),
	lastColWarningType(0)
{
	turnRadius=150;
	owner->mapSquare+=1;						//to force los recalculation

	//From Aircraft::Init
	wantedHeight+=(gs->randFloat()-0.3)*15*(isFighter?2:1);
	maxRudder*=0.99+gs->randFloat()*0.02;
	maxElevator*=0.99+gs->randFloat()*0.02;
	maxAileron*=0.99+gs->randFloat()*0.02;
	maxAcc*=0.99+gs->randFloat()*0.02;

	crashAileron=1-gs->randFloat()*gs->randFloat();
	if(gs->randInt()&1)
		crashAileron=-crashAileron;
	crashElevator=gs->randFloat();
	crashRudder=gs->randFloat()-0.5;

	lastRudderUpdate=gs->frameNum;
	lastElevatorPos=0;
	lastRudderPos=0;
	lastAileronPos=0;

	exitVector=gs->randVector();
	if(exitVector.y<0)
		exitVector.y=-exitVector.y;
	exitVector.y+=1;
}

CAirMoveType::~CAirMoveType(void)
{
}

void CAirMoveType::Update(void)
{
	float3 &pos=owner->pos;

	//This is only set to false after the plane has finished constructing
	if (useHeading){
		useHeading = false;
		SetState(AIRCRAFT_TAKEOFF);
	}

#ifdef DEBUG_AIRCRAFT
	lines.clear();
#endif
#ifdef DIRECT_CONTROL_ALLOWED
	if(owner->directControl && !(aircraftState==AIRCRAFT_CRASHING)){
		SetState(AIRCRAFT_FLYING);
		DirectControlStruct* dc=owner->directControl;
		
		inefficientAttackTime=0;
		if(dc->forward || dc->back || dc->left || dc->right){
			float aileron=0;
			float elevator=0;
			if(dc->forward)
				elevator-=1;
			if(dc->back)
				elevator+=1;
			if(dc->right)
				aileron+=1;
			if(dc->left)
				aileron-=1;
			UpdateAirPhysics(0,aileron,elevator,1,owner->frontdir);
			maneuver=0;
			goto EndNormalControl;		//ok so goto is bad i know
		}
	}
#endif
	switch(aircraftState){
	case AIRCRAFT_FLYING:
#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		info->AddLine("Flying %i %i %.1f %i",moveState,fireState,inefficientAttackTime,(int)isFighter);
	}
#endif
		if(owner->userTarget || owner->userAttackGround){
			inefficientAttackTime=min(inefficientAttackTime,gs->frameNum-owner->lastFireWeapon);
			if(owner->userTarget){
				goalPos=owner->userTarget->pos;
			} else {
				goalPos=owner->userAttackPos;
			}
			if(maneuver){
				UpdateManeuver();
				inefficientAttackTime=0;
			} else if(isFighter && goalPos.distance(pos)<owner->maxRange*4){
				inefficientAttackTime++;
				UpdateFighterAttack();
			}else{
				inefficientAttackTime=0;
				UpdateAttack();
			}
		}else{
			inefficientAttackTime=0;
			UpdateFlying(wantedHeight,1);
		}
		break;
	case AIRCRAFT_LANDED:
		inefficientAttackTime=0;
		UpdateLanded();
		break;
	case AIRCRAFT_LANDING:
		inefficientAttackTime=0;
		UpdateLanding();
		break;
	case AIRCRAFT_CRASHING:
		owner->crashing=true;
		UpdateAirPhysics(crashRudder,crashAileron,crashElevator,0,owner->frontdir);
		new CSmokeProjectile(owner->midPos,gs->randVector()*0.08,100+gs->randFloat()*50,5,0.2,owner,0.4);
		if(!(gs->frameNum&3) && max(0,ground->GetApproximateHeight(pos.x,pos.z))+5+owner->radius>pos.y)
			owner->KillUnit(true,false);
		break;
	case AIRCRAFT_TAKEOFF:
		UpdateTakeOff(wantedHeight);
	default:
		break;
	}
EndNormalControl:
	if(pos!=oldpos){
		oldpos=pos;
		if(aircraftState!=AIRCRAFT_TAKEOFF){		//dont check cols when taking off (hack to avoid being repulsed by build fac)
			vector<CUnit*> nearUnits=qf->GetUnitsExact(pos,owner->radius+6);
			vector<CUnit*>::iterator ui;
			for(ui=nearUnits.begin();ui!=nearUnits.end();++ui){
				float sqDist=(pos-(*ui)->pos).SqLength();
				float totRad=owner->radius+(*ui)->radius;
				if(sqDist<totRad*totRad && sqDist!=0){
					float dist=sqrt(sqDist);
					float3 dif=pos-(*ui)->pos;
					dif/=dist;
					if((*ui)->immobile){
						pos-=dif*(dist-totRad);
						owner->midPos=pos+owner->frontdir*owner->relMidPos.z + owner->updir*owner->relMidPos.y + owner->rightdir*owner->relMidPos.x;	
						owner->speed*=0.99f;
						float damage=(((*ui)->speed-owner->speed)*0.1).SqLength();
						owner->DoDamage(DamageArray()*damage,0,ZeroVector);
						(*ui)->DoDamage(DamageArray()*damage,0,ZeroVector);
					} else {
						float part=owner->mass/(owner->mass+(*ui)->mass);
						pos-=dif*(dist-totRad)*(1-part);
						owner->midPos=pos+owner->frontdir*owner->relMidPos.z + owner->updir*owner->relMidPos.y + owner->rightdir*owner->relMidPos.x;	
						CUnit* u=(CUnit*)(*ui);
						u->pos+=dif*(dist-totRad)*(part);
						u->midPos=u->pos+u->frontdir*u->relMidPos.z + u->updir*u->relMidPos.y + u->rightdir*u->relMidPos.x;	
						float damage=(((*ui)->speed-owner->speed)*0.1).SqLength();
						owner->DoDamage(DamageArray()*damage,0,ZeroVector);
						(*ui)->DoDamage(DamageArray()*damage,0,ZeroVector);
						owner->speed*=0.99f;
					}
				}
			}
		}
		if(pos.x<0){
			pos.x+=1.5;
			owner->midPos.x+=1.5;
		}else if(pos.x>float3::maxxpos){
			pos.x-=1.5;
			owner->midPos.x-=1.5;
		}

		if(pos.z<0){
			pos.z+=1.5;
			owner->midPos.z+=1.5;
		}else if(pos.z>float3::maxzpos){
			pos.z-=1.5;
			owner->midPos.z-=1.5;
		}
	}
#ifdef DEBUG_AIRCRAFT
	if(lastColWarningType==1){
		int g=geometricObjects->AddLine(owner->pos,lastColWarning->pos,10,1,1);
		geometricObjects->SetColor(g,0.2,1,0.2,0.6);
	} else if(lastColWarningType==2){
		int g=geometricObjects->AddLine(owner->pos,lastColWarning->pos,10,1,1);
		if(owner->frontdir.dot(lastColWarning->midPos+lastColWarning->speed*20 - owner->midPos - owner->speed*20)<0)
			geometricObjects->SetColor(g,1,0.2,0.2,0.6);
		else
			geometricObjects->SetColor(g,1,1,0.2,0.6);
	}
#endif
}

void CAirMoveType::SlowUpdate(void)
{
	if(owner->pos!=oldSlowUpdatePos){
		oldSlowUpdatePos=owner->pos;
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
}

void CAirMoveType::UpdateManeuver(void)
{
#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		info->AddLine("UpdataMan %i %i",maneuver,maneuverSubState);
	}
#endif
	float speedf=owner->speed.Length();
	switch(maneuver){
	case 1:{
		int aileron=0,elevator=0;
		if(owner->updir.y>0){
			if(owner->rightdir.y>maxAileron*speedf){
				aileron=1;
			}else if(owner->rightdir.y<-maxAileron*speedf){
				aileron=-1;
			}
		}
		if(fabs(owner->rightdir.y)<maxAileron*3*speedf || owner->updir.y<0)
			elevator=1;
		UpdateAirPhysics(0,aileron,elevator,1,owner->frontdir);
		if((owner->updir.y<0 && owner->frontdir.y<0) || speedf<0.8)
			maneuver=0;
		break;}
	case 2:{
		int aileron=0,elevator=0;
		if(maneuverSubState==0){
			if(owner->rightdir.y>=0){
				aileron=-1;
			}else{
				aileron=1;
			}
		}
		if(owner->frontdir.y<-0.7)
			maneuverSubState=1;
		if(maneuverSubState==1 || owner->updir.y<0)
			elevator=1;
		UpdateAirPhysics(0,aileron,elevator,1,owner->frontdir);

		if((owner->updir.y>0 && owner->frontdir.y>0 && maneuverSubState==1) || speedf<0.2)
			maneuver=0;
		break;}
	default:
		UpdateAirPhysics(0,0,0,1,owner->frontdir);
		maneuver=0;
		break;
	}
}

void CAirMoveType::UpdateFighterAttack(void)
{
	float3 &pos = owner->pos;
	float3 &rightdir = owner->rightdir;
	float3 &frontdir = owner->frontdir;
	float3 &updir = owner->updir;
	float3 &speed = owner->speed;

	float speedf=owner->speed.Length();
	if(speedf<0.01){
		UpdateAirPhysics(0,0,0,1,owner->frontdir);
		return;
	}

	if(!((gs->frameNum+owner->id)&3))
		CheckForCollision();

	bool groundTarget=!owner->userTarget || !owner->userTarget->unitDef->canfly;
	if(groundTarget){
		if(frontdir.dot(goalPos-pos)<0 && (pos-goalPos).SqLength()<turnRadius*turnRadius){
			float3 dif=pos-goalPos;
			dif.y=0;
			dif.Normalize();
			goalPos=goalPos+dif*turnRadius*4;
		} else if(frontdir.dot(goalPos-pos)<owner->maxRange*0.7){
			goalPos+=exitVector*(owner->userTarget?owner->userTarget->radius+owner->radius+10:owner->radius+40);
		}
	}
	float3 tgp=goalPos+(goalPos-oldGoalPos)*8;
	oldGoalPos=goalPos;
	goalPos=tgp;

	float goalLength=(goalPos-pos).Length();
	float3 goalDir=(goalPos-pos)/goalLength;
	
	float aileron=0;
	float rudder=0;
	float elevator=0;
	float engine=0;

	float gHeight=ground->GetHeight(pos.x,pos.z);

	float goalDot=rightdir.dot(goalDir);
	goalDot/=goalDir.dot(frontdir)*0.5+0.501;

	if(goalDir.dot(frontdir)<-0.2+inefficientAttackTime*0.002 && frontdir.y>-0.2 && speedf>2.0 && gs->randFloat()>0.996)
		maneuver=1;

	if(goalDir.dot(frontdir)<-0.2+inefficientAttackTime*0.002 && fabs(frontdir.y)<0.2 && gs->randFloat()>0.996 && gHeight+400<pos.y){
		maneuver=2;
		maneuverSubState=0;
	}

	//roll
	if(speedf>0.45 && pos.y+owner->speed.y*60*fabs(frontdir.y)+min(0,updir.y)*150>gHeight+60+fabs(rightdir.y)*150){
		float goalBankDif=goalDot+rightdir.y*0.2;
		if(goalBankDif>maxAileron*speedf*4){
			aileron=1;
		} else if(goalBankDif<-maxAileron*speedf*4){
			aileron=-1;
		} else {
			aileron=goalBankDif/(maxAileron*speedf*4);
		}
	} else {
		if(rightdir.y>0){
			if(rightdir.y>maxAileron*speedf || frontdir.y<-0.7)
				aileron=1;
			else
				aileron=rightdir.y/(maxAileron*speedf);
		} else {
			if(rightdir.y<-maxAileron*speedf || frontdir.y<-0.7)
				aileron=-1;
			else 
				aileron=rightdir.y/(maxAileron*speedf);
		}
	}

	//yaw
	if(pos.y>gHeight+30){
		if(goalDot<-maxRudder*speedf){
			rudder=-1;
		} else if(goalDot>maxRudder*speedf){
			rudder=1;
		} else {
			rudder=goalDot/(maxRudder*speedf);
		}
	}

	float upside=1;
	if(updir.y<-0.3)
		upside=-1;

	//pitch
	if(speedf<1.5){
		if(frontdir.y<0.0){
			elevator=upside;
		} else if(frontdir.y>0.0){
			elevator=-upside;
		}
	} else {
		float gHeight2=ground->GetHeight(pos.x+speed.x*30,pos.z+speed.z*30);
		float hdif=max(gHeight,gHeight2)+60-pos.y-frontdir.y*speedf*20;
		float minPitch;//=min(1.0f,hdif/(maxElevator*speedf*speedf*20));

		if(hdif<-(maxElevator*speedf*speedf*20)){
			minPitch=-1;
		} else if(hdif>(maxElevator*speedf*speedf*20)){
			minPitch=1;
		} else {
			minPitch=hdif/(maxElevator*speedf*speedf*20);
		}

/*		if(pos.y+min(0,owner->speed.y)*70*fabs(frontdir.y)+min(0,updir.y)*50<gHeight+50){
		if(frontdir.y<0.5){
			elevator=upside;
		} else if(frontdir.y>0.55){
			elevator=-upside;
		}*/
//	} else {
		if(lastColWarningType==2 && frontdir.dot(lastColWarning->pos+lastColWarning->speed*20-pos-owner->speed*20)<0){
/*			float pitchMod=updir.y>0?1:-1;
			if(lastColWarning->pos.y>pos.y)
				elevator=-pitchMod;
			else
				elevator=pitchMod;
/*/			elevator=updir.dot(lastColWarning->midPos-owner->midPos)>0?-1:1;/**/
		} else {
			float hdif=goalDir.dot(updir);
			if(hdif<-maxElevator*speedf){
				elevator=-1;
			} else if(hdif>maxElevator*speedf){
				elevator=1;
			} else {
				elevator=hdif/(maxElevator*speedf);
			}
		}
		if(elevator*upside<minPitch)
			elevator=minPitch*upside;
	}
#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		info->AddLine("FAttack %.1f %.1f %.2f",pos.y-gHeight,goalLength,goalDir.dot(frontdir));
	}
#endif

	if(groundTarget)
		engine=1;
	else
		engine=min(1,goalLength/owner->maxRange+1-goalDir.dot(frontdir)*0.7);

	UpdateAirPhysics(rudder,aileron,elevator,engine,owner->frontdir);
/*
	std::vector<CWeapon*>::iterator wi;
	for(wi=owner->weapons.begin();wi!=owner->weapons.end();++wi){
		(*wi)->targetPos=goalPos;
		if(owner->userTarget){
			(*wi)->AttackUnit(owner->userTarget,true);
		}
	}*/
/*	DrawLine dl;
	dl.color=UpVector;
	dl.pos1=pos;
	dl.pos2=goalPos;
	lines.push_back(dl);
	dl.color=float3(1,0,0);
	dl.pos1=pos;
	dl.pos2=pos+frontdir*maxRange;
	lines.push_back(dl);/**/
}

void CAirMoveType::UpdateAttack(void)
{
/*	std::vector<CWeapon*>::iterator wi;
	for(wi=owner->weapons.begin();wi!=owner->weapons.end();++wi){
		(*wi)->targetPos=goalPos;
		if(owner->userTarget){
			(*wi)->AttackUnit(owner->userTarget,true);
		}
	}
*/
	UpdateFlying(wantedHeight,1);
}

void CAirMoveType::UpdateFlying(float wantedHeight,float engine)
{
	float3 &pos = owner->pos;
	float3 &rightdir = owner->rightdir;
	float3 &frontdir = owner->frontdir;
	float3 &updir = owner->updir;
	float3 &speed = owner->speed;

	float speedf=speed.Length();
	float goalLength=(goalPos-pos).Length();
	float3 goalDir=(goalPos-pos)/goalLength;
	goalDir.Normalize();

	float aileron=0;
	float rudder=0;
	float elevator=0;

	float gHeight=ground->GetHeight(pos.x,pos.z);

	if(!((gs->frameNum+owner->id)&3))
		CheckForCollision();

	float otherThreat=0;
	float3 otherDir;
	if(lastColWarning){
		float3 otherDif=lastColWarning->pos-pos;
		float otherLength=otherDif.Length();
		otherDir=otherDif/otherLength;
		otherThreat=max(1200,goalLength)/otherLength*0.036;
	}

	float goalDot=rightdir.dot(goalDir);
	goalDot/=goalDir.dot(frontdir)*0.5+0.501;
	if(goalDir.dot(frontdir)<-0.1 && goalLength<turnRadius
#ifdef DIRECT_CONTROL_ALLOWED
		&& (!owner->directControl || owner->directControl->mouse2)
#endif
		)
		goalDot=-goalDot;
	if(lastColWarning){
		goalDot-=otherDir.dot(rightdir)*otherThreat;
	}
	//roll
	if(speedf>1.5 && pos.y+speed.y*10>gHeight+wantedHeight*0.6){
		float goalBankDif=goalDot+rightdir.y*0.5;
		if(goalBankDif>maxAileron*speedf*4 && rightdir.y>-maxBank){
			aileron=1;
		} else if(goalBankDif<-maxAileron*speedf*4 && rightdir.y<maxBank){
			aileron=-1;
		} else {
			if(fabs(rightdir.y)<maxBank)
				aileron=goalBankDif/(maxAileron*speedf*4);
			else {
				if(rightdir.y<0 && goalBankDif<0)
					aileron=-1;
				else if(rightdir.y>0 && goalBankDif>0)
					aileron=1;
			}
		}
	} else {
		if(rightdir.y>0.01){
			aileron=1;
		} else if(rightdir.y<-0.01){
			aileron=-1;
		}
	}

	//yaw
	if(pos.y>gHeight+15){
		if(goalDot<-maxRudder*speedf*2){
			rudder=-1;;
		} else if(goalDot>maxRudder*speedf*2){
			rudder=1;
		} else {
			rudder=goalDot/(maxRudder*speedf*2);
		}
	}

	//pitch
	if(speedf>0.8){
		if(lastColWarningType==2 && frontdir.dot(lastColWarning->midPos+lastColWarning->speed*20 - owner->midPos - owner->speed*20)<0){
/*			float pitchMod=updir.y>0?1:-1;
			if(lastColWarning->pos.y>pos.y)
				elevator=-pitchMod;
			else
				elevator=pitchMod;
/*/			elevator=updir.dot(lastColWarning->midPos-owner->midPos)>0?-1:1;/**/
		} else {
			float gHeight2=ground->GetHeight(pos.x+speed.x*40,pos.z+speed.z*40);
			float hdif=max(gHeight,gHeight2)+wantedHeight-pos.y-frontdir.y*speedf*20;
			if(hdif<-(maxElevator*speedf*speedf*20) && frontdir.y>-maxPitch){
				elevator=-1;
			} else if(hdif>(maxElevator*speedf*speedf*20) && frontdir.y<maxPitch){
				elevator=1;
			} else {
				if(fabs(frontdir.y)<maxPitch)
					elevator=hdif/(maxElevator*speedf*speedf*20);
			}
		}
	} else {
		if(frontdir.y<-0.1){
			elevator=1;
		} else if(frontdir.y>0.15){
			elevator=-1;
		}
	}
	UpdateAirPhysics(rudder,aileron,elevator,engine,owner->frontdir);
}

void CAirMoveType::UpdateLanded(void)
{
	float3 &pos = owner->pos;
	float3 &rightdir = owner->rightdir;
	float3 &frontdir = owner->frontdir;
	float3 &updir = owner->updir;
	float3 &speed = owner->speed;

	speed=ZeroVector;

	updir=ground->GetNormal(pos.x,pos.z);
	frontdir.Normalize();
	rightdir=frontdir.cross(updir);
	rightdir.Normalize();
	updir=rightdir.cross(frontdir);
}

void CAirMoveType::UpdateTakeOff(float wantedHeight)
{
	float3& pos=owner->pos;
	float3& speed=owner->speed;
	float h=pos.y-ground->GetHeight(pos.x,pos.z);
	
	if(h>wantedHeight)
		SetState(AIRCRAFT_FLYING);

	if(h+speed.y*20<wantedHeight*1.02)
		speed.y+=maxAcc;
	else
		speed.y-=maxAcc;
	if(h>wantedHeight*0.4)
		speed+=owner->frontdir*maxAcc;

	speed*=invDrag;

	owner->frontdir.Normalize();
	owner->rightdir=owner->frontdir.cross(owner->updir);
	owner->rightdir.Normalize();
	owner->updir=owner->rightdir.cross(owner->frontdir);

	pos+=speed;
	owner->midPos=pos+owner->frontdir*owner->relMidPos.z + owner->updir*owner->relMidPos.y + owner->rightdir*owner->relMidPos.x;
}

void CAirMoveType::UpdateLanding(void)
{
	float3 &pos = owner->pos;
	float3 &rightdir = owner->rightdir;
	float3 &frontdir = owner->frontdir;
	float3 &updir = owner->updir;
	float3 &speed = owner->speed;
	float speedf=speed.Length();

	//find a landing spot if we dont have one
	if(reservedLandingPos.x<0){
		reservedLandingPos=FindLandingPos();
		if(reservedLandingPos.x>0){
			reservedLandingPos.y+=wantedHeight;
			float3 tp=pos;
			pos=reservedLandingPos;
			owner->physicalState = CSolidObject::OnGround;
			owner->Block();
			owner->physicalState = CSolidObject::Flying;
			pos=tp;
			owner->Deactivate();
			owner->cob->Call(COBFN_StopMoving);
		} else {
			UpdateFlying(wantedHeight,1);
			return;
		}
	} else {	//see if landing spot is still empty
/*		float3 tpos=owner->pos;
		owner->pos=reservedLandingPos;
		int2 mp=owner->GetMapPos();
		owner->pos=tpos;

		for(int z=mp.y; z<mp.y+owner->ysize; z++){
			for(int x=mp.x; x<mp.x+owner->xsize; x++){
				if(readmap->groundBlockingObjectMap[z*gs->mapx+x]!=owner){
					owner->UnBlock();
					reservedLandingPos.x=-1;
					UpdateFlying(wantedHeight,1);
					return;
				}
			}
		}*/
	}

	//update our speed
	float3 dif=reservedLandingPos-pos;
	float dist=dif.Length();
	dif/=dist;
	
	float wsf=min(owner->unitDef->speed,dist/speedf*1.8*maxAcc);
	float3 wantedSpeed=dif*wsf;

	float3 delta = wantedSpeed - speed;
	float dl=delta.Length();

	if(dl<maxAcc*3)
		speed=wantedSpeed;
	else
		speed+=delta/dl*maxAcc*3;

	pos+=speed;

	//make the aircraft right itself up and turn toward goal
	if(rightdir.y<-0.01)
		updir-=rightdir*0.02;
	else if(rightdir.y>0.01)
		updir+=rightdir*0.02;

	if(frontdir.y<-0.01)
		frontdir+=updir*0.02;
	else if(frontdir.y>0.01)
		frontdir-=updir*0.02;

	if(rightdir.dot(dif)>0.01)
		frontdir+=rightdir*0.02;
	else if(rightdir.dot(dif)<-0.01)
		frontdir-=rightdir*0.02;

	frontdir.Normalize();
	rightdir=frontdir.cross(updir);
	rightdir.Normalize();
	updir=rightdir.cross(frontdir);

	owner->midPos=pos+frontdir*owner->relMidPos.z + updir*owner->relMidPos.y + rightdir*owner->relMidPos.x;

	//see if we are at the landing spot
	if(dist<1){
		float h=ground->GetHeight(pos.x,pos.z);
		if(abs(reservedLandingPos.y-h)>1)
			reservedLandingPos.y=h;
		else{
			SetState(AIRCRAFT_LANDED);
		}
	}
}

void CAirMoveType::UpdateAirPhysics(float rudder, float aileron, float elevator,float engine,const float3& engineVector)
{
	float3 &pos = owner->pos;
	float3 &rightdir = owner->rightdir;
	float3 &frontdir = owner->frontdir;
	float3 &updir = owner->updir;
	float3 &speed = owner->speed;

	lastRudderPos=rudder;
	lastAileronPos=aileron;
	lastElevatorPos=elevator;

	float speedf=speed.Length();
	float3 speeddir=frontdir;
	if(speedf!=0)
		speeddir=speed/speedf;

	float gHeight=ground->GetHeight(pos.x,pos.z);

#ifdef DIRECT_CONTROL_ALLOWED
	if(owner->directControl)
		if((pos.y-gHeight)>wantedHeight*1.2)
			engine=max(0,min(engine,1-(pos.y-gHeight-wantedHeight*1.2)/wantedHeight));
#endif

	speed+=engineVector*maxAcc*engine;

	speed.y+=gs->gravity*myGravity;
	speed*=invDrag;

	float3 wingDir=updir*(1-wingAngle)-frontdir*wingAngle;
	float wingForce=wingDir.dot(speed)*wingDrag;
	speed-=wingDir*wingForce;

	frontdir+=rightdir*rudder*maxRudder*speedf;
	updir+=rightdir*aileron*maxAileron*speedf;
	frontdir+=updir*elevator*maxElevator*speedf;
	frontdir+=(speeddir-frontdir)*frontToSpeed;
	speed+=(frontdir*speedf-speed)*speedToFront;
	pos+=speed;

	if(gHeight>owner->pos.y-owner->model->radius*0.2){
/*		float damage=0;
		if(speed.y>0.05)
			damage+=speed.y*speed.y*1000;
		if(updir.y<0.98)
			damage+=(1-updir.y)*1000;
		if(damage>0)
			owner->DoDamage(DamageArray()*(damage*0.1),0,ZeroVector);	//dont do damage until they can avoid ground better
*/
		pos.y=gHeight+owner->model->radius*0.2+0.01;
		speed.y=speed.Length2D()*0.5;
		updir=UpVector;
		frontdir.y=0.5;
	}

	frontdir.Normalize();
	rightdir=frontdir.cross(updir);
	rightdir.Normalize();
	updir=rightdir.cross(frontdir);

	owner->midPos=pos+frontdir*owner->relMidPos.z + updir*owner->relMidPos.y + rightdir*owner->relMidPos.x;
#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		info->AddLine("UpdataAP %.1f %.1f %.1f %.1f",speedf,pos.x,pos.y,pos.z);
//		info->AddLine("Rudders %.1f %.1f %.1f %.1f",rudder,aileron,elevator,engine);
	}
#endif
}

void CAirMoveType::SetState(CAirMoveType::AircraftState state)
{
	if(aircraftState==AIRCRAFT_CRASHING || state==aircraftState)
		return;

/*	if (state == AIRCRAFT_LANDING)
		owner->cob->Call(COBFN_Deactivate);
		else if (state == aircraft_flying)
		//cob->Call(COBFN_Activate); */
	
	if (state == AIRCRAFT_FLYING) {
		owner->Activate();
		owner->cob->Call(COBFN_StartMoving);
	}

	if(state==AIRCRAFT_LANDED){
		owner->heading=GetHeadingFromVector(owner->frontdir.x, owner->frontdir.z);
		owner->physicalState = CSolidObject::OnGround;
		owner->useAirLos=false;
	}else{
		owner->physicalState = CSolidObject::Flying;
		owner->useAirLos=true;
		if(state!=AIRCRAFT_LANDING){
			reservedLandingPos.x=-1;
			owner->UnBlock();
		}
	}

	if(aircraftState==AIRCRAFT_LANDED && reservedLandingPos.x>0){
		reservedLandingPos.x=-1;
	}
	subState=0;
	if(state!=AIRCRAFT_TAKEOFF || aircraftState==AIRCRAFT_LANDED)		//make sure we only go into takeoff if actually landed
		aircraftState=state;
	else
		aircraftState=AIRCRAFT_TAKEOFF;
}

void CAirMoveType::ImpulseAdded(void)
{
	if(aircraftState==AIRCRAFT_FLYING){
		owner->speed+=owner->residualImpulse;
		owner->residualImpulse=ZeroVector;
	}
}

float3 CAirMoveType::FindLandingPos(void)
{
	float3 ret(-1,-1,-1);

	float3 tryPos=owner->pos+owner->speed*owner->speed.Length()/(maxAcc*3);
	tryPos.y=ground->GetHeight2(tryPos.x,tryPos.z);

	if(tryPos.y<0)
		return ret;
	float3 tpos=owner->pos;
	owner->pos=tryPos;
	int2 mp=owner->GetMapPos();
	owner->pos=tpos;

	for(int z=mp.y; z<mp.y+owner->ysize; z++){
		for(int x=mp.x; x<mp.x+owner->xsize; x++){
			if(readmap->groundBlockingObjectMap[z*gs->mapx+x]){
				return ret;
			}
		}
	}

	if(ground->GetSlope(tryPos.x,tryPos.z)>0.03)
		return ret;

	return tryPos;
}

void CAirMoveType::CheckForCollision(void)
{
	float3& pos=owner->midPos;
	float3& forward=owner->frontdir;
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
			float minOrtoDif=((*ui)->radius+owner->radius)*2+frontLength*0.1+10;		//note that the radiuses is multiplied by two since we rely on the aircrafts having to small radiuses (see unitloader)
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

void CAirMoveType::DependentDied(CObject* o)
{
	if(o==lastColWarning){
		lastColWarning=0;
		lastColWarningType=0;
	}
	CMoveType::DependentDied(o);
}
