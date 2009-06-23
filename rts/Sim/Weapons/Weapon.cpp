// Weapon.cpp: implementation of the CWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "creg/STL_List.h"
#include "float3.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "myMath.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "Sync/SyncTracer.h"
#include "Sound/AudioChannel.h"
#include "EventHandler.h"
#include "WeaponDefHandler.h"
#include "Weapon.h"
#include "mmgr.h"

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
	CR_MEMBER(cobHasBlockShot),
	CR_MEMBER(hasTargetWeight),
	CR_MEMBER(angleGood),
	CR_MEMBER(avoidTarget),
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
	CR_MEMBER(stockpileTime),
	CR_MEMBER(buildPercent),
	CR_MEMBER(numStockpiled),
	CR_MEMBER(numStockpileQued),
	CR_MEMBER(interceptTarget),
	CR_MEMBER(salvoError),
	CR_ENUM_MEMBER(targetType),
	CR_MEMBER(sprayAngle),
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
	CR_MEMBER(avoidNeutral),
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

CWeapon::CWeapon(CUnit* owner):
	owner(owner),
	weaponDef(0),
	haveUserTarget(false),
	areaOfEffect(1),
	relWeaponPos(0,1,0),
	weaponPos(0,0,0),
	relWeaponMuzzlePos(0,1,0),
	weaponMuzzlePos(0,0,0),
	weaponDir(0,0,0),
	muzzleFlareSize(1),
	useWeaponPosForAim(0),
	hasCloseTarget(false),
	reloadTime(1),
	reloadStatus(0),
	range(1),
	heightMod(0),
	projectileSpeed(1),
	accuracy(0),
	sprayAngle(0),
	salvoDelay(0),
	salvoSize(1),
	nextSalvo(0),
	salvoLeft(0),
	salvoError(0,0,0),
	targetType(Target_None),
	targetUnit(0),
	targetPos(1,1,1),
	lastTargetRetry(-100),
	predict(0),
	predictSpeedMod(1),
	metalFireCost(0),
	energyFireCost(0),
	fireSoundId(0),
	fireSoundVolume(0),
	cobHasBlockShot(false),
	hasTargetWeight(false),
	angleGood(false),
	avoidTarget(false),
	subClassReady(true),
	onlyForward(false),
	maxAngleDif(0),
	wantedDir(0,1,0),
	lastRequestedDir(0,-1,0),
	lastRequest(0),
	badTargetCategory(0),
	onlyTargetCategory(0xffffffff),
	interceptTarget(0),
	stockpileTime(1),
	buildPercent(0),
	numStockpiled(0),
	numStockpileQued(0),
	errorVector(ZeroVector),
	errorVectorAdd(ZeroVector),
	lastErrorVectorUpdate(0),
	slavedTo(0),
	mainDir(0,0,1),
	maxMainDirAngleDif(-1),
	avoidFriendly(true),
	avoidFeature(true),
	avoidNeutral(true),
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
	if (weaponDef->interceptor)
		interceptHandler.RemoveInterceptorWeapon(this);
}


void CWeapon::SetWeaponNum(int num)
{
	weaponNum = num;

	const int n = COBFN_Weapon_Funcs * weaponNum;
	cobHasBlockShot = owner->script->HasFunction(COBFN_BlockShot + n);
	hasTargetWeight = owner->script->HasFunction(COBFN_TargetWeight + n);
}


inline bool CWeapon::CobBlockShot(const CUnit* targetUnit)
{
	if (!cobHasBlockShot) {
		return false;
	}

	return owner->script->BlockShot(weaponNum, targetUnit, haveUserTarget);
}


float CWeapon::TargetWeight(const CUnit* targetUnit) const
{
	return owner->script->TargetWeight(weaponNum, targetUnit);
}



static inline bool isBeingServicedOnPad(CUnit* u)
{
	AAirMoveType *a = dynamic_cast<AAirMoveType*>(u->moveType);
	return a && a->padStatus != 0;
}

void CWeapon::Update()
{
	if (hasCloseTarget) {
		int piece;
		// if we couldn't get a line of fire from the muzzle try if we can get it from the aim piece
		if (useWeaponPosForAim) {
			piece = owner->script->QueryWeapon(weaponNum);
		} else {
			piece = owner->script->AimFromWeapon(weaponNum);
		}
		relWeaponMuzzlePos = owner->script->GetPiecePos(piece);

		//FIXME: this might be potential speedup?
		// (AimFromWeapon may have been called already 3 lines ago)
		//if (useWeaponPosForAim)
		piece = owner->script->AimFromWeapon(weaponNum);
		relWeaponPos = owner->script->GetPiecePos(piece);
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

		float3 lead = targetUnit->speed * (weaponDef->predictBoost+predictSpeedMod * (1.0f - weaponDef->predictBoost)) * predict;

		if (weaponDef->leadLimit >= 0.0f && lead.SqLength() > Square(weaponDef->leadLimit + weaponDef->leadBonus * owner->experience)) {
			lead *= (weaponDef->leadLimit + weaponDef->leadBonus*owner->experience) / (lead.Length() + 0.01f);
		}

		targetPos = helper->GetUnitErrorPos(targetUnit, owner->allyteam) + lead;
		targetPos += errorVector * (weaponDef->targetMoveError * 30 * targetUnit->speed.Length() * (1.0f - owner->limExperience));
		float appHeight = ground->GetApproximateHeight(targetPos.x, targetPos.z) + 2;

		if (targetPos.y < appHeight)
			targetPos.y = appHeight;

		if (!weaponDef->waterweapon && targetPos.y < 1.0f)
			targetPos.y = 1.0f;
	}

	if (weaponDef->interceptor) {
		CheckIntercept();
	}
	if (targetType != Target_None){
		if (onlyForward) {
			float3 goaldir = targetPos - owner->pos;
			goaldir.Normalize();
			angleGood = (owner->frontdir.dot(goaldir) > maxAngleDif);
		} else if (lastRequestedDir.dot(wantedDir) < maxAngleDif || lastRequest + 15 < gs->frameNum) {
			angleGood=false;
			lastRequestedDir=wantedDir;
			lastRequest=gs->frameNum;

			const float heading = GetHeadingFromVectorF(wantedDir.x, wantedDir.z);
			const float pitch = asin(wantedDir.dot(owner->updir));
			// for COB, this sets anglegood to return value of aim script when it finished,
			// for Lua, there exists a callout to set the anglegood member.
			// FIXME: convert CSolidObject::heading to radians too.
			owner->script->AimWeapon(weaponNum, ClampRad(heading - owner->heading * TAANG2RAD), pitch);
		}
	}
	if(weaponDef->stockpile && numStockpileQued){
		float p=1.0f/stockpileTime;
		if(teamHandler->Team(owner->team)->metal>=metalFireCost*p && teamHandler->Team(owner->team)->energy>=energyFireCost*p){
			owner->UseEnergy(energyFireCost*p);
			owner->UseMetal(metalFireCost*p);
			buildPercent+=p;
		} else {
			// update the energy and metal required counts
			teamHandler->Team(owner->team)->energyPull += energyFireCost*p;
			teamHandler->Team(owner->team)->metalPull += metalFireCost*p;
		}
		if(buildPercent>=1){
			const int oldCount = numStockpiled;
			buildPercent=0;
			numStockpileQued--;
			numStockpiled++;
			owner->commandAI->StockpileChanged(this);
			eventHandler.StockpileChanged(owner, this, oldCount);
		}
	}

	if ((salvoLeft == 0)
	    && (!owner->directControl || owner->directControl->mouse1
	                              || owner->directControl->mouse2)
	    && (targetType != Target_None)
	    && angleGood
	    && subClassReady
	    && (reloadStatus <= gs->frameNum)
	    && (!weaponDef->stockpile || numStockpiled)
	    && (weaponDef->fireSubmersed || (weaponMuzzlePos.y > 0))
	    && ((((owner->unitDef->maxFuel == 0) || (owner->currentFuel > 0) || (fuelUsage == 0)) &&
	       !isBeingServicedOnPad(owner)))
	   )
	{
		if ((weaponDef->stockpile ||
		     (teamHandler->Team(owner->team)->metal >= metalFireCost &&
		      teamHandler->Team(owner->team)->energy >= energyFireCost)))
		{
			const int piece = owner->script->QueryWeapon(weaponNum);
			owner->script->GetEmitDirPos(piece, relWeaponMuzzlePos, weaponDir);
			weaponMuzzlePos = owner->pos + owner->frontdir * relWeaponMuzzlePos.z +
			                               owner->updir    * relWeaponMuzzlePos.y +
			                               owner->rightdir * relWeaponMuzzlePos.x;
			useWeaponPosForAim = reloadTime / 16 + 8;
			weaponDir = owner->frontdir * weaponDir.z +
			            owner->updir    * weaponDir.y +
			            owner->rightdir * weaponDir.x;
			weaponDir.Normalize();

			if (TryTarget(targetPos,haveUserTarget,targetUnit) && !CobBlockShot(targetUnit)) {
				if(weaponDef->stockpile){
					const int oldCount = numStockpiled;
					numStockpiled--;
					owner->commandAI->StockpileChanged(this);
					eventHandler.StockpileChanged(owner, this, oldCount);
				} else {
					owner->UseEnergy(energyFireCost);
					owner->UseMetal(metalFireCost);
					owner->currentFuel = std::max(0.0f, owner->currentFuel - fuelUsage);
				}
				reloadStatus=gs->frameNum+(int)(reloadTime/owner->reloadSpeed);

				salvoLeft=salvoSize;
				nextSalvo=gs->frameNum;
				salvoError=gs->randVector()*(owner->isMoving?weaponDef->movingAccuracy:accuracy);
				if(targetType==Target_Pos || (targetType==Target_Unit && !(targetUnit->losStatus[owner->allyteam] & LOS_INLOS)))		//area firing stuff is to effective at radar firing...
					salvoError*=1.3f;

				owner->lastMuzzleFlameSize=muzzleFlareSize;
				owner->lastMuzzleFlameDir=wantedDir;
				owner->script->FireWeapon(weaponNum);
			}
		} else {
			// FIXME  -- never reached?
			if (TryTarget(targetPos,haveUserTarget,targetUnit) && !weaponDef->stockpile) {
				// update the energy and metal required counts
				const int minPeriod = std::max(1, (int)(reloadTime / owner->reloadSpeed));
				const float averageFactor = 1.0f / (float)minPeriod;
				teamHandler->Team(owner->team)->energyPull += averageFactor * energyFireCost;
				teamHandler->Team(owner->team)->metalPull += averageFactor * metalFireCost;
			}
		}
	}
	if(salvoLeft && nextSalvo<=gs->frameNum ){
		salvoLeft--;
		nextSalvo=gs->frameNum+salvoDelay;
		owner->lastFireWeapon=gs->frameNum;

		int projectiles = projectilesPerShot;

		while(projectiles > 0) {
			--projectiles;

			// add to the commandShotCount if this is the last salvo,
			// and it is being directed towards the current target
			// (helps when deciding if a queued ground attack order has been completed)
			if (((salvoLeft == 0) && (owner->commandShotCount >= 0) &&
			    ((targetType == Target_Pos) && (targetPos == owner->userAttackPos))) ||
					((targetType == Target_Unit) && (targetUnit == owner->userTarget))) {
				owner->commandShotCount++;
			}

			owner->script->Shot(weaponNum);

			int piece = owner->script->AimFromWeapon(weaponNum);
			relWeaponPos = owner->script->GetPiecePos(piece);

			piece = owner->script->/*AimFromWeapon*/QueryWeapon(weaponNum);
			owner->script->GetEmitDirPos(piece, relWeaponMuzzlePos, weaponDir);

			weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;

			weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
			weaponDir = owner->frontdir * weaponDir.z + owner->updir * weaponDir.y + owner->rightdir * weaponDir.x;
			weaponDir.Normalize();

	//		logOutput.Print("RelPosFire %f %f %f",relWeaponPos.x,relWeaponPos.y,relWeaponPos.z);

			if (owner->unitDef->decloakOnFire && (owner->scriptCloak <= 2)) {
				if (owner->isCloaked) {
					owner->isCloaked = false;
					eventHandler.UnitDecloaked(owner);
				}
				owner->curCloakTimeout = gs->frameNum + owner->cloakTimeout;
			}

			Fire();
		}

		//Rock the unit in the direction of the fireing
		if (owner->script->HasFunction(COBFN_RockUnit)) {
			float3 rockDir = wantedDir;
			rockDir.y = 0;
			rockDir = -rockDir.Normalize();
			owner->script->RockUnit(rockDir);
		}

		owner->commandAI->WeaponFired(this);

		if(salvoLeft==0){
			owner->script->EndBurst(weaponNum);
		}
#ifdef TRACE_SYNC
	tracefile << "Weapon fire: ";
	tracefile << weaponPos.x << " " << weaponPos.y << " " << weaponPos.z << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z << "\n";
#endif
	}
}

bool CWeapon::AttackGround(float3 pos, bool userTarget)
{
	if (!userTarget && weaponDef->noAutoTarget) {
		return false;
	}
	if (weaponDef->interceptor || !weaponDef->canAttackGround ||
	    (weaponDef->onlyTargetCategory != 0xffffffff)) {
		return false;
	}

	if (!weaponDef->waterweapon && (pos.y < 1.0f)) {
		pos.y = 1.0f;
	}
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
	avoidTarget=false;
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


inline bool CWeapon::ShouldCheckForNewTarget() const
{
	if (weaponDef->noAutoTarget) { return false; }
	if (owner->fireState < 2)    { return false; }
	if (haveUserTarget)          { return false; }

	if (targetType == Target_None) { return true; }

	if (avoidTarget)             { return true; }

	if (targetType == Target_Unit) {
		if (targetUnit->category & badTargetCategory) {
			return true;
		}
	}

	if (gs->frameNum > (lastTargetRetry + 65)) {
		return true;
	}

	return false;
}


void CWeapon::SlowUpdate(bool noAutoTargetOverride)
{
#ifdef TRACE_SYNC
	tracefile << "Weapon slow update: ";
	tracefile << owner->id << " " << weaponNum <<  "\n";
#endif
	//If we can't get a line of fire from the muzzle try the aim piece instead since the weapon may just be turned in a wrong way
	int piece;
	if (useWeaponPosForAim) {
		piece = owner->script->QueryWeapon(weaponNum);
		if (useWeaponPosForAim > 1)
			useWeaponPosForAim--;
	} else {
		piece = owner->script->AimFromWeapon(weaponNum);
	}
	relWeaponMuzzlePos = owner->script->GetPiecePos(piece);
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;

	//FIXME: this might be potential speedup?
	// (AimFromWeapon may have been called already 5 lines ago)
	//if (useWeaponPosForAim)
	piece = owner->script->AimFromWeapon(weaponNum);
	relWeaponPos = owner->script->GetPiecePos(piece);

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

	if (targetType==Target_Unit && !haveUserTarget && targetUnit->neutral && owner->fireState < 3)
		HoldFire();

	//happens if the target or the unit has switched teams
	//should be handled by /ally processing now
	if (targetType==Target_Unit && !haveUserTarget && teamHandler->Ally(owner->allyteam, targetUnit->allyteam))
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

/*		owner->fireState>=2 && !haveUserTarget &&
	if (!weaponDef->noAutoTarget && !noAutoTargetOverride) {
		    ((targetType == Target_None) ||
		     ((targetType == Target_Unit) &&
		      ((targetUnit->category & badTargetCategory) ||
		       (targetUnit->neutral && (owner->fireState < 3)))) ||
		     (gs->frameNum > lastTargetRetry + 65))) {
*/
	if (!noAutoTargetOverride && ShouldCheckForNewTarget()) {
		lastTargetRetry = gs->frameNum;
		std::map<float, CUnit*> targets;
		helper->GenerateTargets(this, targetUnit, targets);

		for (std::map<float,CUnit*>::iterator ti=targets.begin();ti!=targets.end();++ti) {
			if (ti->second->neutral && (owner->fireState < 3)) {
				continue;
			}
			if (targetUnit && (ti->second->category & badTargetCategory)) {
				continue;
			}
			float3 tp(ti->second->midPos);
			tp+=errorVector*(weaponDef->targetMoveError*30*ti->second->speed.Length()*(1.0f-owner->limExperience));
			float appHeight=ground->GetApproximateHeight(tp.x,tp.z)+2;
			if (tp.y < appHeight) {
				tp.y = appHeight;
			}

			if (TryTarget(tp, false, ti->second)) {
				if (targetUnit) {
					DeleteDeathDependence(targetUnit);
				}
				targetType = Target_Unit;
				targetUnit = ti->second;
				targetPos = tp;
				AddDeathDependence(targetUnit);
				break;
			}
		}
	}
	if (targetType != Target_None) {
		owner->haveTarget = true;
		if (haveUserTarget) {
			owner->haveUserTarget = true;
		}
	} else {	//if we cant target anything try switching aim point
		if (useWeaponPosForAim && (useWeaponPosForAim == 1)) {
			useWeaponPosForAim = 0;
		} else {
			useWeaponPosForAim = 1;
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
	if (o==interceptTarget)
		interceptTarget = 0;
}

bool CWeapon::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (unit && !(onlyTargetCategory & unit->category)) {
		return false;
	}

	if (unit && ((unit->isDead  && (modInfo.fireAtKilled   == 0)) ||
	            (unit->crashing && (modInfo.fireAtCrashing == 0)))) {
		return false;
	}
	if (weaponDef->stockpile && !numStockpiled) {
		return false;
	}


	float3 dif = pos - weaponMuzzlePos;
	float heightDiff = 0.0f; // negative when target below owner
	const float absTB = streflop::fabsf(targetBorder);

	if (targetBorder != 0.0f && unit) {
		float3 difDir(dif);
		difDir.Normalize();


		CollisionVolume* cvOld = unit->collisionVolume;
		CollisionVolume  cvNew = CollisionVolume(unit->collisionVolume);
		CollisionQuery   cq;

		cvNew.RescaleAxes(absTB, absTB, absTB);
		cvNew.SetTestType(COLVOL_TEST_DISC);

		unit->collisionVolume = &cvNew;

		if (CCollisionHandler::DetectHit(unit, weaponMuzzlePos, ZeroVector, NULL)) {
			// weapon inside target unit's volume, no
			// real need to calculate penetration depth
			dif = ZeroVector;
		} else {
			// raytrace to find the proper correction
			// factor for non-spherical volumes based
			// on ingress position
			cvNew.SetTestType(COLVOL_TEST_CONT);

			// intersection is not guaranteed if the
			// volume has an offset, since here we're
			// shooting at the target's midpoint
			if (CCollisionHandler::DetectHit(unit, weaponMuzzlePos, pos + (difDir * cvNew.GetBoundingRadius() * 2.0f), &cq)) {
				if (targetBorder > 0.0f) { dif -= (difDir * ((pos - cq.p0).Length())); }
				if (targetBorder < 0.0f) { dif += (difDir * ((cq.p1 - pos).Length())); }
			}
		}

		unit->collisionVolume = cvOld;


		heightDiff = (weaponPos.y + dif.y) - owner->pos.y;
	} else {
		heightDiff = pos.y - owner->pos.y;
	}


	float r;
	if (!unit || cylinderTargetting < 0.01f) {
		r = GetRange2D(heightDiff * heightMod);
	} else {
		if (cylinderTargetting * range > fabs(heightDiff) * heightMod) {
			r = GetRange2D(0);
		} else {
			r = 0;
		}
	}

	if (dif.SqLength2D() >= r * r)
		return false;

	if (maxMainDirAngleDif > -0.999f) {
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
	short weaponHeadding = GetHeadingFromVector(mainDir.x, mainDir.z);
	short enemyHeadding = GetHeadingFromVector(
		tempTargetPos.x - weaponPos.x, tempTargetPos.z - weaponPos.z);
	return TryTargetHeading(enemyHeadding - weaponHeadding, tempTargetPos,userTarget, unit);
}

bool CWeapon::TryTargetRotate(float3 pos, bool userTarget) {
	if (!userTarget && weaponDef->noAutoTarget) {
		return false;
	}
	if (weaponDef->interceptor || !weaponDef->canAttackGround ||
	    (weaponDef->onlyTargetCategory != 0xffffffff)) {
		return false;
	}

	if (!weaponDef->waterweapon && pos.y < 1) {
		pos.y = 1;
	}

	short weaponHeading = GetHeadingFromVector(mainDir.x, mainDir.z);
	short enemyHeading = GetHeadingFromVector(
		pos.x - weaponPos.x, pos.z - weaponPos.z);

	return TryTargetHeading(enemyHeading - weaponHeading, pos, userTarget, 0);
}

bool CWeapon::TryTargetHeading(short heading, float3 pos, bool userTarget, CUnit* unit) {
	float3 tempfrontdir(owner->frontdir);
	float3 temprightdir(owner->rightdir);
	short tempHeadding = owner->heading;
	owner->heading = heading;
	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->rightdir = owner->frontdir.cross(owner->updir);
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
	bool val = TryTarget(pos, userTarget, unit);
	owner->frontdir = tempfrontdir;
	owner->rightdir = temprightdir;
	owner->heading = tempHeadding;
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
	weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
	return val;

}

void CWeapon::Init(void)
{
	int piece = owner->script->AimFromWeapon(weaponNum);
	relWeaponPos = owner->script->GetPiecePos(piece);
	weaponPos = owner->pos + owner->frontdir * relWeaponPos.z + owner->updir * relWeaponPos.y + owner->rightdir * relWeaponPos.x;
	piece = owner->script->QueryWeapon(weaponNum);
	relWeaponMuzzlePos = owner->script->GetPiecePos(piece);
	weaponMuzzlePos = owner->pos + owner->frontdir * relWeaponMuzzlePos.z + owner->updir * relWeaponMuzzlePos.y + owner->rightdir * relWeaponMuzzlePos.x;

	if (range > owner->maxRange) {
		owner->maxRange = range;
	}

	muzzleFlareSize = std::min(areaOfEffect * 0.2f, std::min(1500.f, weaponDef->damages[0]) * 0.003f);

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

void CWeapon::Fire()
{
#ifdef TRACE_SYNC
	tracefile << weaponDef->name << " fire: ";
	tracefile << owner->pos.x << " " << owner->dir.x << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z;
	tracefile << sprayAngle << " " <<  " " << salvoError.x << " " << salvoError.z << " " << owner->limExperience << " " << projectileSpeed << "\n";
#endif
	FireImpl();
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		Channels::Battle.PlaySample(fireSoundId, owner, fireSoundVolume);
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


void CWeapon::StopAttackingAllyTeam(int ally)
{
	if (targetUnit && targetUnit->allyteam == ally) {
		HoldFire();
	}
}
