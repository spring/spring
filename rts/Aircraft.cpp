#include "stdafx.h"
#include "aircraft.h"
#include "commandai.h"
#include "mygl.h"
#include <gl/glu.h>
#include "unithandler.h"
#include "gamehelper.h"
#include "ground.h"
#include "loshandler.h"
#include "quadfield.h"
#include "team.h"
#include "infoconsole.h"
#include "smokeprojectile.h"
#include "weapon.h"
#include "readmap.h"
#include "projectilehandler.h"
#include "3doparser.h"
#include ".\aircraft.h"
#include "cobinstance.h"
#include "cobfile.h"
#include "selectedunits.h"
//#include "unit3dloader.h"

//#include "mmgr.h"

//#define DEBUG_AIRCRAFT

float CAircraft::transMatrix[16];

CAircraft::CAircraft(float3 &pos,int team)
: CUnit(pos,team),
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
	basePos(pos),
	baseDir(1,0,0),
	bingoFuel(false),
	subState(0),
	myGravity(0.8),
	mySide(1),
	isFighter(false),
	oldpos(0,0,0),
	oldGoalPos(0,1,0),
	maneuver(0),
	maneuverSubState(0),
	inefficientAttackTime(0)
{
	for(int a=0;a<16;++a)
		transMatrix[a]=0;
	transMatrix[15]=1;

	turnRadius=150;
	mapSquare+=1;						//to force los recalculation

	armorType=Armor_Aircraft;
}

CAircraft::~CAircraft(void)
{
}

void CAircraft::Update(void)
{
	if(beingBuilt)
		return;

	if(pos.x<4)
		pos.x=4;
	if(pos.x>g.mapx*SQUARE_SIZE-4)
		pos.x=g.mapx*SQUARE_SIZE-4;
	if(pos.z<4)
		pos.z=4;
	if(pos.z>g.mapy*SQUARE_SIZE-4)
		pos.z=g.mapy*SQUARE_SIZE-4;

	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi){
		(*wi)->Update();
	}

	bonusShieldSaved+=0.1;
	lines.clear();
	switch(aircraftState){
	case AIRCRAFT_FLYING:
#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		info->AddLine("Flying %i %i %.1f %i",moveState,fireState,inefficientAttackTime,(int)isFighter);
	}
#endif
		if(userTarget || userAttackGround){
			inefficientAttackTime=min(inefficientAttackTime,g.frameNum-lastFireWeapon);
			if(userTarget){
				goalPos=userTarget->pos;
			} else {
				goalPos=userAttackPos;
			}
			if(maneuver){
				UpdateManeuver();
				inefficientAttackTime=0;
			} else if(isFighter && goalPos.distance(pos)<maxRange*4){
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
		crashing=true;
		UpdateAirPhysics(crashRudder,crashAileron,crashElevator,0);
		new CSmokeProjectile(midPos,g.randVector()*0.08,100+g.randFloat()*50,5,0.2,this,0.4);
		if(!(g.frameNum&3) && max(0,ground->GetApproximateHeight(pos.x,pos.z))+5+radius>pos.y)
			KillUnit(true);
		break;
	default:
		break;
	}
	if(!(pos==oldpos)){
		oldpos=pos;
		vector<CUnit*> nearUnits=qf->GetUnitsExact(pos,radius+6);
		vector<CUnit*>::iterator ui;
		for(ui=nearUnits.begin();ui!=nearUnits.end();++ui){
			float sqDist=(pos-(*ui)->pos).SqLength();
			float totRad=radius+(*ui)->radius;
			if(sqDist<totRad*totRad && sqDist!=0 && *ui!=this){
				float dist=sqrt(sqDist);
				float3 dif=pos-(*ui)->pos;
				dif/=dist;
				if((*ui)->mass>=100000){
					pos-=dif*(dist-totRad);
					midPos=pos+frontdir*relMidPos.z + updir*relMidPos.y + rightdir*relMidPos.x;	
					speed*=0.99f;
					float damage=((*ui)->speed-speed).SqLength();
					DoDamage(DamageArray()*damage,0);
					(*ui)->DoDamage(DamageArray()*damage,0);
				} else {
					float part=mass/(mass+(*ui)->mass);
					pos-=dif*(dist-totRad)*(1-part);
					midPos=pos+frontdir*relMidPos.z + updir*relMidPos.y + rightdir*relMidPos.x;	
					CUnit* u=(CUnit*)(*ui);
					u->pos+=dif*(dist-totRad)*(part);
					u->midPos=u->pos+u->frontdir*u->relMidPos.z + u->updir*u->relMidPos.y + u->rightdir*u->relMidPos.x;	
					float damage=(((*ui)->speed-speed)*0.1).SqLength();
					DoDamage(DamageArray()*damage,0);
					(*ui)->DoDamage(DamageArray()*damage,0);
					speed*=0.99f;
				}
			}
		}
	}
/*
	DrawLine dl;
	dl.color=float3(1,1,1);
	dl.pos1=basePos;
	dl.pos1.y=ground->GetHeight(dl.pos1.x,dl.pos1.z)+wantedHeight;
	dl.pos2=basePos+baseDir*100;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);
	dl.pos1=pos;
	dl.pos2=goalPos;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);/**/
}

void CAircraft::SlowUpdate(void)
{
	if(beingBuilt)
		return;

	commandAI->SlowUpdate();
	lastSlowUpdate=g.frameNum;

	if(aircraftState==AIRCRAFT_LANDED){
		if(health<maxHealth){
			health+=1;
		}
		bingoFuel=false;
	}
	//g.teams[team]->UseMetalUpkeep(metalUpkeep);
	//g.teams[team]->UseEnergyUpkeep(energyUpkeep);

	if(aircraftState==AIRCRAFT_FLYING || aircraftState==AIRCRAFT_LANDING){
		TestBingoFuel(0);
	}

	for(vector<CWeapon*>::iterator wi=weapons.begin();wi!=weapons.end();++wi)
		(*wi)->SlowUpdate();

	int newmapSquare=ground->GetSquare(pos);
	if(newmapSquare!=mapSquare){
		mapSquare=newmapSquare;
		if(mapSquare<0 || mapSquare>g.mapx*g.mapy){
			char t[500];
			sprintf(t,"Errenous mapsquare for aircraft %i",mapSquare);
			MessageBox(0,"",t,0);
		}
		float oldlh=losHeight;
		float h=pos.y-ground->GetApproximateHeight(pos.x,pos.z);
		if(losHeight>h)
			losHeight=h;
		loshandler->MoveUnit(this,false);
		losHeight=oldlh;
	}
	qf->MovedUnit(this);

	if(experience!=oldExperience)
		ExperienceChange();
}

void CAircraft::Draw(void)
{
	if(beingBuilt){
		CUnit::Draw();
		return;
	}

	float dt=g.frameNum+g.timeOffset-lastRudderUpdate;
	lastRudderUpdate=g.frameNum+g.timeOffset;
/*	float dif=1-pow(0.9f,dt);
	rudder.rotation+=(lastRudderPos-rudder.rotation)*dif;
	elevator.rotation+=(lastElevatorPos-elevator.rotation)*dif;
	aileronRight.rotation+=(lastAileronPos-aileronRight.rotation)*dif;
	aileronLeft.rotation+=(lastAileronPos-aileronLeft.rotation)*dif;
*/	glPushMatrix();
	float3 interPos=pos+speed*g.timeOffset;
	glTranslatef(interPos.x,interPos.y,interPos.z);

	transMatrix[0]=rightdir.x;
	transMatrix[1]=rightdir.y;
	transMatrix[2]=rightdir.z;
	transMatrix[4]=updir.x;
	transMatrix[5]=updir.y;
	transMatrix[6]=updir.z;
	transMatrix[8]=frontdir.x;
	transMatrix[9]=frontdir.y;
	transMatrix[10]=frontdir.z;
	glMultMatrixf(transMatrix);
//	glColor3f(1,1,1);
	if(uh->unitDrawSize!=1 && aihint<1000)
		glScalef(uh->unitDrawSize,uh->unitDrawSize,uh->unitDrawSize);

	//glCallList(model->displist);
	//glCallList(model3do->displist);
	//model->rootobject.DrawStatic();
	localmodel->Draw();
	/*for(std::vector<RudderInfo*>::iterator ri=rudders.begin();ri!=rudders.end();++ri){
		glPushMatrix();
		glTranslatef((*ri)->model->geometry->offset.x,(*ri)->model->geometry->offset.y,(*ri)->model->geometry->offset.z);
		glRotatef((*ri)->rotation*45,(*ri)->model->geometry->rotVector.x,(*ri)->model->geometry->rotVector.y,(*ri)->model->geometry->rotVector.z);
		glCallList((*ri)->model->displist);
		glPopMatrix();
	}*/
	/*if(g.drawdebug){
		GLUquadricObj* q=gluNewQuadric();
		gluQuadricDrawStyle(q,GLU_LINE);
		gluSphere(q,radius,10,10);
		gluDeleteQuadric(q);
	}*/
	glPopMatrix();
/*
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glBegin(GL_LINES);
	std::vector<DrawLine>::iterator li;
	for(li=lines.begin();li!=lines.end();++li){
		glColor3f(li->color.x,li->color.y,li->color.z);
		glVertexf3(li->pos1);
		glVertexf3(li->pos2);
	}
	glEnd();
	glEnable(GL_LIGHTING);
	glEnable(GL_FOG);
	glEnable(GL_TEXTURE_2D);/**/
}

void CAircraft::UpdateAirPhysics(float rudder, float aileron, float elevator,float engine)
{
	lastRudderPos=rudder;
	lastAileronPos=aileron;
	lastElevatorPos=elevator;

	float speedf=speed.Length();
	float3 speeddir=frontdir;
	if(speedf!=0)
		speeddir=speed/speedf;

//	if(supply>0)
		speed+=frontdir*maxAcc*engine;

	speed.y+=GRAVITY*myGravity;
	speed*=invDrag;

	float3 wingDir=updir*(1-wingAngle)-frontdir*wingAngle;
	float wingForce=wingDir.dot(speed)*wingDrag;
	speed-=wingDir*wingForce;

	float gHeight=ground->GetHeight(pos.x,pos.z);

	frontdir+=rightdir*rudder*maxRudder*speedf;
	updir+=rightdir*aileron*maxAileron*speedf;
	frontdir+=updir*elevator*maxElevator*speedf;
	frontdir+=(speeddir-frontdir)*frontToSpeed;
	speed+=(frontdir*speedf-speed)*speedToFront;
	pos+=speed;

	pos.CheckInBounds();

	if(gHeight>pos.y-model->radius){
		float damage=0;
		if(speed.y>0.05)
			damage+=speed.y*speed.y*1000;
		if(updir.y<0.98)
			damage+=(1-updir.y)*1000;
		if(damage>0)
			CUnit::DoDamage(DamageArray()*damage,0);

		pos.y=gHeight+model->radius+0.01;
		speed.y=0;
		updir=float3(0,1,0);
		frontdir.y=0.12;
	}

	frontdir.Normalize();
	rightdir=frontdir.cross(updir);
	rightdir.Normalize();
	updir=rightdir.cross(frontdir);

	midPos=pos+frontdir*relMidPos.z + updir*relMidPos.y + rightdir*relMidPos.x;
#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		info->AddLine("UpdataAP %.1f %.1f %.1f %.1f",speedf,pos.x,pos.y,pos.z);
//		info->AddLine("Rudders %.1f %.1f %.1f %.1f",rudder,aileron,elevator,engine);
	}
#endif
}

void CAircraft::UpdateFlying(float wantedHeight,float engine)
{
	float speedf=speed.Length();
	float goalLength=(goalPos-pos).Length();
	float3 goalDir=(goalPos-pos)/goalLength;
	goalDir.Normalize();

	float aileron=0;
	float rudder=0;
	float elevator=0;

	float gHeight=ground->GetHeight(pos.x,pos.z);

	CUnit* other;
	other=helper->GetClosestUnit(pos+frontdir*154,153);
	float otherThreat=0;
	float3 otherDir;
	if(other && other!=this){
		float3 otherDif=other->pos-pos;
		float otherLength=otherDif.Length();
		otherDir=otherDif/otherLength;
		otherThreat=max(1200,goalLength)/otherLength*0.036;
	}

	float goalDot=rightdir.dot(goalDir);
	goalDot/=goalDir.dot(frontdir)*0.5+0.501;
	if(goalDir.dot(frontdir)<-0.1 && goalLength<turnRadius)
		goalDot=-goalDot;
	if(other){
		goalDot-=otherDir.dot(rightdir)*otherThreat;
	}
	//roll
	if(speedf>1.5 && pos.y+speed.y*10>gHeight+40){
		float goalBankDif=goalDot+rightdir.y;
		if(goalBankDif>maxAileron*speedf*4 && rightdir.y>-maxBank){
			aileron=1;
		} else if(goalBankDif<-maxAileron*speedf*4 && rightdir.y<maxBank){
			aileron=-1;
		} else {
			if(fabs(rightdir.y)<maxBank)
				aileron=goalBankDif/(maxAileron*speedf*4);
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
	if(speedf>1.5 || (aircraftState==AIRCRAFT_LANDING && speedf>1.0)){
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
	} else {
		if(frontdir.y<0.1){
			elevator=1;
		} else if(frontdir.y>0.15){
			elevator=-1;
		}
	}
	UpdateAirPhysics(rudder,aileron,elevator,engine);
}

bool CAircraft::TestBingoFuel(float supplyDif)
{
	//info->AddLine("time %f supply %f %f",basePos.distance2D(pos)/maxSpeed/30,basePos.distance2D(pos)*((flightSupplyUpkeep+supplyUpkeep)*(1.0/16.0)*1.3)/maxSpeed,(supply-supplyDif-0.14));
//	if((basePos+baseDir*100).distance2D(pos)*((flightSupplyUpkeep+supplyUpkeep)*(1.0/16)*1.1)>((supply-supplyDif-0.14)*maxSpeed)){
	return true;/*
	if(bingoFuel){
		return true;
	}
	if(supply<supplyDif+0.01){
		SetState(AIRCRAFT_LANDING);
		bingoFuel=true;
		return true;
	}
	return false;*/
}

void CAircraft::UpdateLanded(void)
{
	float rdot=rightdir.dot(baseDir);
	if(rdot<-0.01){
		frontdir-=rightdir*0.01;
	} else if(rdot>0.01){
		frontdir+=rightdir*0.01;		
	} else if(frontdir.dot(baseDir)<0){
		frontdir+=rightdir*0.01;				
	}

	if(ground->GetApproximateHeight(pos.x,pos.z)<=0)
		DoDamage(DamageArray()*3,0);
	updir=float3(0,1,0);
	frontdir.Normalize();
	rightdir=frontdir.cross(updir);
	rightdir.Normalize();
	updir=rightdir.cross(frontdir);
}

void CAircraft::UpdateLanding(void)
{
	float3 initPoint=basePos+baseDir*400;
	float3 baseRightDir=baseDir.cross(float3(0,1,0));

	switch(subState){
	case 0:{
		mySide=-1;
		if((pos-basePos).dot(baseRightDir)>0)
			mySide=1;
		float3 turnCenter=initPoint+baseRightDir*mySide*turnRadius;
		if(turnCenter.distance2D(pos)<turnRadius*2.1){
			float3 dif=(pos-turnCenter).Normalize();
			goalPos=turnCenter+dif*turnRadius*2.1;
		} else {
			subState++;
		}
		UpdateFlying(wantedHeight,1);
		break;}
	case 1:{
		mySide=-1;
		if((pos-basePos).dot(baseRightDir)>0)
			mySide=1;
		float3 turnCenter=initPoint+baseRightDir*mySide*turnRadius;
		float3 ac2tc=turnCenter-pos;
		ac2tc.y=0;
		ac2tc.Normalize();
		goalPos=turnCenter+ac2tc.cross(float3(0,mySide,0))*turnRadius;
		UpdateFlying(min(wantedHeight,initPoint.distance2D(pos)*0.15+50),1);
		float3 fd=frontdir;
		fd.y=0;
		fd.Normalize();
		float3 goalDir=goalPos-pos;
		goalDir.y=0;
		goalDir.Normalize();
		if(goalDir.dot(fd)>0.999)
			subState++;
/*	DrawLine dl;
	dl.color=float3(0,1,0);
	dl.pos1=turnCenter;
	dl.pos1.y=ground->GetHeight(dl.pos1.x,dl.pos1.z)+wantedHeight;
	dl.pos2=initPoint;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);
	dl.color=float3(1,0,0);
	dl.pos1=turnCenter;
	dl.pos1.y=ground->GetHeight(dl.pos1.x,dl.pos1.z)+wantedHeight;
	dl.pos2=goalPos;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);*/
		break;}
	case 2:
		UpdateFlying(min(wantedHeight,initPoint.distance2D(pos)*0.15+50),1);
		if(goalPos.distance2D(pos)<60)
			subState++;{
/*	DrawLine dl;
	dl.color=float3(0,1,0);
	dl.pos1=pos;
	dl.pos1.y=ground->GetHeight(dl.pos1.x,dl.pos1.z)+wantedHeight;
	dl.pos2=initPoint;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);
	dl.color=float3(1,0,0);
	dl.pos1=basePos;
	dl.pos1.y=ground->GetHeight(dl.pos1.x,dl.pos1.z)+wantedHeight;
	dl.pos2=basePos+baseDir*100;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);*/
		break;}
	case 3:{
		float3 turnCenter=initPoint+baseRightDir*mySide*turnRadius;
		float3 ac2tc=turnCenter-pos;
		ac2tc.y=0;
		ac2tc.Normalize();
		
		float3 v2=ac2tc.cross(float3(0,mySide,0));

		float3 goalVec=-ac2tc+v2*2.5;
		goalVec.Normalize();

		goalPos=turnCenter+goalVec*turnRadius;
		UpdateFlying(min(wantedHeight,initPoint.distance2D(pos)*0.15+50),speed.Length()<1);
		if(initPoint.distance2D(pos)<38)
			subState++;
/*	DrawLine dl;
	dl.color=float3(0,1,0);
	dl.pos1=turnCenter;
	dl.pos1.y=ground->GetHeight(dl.pos1.x,dl.pos1.z)+wantedHeight;
	dl.pos2=pos;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);
	dl.color=float3(1,0,0);
	dl.pos1=turnCenter;
	dl.pos1.y=ground->GetHeight(dl.pos1.x,dl.pos1.z)+wantedHeight;
	dl.pos2=goalPos;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);
	dl.color=float3(1,1,0);
	dl.pos1=basePos;
	dl.pos1.y=ground->GetHeight(dl.pos1.x,dl.pos1.z)+wantedHeight;
	dl.pos2=basePos+baseDir*100;;
	dl.pos2.y=ground->GetHeight(dl.pos2.x,dl.pos2.z)+wantedHeight;
	lines.push_back(dl);*/
		break;}
	case 4:{
		goalPos=basePos;
		float engine=1;
		float goalDist=pos.distance2D(basePos);
		if(goalDist<speed.Length()*90+3)
			engine=-3;
		float3 bd=basePos-pos;
		bd.Normalize();
		float3 rspeed=speed.cross(float3(0,1,0));
		float rdot=rspeed.dot(bd);
		speed+=rightdir*rdot*0.01;
		UpdateFlying(max(20,min(50,-30*goalDist*0.2)),engine);
		if(speed.Length()<0.3){
			speed=float3(0,0,0);
			SetState(AIRCRAFT_LANDED);
		}
		break;}
	default:
		UpdateAirPhysics(0,1,0,1);
		break;
	}
}

void CAircraft::SetState(CAircraft::AircraftState state)
{
	if(aircraftState!=state && (!bingoFuel || state==AIRCRAFT_LANDED || state==AIRCRAFT_CRASHING)){
		if(state==AIRCRAFT_LANDED){
			useAirLos=false;
		}else{
			useAirLos=true;
		}
		aircraftState=state;
		subState=0;
	}
}

void CAircraft::UpdateAttack(void)
{
	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi){
		(*wi)->targetPos=goalPos;
		if(userTarget){
			(*wi)->AttackUnit(userTarget,true);
		}
	}

	UpdateFlying(wantedHeight,1);
}

void CAircraft::Init(void)
{
//	info->AddLine("%f",maxSpeed);
	cob->Call(COBFN_Activate);
	wantedHeight+=g.randFloat()*5*(isFighter?2:1);
	maxRudder*=0.99+g.randFloat()*0.02;
	maxElevator*=0.99+g.randFloat()*0.02;
	maxAileron*=0.99+g.randFloat()*0.02;
	maxAcc*=0.99+g.randFloat()*0.02;

	crashAileron=1-g.randFloat()*g.randFloat();
	if(g.randInt()&1)
		crashAileron=-crashAileron;
	crashElevator=g.randFloat();
	crashRudder=g.randFloat()-0.5;

	/*std::vector<CUnit3DLoader::Propeller>::iterator pi;
	for(pi=model->geometry->propellers.begin();pi!=model->geometry->propellers.end();++pi){
		myPropellers.push_back(new CPropeller(pos,pi->pos,pi->size,this));
	}*/

	/*for(std::vector<CUnit3DLoader::UnitModel*>::iterator umi=model->subModels.begin();umi!=model->subModels.end();++umi){
		if((*umi)->name=="sidoroder"){
			rudder.model=*umi;
			rudder.rotation=0;
			rudders.push_back(&rudder);
		} else if((*umi)->name=="hojdroder"){
			elevator.model=*umi;
			elevator.rotation=0;
			rudders.push_back(&elevator);
		} else if((*umi)->name=="skevroder_v"){
			aileronLeft.model=*umi;
			aileronLeft.rotation=0;
			rudders.push_back(&aileronLeft);
		} else if((*umi)->name=="skevroder_h"){
			aileronRight.model=*umi;
			aileronRight.rotation=0;
			rudders.push_back(&aileronRight);
		}
	}*/
	lastRudderUpdate=g.frameNum;
	lastElevatorPos=0;
	lastRudderPos=0;
	lastAileronPos=0;

	frontdir=baseDir;
	CUnit::Init();
}

void CAircraft::UpdateFighterAttack(void)
{
	float speedf=speed.Length();
	if(speedf<0.01){
		UpdateAirPhysics(0,0,0,1);
		return;
	}
	if((!userTarget || userTarget->armorType!=Armor_Aircraft) && frontdir.dot(goalPos-pos)<0 && (pos-goalPos).SqLength()<turnRadius*turnRadius){
		float3 dif=pos-goalPos;
		dif.y=0;
		dif.Normalize();
		goalPos=goalPos+dif*turnRadius*4;
	}
	float3 tgp=goalPos+(goalPos-oldGoalPos)*8;
	oldGoalPos=goalPos;
	goalPos=tgp;

	float goalLength=(goalPos-pos).Length();
	float3 goalDir=(goalPos-pos)/goalLength;
	
	float aileron=0;
	float rudder=0;
	float elevator=0;

	float gHeight=ground->GetHeight(pos.x,pos.z);

	float goalDot=rightdir.dot(goalDir);
	goalDot/=goalDir.dot(frontdir)*0.5+0.501;

	if(goalDir.dot(frontdir)<-0.2+inefficientAttackTime*0.002 && frontdir.y>-0.2 && speedf>2.0 && g.randFloat()>0.996)
		maneuver=1;

	if(goalDir.dot(frontdir)<-0.2+inefficientAttackTime*0.002 && fabs(frontdir.y)<0.2 && g.randFloat()>0.996 && gHeight+400<pos.y){
		maneuver=2;
		maneuverSubState=0;
	}

	//roll
	if(speedf>0.45 && pos.y+speed.y*60*fabs(frontdir.y)+min(0,updir.y)*150>gHeight+60+fabs(rightdir.y)*150){
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

	//pitch
	if(speedf<2){
		if(frontdir.y<0.0){
			elevator=1;
		} else if(frontdir.y>0.0){
			elevator=-1;
		}
	} else if(pos.y+min(0,speed.y)*70*fabs(frontdir.y)+min(0,updir.y)*50<gHeight+50){
		float upside=1;
		if(updir.y<-0.3)
			upside=-1;
		if(frontdir.y<0.3){
			elevator=upside;
		} else if(frontdir.y>0.35){
			elevator=-upside;
		}
	} else {
		CUnit* u=0;
		float l=helper->TraceRay(pos,speed/speedf,speedf*30,0,this,u);
		if(u && frontdir.dot(u->pos+u->speed*10-pos-speed*10)<0){
			float pitchMod=updir.y>0?1:-1;
			if(u->pos.y>pos.y)
				elevator=-pitchMod;
			else
				elevator=pitchMod;
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
	}
#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		info->AddLine("FAttack %.1f %.1f %.2f",pos.y-gHeight,goalLength,goalDir.dot(frontdir));
	}
#endif

	
	UpdateAirPhysics(rudder,aileron,elevator,min(1,goalLength/maxRange+1-goalDir.dot(frontdir)*0.7));

	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi){
		(*wi)->targetPos=goalPos;
		if(userTarget){
			(*wi)->AttackUnit(userTarget,true);
		}
	}
/*	DrawLine dl;
	dl.color=float3(0,1,0);
	dl.pos1=pos;
	dl.pos2=goalPos;
	lines.push_back(dl);
	dl.color=float3(1,0,0);
	dl.pos1=pos;
	dl.pos2=pos+frontdir*maxRange;
	lines.push_back(dl);/**/
}


void CAircraft::UpdateManeuver(void)
{
#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		info->AddLine("UpdataMan %i %i",maneuver,maneuverSubState);
	}
#endif
	float speedf=speed.Length();
	switch(maneuver){
	case 1:{
		int aileron=0,elevator=0;
		if(updir.y>0){
			if(rightdir.y>maxAileron*speedf){
				aileron=1;
			}else if(rightdir.y<-maxAileron*speedf){
				aileron=-1;
			}
		}
		if(fabs(rightdir.y)<maxAileron*3*speedf || updir.y<0)
			elevator=1;
		UpdateAirPhysics(0,aileron,elevator,1);
		if((updir.y<0 && frontdir.y<0) || speedf<0.8)
			maneuver=0;
		break;}
	case 2:{
		int aileron=0,elevator=0;
		if(maneuverSubState==0){
			if(rightdir.y>=0){
				aileron=-1;
			}else{
				aileron=1;
			}
		}
		if(frontdir.y<-0.7)
			maneuverSubState=1;
		if(maneuverSubState==1 || updir.y<0)
			elevator=1;
		UpdateAirPhysics(0,aileron,elevator,1);

		if((updir.y>0 && frontdir.y>0 && maneuverSubState==1) || speedf<0.2)
			maneuver=0;
		break;}
	default:
		UpdateAirPhysics(0,0,0,1);
		maneuver=0;
		break;
	}
}

void CAircraft::KillUnit(bool selfDestruct)
{
	if(selfDestruct || beingBuilt || g.randFloat()<0.5){
		CUnit::KillUnit(selfDestruct);
	} else {
		health=200;
		isDead=false;
		crashing=true;
		SetState(AIRCRAFT_CRASHING);
	}
}
