#include "StdAfx.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "Game/Team.h"
#include "LogOutput.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "myMath.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/QuadField.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/ModInfo.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/FireProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/GeoThermSmokeProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "mmgr.h"

CR_BIND_DERIVED(CFeature, CSolidObject, )

CR_REG_METADATA(CFeature, (
				//CR_MEMBER(model),
				CR_MEMBER(createdFromUnit),
				CR_MEMBER(isRepairingBeforeResurrect),
				CR_MEMBER(resurrectProgress),
				CR_MEMBER(health),
				CR_MEMBER(reclaimLeft),
				CR_MEMBER(id),
				CR_MEMBER(allyteam),
				CR_MEMBER(team),
				CR_MEMBER(noSelect),
				CR_MEMBER(tempNum),
				CR_MEMBER(lastReclaim),
//				CR_MEMBER(def),
				CR_MEMBER(defName),
				CR_MEMBER(transMatrix),
				CR_MEMBER(inUpdateQue),
				CR_MEMBER(drawQuad),
				CR_MEMBER(finalHeight),
				CR_MEMBER(myFire),
				CR_MEMBER(fireTime),
				CR_MEMBER(emitSmokeTime),
				CR_RESERVED(64),
				CR_POSTLOAD(PostLoad)
				));


CFeature::CFeature()
:	def(0),
	inUpdateQue(false),
	reclaimLeft(1),
	fireTime(0),
	myFire(0),
	drawQuad(-1),
	team(0),
	allyteam(0),
	noSelect(false),
	tempNum(0),
	emitSmokeTime(0),
	lastReclaim(0),
	isRepairingBeforeResurrect(false),
	resurrectProgress(0),
	health(0),
	id(0),
	finalHeight(0),
	solidOnTop(0),
	model(NULL)
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

	if (def->geoThermal) {
		CGeoThermSmokeProjectile::GeoThermDestroyed(this);
	}
}

void CFeature::PostLoad()
{
	def = featureHandler->GetFeatureDef(defName);
	if (def->drawType == DRAWTYPE_3DO) {
		model = def->LoadModel(team);
		height = model->height;
		SetRadius(model->radius);
		midPos = pos + model->relMidPos;
	}
	else if (def->drawType == DRAWTYPE_TREE){
		midPos = pos + (UpVector * TREE_RADIUS);
		height = 2 * TREE_RADIUS;
	}
	else {
		midPos = pos;
	}
	if (def->drawType == DRAWTYPE_TREE) {
		treeDrawer->AddTree(def->modelType, pos, 1);
	}
}

void CFeature::ChangeTeam(int newTeam)
{
	if (newTeam < 0) {
		team = 0; // NOTE: this should probably be -1, would need work
		allyteam = -1;
	} else {
		team = newTeam;
		allyteam = gs->AllyTeam(newTeam);
	}

	if (def->drawType == DRAWTYPE_3DO){
		model = def->LoadModel(team);
	}
}


void CFeature::Initialize(const float3& _pos, const FeatureDef* _def, short int _heading,
                          int facing, int _team, std::string fromUnit)
{
	pos = _pos;
	def = _def;
	defName = def->myName;
	heading = _heading;
	buildFacing = facing;
	team = _team;
	createdFromUnit = fromUnit;

	ChangeTeam(team);

	pos.CheckInBounds();

	health   = def->maxHealth;
	blocking = def->blocking;
	xsize    = def->xsize;
	ysize    = def->ysize;
	mass     = def->mass;
	noSelect = def->noSelect;

	if (def->drawType == DRAWTYPE_3DO) {
		model = def->LoadModel(team);
		height = model->height;
		SetRadius(model->radius);
		midPos = pos + model->relMidPos;

		// CFeatureHandler left this volume's axis-scales uninitialized
		if (def->collisionVolume->GetScale(COLVOL_AXIS_X) < 0.01f &&
			def->collisionVolume->GetScale(COLVOL_AXIS_Y) < 0.01f &&
			def->collisionVolume->GetScale(COLVOL_AXIS_Z) < 0.01f) {
			def->collisionVolume->SetDefaultScale(model->radius);
		}
	}
	else if (def->drawType == DRAWTYPE_TREE){
		SetRadius(TREE_RADIUS);
		midPos = pos + (UpVector * TREE_RADIUS);
		height = 2 * TREE_RADIUS;
		def->collisionVolume->SetDefaultScale(TREE_RADIUS);
	}
	else {
		SetRadius(0.0f);
		midPos = pos;
	}

	featureHandler->AddFeature(this);

	qf->AddFeature(this);

	CalculateTransform ();
//	this->pos.y=ground->GetHeight(pos.x,pos.z);

	if (blocking) {
		Block();
	}

	if (def->floating) {
		finalHeight = ground->GetHeight(pos.x, pos.z);
	} else {
		finalHeight = ground->GetHeight2(pos.x, pos.z);
	}

	if (def->drawType == DRAWTYPE_TREE) {
		treeDrawer->AddTree(def->modelType, pos, 1);
	}
}


void CFeature::CalculateTransform()
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
	const float oldReclaimLeft = reclaimLeft;

	if (amount > 0.0f) {
		// Check they are trying to repair a feature that can be resurrected
		if (createdFromUnit == "") {
			return false;
		}

		// 'Repairing' previously-sucked features prior to resurrection
		// This is reclaim-option independant - repairing features should always
		// be like other repairing - gradual and multi-unit
		// Lots of this code is stolen from unit->AddBuildPower

		isRepairingBeforeResurrect = true; // Stop them exploiting chunk reclaiming

		if (reclaimLeft >= 1) {
			return false; // cant repair a 'fresh' feature
		}

		// Work out how much to try to put back, based on the speed this unit would reclaim at.
		const float part = (100 - amount) * 0.02f / max(10.0f, (def->metal + def->energy));

		// Work out how much that will cost
		const float metalUse  = part * def->metal;
		const float energyUse = part * def->energy;
		if ((gs->Team(builder->team)->metal  >= metalUse)  &&
		    (gs->Team(builder->team)->energy >= energyUse) &&
				(!luaRules || luaRules->AllowFeatureBuildStep(builder, this, part))) {
			builder->UseMetal(metalUse);
			builder->UseEnergy(energyUse);
			reclaimLeft+=part;
			if (reclaimLeft >= 1) {
				isRepairingBeforeResurrect = false; // They can start reclaiming it again if they so wish
				reclaimLeft = 1;
			}
			return true;
		}
		else {
			// update the energy and metal required counts
			gs->Team(builder->team)->energyPull += energyUse;
			gs->Team(builder->team)->metalPull  += metalUse;
		}
		return false;
	}
	else { // Reclaiming
		// avoid multisuck when reclaim has already completed during this frame
		if (reclaimLeft <= 0) {
			return false;
		}

		// don't let them exploit chunk reclaim
		if (isRepairingBeforeResurrect && (modInfo.reclaimMethod > 1)) {
			return false;
		}

		// make sure several units cant reclaim at once on a single feature
		if ((modInfo.multiReclaim == 0) && (lastReclaim == gs->frameNum)) {
			return true;
		}

		const float part = ((100 - amount) * 0.02f / max(10.0f, (def->reclaimTime)));

		if (luaRules && !luaRules->AllowFeatureBuildStep(builder, this, part)) {
			return false;
		}

		reclaimLeft -= part;

		// stop the last bit giving too much resource
		if (reclaimLeft < 0) {
			reclaimLeft = 0;
		}

		const float fractionReclaimed = oldReclaimLeft - reclaimLeft;

		if ((modInfo.reclaimMethod == 1) && (reclaimLeft == 0)) {
			// All-at-end method
			builder->AddMetal(def->metal);
			builder->AddEnergy(def->energy);
		}
		else if (modInfo.reclaimMethod == 0) {
			// Gradual reclaim
			builder->AddMetal(def->metal * fractionReclaimed);
			builder->AddEnergy(def->energy * fractionReclaimed);
		}
		else {
			// Chunky reclaiming, work out how many chunk boundaries we crossed
			const float chunkSize = 1.0f / modInfo.reclaimMethod;
			const int oldChunk = ChunkNumber(oldReclaimLeft);
			const int newChunk = ChunkNumber(reclaimLeft);
			if (oldChunk != newChunk) {
				const float noChunks = (float)oldChunk - (float)newChunk;
				builder->AddMetal(noChunks * def->metal * chunkSize);
				builder->AddEnergy(noChunks * def->energy * chunkSize);
			}
		}

		// Has the reclaim finished?
		if (reclaimLeft <= 0) {
			featureHandler->DeleteFeature(this);
			return false;
		}

		lastReclaim = gs->frameNum;
		return true;
	}

	// Should never get here
	assert(false);
	return false;
}


void CFeature::DoDamage(const DamageArray& damages, CUnit* attacker,const float3& impulse)
{
	if (damages.paralyzeDamageTime) {
		return; // paralyzers do not damage features
	}

	residualImpulse = impulse;
	health -= damages[0];

	if (health <= 0 && def->destructable) {
		CFeature* deathFeature = featureHandler->CreateWreckage(
			pos, def->deathFeature, heading,
			buildFacing, 1, team, -1, false, ""
		);

		if (deathFeature) {
			// if a partially reclaimed corpse got blasted,
			// ensure its wreck is not worth the full amount
			// (which might be more than the amount remaining)
			deathFeature->reclaimLeft = reclaimLeft;
		}

		featureHandler->DeleteFeature(this);
		blockHeightChanges = false;

		if (def->drawType == DRAWTYPE_TREE) {
			if (impulse.Length2D() > 0.5f) {
				treeDrawer->AddFallingTree(pos, impulse, def->modelType);
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


void CFeature::ForcedMove(const float3& newPos)
{
	featureHandler->UpdateDrawQuad(this, newPos);

	// remove from managers
	qf->RemoveFeature(this);
	if (def->drawType == DRAWTYPE_TREE) {
		treeDrawer->DeleteTree(pos);
	}
	UnBlock();

	pos = newPos;

	// setup finalHeight
	if (def->floating) {
		finalHeight = ground->GetHeight(pos.x, pos.z);
	} else {
		finalHeight = ground->GetHeight2(pos.x, pos.z);
	}

	// setup midPos
	if (def->drawType == DRAWTYPE_3DO) {
		midPos = pos + model->relMidPos;
	} else if (def->drawType == DRAWTYPE_TREE){
		midPos = pos + (UpVector * TREE_RADIUS);
	} else {
		midPos = pos;
	}

	// insert into managers
	qf->AddFeature(this);
	if (def->drawType == DRAWTYPE_TREE) {
		treeDrawer->AddTree(def->modelType, pos, 1.0f);
	}
	Block();
}


void CFeature::ForcedSpin(const float3& newDir)
{
/*
	heading = GetHeadingFromVector(newDir.x, newDir.z);
	CalculateTransform();
	if (def->drawType == DRAWTYPE_TREE) {
		treeDrawer->DeleteTree(pos);
		treeDrawer->AddTree(def->modelType, pos, 1.0f);
	}
*/

	CMatrix44f tmp;
	tmp.RotateZ(newDir.z);
	tmp.RotateX(newDir.x);
	tmp.RotateY(newDir.y);
	tmp.Translate(pos);
	transMatrix = tmp;

//	const float clamped = fmod(newDir.y, PI * 2.0);
//	heading = (short int)(clamped * 65536);
}


bool CFeature::Update(void)
{
	bool retValue=false;

	if(pos.y>finalHeight){
		const float3 oldPos = pos;
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
		if (def->drawType == DRAWTYPE_TREE) {
			treeDrawer->DeleteTree(oldPos);
			treeDrawer->AddTree(def->modelType, pos, 1.0f);
		}
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
			float bestDist2 = 0;
			CSolidObject *so = NULL;
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

				SAFE_NEW CGeoThermSmokeProjectile(gu->usRandVector() * 10 +
				                                  float3(pos.x, pos.y-10, pos.z),
				                                  speed, int(50+gu->usRandFloat()*7),
				                                  this);
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
	if (model->textureType) {
		unitDrawer->SetS3OTeamColour(team);
	}
	model->DrawStatic();
	glPopMatrix();
}


int CFeature::ChunkNumber(float f)
{
	return (int) ceil(f * modInfo.reclaimMethod);
}


float CFeature::RemainingResource(float res) const
{
	// Gradual reclaim
	if (modInfo.reclaimMethod == 0) {
		return res * reclaimLeft;
	}

	// Old style - all reclaimed at the end
	if (modInfo.reclaimMethod == 1) {
		return res;
	}

	// Otherwise we are doing chunk reclaiming
	float chunkSize = res / modInfo.reclaimMethod; // resource/no_chunks
	float chunksLeft = ceil(reclaimLeft * modInfo.reclaimMethod);
	return chunkSize * chunksLeft;
}


float CFeature::RemainingMetal() const
{
	return RemainingResource(def->metal);
}


float CFeature::RemainingEnergy() const
{
	return RemainingResource(def->energy);
}
