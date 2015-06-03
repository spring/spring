/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/creg/STL_Map.h"
#include "WeaponDefHandler.h"
#include "Weapon.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/Scripts/NullUnitScript.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/Cannon.h"
#include "Sim/Weapons/NoWeapon.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/Sync/SyncTracer.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Log/ILog.h"

CR_BIND_DERIVED(CWeapon, CObject, (NULL, NULL))

CR_REG_METADATA(CWeapon, (
	CR_MEMBER(owner),
	CR_MEMBER(aimFromPiece),
	CR_MEMBER(muzzlePiece),
	CR_MEMBER(range),
	CR_MEMBER(reloadTime),
	CR_MEMBER(reloadStatus),
	CR_MEMBER(salvoLeft),
	CR_MEMBER(salvoDelay),
	CR_MEMBER(salvoSize),
	CR_MEMBER(projectilesPerShot),
	CR_MEMBER(nextSalvo),
	CR_MEMBER(predict),
	CR_MEMBER(accuracyError),
	CR_MEMBER(projectileSpeed),
	CR_MEMBER(predictSpeedMod),
	CR_MEMBER(fireSoundId),
	CR_MEMBER(fireSoundVolume),
	CR_MEMBER(hasBlockShot),
	CR_MEMBER(hasTargetWeight),
	CR_MEMBER(angleGood),
	CR_MEMBER(avoidTarget),
	CR_MEMBER(onlyForward),
	CR_MEMBER(muzzleFlareSize),
	CR_MEMBER(doTargetGroundPos),
	CR_MEMBER(alreadyWarnedAboutMissingPieces),

	CR_MEMBER(badTargetCategory),
	CR_MEMBER(onlyTargetCategory),
	CR_MEMBER(incomingProjectiles),
	CR_MEMBER(weaponDef),
	CR_MEMBER(buildPercent),
	CR_MEMBER(numStockpiled),
	CR_MEMBER(numStockpileQued),
	CR_MEMBER(sprayAngle),

	CR_MEMBER(lastRequest),
	CR_MEMBER(lastTargetRetry),

	CR_MEMBER(slavedTo),
	CR_MEMBER(maxForwardAngleDif),
	CR_MEMBER(maxMainDirAngleDif),
	CR_MEMBER(heightBoostFactor),
	CR_MEMBER(avoidFlags),
	CR_MEMBER(collisionFlags),
	CR_MEMBER(fuelUsage),
	CR_MEMBER(weaponNum),

	CR_MEMBER(relAimFromPos),
	CR_MEMBER(aimFromPos),
	CR_MEMBER(relWeaponMuzzlePos),
	CR_MEMBER(weaponMuzzlePos),
	CR_MEMBER(weaponDir),
	CR_MEMBER(mainDir),
	CR_MEMBER(wantedDir),
	CR_MEMBER(lastRequestedDir),
	CR_MEMBER(salvoError),
	CR_MEMBER(errorVector),
	CR_MEMBER(errorVectorAdd),

	CR_MEMBER(currentTarget),
	CR_MEMBER(currentTargetPos)
))



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWeapon::CWeapon(CUnit* owner, const WeaponDef* def):
	owner(owner),
	weaponDef(def),
	weaponNum(-1),
	aimFromPiece(-1),
	muzzlePiece(-1),
	reloadTime(1),
	reloadStatus(0),
	range(1),
	projectileSpeed(1),
	accuracyError(0),
	sprayAngle(0),
	salvoDelay(0),
	salvoSize(1),
	projectilesPerShot(1),
	nextSalvo(0),
	salvoLeft(0),
	predict(0),
	predictSpeedMod(1),

	hasBlockShot(false),
	hasTargetWeight(false),
	angleGood(false),
	avoidTarget(false),
	onlyForward(false),
	doTargetGroundPos(false),
	alreadyWarnedAboutMissingPieces(false),
	badTargetCategory(0),
	onlyTargetCategory(0xffffffff),

	buildPercent(0),
	numStockpiled(0),
	numStockpileQued(0),

	lastRequest(0),
	lastTargetRetry(-100),

	slavedTo(NULL),
	maxForwardAngleDif(0.0f),
	maxMainDirAngleDif(-1.0f),
	heightBoostFactor(-1.f),
	avoidFlags(0),
	collisionFlags(0),
	fuelUsage(0),

	relAimFromPos(UpVector),
	aimFromPos(ZeroVector),
	relWeaponMuzzlePos(UpVector),
	weaponMuzzlePos(ZeroVector),
	weaponDir(ZeroVector),
	mainDir(FwdVector),
	wantedDir(UpVector),
	lastRequestedDir(-UpVector),
	salvoError(ZeroVector),
	errorVector(ZeroVector),
	errorVectorAdd(ZeroVector),
	muzzleFlareSize(1),
	fireSoundId(0),
	fireSoundVolume(0)
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

	UpdateWeaponPieces();
	UpdateWeaponVectors();
	hasBlockShot = owner->script->HasBlockShot(weaponNum);
	hasTargetWeight = owner->script->HasTargetWeight(weaponNum);
}


inline bool CWeapon::CobBlockShot() const
{
	if (!hasBlockShot) {
		return false;
	}
	return owner->script->BlockShot(weaponNum, currentTarget.unit, currentTarget.isUserTarget);
}


float CWeapon::TargetWeight(const CUnit* targetUnit) const
{
	return owner->script->TargetWeight(weaponNum, targetUnit);
}


void CWeapon::UpdateWeaponPieces(const bool updateAimFrom)
{
	// Call UnitScript
	muzzlePiece = owner->script->QueryWeapon(weaponNum);
	if (updateAimFrom) aimFromPiece = owner->script->AimFromWeapon(weaponNum);

	// Some UnitScripts only implement on of them
	const bool aimExists = owner->script->PieceExists(aimFromPiece);
	const bool muzExists = owner->script->PieceExists(muzzlePiece);
	if (aimExists && muzExists) {
		// everything fine
	} else
	if (!aimExists && muzExists) {
		aimFromPiece = muzzlePiece;
	} else
	if (aimExists && !muzExists) {
		muzzlePiece = aimFromPiece;
	} else
	if (!aimExists && !muzExists) {
		if (!alreadyWarnedAboutMissingPieces && (owner->script != &CNullUnitScript::value) && !weaponDef->isShield && (dynamic_cast<CNoWeapon*>(this) == nullptr)) {
			LOG_L(L_WARNING, "%s: weapon%i: Neither AimFromWeapon nor QueryWeapon defined or returned invalid pieceids", owner->unitDef->name.c_str(), weaponNum);
			alreadyWarnedAboutMissingPieces = true;
		}
		aimFromPiece = -1;
		muzzlePiece = -1;
	}
}


void CWeapon::UpdateWeaponVectors()
{
	relAimFromPos = owner->script->GetPiecePos(aimFromPiece);
	owner->script->GetEmitDirPos(muzzlePiece, relWeaponMuzzlePos, weaponDir);

	aimFromPos = owner->GetObjectSpacePos(relAimFromPos);
	weaponMuzzlePos = owner->GetObjectSpacePos(relWeaponMuzzlePos);
	weaponDir = owner->GetObjectSpaceVec(weaponDir).SafeNormalize();

	// hope that we are underground because we are a popup weapon and will come above ground later
	if (aimFromPos.y < CGround::GetHeightReal(aimFromPos.x, aimFromPos.z)) {
		aimFromPos = owner->pos + UpVector * 10;
	}
}


void CWeapon::UpdateWantedDir()
{
	if (!onlyForward) {
		wantedDir = (currentTargetPos - aimFromPos).SafeNormalize();
	} else {
		wantedDir = owner->frontdir;
	}
}


float CWeapon::GetPredictFactor(float3 p) const
{
	return (p - aimFromPos).Length() / projectileSpeed;
}


void CWeapon::Update()
{
	predict = GetPredictFactor(GetTargetPos(currentTarget));
	predict = Clamp(predict, 0.f, 50000.0f);
	errorVector += errorVectorAdd;

	UpdateWeaponVectors();
	currentTargetPos = GetLeadTargetPos(currentTarget);

	if (!UpdateStockpile())
		return;

	UpdateAim();
	UpdateFire();
	UpdateSalvo();
#ifdef TRACE_SYNC
	tracefile << __FUNCTION__;
	tracefile << aimFromPos.x << " " << aimFromPos.y << " " << aimFromPos.z << " " << currentTargetPos.x << " " << currentTargetPos.y << " " << currentTargetPos.z << "\n";
#endif
}


void CWeapon::UpdateAim()
{
	if (!HaveTarget())
		return;

	UpdateWantedDir();

	// Check fire angle constraints
	const float3 worldTargetDir = (currentTargetPos - owner->pos).SafeNormalize();
	const float3 worldMainDir = owner->GetObjectSpaceVec(mainDir);
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

	// reaim weapon when needed
	ReAimWeapon();
}


void CWeapon::ReAimWeapon()
{
	// NOTE:
	//   let scripts do active aiming even if we are an onlyForward weapon
	//   (reduces how far the entire unit must turn to face worldTargetDir)

	bool reAim = false;

	// periodically re-aim the weapon (by calling the script's AimWeapon
	// every N=15 frames regardless of current angleGood state)
	// if it does not (eg. because AimWeapon always spawns a thread to
	// aim the weapon and defers setting angleGood to it) then this can
	// lead to irregular/stuttering firing behavior, even in scenarios
	// when the weapon does not have to re-aim
	reAim |= (gs->frameNum >= (lastRequest + (GAME_SPEED >> 1)));

	// check max FireAngle
	reAim |= (wantedDir.dot(lastRequestedDir) <= weaponDef->maxFireAngle);
	reAim |= (wantedDir.dot(lastRequestedDir) <= math::cos(20.f));

	//note: angleGood checks unit/maindir, not the weapon's current aim dir!!!
	//reAim |= (!angleGood);

	if (!reAim)
		return;

	// if we should block further fireing till AimWeapon() has finished
	if (!weaponDef->allowNonBlockingAim)
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


bool CWeapon::CanFire(bool ignoreAngleGood, bool ignoreTargetType, bool ignoreRequestedDir) const
{
	if (!ignoreAngleGood && !angleGood)
		return false;

	if ((salvoLeft > 0) || (nextSalvo > gs->frameNum))
		return false;

	if (!ignoreTargetType && !HaveTarget())
		return false;

	if (reloadStatus > gs->frameNum)
		return false;

	if (weaponDef->stockpile && numStockpiled == 0)
		return false;

	// muzzle is underwater but we cannot fire underwater
	if (!weaponDef->fireSubmersed && aimFromPos.y <= 0.0f)
		return false;

	// sanity check to force new aim
	if (weaponDef->maxFireAngle > -1.0f) {
		if (!ignoreRequestedDir && wantedDir.dot(lastRequestedDir) <= weaponDef->maxFireAngle)
			return false;
	}

	if ((fuelUsage > 0.0f) && (owner->currentFuel <= 0.0f))
		return false;

	// if in FPS mode, player must be pressing at least one button to fire
	const CPlayer* fpsPlayer = owner->fpsControlPlayer;
	if (fpsPlayer != NULL && !fpsPlayer->fpsController.mouse1 && !fpsPlayer->fpsController.mouse2)
		return false;

	// FIXME: there is already CUnit::dontUseWeapons but only used by HoverAirMoveType when landed
	const AAirMoveType* airMoveType = dynamic_cast<AAirMoveType*>(owner->moveType);
	if (airMoveType != NULL && airMoveType->GetPadStatus() != AAirMoveType::PAD_STATUS_FLYING)
		return false;

	return true;
}

void CWeapon::UpdateFire()
{
	if (!CanFire(false, false, false))
		return;

	if (!TryTarget(currentTargetPos, currentTarget))
		return;

	// pre-check if we got enough resources (so CobBlockShot gets only called when really possible to shoot)
	auto shotRes = SResourcePack(weaponDef->metalcost, weaponDef->energycost);
	if (!weaponDef->stockpile && !owner->HaveResources(shotRes))
		return;

	if (CobBlockShot())
		return;

	if (!weaponDef->stockpile) {
		// use resource for shoot
		CTeam* ownerTeam = teamHandler->Team(owner->team);
		if (!owner->UseResources(shotRes)) {
			// not enough resource, update pull (needs factor cause called each ::Update() and not at reloadtime!)
			const int minPeriod = std::max(1, (int)(reloadTime / owner->reloadSpeed));
			const float averageFactor = 1.0f / minPeriod;
			ownerTeam->resPull.energy += (averageFactor * weaponDef->energycost);
			ownerTeam->resPull.metal  += (averageFactor * weaponDef->metalcost);
			return;
		}
		ownerTeam->resPull += shotRes;
		owner->currentFuel = std::max(0.0f, owner->currentFuel - fuelUsage);
	} else {
		const int oldCount = numStockpiled;
		numStockpiled--;
		owner->commandAI->StockpileChanged(this);
		eventHandler.StockpileChanged(owner, this, oldCount);
	}

	reloadStatus = gs->frameNum + int(reloadTime / owner->reloadSpeed);

	salvoLeft = salvoSize;
	nextSalvo = gs->frameNum;
	salvoError = gs->randVector() * (owner->IsMoving()? weaponDef->movingAccuracy: accuracyError);

	if (currentTarget.type == Target_Pos || (currentTarget.type == Target_Unit && !(currentTarget.unit->losStatus[owner->allyteam] & LOS_INLOS))) {
		// area firing stuff is too effective at radar firing...
		salvoError *= 1.3f;
	}

	owner->lastMuzzleFlameSize = muzzleFlareSize;
	owner->lastMuzzleFlameDir = wantedDir;
	owner->script->FireWeapon(weaponNum);
}


bool CWeapon::UpdateStockpile()
{
	if (!weaponDef->stockpile)
		return true;

	if (numStockpileQued > 0) {
		const float p = 1.0f / weaponDef->stockpileTime;
		auto res = SResourcePack(weaponDef->metalcost * p, weaponDef->energycost * p);

		if (owner->UseResources(res)) {
			buildPercent += p;
		} else {
			// update the energy and metal required counts
			teamHandler->Team(owner->team)->resPull += res;
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

	return (numStockpiled > 0) || (salvoLeft > 0);
}


void CWeapon::UpdateSalvo()
{
	if (!salvoLeft || nextSalvo > gs->frameNum)
		return;

	salvoLeft--;
	nextSalvo = gs->frameNum + salvoDelay;

	// Decloak
	if ((owner->scriptCloak <= 2) && owner->unitDef->decloakOnFire) {
		if (owner->isCloaked) {
			owner->isCloaked = false;
			eventHandler.UnitDecloaked(owner);
		}
		owner->curCloakTimeout = gs->frameNum + owner->cloakTimeout;
	}

	for (int i = 0; i < projectilesPerShot; ++i) {
		owner->script->Shot(weaponNum);

		Fire(false);

		// Update Muzzle Piece/Pos
		UpdateWeaponPieces(false); // calls script->QueryWeapon()
		UpdateWeaponVectors();
	}

	// Rock the unit in the direction of fire
	if (owner->script->HasRockUnit()) {
		const float3 rockDir = (-wantedDir).SafeNormalize2D();
		owner->script->RockUnit(rockDir);
	}

	const bool searchForNewTarget = (salvoLeft == 0) && (currentTarget == owner->curTarget);
	owner->commandAI->WeaponFired(this, searchForNewTarget);

	if (salvoLeft == 0) {
		owner->script->EndBurst(weaponNum);
	}
}


bool CWeapon::Attack(const SWeaponTarget& newTarget)
{
	if (newTarget == currentTarget)
		return true;

	UpdateWeaponVectors();

	switch (newTarget.type) {
		case Target_None: {
			SetAttackTarget(newTarget);
			return true;
		} break;
		case Target_Unit:
		case Target_Pos:
		case Target_Intercept: {
			if (!TryTarget(newTarget))
				return false;

			SetAttackTarget(newTarget);
			avoidTarget = false;
			return true;
		} break;
	};
	return false;
}


void CWeapon::SetAttackTarget(const SWeaponTarget& newTarget)
{
	if (newTarget == currentTarget)
		return;

	DropCurrentTarget();
	currentTarget = newTarget;
	if (newTarget.type == Target_Unit) {
		AddDeathDependence(newTarget.unit, DEPENDENCE_TARGETUNIT);
	}

	currentTargetPos = GetLeadTargetPos(newTarget);
	UpdateWantedDir();
}


void CWeapon::DropCurrentTarget()
{
	if (currentTarget.type == Target_Unit) {
		DeleteDeathDependence(currentTarget.unit, DEPENDENCE_TARGETUNIT);
	}
	currentTarget = SWeaponTarget();
}


bool CWeapon::AllowWeaponAutoTarget() const
{
	const int checkAllowed = eventHandler.AllowWeaponTargetCheck(owner->id, weaponNum, weaponDef->id);
	if (checkAllowed >= 0) {
		return checkAllowed;
	}

	if (weaponDef->noAutoTarget)                 { return false; }
	if (owner->fireState < FIRESTATE_FIREATWILL) { return false; }
	if (slavedTo != NULL) { return false; }

	// if CAI has an auto-generated attack order, do not interfere
	if (!owner->commandAI->CanWeaponAutoTarget()) { return false; }

	if (!HaveTarget()) { return true; }
	if (avoidTarget)   { return true; }

	if (currentTarget.type == Target_Unit) {
		if (!TryTarget(SWeaponTarget(currentTarget.unit, currentTarget.isUserTarget))) {
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
		if (!currentTarget.isUserTarget) {
			if (currentTarget.unit->category & badTargetCategory) {
				return true;
			}
		}
	}

	if (currentTarget.isUserTarget) { return false; }

	if (gs->frameNum > (lastTargetRetry + 65)) {
		return true;
	}

	return false;
}

void CWeapon::AutoTarget()
{
	// 1. return fire at our last attacker if allowed
	if ((owner->lastAttacker != nullptr) && ((owner->lastAttackFrame + 200) <= gs->frameNum)) {
		if (Attack(SWeaponTarget(owner->lastAttacker)))
			return;
	}

	// 2. search for other in range targets
	lastTargetRetry = gs->frameNum;

	std::multimap<float, CUnit*> targets;
	std::multimap<float, CUnit*>::const_iterator targetsIt;

	const CUnit* avoidUnit = (avoidTarget && currentTarget.type == Target_Unit) ? currentTarget.unit : nullptr;
	const CUnit* ignorUnit = (!avoidTarget && currentTarget.type == Target_Unit) ? currentTarget.unit : nullptr;

	// NOTE:
	//   sorts by INCREASING order of priority, so lower equals better
	//   <targets> can contain duplicates if a unit covers multiple quads
	//   <targets> is normally sorted such that all bad TC units are at the
	//   end, but Lua can mess with the ordering arbitrarily
	CGameHelper::GenerateWeaponTargets(this, avoidUnit, targets);

	CUnit* goodTargetUnit = nullptr;
	CUnit* badTargetUnit = nullptr;

	for (targetsIt = targets.begin(); targetsIt != targets.end(); ++targetsIt) {
		CUnit* unit = targetsIt->second;

		// save the "best" bad target in case we have no other
		// good targets (of higher priority) left in <targets>
		const bool isBadTarget = (unit->category & badTargetCategory);
		if (isBadTarget && (badTargetUnit != nullptr))
			continue;

		if (!TryTarget(SWeaponTarget(unit)))
			continue;

		if (unit->IsNeutral() && (owner->fireState < FIRESTATE_FIREATNEUTRAL))
			continue;

		if (unit == ignorUnit)
			continue;

		if (isBadTarget) {
			badTargetUnit = unit;
		} else {
			goodTargetUnit = unit;
			break;
		}
	}

	if (goodTargetUnit == nullptr)
		goodTargetUnit = badTargetUnit;

	if (goodTargetUnit) {
		// pick our new target
		SetAttackTarget(SWeaponTarget(goodTargetUnit));
	}
}


void CWeapon::SlowUpdate()
{
	SlowUpdate(false);
}

void CWeapon::SlowUpdate(bool noAutoTargetOverride)
{
	errorVectorAdd = (gs->randVector() - errorVector) * (1.0f / UNIT_SLOWUPDATE_RATE);
	predictSpeedMod = 1.0f + (gs->randFloat() - 0.5f) * 2 * (1.0f - owner->limExperience);

#ifdef TRACE_SYNC
	tracefile << "Weapon slow update: ";
	tracefile << owner->id << " " << weaponNum <<  "\n";
#endif

	UpdateWeaponPieces();
	UpdateWeaponVectors();

	// HoldFire: if Weapon Target isn't valid
	HoldIfTargetInvalid();

	// SlavedWeapon: Update Weapon Target
	if (slavedTo != NULL) {
		// clone targets from the weapon we are slaved to
		SetAttackTarget(slavedTo->currentTarget);
	} else
	if (weaponDef->interceptor) {
		// keep track of the closest projectile heading our way (if any)
		UpdateInterceptTarget();
	} else
	if (owner->curTarget.type != Target_None) {
		// If unit got an attack target, clone the job (independent of AutoTarget!)
		// Also do this unconditionally (owner's target always has priority over weapon one!)
		Attack(owner->curTarget);
	}

	// AutoTarget: Find new/better Target
	if (!noAutoTargetOverride && AllowWeaponAutoTarget()) {
		AutoTarget();
	}
}


void CWeapon::HoldIfTargetInvalid()
{
	if (!HaveTarget())
		return;

	if (!TryTarget(currentTarget)) {
		DropCurrentTarget();
		return;
	}
}


void CWeapon::DependentDied(CObject* o)
{
	if (o == currentTarget.unit) {
		DropCurrentTarget();
	}

	if (o == currentTarget.intercept) {
		DropCurrentTarget();
	}

	// NOTE: DependentDied is called from ~CObject-->Detach, object is just barely valid
	if (weaponDef->interceptor || weaponDef->isShield) {
		incomingProjectiles.erase(static_cast<CWeaponProjectile*>(o)->id);
	}
}


bool CWeapon::TargetUnderWater(const SWeaponTarget& target)
{
	switch (target.type) {
		case Target_None: return false;
		case Target_Unit: return target.unit->IsUnderWater();
		case Target_Pos:  return (target.groundPos.y < 0.0f); // consistent with CSolidObject::IsUnderWater (LT)
		case Target_Intercept: return (target.intercept->pos.y < 0.0f);
		default: return false;
	}
}


bool CWeapon::TargetInWater(const SWeaponTarget& target)
{
	switch (target.type) {
		case Target_None: return false;
		case Target_Unit: return target.unit->IsInWater();
		case Target_Pos:  return (target.groundPos.y <= 0.0f); // consistent with CSolidObject::IsInWater (LE)
		case Target_Intercept: return (target.intercept->pos.y <= 0.0f);
		default: return false;
	}
}


bool CWeapon::CheckTargetAngleConstraint(const float3 worldTargetDir, const float3 worldWeaponDir) const
{
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


float3 CWeapon::GetTargetBorderPos(
	const CUnit* targetUnit,
	const float3 rawTargetPos,
	const float3 rawTargetDir) const
{
	float3 targetBorderPos = rawTargetPos;

	if (weaponDef->targetBorder == 0.0f)
		return targetBorderPos;
	if (targetUnit == NULL)
		return targetBorderPos;
	if (rawTargetDir == ZeroVector)
		return targetBorderPos;

	const float tbScale = math::fabsf(weaponDef->targetBorder);

	CollisionVolume  tmpColVol = CollisionVolume(targetUnit->collisionVolume);
	CollisionQuery   tmpColQry;

	// test for "collision" with a temporarily volume
	// (scaled uniformly by the absolute target-border
	// factor)
	tmpColVol.RescaleAxes(float3(tbScale, tbScale, tbScale));
	tmpColVol.SetBoundingRadius();
	tmpColVol.SetUseContHitTest(false);

	if (CCollisionHandler::DetectHit(&tmpColVol, targetUnit, weaponMuzzlePos, ZeroVector, NULL)) { //FIXME use aimFromPos ?
		// our weapon muzzle is inside the target unit's volume
		targetBorderPos = weaponMuzzlePos;
	} else {
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
			targetBorderPos = (weaponDef->targetBorder > 0.0f) ? tmpColQry.GetIngressPos() : tmpColQry.GetEgressPos();
		}
	}

	return targetBorderPos;
}


bool CWeapon::TryTarget(const float3 tgtPos, const SWeaponTarget& trg) const
{
	assert(GetLeadTargetPos(trg).SqDistance(tgtPos) < Square(250.f));

	if (!TestTarget(tgtPos, trg))
		return false;

	if (!TestRange(tgtPos, trg))
		return false;

	//FIXME add a forcedUserTarget (a forced fire mode enabled with ctrl key or something) and skip the tests below then
	return HaveFreeLineOfFire(tgtPos, trg);
}


bool CWeapon::TestTarget(const float3 tgtPos, const SWeaponTarget& trg) const
{
	if ((trg.isManualFire != weaponDef->manualfire) && owner->unitDef->canManualFire)
		return false;

	if (!trg.isUserTarget && weaponDef->noAutoTarget)
		return false;

	switch (trg.type) {
		case Target_None: {
			return true;
		} break;
		case Target_Unit: {
			if (trg.unit == owner || trg.unit == nullptr)
				return false;
			if ((trg.unit->category & onlyTargetCategory) == 0)
				return false;
			if (trg.unit->isDead && modInfo.fireAtKilled == 0)
				return false;
			if (trg.unit->IsCrashing() && modInfo.fireAtCrashing == 0)
				return false;
			if ((trg.unit->losStatus[owner->allyteam] & (LOS_INLOS | LOS_INRADAR)) == 0)
				return false;
			if (!trg.isUserTarget && trg.unit->IsNeutral() && owner->fireState < FIRESTATE_FIREATNEUTRAL)
				return false;

			// don't fire at allied targets
			if (!trg.isUserTarget && teamHandler->Ally(owner->allyteam, trg.unit->allyteam))
				return false;

			if (trg.unit->GetTransporter() != NULL) {
				if (!modInfo.targetableTransportedUnits)
					return false;
				// the transportee might be "hidden" below terrain, in which case we can't target it
				if (trg.unit->pos.y < CGround::GetHeightReal(trg.unit->pos.x, trg.unit->pos.z))
					return false;
			}
		} break;
		case Target_Pos: {
			if (!weaponDef->canAttackGround)
				return false;
		} break;
		case Target_Intercept: {
			//FIXME
			//if (weaponDef->interceptSolo && currentTarget.intercept->IsBeingIntercepted())
			//	return false;
			if (!weaponDef->interceptor)
				return false;
			if (!currentTarget.intercept->CanBeInterceptedBy(weaponDef))
				return false;
		} break;
		default: break;
	}

	// interceptor can only target projectiles!
	if (trg.type != Target_Intercept && weaponDef->interceptor)
		return false;

	// water weapon checks
	if (!weaponDef->waterweapon) {
		// we cannot pick targets underwater, check where target is in relation to us
		if (!owner->IsUnderWater() && TargetUnderWater(trg))
			return false;
		// if we are underwater but target is *not* in water, fireSubmersed gets checked
		if (owner->IsUnderWater() && TargetInWater(trg))
			return false;
	}

	return true;
}

bool CWeapon::TestRange(const float3 tgtPos, const SWeaponTarget& trg) const
{
	const float3 tmpTargetDir = (tgtPos - aimFromPos).SafeNormalize();

	const float heightDiff = tgtPos.y - owner->pos.y;
	float weaponRange = 0.0f; // range modified by heightDiff and cylinderTargeting

	if (trg.type == Target_Pos || weaponDef->cylinderTargeting < 0.01f) {
		// check range in a sphere (with extra radius <heightDiff * heightMod>)
		weaponRange = GetRange2D(heightDiff * weaponDef->heightmod);
	} else {
		// check range in a cylinder (with height <cylinderTargeting * range>)
		if ((weaponDef->cylinderTargeting * range) > (math::fabsf(heightDiff) * weaponDef->heightmod)) {
			weaponRange = GetRange2D(0.0f);
		}
	}

	if (aimFromPos.SqDistance2D(tgtPos) > (weaponRange * weaponRange))
		return false;

	// NOTE: mainDir is in unit-space
	const float3 worldMainDir = owner->GetObjectSpaceVec(mainDir);

	return (CheckTargetAngleConstraint(tmpTargetDir, worldMainDir));
}


bool CWeapon::HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg) const
{
	float3 dir = pos - aimFromPos;

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

		const float gdst = TraceRay::TraceRay(aimFromPos, dir, length, ~Collision::NOGROUND, owner, unit, feature);
		const float3 gpos = aimFromPos + dir * gdst;

		// true iff ground does not block the ray of length <length> from <pos> along <dir>
		if ((gdst > 0.0f) && (gpos.SqDistance(pos) > Square(weaponDef->damageAreaOfEffect)))
			return false;
	}

	// friendly, neutral & feature check
	if (TraceRay::TestCone(aimFromPos, dir, length, spread, owner->allyteam, avoidFlags, owner)) {
		return false;
	}

	return true;
}


bool CWeapon::TryTarget(const SWeaponTarget& trg) const {
	return TryTarget(GetLeadTargetPos(trg), trg);
}


bool CWeapon::TryTargetRotate(const CUnit* unit, bool userTarget)
{
	const float3 tempTargetPos = GetUnitLeadTargetPos(unit);
	const short weaponHeading = GetHeadingFromVector(mainDir.x, mainDir.z);
	const short enemyHeading = GetHeadingFromVector(tempTargetPos.x - aimFromPos.x, tempTargetPos.z - aimFromPos.z);

	return TryTargetHeading(enemyHeading - weaponHeading, SWeaponTarget(unit, userTarget));
}


bool CWeapon::TryTargetRotate(float3 pos, bool userTarget)
{
	AdjustTargetPosToWater(pos, true);
	const short weaponHeading = GetHeadingFromVector(mainDir.x, mainDir.z);
	const short enemyHeading = GetHeadingFromVector(pos.x - aimFromPos.x, pos.z - aimFromPos.z);

	return TryTargetHeading(enemyHeading - weaponHeading, SWeaponTarget(pos, userTarget));
}


bool CWeapon::TryTargetHeading(short heading, const SWeaponTarget& trg)
{
	const float3 tempfrontdir(owner->frontdir);
	const float3 temprightdir(owner->rightdir);
	const short tempHeading = owner->heading;

	owner->heading = heading;
	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->rightdir = owner->frontdir.cross(owner->updir);
	UpdateWeaponVectors();

	const bool val = TryTarget(trg);

	owner->frontdir = tempfrontdir;
	owner->rightdir = temprightdir;
	owner->heading = tempHeading;
	UpdateWeaponVectors();

	return val;
}


void CWeapon::Init()
{
	UpdateWeaponPieces();
	UpdateWeaponVectors();

	muzzleFlareSize = std::min(weaponDef->damageAreaOfEffect * 0.2f, std::min(1500.f, weaponDef->damages[0]) * 0.003f);

	if (weaponDef->interceptor)
		interceptHandler.AddInterceptorWeapon(this);

	if (weaponDef->stockpile) {
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


void CWeapon::Fire(bool scriptCall)
{
#ifdef TRACE_SYNC
	tracefile << weaponDef->name.c_str() << " fire: ";
	tracefile << owner->pos.x << " " << owner->frontdir.x << " " << currentTargetPos.x << " " << currentTargetPos.y << " " << currentTargetPos.z;
	tracefile << sprayAngle << " " <<  " " << salvoError.x << " " << salvoError.z << " " << owner->limExperience << " " << projectileSpeed << "\n";
#endif
	owner->lastFireWeapon = gs->frameNum;

	FireImpl(scriptCall);

	if (fireSoundId > 0 && (!weaponDef->soundTrigger || salvoLeft == salvoSize - 1)) {
		Channels::Battle->PlaySample(fireSoundId, owner, fireSoundVolume);
	}
}


void CWeapon::UpdateInterceptTarget()
{
	CWeaponProjectile* newTarget = nullptr;
	float minInterceptTargetDistSq = std::numeric_limits<float>::max();
	if (currentTarget.type == Target_Intercept) {
		minInterceptTargetDistSq = aimFromPos.SqDistance(currentTarget.intercept->pos);
	}

	for (std::map<int, CWeaponProjectile*>::iterator pi = incomingProjectiles.begin(); pi != incomingProjectiles.end(); ++pi) {
		CWeaponProjectile* p = pi->second;
		const float curInterceptTargetDistSq = aimFromPos.SqDistance(p->pos);

		// set by CWeaponProjectile's ctor when the interceptor fires
		if (weaponDef->interceptSolo && p->IsBeingIntercepted()) //FIXME add bad target?
			continue;

		if (curInterceptTargetDistSq >= minInterceptTargetDistSq)
			continue;

		minInterceptTargetDistSq = curInterceptTargetDistSq;

		// trigger us to auto-fire at this incoming projectile
		// we do not really need to set targetPos here since it
		// will be read from params.target (GetProjectileParams)
		// when our subclass Fire()'s
		newTarget = p;
	}

	if (newTarget) {
		DropCurrentTarget();
		currentTarget = SWeaponTarget(newTarget);
	}
}


ProjectileParams CWeapon::GetProjectileParams()
{
	ProjectileParams params;
	params.weaponID = weaponNum;
	params.owner = owner;
	params.weaponDef = weaponDef;

	switch (currentTarget.type) {
		case Target_None: { } break;
		case Target_Unit: { params.target = currentTarget.unit; } break;
		case Target_Pos:  { } break;
		case Target_Intercept: { params.target = currentTarget.intercept; } break;
	}

	return params;
}


float CWeapon::GetRange2D(const float yDiff) const
{
	const float root1 = range * range - yDiff * yDiff;
	return (root1 > 0.0f) ? math::sqrt(root1) : 0.0f;
}


void CWeapon::StopAttackingAllyTeam(const int ally)
{
	if ((currentTarget.type == Target_Unit) && currentTarget.unit->allyteam == ally) {
		DropCurrentTarget();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE:
//   GUIHandler places (some) user ground-attack orders on the
//   water surface, others on the ocean floor and in both cases
//   without examining weapon abilities (its logic is "obtuse")
//
//   this inconsistency would be hard(er) to fix on the UI side
//   so we must adjust all such target positions in synced code
//
//   see also CommandAI::AdjustGroundAttackCommand
void CWeapon::AdjustTargetPosToWater(float3& tgtPos, bool attackGround) const
{
	if (!attackGround)
		return;

	tgtPos.y = std::max(tgtPos.y, CGround::GetHeightReal(tgtPos.x, tgtPos.z));
	tgtPos.y = std::max(tgtPos.y, tgtPos.y * weaponDef->waterweapon);

	// prevent range hax in FPS mode
	if (owner->UnderFirstPersonControl() && dynamic_cast<const CCannon*>(this)) {
		tgtPos.y = CGround::GetHeightAboveWater(tgtPos.x, tgtPos.z);
	}
}


float3 CWeapon::GetUnitPositionWithError(const CUnit* unit) const
{
	float3 errorPos = unit->GetErrorPos(owner->allyteam, true);
	if (doTargetGroundPos) errorPos -= unit->aimPos - unit->pos;
	const float errorScale = (MoveErrorExperience() * GAME_SPEED * unit->speed.w);
	return errorPos + errorVector * errorScale;
}


float3 CWeapon::GetUnitLeadTargetPos(const CUnit* unit) const
{
	const float3 tmpTargetPos = GetUnitPositionWithError(unit) + GetLeadVec(unit);
	const float3 tmpTargetDir = (tmpTargetPos - aimFromPos).SafeNormalize();

	float3 aimPos = GetTargetBorderPos(unit, tmpTargetPos, tmpTargetDir);

	// never target below terrain
	// never target below water if not a water-weapon
	aimPos.y = std::max(aimPos.y, CGround::GetApproximateHeight(aimPos.x, aimPos.z) + 2.0f);
	aimPos.y = std::max(aimPos.y, aimPos.y * weaponDef->waterweapon);

	return aimPos;
}


float3 CWeapon::GetLeadVec(const CUnit* unit) const
{
	float3 lead = unit->speed * predict * mix(predictSpeedMod, 1.0f, weaponDef->predictBoost);
	if (weaponDef->leadLimit >= 0.0f) {
		const float leadBonus = weaponDef->leadLimit + weaponDef->leadBonus * owner->experience;
		if (lead.SqLength() > Square(leadBonus))
			lead *= (leadBonus) / (lead.Length() + 0.01f);
	}
	return lead;
}


float CWeapon::ExperienceErrorScale() const
{
	// accuracy (error) is increased (decreased) with experience
	// scale is 1.0f - (limExperience * expAccWeight), such that
	// for weight=0 scale is 1 and for weight=1 scale is 1 - exp
	// (lower is better)
	//
	//   for accWeight=0.00 and {0.25, 0.50, 0.75, 1.0} exp, scale=(1.0 - {0.25*0.00, 0.5*0.00, 0.75*0.00, 1.0*0.00}) = {1.0000, 1.000, 1.0000, 1.00}
	//   for accWeight=0.25 and {0.25, 0.50, 0.75, 1.0} exp, scale=(1.0 - {0.25*0.25, 0.5*0.25, 0.75*0.25, 1.0*0.25}) = {0.9375, 0.875, 0.8125, 0.75}
	//   for accWeight=0.50 and {0.25, 0.50, 0.75, 1.0} exp, scale=(1.0 - {0.25*0.50, 0.5*0.50, 0.75*0.50, 1.0*0.50}) = {0.8750, 0.750, 0.6250, 0.50}
	//   for accWeight=1.00 and {0.25, 0.50, 0.75, 1.0} exp, scale=(1.0 - {0.25*1.00, 0.5*1.00, 0.75*1.00, 1.0*0.75}) = {0.7500, 0.500, 0.2500, 0.25}
	return (CUnit::ExperienceScale(owner->limExperience, weaponDef->ownerExpAccWeight));
}


float CWeapon::MoveErrorExperience() const
{
	return (ExperienceErrorScale() * weaponDef->targetMoveError);
}


float3 CWeapon::GetLeadTargetPos(const SWeaponTarget& target) const
{
	switch (target.type) {
		case Target_None:      return currentTargetPos;
		case Target_Unit:      return GetUnitLeadTargetPos(target.unit);
		case Target_Pos: {
			float3 p = target.groundPos;
			AdjustTargetPosToWater(p, true);
			return p;
		} break;
		case Target_Intercept: return target.intercept->pos + target.intercept->speed;
	}

	return currentTargetPos;
}


float3 CWeapon::GetTargetPos(const SWeaponTarget& target) const
{
	switch (target.type) {
		case Target_None:      return currentTargetPos;
		case Target_Unit:      return target.unit->pos;
		case Target_Pos:       return target.groundPos;
		case Target_Intercept: return target.intercept->pos;
	}

	return currentTargetPos;
}

