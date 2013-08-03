/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/creg/STL_Map.h"
#include "WeaponDefHandler.h"
#include "Weapon.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Game/Players/Player.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "System/EventHandler.h"
#include "System/float3.h"
#include "System/myMath.h"
#include "System/Sync/SyncTracer.h"
#include "System/Sound/SoundChannels.h"
#include "System/Log/ILog.h"

CR_BIND_DERIVED(CWeapon, CObject, (NULL));

CR_REG_METADATA(CWeapon, (
	CR_MEMBER(owner),
	CR_MEMBER(range),
	CR_MEMBER(heightMod),
	CR_MEMBER(reloadTime),
	CR_MEMBER(reloadStatus),
	CR_MEMBER(salvoLeft),
	CR_MEMBER(salvoDelay),
	CR_MEMBER(salvoSize),
	CR_MEMBER(projectilesPerShot),
	CR_MEMBER(nextSalvo),
	CR_MEMBER(predict),
	CR_MEMBER(targetUnit),
	CR_MEMBER(accuracy),
	CR_MEMBER(projectileSpeed),
	CR_MEMBER(predictSpeedMod),
	CR_MEMBER(metalFireCost),
	CR_MEMBER(energyFireCost),
	CR_MEMBER(fireSoundId),
	CR_MEMBER(fireSoundVolume),
	CR_MEMBER(hasBlockShot),
	CR_MEMBER(hasTargetWeight),
	CR_MEMBER(angleGood),
	CR_MEMBER(avoidTarget),
	CR_MEMBER(haveUserTarget),
	CR_MEMBER(subClassReady),
	CR_MEMBER(onlyForward),
	CR_MEMBER(muzzleFlareSize),
	CR_MEMBER(craterAreaOfEffect),
	CR_MEMBER(damageAreaOfEffect),

	CR_MEMBER(badTargetCategory),
	CR_MEMBER(onlyTargetCategory),
	CR_MEMBER(incomingProjectiles),
	CR_MEMBER(weaponDef),
	CR_MEMBER(stockpileTime),
	CR_MEMBER(buildPercent),
	CR_MEMBER(numStockpiled),
	CR_MEMBER(numStockpileQued),
	CR_MEMBER(interceptTarget),
	CR_ENUM_MEMBER(targetType),
	CR_MEMBER(sprayAngle),
	CR_MEMBER(useWeaponPosForAim),

	CR_MEMBER(lastRequest),
	CR_MEMBER(lastTargetRetry),
	CR_MEMBER(lastErrorVectorUpdate),

	CR_MEMBER(slavedTo),
	CR_MEMBER(maxForwardAngleDif),
	CR_MEMBER(maxMainDirAngleDif),
	CR_MEMBER(hasCloseTarget),
	CR_MEMBER(targetBorder),
	CR_MEMBER(cylinderTargeting),
	CR_MEMBER(minIntensity),
	CR_MEMBER(heightBoostFactor),
	CR_MEMBER(avoidFlags),
	CR_MEMBER(collisionFlags),
	CR_MEMBER(fuelUsage),
	CR_MEMBER(weaponNum),

	CR_MEMBER(relWeaponPos),
	CR_MEMBER(weaponPos),
	CR_MEMBER(relWeaponMuzzlePos),
	CR_MEMBER(weaponMuzzlePos),
	CR_MEMBER(weaponDir),
	CR_MEMBER(mainDir),
	CR_MEMBER(wantedDir),
	CR_MEMBER(lastRequestedDir),
	CR_MEMBER(salvoError),
	CR_MEMBER(errorVector),
	CR_MEMBER(errorVectorAdd),
	CR_MEMBER(targetPos),
	CR_MEMBER(targetBorderPos)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWeapon::CWeapon(CUnit* owner):
	owner(owner),
	weaponDef(0),
	weaponNum(-1),
	haveUserTarget(false),
	craterAreaOfEffect(1.0f),
	damageAreaOfEffect(1.0f),
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
	projectilesPerShot(1),
	nextSalvo(0),
	salvoLeft(0),
	targetType(Target_None),
	targetUnit(0),
	predict(0),
	predictSpeedMod(1),
	metalFireCost(0),
	energyFireCost(0),
	fireSoundId(0),
	fireSoundVolume(0),
	hasBlockShot(false),
	hasTargetWeight(false),
	angleGood(false),
	avoidTarget(false),
	subClassReady(true),
	onlyForward(false),
	badTargetCategory(0),
	onlyTargetCategory(0xffffffff),

	interceptTarget(NULL),
	stockpileTime(1),
	buildPercent(0),
	numStockpiled(0),
	numStockpileQued(0),

	lastRequest(0),
	lastTargetRetry(-100),
	lastErrorVectorUpdate(0),

	slavedTo(0),
	maxForwardAngleDif(0.0f),
	maxMainDirAngleDif(-1.0f),
	targetBorder(0.f),
	cylinderTargeting(0.f),
	minIntensity(0.f),
	heightBoostFactor(-1.f),
	avoidFlags(0),
	collisionFlags(0),
	fuelUsage(0),

	relWeaponPos(UpVector),
	weaponPos(ZeroVector),
	relWeaponMuzzlePos(UpVector),
	weaponMuzzlePos(ZeroVector),
	weaponDir(ZeroVector),
	mainDir(FwdVector),
	wantedDir(UpVector),
	lastRequestedDir(-UpVector),
	salvoError(ZeroVector),
	errorVector(ZeroVector),
	errorVectorAdd(ZeroVector),
	targetPos(OnesVector)
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

	hasBlockShot = owner->script->HasBlockShot(weaponNum);
	hasTargetWeight = owner->script->HasTargetWeight(weaponNum);
}


inline bool CWeapon::CobBlockShot(const CUnit* targetUnit)
{
	if (!hasBlockShot) {
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
	const AAirMoveType* a = dynamic_cast<AAirMoveType*>(u->moveType);
	return (a != NULL && a->GetPadStatus() != 0);
}


void CWeapon::UpdateRelWeaponPos()
{
	// If we can't get a line of fire from the muzzle, try
	// the aim piece instead (since the weapon may just be
	// turned in a wrong way)
	int weaponPiece = -1;
	bool weaponAimed = (useWeaponPosForAim == 0);

	if (!weaponAimed) {
		weaponPiece = owner->script->QueryWeapon(weaponNum);
	} else {
		weaponPiece = owner->script->AimFromWeapon(weaponNum);
	}

	relWeaponMuzzlePos = owner->script->GetPiecePos(weaponPiece);

	if (!weaponAimed) {
		weaponPiece = owner->script->AimFromWeapon(weaponNum);
	}

	relWeaponPos = owner->script->GetPiecePos(weaponPiece);
}

void CWeapon::Update()
{
	if (!UpdateStockpile())
		return;

	UpdateTargeting();
	UpdateFire();
	UpdateSalvo();

#ifdef TRACE_SYNC
	tracefile << __FUNCTION__;
	tracefile << weaponPos.x << " " << weaponPos.y << " " << weaponPos.z << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z << "\n";
#endif
}


void CWeapon::UpdateTargeting()
{
	if (hasCloseTarget) {
		UpdateRelWeaponPos();
	}

	if (targetType == Target_Unit) {
		if (lastErrorVectorUpdate < gs->frameNum - UNIT_SLOWUPDATE_RATE) {
			errorVectorAdd = (gs->randVector() - errorVector) * (1.0f / UNIT_SLOWUPDATE_RATE);
			lastErrorVectorUpdate = gs->frameNum;
		}

		// to prevent runaway prediction (happens sometimes when a missile
		// is moving *away* from its target), we may need to disable missiles
		// in case they fly around too long
		predict = std::min(predict, 50000.0f);
		errorVector += errorVectorAdd;

		float3 lead = targetUnit->speed * (weaponDef->predictBoost + predictSpeedMod * (1.0f - weaponDef->predictBoost)) * predict;

		if (weaponDef->leadLimit >= 0.0f && lead.SqLength() > Square(weaponDef->leadLimit + weaponDef->leadBonus * owner->experience)) {
			lead *= (weaponDef->leadLimit + weaponDef->leadBonus*owner->experience) / (lead.Length() + 0.01f);
		}

		const float3 errorPos = targetUnit->GetErrorPos(owner->allyteam, true);
		const float errorScale = ( MoveErrorExperience() * GAME_SPEED * targetUnit->speed.Length() );

		float3 tmpTargetPos = errorPos + lead + errorVector * errorScale;
		float3 tmpTargetVec = tmpTargetPos - weaponMuzzlePos;
		float3 tmpTargetDir = tmpTargetVec;

		SetTargetBorderPos(targetUnit, tmpTargetPos, tmpTargetVec, tmpTargetDir);

		targetPos = (targetBorder == 0.0f)? tmpTargetPos: targetBorderPos;
		targetPos.y = std::max(targetPos.y, ground->GetApproximateHeight(targetPos.x, targetPos.z) + 2.0f);

		if (!weaponDef->waterweapon) {
			targetPos.y = std::max(targetPos.y, 0.0f);
		}
	}

	if (weaponDef->interceptor) {
		// keep track of the closest projectile heading our way (if any)
		UpdateInterceptTarget();
	}

	if (targetType != Target_None) {
		const float3 worldTargetDir = (targetPos - owner->pos).SafeNormalize();
		const float3 worldMainDir =
			owner->frontdir * mainDir.z +
			owner->rightdir * mainDir.x +
			owner->updir    * mainDir.y;
		const bool targetAngleConstraint = CheckTargetAngleConstraint(worldTargetDir, worldMainDir);

		if (angleGood && !targetAngleConstraint) {
 			// weapon finished a previously started AimWeapon thread and wants to
 			// fire, but target is no longer within contraints --> wait for re-aim
 			angleGood = false;
 		}
		if (onlyForward && targetAngleConstraint) {
			// NOTE:
			//   this should not need to be here, but many legacy scripts do not
			//   seem to define Aim*Ary in COB for units with onlyForward weapons
			//   (so angleGood is never set to true) -- REMOVE AFTER 90.0
			angleGood = true;
		}

		if (gs->frameNum >= (lastRequest + (GAME_SPEED >> 1))) {
			// periodically re-aim the weapon (by calling the script's AimWeapon
			// every N=15 frames regardless of current angleGood state)
			//
			// NOTE:
			//   let scripts do active aiming even if we are an onlyForward weapon
			//   (reduces how far the entire unit must turn to face worldTargetDir)
			//
			//   if AimWeapon sets angleGood immediately (ie. before it returns),
			//   the weapon can continuously fire at its maximum rate once every
			//   int(reloadTime / owner->reloadSpeed) frames
			//
			//   if it does not (eg. because AimWeapon always spawns a thread to
			//   aim the weapon and defers setting angleGood to it) then this can
			//   lead to irregular/stuttering firing behavior, even in scenarios
			//   when the weapon does not have to re-aim --> detecting this case
			//   is the responsibility of the script
			angleGood = false;

			lastRequestedDir = wantedDir;
			lastRequest = gs->frameNum;

			const float heading = GetHeadingFromVectorF(wantedDir.x, wantedDir.z);
			const float pitch = math::asin(Clamp(wantedDir.dot(owner->updir), -1.0f, 1.0f));

			// for COB, this sets <angleGood> to return value of AimWeapon when finished,
			// for LUS, there exists a callout to set the <angleGood> member directly.
			// FIXME: convert CSolidObject::heading to radians too.
			owner->script->AimWeapon(weaponNum, ClampRad(heading - owner->heading * TAANG2RAD), pitch);
		}
	}
}


void CWeapon::UpdateFire()
{
	bool canFire = true;
	const CPlayer* fpsPlayer = owner->fpsControlPlayer;

	//FIXME merge some of the checks with TryTarget/TestRange/TestTarget
	canFire = canFire && angleGood;
	canFire = canFire && subClassReady;
	canFire = canFire && (salvoLeft == 0);
	canFire = canFire && (targetType != Target_None);
	canFire = canFire && (reloadStatus <= gs->frameNum);
	canFire = canFire && (!weaponDef->stockpile || (numStockpiled > 0));
	canFire = canFire && (weaponDef->fireSubmersed || (weaponMuzzlePos.y > 0));
	canFire = canFire && ((fpsPlayer == NULL)
		 || fpsPlayer->fpsController.mouse1
		 || fpsPlayer->fpsController.mouse2);
	canFire = canFire && ((owner->unitDef->maxFuel == 0) || (owner->currentFuel > 0) || (fuelUsage == 0));
	canFire = canFire && !isBeingServicedOnPad(owner);
	canFire = canFire && (wantedDir.dot(lastRequestedDir) > 0.94); // ~20 degree sanity check to force new aim

	if (canFire) {
		if ((weaponDef->stockpile ||
		     (teamHandler->Team(owner->team)->metal >= metalFireCost &&
		      teamHandler->Team(owner->team)->energy >= energyFireCost)))
		{
			const int piece = owner->script->QueryWeapon(weaponNum);
			owner->script->GetEmitDirPos(piece, relWeaponMuzzlePos, weaponDir);
			weaponMuzzlePos = owner->pos + owner->frontdir * relWeaponMuzzlePos.z +
			                               owner->updir    * relWeaponMuzzlePos.y +
			                               owner->rightdir * relWeaponMuzzlePos.x;
			weaponDir = owner->frontdir * weaponDir.z +
			            owner->updir    * weaponDir.y +
			            owner->rightdir * weaponDir.x;
			weaponDir.SafeNormalize();
			useWeaponPosForAim = reloadTime / 16 + 8;

			if (TryTarget(targetPos, haveUserTarget, targetUnit) && !CobBlockShot(targetUnit)) {
				if (weaponDef->stockpile) {
					const int oldCount = numStockpiled;
					numStockpiled--;
					owner->commandAI->StockpileChanged(this);
					eventHandler.StockpileChanged(owner, this, oldCount);
				} else {
					owner->UseEnergy(energyFireCost);
					owner->UseMetal(metalFireCost);
					owner->currentFuel = std::max(0.0f, owner->currentFuel - fuelUsage);
				}

				reloadStatus = gs->frameNum + int(reloadTime / owner->reloadSpeed);

				salvoLeft = salvoSize;
				nextSalvo = gs->frameNum;
				salvoError = gs->randVector() * (owner->isMoving? weaponDef->movingAccuracy: accuracy);

				if (targetType == Target_Pos || (targetType == Target_Unit && !(targetUnit->losStatus[owner->allyteam] & LOS_INLOS))) {
					// area firing stuff is too effective at radar firing...
					salvoError *= 1.3f;
				}

				owner->lastMuzzleFlameSize = muzzleFlareSize;
				owner->lastMuzzleFlameDir = wantedDir;
				owner->script->FireWeapon(weaponNum);
			}
		} else {
			if (!weaponDef->stockpile && TryTarget(targetPos, haveUserTarget, targetUnit)) {
				// update the energy and metal required counts
				const int minPeriod = std::max(1, (int)(reloadTime / owner->reloadSpeed));
				const float averageFactor = 1.0f / (float)minPeriod;
				teamHandler->Team(owner->team)->energyPull += averageFactor * energyFireCost;
				teamHandler->Team(owner->team)->metalPull += averageFactor * metalFireCost;
			}
		}
	}
}


bool CWeapon::UpdateStockpile()
{
	if (weaponDef->stockpile) {
		if (numStockpileQued > 0) {
			const float p = 1.0f / stockpileTime;

			if (teamHandler->Team(owner->team)->metal >= metalFireCost*p && teamHandler->Team(owner->team)->energy >= energyFireCost*p) {
				owner->UseEnergy(energyFireCost * p);
				owner->UseMetal(metalFireCost * p);
				buildPercent += p;
			} else {
				// update the energy and metal required counts
				teamHandler->Team(owner->team)->energyPull += (energyFireCost * p);
				teamHandler->Team(owner->team)->metalPull += (metalFireCost * p);
			}
			if (buildPercent >= 1) {
				const int oldCount = numStockpiled;
				buildPercent = 0;
				numStockpileQued--;
				numStockpiled++;
				owner->commandAI->StockpileChanged(this);
				eventHandler.StockpileChanged(owner, this, oldCount);
			}
		}

		if (numStockpiled <= 0 && salvoLeft <= 0) {
			return false;
		}
	}

	return true;
}


void CWeapon::UpdateSalvo()
{
	if (salvoLeft && nextSalvo <= gs->frameNum) {
		salvoLeft--;
		nextSalvo = gs->frameNum + salvoDelay;
		owner->lastFireWeapon = gs->frameNum;

		int projectiles = projectilesPerShot;

		const bool attackingPos = ((targetType == Target_Pos) && (targetPos == owner->attackPos));
		const bool attackingUnit = ((targetType == Target_Unit) && (targetUnit == owner->attackTarget));

		while (projectiles > 0) {
			--projectiles;

			owner->script->Shot(weaponNum);

			int piece = owner->script->AimFromWeapon(weaponNum);
			relWeaponPos = owner->script->GetPiecePos(piece);

			piece = owner->script->QueryWeapon(weaponNum);
			owner->script->GetEmitDirPos(piece, relWeaponMuzzlePos, weaponDir);

			weaponPos = owner->pos +
				owner->frontdir * relWeaponPos.z +
				owner->updir    * relWeaponPos.y +
				owner->rightdir * relWeaponPos.x;
			weaponMuzzlePos = owner->pos +
				owner->frontdir * relWeaponMuzzlePos.z +
				owner->updir    * relWeaponMuzzlePos.y +
				owner->rightdir * relWeaponMuzzlePos.x;

			weaponDir =
				owner->frontdir * weaponDir.z +
				owner->updir    * weaponDir.y +
				owner->rightdir * weaponDir.x;
			weaponDir.SafeNormalize();

			if (owner->unitDef->decloakOnFire && (owner->scriptCloak <= 2)) {
				if (owner->isCloaked) {
					owner->isCloaked = false;
					eventHandler.UnitDecloaked(owner);
				}
				owner->curCloakTimeout = gs->frameNum + owner->cloakTimeout;
			}

			Fire();
		}

		//Rock the unit in the direction of fire
		if (owner->script->HasRockUnit()) {
			float3 rockDir = wantedDir;
			rockDir.y = 0.0f;
			rockDir = -rockDir.SafeNormalize();
			owner->script->RockUnit(rockDir);
		}

		owner->commandAI->WeaponFired(this, weaponNum == 0, (salvoLeft == 0 && (attackingPos || attackingUnit)));

		if (salvoLeft == 0) {
			owner->script->EndBurst(weaponNum);
		}
	}
}

bool CWeapon::AttackGround(float3 newTargetPos, bool isUserTarget)
{
	if (!isUserTarget && weaponDef->noAutoTarget) {
		return false;
	}
	if (weaponDef->interceptor || !weaponDef->canAttackGround) {
		return false;
	}

	// keep target positions on the surface if this weapon hates water
	if (!weaponDef->waterweapon) {
		newTargetPos.y = std::max(newTargetPos.y, 0.0f);
	}

	weaponMuzzlePos =
		owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;

	if (weaponMuzzlePos.y < ground->GetHeightReal(weaponMuzzlePos.x, weaponMuzzlePos.z)) {
		// hope that we are underground because we are a popup weapon and will come above ground later
		weaponMuzzlePos = owner->pos + UpVector * 10;
	}

	if (!TryTarget(newTargetPos, isUserTarget, NULL))
		return false;

	if (targetUnit != NULL) {
		DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
		targetUnit = NULL;
	}

	haveUserTarget = isUserTarget;
	targetType = Target_Pos;
	targetPos = newTargetPos;

	return true;
}

bool CWeapon::AttackUnit(CUnit* newTargetUnit, bool isUserTarget)
{
	if ((!isUserTarget && weaponDef->noAutoTarget)) {
		return false;
	}
	if (weaponDef->interceptor)
		return false;

	weaponPos =
		owner->pos +
		owner->frontdir * relWeaponPos.z +
		owner->updir    * relWeaponPos.y +
		owner->rightdir * relWeaponPos.x;
	weaponMuzzlePos =
		owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;

	if (weaponMuzzlePos.y < ground->GetHeightReal(weaponMuzzlePos.x, weaponMuzzlePos.z)) {
		// hope that we are underground because we are a popup weapon and will come above ground later
		weaponMuzzlePos = owner->pos + UpVector * 10;
	}

	if (newTargetUnit == NULL) {
		if (targetType != Target_Unit) {
			// make the unit be more likely to keep the current target if user starts to move it
			targetType = Target_None;
		}

		// cannot have a user-target without a unit
		haveUserTarget = false;
		return false;
	}

	// check if it is theoretically impossible for us to attack this unit
	// must be done before we assign <unit> to <targetUnit>, which itself
	// must precede the TryTarget call (since we do want to assign if it
	// is eg. just out of range currently --> however, this in turn causes
	// "lock-on" targeting behavior which is less desirable, eg. we want a
	// lock on a user-selected target that moved out of range to be broken
	// after some time so automatic targeting can select new in-range units)
	//
	// note that TryTarget is also called from other places and so has to
	// repeat this check, but the redundancy added is minimal
	#if 0
	if (!(onlyTargetCategory & newTargetUnit->category)) {
		return false;
	}
	#endif

	const float3 errorPos = newTargetUnit->GetErrorPos(owner->allyteam, true);
	const float errorScale = (MoveErrorExperience() * GAME_SPEED * newTargetUnit->speed.Length() );
	const float3 newTargetPos = errorPos + errorVector * errorScale;

	if (!TryTarget(newTargetPos, isUserTarget, newTargetUnit))
		return false;

	if (targetUnit != NULL) {
		DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
		targetUnit = NULL;
	}

	haveUserTarget = isUserTarget;
	targetType = Target_Unit;
	targetUnit = newTargetUnit;
	targetPos = (targetBorder == 0.0f)? newTargetPos: targetBorderPos;
	targetPos.y = std::max(targetPos.y, ground->GetApproximateHeight(targetPos.x, targetPos.z) + 2.0f);

	AddDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
	avoidTarget = false;

	return true;
}


void CWeapon::HoldFire()
{
	if (targetUnit) {
		DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
		targetUnit = NULL;
	}

	targetType = Target_None;

	if (!weaponDef->noAutoTarget) {
		// if haveUserTarget is set to false unconditionally, a subsequent
		// call to AttackUnit from Unit::SlowUpdateWeapons would abort the
		// attack for noAutoTarget weapons
		haveUserTarget = false;
	}
}



bool CWeapon::AllowWeaponTargetCheck()
{
	if (luaRules != NULL) {
		const int checkAllowed = luaRules->AllowWeaponTargetCheck(owner->id, weaponNum, weaponDef->id);

		if (checkAllowed >= 0) {
			return checkAllowed;
		}
	}

	if (weaponDef->noAutoTarget)                 { return false; }
	if (owner->fireState < FIRESTATE_FIREATWILL) { return false; }

	if (avoidTarget)               { return true; }
	if (targetType == Target_None) { return true; }

	if (targetType == Target_Unit) {
		if (targetUnit->category & badTargetCategory) {
			return true;
		}
		if (!TryTarget(targetUnit, haveUserTarget)) {
			// if we have a user-target (ie. a user attack order)
			// then only allow generating opportunity targets iff
			// it is not possible to hit the user's chosen unit
			// TODO: this makes it easy to add toggle-able locking
			//
			// this will switch <targetUnit>, but the CAI will keep
			// calling AttackUnit while the original order target is
			// alive to put it back when possible
			//
			// note that the CAI itself only auto-picks a target
			// when a unit has no commands left in its queue, so
			// it can not interfere
			return true;
		}
	}

	if (gs->frameNum > (lastTargetRetry + 65)) {
		return true;
	}

	return false;
}

void CWeapon::AutoTarget() {
	lastTargetRetry = gs->frameNum;

	std::multimap<float, CUnit*> targets;
	std::multimap<float, CUnit*>::const_iterator targetsIt;

	// NOTE:
	//   sorts by INCREASING order of priority, so lower equals better
	//   <targets> can contain duplicates if a unit covers multiple quads
	//   <targets> is normally sorted such that all bad TC units are at the
	//   end, but Lua can mess with the ordering arbitrarily
	CGameHelper::GenerateWeaponTargets(this, targetUnit, targets);

	CUnit* prevTargetUnit = NULL;
	CUnit* goodTargetUnit = NULL;
	CUnit* badTargetUnit = NULL;

	float3 nextTargetPos = ZeroVector;

	for (targetsIt = targets.begin(); targetsIt != targets.end(); ++targetsIt) {
		CUnit* nextTargetUnit = targetsIt->second;

		if (nextTargetUnit == prevTargetUnit)
			continue; // filter consecutive duplicates
		if (nextTargetUnit->IsNeutral() && (owner->fireState <= FIRESTATE_FIREATWILL))
			continue;

		const float weaponError = MoveErrorExperience() * GAME_SPEED * nextTargetUnit->speed.Length();

		prevTargetUnit = nextTargetUnit;
		nextTargetPos = nextTargetUnit->aimPos + (errorVector * weaponError);

		const float appHeight = ground->GetApproximateHeight(nextTargetPos.x, nextTargetPos.z) + 2.0f;

		if (nextTargetPos.y < appHeight) {
			nextTargetPos.y = appHeight;
		}

		if (!TryTarget(nextTargetPos, false, nextTargetUnit))
			continue;

		if ((nextTargetUnit->category & badTargetCategory) != 0) {
			// save the "best" bad target in case we have no other
			// good targets (of higher priority) left in <targets>
			if (badTargetUnit != NULL)
				continue;

			badTargetUnit = nextTargetUnit;
		} else {
			goodTargetUnit = nextTargetUnit;
			break;
		}
	}

	if (goodTargetUnit != NULL || badTargetUnit != NULL) {
		const bool haveOldTarget = (targetUnit != NULL);
		const bool haveNewTarget =
			(goodTargetUnit != NULL && goodTargetUnit != targetUnit) ||
			( badTargetUnit != NULL &&  badTargetUnit != targetUnit);

		if (haveOldTarget && haveNewTarget) {
			// delete our old target dependence if we are switching targets
			DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
		}

		// pick our new target
		targetType = Target_Unit;
		targetUnit = (goodTargetUnit != NULL)? goodTargetUnit: badTargetUnit;
		targetPos = nextTargetPos;

		if (!haveOldTarget || haveNewTarget) {
			// add new target dependence if we had no target or switched
			AddDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
		}
	}
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

	UpdateRelWeaponPos();
	useWeaponPosForAim = std::max(0, useWeaponPosForAim - 1);

	weaponMuzzlePos =
		owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;
	weaponPos =
		owner->pos +
		owner->frontdir * relWeaponPos.z +
		owner->updir    * relWeaponPos.y +
		owner->rightdir * relWeaponPos.x;

	if (weaponMuzzlePos.y < ground->GetHeightReal(weaponMuzzlePos.x, weaponMuzzlePos.z)) {
		// hope that we are underground because we are a popup weapon and will come above ground later
		weaponMuzzlePos = owner->pos + UpVector * 10;
	}

	predictSpeedMod = 1.0f + (gs->randFloat() - 0.5f) * 2 * (1.0f - owner->limExperience);
	hasCloseTarget = ((targetPos - weaponPos).SqLength() < relWeaponPos.SqLength() * 16);


	if (targetType != Target_None && !TryTarget(targetPos, haveUserTarget, targetUnit)) {
		HoldFire();
	}

	if (targetType == Target_Unit) {
		// stop firing at cloaked targets
		if (targetUnit != NULL && targetUnit->isCloaked && !(targetUnit->losStatus[owner->allyteam] & (LOS_INLOS | LOS_INRADAR)))
			HoldFire();

		if (!haveUserTarget) {
			// stop firing at neutral targets (unless in FAW mode)
			// note: HoldFire sets targetUnit to NULL, so recheck
			if (targetUnit != NULL && targetUnit->IsNeutral() && owner->fireState <= FIRESTATE_FIREATWILL)
				HoldFire();

			// stop firing at allied targets
			//
			// this situation (unit keeps attacking its target if the
			// target or the unit switches to an allied team) should
			// be handled by /ally processing now
			if (targetUnit != NULL && teamHandler->Ally(owner->allyteam, targetUnit->allyteam))
				HoldFire();
		}
	}

	if (slavedTo) {
		// use targets from the thing we are slaved to
		if (targetUnit) {
			DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
			targetUnit = NULL;
		}
		targetType = Target_None;

		if (slavedTo->targetType == Target_Unit) {
			const float3 tp =
				slavedTo->targetUnit->GetErrorPos(owner->allyteam, true) +
				errorVector * (MoveErrorExperience() * GAME_SPEED * slavedTo->targetUnit->speed.Length() );

			if (TryTarget(tp, false, slavedTo->targetUnit)) {
				targetType = Target_Unit;
				targetUnit = slavedTo->targetUnit;
				targetPos = tp;

				AddDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
			}
		} else if (slavedTo->targetType == Target_Pos) {
			if (TryTarget(slavedTo->targetPos, false, 0)) {
				targetType = Target_Pos;
				targetPos = slavedTo->targetPos;
			}
		}
		return;
	}


	if (!noAutoTargetOverride && AllowWeaponTargetCheck()) {
		AutoTarget();
	}

	if (targetType == Target_None) {
		// if we can't target anything, try switching aim point
		useWeaponPosForAim = std::max(0, useWeaponPosForAim - 1);
	}
}

void CWeapon::DependentDied(CObject* o)
{
	if (o == targetUnit) {
		targetUnit = NULL;
		if (targetType == Target_Unit) {
			targetType = Target_None;
			haveUserTarget = false;
		}
	}

	// NOTE: DependentDied is called from ~CObject-->Detach, object is just barely valid
	if (weaponDef->interceptor || weaponDef->isShield) {
		incomingProjectiles.erase(static_cast<CWeaponProjectile*>(o)->id);
	}

	if (o == interceptTarget) {
		interceptTarget = NULL;
	}
}

bool CWeapon::TargetUnitOrPositionInUnderWater(const float3& targetPos, const CUnit* targetUnit)
{
	if (targetUnit != NULL) {
		return (targetUnit->IsUnderWater());
	} else {
		return (targetPos.y < 0.0f);
	}
}

bool CWeapon::TargetUnitOrPositionInWater(const float3& targetPos, const CUnit* targetUnit)
{
	if (targetUnit != NULL) {
		return (targetUnit->IsInWater());
	} else {
		// consistent with CUnit::IsInWater(), and needed because
		// GUIHandler places (some) user ground-attack orders on
		// water surface
		return (targetPos.y <= 0.0f);
	}
}

bool CWeapon::CheckTargetAngleConstraint(const float3& worldTargetDir, const float3& worldWeaponDir) const {
	if (onlyForward) {
		if (maxForwardAngleDif > -1.0f) {
			// if we are not a turret, we care about our owner's direction
			if (owner->frontdir.dot(worldTargetDir) < maxForwardAngleDif)
				return false;
		}
	} else {
		if (maxMainDirAngleDif > -1.0f) {
			if (worldWeaponDir.dot(worldTargetDir) < maxMainDirAngleDif)
				return false;
		}
	}

	return true;
}


bool CWeapon::SetTargetBorderPos(
	CUnit* targetUnit,
	float3& rawTargetPos,
	float3& rawTargetVec,
	float3& rawTargetDir)
{
	if (targetBorder == 0.0f)
		return false;
	if (targetUnit == NULL)
		return false;

	const float tbScale = math::fabsf(targetBorder);

	CollisionVolume  tmpColVol = CollisionVolume(targetUnit->collisionVolume);
	CollisionQuery   tmpColQry;

	// test for "collision" with a temporarily volume
	// (scaled uniformly by the absolute target-border
	// factor)
	tmpColVol.RescaleAxes(float3(tbScale, tbScale, tbScale));
	tmpColVol.SetBoundingRadius();
	tmpColVol.SetUseContHitTest(false);

	targetBorderPos = rawTargetPos;

	if (CCollisionHandler::DetectHit(&tmpColVol, targetUnit, weaponMuzzlePos, ZeroVector, NULL)) {
		// our weapon muzzle is inside the target unit's volume; this
		// means we do not need to make any adjustments to targetVec
		// (in this case targetBorderPos remains equal to targetPos)
		rawTargetVec = ZeroVector;
	} else {
		rawTargetDir = rawTargetDir.SafeNormalize();

		// otherwise, perform a raytrace to find the proper length correction
		// factor for non-spherical coldet volumes based on the ray's ingress
		// (for positive TB values) or egress (for negative TB values) position;
		// this either increases or decreases the length of <targetVec> but does
		// not change its direction
		tmpColVol.SetUseContHitTest(true);

		// make the ray-segment long enough so it can reach the far side of the
		// scaled collision volume (helps to ensure a ray-intersection is found)
		//
		// note: ray-intersection is NOT guaranteed if the volume itself has a
		// non-zero offset, since here we are "shooting" at the target UNIT's
		// aimpoint
		const float3 targetOffset = rawTargetDir * (tmpColVol.GetBoundingRadius() * 2.0f);
		const float3 targetRayPos = rawTargetPos + targetOffset;

		// adjust the length of <targetVec> based on the targetBorder factor
		if (CCollisionHandler::DetectHit(&tmpColVol, targetUnit, weaponMuzzlePos, targetRayPos, &tmpColQry)) {
			if (targetBorder > 0.0f) { rawTargetVec -= (rawTargetDir * rawTargetPos.distance(tmpColQry.GetIngressPos())); }
			if (targetBorder < 0.0f) { rawTargetVec += (rawTargetDir * rawTargetPos.distance(tmpColQry.GetEgressPos())); }

			targetBorderPos = weaponMuzzlePos + rawTargetVec;
		}
	}

	// true indicates we took the else-branch and rawTargetDir was normalized
	// note: this does *NOT* also imply that targetBorderPos != rawTargetPos
	return (rawTargetDir.SqLength() == 1.0f);
}

bool CWeapon::GetTargetBorderPos(const CUnit* targetUnit, const float3& rawTargetPos, float3& rawTargetVec, float3& rawTargetDir) const
{
	if (targetBorder == 0.0f)
		return false;
	if (targetUnit == NULL)
		return false;

	const float tbScale = math::fabsf(targetBorder);

	CollisionVolume  tmpColVol(targetUnit->collisionVolume);
	CollisionQuery   tmpColQry;

	// test for "collision" with a temporarily volume
	// (scaled uniformly by the absolute target-border
	// factor)
	tmpColVol.RescaleAxes(float3(tbScale, tbScale, tbScale));
	tmpColVol.SetBoundingRadius();
	tmpColVol.SetUseContHitTest(false);

	if (CCollisionHandler::DetectHit(&tmpColVol, targetUnit, weaponMuzzlePos, ZeroVector, NULL)) {
		// our weapon muzzle is inside the target unit's volume; this
		// means we do not need to make any adjustments to targetVec
		rawTargetVec = ZeroVector;
	} else {
		rawTargetDir = rawTargetDir.SafeNormalize();

		// otherwise, perform a raytrace to find the proper length correction
		// factor for non-spherical coldet volumes based on the ray's ingress
		// (for positive TB values) or egress (for negative TB values) position;
		// this either increases or decreases the length of <targetVec> but does
		// not change its direction
		tmpColVol.SetUseContHitTest(true);

		// make the ray-segment long enough so it can reach the far side of the
		// scaled collision volume (helps to ensure a ray-intersection is found)
		//
		// note: ray-intersection is NOT guaranteed if the volume itself has a
		// non-zero offset, since here we are "shooting" at the target UNIT's
		// aimpoint
		const float3 targetOffset = rawTargetDir * (tmpColVol.GetBoundingRadius() * 2.0f);
		const float3 targetRayPos = rawTargetPos + targetOffset;

		// adjust the length of <targetVec> based on the targetBorder factor
		if (CCollisionHandler::DetectHit(&tmpColVol, targetUnit, weaponMuzzlePos, targetRayPos, &tmpColQry)) {
			if (targetBorder > 0.0f) { rawTargetVec -= (rawTargetDir * rawTargetPos.distance(tmpColQry.GetIngressPos())); }
			if (targetBorder < 0.0f) { rawTargetVec += (rawTargetDir * rawTargetPos.distance(tmpColQry.GetEgressPos())); }
		}
	}

	// true indicates we took the else-branch and rawTargetDir was normalized
	return (rawTargetDir.SqLength() == 1.0f);
}

bool CWeapon::TryTarget(const float3& tgtPos, bool userTarget, const CUnit* targetUnit) const
{
	if (!TestTarget(tgtPos, userTarget, targetUnit))
		return false;

	if (!TestRange(tgtPos, userTarget, targetUnit))
		return false;

	//FIXME add a forcedUserTarget (a forced fire mode enabled with ctrl key or something) and skip the tests below then
	return (HaveFreeLineOfFire(tgtPos, userTarget, targetUnit));
}


// if targetUnit != NULL, this checks our onlyTargetCategory against unit->category
// etc. as well as range, otherwise the only concern is range and angular difference
// (terrain is NOT checked here, HaveFreeLineOfFire does that)
bool CWeapon::TestTarget(const float3& tgtPos, bool /*userTarget*/, const CUnit* targetUnit) const
{
	if (targetUnit && (targetUnit == owner)) {
		return false;
	}

	if (targetUnit && !(onlyTargetCategory & targetUnit->category)) {
		return false;
	}

	// test: ground/water -> water
	if (!weaponDef->waterweapon && TargetUnitOrPositionInUnderWater(tgtPos, targetUnit))
		return false;

	if (weaponMuzzlePos.y < 0.0f) {
		// weapon muzzle underwater but we cannot fire underwater
		if (!weaponDef->fireSubmersed) {
			return false;
		}
		// target outside water but we cannot travel out of water
		if (!weaponDef->submissile && !TargetUnitOrPositionInWater(tgtPos, targetUnit)) {
			return false;
		}
	}

	if (targetUnit && ((targetUnit->isDead       && (modInfo.fireAtKilled   == 0)) ||
	                   (targetUnit->IsCrashing() && (modInfo.fireAtCrashing == 0)))) {
		return false;
	}

	return true;
}

bool CWeapon::TestRange(const float3& tgtPos, bool /*userTarget*/, const CUnit* targetUnit) const
{
	float3 tmpTargetPos = tgtPos;
	float3 tmpTargetVec = tmpTargetPos - weaponMuzzlePos;
	float3 tmpTargetDir = tmpTargetVec;

	const bool normalized = GetTargetBorderPos(targetUnit, tmpTargetPos, tmpTargetVec, tmpTargetDir);

	const float heightDiff = (weaponMuzzlePos.y + tmpTargetVec.y) - owner->pos.y; // negative when target below owner
	float weaponRange = 0.0f; // range modified by heightDiff and cylinderTargeting

	if (targetUnit == NULL || cylinderTargeting < 0.01f) {
		// check range in a sphere (with extra radius <heightDiff * heightMod>)
		weaponRange = GetRange2D(heightDiff * heightMod);
	} else {
		// check range in a cylinder (with height <cylinderTargeting * range>)
		if ((cylinderTargeting * range) > (math::fabsf(heightDiff) * heightMod)) {
			weaponRange = GetRange2D(0.0f);
		}
	}

	if (tmpTargetVec.SqLength2D() >= (weaponRange * weaponRange))
		return false;

	// NOTE: mainDir is in unit-space
	const float3 targetNormDir = normalized? tmpTargetDir: tmpTargetDir.SafeNormalize();
	const float3 worldMainDir =
		owner->frontdir * mainDir.z +
		owner->rightdir * mainDir.x +
		owner->updir    * mainDir.y;

	return (CheckTargetAngleConstraint(targetNormDir, worldMainDir));
}


bool CWeapon::HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const
{
	float3 dir = pos - weaponMuzzlePos;

	const float length = dir.Length();
	const float spread = AccuracyExperience() + SprayAngleExperience();

	if (length == 0.0f)
		return true;

	dir /= length;

	// ground check
	if ((avoidFlags & Collision::NOGROUND) == 0) {
		// NOTE:
		//     ballistic weapons (Cannon / Missile icw. trajectoryHeight) do not call this,
		//     they use TrajectoryGroundCol with an external check for the NOGROUND flag
		CUnit* unit = NULL;
		CFeature* feature = NULL;

		const float gdst = TraceRay::TraceRay(weaponMuzzlePos, dir, length, ~Collision::NOGROUND, owner, unit, feature);
		const float3 gpos = weaponMuzzlePos + dir * gdst;

		// true iff ground does not block the ray of length <length> from <pos> along <dir>
		if ((gdst > 0.0f) && (gpos.SqDistance(pos) > Square(damageAreaOfEffect)))
			return false;
	}

	// friendly, neutral & feature check
	if (TraceRay::TestCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, avoidFlags, owner)) {
		return false;
	}

	return true;
}


bool CWeapon::TryTarget(CUnit* unit, bool userTarget) {
	const float3 errorPos = unit->GetErrorPos(owner->allyteam, true);
	const float errorScale = ( MoveErrorExperience() * GAME_SPEED * unit->speed.Length() );

	float3 tempTargetPos = errorPos + errorVector * errorScale;
	tempTargetPos.y = std::max(tempTargetPos.y, ground->GetApproximateHeight(tempTargetPos.x, tempTargetPos.z) + 2.0f);

	return TryTarget(tempTargetPos, userTarget, unit);
}

bool CWeapon::TryTargetRotate(CUnit* unit, bool userTarget) {
	const float3 errorPos = unit->GetErrorPos(owner->allyteam, true);
	const float errorScale = ( MoveErrorExperience() * GAME_SPEED * unit->speed.Length() );

	float3 tempTargetPos = errorPos + errorVector * errorScale;
	tempTargetPos.y = std::max(tempTargetPos.y, ground->GetApproximateHeight(tempTargetPos.x, tempTargetPos.z) + 2.0f);

	const short weaponHeading = GetHeadingFromVector(mainDir.x, mainDir.z);
	const short enemyHeading = GetHeadingFromVector(tempTargetPos.x - weaponPos.x, tempTargetPos.z - weaponPos.z);

	return TryTargetHeading(enemyHeading - weaponHeading, tempTargetPos, userTarget, unit);
}

bool CWeapon::TryTargetRotate(float3 pos, bool userTarget) {
	if (!userTarget && weaponDef->noAutoTarget) {
		return false;
	}
	if (weaponDef->interceptor || !weaponDef->canAttackGround) {
		return false;
	}

	if (!weaponDef->waterweapon) {
		pos.y = std::max(pos.y, 0.0f);
	}

	const short weaponHeading = GetHeadingFromVector(mainDir.x, mainDir.z);
	const short enemyHeading = GetHeadingFromVector(
		pos.x - weaponPos.x, pos.z - weaponPos.z);

	return TryTargetHeading(enemyHeading - weaponHeading, pos, userTarget, 0);
}

bool CWeapon::TryTargetHeading(short heading, float3 pos, bool userTarget, CUnit* unit) {
	const float3 tempfrontdir(owner->frontdir);
	const float3 temprightdir(owner->rightdir);
	const short tempHeading = owner->heading;

	owner->heading = heading;
	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->rightdir = owner->frontdir.cross(owner->updir);

	weaponPos = owner->pos +
		owner->frontdir * relWeaponPos.z +
		owner->updir    * relWeaponPos.y +
		owner->rightdir * relWeaponPos.x;
	weaponMuzzlePos = owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;

	const bool val = TryTarget(pos, userTarget, unit);

	owner->frontdir = tempfrontdir;
	owner->rightdir = temprightdir;
	owner->heading = tempHeading;

	weaponPos = owner->pos +
		owner->frontdir * relWeaponPos.z +
		owner->updir    * relWeaponPos.y +
		owner->rightdir * relWeaponPos.x;
	weaponMuzzlePos = owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;

	return val;

}

void CWeapon::Init()
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

	muzzleFlareSize = std::min(damageAreaOfEffect * 0.2f, std::min(1500.f, weaponDef->damages[0]) * 0.003f);

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
	tracefile << weaponDef->name.c_str() << " fire: ";
	tracefile << owner->pos.x << " " << owner->frontdir.x << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z;
	tracefile << sprayAngle << " " <<  " " << salvoError.x << " " << salvoError.z << " " << owner->limExperience << " " << projectileSpeed << "\n";
#endif
	FireImpl();

	if (fireSoundId > 0 && (!weaponDef->soundTrigger || salvoLeft == salvoSize - 1)) {
		Channels::Battle.PlaySample(fireSoundId, owner, fireSoundVolume);
	}
}

void CWeapon::UpdateInterceptTarget()
{
	targetType = Target_None;

	float minInterceptTargetDistSq = std::numeric_limits<float>::max();
	float curInterceptTargetDistSq = std::numeric_limits<float>::min();

	for (std::map<int, CWeaponProjectile*>::iterator pi = incomingProjectiles.begin(); pi != incomingProjectiles.end(); ++pi) {
		CWeaponProjectile* p = pi->second;

		// set by CWeaponProjectile's ctor when the interceptor fires
		if (p->IsBeingIntercepted())
			continue;
		if ((curInterceptTargetDistSq = (p->pos - weaponPos).SqLength()) >= minInterceptTargetDistSq)
			continue;

		minInterceptTargetDistSq = curInterceptTargetDistSq;

		// NOTE:
		//     <incomingProjectiles> is sorted by increasing projectile ID
		//     however projectiles launched later in time (which are still
		//     likely out of range) can be assigned *lower* ID's than older
		//     projectiles (which might be almost in range already), so if
		//     we already have an interception target we should not replace
		//     it unless another incoming projectile <p> is closer
		//
		//     this is still not optimal (closer projectiles should receive
		//     higher priority), so just always look for the overall closest
		// if ((interceptTarget != NULL) && ((p->pos - weaponPos).SqLength() >= (interceptTarget->pos - weaponPos).SqLength()))
		//     continue;

		// keep targetPos in sync with the incoming projectile's position
		interceptTarget = p;
		targetType = Target_Intercept;
		targetPos = p->pos + p->speed;
	}
}


ProjectileParams CWeapon::GetProjectileParams()
{
	ProjectileParams params;
	params.target = targetUnit;
	params.weaponDef = weaponDef;
	params.owner = owner;

	if (interceptTarget) {
		params.target = interceptTarget;
	}

	return params;
}


float CWeapon::GetRange2D(float yDiff) const
{
	const float root1 = range * range - yDiff * yDiff;
	if (root1 < 0) {
		return 0;
	} else {
		return math::sqrt(root1);
	}
}


void CWeapon::StopAttackingAllyTeam(int ally)
{
	if (targetUnit && targetUnit->allyteam == ally) {
		HoldFire();
	}
}

float CWeapon::ExperienceScale() const
{
	return (1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);
}

float CWeapon::MoveErrorExperience() const
{
	return weaponDef->targetMoveError*(1.0f - owner->limExperience);
}
