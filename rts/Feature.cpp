#include "StdAfx.h"
#include "Feature.h"
#include "myGL.h"
#include "FeatureHandler.h"
#include "3DOParser.h"
#include "Ground.h"
#include "QuadField.h"
#include "DamageArray.h"
#include "ReadMap.h"
#include "InfoConsole.h"
#include "Unit.h"
#include "BaseTreeDrawer.h"
#include "FireProjectile.h"
#include "SmokeProjectile.h"
#include "ProjectileHandler.h"
//#include "mmgr.h"

CFeature::CFeature(const float3& pos,FeatureDef* def,short int heading,int allyteam,std::string fromUnit)
: CSolidObject(pos),
	def(def),
	inUpdateQue(false),
	reclaimLeft(1),
	fireTime(0),
	myFire(0),
	drawQuad(-1),
	allyteam(allyteam),
	tempNum(0),
	emitSmokeTime(0),
	lastReclaim(0),
	createdFromUnit(fromUnit),
	resurrectProgress(0)
{
	this->pos.CheckInBounds();
	this->heading=heading;
	health=def->maxHealth;
	SetRadius(def->radius);
	blocking=def->blocking;
	xsize=def->xsize;
	ysize=def->ysize;
	mass=def->mass;
	immobile=true;
	physicalState = OnGround;

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

//	this->pos.y=ground->GetHeight(pos.x,pos.z);
	transMatrix.Translate(pos.x,pos.y,pos.z);
	transMatrix.RotateY(-heading*PI/0x7fff);
	transMatrix.SetUpVector(ground->GetNormal(pos.x,pos.z));

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
