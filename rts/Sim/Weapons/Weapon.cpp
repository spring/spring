// Weapon.cpp: implementation of the CWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Weapon.h"
#include "Sim/Units/Unit.h"
#include "Game/GameHelper.h"
#include "Game/Team.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/CobFile.h"
#include "myMath.h"
#include "LogOutput.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sync/SyncTracer.h"
#include "WeaponDefHandler.h"
#include "Sim/Projectiles/WeaponProjectile.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Game/Player.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/ModInfo.h"
#include "creg/STL_List.h"
#include "mmgr.h"
#include "float3.h"

CR_BIND_DERIVED(CWeapon, CObject, (NULL));

CR_REG_METADATA(CWeapon,(
	CR_MEMBER(owner),
	CR_MEMBER(range),
	CR_MEMBER(heightMod),
	CR_MEMBER(reloadTime),
	CR_MEMBER(reloadStatus),
	CR_MEMBER(salvoLeft),
	CR_MEMBER(salvoDelay),
	CR_MEMBER(salvoSize),
	CR_MEMBER(nextSalvo),
	CR_MEMBER(predict),
	CR_MEMBER(targetUnit),
	CR_MEMBER(accuracy),
	CR_MEMBER(projectileSpeed),
	CR_MEMBER(predictSpeedMod),
	CR_MEMBER(metalFireCost),
	CR_MEMBER(energyFireCost),
	CR_MEMBER(targetPos),
	CR_MEMBER(fireSoundId),
	CR_MEMBER(fireSoundVolume),
	CR_MEMBER(angleGood),
	CR_MEMBER(maxAngleDif),
	CR_MEMBER(wantedDir),
	CR_MEMBER(lastRequestedDir),
	CR_MEMBER(haveUserTarget),
	CR_MEMBER(subClassReady),
	CR_MEMBER(onlyForward),
	CR_MEMBER(weaponPos),
	CR_MEMBER(weaponMuzzlePos),
	CR_MEMBER(weaponDir),
	CR_MEMBER(lastRequest),
	CR_MEMBER(relWeaponPos),
	CR_MEMBER(relWeaponMuzzlePos),
	CR_MEMBER(muzzleFlareSize),
	CR_MEMBER(lastTargetRetry),
	CR_MEMBER(areaOfEffect),
	CR_MEMBER(badTargetCategory),
	CR_MEMBER(onlyTargetCategory),
	CR_MEMBER(incoming),
//	CR_MEMBER(weaponDef),
	CR_MEMBER(buildPercent),
	CR_MEMBER(numStockpiled),
	CR_MEMBER(numStockpileQued),
	CR_MEMBER(interceptTarget),
	CR_MEMBER(salvoError),
	CR_ENUM_MEMBER(targetType),
	CR_MEMBER(sprayangle),
	CR_MEMBER(useWeaponPosForAim),
	CR_MEMBER(errorVector),
	CR_MEMBER(errorVectorAdd),
	CR_MEMBER(lastErrorVectorUpdate),
	CR_MEMBER(slavedTo),
	CR_MEMBER(mainDir),
	CR_MEMBER(maxMainDirAngleDif),
	CR_MEMBER(hasCloseTarget),
	CR_MEMBER(avoidFriendly),
	CR_MEMBER(avoidFeature),
	CR_MEMBER(targetBorder),
	CR_MEMBER(cylinderTargetting),
	CR_MEMBER(minIntensity),
	CR_MEMBER(heightBoostFactor),
	CR_MEMBER(collisionFlags),
	CR_MEMBER(fuelUsage),
	CR_MEMBER(weaponNum),
	CR_RESERVED(64)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static void ScriptCallback(int retCode,void* p1,void* p2)
{
	if(retCode==1)
		((CWeapon*)p1)->ScriptReady();
}

CWeapon::CWeapon(CUnit* owner)
:	targetType(Target_None),
	owner(owner),
	range(1),
	heightMod(0),
	reloadTime(1),
	reloadStatus(0),
	salvoLeft(0),
	salvoDelay(0),
	salvoSize(1),
	nextSalvo(0),
	predict(0),
	targetUnit(0),
	accuracy(0),
	projectileSpeed(1),
	predictSpeedMod(1),
	metalFireCost(0),
	energyFireCost(0),
	targetPos(1,1,1),
	fireSoundId(0),
	fireSoundVolume(0),
	angleGood(false),
	maxAngleDif(0),
	wantedDir(0,1,0),
	lastRequestedDir(0,-1,0),
	haveUserTarget(false),
	subClassReady(true),
	onlyForward(false),
	weaponPos(0,0,0),
	weaponMuzzlePos(0,0,0),
	weaponDir(0,0,0),
	lastRequest(0),
	relWeaponPos(0,1,0),
	relWeaponMuzzlePos(0,1,0),
	muzzleFlareSize(1),
	lastTargetRetry(-100),
	areaOfEffect(1),
	badTargetCategory(0),
	onlyTargetCategory(0xffffffff),
	weaponDef(0),
	buildPercent(0),
	numStockpiled(0),
	numStockpileQued(0),
	interceptTarget(0),
	salvoError(0,0,0),
	sprayangle(0),
	useWeaponPosForAim(0),
	errorVector(ZeroVector),
	errorVectorAdd(ZeroVector),
	lastErrorVectorUpdate(0),
	slavedTo(0),
	mainDir(0,0,1),
	maxMainDirAngleDif(-1),
	hasCloseTarget(false),
	avoidFriendly(true),
	avoidFeature(true),
	targetBorder(0.f),
	cylinderTargetting(0.f),
	minIntensity(0.f),
	heightBoostFactor(-1.f),
	collisionFlags(0),
	fuelUsage(0)
{
}

CWeapon::~CWeapon()
{
	if(weaponDef->interceptor)
		interceptHandler.RemoveInterceptorWeapon(this);
}

void CWeapon::Update()
{
	if(hasCloseTarget){
		std::vector<int> args;
		args.push_back(0);
		if(useWeaponPosForAim){ //if we couldn't get a line of fire from the muzzle try if we can get it from the aim piece
			owner->cob->Call(COBFN_QueryPrimary+weaponNum,args);
		} else {
			owner->cob->Call(COBFN_AimFromPrimary+weaponNum,args);
		}
		relWeaponMuzzlePos=owner->localmodel->GetPiecePos(args[0]);

		owner->cob->Call(COBFN_AimFromPrimary+weaponNum,args);
		relWeaponPos=owner->localmodel->GetPiecePos(args[0]);
	}

	if(targetType==Target_Unit){
		if(lastErrorVectorUpdate<gs->frameNum-16){
			float3 newErrorVector(gs->randVector());
			errorVectorAdd=(newErrorVector-errorVector)*(1.0f/16.0f);
			lastErrorVectorUpdate=gs->frameNum;
		}
		errorVector+=errorVectorAdd;
		if (predict > 50000) {
			/* to prevent runaway prediction (happens sometimes when a missile is moving *away* from it's target), we may need to disable missiles in case they fly around too long */
			predict = 50000;
		}
		if(weaponDef->selfExplode){	//assumes that only flakker like units that need to hit aircrafts has this,change to a separate tag later
			targetPos=helper->GetUnitErrorPos(targetUnit,owner->allyteam)+targetUnit->speed*(0.5f+predictSpeedMod*0.5f)*predict;
		} else {
			targetPos=helper->GetUnitErrorPos(targetUnit,owner->allyteam)+targetUnit->speed*predictSpeedMod*predict;
		}
		targetPos+=errorVector*(weaponDef->targetMoveError*30*targetUnit->speed.Length()*(1.0f-owner->limExperience));
		float appHeight=ground->GetApproximateHeight(targetPos.x,targetPos.z)+2;
		if(targetPos.y < appHeight)
			targetPos.y=appHeight;

		if(!weaponDef->waterweapon && targetPos.y<1)
			targetPos.y=1;
	}

	if(weaponDef->interceptor) {
		CheckIntercept();
	}
	if(targetType!=Target_None){
		if(onlyForward){
			float3 goaldir=targetPos-owner->pos;
			goaldir.Normalize();
			angleGood=owner->frontdir.dot(goaldir) > maxAngleDif;
		} else if(lastRequestedDir.dot(wantedDir)<maxAngleDif || lastRequest+15<gs->frameNum){
			angleGood=false;
			lastRequestedDir=wantedDir;
			lastRequest=gs->frameNum;

			short int heading=GetHeadingFromVector(wantedDir.x,wantedDir.z);
			short int pitch=(short int) (asin(wantedDir.dot(owner->updir))*(32768/PI));
			std::vector<int> args;
			args.push_back(short(heading - owner->heading));
			args.push_back(pitch);
			owner->cob->Call(COBFN_AimPrimary+weaponNum,args,ScriptCallback,this,0);
		}
	}
	if(weaponDef->stockpile && numStockpileQued){
		float p=1.0f/reloadTime;
		if(gs->Team(owner->team)->metal>=metalFireCost*p && gs->Team(owner->team)->energy>=energyFireCost*p){
			owner->UseEnergy(energyFireCost*p);
			owner->UseMetal(metalFireCost*p);
			buildPercent+=p;
		} else {
			// update the energy and metal required counts
			gs->Team(owner->team)->energyPull += energyFireCost*p;
			gs->Team(owner->team)->metalPull += metalFireCost*p;
		}
		if(buildPercent>=1){
			buildPercent=0;
			numStockpileQued--;
			numStockpiled++;
			owner->commandAI->StockpileChanged(this);
		}
	}

	if(salvoLeft==0
#ifdef DIRECT_CONTROL_ALLOWED
	&& (!owner->directControl || owner->directControl->mouse1 || owner->directControl->mouse2)
#endif
	&& targetType!=Target_None
	&& angleGood
	&& subClassReady
	&& reloadStatus<=gs->frameNum
	&& (!weaponDef->stockpile || numStockpiled)
	&& (weaponDef->fireSubmersed || weaponMuzzlePos.y>0)
	&& (owner->unitDef->maxFuel==0 || owner->currentFuel > 0)
	){
		if ((weaponDef->stockpile || (gs->Team(owner->team)->metal>=metalFireCost && gs->Team(owner->team)->energy>=energyFireCost))) {
			std::vector<int> args;
			args.push_back(0);
			owner->cob->Call(COBFN_QueryPrimary+weaponNum,args);
			owner->localmodel->GetEmitDirPos(args[0], relWeaponMuzzlePos, weaponDir);
			weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
			useWeaponPosForAim=reloadTime/16+8;
			weaponDir = owner->frontdir * weaponDir.z + owner->updir * weaponDir.y + owner->rightdir * weaponDir.x;
			weaponDir.Normalize();

			if(TryTarget(targetPos,haveUserTarget,targetUnit)){
				if(weaponDef->stockpile){
					numStockpiled--;
					owner->commandAI->StockpileChanged(this);
				} else {
					owner->UseEnergy(energyFireCost);
					owner->UseMetal(metalFireCost);
					owner->currentFuel = max(0.0f, owner->currentFuel - fuelUsage);
				}
				if(weaponDef->stockpile)
					reloadStatus=gs->frameNum+60;
				else
					reloadStatus=gs->frameNum+(int)(reloadTime/owner->reloadSpeed);

				salvoLeft=salvoSize;
				nextSalvo=gs->frameNum;
				salvoError=gs->randVector()*(owner->isMoving?weaponDef->movingAccuracy:accuracy);
				if(targetType==Target_Pos || (targetType==Target_Unit && !(targetUnit->losStatus[owner->allyteam] & LOS_INLOS)))		//area firing stuff is to effective at radar firing...
					salvoError*=1.3f;

				owner->lastMuzzleFlameSize=muzzleFlareSize;
				owner->lastMuzzleFlameDir=wantedDir;
				owner->cob->Call(COBFN_FirePrimary+weaponNum);
			}
		} else {
			if (TryTarget(targetPos,haveUserTarget,targetUnit) && !weaponDef->stockpile) {
				// update the energy and metal required counts
				const int minPeriod = max(1, (int)(reloadTime / owner->reloadSpeed));
				const float averageFactor = 1.0f / (float)minPeriod;
				gs->Team(owner->team)->energyPull += averageFactor * energyFireCost;
				gs->Team(owner->team)->metalPull += averageFactor * metalFireCost;
			}
		}
	}
	if(salvoLeft && nextSalvo<=gs->frameNum ){
		salvoLeft--;
		nextSalvo=gs->frameNum+salvoDelay;
		owner->lastFireWeapon=gs->frameNum;

		int projectiles = weaponDef->projectilespershot;

		while(projectiles > 0) {
			--projectiles;

			// add to the commandShotCount if this is the last salvo,
			// and it is being directed towards the current target
			// (helps when deciding if a queued ground attack order has been completed)
			if ((salvoLeft == 0) && (owner->commandShotCount >= 0) &&
			    ((targetType == Target_Pos) && (targetPos == owner->userAttackPos)) ||
					((targetType == Target_Unit) && (targetUnit == owner->userTarget))) {
				owner->commandShotCount++;
			}

			std::vector<int> args;
			args.push_back(0);

			owner->cob->Call(COBFN_Shot+weaponNum,0);

			owner->cob->Call(COBFN_AimFromPrimary+weaponNum,args);
			relWeaponPos=owner->localmodel->GetPiecePos(args[0]);

			owner->cob->Call(/*COBFN_AimFromPrimary+weaponNum*/COBFN_QueryPrimary+weaponNum/**/,args);
			owner->localmodel->GetEmitDirPos(args[0], relWeaponMuzzlePos, weaponDir);

			weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;

			weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
			weaponDir = owner->frontdir * weaponDir.z + owner->updir * weaponDir.y + owner->rightdir * weaponDir.x;
			weaponDir.Normalize();

	//		logOutput.Print("RelPosFire %f %f %f",relWeaponPos.x,relWeaponPos.y,relWeaponPos.z);

			if (owner->unitDef->decloakOnFire && (owner->scriptCloak <= 2)) {
				owner->isCloaked = false;
				owner->curCloakTimeout = gs->frameNum + owner->cloakTimeout;
			}

			Fire();
		}

		//Rock the unit in the direction of the fireing
		float3 rockDir = wantedDir;
		rockDir.y = 0;
		rockDir = -rockDir.Normalize();
		std::vector<int> rockAngles;
		rockAngles.push_back((int)(500 * rockDir.z));
		rockAngles.push_back((int)(500 * rockDir.x));
		owner->cob->Call(COBFN_RockUnit,  rockAngles);

		owner->commandAI->WeaponFired(this);

		if(salvoLeft==0){
			owner->cob->Call(COBFN_EndBurst+weaponNum);
		}
#ifdef TRACE_SYNC
	tracefile << "Weapon fire: ";
	tracefile << weaponPos.x << " " << weaponPos.y << " " << weaponPos.z << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z << "\n";
#endif
	}
}

bool CWeapon::AttackGround(float3 pos, bool userTarget)
{
	if((!userTarget && weaponDef->noAutoTarget))
		return false;
	if(weaponDef->interceptor || weaponDef->onlyTargetCategory!=0xffffffff || !weaponDef->canAttackGround)
		return false;

	if(!weaponDef->waterweapon && pos.y<1)
		pos.y=1;
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
	if(weaponMuzzlePos.y<ground->GetHeight2(weaponMuzzlePos.x,weaponMuzzlePos.z))
		weaponMuzzlePos=owner->pos+UpVector*10;		//hope that we are underground because we are a popup weapon and will come above ground later

	if(!TryTarget(pos,userTarget,0))
		return false;
	if(targetUnit){
		DeleteDeathDependence(targetUnit);
		targetUnit=0;
	}
	haveUserTarget=userTarget;
	targetType=Target_Pos;
	targetPos=pos;
	return true;
}

bool CWeapon::AttackUnit(CUnit *unit, bool userTarget)
{
	if((!userTarget && weaponDef->noAutoTarget))
		return false;
	if(weaponDef->interceptor)
		return false;

	weaponPos= owner->pos + owner->frontdir * relWeaponPos.z
		+ owner->updir * relWeaponPos.y + owner->rightdir * relWeaponPos.x;
	weaponMuzzlePos= owner->pos + owner->frontdir * relWeaponMuzzlePos.z
		+ owner->updir * relWeaponMuzzlePos.y + owner->rightdir * relWeaponMuzzlePos.x;
	if(weaponMuzzlePos.y < ground->GetHeight2(weaponMuzzlePos.x, weaponMuzzlePos.z))
		weaponMuzzlePos = owner->pos + UpVector * 10;
	//hope that we are underground because we are a popup weapon and will come above ground later

	if(!unit){
		if(targetType!=Target_Unit)	//make the unit be more likely to keep the current target if user start to move it
			targetType=Target_None;
		haveUserTarget=false;
		return false;
	}
	float3 tempTargetPos(helper->GetUnitErrorPos(unit,owner->allyteam));
	tempTargetPos+=errorVector*(weaponDef->targetMoveError*30*unit->speed.Length()*(1.0f-owner->limExperience));
	float appHeight=ground->GetApproximateHeight(tempTargetPos.x,tempTargetPos.z)+2;
	if(tempTargetPos.y < appHeight)
		tempTargetPos.y=appHeight;

	if(!TryTarget(tempTargetPos,userTarget,unit))
		return false;

	if(targetUnit){
		DeleteDeathDependence(targetUnit);
		targetUnit=0;
	}
	haveUserTarget=userTarget;
	targetType=Target_Unit;
	targetUnit=unit;
	targetPos=tempTargetPos;
	AddDeathDependence(targetUnit);
	return true;
}

void CWeapon::HoldFire()
{
	if(targetUnit){
		DeleteDeathDependence(targetUnit);
		targetUnit=0;
	}
	targetType=Target_None;
	haveUserTarget=false;
}

void CWeapon::SlowUpdate()
{
	SlowUpdate(false);
}

void CWeapon::SlowUpdate(bool noAutoTargetOverride)
{
#ifdef TRACE_SYNC
	tracefile << "Weapon slow update: ";
	tracefile << owner->id << " " << weaponNum <<  "\n";
#endif
	std::vector<int> args;
	args.push_back(0);
	if(useWeaponPosForAim){ //If we can't get a line of fire from the muzzle try the aim piece instead since the weapon may just be turned in a wrong way
		owner->cob->Call(COBFN_QueryPrimary+weaponNum,args);
		if(useWeaponPosForAim>1)
			useWeaponPosForAim--;
	} else {
		owner->cob->Call(COBFN_AimFromPrimary+weaponNum,args);
	}
	relWeaponMuzzlePos=owner->localmodel->GetPiecePos(args[0]);
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;

	owner->cob->Call(COBFN_AimFromPrimary+weaponNum,args);
	relWeaponPos=owner->localmodel->GetPiecePos(args[0]);
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;

	if(weaponMuzzlePos.y<ground->GetHeight2(weaponMuzzlePos.x,weaponMuzzlePos.z))
		weaponMuzzlePos=owner->pos+UpVector*10;		//hope that we are underground because we are a popup weapon and will come above ground later

	predictSpeedMod=1+(gs->randFloat()-0.5f)*2*(1-owner->limExperience);

	if((targetPos-weaponPos).SqLength() < relWeaponPos.SqLength()*16)
		hasCloseTarget=true;
	else
		hasCloseTarget=false;

	if(targetType!=Target_None && !TryTarget(targetPos,haveUserTarget,targetUnit)){
		HoldFire();
	}
	if(targetType==Target_Unit && targetUnit->isCloaked && !(targetUnit->losStatus[owner->allyteam] & (LOS_INLOS | LOS_INRADAR)))
		HoldFire();

	if(slavedTo){	//use targets from the thing we are slaved to
		if(targetUnit){
			DeleteDeathDependence(targetUnit);
			targetUnit=0;
		}
		targetType=Target_None;
		if(slavedTo->targetType==Target_Unit){
			float3 tp=helper->GetUnitErrorPos(slavedTo->targetUnit,owner->allyteam);
			tp+=errorVector*(weaponDef->targetMoveError*30*slavedTo->targetUnit->speed.Length()*(1.0f-owner->limExperience));
			if(TryTarget(tp,false,slavedTo->targetUnit)){
				targetType=Target_Unit;
				targetUnit=slavedTo->targetUnit;
				targetPos=tp;
				AddDeathDependence(targetUnit);
			}
		} else if(slavedTo->targetType==Target_Pos){
			if(TryTarget(slavedTo->targetPos,false,0)){
				targetType=Target_Pos;
				targetPos=slavedTo->targetPos;
			}
		}
		return;
	}

	if (!weaponDef->noAutoTarget && !noAutoTargetOverride) {
		if(owner->fireState==2 && !haveUserTarget && (targetType==Target_None || (targetType==Target_Unit && (targetUnit->category & badTargetCategory)) || gs->frameNum>lastTargetRetry+65)){
			lastTargetRetry=gs->frameNum;
			std::map<float,CUnit*> targets;
			helper->GenerateTargets(this,targetUnit,targets);

			for(std::map<float,CUnit*>::iterator ti=targets.begin();ti!=targets.end();++ti){
				if(targetUnit && (ti->second->category & badTargetCategory))
					continue;
				float3 tp(ti->second->midPos);
				tp+=errorVector*(weaponDef->targetMoveError*30*ti->second->speed.Length()*(1.0f-owner->limExperience));
				float appHeight=ground->GetApproximateHeight(tp.x,tp.z)+2;
				if(tp.y < appHeight)
					tp.y=appHeight;

				if(TryTarget(tp,false,ti->second)){
					if(targetUnit){
						DeleteDeathDependence(targetUnit);
					}
					targetType=Target_Unit;
					targetUnit=ti->second;
					targetPos=tp;
					AddDeathDependence(targetUnit);
					break;
				}
			}
		}
	}
	if(targetType!=Target_None){
		owner->haveTarget=true;
		if(haveUserTarget)
			owner->haveUserTarget=true;
	} else {	//if we cant target anything try switching aim point
		if(useWeaponPosForAim && useWeaponPosForAim==1){
			useWeaponPosForAim=0;
		} else {
			useWeaponPosForAim=1;
		}
	}
}

void CWeapon::DependentDied(CObject *o)
{
	if(o==targetUnit){
		targetUnit=0;
		if(targetType==Target_Unit){
			targetType=Target_None;
			haveUserTarget=false;
		}
	}
	if(weaponDef->interceptor){
		incoming.remove((CWeaponProjectile*)o);
	}
}

bool CWeapon::TryTarget(const float3 &pos,bool userTarget,CUnit* unit)
{
	if(unit && !(onlyTargetCategory&unit->category))
		return false;

	if(unit && ((unit->isDead && (modInfo->fireAtKilled==0)) || (unit->crashing && (modInfo->fireAtCrashing==0))))
		return false;

	if(weaponDef->stockpile && !numStockpiled)
		return false;

	float3 dif=pos-weaponMuzzlePos;
	float heightDiff; // negative when target below owner

	if (targetBorder != 0 && unit) {
		float3 diff(dif);
		diff.Normalize();
		// weapon inside target sphere
		if (dif.SqLength() < unit->sqRadius*targetBorder*targetBorder) {
			dif -= diff*(dif.Length() - 10); // a hack
			//logOutput << "inside\n";
		} else {
			dif -= diff*(unit->radius*targetBorder);
			//logOutput << "outside\n";
		}
		//geometricObjects->AddLine(weaponMuzzlePos, weaponMuzzlePos+dif, 3, 0, 16);
		heightDiff = (weaponPos.y + dif.y) - owner->pos.y;
	} else {
		heightDiff = pos.y - owner->pos.y;
	}

	float r;
	if (!unit || cylinderTargetting < 0.01) {
		r=GetRange2D(heightDiff*heightMod);
	} else {
		if (cylinderTargetting * unit->radius > fabs(heightDiff)*heightMod) {
			r = GetRange2D(0);
		} else {
			r = 0;
		}
	}

	if(dif.SqLength2D()>=r*r)
		return false;

	if(maxMainDirAngleDif>-0.999f){
		dif.Normalize();
		float3 modMainDir=owner->frontdir*mainDir.z+owner->rightdir*mainDir.x+owner->updir*mainDir.y;

//		geometricObjects->AddLine(weaponPos,weaponPos+modMainDir*50,3,0,16);
		if(modMainDir.dot(dif)<maxMainDirAngleDif)
			return false;
	}
	return true;
}

bool CWeapon::TryTarget(CUnit* unit, bool userTarget){
	float3 tempTargetPos(helper->GetUnitErrorPos(unit,owner->allyteam));
	tempTargetPos+=errorVector*(weaponDef->targetMoveError*30*unit->speed.Length()*(1.0f-owner->limExperience));
	float appHeight=ground->GetApproximateHeight(tempTargetPos.x,tempTargetPos.z)+2;
	if(tempTargetPos.y < appHeight){
		tempTargetPos.y=appHeight;
	}
	return TryTarget(tempTargetPos,userTarget,unit);
}

bool CWeapon::TryTargetRotate(CUnit* unit, bool userTarget){
	float3 tempTargetPos(helper->GetUnitErrorPos(unit,owner->allyteam));
	tempTargetPos+=errorVector*(weaponDef->targetMoveError*30*unit->speed.Length()*(1.0f-owner->limExperience));
	float appHeight=ground->GetApproximateHeight(tempTargetPos.x,tempTargetPos.z)+2;
	if(tempTargetPos.y < appHeight){
		tempTargetPos.y=appHeight;
	}
	float3 tempfrontdir(owner->frontdir);
	float3 temprightdir(owner->rightdir);
	short tempHeadding = owner->heading;
	short weaponHeadding = GetHeadingFromVector(mainDir.x, mainDir.z);
	short enemyHeadding = GetHeadingFromVector(
		tempTargetPos.x - weaponPos.x, tempTargetPos.z - weaponPos.z);
	owner->heading = enemyHeadding - weaponHeadding;
	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->rightdir = owner->frontdir.cross(owner->updir);
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
	bool val = TryTarget(tempTargetPos,userTarget,unit);
	owner->frontdir = tempfrontdir;
	owner->rightdir = temprightdir;
	owner->heading = tempHeadding;
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
	return val;
}

bool CWeapon::TryTargetRotate(float3 pos, bool userTarget) {
	if (!userTarget && weaponDef->noAutoTarget) {
		return false;
	}
	if(weaponDef->interceptor || weaponDef->onlyTargetCategory!=0xffffffff
			|| !weaponDef->canAttackGround){
		return false;
	}

	if(!weaponDef->waterweapon && pos.y<1){
		pos.y=1;
	}
	float3 tempfrontdir(owner->frontdir);
	float3 temprightdir(owner->rightdir);
	short tempHeadding = owner->heading;
	short weaponHeadding = GetHeadingFromVector(mainDir.x, mainDir.z);
	short enemyHeadding = GetHeadingFromVector(
		pos.x - weaponPos.x, pos.z - weaponPos.z);
	owner->heading = enemyHeadding - weaponHeadding;
	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->rightdir = owner->frontdir.cross(owner->updir);
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
	bool val = TryTarget(pos, userTarget, 0);
	owner->frontdir = tempfrontdir;
	owner->rightdir = temprightdir;
	owner->heading = tempHeadding;
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
	return val;
}

void CWeapon::Init(void)
{
	std::vector<int> args;
	args.push_back(0);
	owner->cob->Call(COBFN_AimFromPrimary+weaponNum,args);
	relWeaponPos=owner->localmodel->GetPiecePos(args[0]);
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;

	owner->cob->Call(COBFN_QueryPrimary+weaponNum,args);
	relWeaponMuzzlePos=owner->localmodel->GetPiecePos(args[0]);
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
//	logOutput.Print("RelPos %f %f %f",relWeaponPos.x,relWeaponPos.y,relWeaponPos.z);

	if (range > owner->maxRange) {
		owner->maxRange = range;
	}

	muzzleFlareSize = min(areaOfEffect*0.2f,min(1500.f,weaponDef->damages[0])*0.003f);

	if (weaponDef->interceptor)
		interceptHandler.AddInterceptorWeapon(this);

	if(weaponDef->stockpile){
		owner->stockpileWeapon = this;
		owner->commandAI->AddStockpileWeapon(this);
	}

	if (weaponDef->isShield) {
		if ((owner->shieldWeapon == NULL) ||
		    (owner->shieldWeapon->weaponDef->shieldRadius < weaponDef->shieldRadius)) {
			owner->shieldWeapon = this;
		}
	}
}

void CWeapon::ScriptReady(void)
{
	angleGood=true;
}

void CWeapon::CheckIntercept(void)
{
	targetType=Target_None;

	for(std::list<CWeaponProjectile*>::iterator pi=incoming.begin();pi!=incoming.end();++pi){
		if((*pi)->targeted)
			continue;
		targetType=Target_Intercept;
		interceptTarget=*pi;
		targetPos=(*pi)->pos;

		break;
	}
}

float CWeapon::GetRange2D(float yDiff) const
{
	float root1 = range*range - yDiff*yDiff;
	if(root1 < 0){
		return 0;
	} else {
		return sqrt(root1);
	}
}
