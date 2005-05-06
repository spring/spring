// Unit.cpp: implementation of the CUnit class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include "StdAfx.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "myGL.h"
#include "CommandAI.h"
#include "Weapon.h"
#include "QuadField.h"
#include "Team.h"
#include "Ground.h"
#include "ReadMap.h"
#include "InfoConsole.h"
#include "GameHelper.h"
#include "SyncTracer.h"
#include "LosHandler.h"
//#include "Unit3DLoader.h"
#include "Camera.h"
#include "glFont.h"
#include "Group.h"
#include "myMath.h"
#include "3DOParser.h"
#include "CobInstance.h"
#include "Unit.h"
#include "CobFile.h"
#include "FeatureHandler.h"
#include "math.h"
#include "UnitDef.h"
#include "WeaponDefHandler.h"
#include "MoveType.h"
#include "Matrix44f.h"
#include "MetalMap.h"
#include "Wind.h"
#include "Sound.h"
#include "RadarHandler.h"
#include "AirMoveType.h"
#include "Aircraft.h"
#include "Building.h"
#include "ShadowHandler.h"
#include "Player.h"
//#include "mmgr.h"
#include "LoadSaveInterface.h"
#include "BaseWater.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const float MINIMUM_SPEED = 0.01;

CUnit::CUnit(const float3 &pos,int team,UnitDef* unitDef)
:	CSolidObject(pos),
	unitDef(unitDef),
	maxHealth(100),
	health(100),
	power(100),
	experience(0),
	logExperience(0),
	limExperience(0),
	armorType(0),
	beingBuilt(true),
	team(team),
	allyteam(gs->team2allyteam[team]),
	restTime(0),
	controlRadius(2),
	reloadSpeed(1),
	commandAI(0),
	tempNum(0),
	losRadius(0),
	airLosRadius(0),
	losHeight(0),
	metalCost(100),
	energyCost(0),
	buildTime(100),
	frontdir(0,0,1),
	rightdir(-1,0,0),
	updir(0,1,0),
	upright(true),
	maxRange(0),
	haveTarget(false),
	lastAttacker(0),
	lastAttack(-200),
	userTarget(0),
	userAttackGround(false),
	lastLosUpdate(0),
	fireState(2),
	moveState(0),
	lastSlowUpdate(0),
	los(0),
	userAttackPos(0,0,0),
	crashing(false),
	cob(0),
	bonusShieldDir(1,0,0),
	bonusShieldSaved(10),
	group(0),
	lastDamage(-100),
	lastFireWeapon(0),
	lastMuzzleFlameSize(0),
	lastMuzzleFlameDir(0,1,0),
	category(0),
	recentDamage(0),
	armoredState(false),
	armoredMultiple(1),
	curArmorMultiple(1),
	buildProgress(0),
	realLosRadius(0),
	realAirLosRadius(0),
	inBuildStance(false),
	isDead(false),
	nextPosErrorUpdate(1),
	posErrorDelta(0,0,0),
	inTransport(false),
	toBeTransported(false),
	hasUWWeapons(false),
	lastNanoAdd(gs->frameNum),
	cloakTimeout(128),
	curCloakTimeout(gs->frameNum),
	isCloaked(false),
	wantCloak(false),
	stockpileWeapon(0),
	haveDGunRequest(false),
	jammerRadius(0),
	radarRadius(0),
	sonarRadius(0),
	stunned(false),
	metalUse(0),
	energyUse(0),
	metalMake(0),
	energyMake(0),
	metalUseI(0),
	energyUseI(0),
	metalMakeI(0),
	energyMakeI(0),
	metalUseold(0),
	energyUseold(0),
	metalMakeold(0),
	energyMakeold(0),
	energyTickMake(0),
	metalExtract(0),
	lastTerrainType(-1),
	curTerrainType(0),
	relMidPos(0,0,0),
	selfDCountdown(0),
	useHighTrajectory(false)
{
#ifdef DIRECT_CONTROL_ALLOWED
	directControl=0;
#endif

	activated = false;

	localmodel=NULL;
	SetRadius(1.2f);
	id=uh->AddUnit(this);
	qf->MovedUnit(this);
	mapSquare=ground->GetSquare(pos);
	oldRadarPos.x=-1;

	posErrorVector=gs->randVector();
	posErrorVector.y*=0.2;
#ifdef TRACE_SYNC
	tracefile << "New unit: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif
}

CUnit::~CUnit()
{
#ifdef TRACE_SYNC
	tracefile << "Unit died: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif

#ifdef DIRECT_CONTROL_ALLOWED
	if(directControl){
		ENTER_MIXED;
		if(gu->directControl==this)
			gu->directControl=0;
		ENTER_SYNCED;
		directControl->myController->playerControlledUnit=0;
		directControl=0;
	}
#endif

	if(activated && unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]*=radarhandler->targFacEffect;
	}

	if(!beingBuilt){
		gs->teams[team]->metalStorage-=unitDef->metalStorage;
		gs->teams[team]->energyStorage-=unitDef->energyStorage;
	}

	delete commandAI;
	delete moveType;

	if(group)
		group->RemoveUnit(this);
	group=0;

	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi)
		delete *wi;

	qf->RemoveUnit(this);
	loshandler->DelayedFreeInstance(los);
	los=0;

	if(radarRadius || jammerRadius || sonarRadius)
		radarhandler->RemoveUnit(this);

	if(!isDead && dynamic_cast<CBuilding*>(this)){
		for(int z=mapPos.y; z<mapPos.y+ysize; z++){
			for(int x=mapPos.x; x<mapPos.x+xsize; x++){
				readmap->typemap[z*gs->mapx+x]&=127;
			}
		}
	}

	delete cob;
	delete localmodel;
}

void CUnit::Update()
{
	posErrorVector+=posErrorDelta;
	if(beingBuilt || stunned)
		return;
	restTime++;
	recentDamage*=0.9;
	bonusShieldSaved+=0.005;

	moveType->Update();

	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi)
		(*wi)->Update();
}

void CUnit::SlowUpdate()
{
	--nextPosErrorUpdate;
	if(nextPosErrorUpdate==0){
		float3 newPosError(gs->randVector());
		newPosError.y*=0.2;
		if(posErrorVector.dot(newPosError)<0)
			newPosError=-newPosError;
		posErrorDelta=(newPosError-posErrorVector)*(1.0/256);
		nextPosErrorUpdate=16;
	}

	if(beingBuilt){
		if(lastNanoAdd<gs->frameNum-200){
			health-=maxHealth/(buildTime*0.03);
			buildProgress-=1/(buildTime*0.03);
			AddMetal(metalCost/(buildTime*0.03));
			if(health<0)
				KillUnit(false,true);
		}
		return;
	}
	if(selfDCountdown){
		selfDCountdown--;
		if(selfDCountdown<=1){
			KillUnit(true,false);
			selfDCountdown=0;
			return;
		}
		ENTER_MIXED;
		if(selfDCountdown&1 && team==gu->myTeam)
			info->AddLine("%s: Self destruct in %i s",unitDef->humanName.c_str(),selfDCountdown/2);
		ENTER_SYNCED;
	}
	if(stunned)
		return;

	lastSlowUpdate=gs->frameNum;

	commandAI->SlowUpdate();
	moveType->SlowUpdate();

	metalMake = metalMakeI + metalMakeold;
	metalUse = metalUseI+ metalUseold;
	energyMake = energyMakeI + energyMakeold;
	energyUse = energyUseI + energyUseold;
	metalMakeold = metalMakeI;
	metalUseold = metalUseI;
	energyMakeold = energyMakeI;
	energyUseold = energyUseI;

	metalMakeI=metalUseI=energyMakeI=energyUseI=0;

	AddMetal(unitDef->metalMake*0.5f);
	if(activated)
	{
		if(UseEnergy(unitDef->energyUpkeep*0.5f))
		{
			AddMetal(unitDef->makesMetal*0.5f);
			if(unitDef->extractsMetal>0)
				AddMetal(metalExtract * 0.5f);
		}
		UseMetal(unitDef->metalUpkeep*0.5f);

		if(unitDef->windGenerator>0)
		{
			if(wind.curStrength > unitDef->windGenerator)
			{
 				AddEnergy(unitDef->windGenerator*0.5f);
			}
			else
			{
 				AddEnergy(wind.curStrength*0.5f);
			}
		}
	}
	AddEnergy(energyTickMake*0.5f);

	if(health<maxHealth && restTime>600){
		health+=10;
	}

	bonusShieldSaved+=0.05f;
	residualImpulse*=0.6;

	if(wantCloak){
		if(helper->GetClosestEnemyUnit(pos,unitDef->decloakDistance,allyteam))
			curCloakTimeout=gs->frameNum+cloakTimeout;
		if(isCloaked || gs->frameNum>=curCloakTimeout){
			float cloakCost=unitDef->cloakCost;
			if(speed.SqLength()>0.2)
				cloakCost=unitDef->cloakCostMoving;
			if(UseEnergy(cloakCost)){
				isCloaked=true;
			} else {
				isCloaked=false;
			}
		} else {
			isCloaked=false;
		}
	} else {
		isCloaked=false;
	}

	if(unitDef->canKamikaze && fireState==2){
		CUnit* u=helper->GetClosestEnemyUnit(pos,unitDef->kamikazeDist,allyteam);
		if(u && u->speed.dot(pos - u->pos)<0)		//self destruct when unit start moving away from mine, should maximize damage
			KillUnit(true,false);
	}

	if(!weapons.empty()){
		haveTarget=false;
		haveUserTarget=false;

		//aircraft does not want this
		if (moveType->useHeading) {
			frontdir=GetVectorFromHeading(heading);
			if(upright || !unitDef->canmove){
				updir=UpVector;
				rightdir=frontdir.cross(updir);
			} else {
				updir=ground->GetNormal(pos.x,pos.z);
				rightdir=frontdir.cross(updir);
				rightdir.Normalize();
				frontdir=updir.cross(rightdir);
			}
		}

		for(vector<CWeapon*>::iterator wi=weapons.begin();wi!=weapons.end();++wi){
			CWeapon* w=*wi;
			if(userTarget && !w->haveUserTarget && (haveDGunRequest || !unitDef->canDGun || !w->weaponDef->manualfire))
				w->AttackUnit(userTarget,true);
			else if(userAttackGround && !w->haveUserTarget && (haveDGunRequest || !unitDef->canDGun || !w->weaponDef->manualfire))
				w->AttackGround(userAttackPos,true);
			
			w->SlowUpdate();

			if(w->targetType==Target_None && fireState>0 && lastAttacker && lastAttack+200>gs->frameNum)
				w->AttackUnit(lastAttacker,false);
		}
	}

	CalculateTerrainType();
	UpdateTerrainType();
}

void CUnit::DoDamage(const DamageArray& damages, CUnit *attacker,const float3& impulse)
{
	if(isDead)
		return;

	residualImpulse+=impulse/mass;
	moveType->ImpulseAdded();

	float damage=damages[armorType];

	if(attacker){
		float3 adir=attacker->pos-pos;
		adir.Normalize();
		bonusShieldDir+=adir*bonusShieldSaved;		//not the best way to do it(but fast)
		bonusShieldDir.Normalize();
//		info->AddLine("shield mult %f %f %i %i", 2-adir.dot(bonusShieldDir),bonusShieldSaved,id,attacker->id);
		bonusShieldSaved=0;
		damage*=1.5-adir.dot(bonusShieldDir)*0.5;
	}
	float3 hitDir = impulse;
	hitDir.y = 0;
	hitDir = -hitDir.Normalize();
	std::vector<long> hitAngles;
	hitAngles.push_back(500 * hitDir.z);
	hitAngles.push_back(500 * hitDir.x);
	cob->Call(COBFN_HitByWeapon, hitAngles);	

	damage*=curArmorMultiple;

	restTime=0;
	health-=damage;
	recentDamage+=damage;
	if(health<=0){
		KillUnit(false,false);
		if(isDead && attacker!=0 && !gs->allies[allyteam][attacker->allyteam] && !beingBuilt){
			attacker->experience+=0.1*power/attacker->power;
			gs->teams[attacker->team]->currentStats.unitsKilled++;
		}
	}
	if(attacker!=0 && !gs->allies[allyteam][attacker->allyteam]){
//		info->AddLine("%s has %f/%f h attacker %s do %f d",tooltip.c_str(),health,maxHealth,attacker->tooltip.c_str(),damage);
		SetLastAttacker(attacker);
		attacker->experience+=0.1*power/attacker->power*damage/maxHealth;
		attacker->ExperienceChange();
		ENTER_UNSYNCED;
		if(uh->lastDamageWarning+100<gs->frameNum && team==gu->myTeam && !camera->InView(midPos,radius+50)){
			info->AddLine("%s is being attacked",unitDef->humanName.c_str());
			info->SetLastMsgPos(pos);
			uh->lastDamageWarning=gs->frameNum;
		}
		ENTER_SYNCED;
	}
//	if(attacker!=0 && attacker->team==team)
//		info->AddLine("FF by %i %s on %i %s",attacker->id,attacker->tooltip.c_str(),id,tooltip.c_str());

#ifdef TRACE_SYNC
	tracefile << "Damage: ";
	tracefile << id << " " << damage << "\n";
#endif
}

void CUnit::Kill(float3& impulse) {
	DamageArray da;
	DoDamage(da*(health/da[armorType]), 0, impulse);
}


void CUnit::Draw()
{
	glPushMatrix();
	float3 interPos=pos+speed*gu->timeOffset;

	if (physicalState == Flying && unitDef->canmove) {
		//aircraft or skidding ground unit
		CMatrix44f transMatrix(interPos,-rightdir,updir,frontdir);

		glMultMatrixf(&transMatrix[0]);		
	} else if(upright || !unitDef->canmove){
		glTranslatef3(interPos);
		if(heading!=0)
			glRotatef(heading*(180.0/32768.0),0,1,0);
	} else {
		float3 frontDir=GetVectorFromHeading(heading);		//making local copies of vectors
		float3 upDir=ground->GetSmoothNormal(pos.x,pos.z);
		float3 rightDir=frontDir.cross(upDir);
		rightDir.Normalize();
		frontDir=upDir.cross(rightDir);

		CMatrix44f transMatrix(interPos,-rightDir,upDir,frontDir);

		glMultMatrixf(&transMatrix[0]);
	}

	if(beingBuilt){
		if(shadowHandler->inShadowPass){
			if(buildProgress>0.66)
				localmodel->Draw();

		} else {
			float height=model->height;
			float start=model->rootobject->miny;
			glEnable(GL_CLIP_PLANE0);
			glEnable(GL_CLIP_PLANE1);

			float col=fabs(128.0-((gs->frameNum*4)&255))/255.0+0.5f;
			glColor3f(col*0.5,col,col*0.5);

			if(shadowHandler->drawShadows && !water->drawReflection){
				glDisable(GL_VERTEX_PROGRAM_ARB);
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE0_ARB);
				glDisable(GL_FOG);
			} else {
				glDisable(GL_LIGHTING);
				glDisable(GL_TEXTURE_2D);
			}
			double plane[4]={0,-1,0,start+height*buildProgress*3};
			glClipPlane(GL_CLIP_PLANE0 ,plane);
			double plane2[4]={0,1,0,-start-height*(buildProgress*10-9)};
			glClipPlane(GL_CLIP_PLANE1 ,plane2);
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
			localmodel->Draw();
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

			if(buildProgress>0.33){
				col=1.5-col;
				glColor3f(col*0.5,col,col*0.5);
				double plane[4]={0,-1,0,start+height*(buildProgress*3-1)};
				glClipPlane(GL_CLIP_PLANE0 ,plane);
				double plane2[4]={0,1,0,-start-height*(buildProgress*3-2)};
				glClipPlane(GL_CLIP_PLANE1 ,plane2);

				localmodel->Draw();
			}
			glDisable(GL_CLIP_PLANE1);
			if(shadowHandler->drawShadows && !water->drawReflection){
				glEnable(GL_VERTEX_PROGRAM_ARB);
				glEnable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glEnable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE0_ARB);
				glEnable(GL_FOG);
			} else {
				glEnable(GL_LIGHTING);
				glColor3f(1,1,1);
				glEnable(GL_TEXTURE_2D);
			}
			if(buildProgress>0.66){
				double plane[4]={0,-1,0,start+height*(buildProgress*3-2)};
				glClipPlane(GL_CLIP_PLANE0 ,plane);
				if(shadowHandler->drawShadows && !water->drawReflection){
					glPolygonOffset(1,1);
					glEnable(GL_POLYGON_OFFSET_FILL);
				}
				localmodel->Draw();
				if(shadowHandler->drawShadows && !water->drawReflection){
					glDisable(GL_POLYGON_OFFSET_FILL);
				}
			}
			glDisable(GL_CLIP_PLANE0);
		}
	} else {
		localmodel->Draw();
	}
	if(gu->drawdebug){
		glPushMatrix();
		glTranslatef3(frontdir*relMidPos.z + updir*relMidPos.y + rightdir*relMidPos.x);
		GLUquadricObj* q=gluNewQuadric();
		gluQuadricDrawStyle(q,GLU_LINE);
		gluSphere(q,radius,10,10);
		gluDeleteQuadric(q);
		glPopMatrix();
	}
	glPopMatrix();
}

void CUnit::DrawStats()
{
	float3 interPos=pos+speed*gu->timeOffset;
	interPos.y+=model->height+5;

	glBegin(GL_QUADS);

	float hpp = health/maxHealth;
	float end=(0.5-(hpp))*10;

	//black background for healthbar
	glColor3f(0.0,0.0,0.0);
	glVertexf3(interPos+(camera->up*6-camera->right*end));
	glVertexf3(interPos+(camera->up*6+camera->right*5));
	glVertexf3(interPos+(camera->up*4+camera->right*5));
	glVertexf3(interPos+(camera->up*4-camera->right*end));

	if(hpp>0.5f)
		glColor3f(1.0f-((hpp-0.5f)*2.0f),1.0f,0.0);
	else
		glColor3f(1.0f,hpp*2.0f,0.0);

	//healthbar
	glVertexf3(interPos+(camera->up*6-camera->right*5));
	glVertexf3(interPos+(camera->up*6-camera->right*end));
	glVertexf3(interPos+(camera->up*4-camera->right*end));
	glVertexf3(interPos+(camera->up*4-camera->right*5));



	if(gu->myTeam!=team && !gu->spectating){
		glEnd();
		return;
	}
	glColor3f(1,1,1);
	end=(limExperience*0.8f)*10;
	glVertexf3(interPos+(-camera->up*2+camera->right*6));
	glVertexf3(interPos+(-camera->up*2+camera->right*8));
	glVertexf3(interPos+(camera->up*(end-2)+camera->right*8));
	glVertexf3(interPos+(camera->up*(end-2)+camera->right*6));
	glEnd();

	if(group){
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glTranslatef3(interPos-camera->right*10);
		glScalef(10,10,10);
		font->glWorldPrint("%d",group->id);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glPopMatrix();
	}
	if(beingBuilt){
		glColor3f(1,0,0);
		float end=(buildProgress*0.8f)*10;
		glBegin(GL_QUADS);
		glVertexf3(interPos-camera->up*2-camera->right*6);
		glVertexf3(interPos-camera->up*2-camera->right*8);
		glVertexf3(interPos+camera->up*(end-2)-camera->right*8);
		glVertexf3(interPos+camera->up*(end-2)-camera->right*6);
		glEnd();
	}
	if(stockpileWeapon){
		glColor3f(1,0,0);
		float end=(stockpileWeapon->buildPercent*0.8f)*10;
		glBegin(GL_QUADS);
		glVertexf3(interPos-camera->up*2-camera->right*6);
		glVertexf3(interPos-camera->up*2-camera->right*8);
		glVertexf3(interPos+camera->up*(end-2)-camera->right*8);
		glVertexf3(interPos+camera->up*(end-2)-camera->right*6);
		glEnd();
	}
}

void CUnit::ExperienceChange()
{
	logExperience=log(experience+1);
	limExperience=1-(1/(experience+1));

	power=unitDef->power*(1+logExperience);
	reloadSpeed=(1+limExperience*0.7);	

	float oldMaxHealth=maxHealth;
	maxHealth=unitDef->health*(1+logExperience*0.7);
	health+=(maxHealth-oldMaxHealth)*(health/oldMaxHealth);
}

void CUnit::ChangeLos(int l,int airlos)
{
	loshandler->FreeInstance(los);
	los=0;
	losRadius=l;
	airLosRadius=airlos;
	loshandler->MoveUnit(this,false);
}

void CUnit::ChangeTeam(int newteam,ChangeType type)
{
	qf->RemoveUnit(this);
	quads.clear();
	loshandler->FreeInstance(los);
	los=0;
	if(radarRadius || jammerRadius || sonarRadius)
		radarhandler->RemoveUnit(this);

	if(type==ChangeGiven){
		gs->teams[team]->RemoveUnit(this,CTeam::RemoveGiven);
		gs->teams[newteam]->AddUnit(this,CTeam::AddGiven);
	} else {
		gs->teams[team]->RemoveUnit(this,CTeam::RemoveCaptured);
		gs->teams[newteam]->AddUnit(this,CTeam::AddCaptured);
	}

	if(!beingBuilt){
		gs->teams[team]->metalStorage-=unitDef->metalStorage;
		gs->teams[team]->energyStorage-=unitDef->energyStorage;

		gs->teams[newteam]->metalStorage+=unitDef->metalStorage;
		gs->teams[newteam]->energyStorage+=unitDef->energyStorage;
	}

	commandAI->commandQue.clear();

	team=newteam;
	allyteam=gs->team2allyteam[team];

	loshandler->MoveUnit(this,false);
	qf->MovedUnit(this);
	if(radarRadius || jammerRadius || sonarRadius)
		radarhandler->MoveUnit(this);

	//model=unitModelLoader->GetModel(model->name,newteam);
	model = unit3doparser->Load3DO(model->name.c_str(),1,newteam);
	delete localmodel;
	localmodel = unit3doparser->CreateLocalModel(model, &cob->pieces);

	if(type==ChangeGiven && unitDef->energyUpkeep>25){//deactivate to prevent the old give metal maker trick
		Command c;
		c.id=CMD_ONOFF;
		c.params.push_back(0);
		commandAI->GiveCommand(c);
	}
}

bool CUnit::AttackUnit(CUnit *unit,bool dgun)
{
	bool r=false;
	haveDGunRequest=dgun;
	userAttackGround=false;
	SetUserTarget(unit);
	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi)
		if(dgun || !unitDef->canDGun || !(*wi)->weaponDef->manualfire)
			if((*wi)->AttackUnit(unit,true))
				r=true;
	return r;
}

bool CUnit::AttackGround(const float3 &pos, bool dgun)
{
	bool r=false;
	haveDGunRequest=dgun;
	SetUserTarget(0);
	userAttackPos=pos;
	userAttackGround=true;
	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi)
		if(dgun || !unitDef->canDGun || !(*wi)->weaponDef->manualfire)
			if((*wi)->AttackGround(pos,true))
				r=true;
	return r;
}

void CUnit::SetLastAttacker(CUnit* attacker)
{
	lastAttack=gs->frameNum;
	if(lastAttacker && lastAttacker!=userTarget)
		DeleteDeathDependence(lastAttacker);

	lastAttacker=attacker;
	if(attacker)
		AddDeathDependence(attacker);
}

void CUnit::DependentDied(CObject* o)
{
	if(o==lastAttacker){
//		info->AddLine("Last attacker died %i %i",lastAttacker->id,id);
		lastAttacker=0;
	}
	if(o==userTarget)
		userTarget=0;
}

void CUnit::SetUserTarget(CUnit* target)
{
	if(userTarget && lastAttacker!=userTarget)
		DeleteDeathDependence(userTarget);

	userTarget=target;
	if(target){
		AddDeathDependence(target);
	}
}


void CUnit::Init(void)
{
	relMidPos=model->rootobject->relMidPos;
	midPos=pos+frontdir*relMidPos.z + updir*relMidPos.y + rightdir*relMidPos.x;
	losHeight=relMidPos.y+radius*0.5;
	height = model->height;		//TODO: This one would be much better to have either in Constructor or UnitLoader!//why this is always called in unitloader

	//All ships starts on water, all other on ground.
	//TODO: Improve this. There might be cases when this is not correct.
	if(unitDef->movedata && unitDef->movedata->moveType == MoveData::Ship_Move) {
		physicalState = Floating;
	} else {
		physicalState = OnGround;
	}
	
	//All units are set as ground-blocking.
	blocking = true;

	if(power<5){
		info->AddLine("Unit %s is really cheap? %f",unitDef->humanName.c_str(),power);
		power=10;
	}
//	if(pos.y+model->height<0)	//NOTE: Should not be needed.
//		isUnderWater=true;

	Block();

	if(dynamic_cast<CBuilding*>(this)){
		for(int z=mapPos.y; z<mapPos.y+ysize; z++){
			for(int x=mapPos.x; x<mapPos.x+xsize; x++){
				readmap->typemap[z*gs->mapx+x]|=128;
			}
		}
	}

	UpdateTerrainType();
}

void CUnit::UpdateTerrainType()
{
	if (curTerrainType != lastTerrainType) {
		cob->Call(COBFN_SetSFXOccupy, curTerrainType);
		lastTerrainType = curTerrainType;
	}
}

void CUnit::CalculateTerrainType()
{
	//Optimization: there's only about one unit that actually needs this information
	if (!cob->HasScriptFunction(COBFN_SetSFXOccupy)) 
		return;

	if (inTransport) {
		curTerrainType = 0;
		return;
	}

	float height = ground->GetApproximateHeight(pos.x, pos.z);

	//Deep sea?
	if (height < -5) {
		if (upright)
			curTerrainType = 2;
		else 
			curTerrainType = 1;
	}
	//Shore
	else if (height < 0) {
		if (upright)
			curTerrainType = 1;
	}
	//Land
	else {
		curTerrainType = 4;
	}
}

void CUnit::SetGroup(CGroup* newGroup)
{
	if(group!=0){
		group->RemoveUnit(this);
	}
	group=newGroup;

	if(group){
		if(!group->AddUnit(this))
			group=0;									//group ai didnt accept us
	}
}

bool CUnit::AddBuildPower(float amount,CUnit* builder)
{
	if(amount>0){	//build/repair
		if(!beingBuilt && health>=maxHealth)
			return false;
		
		lastNanoAdd=gs->frameNum;
		float part=amount/buildTime;

		if(beingBuilt){
			float metalUse=metalCost*part;
			float energyUse=energyCost*part;
			if(gs->teams[builder->team]->metal>metalUse && gs->teams[builder->team]->energy>energyUse){
				//gs->teams[builder->team]->UseMetal(metalUse);
				//gs->teams[builder->team]->UseEnergy(energyUse);
				builder->UseMetal(metalUse);
				builder->UseEnergy(energyUse);
				health+=maxHealth*part;
				buildProgress+=part;
				if(buildProgress>=1){
					if(health>maxHealth)
						health=maxHealth;
					FinishedBuilding();
				}
				return true;
			}
			return false;
		}	else {
			if(health<maxHealth){
				health+=maxHealth*part;
				return true;
			}
			return false;
		}
	} else {	//reclaim
		if(health<0)
			return false;
		restTime=0;
		float part=amount/buildTime;
		health+=maxHealth*part;
		if(beingBuilt){
			builder->AddMetal(metalCost*-part);
			buildProgress+=part;
			if(buildProgress<0 || health<0){
				KillUnit(false,true);
				return false;
			}
		} else {
			if(health<0){
				builder->AddMetal(metalCost);
				KillUnit(false,true);
				return false;
			}
		}
		return true;
	}
	return false;
}

/*void CUnit::SetGoal(float3 pos)
{

}*/
void CUnit::FinishedBuilding(void)
{
	beingBuilt=false;
	buildProgress=1;

	ChangeLos(realLosRadius,realAirLosRadius);

	if(unitDef->startCloaked)
		wantCloak=true;

	if(unitDef->windGenerator>0)
	{
		if(wind.curStrength > unitDef->windGenerator)
		{
			cob->Call(COBFN_SetSpeed, (long)(unitDef->windGenerator));
		}
		else
		{
			cob->Call(COBFN_SetSpeed, (long)(wind.curStrength*3000.0f));
		}
		cob->Call(COBFN_SetDirection, (long)GetHeadingFromVector(wind.curDir.x, wind.curDir.z));
	}

	if(unitDef->activateWhenBuilt)
	{
		Activate();
	}

	gs->teams[team]->metalStorage+=unitDef->metalStorage;
	gs->teams[team]->energyStorage+=unitDef->energyStorage;

	//Sets the frontdir in sync with heading.
	frontdir = GetVectorFromHeading(heading) + float3(0,frontdir.y,0);

	if(unitDef->isFeature){
		UnBlock();
		featureHandler->CreateWreckage(pos,wreckName, heading, 0,allyteam,false);
		KillUnit(false,true);
	}
}

void CUnit::KillUnit(bool selfDestruct,bool reclaimed)
{
	if(isDead)
		return;
	if(dynamic_cast<CAirMoveType*>(moveType) && !beingBuilt){
		if(!selfDestruct && !reclaimed && gs->randFloat()>recentDamage*0.7/maxHealth+0.2){
			((CAirMoveType*)moveType)->SetState(CAirMoveType::AIRCRAFT_CRASHING);
			health=maxHealth*0.5;
			return;
		}
	}
	isDead=true;

	if(dynamic_cast<CBuilding*>(this)){
		for(int z=mapPos.y; z<mapPos.y+ysize; z++){
			for(int x=mapPos.x; x<mapPos.x+xsize; x++){
				readmap->typemap[z*gs->mapx+x]&=127;
			}
		}
	}

	if(!reclaimed && !beingBuilt){
		string exp;
		if(selfDestruct)
			exp=unitDef->selfDExplosion;
		else
			exp=unitDef->deathExplosion;

		if(!exp.empty()){
			WeaponDef* wd=weaponDefHandler->GetWeapon(exp);
			if(wd){
				helper->Explosion(pos,wd->damages,wd->areaOfEffect,this);

				// Play explosion sound
				CWeaponDefHandler::LoadSound(wd->soundhit);
				if (wd->soundhit.id) {
					sound->PlaySound(wd->soundhit.id, pos, wd->soundhit.volume);
				}
				
				//info->AddLine("Should play %s (%d)", wd->soundhit.name.c_str(), wd->soundhit.id); 
			}
		}
		if(selfDestruct)
			recentDamage+=maxHealth*2;

		vector<long> args;
		args.push_back(recentDamage/maxHealth*100);
		args.push_back(0);
		cob->Call(COBFN_Killed, args);

		UnBlock();
		featureHandler->CreateWreckage(pos,wreckName, heading, args[1],-1,true);
	}
	uh->DeleteUnit(this);
}

bool CUnit::UseMetal(float metal)
{
	if(metal<0){
		AddMetal(-metal);
		return true;
	}
/*#ifdef TRACE_SYNC
	if(metal!=0){
		tracefile << "Use metal unit: ";
		tracefile << id << " " << metal << " " << unitDef->humanName.c_str() << " " << (float)gs->teams[team]->metal << " ";
	}
#endif*/
	bool canUse=gs->teams[team]->UseMetal(metal);
	if(canUse)
		metalUseI += metal;
/*#ifdef TRACE_SYNC
	if(metal!=0){
		tracefile << (float)gs->teams[team]->metal << "\n";
	}
#endif*/
	return canUse;
}

void CUnit::AddMetal(float metal)
{
	if(metal<0){
		UseMetal(-metal);
		return;
	}
/*#ifdef TRACE_SYNC
	if(metal!=0){
		tracefile << "Add metal unit: ";
		tracefile << id << " " << metal << " " << unitDef->humanName.c_str() << " " << (float)gs->teams[team]->metal << " ";
	}
#endif
*/
	metalMakeI += metal;
	gs->teams[team]->AddMetal(metal);
/*#ifdef TRACE_SYNC
	if(metal!=0){
		tracefile << (float)gs->teams[team]->metal << "\n";
	}
#endif*/
}

bool CUnit::UseEnergy(float energy)
{
	if(energy<0){
		AddEnergy(-energy);
		return true;
	}
	bool canUse=gs->teams[team]->UseEnergy(energy);
	if(canUse)
		energyUseI += energy;
	return canUse;
}

void CUnit::AddEnergy(float energy)
{
	if(energy<0){
		UseEnergy(-energy);
		return;
	}
	energyMakeI += energy;
	gs->teams[team]->AddEnergy(energy);
}

void CUnit::Activate()
{
	//if(unitDef->tidalGenerator>0)
	//	cob->Call(COBFN_SetSpeed, (long)(readmap->tidalStrength*3000.0f*unitDef->tidalGenerator));

	if(activated)
		return;

	cob->Call(COBFN_Activate);
	activated = true;

	if(unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]/=radarhandler->targFacEffect;
	}
	if(radarRadius || jammerRadius || sonarRadius)
		radarhandler->MoveUnit(this);

	if(unitDef->sounds.activate.id)
		sound->PlaySound(unitDef->sounds.activate.id, pos, unitDef->sounds.activate.volume);
}

void CUnit::Deactivate()
{
	if(!activated)
		return;

	cob->Call(COBFN_Deactivate);
	activated = false;

	if(unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]*=radarhandler->targFacEffect;
	}
	if(radarRadius || jammerRadius || sonarRadius)
		radarhandler->RemoveUnit(this);

	if(unitDef->sounds.deactivate.id)
		sound->PlaySound(unitDef->sounds.deactivate.id, pos, unitDef->sounds.deactivate.volume);
}

void CUnit::PushWind(float x, float z, float strength)
{
	if(strength > unitDef->windGenerator)
	{
		cob->Call(COBFN_SetSpeed, (long)(unitDef->windGenerator));
	}
	else
	{
		cob->Call(COBFN_SetSpeed, (long)(strength*3000.0f));
	}

	cob->Call(COBFN_SetDirection, (long)GetHeadingFromVector(x, z));
}

void CUnit::LoadSave(CLoadSaveInterface* file, bool loading)
{
	file->lsShort(heading);
	file->lsFloat(buildProgress);
	file->lsFloat(health);
	file->lsFloat(experience);
	if(loading)
		ExperienceChange();
	file->lsInt(moveState);
	file->lsInt(fireState);
	file->lsBool(isCloaked);
	file->lsBool(wantCloak);
	commandAI->LoadSave(file,loading);
}