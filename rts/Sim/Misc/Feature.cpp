#include "StdAfx.h"
#include "Feature.h"
#include "Rendering/GL/myGL.h"
#include "FeatureHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Map/Ground.h"
#include "QuadField.h"
#include "DamageArray.h"
#include "Map/ReadMap.h"
#include "Game/UI/InfoConsole.h"
#include "Sim/Units/Unit.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Sim/Projectiles/FireProjectile.h"
#include "Sim/Projectiles/SmokeProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "myMath.h"
#include "mmgr.h"

CR_BIND_DERIVED(CFeature, CSolidObject)

CR_REG_METADATA(CFeature, (
				CR_MEMBER(createdFromUnit),
				CR_MEMBER(resurrectProgress),
				CR_MEMBER(health),
				CR_MEMBER(reclaimLeft),
				CR_MEMBER(id),
				CR_MEMBER(allyteam),
				CR_MEMBER(tempNum),
				CR_MEMBER(lastReclaim),
				CR_MEMBER(def),
				//CR_MEMBER(transMatrix),
				CR_MEMBER(inUpdateQue),
				CR_MEMBER(drawQuad),
				CR_MEMBER(finalHeight),
				CR_MEMBER(myFire),
				CR_MEMBER(fireTime),
				CR_MEMBER(emitSmokeTime)));

CFeature::CFeature()
:	def(0),
	inUpdateQue(false),
	reclaimLeft(1),
	fireTime(0),
	myFire(0),
	drawQuad(-1),
	allyteam(0),
	tempNum(0),
	emitSmokeTime(0),
	lastReclaim(0),
	resurrectProgress(0),
	health(0),
	id(0),
	finalHeight(0)
{
	immobile=true;
	physicalState = OnGround;
}

CFeature::~CFeature(void)
{
	if(blocking){
		UnBlock();
	}
	qf->RemoveFeature(this);
	if(def->drawType==DRAWTYPE_TREE)
		treeDrawer->DeleteTree(pos);

	if(myFire){
		myFire->StopFire();
		myFire=0;
	}
}

void CFeature::Initialize (const float3& pos,FeatureDef* def,short int heading,int allyteam,std::string fromUnit)
{
	this->def=def;
	createdFromUnit=fromUnit;
	this->pos=pos;
	this->allyteam=allyteam;
	this->pos.CheckInBounds();
	this->heading=heading;
	health=def->maxHealth;
	SetRadius(def->radius);
	blocking=def->blocking;
	xsize=def->xsize;
	ysize=def->ysize;
	mass=def->mass;

	if(def->drawType==DRAWTYPE_3DO){
		if(def->model==0){
			def->model=modelParser->Load3DO(def->modelname.c_str());
			height=def->model->height;
			def->radius=def->model->radius;
			SetRadius(def->radius);
		}
		midPos=pos+def->model->relMidPos;
	} else if(def->drawType==DRAWTYPE_TREE){
		midPos=pos+UpVector*def->radius;
		height = 2*def->radius;
	} else {
		midPos=pos;
	}
	id=featureHandler->AddFeature(this);
	qf->AddFeature(this);

	CalculateTransform ();
//	this->pos.y=ground->GetHeight(pos.x,pos.z);

	if(blocking){
		Block();
	}
	if(def->floating){
		finalHeight=ground->GetHeight(pos.x,pos.z);
	} else {
		finalHeight=ground->GetHeight2(pos.x,pos.z);
	}

	if(def->drawType==DRAWTYPE_TREE)
		treeDrawer->AddTree(def->modelType,pos,1);
}

void CFeature::CalculateTransform ()
{
	float3 frontDir=GetVectorFromHeading(heading);
	float3 upDir;

	if (def->upright) upDir = float3(0.0f,1.0f,0.0f);
	else upDir = ground->GetNormal(pos.x,pos.z);

	float3 rightDir=frontDir.cross(upDir);
	rightDir.Normalize();
	frontDir=upDir.cross(rightDir);
	frontDir.Normalize ();

	transMatrix = CMatrix44f (pos,-rightDir,upDir,frontDir);
}

bool CFeature::AddBuildPower(float amount, CUnit* builder)
{
	if(amount>0){
		return false;		//cant repair a feature
	} else {
		if(reclaimLeft<0)	//avoid multisuck :)
			return false;
		if(lastReclaim==gs->frameNum)	//make sure several units cant reclaim at once on a single feature
			return true;
		float part=(100-amount)*0.02/max(10.0f,(def->metal+def->energy));
		reclaimLeft-=part;
		lastReclaim=gs->frameNum;
		if(reclaimLeft<0){
			builder->AddMetal(def->metal);
			builder->AddEnergy(def->energy);
			featureHandler->DeleteFeature(this);
			return false;
		}
		return true;
	}
	return false;
}

void CFeature::DoDamage(const DamageArray& damages, CUnit* attacker,const float3& impulse)
{
	residualImpulse=impulse;
	health-=damages[0];
	if(health<=0 && def->destructable){
		featureHandler->CreateWreckage(pos,def->deathFeature,heading,1,-1,false,"");
		featureHandler->DeleteFeature(this);
		blockHeightChanges=false;

		if(def->drawType==DRAWTYPE_TREE){
			if(impulse.Length2D()>0.5){
				treeDrawer->AddFallingTree(pos,impulse,def->modelType);
			}
		}
	}
}

void CFeature::Kill(float3& impulse) {
	DamageArray damage;
	DoDamage(damage*(health+1), 0, impulse);
}

bool CFeature::Update(void)
{
	bool retValue=false;

	if(pos.y>finalHeight){
		if(pos.y>0){	//fall faster when above water
			pos.y-=0.8;
			midPos.y-=0.8;
			transMatrix[13]-=0.8;
		} else {
			pos.y-=0.4;
			midPos.y-=0.4;
			transMatrix[13]-=0.4;
		}
//		info->AddLine("feature sinking");
		retValue=true;
	}
	if(emitSmokeTime!=0){
		--emitSmokeTime;
		PUSH_CODE_MODE;
		ENTER_MIXED;
		if(!(gs->frameNum+id & 3) && ph->particleSaturation<0.7){
			new CSmokeProjectile(midPos+gu->usRandVector()*radius*0.3,gu->usRandVector()*0.3+UpVector,emitSmokeTime/6+20,6,0.4,0,0.5);
		}
		POP_CODE_MODE;
		retValue=true;
	}

	if(fireTime>0){
		fireTime--;
		if(fireTime==1)
			featureHandler->DeleteFeature(this);
		retValue=true;
	}

	if(def->geoThermal){
		PUSH_CODE_MODE;
		ENTER_MIXED;
		if((ph->particleSaturation<0.7 && !(gs->frameNum&1)) || (ph->particleSaturation<1 && !(gs->frameNum&3))){
			float3 speed=gu->usRandVector()*0.1;
			speed.y+=1;
			new CSmokeProjectile(pos,speed,70+gu->usRandFloat()*20,3,0.15,0,0.8);
		}
		POP_CODE_MODE;
		retValue=true;
	}


	return retValue;
}

void CFeature::StartFire(void)
{
	if(fireTime || !def->burnable)
		return;

	fireTime=200+(int)(gs->randFloat()*30);
	featureHandler->SetFeatureUpdateable(this);

	myFire=new CFireProjectile(midPos,UpVector,0,300,radius*0.8,70,20);
}

void CFeature::DrawS3O()
{
	glPushMatrix();
	glMultMatrixf(transMatrix.m);
	def->model->DrawStatic();
	glPopMatrix();
}

FeatureDef::~FeatureDef() {
}
