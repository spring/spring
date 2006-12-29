#include "StdAfx.h"
#include "Feature.h"
#include "Rendering/GL/myGL.h"
#include "FeatureHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Map/Ground.h"
#include "QuadField.h"
#include "DamageArray.h"
#include "Map/ReadMap.h"
#include "Game/Team.h"
#include "LogOutput.h"
#include "Sim/Units/Unit.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Sim/ModInfo.h"
#include "Sim/Projectiles/FireProjectile.h"
#include "Sim/Projectiles/SmokeProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/GeoThermSmokeProjectile.h"
#include "myMath.h"
#include "mmgr.h"

CR_BIND_DERIVED(CFeature, CSolidObject)

CR_REG_METADATA(CFeature, (
				CR_MEMBER(createdFromUnit),
				CR_MEMBER(isRepairingBeforeResurrect),
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
	isRepairingBeforeResurrect(false),
	resurrectProgress(0),
	health(0),
	id(0),
	finalHeight(0),
	solidOnTop(0)
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

void CFeature::Initialize (const float3& pos,FeatureDef* def,short int heading,int facing, int allyteam,std::string fromUnit)
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
	buildFacing=facing;

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
	float oldReclaimLeft = reclaimLeft;
	float fractionReclaimed;
	if(amount>0){
		
		// Check they are trying to repair a feature that can be resurrected
		if(createdFromUnit == "")
			return false;

		// 'Repairing' previously-sucked features prior to resurrection
		// This is reclaim-option independant - repairing features should always
		// be like other repairing - gradual and multi-unit
		// Lots of this code is stolen from unit->AddBuildPower
		
		isRepairingBeforeResurrect = true; // Stop them exploiting chunk reclaiming

		if (reclaimLeft >= 1)
			return false;		// cant repair a 'fresh' feature

		// Work out how much to try to put back, based on the speed this unit would reclaim at.
		float part=(100-amount)*0.02f/max(10.0f,(def->metal+def->energy));

		// Work out how much that will cost
		float metalUse=def->metal*part;
		float energyUse=def->energy*part;
		if (gs->Team(builder->team)->metal >= metalUse && gs->Team(builder->team)->energy >= energyUse)
		{
			builder->UseMetal(metalUse);
			builder->UseEnergy(energyUse);
			reclaimLeft+=part;
			if(reclaimLeft>=1)
			{
				isRepairingBeforeResurrect = false; // They can start reclaiming it again if they so wish
				reclaimLeft = 1;
			}
			return true;
		} else {
			// update the energy and metal required counts
			gs->Team(builder->team)->energyPull += energyUse;
			gs->Team(builder->team)->metalPull += metalUse;
		}
		return false;


	} else {
		// Reclaiming
		if(reclaimLeft <= 0)	// avoid multisuck when reclaim has already completed during this frame
			return false;
		
		if(isRepairingBeforeResurrect && modInfo->reclaimMethod > 1) // don't let them exploit chunk reclaim
			return false;
		
		if(modInfo->multiReclaim == 0 && lastReclaim == gs->frameNum) // make sure several units cant reclaim at once on a single feature
			return true;
		
		float part=(100-amount)*0.02f/max(10.0f,(def->metal+def->energy));
		reclaimLeft-=part;
		
		// Stop the last bit giving too much resource
		if(reclaimLeft < 0) reclaimLeft = 0;

		fractionReclaimed = oldReclaimLeft-reclaimLeft;

		if(modInfo->reclaimMethod == 1 && reclaimLeft == 0) // All-at-end method
		{
			builder->AddMetal(def->metal);
			builder->AddEnergy(def->energy);
		}
		else if(modInfo->reclaimMethod == 0) // Gradual reclaim
		{
			builder->AddMetal(def->metal * fractionReclaimed);
			builder->AddEnergy(def->energy * fractionReclaimed);
		}
		else  // Chunky reclaiming
		{
			// Work out how many chunk boundaries we crossed
			float chunkSize = 1.0f / modInfo->reclaimMethod;
			int oldChunk = ChunkNumber(oldReclaimLeft);
			int newChunk = ChunkNumber(reclaimLeft);
			if (oldChunk != newChunk)
			{
				float noChunks = (float)oldChunk - (float)newChunk;
				builder->AddMetal(noChunks * def->metal * chunkSize);
				builder->AddEnergy(noChunks * def->energy * chunkSize);
			}
		}

		// Has the reclaim finished?
		if(reclaimLeft<=0)
		{
			featureHandler->DeleteFeature(this);
			return false;
		}

		lastReclaim=gs->frameNum;
		return true;
	}
	// Should never get here
	assert(false);
	return false;
}

void CFeature::DoDamage(const DamageArray& damages, CUnit* attacker,const float3& impulse)
{
	residualImpulse=impulse;
	health-=damages[0];
	if(health<=0 && def->destructable){
		featureHandler->CreateWreckage(pos,def->deathFeature,heading,buildFacing, 1,-1,false,"");
		featureHandler->DeleteFeature(this);
		blockHeightChanges=false;

		if(def->drawType==DRAWTYPE_TREE){
			if(impulse.Length2D()>0.5f){
				treeDrawer->AddFallingTree(pos,impulse,def->modelType);
			}
		}
	}
}

void CFeature::Kill(float3& impulse) {
	DamageArray damage;
	DoDamage(damage*(health+1), 0, impulse);
}

void CFeature::DependentDied(CObject *o)
{
	if (o == solidOnTop)
		solidOnTop = 0;

	CSolidObject::DependentDied(o);
}

bool CFeature::Update(void)
{
	bool retValue=false;

	if(pos.y>finalHeight){
		if(pos.y>0){	//fall faster when above water
			pos.y-=0.8f;
			midPos.y-=0.8f;
			transMatrix[13]-=0.8f;
		} else {
			pos.y-=0.4f;
			midPos.y-=0.4f;
			transMatrix[13]-=0.4f;
		}
//		logOutput.Print("feature sinking");
		retValue=true;
	}
	if(emitSmokeTime!=0){
		--emitSmokeTime;
		PUSH_CODE_MODE;
		ENTER_MIXED;
		if(!(gs->frameNum+id & 3) && ph->particleSaturation<0.7f){
			SAFE_NEW CSmokeProjectile(midPos+gu->usRandVector()*radius*0.3f,gu->usRandVector()*0.3f+UpVector,emitSmokeTime/6+20,6,0.4f,0,0.5f);
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

		if ((gs->frameNum+id % 5) % 5 == 0) 
		{
			// Find the unit closest to the geothermal
			vector<CSolidObject*> objs = qf->GetSolidsExact(pos, 0.0f);
			float bestDist2;
			CSolidObject *so=0;
			for (vector<CSolidObject*>::iterator oi=objs.begin();oi!=objs.end();++oi) {
				float dist2 = ((*oi)->pos-pos).SqLength();
				if (!so || dist2 < bestDist2)  {
					bestDist2 = dist2;
					so = *oi;
				}
			}

			if (so!=solidOnTop) {
				if (solidOnTop) 
					DeleteDeathDependence(solidOnTop);
				if (so)
					AddDeathDependence(so);
			}
			solidOnTop = so;
		}

		// Hide the smoke if there is a geothermal unit on the vent
		CUnit *u = dynamic_cast<CUnit*>(solidOnTop);
		if (!u || !u->unitDef->needGeo) {
			if((ph->particleSaturation<0.7f) || (ph->particleSaturation<1 && !(gs->frameNum&3))){
				float3 speed=gu->usRandVector()*0.5f;
				speed.y+=2.0f;

				SAFE_NEW CGeoThermSmokeProjectile(gu->usRandVector()*10 + float3(pos.x,pos.y-10,pos.z),speed,int(50+gu->usRandFloat()*7), this);
			}
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

	myFire=SAFE_NEW CFireProjectile(midPos,UpVector,0,300,radius*0.8f,70,20);
}

void CFeature::DrawS3O()
{
	glPushMatrix();
	glMultMatrixf(transMatrix.m);
	def->model->DrawStatic();
	glPopMatrix();
}

int CFeature::ChunkNumber(float f)
{
	return (int) ceil(f * modInfo->reclaimMethod);	
}

float CFeature::RemainingResource(float res)
{
	// Old style - all reclaimed at the end
	if(modInfo->reclaimMethod == 0)
		return res * reclaimLeft;

	// Gradual reclaim
	if(modInfo->reclaimMethod == 1)
		return res;

	// Otherwise we are doing chunk reclaiming
	float chunkSize = res / modInfo->reclaimMethod; // resource/no_chunks
	float chunksLeft = ceil(reclaimLeft * modInfo->reclaimMethod);
	return chunkSize * chunksLeft;
}

float CFeature::RemainingMetal()
{
	return RemainingResource(def->metal);
}
float CFeature::RemainingEnergy()
{
	return RemainingResource(def->energy);
}


FeatureDef::~FeatureDef() {
}
