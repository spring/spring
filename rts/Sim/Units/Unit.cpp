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
#include "3DModelParser.h"
#include "CobInstance.h"
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
#include "Building.h"
#include "ShadowHandler.h"
#include "Player.h"
#include "LoadSaveInterface.h"
#include "BaseWater.h"
#include "GlobalAIHandler.h"
#include "Feature.h"
#include "MissileProjectile.h"
#include "FlareProjectile.h"
#include "HeatCloudProjectile.h"
#include "MiniMap.h"
#include "UnitDrawer.h"
#include "AirBaseHandler.h"
#include "TransportUnit.h"
//#include "mmgr.h"

#include "GameSetup.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

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
	allyteam(gs->AllyTeam(team)),
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
	transporter(0),
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
	sonarJamRadius(0),
	radarRadius(0),
	sonarRadius(0),
	hasRadarCapacity(false),
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
	useHighTrajectory(false),
	deathCountdown(0),
	delayedWreckLevel(-1),
	paralyzeDamage(0),
	captureProgress(0),
	myTrack(0),
	lastFlareDrop(0),
	dontFire(false)
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

	for(int a=0;a<MAX_TEAMS;++a){
		losStatus[a]=0;
	}
	losStatus[allyteam]=LOS_INTEAM | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;

	posErrorVector=gs->randVector();
	posErrorVector.y*=0.2;
#ifdef TRACE_SYNC
	tracefile << "New unit: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif
}

CUnit::~CUnit()
{
	if(delayedWreckLevel>=0){
		featureHandler->CreateWreckage(pos,wreckName, heading, delayedWreckLevel,-1,true,unitDef->name);
	}

	if(unitDef->isAirBase){
		airBaseHandler->DeregisterAirBase(this);
	}

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

	if(!gameSetup || gameSetup->ghostedBuildings) {
		if(!(losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR)) && (losStatus[gu->myAllyTeam] & (LOS_PREVLOS)) && !gu->spectating){
			CUnitDrawer::GhostBuilding gb;
			gb.pos=pos;
			gb.model=model;
			unitDrawer->ghostBuildings.push_back(gb);
		}
	}

	if(activated && unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]*=radarhandler->targFacEffect;
	}

	if(!beingBuilt){
		gs->Team(team)->metalStorage-=unitDef->metalStorage;
		gs->Team(team)->energyStorage-=unitDef->energyStorage;
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

	if(hasRadarCapacity)
		radarhandler->RemoveUnit(this);

	delete cob;
	delete localmodel;
}

void CUnit::Update()
{
	posErrorVector+=posErrorDelta;

	if(deathCountdown){
		--deathCountdown;
		if(!deathCountdown)
			uh->DeleteUnit(this);
		return;
	}

	if(beingBuilt)
		return;

	moveType->Update();

	recentDamage*=0.9;
	bonusShieldSaved+=0.005;

	if(stunned)
		return;

	restTime++;

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

	for(int a=0;a<gs->activeAllyTeams;++a){
		if(losStatus[a] & LOS_INTEAM){
		} else if(loshandler->InLos(this,a)){
			if(!(losStatus[a]&LOS_INLOS)){
				if(!(losStatus[a]&LOS_INRADAR)){
					globalAI->UnitEnteredRadar(this,a);
				}
				globalAI->UnitEnteredLos(this,a);

				if(mobility || beingBuilt){
					losStatus[a]|=(LOS_INLOS | LOS_INRADAR);
				} else {
					losStatus[a]|=(LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR);	
				}
			}
		} else if(radarhandler->InRadar(this,a)){
			if((losStatus[a] & LOS_INLOS)){
				globalAI->UnitLeftLos(this,a);
				losStatus[a]&= ~LOS_INLOS;
			} else if(!(losStatus[a] & LOS_INRADAR)){
				globalAI->UnitEnteredRadar(this,a);
				losStatus[a]|= LOS_INRADAR;
			}
		} else {
			if((losStatus[a]&LOS_INRADAR)){
				if((losStatus[a]&LOS_INLOS)){
					globalAI->UnitLeftLos(this,a);
					globalAI->UnitLeftRadar(this,a);
				}else{
					globalAI->UnitLeftRadar(this,a);
				}
				losStatus[a]&= ~(LOS_INLOS | LOS_INRADAR | LOS_CONTRADAR);
			}
		}
	}
	if(paralyzeDamage>0){
		paralyzeDamage-=maxHealth*0.005;
		if(paralyzeDamage<0)
			paralyzeDamage=0;
		if(paralyzeDamage<health)
			stunned=false;
	}
	if(stunned){
		isCloaked=false;
		return;
	}

	if(selfDCountdown && !stunned){
		selfDCountdown--;
		if(selfDCountdown<=1){
			if(!beingBuilt)
				KillUnit(true,false,0);
			else
				KillUnit(false,true,0);	//avoid unfinished buildings making an explosion
			selfDCountdown=0;
			return;
		}
		ENTER_MIXED;
		if(selfDCountdown&1 && team==gu->myTeam)
			info->AddLine("%s: Self destruct in %i s",unitDef->humanName.c_str(),selfDCountdown/2);
		ENTER_SYNCED;
	}

	if(beingBuilt){
		if(lastNanoAdd<gs->frameNum-200){
			health-=maxHealth/(buildTime*0.03);
			buildProgress-=1/(buildTime*0.03);
			AddMetal(metalCost/(buildTime*0.03));
			if(health<0)
				KillUnit(false,true,0);
		}
		return;
	}
	//below is stuff that shouldnt be run while being built

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
			if(unitDef->isMetalMaker){
				AddMetal(unitDef->makesMetal*0.5f*uh->metalMakerEfficiency);
				uh->metalMakerIncome+=unitDef->makesMetal;
			} else {
				AddMetal(unitDef->makesMetal*0.5f);
			}
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

	if(health<maxHealth) 
	{
		health += unitDef->autoHeal;

		if(restTime > unitDef->idleTime)
		{
			health += unitDef->idleAutoHeal;
		}
		if(health>maxHealth)
			health=maxHealth;
	}

	bonusShieldSaved+=0.05f;
	residualImpulse*=0.6;

	if(wantCloak){
		if(helper->GetClosestEnemyUnit(pos,unitDef->decloakDistance,allyteam)){
			curCloakTimeout=gs->frameNum+cloakTimeout;
			isCloaked=false;
		}
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

	if(uh->waterDamage && (physicalState==CSolidObject::Floating || (physicalState==CSolidObject::OnGround && pos.y<=-3 && readmap->halfHeightmap[int((pos.z/(SQUARE_SIZE*2))*gs->hmapx+(pos.x/(SQUARE_SIZE*2)))]<-1))){
		DoDamage(DamageArray()*uh->waterDamage,0,ZeroVector);
	}

	if(unitDef->canKamikaze){
		if(fireState==2){
			CUnit* u=helper->GetClosestEnemyUnitNoLosTest(pos,unitDef->kamikazeDist,allyteam);
			if(u && u->physicalState!=CSolidObject::Flying && u->speed.dot(pos - u->pos)<=0)		//self destruct when unit start moving away from mine, should maximize damage
				KillUnit(true,false,0);
		}
		if(userTarget && userTarget->pos.distance(pos)<unitDef->kamikazeDist)
			KillUnit(true,false,0);
		if(userAttackGround && userAttackPos.distance(pos)<unitDef->kamikazeDist)
			KillUnit(true,false,0);
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

		if(!dontFire){
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

	if(damage<0){
//		info->AddLine("Negative damage");
		return;
	}

	if(attacker){
		float3 adir=attacker->pos-pos;
		adir.Normalize();
		bonusShieldDir+=adir*bonusShieldSaved;		//not the best way to do it(but fast)
		bonusShieldDir.Normalize();
//		info->AddLine("shield mult %f %f %i %i", 2-adir.dot(bonusShieldDir),bonusShieldSaved,id,attacker->id);
		bonusShieldSaved=0;
		damage*=1.4-adir.dot(bonusShieldDir)*0.5;
	}
	float3 hitDir = impulse;
	hitDir.y = 0;
	hitDir = -hitDir.Normalize();
	std::vector<long> hitAngles;
	hitAngles.push_back((long)(500 * hitDir.z));
	hitAngles.push_back((long)(500 * hitDir.x));
	cob->Call(COBFN_HitByWeapon, hitAngles);	

	damage*=curArmorMultiple;

	globalAI->UnitDamaged(this,attacker,damage);

	restTime=0;

	if(damages.paralyzeDamage){
		paralyzeDamage+=damage;
		if(health-paralyzeDamage<0){
			stunned=true;
			if(health-paralyzeDamage<-maxHealth*0.1)
				paralyzeDamage=health+maxHealth*0.1;
		}
	} else {
		health-=damage;
	}
	recentDamage+=damage;
	if(health<=0){
		KillUnit(false,false,attacker);
		if(isDead && attacker!=0 && !gs->Ally(allyteam,attacker->allyteam) && !beingBuilt){
			attacker->experience+=0.1*power/attacker->power;
			gs->Team(attacker->team)->currentStats.unitsKilled++;
		}
	}
	if(attacker!=0 && !gs->Ally(allyteam,attacker->allyteam)){
//		info->AddLine("%s has %f/%f h attacker %s do %f d",tooltip.c_str(),health,maxHealth,attacker->tooltip.c_str(),damage);
		SetLastAttacker(attacker);
		attacker->experience+=0.1*power/attacker->power*damage/maxHealth;
		attacker->ExperienceChange();
		ENTER_UNSYNCED;
		if((uh->lastDamageWarning+100<gs->frameNum || (unitDef->isCommander && uh->lastCmdDamageWarning+100<gs->frameNum))&& team==gu->myTeam && !camera->InView(midPos,radius+50) && gu->spectating){
			info->AddLine("%s is being attacked",unitDef->humanName.c_str());
			info->SetLastMsgPos(pos);
			if(unitDef->isCommander || uh->lastDamageWarning+150<gs->frameNum)
				sound->PlaySound(unitDef->sounds.underattack.id,unitDef->isCommander?1:0.4);
			minimap->AddNotification(pos,float3(1,0.3,0.3),unitDef->isCommander?1:0.5);	//todo: make compatible with new gui
			
			uh->lastDamageWarning=gs->frameNum;
			if(unitDef->isCommander)
				uh->lastCmdDamageWarning=gs->frameNum;
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
			float start=model->miny;
			glEnable(GL_CLIP_PLANE0);
			glEnable(GL_CLIP_PLANE1);

			float col=fabs(128.0-((gs->frameNum*4)&255))/255.0+0.5f;
			glColor3f(col*0.5,col,col*0.5);

			if(shadowHandler->drawShadows && !water->drawReflection){
				glDisable(GL_VERTEX_PROGRAM_ARB);
				glDisable(GL_FRAGMENT_PROGRAM_ARB);
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glDisable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE2_ARB);
				glDisable(GL_TEXTURE_CUBE_MAP_ARB);
				glActiveTextureARB(GL_TEXTURE3_ARB);
				glDisable(GL_TEXTURE_CUBE_MAP_ARB);
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
				glEnable(GL_FRAGMENT_PROGRAM_ARB);
				glEnable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glEnable(GL_TEXTURE_2D);
				glActiveTextureARB(GL_TEXTURE2_ARB);
				glEnable(GL_TEXTURE_CUBE_MAP_ARB);
				glActiveTextureARB(GL_TEXTURE3_ARB);
				glEnable(GL_TEXTURE_CUBE_MAP_ARB);
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
	}/**/
	glPopMatrix();
}

void CUnit::DrawStats()
{
	if(gu->myAllyTeam!=allyteam && !gu->spectating && unitDef->hideDamage){
		return;
	}

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

	if(stunned){
		glColor3f(0,0,1.0);
	} else {
		if(hpp>0.5f)
			glColor3f(1.0f-((hpp-0.5f)*2.0f),1.0f,0.0);
		else
			glColor3f(1.0f,hpp*2.0f,0.0);
	}
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

	power=unitDef->power*(1+limExperience);
	reloadSpeed=(1+limExperience*0.4);	

	float oldMaxHealth=maxHealth;
	maxHealth=unitDef->health*(1+limExperience*0.7);
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
	int oldteam=team;
	globalAI->UnitTaken (this, team);

	qf->RemoveUnit(this);
	quads.clear();
	loshandler->FreeInstance(los);
	los=0;
	losStatus[allyteam]=0;
	if(hasRadarCapacity)
		radarhandler->RemoveUnit(this);

	if(unitDef->isAirBase){
		airBaseHandler->DeregisterAirBase(this);
	}

	if(type==ChangeGiven){
		gs->Team(team)->RemoveUnit(this,CTeam::RemoveGiven);
		gs->Team(newteam)->AddUnit(this,CTeam::AddGiven);
	} else {
		gs->Team(team)->RemoveUnit(this,CTeam::RemoveCaptured);
		gs->Team(newteam)->AddUnit(this,CTeam::AddCaptured);
	}

	if(!beingBuilt){
		gs->Team(team)->metalStorage-=unitDef->metalStorage;
		gs->Team(team)->energyStorage-=unitDef->energyStorage;

		gs->Team(newteam)->metalStorage+=unitDef->metalStorage;
		gs->Team(newteam)->energyStorage+=unitDef->energyStorage;
	}

	commandAI->commandQue.clear();

	team=newteam;
	allyteam=gs->AllyTeam(team);

	loshandler->MoveUnit(this,false);
	losStatus[allyteam]=LOS_INTEAM | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;
	qf->MovedUnit(this);
	if(hasRadarCapacity)
		radarhandler->MoveUnit(this);

	//model=unitModelLoader->GetModel(model->name,newteam);
	model = modelParser->Load3DO(model->name.c_str(),1,newteam);
	delete localmodel;
	localmodel = modelParser->CreateLocalModel(model, &cob->pieces);

	if(unitDef->isAirBase){
		airBaseHandler->RegisterAirBase(this);
	}

	if(type==ChangeGiven && unitDef->energyUpkeep>25){//deactivate to prevent the old give metal maker trick
		Command c;
		c.id=CMD_ONOFF;

		c.params.push_back(0);
		commandAI->GiveCommand(c);
	}

	globalAI->UnitGiven (this, oldteam);
}

bool CUnit::AttackUnit(CUnit *unit,bool dgun)
{
	bool r=false;
	haveDGunRequest=dgun;
	userAttackGround=false;
	SetUserTarget(unit);
	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi){
		(*wi)->haveUserTarget=false;
		if(dgun || !unitDef->canDGun || !(*wi)->weaponDef->manualfire)
			if((*wi)->AttackUnit(unit,true))
				r=true;
	}
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
	for(wi=weapons.begin();wi!=weapons.end();++wi){
		(*wi)->haveUserTarget=false;
		if(dgun || !unitDef->canDGun || !(*wi)->weaponDef->manualfire)
			if((*wi)->AttackGround(pos,true))
				r=true;
	}
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
	if(o==transporter)
		transporter=0;
	if(o==lastAttacker){
//		info->AddLine("Last attacker died %i %i",lastAttacker->id,id);
		lastAttacker=0;
	}
	if(o==userTarget)
		userTarget=0;

	incomingMissiles.remove((CMissileProjectile*)o);

	CSolidObject::DependentDied(o);
}

void CUnit::SetUserTarget(CUnit* target)
{
	if(userTarget && lastAttacker!=userTarget)
		DeleteDeathDependence(userTarget);

	userTarget=target;
	for(vector<CWeapon*>::iterator wi=weapons.begin();wi!=weapons.end();++wi)
		(*wi)->haveUserTarget=false;

	if(target){
		AddDeathDependence(target);
	}
}


void CUnit::Init(void)
{
	relMidPos=model->relMidPos;

	midPos=pos+frontdir*relMidPos.z + updir*relMidPos.y + rightdir*relMidPos.x;
	losHeight=relMidPos.y+radius*0.5;
	height = model->height;		//TODO: This one would be much better to have either in Constructor or UnitLoader!//why this is always called in unitloader

	//All ships starts on water, all other on ground.
	//TODO: Improve this. There might be cases when this is not correct.
	if(unitDef->movedata && unitDef->movedata->moveType==MoveData::Hover_Move){
		physicalState = Hovering;		
	} else if(floatOnWater) {
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
	if(pos.y+model->height<1)	//some torp launchers etc is exactly in the surface and should be considered uw anyway
		isUnderWater=true;

	if(!unitDef->canKamikaze || unitDef->type!="Building")	//semi hack to make mines not block ground
		Block();

	UpdateTerrainType();

	globalAI->UnitCreated(this);
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

	if (transporter) {
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

bool CUnit::SetGroup(CGroup* newGroup)
{
	if(group!=0){
		group->RemoveUnit(this);
	}
	group=newGroup;

	if(group){
		if(!group->AddUnit(this)){
			group=0;									//group ai didnt accept us
			return false;
		}
	}
	return true;
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
			if(gs->Team(builder->team)->metal>metalUse && gs->Team(builder->team)->energy>energyUse){
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
				if(health>maxHealth)
					health=maxHealth;
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
				KillUnit(false,true,0);
				return false;
			}
		} else {
			if(health<0){
				builder->AddMetal(metalCost);
				KillUnit(false,true,0);
				return false;
			}
		}
		return true;
	}
	return false;
}

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
			cob->Call(COBFN_SetSpeed, (long)(unitDef->windGenerator*3000.0f));
		}
		else
		{
			cob->Call(COBFN_SetSpeed, (long)(wind.curStrength*3000.0f));
		}
		cob->Call(COBFN_SetDirection, (long)GetHeadingFromVector(-wind.curDir.x, -wind.curDir.z));
	}

	if(unitDef->activateWhenBuilt)
	{
		Activate();
	}

	gs->Team(team)->metalStorage+=unitDef->metalStorage;
	gs->Team(team)->energyStorage+=unitDef->energyStorage;

	//Sets the frontdir in sync with heading.
	frontdir = GetVectorFromHeading(heading) + float3(0,frontdir.y,0);

	if(unitDef->isAirBase){
		airBaseHandler->RegisterAirBase(this);
	}

	globalAI->UnitFinished(this);

	if(unitDef->isFeature){
		UnBlock();
		CFeature* f=featureHandler->CreateWreckage(pos,wreckName, heading, 0,allyteam,false,"");
		if(f){
			f->blockHeightChanges=true;
		}
		KillUnit(false,true,0);
	}
}

void CUnit::KillUnit(bool selfDestruct,bool reclaimed,CUnit *attacker)
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

	globalAI->UnitDestroyed(this,attacker);

	blockHeightChanges=false;
	if(unitDef->isCommander)
		gs->Team(team)->CommanderDied(this);

	if(!reclaimed && !beingBuilt){
		string exp;
		if(selfDestruct)
			exp=unitDef->selfDExplosion;
		else
			exp=unitDef->deathExplosion;

		if(!exp.empty()){
			WeaponDef* wd=weaponDefHandler->GetWeapon(exp);
			if(wd){
				helper->Explosion(midPos,wd->damages,wd->areaOfEffect,this,true,wd->damages[0]>500?1:2);

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
		args.push_back((long)(recentDamage/maxHealth*100));
		args.push_back(0);
		cob->Call(COBFN_Killed, args);

		UnBlock();
		delayedWreckLevel=args[1];
//		featureHandler->CreateWreckage(pos,wreckName, heading, args[1],-1,true);
	}
	if(beingBuilt || dynamic_cast<CAirMoveType*>(moveType) || reclaimed)
		uh->DeleteUnit(this);
	else{
		speed=ZeroVector;
		deathCountdown=5;
		stunned=true;
		paralyzeDamage=1000000;
		if(health<0)
			health=0;
	}
}

bool CUnit::UseMetal(float metal)
{
	if(metal<0){
		AddMetal(-metal);
		return true;
	}
	bool canUse=gs->Team(team)->UseMetal(metal);
	if(canUse)
		metalUseI += metal;
	return canUse;
}

void CUnit::AddMetal(float metal)
{
	if(metal<0){
		UseMetal(-metal);
		return;
	}
	metalMakeI += metal;
	gs->Team(team)->AddMetal(metal);
}

bool CUnit::UseEnergy(float energy)
{
	if(energy<0){
		AddEnergy(-energy);
		return true;
	}
	bool canUse=gs->Team(team)->UseEnergy(energy);
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
	gs->Team(team)->AddEnergy(energy);
}

void CUnit::Activate()
{
	//if(unitDef->tidalGenerator>0)
	//	cob->Call(COBFN_SetSpeed, (long)(readmap->tidalStrength*3000.0f*unitDef->tidalGenerator));

	if(activated)
		return;
	activated = true;

	cob->Call(COBFN_Activate);

	if(unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]/=radarhandler->targFacEffect;
	}
	if(hasRadarCapacity)
		radarhandler->MoveUnit(this);

	if(unitDef->sounds.activate.id)
		sound->PlaySound(unitDef->sounds.activate.id, pos, unitDef->sounds.activate.volume);
}

void CUnit::Deactivate()
{
	if(!activated)
		return;
	activated = false;

	cob->Call(COBFN_Deactivate);

	if(unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]*=radarhandler->targFacEffect;
	}
	if(hasRadarCapacity)
		radarhandler->RemoveUnit(this);

	if(unitDef->sounds.deactivate.id)
		sound->PlaySound(unitDef->sounds.deactivate.id, pos, unitDef->sounds.deactivate.volume);
}

void CUnit::PushWind(float x, float z, float strength)
{
	if(strength > unitDef->windGenerator)
	{
		cob->Call(COBFN_SetSpeed, (long)(unitDef->windGenerator*3000.0f));
	}
	else
	{
		cob->Call(COBFN_SetSpeed, (long)(strength*3000.0f));
	}

	cob->Call(COBFN_SetDirection, (long)GetHeadingFromVector(-x, -z));
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

void CUnit::IncomingMissile(CMissileProjectile* missile)
{
	if(unitDef->canDropFlare){
		incomingMissiles.push_back(missile);
		AddDeathDependence(missile);

		if(lastFlareDrop < gs->frameNum - unitDef->flareReloadTime*30){
			new CFlareProjectile(pos,speed,this,(int)(gs->frameNum+unitDef->flareDelay*(1+gs->randFloat())*15));
			lastFlareDrop=gs->frameNum;
		}
	}
}

void CUnit::TempHoldFire(void)
{
	dontFire=true;
	AttackUnit(0,true);
}

void CUnit::ReleaseTempHoldFire(void)
{
	dontFire=false;
}

void CUnit::DrawS3O(void)
{
	unsigned char* col=gs->Team(team)->color;
	if(unitDrawer->advShading){
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,14, col[0]*(1./255.),col[1]*(1./255.),col[2]*(1./255.),1);
	} else {
		float texConstant[]={col[0]*(1./255.),col[1]*(1./255.),col[2]*(1./255.),1};
		glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,texConstant); 
	}
	Draw();
}
